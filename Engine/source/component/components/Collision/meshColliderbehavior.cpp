//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "component/components/Collision/meshColliderBehavior.h"

//#include "platform/platform.h"
#include "console/consoleTypes.h"
#include "core/util/safeDelete.h"
#include "core/resourceManager.h"
//#include "core/stream/fileStream.h"
#include "console/consoleTypes.h"
#include "console/consoleObject.h"
//#include "ts/tsShapeInstance.h"
#include "core/stream/bitStream.h"
#include "gfx/gfxTransformSaver.h"
#include "gfx/gfxDrawUtil.h"
//#include "gfx/primBuilder.h"
#include "console/engineAPI.h"
//#include "lighting/lightQuery.h"
#include "T3D/physics/physicsPlugin.h"
#include "T3D/physics/physicsBody.h"
#include "T3D/physics/physicsCollision.h"
#include "T3D/gameBase/gameConnection.h"
#include "collision/extrudedPolyList.h"
#include "math/mathIO.h"
#include "gfx/sim/debugDraw.h"

#include "opcode/Opcode.h"
#include "opcode/Ice/IceAABB.h"
#include "opcode/Ice/IcePoint.h"
#include "opcode/OPC_AABBTree.h"
#include "opcode/OPC_AABBCollider.h"

#include "component/components/Physics/physicsInterfaces.h"

//#include "math/mGeometry.h"

#include "component/components/render/renderInterfaces.h"

#include "collision/concretePolyList.h"

extern bool gEditingMission;

static bool sRenderMeshColliders = false;

static Point3F	texGenAxis[18] =
{
   Point3F(0,0,1), Point3F(1,0,0), Point3F(0,-1,0),
   Point3F(0,0,-1), Point3F(1,0,0), Point3F(0,1,0),
   Point3F(1,0,0), Point3F(0,1,0), Point3F(0,0,1),
   Point3F(-1,0,0), Point3F(0,1,0), Point3F(0,0,-1),
   Point3F(0,1,0), Point3F(1,0,0), Point3F(0,0,1),
   Point3F(0,-1,0), Point3F(-1,0,0), Point3F(0,0,-1)
};

//
MeshColliderBehavior::MeshColliderBehavior()
{
   mNetFlags.set(Ghostable | ScopeAlways);

   addComponentField("collisionMasks", "A mask list of all the collision masks that apply to this collider.", "collisionMask", "", "");

   addComponentField("collisionCheckMasks", "A mask list of all the collision masks that this collider checks against when testing collisions.", "collisionMask", "", "");

   addComponentField("blockCollisions", "If enabled, will stop colliding objects from passing through.", "bool", "1", "");

   mFriendlyName = "Mesh Collider";
   mComponentType = "Collision";

   mDescription = getDescriptionText("Enables collision against this entity's visible mesh.");

   mNetworked = true;
}

MeshColliderBehavior::~MeshColliderBehavior()
{
   for(S32 i = 0;i < mFields.size();++i)
   {
      ComponentField &field = mFields[i];
      SAFE_DELETE_ARRAY(field.mFieldDescription);
   }

   SAFE_DELETE_ARRAY(mDescription);
}

IMPLEMENT_CO_NETOBJECT_V1(MeshColliderBehavior);

//////////////////////////////////////////////////////////////////////////
ComponentInstance *MeshColliderBehavior::createInstance()
{
   MeshColliderBehaviorInstance *instance = new MeshColliderBehaviorInstance(this);

   setupFields( instance );

   if(instance->registerObject())
      return instance;

   delete instance;
   return NULL;
}

void MeshColliderBehavior::initPersistFields()
{
   Parent::initPersistFields();
}

//==========================================================================================
//==========================================================================================
MeshColliderBehaviorInstance::MeshColliderBehaviorInstance( Component *btemplate ) 
{
   mTemplate = btemplate;
   mOwner = NULL;

   mConvexList = NULL;
   mTimeoutList = NULL;

   mBlockColliding = true;

   mWorkingQueryBox.minExtents.set(-1e9f, -1e9f, -1e9f);
   mWorkingQueryBox.maxExtents.set(-1e9f, -1e9f, -1e9f);

   mNetFlags.set(Ghostable);

   CollisionMoveMask = ( TerrainObjectType     | PlayerObjectType  | 
      StaticShapeObjectType | VehicleObjectType |
      VehicleBlockerObjectType | DynamicShapeObjectType | EntityObjectType);

   mDebugRender.shapeInstance = NULL;
}

MeshColliderBehaviorInstance::~MeshColliderBehaviorInstance()
{
   delete mConvexList;
   mConvexList = NULL;
}
IMPLEMENT_CO_NETOBJECT_V1(MeshColliderBehaviorInstance);

bool MeshColliderBehaviorInstance::onAdd()
{
   if(! Parent::onAdd())
      return false;

   return true;
}

void MeshColliderBehaviorInstance::onRemove()
{
   if(mConvexList != NULL)
      mConvexList->nukeList();
   Parent::onRemove();
}

void MeshColliderBehaviorInstance::onComponentRemove()
{
   SAFE_DELETE( (dynamic_cast<Entity*>(mOwner))->mPhysicsRep );
   mOwner->disableCollision();

   Parent::onComponentRemove();
}

void MeshColliderBehaviorInstance::onComponentAdd()
{
   Parent::onComponentAdd();

   if(isServerObject())
      prepCollision();
}

void MeshColliderBehaviorInstance::initPersistFields()
{
   Parent::initPersistFields();

   addField("blockCollisions", TypeBool, Offset(mBlockColliding, MeshColliderBehaviorInstance), "");
}

void MeshColliderBehaviorInstance::consoleInit()
{
   Con::addVariable("$MeshCollider::renderCollision", TypeBool, &sRenderMeshColliders, 
      "@brief Determines if the mesh colliders collision mesh should be rendered.\n\n"
      "This is mainly used for the tools and debugging.\n"
	   "@ingroup GameObjects\n");
}

U32 MeshColliderBehaviorInstance::packUpdate(NetConnection *con, U32 mask, BitStream *stream)
{
   U32 retMask = Parent::packUpdate(con, mask, stream);

   return retMask;
}

void MeshColliderBehaviorInstance::unpackUpdate(NetConnection *con, BitStream *stream)
{
   Parent::unpackUpdate(con, stream);

   prepCollision();
}

TSShapeInstance* MeshColliderBehaviorInstance::getShapeInstance()
{
   TSShapeInstanceInterface* shapeInterface = mOwner->getComponent<TSShapeInstanceInterface>();

   if(!shapeInterface)
      return NULL;

   TSShapeInstance* shapeInstance = shapeInterface->getShapeInstance();
   if(shapeInstance)
      return shapeInstance;

   return NULL;
}

void MeshColliderBehaviorInstance::update()
{
   Parent::update();

   if(isServerObject())
      prepCollision();
}

void MeshColliderBehaviorInstance::prepCollision()
{
   if(!mOwner)
      return;

   //Check if our owner has any renderable TSShapes
   TSShapeInstance *shapeInstance = getShapeInstance();

   if( !shapeInstance )
      return;

   bool server = isServerObject();

   shapeInstance->prepCollision();

   // Cleanup any old collision data
   mCollisionDetails.clear();

   if(mConvexList != NULL)
      mConvexList->nukeList();

   Vector<S32> losDetails;

   shapeInstance->getShape()->findColDetails( true, &mCollisionDetails, &losDetails );

   if(!mCollisionDetails.empty())
   {
      mOwner->enableCollision();

      mConvexList = new MeshColliderPolysoupConvex;

      Convex *c = new Convex();
      for (U32 i = 0; i < mCollisionDetails.size(); i++)
         buildConvexOpcode( shapeInstance, mCollisionDetails[i], mOwner->getWorldBox(), c, mConvexList );
   }
   else
      mOwner->disableCollision();

   _updatePhysics();
}

void MeshColliderBehaviorInstance::processTick(const Move* move)
{
   mCollisionList.clear();

   if(VelocityInterface *vI = mOwner->getComponent<VelocityInterface>())
   {
      VectorF velocity = vI->getVelocity();

      //No need to update our working set if we don't have our convex or we're not moving
      if(mConvexList && mConvexList->getObject() && !velocity.isZero())
         updateWorkingCollisionSet((U32)-1);
   }
}

bool MeshColliderBehaviorInstance::checkCollisions( const F32 travelTime, Point3F *velocity, Point3F start )
{
   if(!mConvexList || !mConvexList->getObject())
      return false;

   Point3F end = start + *velocity * travelTime;

   VectorF vector = end - start;

   mCollisionList.clear();

   if (checkEarlyOut(start, *velocity, travelTime, mOwner->getObjBox(), mOwner->getScale(), mConvexList->getBoundingBox(), 0xFFFFFF, mConvexList->getWorkingList()))
      return false;

   bool collided = updateCollisions(travelTime, vector, *velocity);

   handleCollisionList(mCollisionList, *velocity);

   //Basically, if this is false, we still track collisions as normal, but we don't
   //stop the colliding objects, instead only firing off callbacks.
   //This lets use act like a trigger.
   bool mBlockColliding = true;
   if(mBlockColliding)
      return collided;
   else
      return false;
}

bool MeshColliderBehaviorInstance::buildPolyList(PolyListContext context, AbstractPolyList* polyList, const Box3F &, const SphereF &)
{
   if ( !getShapeInstance() )
      return false;

   return getShapeInstance()->buildPolyList(polyList, getShapeInstance()->getCurrentDetail());
}

void MeshColliderBehaviorInstance::_updatePhysics()
{
   SAFE_DELETE( (dynamic_cast<Entity*>(mOwner))->mPhysicsRep );

   if ( !PHYSICSMGR )
      return;

   /*PhysicsCollision *colShape = NULL;
   MatrixF offset( true );
   offset.setPosition( mOwner->getPosition() );
   colShape = PHYSICSMGR->createCollision();
   //colShape->addBox( mOwner->getObjBox().getExtents() * 0.5f * mOwner->getScale(), offset ); 
   colShape->addBox( colliderScale, offset );

   if ( colShape )
   {
   PhysicsWorld *world = PHYSICSMGR->getWorld( mOwner->isServerObject() ? "server" : "client" );
   (dynamic_cast<Entity*>(mOwner))->mPhysicsRep = PHYSICSMGR->createBody();
   (dynamic_cast<Entity*>(mOwner))->mPhysicsRep->init( colShape, 0, 0, mOwner, world );
   (dynamic_cast<Entity*>(mOwner))->mPhysicsRep->setTransform( mOwner->getTransform() );
   }*/
}

bool MeshColliderBehaviorInstance::castRay(const Point3F &start, const Point3F &end, RayInfo* info)
{
   TSShapeInstance *shapeInstance = getShapeInstance();
   if ( !shapeInstance )
      return false;

   // Cast the ray against the currently visible detail
   RayInfo localInfo;
   bool res = castRayOpcode( shapeInstance->getCurrentDetail(), start, end, &localInfo );

   if ( res )
   {
      *info = localInfo;
      info->object = this->getOwner();
      return true;
   }

   //do our callback
   //onRaycastCollision_callback( this, enter );
   return false;
}

bool MeshColliderBehaviorInstance::updateCollisions(F32 time, VectorF vector, VectorF velocity)
{
   //mOwner->getObjBox().getCenter(&mConvexList->mCenter);

   //the way this sets up, we basically make a convex per poly, which is why polysoup collisions can be really inefficient.
   //we'll basically cycle through our convex list, collect only polies that can be extruded(so front-facing to our end point)
   //and then extrude them one at a time untill they're all done, or we collide
   MeshColliderPolysoupConvex *conv = dynamic_cast<MeshColliderPolysoupConvex*>(mConvexList);
   while(conv != NULL)
   {
      conv = dynamic_cast<MeshColliderPolysoupConvex*>(conv->mNext);
   }

   Polyhedron cPolyhedron;
   cPolyhedron.buildBox(mOwner->getTransform(), mOwner->getObjBox(), true);
   ExtrudedPolyList extrudePoly;

   extrudePoly.extrude(cPolyhedron, velocity);
   extrudePoly.setVelocity(velocity);
   extrudePoly.setCollisionList(&mCollisionList);

   Box3F plistBox = mOwner->getObjBox();
   mOwner->getTransform().mul(plistBox); 
   Point3F oldMin = plistBox.minExtents;
   Point3F oldMax = plistBox.maxExtents;
   plistBox.minExtents.setMin(oldMin + (velocity * time) - Point3F(0.1f, 0.1f, 0.1f));
   plistBox.maxExtents.setMax(oldMax + (velocity * time) + Point3F(0.1f, 0.1f, 0.1f));

   // Build list from convex states here...
   CollisionWorkingList& rList = mConvexList->getWorkingList();
   CollisionWorkingList* pList = rList.wLink.mNext;
   while (pList != &rList) {
      Convex* pConvex = pList->mConvex;
      if (pConvex->getObject()->getTypeMask() & CollisionMoveMask) {
         Box3F convexBox = pConvex->getBoundingBox();
         if (plistBox.isOverlapped(convexBox))
         {
            pConvex->getPolyList(&extrudePoly);
         }
      }
      pList = pList->wLink.mNext;
   }

   if(mCollisionList.getCount() > 0)
      return true;
   else
      return false;
}

void MeshColliderBehaviorInstance::updateWorkingCollisionSet(const U32 mask)
{
   //if we have no convex, we cannot collide
   if(!mConvexList)
      return; 

   // First, we need to adjust our velocity for possible acceleration.  It is assumed
   // that we will never accelerate more than 20 m/s for gravity, plus 10 m/s for
   // jetting, and an equivalent 10 m/s for jumping.  We also assume that the
   // working list is updated on a Tick basis, which means we only expand our
   // box by the possible movement in that tick.
   VectorF velocity;
   VelocityInterface* velInterface = mOwner->getComponent<VelocityInterface>();
   if(!velInterface)
      velocity = Point3F(0,0,0);
   else 
      velocity = velInterface->getVelocity();

   Point3F scaledVelocity = velocity * TickSec;
   F32 len    = scaledVelocity.len();
   F32 newLen = len + (10.0f * TickSec);

   // Check to see if it is actually necessary to construct the new working list,
   // or if we can use the cached version from the last query.  We use the x
   // component of the min member of the mWorkingQueryBox, which is lame, but
   // it works ok.
   bool updateSet = false;

   Box3F convexBox = mConvexList->getBoundingBox(mOwner->getTransform(), mOwner->getScale());
   F32 l = (newLen * 1.1f) + 0.1f;  // from Convex::updateWorkingList
   const Point3F  lPoint( l, l, l );
   convexBox.minExtents -= lPoint;
   convexBox.maxExtents += lPoint;

   // Check containment
   if (mWorkingQueryBox.minExtents.x != -1e9f)
   {
      if (mWorkingQueryBox.isContained(convexBox) == false)
         // Needed region is outside the cached region.  Update it.
            updateSet = true;
   }
   else
   {
      // Must update
      updateSet = true;
   }

   // Actually perform the query, if necessary
   if (updateSet == true) {
      const Point3F  twolPoint( 2.0f * l, 2.0f * l, 2.0f * l );
      mWorkingQueryBox = convexBox;
      mWorkingQueryBox.minExtents -= twolPoint;
      mWorkingQueryBox.maxExtents += twolPoint;

      mOwner->disableCollision();
      mConvexList->updateWorkingList(mWorkingQueryBox, U32(-1));
      //isGhost() ? sClientCollisionContactMask : sServerCollisionContactMask);
      mOwner->enableCollision();
   }
}

void MeshColliderBehaviorInstance::prepRenderImage( SceneRenderState *state )
{
   if(sRenderMeshColliders)
   {
      TSShapeInstance* shapeInstance = getShapeInstance();

      if(shapeInstance != NULL && mDebugRender.shapeInstance != shapeInstance)
      {
         mDebugRender.clear();
         mDebugRender.shapeInstance = shapeInstance;

         Vector<S32> losDetails;

         shapeInstance->getShape()->findColDetails( true, &mCollisionDetails, &losDetails );

         if(!mCollisionDetails.empty())
         {
            TSShape* shape = shapeInstance->getShape();

            for(U32 i=0; i < mCollisionDetails.size(); i++)
            {
               // get subshape and object detail
               const TSDetail * detail = &shape->details[mCollisionDetails[i]];
               S32 ss = detail->subShapeNum;
               S32 od = detail->objectDetailNum;

               // nothing emitted yet...
               //bool emitted = false;

               S32 start = shape->subShapeFirstObject[ss];
               S32 end   = shape->subShapeNumObjects[ss] + start;
               if (start<end)
               {
                  MatrixF initialMat = mOwner->getObjToWorld();
                  Point3F initialScale = mOwner->getScale();

                  // set up for first object's node
                  MatrixF mat;
                  MatrixF scaleMat(true);
                  F32* p = scaleMat;
                  p[0]  = initialScale.x;
                  p[5]  = initialScale.y;
                  p[10] = initialScale.z;
                  const MatrixF * previousMat = &shapeInstance->mMeshObjects[start].getTransform();
                  mat.mul(initialMat,scaleMat);
                  mat.mul(*previousMat);

                  // run through objects and collide
                  Opcode::VertexPointers vp;
                  for (S32 i=start; i<end; i++)
                  {
                     TSShapeInstance::MeshObjectInstance * meshInstance = &shapeInstance->mMeshObjects[i];

                     TSMesh * mesh = meshInstance->getMesh(od);

                     mDebugRender.triCount = mesh->mOptTree->GetMeshInterface()->GetNbTriangles();
                     for(U32 j=0; j < mDebugRender.triCount; j++)
                     {
                        mesh->mOptTree->GetMeshInterface()->GetTriangle( vp, j );

                        Point3F a( vp.Vertex[0]->x, vp.Vertex[0]->y, vp.Vertex[0]->z );
                        Point3F b( vp.Vertex[1]->x, vp.Vertex[1]->y, vp.Vertex[1]->z );
                        Point3F c( vp.Vertex[2]->x, vp.Vertex[2]->y, vp.Vertex[2]->z );

                        //MatrixF instanceTrans = shapeInstance->mMeshObjects[start].getTransform();
                        //MatrixF meshTrans = meshInstance->getTransform();
                        //MatrixF meshToObjectMat;

                        //meshToObjectMat.mul(meshTrans, scaleMat);
                        //initialMat.mul(meshToObjectMat);
                        //mat.mul(*meshToObjectMat);

                        // Transform the result into object space!
                        //meshToObjectMat.mulP( a );
                        //meshToObjectMat.mulP( b );
                        //meshToObjectMat.mulP( c );

                        mat.mulP( a );
                        mat.mulP( b );
                        mat.mulP( c );

                        mDebugRender.vertA.push_back(a);
                        mDebugRender.vertB.push_back(b);
                        mDebugRender.vertC.push_back(c);
                     }
                  }
               }
            }
         }
      }

      ObjectRenderInst *ri = state->getRenderPass()->allocInst<ObjectRenderInst>();
      ri->renderDelegate.bind( this, &MeshColliderBehaviorInstance::renderConvex );
      ri->objectIndex = -1;
      ri->type = RenderPassManager::RIT_Editor;
      state->getRenderPass()->addInst( ri );
   }
}

void MeshColliderBehaviorInstance::renderConvex( ObjectRenderInst *ri, SceneRenderState *state, BaseMatInstance *overrideMat )
{
   GFX->enterDebugEvent( ColorI(255,0,255), "MeshColliderBehaviorInstance_renderConvex" );

   GFXStateBlockDesc desc;
   desc.setZReadWrite( true, false );
   desc.setBlend( true );
   desc.setCullMode( GFXCullNone );
   //desc.setZReadWrite( false, false );
   desc.fillMode = GFXFillWireframe;

   if(mDebugRender.triCount <= 0)
      return;

   GFXStateBlockRef sb = GFX->createStateBlock( desc );
   GFX->setStateBlock( sb );

   PrimBuild::color3i( 255, 0, 255 );

   Point3F *pnt;

   PrimBuild::begin( GFXTriangleList, mDebugRender.triCount * 3 );      

   for(U32 i=0; i < mDebugRender.triCount; i++)
   {
      pnt = &mDebugRender.vertA[i];
      PrimBuild::vertex3fv( pnt );

      pnt = &mDebugRender.vertB[i];
      PrimBuild::vertex3fv( pnt );

      pnt = &mDebugRender.vertC[i];
      PrimBuild::vertex3fv( pnt );
   }

   PrimBuild::end();

   GFX->leaveDebugEvent();
}

bool MeshColliderBehaviorInstance::buildConvex(const Box3F& box, Convex* convex)
{
   TSShapeInstance *shapeInstance = getShapeInstance();
   if(!shapeInstance)
      return false;

   // These should really come out of a pool
   mConvexList->collectGarbage();

   for (U32 i = 0; i < mCollisionDetails.size(); i++)
      buildConvexOpcode( shapeInstance, mCollisionDetails[i], box, convex, mConvexList );

   return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool MeshColliderBehaviorInstance::buildConvexOpcode( TSShapeInstance* sI, S32 dl, const Box3F &bounds, Convex *c, Convex *list )
{
   AssertFatal(dl>=0 && dl < sI->getShape()->details.size(),"TSShapeInstance::buildConvexOpcode");

   TSShape* shape = sI->getShape();

   const MatrixF &objMat = mOwner->getObjToWorld();
   const Point3F &objScale = mOwner->getScale();

   // get subshape and object detail
   const TSDetail * detail = &shape->details[dl];
   S32 ss = detail->subShapeNum;
   S32 od = detail->objectDetailNum;

   // nothing emitted yet...
   bool emitted = false;

   S32 start = shape->subShapeFirstObject[ss];
   S32 end   = shape->subShapeNumObjects[ss] + start;
   if (start<end)
   {
      MatrixF initialMat = objMat;
      Point3F initialScale = objScale;

      // set up for first object's node
      MatrixF mat;
      MatrixF scaleMat(true);
      F32* p = scaleMat;
      p[0]  = initialScale.x;
      p[5]  = initialScale.y;
      p[10] = initialScale.z;
      const MatrixF * previousMat = &sI->mMeshObjects[start].getTransform();
      mat.mul(initialMat,scaleMat);
      mat.mul(*previousMat);

      // Update our bounding box...
      Box3F localBox = bounds;
      MatrixF otherMat = mat;
      otherMat.inverse();
      otherMat.mul(localBox);

      // run through objects and collide
      for (S32 i=start; i<end; i++)
      {
         TSShapeInstance::MeshObjectInstance * meshInstance = &sI->mMeshObjects[i];

         if (od >= meshInstance->object->numMeshes)
            continue;

         if (&meshInstance->getTransform() != previousMat)
         {
            // different node from before, set up for this node
            previousMat = &meshInstance->getTransform();

            if (previousMat != NULL)
            {
               mat.mul(initialMat,scaleMat);
               mat.mul(*previousMat);

               // Update our bounding box...
               otherMat = mat;
               otherMat.inverse();
               localBox = bounds;
               otherMat.mul(localBox);
            }
         }

         // collide... note we pass the original mech transform
         // here so that the convex data returned is in mesh space.
         TSMesh * mesh = meshInstance->getMesh(od);
         if ( mesh && !meshInstance->forceHidden && meshInstance->visible > 0.01f && localBox.isOverlapped( mesh->getBounds() ) )
            emitted |= buildMeshOpcode(mesh, *previousMat, localBox, c, list);
         else
            emitted |= false;
      }
   }

   return emitted;
}

bool MeshColliderBehaviorInstance::buildMeshOpcode(  TSMesh *mesh, const MatrixF &meshToObjectMat, 
                                                   const Box3F &nodeBox, Convex *convex, Convex *list)
{
   PROFILE_SCOPE( MeshCollider_buildConvexOpcode );

   // This is small... there is no win for preallocating it.
   Opcode::AABBCollider opCollider;
   opCollider.SetPrimitiveTests( true );

   // This isn't really needed within the AABBCollider as 
   // we don't use temporal coherance... use a static to 
   // remove the allocation overhead.
   static Opcode::AABBCache opCache;

   IceMaths::AABB opBox;
   opBox.SetMinMax(  Point( nodeBox.minExtents.x, nodeBox.minExtents.y, nodeBox.minExtents.z ),
                     Point( nodeBox.maxExtents.x, nodeBox.maxExtents.y, nodeBox.maxExtents.z ) );
   Opcode::CollisionAABB opCBox(opBox);

   if( !opCollider.Collide( opCache, opCBox, *mesh->mOptTree ) )
      return false;

   U32 cnt = opCollider.GetNbTouchedPrimitives();
   const udword *idx = opCollider.GetTouchedPrimitives();

   Opcode::VertexPointers vp;
   for ( S32 i = 0; i < cnt; i++ )
   {
      // First, check our active convexes for a potential match (and clean things
      // up, too.)
      const U32 curIdx = idx[i];

      // See if the square already exists as part of the working set.
      bool gotMatch = false;
      CollisionWorkingList& wl = convex->getWorkingList();
      for ( CollisionWorkingList* itr = wl.wLink.mNext; itr != &wl; itr = itr->wLink.mNext )
      {
         if( itr->mConvex->getType() != TSPolysoupConvexType )
            continue;

         const MeshColliderPolysoupConvex *chunkc = static_cast<MeshColliderPolysoupConvex*>( itr->mConvex );

         if( chunkc->getObject() != mOwner )
            continue;

         if( chunkc->mesh != mesh )
            continue;

         if( chunkc->idx != curIdx )
            continue;

         // A match! Don't need to add it.
         gotMatch = true;
         break;
      }

      if( gotMatch )
         continue;

      // Get the triangle...
      mesh->mOptTree->GetMeshInterface()->GetTriangle( vp, idx[i] );

      Point3F a( vp.Vertex[0]->x, vp.Vertex[0]->y, vp.Vertex[0]->z );
      Point3F b( vp.Vertex[1]->x, vp.Vertex[1]->y, vp.Vertex[1]->z );
      Point3F c( vp.Vertex[2]->x, vp.Vertex[2]->y, vp.Vertex[2]->z );

      // Transform the result into object space!
      meshToObjectMat.mulP( a );
      meshToObjectMat.mulP( b );
      meshToObjectMat.mulP( c );

      //build the shape-center offset
      /*Point3F center = getShapeInstance()->getShape()->center;
      Point3F position = mOwner->getPosition();

      //mOwner->getTransform().mulP(center);

      Point3F posOffset = position;// - center;

      a += posOffset;
      b += posOffset;
      c += posOffset;*/


      //If we're not doing debug rendering on the client, then set up our convex list as normal
      PlaneF p( c, b, a );
      Point3F peak = ((a + b + c) / 3.0f) - (p * 0.15f);

      // Set up the convex...
      MeshColliderPolysoupConvex *cp = new MeshColliderPolysoupConvex();

      list->registerObject( cp );
      convex->addToWorkingList( cp );

      cp->mesh    = mesh;
      cp->idx     = curIdx;
      cp->mObject = mOwner;

      cp->normal = p;
      cp->verts[0] = a;
      cp->verts[1] = b;
      cp->verts[2] = c;
      cp->verts[3] = peak;

      // Update the bounding box.
      Box3F &bounds = cp->box;
      bounds.minExtents.set( F32_MAX,  F32_MAX,  F32_MAX );
      bounds.maxExtents.set( -F32_MAX, -F32_MAX, -F32_MAX );

      bounds.minExtents.setMin( a );
      bounds.minExtents.setMin( b );
      bounds.minExtents.setMin( c );
      bounds.minExtents.setMin( peak );

      bounds.maxExtents.setMax( a );
      bounds.maxExtents.setMax( b );
      bounds.maxExtents.setMax( c );
      bounds.maxExtents.setMax( peak );
   }

   return true;
}

bool MeshColliderBehaviorInstance::castRayOpcode( S32 dl, const Point3F & startPos, const Point3F & endPos, RayInfo *info)
{
   // if dl==-1, nothing to do
   if (dl==-1 || !getShapeInstance())
      return false;

   TSShape *shape = getShapeInstance()->getShape();

   AssertFatal(dl>=0 && dl < shape->details.size(),"TSShapeInstance::castRayOpcode");

   info->t = 100.f;

   // get subshape and object detail
   const TSDetail * detail = &shape->details[dl];
   S32 ss = detail->subShapeNum;
   if ( ss < 0 )
      return false;

   S32 od = detail->objectDetailNum;

   // nothing emitted yet...
   bool emitted = false;

   const MatrixF* saveMat = NULL;
   S32 start = shape->subShapeFirstObject[ss];
   S32 end   = shape->subShapeNumObjects[ss] + start;
   if (start<end)
   {
      MatrixF mat;
      const MatrixF * previousMat = &getShapeInstance()->mMeshObjects[start].getTransform();
      mat = *previousMat;
      mat.inverse();
      Point3F localStart, localEnd;
      mat.mulP(startPos, &localStart);
      mat.mulP(endPos, &localEnd);

      // run through objects and collide
      for (S32 i=start; i<end; i++)
      {
         TSShapeInstance::MeshObjectInstance * meshInstance = &getShapeInstance()->mMeshObjects[i];

         if (od >= meshInstance->object->numMeshes)
            continue;

         if (&meshInstance->getTransform() != previousMat)
         {
            // different node from before, set up for this node
            previousMat = &meshInstance->getTransform();

            if (previousMat != NULL)
            {
               mat = *previousMat;
               mat.inverse();
               mat.mulP(startPos, &localStart);
               mat.mulP(endPos, &localEnd);
            }
         }

         // collide...
         TSMesh * mesh = meshInstance->getMesh(od);
         if (mesh && !meshInstance->forceHidden && meshInstance->visible > 0.01f)
         {
            if ( castRayMeshOpcode(mesh, localStart, localEnd, info, getShapeInstance()->mMaterialList) )
            {
               saveMat = previousMat;
               emitted = true;
            }
         }
      }
   }

   if ( emitted )
   {
      saveMat->mulV(info->normal);
      info->point  = endPos - startPos;
      info->point *= info->t;
      info->point += startPos;
   }

   return emitted;
}

bool MeshColliderBehaviorInstance::castRayMeshOpcode(TSMesh *mesh, const Point3F &s, const Point3F &e, RayInfo *info, TSMaterialList *materials )
{
   Opcode::RayCollider ray;
   Opcode::CollisionFaces cfs;

   IceMaths::Point dir( e.x - s.x, e.y - s.y, e.z - s.z );
   const F32 rayLen = dir.Magnitude();
   IceMaths::Ray vec( Point(s.x, s.y, s.z), dir.Normalize() );

   ray.SetDestination( &cfs);
   ray.SetFirstContact( false );
   ray.SetClosestHit( true );
   ray.SetPrimitiveTests( true );
   ray.SetCulling( true );
   ray.SetMaxDist( rayLen );

   AssertFatal( ray.ValidateSettings() == NULL, "invalid ray settings" );

   // Do collision.
   bool safety = ray.Collide( vec, *mesh->mOptTree );
   AssertFatal( safety, "TSMesh::castRayOpcode - no good ray collide!" );

   // If no hit, just skip out.
   if( cfs.GetNbFaces() == 0 )
      return false;

   // Got a hit!
   AssertFatal( cfs.GetNbFaces() == 1, "bad" );
   const Opcode::CollisionFace &face = cfs.GetFaces()[0];

   // If the cast was successful let's check if the t value is less than what we had
   // and toggle the collision boolean
   // Stupid t... i prefer coffee
   const F32 t = face.mDistance / rayLen;

   if( t < 0.0f || t > 1.0f )
      return false;

   if( t <= info->t )
   {
      info->t = t;

      // Calculate the normal.
      Opcode::VertexPointers vp;
      mesh->mOptTree->GetMeshInterface()->GetTriangle( vp, face.mFaceID );

      if ( materials && vp.MatIdx >= 0 && vp.MatIdx < materials->size() )
         info->material = materials->getMaterialInst( vp.MatIdx );

      // Get the two edges.
      IceMaths::Point baseVert = *vp.Vertex[0];
      IceMaths::Point a = *vp.Vertex[1] - baseVert;
      IceMaths::Point b = *vp.Vertex[2] - baseVert;

      IceMaths::Point n;
      n.Cross( a, b );
      n.Normalize();

      info->normal.set( n.x, n.y, n.z );

      // generate UV coordinate across mesh based on 
      // matching normals, this isn't done by default and is 
      // primarily of interest in matching a collision point to 
      // either a GUI control coordinate or finding a hit pixel in texture space
      if (info->generateTexCoord)
      {
         baseVert = *vp.Vertex[0];
         a = *vp.Vertex[1];
         b = *vp.Vertex[2];

         Point3F facePoint = (1.0f - face.mU - face.mV) * Point3F(baseVert.x, baseVert.y, baseVert.z)  
            + face.mU * Point3F(a.x, a.y, a.z) + face.mV * Point3F(b.x, b.y, b.z);

         U32 faces[1024];
         U32 numFaces = 0;
         for (U32 i = 0; i < mesh->mOptTree->GetMeshInterface()->GetNbTriangles(); i++)
         {
            if ( i == face.mFaceID )
            {
               faces[numFaces++] = i;
            }
            else
            {
               IceMaths::Point n2;

               mesh->mOptTree->GetMeshInterface()->GetTriangle( vp, i );

               baseVert = *vp.Vertex[0];
               a = *vp.Vertex[1] - baseVert;
               b = *vp.Vertex[2] - baseVert;
               n2.Cross( a, b );
               n2.Normalize();

               F32 eps = .01f;
               if ( mFabs(n.x - n2.x) < eps && mFabs(n.y - n2.y) < eps && mFabs(n.z - n2.z) < eps)
               {
                  faces[numFaces++] = i;
               }
            }

            if (numFaces == 1024)
            {
               // too many faces in this collision mesh for UV generation
               return true;
            }

         }

         Point3F min(F32_MAX, F32_MAX, F32_MAX);
         Point3F max(-F32_MAX, -F32_MAX, -F32_MAX);

         for (U32 i = 0; i < numFaces; i++)
         {
            mesh->mOptTree->GetMeshInterface()->GetTriangle( vp, faces[i] );

            for ( U32 j =0; j < 3; j++)
            {
               a = *vp.Vertex[j];

               if (a.x < min.x)
                  min.x = a.x;
               if (a.y < min.y)
                  min.y = a.y;
               if (a.z < min.z)
                  min.z = a.z;

               if (a.x > max.x)
                  max.x = a.x;
               if (a.y > max.y)
                  max.y = a.y;
               if (a.z > max.z)
                  max.z = a.z;

            }

         }

         // slerp
         Point3F s = ( (max - min) - (facePoint - min) ) / (max - min);

         // compute axis
         S32		bestAxis = 0;
         F32      best = 0.f;

         for (U32 i = 0 ; i < 6 ; i++)
         {
            F32 dot = mDot (info->normal, texGenAxis[i*3]);
            if (dot > best)
            {
               best = dot;
               bestAxis = i;
            }
         }

         Point3F xv = texGenAxis[bestAxis*3+1];
         Point3F yv = texGenAxis[bestAxis*3+2];

         S32 sv, tv;

         if (xv.x)
            sv = 0;
         else if (xv.y)
            sv = 1;
         else
            sv = 2;

         if (yv.x)
            tv = 0;
         else if (yv.y)
            tv = 1;
         else
            tv = 2;

         // handle coord translation
         if (bestAxis == 2 || bestAxis == 3)
         {
            S32 x = sv;
            sv = tv;
            tv = x;

            if (yv.z < 0)
               s[sv] = 1.f - s[sv];
         }

         if (bestAxis < 2)
         {
            if (yv.y < 0)
               s[sv] = 1.f - s[sv];
         }

         if (bestAxis > 3)
         {
            s[sv] = 1.f - s[sv];
            if (yv.z > 0)
               s[tv] = 1.f - s[tv];

         }

         // done!
         info->texCoord.set(s[sv], s[tv]);

      }

      return true;
   }

   return false;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
MeshColliderPolysoupConvex::MeshColliderPolysoupConvex()
   :  box( 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f ),
   normal( 0.0f, 0.0f, 0.0f, 0.0f ),
   idx( 0 ),
   mesh( NULL )
{
   mType = TSPolysoupConvexType;

   for ( U32 i = 0; i < 4; ++i )
   {
      verts[i].set( 0.0f, 0.0f, 0.0f );
   }
}

Point3F MeshColliderPolysoupConvex::support(const VectorF& vec) const
{
   F32 bestDot = mDot( verts[0], vec );

   const Point3F *bestP = &verts[0];
   for(S32 i=1; i<4; i++)
   {
      F32 newD = mDot(verts[i], vec);
      if(newD > bestDot)
      {
         bestDot = newD;
         bestP = &verts[i];
      }
   }

   return *bestP;
}

Box3F MeshColliderPolysoupConvex::getBoundingBox() const
{
   Box3F wbox = box;
   wbox.minExtents.convolve( mObject->getScale() );
   wbox.maxExtents.convolve( mObject->getScale() );
   mObject->getTransform().mul(wbox);
   return wbox;
}

Box3F MeshColliderPolysoupConvex::getBoundingBox(const MatrixF& mat, const Point3F& scale) const
{
   AssertISV(false, "MeshColliderPolysoupConvex::getBoundingBox(m,p) - Not implemented. -- XEA");
   return box;
}

void MeshColliderPolysoupConvex::getPolyList(AbstractPolyList *list)
{
   // Transform the list into object space and set the pointer to the object
   MatrixF i( mObject->getTransform() );
   Point3F iS( mObject->getScale() );

   list->setTransform(&i, iS);
   list->setObject(mObject);

   // Add only the original collision triangle
   S32 base =  list->addPoint(verts[0]);
   list->addPoint(verts[2]);
   list->addPoint(verts[1]);

   list->begin(0, (U32)idx ^ (U32)mesh);
   list->vertex(base + 2);
   list->vertex(base + 1);
   list->vertex(base + 0);
   list->plane(base + 0, base + 1, base + 2);
   list->end();
}

void MeshColliderPolysoupConvex::getFeatures(const MatrixF& mat,const VectorF& n, ConvexFeature* cf)
{
   cf->material = 0;
   cf->object = mObject;

   // For a tetrahedron this is pretty easy... first
   // convert everything into world space.
   Point3F tverts[4];
   mat.mulP(verts[0], &tverts[0]);
   mat.mulP(verts[1], &tverts[1]);
   mat.mulP(verts[2], &tverts[2]);
   mat.mulP(verts[3], &tverts[3]);

   // points...
   S32 firstVert = cf->mVertexList.size();
   cf->mVertexList.increment(); cf->mVertexList.last() = tverts[0];
   cf->mVertexList.increment(); cf->mVertexList.last() = tverts[1];
   cf->mVertexList.increment(); cf->mVertexList.last() = tverts[2];
   cf->mVertexList.increment(); cf->mVertexList.last() = tverts[3];

   //    edges...
   cf->mEdgeList.increment();
   cf->mEdgeList.last().vertex[0] = firstVert+0;
   cf->mEdgeList.last().vertex[1] = firstVert+1;

   cf->mEdgeList.increment();
   cf->mEdgeList.last().vertex[0] = firstVert+1;
   cf->mEdgeList.last().vertex[1] = firstVert+2;

   cf->mEdgeList.increment();
   cf->mEdgeList.last().vertex[0] = firstVert+2;
   cf->mEdgeList.last().vertex[1] = firstVert+0;

   cf->mEdgeList.increment();
   cf->mEdgeList.last().vertex[0] = firstVert+3;
   cf->mEdgeList.last().vertex[1] = firstVert+0;

   cf->mEdgeList.increment();
   cf->mEdgeList.last().vertex[0] = firstVert+3;
   cf->mEdgeList.last().vertex[1] = firstVert+1;

   cf->mEdgeList.increment();
   cf->mEdgeList.last().vertex[0] = firstVert+3;
   cf->mEdgeList.last().vertex[1] = firstVert+2;

   //    triangles...
   cf->mFaceList.increment();
   cf->mFaceList.last().normal = PlaneF(tverts[2], tverts[1], tverts[0]);
   cf->mFaceList.last().vertex[0] = firstVert+2;
   cf->mFaceList.last().vertex[1] = firstVert+1;
   cf->mFaceList.last().vertex[2] = firstVert+0;

   cf->mFaceList.increment();
   cf->mFaceList.last().normal = PlaneF(tverts[1], tverts[0], tverts[3]);
   cf->mFaceList.last().vertex[0] = firstVert+1;
   cf->mFaceList.last().vertex[1] = firstVert+0;
   cf->mFaceList.last().vertex[2] = firstVert+3;

   cf->mFaceList.increment();
   cf->mFaceList.last().normal = PlaneF(tverts[2], tverts[1], tverts[3]);
   cf->mFaceList.last().vertex[0] = firstVert+2;
   cf->mFaceList.last().vertex[1] = firstVert+1;
   cf->mFaceList.last().vertex[2] = firstVert+3;

   cf->mFaceList.increment();
   cf->mFaceList.last().normal = PlaneF(tverts[0], tverts[2], tverts[3]);
   cf->mFaceList.last().vertex[0] = firstVert+0;
   cf->mFaceList.last().vertex[1] = firstVert+2;
   cf->mFaceList.last().vertex[2] = firstVert+3;

   // All done!
}

void MeshColliderPolysoupConvex::getAllPolyList(AbstractPolyList *list)
{
   if(mNext)
   {
      MeshColliderPolysoupConvex *itr = dynamic_cast<MeshColliderPolysoupConvex*>(mNext);
      while(itr)
      {
         itr->getPolyList(list);
         itr = dynamic_cast<MeshColliderPolysoupConvex*>(itr->mNext);
      }
   }
}

void MeshColliderPolysoupConvex::render()
{
   ConcretePolyList polyList;
   getAllPolyList( &polyList );
   polyList.render();
}