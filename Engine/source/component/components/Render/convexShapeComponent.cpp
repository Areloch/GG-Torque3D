//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------
#include "console/consoleTypes.h"
#include "component/components/Render/convexShapeComponent.h"
#include "core/util/safeDelete.h"
#include "console/consoleTypes.h"
#include "console/consoleObject.h"
#include "core/stream/bitStream.h"
#include "console/engineAPI.h"
#include "sim/netConnection.h"

#include "lighting/lightQuery.h"
#include "renderInstance/renderPassManager.h"
#include "gfx/gfxTransformSaver.h"
#include "gfx/primBuilder.h"
#include "gfx/gfxDrawUtil.h"

#include "materials/materialManager.h"
#include "materials/materialFeatureTypes.h"
#include "materials/baseMatInstance.h"
#include "collision/optimizedPolyList.h"

#include "core/bitVector.h"
#include "math/mathIO.h"
#include "math/mathUtils.h"

extern bool gEditingMission;

GFXImplementVertexFormat( ConvexShapeVert )
{
   addElement( "POSITION", GFXDeclType_Float3 );
   addElement( "COLOR", GFXDeclType_Color );
   addElement( "NORMAL", GFXDeclType_Float3 );
   addElement( "TANGENT", GFXDeclType_Float3 );
   addElement( "TEXCOORD", GFXDeclType_Float2, 0 );
};

static const U32 sgConvexFaceColorCount = 16;
static const ColorI sgConvexFaceColors[ sgConvexFaceColorCount ] = 
{
   ColorI( 239, 131, 201 ),
   ColorI( 124, 255, 69 ),
   ColorI( 255, 65, 77 ),
   ColorI( 33, 118, 235 ),
   ColorI( 114, 227, 110 ),
   ColorI( 197, 50, 237 ),
   ColorI( 236, 255, 255 ),
   ColorI( 139, 225, 192 ),
   ColorI( 215, 9, 65 ),
   ColorI( 249, 114, 93 ),
   ColorI( 255, 255, 90 ),
   ColorI( 93, 104, 97 ),
   ColorI( 255, 214, 192 ),
   ColorI( 122, 44, 198 ),
   ColorI( 137, 141, 194 ),
   ColorI( 164, 114, 43 )
};
//////////////////////////////////////////////////////////////////////////
// Constructor/Destructor
//////////////////////////////////////////////////////////////////////////

ConvexShapeComponent::ConvexShapeComponent()
{
	mNetFlags.set(Ghostable | ScopeAlways);

	mFriendlyName = "Convex Shape";
   mComponentType = "Render";

	mDescription = getDescriptionText("Renders a convex shape brush.");

	addComponentField("Launch Convex Editor", "Launches the convex shape editor tool.", "fieldButton", "", "");

	mNetworked = true;
}

ConvexShapeComponent::~ConvexShapeComponent()
{
   for(S32 i = 0;i < mFields.size();++i)
   {
      ComponentField &field = mFields[i];
      SAFE_DELETE_ARRAY(field.mFieldDescription);
   }

   SAFE_DELETE_ARRAY(mDescription);
}

IMPLEMENT_CO_NETOBJECT_V1(ConvexShapeComponent);

//////////////////////////////////////////////////////////////////////////
ComponentInstance *ConvexShapeComponent::createInstance()
{
   ConvexShapeComponentInstance *instance = new ConvexShapeComponentInstance(this);

   setupFields( instance );

   if(instance->registerObject())
      return instance;

   delete instance;
   return NULL;
}

//==========================================================================================
//==========================================================================================
ConvexShapeComponentInstance::ConvexShapeComponentInstance( Component *btemplate ) 
{
   mTemplate = btemplate;
   mOwner = NULL;

   mMaterialInst = NULL;

   mNetFlags.set(Ghostable | ScopeAlways);
}

ConvexShapeComponentInstance::~ConvexShapeComponentInstance()
{
}
IMPLEMENT_CO_NETOBJECT_V1(ConvexShapeComponentInstance);

bool ConvexShapeComponentInstance::onAdd()
{
   if(! Parent::onAdd())
      return false;

   return true;
}

void ConvexShapeComponentInstance::onRemove()
{
   Parent::onRemove();
}

//This is mostly a catch for situations where the behavior is re-added to the object and the like and we may need to force an update to the behavior
void ConvexShapeComponentInstance::onComponentAdd()
{
   Parent::onComponentAdd();

   _updateGeometry( true );
}

void ConvexShapeComponentInstance::onComponentRemove()
{
   Parent::onComponentRemove();
}

void ConvexShapeComponentInstance::initPersistFields()
{
   Parent::initPersistFields();

   addProtectedField( "surface", TypeRealString, NULL, &setSurface, &defaultProtectedGetFn, 
        "Do not modify, for internal use.", AbstractClassRep::FIELD_HideInInspectors );
}

void ConvexShapeComponentInstance::inspectPostApply()
{
   Parent::inspectPostApply();

   _updateGeometry( true );

   setMaskBits( UpdateMask );
}

bool ConvexShapeComponentInstance::setSurface( void *object, const char *index, const char *data )
{
	ConvexShapeComponentInstance *shape = static_cast< ConvexShapeComponentInstance* >( object );

	QuatF quat;
	Point3F pos;

	dSscanf( data, "%g %g %g %g %g %g %g", &quat.x, &quat.y, &quat.z, &quat.w, &pos.x, &pos.y, &pos.z );

	MatrixF surface;
	quat.setMatrix( &surface );
	surface.setPosition( pos );

	shape->mSurfaces.push_back( surface );   

	return false;
}

U32 ConvexShapeComponentInstance::packUpdate(NetConnection *con, U32 mask, BitStream *stream)
{
	U32 retMask = Parent::packUpdate(con, mask, stream);

	if ( stream->writeFlag( mask & UpdateMask ) )
	{
		stream->write( mMaterialName );
      
		U32 surfCount = mSurfaces.size();
		stream->writeInt( surfCount, 32 );

		for ( S32 i = 0; i < surfCount; i++ )    
		{
			QuatF quat( mSurfaces[i] );
			Point3F pos( mSurfaces[i].getPosition() );

			mathWrite( *stream, quat );
			mathWrite( *stream, pos );                    
		}
	}

	return retMask;
}

void ConvexShapeComponentInstance::unpackUpdate(NetConnection *con, BitStream *stream)
{
	Parent::unpackUpdate(con, stream);

	if ( stream->readFlag() ) // UpdateMask
	{
		stream->read( &mMaterialName );      

		if ( isProperlyAdded() )
			_updateMaterial();

		mSurfaces.clear();

		const U32 surfCount = stream->readInt( 32 );
		for ( S32 i = 0; i < surfCount; i++ )
		{
			mSurfaces.increment();
			MatrixF &mat = mSurfaces.last();

			QuatF quat;
			Point3F pos;

			mathRead( *stream, &quat );
			mathRead( *stream, &pos ); 

			quat.setMatrix( &mat );
			mat.setPosition( pos );
		}

		if ( isProperlyAdded() )
			_updateGeometry( true );
	}
}

void ConvexShapeComponentInstance::prepRenderImage( SceneRenderState *state )
{   
   /*
   if ( state->isDiffusePass() )
   {
      ObjectRenderInst *ri2 = state->getRenderPass()->allocInst<ObjectRenderInst>();
      ri2->renderDelegate.bind( this, &ConvexShapeComponentInstance::_renderDebug );
      ri2->type = RenderPassManager::RIT_Editor;
      state->getRenderPass()->addInst( ri2 );
   }
   */

   if ( mVertexBuffer.isNull() )
      return;

   // If we don't have a material instance after the override then 
   // we can skip rendering all together.
	BaseMatInstance *warnMatInst = MATMGR->getWarningMatInstance();
   BaseMatInstance *matInst = state->getOverrideMaterial( mMaterialInst ? mMaterialInst : warnMatInst );
   if ( !matInst && !warnMatInst )
      return;

   // Get a handy pointer to our RenderPassmanager
   RenderPassManager *renderPass = state->getRenderPass();

   // Allocate an MeshRenderInst so that we can submit it to the RenderPassManager
   MeshRenderInst *ri = renderPass->allocInst<MeshRenderInst>();

   // Set our RenderInst as a standard mesh render
   ri->type = RenderPassManager::RIT_Mesh;

   // Calculate our sorting point
   if ( state )
   {
      // Calculate our sort point manually.
      const Box3F& rBox = mOwner->getRenderWorldBox();
      ri->sortDistSq = rBox.getSqDistanceToPoint( state->getCameraPosition() );      
   } 
   else 
      ri->sortDistSq = 0.0f;

   // Set up our transforms
   MatrixF objectToWorld = mOwner->getRenderTransform();
   objectToWorld.scale( mOwner->getScale() );

   ri->objectToWorld = renderPass->allocUniqueXform( objectToWorld );
   ri->worldToCamera = renderPass->allocSharedXform(RenderPassManager::View);
   ri->projection    = renderPass->allocSharedXform(RenderPassManager::Projection);

	// If we need lights then set them up.
   if ( matInst->isForwardLit() )
   {
      LightQuery query;
      query.init( mOwner->getWorldSphere() );
		query.getLights( ri->lights, 8 );
   }

   // Make sure we have an up-to-date backbuffer in case
   // our Material would like to make use of it
   // NOTICE: SFXBB is removed and refraction is disabled!
   //ri->backBuffTex = GFX->getSfxBackBuffer();

   // Set our Material
   ri->matInst = matInst;
   if ( matInst->getMaterial()->isTranslucent() )
   {
      ri->translucentSort = true;
      ri->type = RenderPassManager::RIT_Translucent;
   }

   // Set up our vertex buffer and primitive buffer
   ri->vertBuff = &mVertexBuffer;
   ri->primBuff = &mPrimitiveBuffer;

   ri->prim = renderPass->allocPrim();
   ri->prim->type = GFXTriangleList;
   ri->prim->minIndex = 0;
   ri->prim->startIndex = 0;
   ri->prim->numPrimitives = mPrimCount;
   ri->prim->startVertex = 0;
   ri->prim->numVertices = mVertCount;

   // We sort by the material then vertex buffer.
   ri->defaultKey = matInst->getStateHint();
   ri->defaultKey2 = (U32)ri->vertBuff; // Not 64bit safe!

   // Submit our RenderInst to the RenderPassManager
   state->getRenderPass()->addInst( ri );
}

bool ConvexShapeComponentInstance::buildPolyList( PolyListContext context, AbstractPolyList *plist, const Box3F &box, const SphereF &sphere )
{
   if ( mGeometry.points.empty() )	
      return false;

   // If we're exporting deal with that first.
   if ( context == PLC_Export )
   {
      AssertFatal( dynamic_cast<OptimizedPolyList*>( plist ), "ConvexShapeComponentInstance::buildPolyList - Bad polylist for export!" );
      _export( (OptimizedPolyList*)plist, box, sphere );
      return true;
   }

   plist->setTransform( &mOwner->getObjToWorld(), mOwner->getScale() );
   plist->setObject( mOwner );


   // Add points...

   const Vector< Point3F > pointList = mGeometry.points;

   S32 base = plist->addPoint( pointList[0] );

   for ( S32 i = 1; i < pointList.size(); i++ )	
      plist->addPoint( pointList[i] );


   // Add Surfaces...

   const Vector< ConvexShapeComponentInstance::Face > faceList = mGeometry.faces;

   if(context == PLC_Navigation)
   {
      for(S32 i = 0; i < faceList.size(); i++)
      {
         const ConvexShapeComponentInstance::Face &face = faceList[i];

         S32 s = face.triangles.size();
         for(S32 j = 0; j < s; j++)
         {
            plist->begin(0, s*i + j);

            plist->plane(PlaneF(face.centroid, face.normal));

            plist->vertex(base + face.points[face.triangles[j].p0]);
            plist->vertex(base + face.points[face.triangles[j].p1]);
            plist->vertex(base + face.points[face.triangles[j].p2]);

            plist->end();
         }
      }
      return true;
   }

   for ( S32 i = 0; i < faceList.size(); i++ )
   {
      const ConvexShapeComponentInstance::Face &face = faceList[i];		

      plist->begin( 0, i );

      plist->plane( PlaneF( face.centroid, face.normal ) );

      for ( S32 j = 0; j < face.triangles.size(); j++ )
      {
         plist->vertex( base + face.points[ face.triangles[j].p0 ] );
         plist->vertex( base + face.points[ face.triangles[j].p1 ] );
         plist->vertex( base + face.points[ face.triangles[j].p2 ] );
      }      

      plist->end();
   }

   return true;
}

void ConvexShapeComponentInstance::_export( OptimizedPolyList *plist, const Box3F &box, const SphereF &sphere )
{
   BaseMatInstance *matInst = mMaterialInst;
   if ( isServerObject() && getClientObject() )
      matInst = dynamic_cast<ConvexShapeComponentInstance*>(getClientObject())->mMaterialInst;
   
   MatrixF saveMat;
   Point3F saveScale;
   plist->getTransform( &saveMat, &saveScale );

   plist->setTransform( &mOwner->getObjToWorld(), mOwner->getScale() );
   plist->setObject( mOwner );   

   const Vector< Face > faceList = mGeometry.faces;
   const Vector< Point3F > &pointList = mGeometry.points;

   for ( S32 i = 0; i < faceList.size(); i++ )
   {
      const Face &face = faceList[i];		

      plist->begin( matInst, i, OptimizedPolyList::TriangleList );

      plist->plane( PlaneF( face.centroid, face.normal ) );

      for ( S32 j = 0; j < face.triangles.size(); j++ )
      {                  
         for ( S32 k = 0; k < 3; k++ )         
         {
            U32 vertId = face.triangles[j][k];
            plist->vertex( pointList[ face.points[ vertId ] ], face.normal, face.texcoords[ vertId ] );         
         }
      }      

      plist->end();
   }

   plist->setTransform( &saveMat, saveScale );
}

MatrixF ConvexShapeComponentInstance::getSurfaceWorldMat( S32 surfId, bool scaled ) const
{
   if ( surfId < 0 || surfId >= mSurfaces.size() )
      return MatrixF::Identity;      

   MatrixF objToWorld( mOwner->getObjToWorld() );

   if ( scaled )
      objToWorld.scale( mOwner->getScale() );

   MatrixF surfMat;
   surfMat.mul( objToWorld, mSurfaces[surfId] );   

   return surfMat;
}

void ConvexShapeComponentInstance::resizePlanes( const Point3F &size )
{
   //Point3F nSize;
   //mWorldToObj.mulV( nSize );

   for ( S32 i = 0; i < mSurfaces.size(); i++ )
   {
      MatrixF objToPlane( mSurfaces[i] );
      objToPlane.inverse();

      Point3F lim;
      objToPlane.mulV( size, &lim );

      F32 sign = ( mPlanes[i].d > 0.0f ) ? 1.0f : -1.0f;
      mPlanes[i].d = mFabs(lim.z) * 0.5f * sign;
      
      //mPlanes[i].d = -lim.z * 0.5f;      

      mSurfaces[i].setPosition( mPlanes[i].getPosition() );
   }   
}

void ConvexShapeComponentInstance::getSurfaceLineList( S32 surfId, Vector< Point3F > &lineList )
{
   if ( surfId < 0 || surfId > mSurfaces.size() - 1 )
      return;

   S32 faceId = -1;

   for ( S32 i = 0; i < mGeometry.faces.size(); i++ )
   {
      if ( mGeometry.faces[i].id == surfId )
      {
         faceId = i;
         break;
      }
   }

   if ( faceId == -1 )
      return;

   Face &face = mGeometry.faces[faceId];
   const Vector< Point3F > &pointList = mGeometry.points;

   if ( pointList.size() == 0 )
      return;

   for ( S32 i = 0; i < face.winding.size(); i++ )   
      lineList.push_back( pointList[ face.points[ face.winding[i] ] ] );
   
   lineList.push_back( pointList[ face.points[ face.winding.first() ] ] );
}

void ConvexShapeComponentInstance::_updateMaterial()
{   
   // If the material name matches then don't bother updating it.
   if ( mMaterialInst && mMaterialName.equal( mMaterialInst->getMaterial()->getName(), String::NoCase ) )
      return;

   SAFE_DELETE( mMaterialInst );

   Material *material;
   
   if ( !Sim::findObject( mMaterialName, material ) )
      Sim::findObject( "WarningMaterial", material );

   mMaterialInst = material->createMatInstance();

   //GFXStateBlockDesc desc;
   //desc.setCullMode( GFXCullNone );
   //desc.setBlend( false );

   //mMaterialInst->addStateBlockDesc( desc );

   FeatureSet features = MATMGR->getDefaultFeatures();
   //features.addFeature( MFT_DiffuseVertColor );

   mMaterialInst->init( features, getGFXVertexFormat<VertexType>() );

   if ( !mMaterialInst->isValid() )
   {
      SAFE_DELETE( mMaterialInst );
   }
}

void ConvexShapeComponentInstance::_updateGeometry( bool updateCollision )
{
   mPlanes.clear();

   for ( S32 i = 0; i < mSurfaces.size(); i++ )   
      mPlanes.push_back( PlaneF( mSurfaces[i].getPosition(), mSurfaces[i].getUpVector() ) );

	Vector< Point3F > tangents;
	for ( S32 i = 0; i < mSurfaces.size(); i++ )
		tangents.push_back( mSurfaces[i].getRightVector() );
   
   mGeometry.generate( mPlanes, tangents );

   AssertFatal( mGeometry.faces.size() <= mSurfaces.size(), "Got more faces than planes?" );

   const Vector< ConvexShapeComponentInstance::Face > &faceList = mGeometry.faces;
   const Vector< Point3F > &pointList = mGeometry.points;

   // Reset our surface center points.

   for ( S32 i = 0; i < faceList.size(); i++ )
		mSurfaces[ faceList[i].id ].setPosition( faceList[i].centroid );

   mPlanes.clear();

   for ( S32 i = 0; i < mSurfaces.size(); i++ )   
      mPlanes.push_back( PlaneF( mSurfaces[i].getPosition(), mSurfaces[i].getUpVector() ) );

   // Update bounding box.   
   updateBounds( false );

	mVertexBuffer = NULL;
	mPrimitiveBuffer = NULL;
	mVertCount = 0;
	mPrimCount = 0;

   // Server does not need to generate vertex/prim buffers.
   if ( isServerObject() )
      return;

   if ( faceList.empty() )   
      return;


	// Get total vert and prim count.

	for ( S32 i = 0; i < faceList.size(); i++ )	
	{
		U32 count = faceList[i].triangles.size();
		mPrimCount += count;
		mVertCount += count * 3;		
	}

	// Allocate VB and copy in data.

	mVertexBuffer.set( GFX, mVertCount, GFXBufferTypeStatic );
	VertexType *pVert = mVertexBuffer.lock();

	for ( S32 i = 0; i < faceList.size(); i++ )
	{
		const ConvexShapeComponentInstance::Face &face = faceList[i];
		const Vector< U32 > &facePntMap = face.points;
		const Vector< ConvexShapeComponentInstance::Triangle > &triangles = face.triangles;
		const ColorI &faceColor = sgConvexFaceColors[ i % sgConvexFaceColorCount ];

		const Point3F binormal = mCross( face.normal, face.tangent );

		for ( S32 j = 0; j < triangles.size(); j++ )
		{
			for ( S32 k = 0; k < 3; k++ )
			{
				pVert->normal = face.normal;
				pVert->tangent = face.tangent;
				pVert->color = faceColor;			
				pVert->point = pointList[ facePntMap[ triangles[j][k] ] ];
				pVert->texCoord = face.texcoords[ triangles[j][k] ];

				pVert++;
			}
		}		
	}	

	mVertexBuffer.unlock();

	// Allocate PB

   mPrimitiveBuffer.set( GFX, mPrimCount * 3, mPrimCount, GFXBufferTypeStatic );

   U16 *pIndex;
   mPrimitiveBuffer.lock( &pIndex );

   for ( U16 i = 0; i < mPrimCount * 3; i++ )
   {
      *pIndex = i;
      pIndex++;
   }

   mPrimitiveBuffer.unlock();
}

void ConvexShapeComponentInstance::updateBounds( bool recenter )
{
   if ( mGeometry.points.size() == 0 )
      return;

   Vector<Point3F> &pointListOS = mGeometry.points;
   U32 pointCount = pointListOS.size();

   Point3F volumnCenter( 0,0,0 );
   F32 areaSum = 0.0f;

   F32 faceCount = mGeometry.faces.size();

   for ( S32 i = 0; i < faceCount; i++ )   
   {
      volumnCenter += mGeometry.faces[i].centroid * mGeometry.faces[i].area;         
      areaSum += mGeometry.faces[i].area;
   }

   if ( areaSum == 0.0f )
      return;

   volumnCenter /= areaSum;
   
   Box3F newBnds;
  
   newBnds.minExtents = newBnds.maxExtents = Point3F::Zero;
   newBnds.setCenter( volumnCenter );

   for ( S32 i = 0; i < pointCount; i++ )      
      newBnds.extend( pointListOS[i] );

   mOwner->setObjectBox(newBnds);
}

void ConvexShapeComponentInstance::recenter()
{
   if ( mGeometry.points.size() == 0 )
      return;
  
   Point3F volCenterOS( 0,0,0 );
   F32 areaSum = 0.0f;

   F32 faceCount = mGeometry.faces.size();

   for ( S32 i = 0; i < faceCount; i++ )   
   {
      volCenterOS += mGeometry.faces[i].centroid * mGeometry.faces[i].area;         
      areaSum += mGeometry.faces[i].area;
   }

   volCenterOS /= areaSum;

   for ( S32 i = 0; i < mSurfaces.size(); i++ )   
      mSurfaces[i].setPosition( mSurfaces[i].getPosition() - volCenterOS );
   
   Point3F volCenterWS;
   MatrixF objToWorld( mOwner->getObjToWorld() );
   objToWorld.scale( mOwner->getScale() );
   objToWorld.mulP( volCenterOS, &volCenterWS );

   mOwner->setPosition( volCenterWS );

   _updateGeometry(true);   
}

void ConvexShapeComponentInstance::setTransform(MatrixF newTranf)
{
   if ( mGeometry.points.size() == 0 )
      return;
  
   Point3F volCenterOS( 0,0,0 );
   F32 areaSum = 0.0f;

   F32 faceCount = mGeometry.faces.size();

   for ( S32 i = 0; i < faceCount; i++ )   
   {
      volCenterOS += mGeometry.faces[i].centroid * mGeometry.faces[i].area;         
      areaSum += mGeometry.faces[i].area;
   }

   volCenterOS /= areaSum;

   for ( S32 i = 0; i < mSurfaces.size(); i++ )   
      mSurfaces[i].setPosition( mSurfaces[i].getPosition() - volCenterOS );
   
   Point3F volCenterWS;
   MatrixF objToWorld( mOwner->getObjToWorld() );
   objToWorld.scale( mOwner->getScale() );
   objToWorld.mulP( volCenterOS, &volCenterWS );
}

void ConvexShapeComponentInstance::getSurfaceTriangles( S32 surfId, Vector< Point3F > *outPoints, Vector< Point2F > *outCoords, bool worldSpace )
{
   S32 faceId = -1;
   for ( S32 i = 0; i < mGeometry.faces.size(); i++ )
   {
      if ( mGeometry.faces[i].id == surfId )
      {
         faceId = i;
         break;
      }
   }

   if ( faceId == -1 )
      return;

   const ConvexShapeComponentInstance::Face &face = mGeometry.faces[ faceId ];
   const Vector< Point3F > &pointList = mGeometry.points;

   const MatrixF &surfToObj = mSurfaces[ faceId ];
   MatrixF objToSurf( surfToObj );
   objToSurf.inverse();

   Point3F surfScale( 1.5f, 1.5f, 1.0f );

   for ( S32 i = 0; i < face.triangles.size(); i++ )
   {
      for ( S32 j = 0; j < 3; j++ )
      {
         Point3F pnt( pointList[ face.points[ face.triangles[i][j] ] ] );
         
         objToSurf.mulP( pnt );
         pnt *= surfScale;
         surfToObj.mulP( pnt );

         outPoints->push_back( pnt );

         if ( outCoords )
            outCoords->push_back( face.texcoords[ face.triangles[i][j] ] );
      }
   }

   if ( worldSpace )
   {
	   MatrixF objToWorld( mOwner->getObjToWorld() );
	   objToWorld.scale( mOwner->getScale() );

      for ( S32 i = 0; i < outPoints->size(); i++ )      
         objToWorld.mulP( (*outPoints)[i] );      
   }
}
void ConvexShapeComponentInstance::MeshGeometry::generate( const Vector< PlaneF > &planes, const Vector< Point3F > &tangents )
{
   PROFILE_SCOPE( ConvexComponent_Geometry_generate );

   points.clear();
   faces.clear();	

   AssertFatal( planes.size() == tangents.size(), "ConvexShape - incorrect plane/tangent count." );

#ifdef TORQUE_ENABLE_ASSERTS
   for ( S32 i = 0; i < planes.size(); i++ )
   {
      F32 dt = mDot( planes[i], tangents[i] );
      AssertFatal( mIsZero( dt, 0.0001f ), "ConvexShape - non perpendicular input vectors." );
      AssertFatal( planes[i].isUnitLength() && tangents[i].isUnitLength(), "ConvexShape - non unit length input vector." );
   }
#endif

   const U32 planeCount = planes.size();

   Point3F linePt, lineDir;   

   for ( S32 i = 0; i < planeCount; i++ )
   {      
      Vector< MathUtils::Line > collideLines;

      // Find the lines defined by the intersection of this plane with all others.

      for ( S32 j = 0; j < planeCount; j++ )
      {         
         if ( i == j )
            continue;

         if ( planes[i].intersect( planes[j], linePt, lineDir ) )
         {
            collideLines.increment();
            MathUtils::Line &line = collideLines.last();
            line.origin = linePt;
            line.direction = lineDir;   
         }         
      }

      if ( collideLines.empty() )
         continue;

      // Find edges and points defined by the intersection of these lines.
      // As we find them we fill them into our working ConvexShapeComponentInstance::Face
      // structure.
      
      Face newFace;

      for ( S32 j = 0; j < collideLines.size(); j++ )
      {
         Vector< Point3F > collidePoints;

         for ( S32 k = 0; k < collideLines.size(); k++ )
         {
            if ( j == k )
               continue;

            MathUtils::LineSegment segment;
            MathUtils::mShortestSegmentBetweenLines( collideLines[j], collideLines[k], &segment );

            F32 dist = ( segment.p0 - segment.p1 ).len();

            if ( dist < 0.0005f )
            {
               S32 l = 0;
               for ( ; l < planeCount; l++ )
               {
                  if ( planes[l].whichSide( segment.p0 ) == PlaneF::Front )
                     break;
               }

               if ( l == planeCount )
                  collidePoints.push_back( segment.p0 );
            }
         }

         //AssertFatal( collidePoints.size() <= 2, "A line can't collide with more than 2 other lines in a convex shape..." );

         if ( collidePoints.size() != 2 )
            continue;

         // Push back collision points into our points vector
         // if they are not duplicates and determine the id
         // index for those points to be used by Edge(s).    

         const Point3F &pnt0 = collidePoints[0];
         const Point3F &pnt1 = collidePoints[1];
         S32 idx0 = -1;
         S32 idx1 = -1;

         for ( S32 k = 0; k < points.size(); k++ )
         {
            if ( pnt0.equal( points[k] ) )
            {
               idx0 = k;
               break;
            }
         }

         for ( S32 k = 0; k < points.size(); k++ )
         {
            if ( pnt1.equal( points[k] ) )
            {
               idx1 = k;
               break;
            }
         }

         if ( idx0 == -1 )
         {
            points.push_back( pnt0 );               
            idx0 = points.size() - 1;
         }

         if ( idx1 == -1 )
         {
            points.push_back( pnt1 );
            idx1 = points.size() - 1;
         }

         // Construct the Face::Edge defined by this collision.

         S32 localIdx0 = newFace.points.push_back_unique( idx0 );
         S32 localIdx1 = newFace.points.push_back_unique( idx1 );

         newFace.edges.increment();
         ConvexShapeComponentInstance::Edge &newEdge = newFace.edges.last();
         newEdge.p0 = localIdx0;
         newEdge.p1 = localIdx1;
      }    

      if ( newFace.points.size() < 3 )
         continue;

      //AssertFatal( newFace.points.size() == newFace.edges.size(), "ConvexShape - face point count does not equal edge count." );


		// Fill in some basic Face information.

		newFace.id = i;
		newFace.normal = planes[i];
		newFace.tangent = tangents[i];


		// Make a working array of Point3Fs on this face.

		U32 pntCount = newFace.points.size();		
		Point3F *workPoints = new Point3F[ pntCount ];

		for ( S32 j = 0; j < pntCount; j++ )
			workPoints[j] = points[ newFace.points[j] ];


      // Calculate the average point for calculating winding order.

      Point3F averagePnt = Point3F::Zero;

		for ( S32 j = 0; j < pntCount; j++ )
			averagePnt += workPoints[j];

		averagePnt /= pntCount;		


		// Sort points in correct winding order.

		U32 *vertMap = new U32[pntCount];

      MatrixF quadMat( true );
      quadMat.setPosition( averagePnt );
      quadMat.setColumn( 0, newFace.tangent );
      quadMat.setColumn( 1, mCross( newFace.normal, newFace.tangent ) );
      quadMat.setColumn( 2, newFace.normal );
		quadMat.inverse();

      // Transform working points into quad space 
      // so we can work with them as 2D points.

      for ( S32 j = 0; j < pntCount; j++ )
         quadMat.mulP( workPoints[j] );

		MathUtils::sortQuadWindingOrder( true, workPoints, vertMap, pntCount );

      // Save points in winding order.

      for ( S32 j = 0; j < pntCount; j++ )
         newFace.winding.push_back( vertMap[j] );

      // Calculate the area and centroid of the face.

      newFace.area = 0.0f;
      for ( S32 j = 0; j < pntCount; j++ )
      {
         S32 k = ( j + 1 ) % pntCount;
         const Point3F &p0 = workPoints[ vertMap[j] ];
         const Point3F &p1 = workPoints[ vertMap[k] ];
         
         // Note that this calculation returns positive area for clockwise winding
         // and negative area for counterclockwise winding.
         newFace.area += p0.y * p1.x;
         newFace.area -= p0.x * p1.y;                  
      }

      //AssertFatal( newFace.area > 0.0f, "ConvexShape - face area was not positive." );
      if ( newFace.area > 0.0f )
         newFace.area /= 2.0f;      

      F32 factor;
      F32 cx = 0.0f, cy = 0.0f;
      
      for ( S32 j = 0; j < pntCount; j++ )
      {
         S32 k = ( j + 1 ) % pntCount;
         const Point3F &p0 = workPoints[ vertMap[j] ];
         const Point3F &p1 = workPoints[ vertMap[k] ];

         factor = p0.x * p1.y - p1.x * p0.y;
         cx += ( p0.x + p1.x ) * factor;
         cy += ( p0.y + p1.y ) * factor;
      }
      
      factor = 1.0f / ( newFace.area * 6.0f );
      newFace.centroid.set( cx * factor, cy * factor, 0.0f );
      quadMat.inverse();
      quadMat.mulP( newFace.centroid );

      delete [] workPoints;
      workPoints = NULL;

		// Make polygons / triangles for this face.

		const U32 polyCount = pntCount - 2;

		newFace.triangles.setSize( polyCount );

		for ( S32 j = 0; j < polyCount; j++ )
		{
			ConvexShapeComponentInstance::Triangle &poly = newFace.triangles[j];

			poly.p0 = vertMap[0];

			if ( j == 0 )
			{
				poly.p1 = vertMap[ 1 ];
				poly.p2 = vertMap[ 2 ];
			}
			else
			{
				poly.p1 = vertMap[ 1 + j ];
				poly.p2 = vertMap[ 2 + j ];
			}
		}

		delete [] vertMap;


		// Calculate texture coordinates for each point in this face.

		const Point3F binormal = mCross( newFace.normal, newFace.tangent );
		PlaneF planey( newFace.centroid - 0.5f * binormal, binormal );
		PlaneF planex( newFace.centroid - 0.5f * newFace.tangent, newFace.tangent );

		newFace.texcoords.setSize( newFace.points.size() );

		for ( S32 j = 0; j < newFace.points.size(); j++ )
		{
			F32 x = planex.distToPlane( points[ newFace.points[ j ] ] );
			F32 y = planey.distToPlane( points[ newFace.points[ j ] ] );

			newFace.texcoords[j].set( -x, -y );
		}

      // Data verification tests.
#ifdef TORQUE_ENABLE_ASSERTS
      //S32 triCount = newFace.triangles.size();
      //S32 edgeCount = newFace.edges.size();
      //AssertFatal( triCount == edgeCount - 2, "ConvexShape - triangle/edge count do not match." );

      /*
      for ( S32 j = 0; j < triCount; j++ )
      {
         F32 area = MathUtils::mTriangleArea( points[ newFace.points[ newFace.triangles[j][0] ] ], 
                                              points[ newFace.points[ newFace.triangles[j][1] ] ],
                                              points[ newFace.points[ newFace.triangles[j][2] ] ] );
         AssertFatal( area > 0.0f, "ConvexShape - triangle winding bad." );
      }*/
#endif


      // Done with this Face.
      
      faces.push_back( newFace );
   }
}

void ConvexShapeComponentInstance::renderFaceEdges( S32 faceid, const ColorI &color /*= ColorI::WHITE*/, F32 lineWidth /*= 1.0f */ )
{
   const Vector< ConvexShapeComponentInstance::Face > &faceList = mGeometry.faces;

   if ( faceid >= faceList.size() )
      return;

   GFXTransformSaver saver;
   MatrixF xfm( getOwner()->getObjToWorld() );
   xfm.scale( getOwner()->getScale() );
   GFX->multWorld( xfm );

   GFXStateBlockDesc desc;
   desc.setBlend( true );
   GFX->setStateBlockByDesc( desc );

   MatrixF projBias(true);
   const Frustum& frustum = GFX->getFrustum();
   MathUtils::getZBiasProjectionMatrix( 0.001f, frustum, &projBias );
   GFX->setProjectionMatrix( projBias );

   S32 s = faceid;
   S32 e = faceid + 1;

   if ( faceid == -1 )
   {
      s = 0;
      e = faceList.size();
   }

   for ( S32 i = s; i < e; i++ )
   {
      const Face &face = faceList[i];
      const Vector< Edge > &edgeList = face.edges;
      const Vector< U32 > &facePntList = face.points;
      const Vector< Point3F > &pointList = mGeometry.points;

      PrimBuild::begin( GFXLineList, edgeList.size() * 2 );

      PrimBuild::color( color );

      for ( S32 j = 0; j < edgeList.size(); j++ )         
      {
         PrimBuild::vertex3fv( pointList[ facePntList[ edgeList[j].p0 ] ] );
         PrimBuild::vertex3fv( pointList[ facePntList[ edgeList[j].p1 ] ] );
      }

      PrimBuild::end();
   }
}

//we only ever handle raycasts in the case of the editor being used.
//otherwise we'll always defer to our collision component
bool ConvexShapeComponentInstance::castRay( const Point3F &start, const Point3F &end, RayInfo *info )
{
   if(gEditingMission)
   {
	   if ( mPlanes.empty() )
		  return false;   

	   const Vector< PlaneF > &planeList = mPlanes;
	   const U32 planeCount = planeList.size();  

	   F32 t;
	   F32 tmin = F32_MAX;
	   S32 hitFace = -1;
	   Point3F hitPnt, pnt;
	   VectorF rayDir( end - start );
	   rayDir.normalizeSafe();

	   if ( false )
	   {
		  PlaneF plane( Point3F(0,0,0), Point3F(0,0,1) );
		  Point3F sp( 0,0,-1 );
		  Point3F ep( 0,0,1 );

		  F32 t = plane.intersect( sp, ep );
		  Point3F hitPnt;
		  hitPnt.interpolate( sp, ep, t );
	   }

	   for ( S32 i = 0; i < planeCount; i++ )
	   {
		  // Don't hit the back-side of planes.
		  if ( mDot( rayDir, planeList[i] ) >= 0.0f )
			 continue;

		  t = planeList[i].intersect( start, end );

		  if ( t >= 0.0f && t <= 1.0f && t < tmin )
		  {
			 pnt.interpolate( start, end, t );

			 S32 j = 0;
			 for ( ; j < planeCount; j++ )
			 {
				if ( i == j )
				   continue;

				F32 dist = planeList[j].distToPlane( pnt );
				if ( dist > 1.0e-004f )
				   break;
			 }

			 if ( j == planeCount )
			 {
				tmin = t;
				hitFace = i;
			 }
		  }
	   }

	   if ( hitFace == -1 )
		  return false;

	   info->face = hitFace;            
	   info->material = mMaterialInst;
	   info->normal = planeList[ hitFace ];
	   info->object = this->mOwner;
	   info->t = tmin;
	   info->userData = this;

	   //mObjToWorld.mulV( info->normal );

	   return true;
   }
}

void ConvexShapeComponentInstance::packToStream( Stream &stream, U32 tabStop, S32 behaviorID, U32 flags )
{
	Parent::packToStream(stream, tabStop, behaviorID, flags);

	//now to store out the surface data
	S32 count = mSurfaces.size();
   if ( count > smMaxSurfaces )
   {
       Con::errorf( "ConvexShape has too many surfaces to save! Truncated value %d to maximum value of %d", count, smMaxSurfaces );
       count = smMaxSurfaces;
   }

   for ( U32 i = 0; i < count; i++ )
   {      
      const MatrixF &mat = mSurfaces[i];

		QuatF quat( mat );
		Point3F pos( mat.getPosition() );

      stream.writeTabs(tabStop);

      char buffer[1024];
      dMemset( buffer, 0, 1024 );      
      
      dSprintf( buffer, 1024, "surface = \"%g %g %g %g %g %g %g\";", 
         quat.x, quat.y, quat.z, quat.w, pos.x, pos.y, pos.z );      

      stream.writeLine( (const U8*)buffer );
   }
}