//-----------------------------------------------------------------------------
// Copyright (c) 2015 Andrew Mac
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.
//-----------------------------------------------------------------------------

#include "platform/platform.h"
#include "T3D/offlineLPV/offlineLPV.h"
#include "scene/sceneManager.h"
#include "scene/sceneRenderState.h"
#include "gfx/gfxDevice.h"
#include "gfx/gfxDrawUtil.h"
#include "gfx/sim/debugDraw.h"
#include "util/tempAlloc.h"
#include "materials/materialDefinition.h"
#include "materials/materialManager.h"
#include "materials/materialFeatureTypes.h"
#include "materials/matInstance.h"
#include "materials/processedMaterial.h"
#include "console/consoleTypes.h"
#include "core/stream/bitStream.h"
#include "gfx/gfxDevice.h"
#include "console/console.h"
#include "console/engineAPI.h"
#include "gfx/gfxTextureHandle.h"
#include "scene/sceneContainer.h"
#include "math/mRandom.h"
#include "T3D/lightBase.h"
#include "lighting/shadowMap/lightShadowMap.h"
#include "renderInstance/renderPassManager.h"
#include "renderInstance/renderBinManager.h"
#include "gfx/gfxTextureManager.h"
#include "materials/shaderData.h"
#include "gfx/sim/gfxStateBlockData.h"
#include "gfx/util/screenspace.h"
#include "gfx/gfxTextureHandle.h"
#include "postFx/postEffectCommon.h"
#include "core/stream/fileStream.h"
#include "core/fileObject.h"
#include "math/mPolyhedron.impl.h"
#include "lighting/advanced/advancedLightBinManager.h"

#include "gui/controls/guiTextCtrl.h"

#include "collision/optimizedPolyList.h"

IMPLEMENT_CO_NETOBJECT_V1( OfflineLPV );

ConsoleDocClass( OfflineLPV,
   "@brief An invisible shape that allow objects within it to be lit with static global illumination.\n\n"

   "OfflineLPV is used to add static global illumination to an area of the scene. It's results are generated\n\n"
   "at runtime based on the settings configured in the editor. \n\n"

   "@ingroup enviroMisc"
);

GFX_ImplementTextureProfile( LPVProfile,
                              GFXTextureProfile::DiffuseMap,
                              GFXTextureProfile::PreserveSize | GFXTextureProfile::KeepBitmap,
                              GFXTextureProfile::NONE );

//-----------------------------------------------------------------------------

OfflineLPV::OfflineLPV()
   : mTransformDirty( true ),
     mSilhouetteExtractor( mPolyhedron )
{
   VECTOR_SET_ASSOCIATION( mWSPoints );
   VECTOR_SET_ASSOCIATION( mVolumeQueryList );

   mNetFlags.set( Ghostable | ScopeAlways );
   mObjScale.set( 1.f, 1.f, 1.f );
   mObjBox.set(
      Point3F( -0.5f, -0.5f, -0.5f ),
      Point3F( 0.5f, 0.5f, 0.5f )
   );

   mObjToWorld.identity();
   mWorldToObj.identity();

   mVoxelSize           = 1.0f;
   mRegenVolume         = false;
   mShowVoxels          = false;
   mInjectLights        = false;
   mShowDirectLight     = false;
   mPropagationStage    = 0;
   mPropagateLights     = false;
   mShowPropagated      = false;
   mExportPropagated    = false;
   mExportDirectLight   = false;
   mRenderReflection    = false;
   mFileName            = "";
   mSaveResults         = false;
   mLoadResults         = false;

   mGeometryGrid           = NULL;
   mLightGrid              = NULL;
   mPropagatedLightGrid    = NULL;
   mPropagatedLightGridA   = NULL;
   mPropagatedLightGridB   = NULL;
   mPropagatedTexture      = NULL;
   mDirectLightTexture     = NULL;
   mLightInfoTarget        = NULL;
   mPrepassTarget          = NULL;
   mMatInfoTarget          = NULL;
   mPropagatedShader       = NULL;
   mPropagatedShaderConsts = NULL;
   mReflectShader          = NULL;
   mReflectShaderConsts    = NULL;
   mRenderTarget           = NULL;
   

   mLightIntensity = 1.0f;
   mEmissiveIntensity = 1.0f;
   mPropagationMultiplier = 1.0f;
   mGIMultiplier = 1.0f;

   resetWorldBox();

   RenderPassManager::getRenderBinSignal().notify( this, &OfflineLPV::_handleBinEvent );

   // SSAO Mask support.
   mSSAOMaskTarget = NULL;
   Con::NotifyDelegate callback( this, &OfflineLPV::_initShaders );
   Con::addVariableNotify( "$AL::UseSSAOMask", callback );

   mDebugRender.wireMeshRender.clear();
}

OfflineLPV::~OfflineLPV()
{
   SAFE_DELETE(mGeometryGrid);
   SAFE_DELETE(mLightGrid);
   SAFE_DELETE(mPropagatedLightGridA);
   SAFE_DELETE(mPropagatedLightGridB);

   mPropagatedTexture      = NULL;
   mDirectLightTexture     = NULL;
   mLightInfoTarget        = NULL;
   mPrepassTarget          = NULL;
   mMatInfoTarget          = NULL;
   mPropagatedShader       = NULL;
   mPropagatedShaderConsts = NULL;
   mReflectShader          = NULL;
   mReflectShaderConsts    = NULL;
   mRenderTarget           = NULL;
}

void OfflineLPV::initPersistFields()
{
   //
   addGroup("OfflineLPV - Geometry");

      addField("voxelSize", TypeF32, Offset(mVoxelSize, OfflineLPV), "");

      addProtectedField( "regenVolume", TypeBool, Offset( mRegenVolume, OfflineLPV ),
         &_setRegenVolume, &defaultProtectedGetFn, "Regenerate Voxel Grid", AbstractClassRep::FieldFlags::FIELD_ComponentInspectors);

      addField("showVoxels", TypeBool, Offset(mShowVoxels, OfflineLPV), "");

   endGroup("OfflineLPV - Geometry");

   addGroup("OfflineLPV - Lights");

      addField("lightIntensity", TypeF32, Offset(mLightIntensity, OfflineLPV), "");
      addField("emissiveIntensity", TypeF32, Offset(mEmissiveIntensity, OfflineLPV), "");

      addProtectedField( "injectLights", TypeBool, Offset( mInjectLights, OfflineLPV ),
            &_setInjectLights, &defaultProtectedGetFn, "Inject scene lights into grid.", AbstractClassRep::FieldFlags::FIELD_ComponentInspectors);

      addField("showDirectLight", TypeBool, Offset(mShowDirectLight, OfflineLPV), "");

   endGroup("OfflineLPV - Lights");

   //
   addGroup("OfflineLPV - Propagation");
      addField("propagationMultiplier", TypeF32, Offset(mPropagationMultiplier, OfflineLPV), "Strength/Weaken propagation.");

      addProtectedField( "propagateLights", TypeBool, Offset( mPropagateLights, OfflineLPV ),
            &_setPropagateLights, &defaultProtectedGetFn, "Perform light propagation.", AbstractClassRep::FieldFlags::FIELD_ComponentInspectors);

      addField("showPropagated", TypeBool, Offset(mShowPropagated, OfflineLPV), "Render propagated light.");

      addField("GIMultiplier", TypeF32, Offset(mGIMultiplier, OfflineLPV), "");

      addProtectedField( "exportPropagated", TypeBool, Offset( mExportPropagated, OfflineLPV ),
            &_setExportPropagated, &defaultProtectedGetFn, "Export propagated light grid to display.", AbstractClassRep::FieldFlags::FIELD_ComponentInspectors);

   endGroup("OfflineLPV - Propagation");

   //
   addGroup("OfflineLPV - Reflection");

      addProtectedField( "exportDirectLight", TypeBool, Offset( mExportDirectLight, OfflineLPV ),
            &_setExportDirectLight, &defaultProtectedGetFn, "Export direct light grid for reflections.", AbstractClassRep::FieldFlags::FIELD_ComponentInspectors);

      addField("renderReflection", TypeBool, Offset(mRenderReflection, OfflineLPV), "Render reflections.");

   endGroup("OfflineLPV - Reflection");

   //
   addGroup("OfflineLPV - Save/Load");

      addField( "fileName", TypeStringFilename, Offset( mFileName, OfflineLPV ), "Path of file to save and load results." );

      addProtectedField( "saveResults", TypeBool, Offset( mSaveResults, OfflineLPV ),
            &_setSaveResults, &defaultProtectedGetFn, "Save the results of the light propagation.", AbstractClassRep::FieldFlags::FIELD_ComponentInspectors);

      addProtectedField( "loadResults", TypeBool, Offset( mLoadResults, OfflineLPV ),
            &_setLoadResults, &defaultProtectedGetFn, "Load the results of the light propagation.", AbstractClassRep::FieldFlags::FIELD_ComponentInspectors);

   endGroup("OfflineLPV - Save/Load");

   Parent::initPersistFields();
}

//-----------------------------------------------------------------------------

void OfflineLPV::consoleInit()
{
   // Disable rendering of occlusion volumes by default.
   getStaticClassRep()->mIsRenderEnabled = false;
}

//-----------------------------------------------------------------------------

bool OfflineLPV::onAdd()
{
   if( !Parent::onAdd() )
      return false;

   if ( isClientObject() )  
      load();

   // None of these are actual settings to be saved.
   mRegenVolume         = false;
   mInjectLights        = false;
   mPropagateLights     = false;
   mExportPropagated    = false;
   mExportDirectLight   = false;
   mSaveResults         = false;
   mLoadResults         = false;
   mShowVoxels          = false;
   mShowDirectLight     = false;
   mShowPropagated      = false;

   // Set up the silhouette extractor.
   mSilhouetteExtractor = SilhouetteExtractorType( mPolyhedron );

   return true;
}

void OfflineLPV::onRemove()
{
   if ( isClientObject() )  
   {

   }
   Parent::onRemove();
}

//-----------------------------------------------------------------------------

// Returns a Point3I with the voxel count in each direction.
Point3I OfflineLPV::getVoxelCount()
{
   Point3I voxelCount;
   Box3F worldBox = getWorldBox();

   voxelCount.x = worldBox.len_x() / mVoxelSize;
   voxelCount.y = worldBox.len_y() / mVoxelSize;
   voxelCount.z = worldBox.len_z() / mVoxelSize;

   return voxelCount;
}

// Returns a Point3F with the world space dimensions of a voxel.
Point3F OfflineLPV::getWorldSpaceVoxelSize()
{
   Box3F worldBox = getWorldBox();
   Point3F bottom_corner = worldBox.minExtents;
   Point3F top_corner = worldBox.maxExtents;
   Point3I voxelCount = getVoxelCount();
   Point3F difference = (top_corner - bottom_corner);
   difference.x = difference.x / voxelCount.x;
   difference.y = difference.y / voxelCount.y;
   difference.z = difference.z / voxelCount.z;
   return difference;
}

//--- Debug Voxel Rendering ---
static const Point3F cubePoints[8] = 
{
   Point3F(-1, -1, -1), Point3F(-1, -1,  1), Point3F(-1,  1, -1), Point3F(-1,  1,  1),
   Point3F( 1, -1, -1), Point3F( 1, -1,  1), Point3F( 1,  1, -1), Point3F( 1,  1,  1)
};

static const U32 cubeFaces[6][4] = 
{
   { 0, 4, 6, 2 }, { 0, 2, 3, 1 }, { 0, 1, 5, 4 },
   { 3, 2, 6, 7 }, { 7, 6, 4, 5 }, { 3, 7, 5, 1 }
};

void OfflineLPV::_rebuildDebugVoxels()
{
   Box3F worldBox = getWorldBox();
   Point3F bottom_corner = worldBox.minExtents;
   Point3F top_corner = worldBox.maxExtents;
   Point3F wsVoxelSize = getWorldSpaceVoxelSize();
   Point3I voxelCount = getVoxelCount();

   mDebugVoxels.clear();
   U32 pos = 0;
   for ( U32 z = 0; z < voxelCount.z; z++ )
   {
      for ( U32 y = 0; y < voxelCount.y; y++ )
      {
         for ( U32 x = 0; x < voxelCount.x; x++ )
         {
            // Calculate voxel space.
            Box3F new_box;
            new_box.set(bottom_corner + Point3F(wsVoxelSize.x * x, wsVoxelSize.y * y, wsVoxelSize.z * z), 
               bottom_corner + Point3F(wsVoxelSize.x * (x + 1), wsVoxelSize.y * (y + 1), wsVoxelSize.z * (z + 1)));
            DebugVoxel d;
            d.position = new_box.getCenter();
            d.size = new_box.getExtents();

            // Propagated Light Grid Visualization.
            if ( mShowPropagated && mPropagatedLightGrid )
            {
               LinearColorF decoded_color = decodeSH(Point3F(0, 0, 0), mPropagatedLightGrid[pos]);
               if ( decoded_color.red > 0.0f || decoded_color.green > 0.0f || decoded_color.blue > 0.0f )
               {
                  d.color = decoded_color;
                  mDebugVoxels.push_back(d);
               }

               pos++;
               continue;
            }

            // Direct Light Grid Visualization.
            if ( mShowDirectLight && mLightGrid )
            {
               LinearColorF decoded_color = decodeSH(Point3F(0, 0, 0), mLightGrid[pos]);
               if ( decoded_color.red > 0.0f || decoded_color.green > 0.0f || decoded_color.blue > 0.0f )
               {
                  d.color = decoded_color;
                  mDebugVoxels.push_back(d);
               }

               pos++;
               continue;
            }

            // Geometry Grid Visualization.
            if ( mShowVoxels && mGeometryGrid )
            {
               if ( mGeometryGrid[pos].color.alpha > 0.0f )
               {
                  d.color = mGeometryGrid[pos].color;
                  mDebugVoxels.push_back(d);
               }

               pos++;
               continue;
            }

            pos++;
         }
      }
   }

   _rebuildDebugVoxelsVB();
}

void OfflineLPV::_rebuildDebugVoxelsVB()
{
   if ( mDebugVoxels.size() < 1 ) return;

   U32 numVerts = mDebugVoxels.size() * 36;
   if ( mDebugVoxelVB.isNull() || mDebugVoxelVB->mNumVerts != numVerts )
      mDebugVoxelVB.set(GFX, mDebugVoxels.size() * 36, GFXBufferTypeDynamic );

   mDebugVoxelVB.lock();

   U32 vert_pos = 0;
   for( U32 n = 0; n < mDebugVoxels.size(); ++n)
   {
      Point3F halfSize = mDebugVoxels[n].size * 0.5f;

      // setup 6 line loops
      U32 vertexIndex = vert_pos;
      U32 idx;
      for(S32 i = 0; i < 6; i++)
      {
         idx = cubeFaces[i][0];
         mDebugVoxelVB[vertexIndex].point = cubePoints[idx] * halfSize;      
         mDebugVoxelVB[vertexIndex].color = mDebugVoxels[n].color.toColorI();
         vertexIndex++;

         idx = cubeFaces[i][1];
         mDebugVoxelVB[vertexIndex].point = cubePoints[idx] * halfSize;
         mDebugVoxelVB[vertexIndex].color = mDebugVoxels[n].color.toColorI();
         vertexIndex++;

         idx = cubeFaces[i][3];
         mDebugVoxelVB[vertexIndex].point = cubePoints[idx] * halfSize;
         mDebugVoxelVB[vertexIndex].color = mDebugVoxels[n].color.toColorI();
         vertexIndex++;

         idx = cubeFaces[i][1];
         mDebugVoxelVB[vertexIndex].point = cubePoints[idx] * halfSize;
         mDebugVoxelVB[vertexIndex].color = mDebugVoxels[n].color.toColorI();
         vertexIndex++;

         idx = cubeFaces[i][2];
         mDebugVoxelVB[vertexIndex].point = cubePoints[idx] * halfSize;
         mDebugVoxelVB[vertexIndex].color = mDebugVoxels[n].color.toColorI();
         vertexIndex++;

         idx = cubeFaces[i][3];
         mDebugVoxelVB[vertexIndex].point = cubePoints[idx] * halfSize;
         mDebugVoxelVB[vertexIndex].color = mDebugVoxels[n].color.toColorI();
         vertexIndex++;
      }

      // Apply position offset
      for ( U32 i = vert_pos; i < (vert_pos + 36); i++ )
         mDebugVoxelVB[i].point += mDebugVoxels[n].position;

      vert_pos += 36;
   }

   mDebugVoxelVB.unlock();
}

// Renders the visualization of the volume in the editor.
void OfflineLPV::_renderObject( ObjectRenderInst* ri, SceneRenderState* state, BaseMatInstance* overrideMat )
{
   Parent::_renderObject( ri, state, overrideMat );

   if (mDebugVoxels.size() < 1 || !(mShowVoxels || mShowDirectLight || mShowPropagated)) return;

   GFXStateBlockDesc desc;
   desc.setZReadWrite( true, true );
   desc.setBlend( false );
   desc.setFillModeSolid();

   GFX->setStateBlockByDesc( desc );
   GFX->setVertexBuffer( mDebugVoxelVB );
   GFX->setupGenericShaders();

   GFX->drawPrimitive( GFXTriangleList, 0, mDebugVoxels.size() * 12 );

   GFX->enterDebugEvent(ColorI(255, 0, 255), "OfflineLPV_DebugRender");

   if (!mDebugRender.wireMeshRender.bufferData.empty())
   {
      //first, we'll render the wire mesh data
      GFXStateBlockDesc desc;
      desc.setZReadWrite(true, false);
      desc.setBlend(true);
      desc.setCullMode(GFXCullNone);
      //desc.setZReadWrite( false, false );
      desc.fillMode = GFXFillWireframe;

      if (mDebugRender.wireMeshRender.bufferData.empty())
         return;

      GFXStateBlockRef sb = GFX->createStateBlock(desc);
      GFX->setStateBlock(sb);

      PrimBuild::color3i(255, 255, 255);

      Point3F *pnt;

      for (U32 b = 0; b < mDebugRender.wireMeshRender.bufferData.size(); b++)
      {
         U32 indexCnt = mDebugRender.wireMeshRender.bufferData[b].triCount * 3;
         PrimBuild::begin(GFXTriangleList, mDebugRender.wireMeshRender.bufferData[b].triCount * 3);

         U32 triCnt = mDebugRender.wireMeshRender.bufferData[b].triCount;
         for (U32 i = 0; i < mDebugRender.wireMeshRender.bufferData[b].triCount; i++)
         {
            pnt = &mDebugRender.wireMeshRender.bufferData[b].vertA[i];
            PrimBuild::vertex3fv(pnt);

            pnt = &mDebugRender.wireMeshRender.bufferData[b].vertB[i];
            PrimBuild::vertex3fv(pnt);

            pnt = &mDebugRender.wireMeshRender.bufferData[b].vertC[i];
            PrimBuild::vertex3fv(pnt);
         }

         PrimBuild::end();
      }
   }
}

//-----------------------------------------------------------------------------

void OfflineLPV::setTransform( const MatrixF& mat )
{
   Parent::setTransform( mat );
   mTransformDirty = true;
}

//-----------------------------------------------------------------------------

void OfflineLPV::buildSilhouette( const SceneCameraState& cameraState, Vector< Point3F >& outPoints )
{
   // Extract the silhouette of the polyhedron.  This works differently
   // depending on whether we project orthogonally or in perspective.

   TempAlloc< U32 > indices( mPolyhedron.getNumPoints() );
   U32 numPoints;

   if( cameraState.getFrustum().isOrtho() )
   {
      // Transform the view direction into object space.
      Point3F osViewDir;
      getWorldTransform().mulV( cameraState.getViewDirection(), &osViewDir );

      // And extract the silhouette.
      SilhouetteExtractorOrtho< PolyhedronType > extractor( mPolyhedron );
      numPoints = extractor.extractSilhouette( osViewDir, indices, indices.size );
   }
   else
   {
      // Create a transform to go from view space to object space.
      MatrixF camView( true );
      camView.scale( Point3F( 1.0f / getScale().x, 1.0f / getScale().y, 1.0f / getScale().z ) );
      camView.mul( getRenderWorldTransform() );
      camView.mul( cameraState.getViewWorldMatrix() );

      // Do a perspective-correct silhouette extraction.
      numPoints = mSilhouetteExtractor.extractSilhouette(
         camView,
         indices, indices.size );
   }

   // If we haven't yet, transform the polyhedron's points
   // to world space.
   if( mTransformDirty )
   {
      const U32 numPoints = mPolyhedron.getNumPoints();
      const PolyhedronType::PointType* points = getPolyhedron().getPoints();

      mWSPoints.setSize( numPoints );
      for( U32 i = 0; i < numPoints; ++ i )
      {
         Point3F p = points[ i ];
         p.convolve( getScale() );
         getTransform().mulP( p, &mWSPoints[ i ] );
      }

      mTransformDirty = false;
   }

   // Now store the points.
   outPoints.setSize( numPoints );
   for( U32 i = 0; i < numPoints; ++ i )
      outPoints[ i ] = mWSPoints[ indices[ i ] ];
}

//-----------------------------------------------------------------------------

U32 OfflineLPV::packUpdate( NetConnection *connection, U32 mask, BitStream *stream )
{
   U32 retMask = Parent::packUpdate( connection, mask, stream );

   // TODO: Should probably use a mask here rather than sending every update.
   stream->write(mVoxelSize);
   stream->write(mFileName);
   stream->writeFlag(mShowVoxels);
   stream->writeFlag(mShowDirectLight);
   stream->writeFlag(mShowPropagated);
   stream->writeFlag(mRenderReflection);

   stream->write(mLightIntensity);
   stream->write(mEmissiveIntensity);
   stream->write(mPropagationMultiplier);
   stream->write(mGIMultiplier);

   stream->writeFlag(mRegenVolume);
   if ( mRegenVolume )
      mRegenVolume = false;

   stream->writeFlag(mInjectLights);
   if ( mInjectLights )
      mInjectLights = false;

   stream->writeFlag(mPropagateLights);
   if ( mPropagateLights )
      mPropagateLights = false;

   stream->writeFlag(mExportPropagated);
   if ( mExportPropagated )
      mExportPropagated = false;

   stream->writeFlag(mExportDirectLight);
   if ( mExportDirectLight )
      mExportDirectLight = false;

   stream->writeFlag(mSaveResults);
   if ( mSaveResults )
      mSaveResults = false;

   stream->writeFlag(mLoadResults);
   if ( mLoadResults )
      mLoadResults = false;

   return retMask;  
}

void OfflineLPV::unpackUpdate( NetConnection *connection, BitStream *stream )
{
   Parent::unpackUpdate( connection, stream );

   stream->read(&mVoxelSize);
   stream->read(&mFileName);

   // Show geometry grid?
   bool old_show_voxels = mShowVoxels;
   mShowVoxels = stream->readFlag();
   if ( mShowVoxels != old_show_voxels ) _rebuildDebugVoxels();

   // Show direct light?
   bool old_show_light = mShowDirectLight;
   mShowDirectLight = stream->readFlag();
   if ( mShowDirectLight != old_show_light ) _rebuildDebugVoxels();

   // Show propagated?
   bool old_show_prop = mShowPropagated;
   mShowPropagated = stream->readFlag();
   if ( mShowPropagated != old_show_prop ) _rebuildDebugVoxels();

   mRenderReflection = stream->readFlag();

   stream->read(&mLightIntensity);
   stream->read(&mEmissiveIntensity);
   stream->read(&mPropagationMultiplier);
   stream->read(&mGIMultiplier);

   // Regen Volume Triggered?
   if ( stream->readFlag() )
      regenVolume();

   // Inject Lights Triggered?
   if ( stream->readFlag() )
      injectLights();

   // Propagate Lights Triggered?
   if ( stream->readFlag() )
   {
      Point3I voxelCount = getVoxelCount();

      if (!mPropagatedLightGridA)
         mPropagatedLightGridA = new SHVoxel[voxelCount.x * voxelCount.y * voxelCount.z];

      if (!mPropagatedLightGridB)
         mPropagatedLightGridB = new SHVoxel[voxelCount.x * voxelCount.y * voxelCount.z];

      switch(mPropagationStage)
      {
         case 0:
            mPropagatedLightGrid = mPropagatedLightGridA;
            propagateLights(mLightGrid, mPropagatedLightGrid, true);
            mPropagationStage = 1;
            break;

         case 1:
            mPropagatedLightGrid = mPropagatedLightGridB;
            propagateLights(mPropagatedLightGridA, mPropagatedLightGridB);
            mPropagationStage = 2;
            break;

         case 2:
            mPropagatedLightGrid = mPropagatedLightGridA;
            propagateLights(mPropagatedLightGridB, mPropagatedLightGridA);
            mPropagationStage = 1;
            break;
      }
   }

   // Export Propagated Light?
   if ( stream->readFlag() )
      exportPropagatedLight(mPropagatedLightGrid);

   // Export Direct Light?
   if ( stream->readFlag() )
      exportDirectLight(mLightGrid);

   // Save Results?
   if ( stream->readFlag() )
      save();

   // Load Results?
   if ( stream->readFlag() )
      load();
}

//-----------------------------------------------------------------------------

void OfflineLPV::inspectPostApply()
{
   Parent::inspectPostApply();
   setMaskBits(U32(-1));
}

//--- Scene Voxelization ---

bool OfflineLPV::_setRegenVolume( void *object, const char *index, const char *data )
{
   OfflineLPV* volume = reinterpret_cast< OfflineLPV* >( object );
   volume->mRegenVolume = true;
   return false;
}

Resource<GBitmap> OfflineLPV::getOrCreateTexture(FileName textureName)
{

   for (U32 i = 0; i< mTextureCache.size(); i++)
   {
      if (mTextureCache[i].mFileReference == textureName)
         return mTextureCache[i].mTexture;
   }

   Resource<GBitmap> texture = GBitmap::load(textureName);
   if (texture != NULL)
   {
      TextureCache entry;
      entry.mFileReference = textureName;
      entry.mTexture = texture;
      mTextureCache.push_back(entry);
   }

   return NULL;
}

void OfflineLPV::regenVolume()
{
   Box3F worldBox          = getWorldBox();
   Point3F bottom_corner   = worldBox.minExtents;
   Point3F top_corner      = worldBox.maxExtents;
   Point3F wsVoxelSize     = getWorldSpaceVoxelSize();
   Point3I voxelCount      = getVoxelCount();

   mTextureCache.clear();
   //get our editor status bar so we can keep tabs on progress
   GuiTextCtrl * statusBarGuiCtrl = dynamic_cast<GuiTextCtrl*>(Sim::findObject("EWorldEditorStatusBarInfo"));
   String statusBarGuiText = "";

   if (statusBarGuiCtrl)
   {
      statusBarGuiText = statusBarGuiCtrl->getText();
      statusBarGuiCtrl->setText("Beginning Regen of the LPV. Clearing old LPV data.");
   }

   RayInfo rayInfo;
   rayInfo.generateTexCoord = true;
   SceneContainer* container = getContainer();
   if ( !container ) return;

   // Allocate our grids.
   SAFE_DELETE(mGeometryGrid);
   SAFE_DELETE(mLightGrid);
   SAFE_DELETE(mPropagatedLightGridA);
   SAFE_DELETE(mPropagatedLightGridB);
   mGeometryGrid           = new GeometryVoxel[voxelCount.x * voxelCount.y * voxelCount.z];
   mLightGrid              = new SHVoxel[voxelCount.x * voxelCount.y * voxelCount.z];
   mPropagatedLightGridA   = new SHVoxel[voxelCount.x * voxelCount.y * voxelCount.z];
   mPropagatedLightGridB   = new SHVoxel[voxelCount.x * voxelCount.y * voxelCount.z];

   // Initialize the volume textures (if they aren't already.)
   _initVolumeTextures(voxelCount);

   mPropagationStage = 0;
   mPropagatedLightGrid = mPropagatedLightGridA;

   mDebugVoxels.clear();

   //Step one, we poll the scene container and find all objects that overlap with us
   SphereF sphere;
   sphere.center = (worldBox.minExtents + worldBox.maxExtents) * 0.5;
   VectorF bv = worldBox.maxExtents - sphere.center;
   sphere.radius = bv.len();

   if (statusBarGuiCtrl)
   {
      statusBarGuiCtrl->setText("Voxelizing Static Geometry. 0% complete.");
   }

   S32 processedTris = 0;
   S32 totalTris = 0;

   Vector<OptimizedPolyList> polyLists;

   SimpleQueryList sql;
   container->findObjects(worldBox, STATIC_COLLISION_TYPEMASK, SimpleQueryList::insertionCallback, &sql);
   for (U32 i = 0; i < sql.mList.size(); i++)
   {
      OptimizedPolyList polyList;

      sql.mList[i]->buildPolyList(PLC_Export, &polyList, worldBox, sphere);

      polyLists.push_back(polyList);

      totalTris += polyList.mPolyList.size();
   }

   for (U32 i = 0; i < polyLists.size(); i++)
   {
      if (!polyLists[i].isEmpty())
      {
         Vector<U32> tempIndices;
         tempIndices.reserve(4);

         for (U32 j = 0; j < polyLists[i].mPolyList.size(); j++)
         {
            const OptimizedPolyList::Poly& poly = polyLists[i].mPolyList[j];

            if (poly.vertexCount < 3)
               continue;

            tempIndices.setSize(poly.vertexCount);
            dMemset(tempIndices.address(), 0, poly.vertexCount);

            if (poly.type == OptimizedPolyList::TriangleStrip||
               poly.type == OptimizedPolyList::TriangleFan)
            {
               tempIndices[0] = 0;
               U32 idx = 1;

               for (U32 k = 1; k < poly.vertexCount; k += 2)
                  tempIndices[idx++] = k;

               for (U32 k = ((poly.vertexCount - 1) & (~0x1)); k > 0; k -= 2)
                  tempIndices[idx++] = k;
            }
            else if (poly.type == OptimizedPolyList::TriangleList)
            {
               for (U32 k = 0; k < poly.vertexCount; k++)
                  tempIndices[k] = k;
            }
            else
               continue;

            const U32& firstIdx = polyLists[i].mIndexList[poly.vertexStart];
            const OptimizedPolyList::VertIndex& firstVertIdx = polyLists[i].mVertexList[firstIdx];

            for (U32 k = 1; k < poly.vertexCount - 1; k++)
            {
               const U32& secondIdx = polyLists[i].mIndexList[poly.vertexStart + tempIndices[k]];
               const U32& thirdIdx = polyLists[i].mIndexList[poly.vertexStart + tempIndices[k + 1]];

               const OptimizedPolyList::VertIndex& secondVertIdx = polyLists[i].mVertexList[secondIdx];
               const OptimizedPolyList::VertIndex& thirdVertIdx = polyLists[i].mVertexList[thirdIdx];

               Point3F vertA = polyLists[i].mPoints[firstVertIdx.vertIdx];
               Point3F vertB = polyLists[i].mPoints[secondVertIdx.vertIdx];
               Point3F vertC = polyLists[i].mPoints[thirdVertIdx.vertIdx];
               
               Point2F uvA = polyLists[i].mUV0s[firstVertIdx.uv0Idx];
               Point2F uvB = polyLists[i].mUV0s[secondVertIdx.uv0Idx];
               Point2F uvC = polyLists[i].mUV0s[thirdVertIdx.uv0Idx];

               //First, test if any of them are contained inside. If they are, we're done
               if (worldBox.isContained(vertA) || worldBox.isContained(vertB) || worldBox.isContained(vertC))
               {
                  //can only do 65000 verts in a call, so split it up
                  if (mDebugRender.wireMeshRender.bufferData.empty() || mDebugRender.wireMeshRender.bufferData.last().triCount >= 21666)
                  {
                     DebugRenderStash::WireMeshData::data newBufferData;
                     mDebugRender.wireMeshRender.bufferData.push_back(newBufferData);
                  }

                  mDebugRender.wireMeshRender.bufferData.last().triCount++;
                  mDebugRender.wireMeshRender.bufferData.last().vertA.push_back(vertA);
                  mDebugRender.wireMeshRender.bufferData.last().vertB.push_back(vertB);
                  mDebugRender.wireMeshRender.bufferData.last().vertC.push_back(vertC);

                  PlaneF triPlane = PlaneF(vertA, vertB, vertC);

                  Point3F triCenter = (vertA + vertB + vertC) / 3;

                  Box3F triBox;

                  Vector<Point3F> verts;
                  verts.push_back(vertA);
                  verts.push_back(vertB);
                  verts.push_back(vertC);

                  triBox = Box3F::aroundPoints(verts.address(), verts.size());

                  //get the voxels our tri's bounds overlap
                  Point3I minExtIdx = getVoxel(triBox.minExtents);
                  Point3I maxExtIdx = getVoxel(triBox.maxExtents);

                  U32 xVoxCount = mAbs(maxExtIdx.x - minExtIdx.x);
                  U32 yVoxCount = mAbs(maxExtIdx.y - minExtIdx.y);
                  U32 zVoxCount = mAbs(maxExtIdx.z - minExtIdx.z);

                  xVoxCount = xVoxCount > 0 ? xVoxCount : 1;
                  yVoxCount = yVoxCount > 0 ? yVoxCount : 1;
                  zVoxCount = zVoxCount > 0 ? zVoxCount : 1;

                  U32 xStart = maxExtIdx.x > minExtIdx.x ? minExtIdx.x : maxExtIdx.x;
                  U32 yStart = maxExtIdx.y > minExtIdx.y ? minExtIdx.y : maxExtIdx.y;
                  U32 zStart = maxExtIdx.z > minExtIdx.z ? minExtIdx.z : maxExtIdx.z;

                  //now, iterate through the smaller subset of voxels that this encompasses, and test them. Early out if it isn't valid
                  for (U32 x = xStart; x < xStart + xVoxCount; x++)
                  {
                     for (U32 y = yStart; y < yStart + yVoxCount; y++)
                     {
                        for (U32 z = zStart; z < zStart + zVoxCount; z++)
                        {
                           bool newVoxel = false;

                           S32 voxIndex = getVoxelIndex(x, y, z);
                           if (voxIndex == -1)
                              continue;

                           LinearColorF voxel_color = LinearColorF(0, 0, 0, 0);
                           bool emissive = false;

                           Box3F voxBox = Box3F(bottom_corner + Point3F(mVoxelSize * x, mVoxelSize * y, mVoxelSize * z),
                              bottom_corner + Point3F(mVoxelSize * (x + 1), mVoxelSize * (y + 1), mVoxelSize * (z + 1)));

                           if (triPlane.whichSide(voxBox) == PlaneF::Side::On)
                           {
                              //early out test. If the box containts any or all of the verts, it's assumed to intersect
                              bool containsVertA = voxBox.isContained(vertA);
                              bool containsVertB = voxBox.isContained(vertB);
                              bool containsVertC = voxBox.isContained(vertC);
                              if (containsVertA || containsVertB || containsVertC)
                              {
                                 voxel_color = LinearColorF(255, 255, 255, 255);
                                 if ( poly.material > -1 )
                                 {
                                    Point2F uv;
                                    if ( containsVertA ) uv = uvA;
                                    if ( containsVertB ) uv = uvB;
                                    if ( containsVertC ) uv = uvC;

                                    Material* mat = dynamic_cast<Material*>(polyLists[i].mMaterialList[poly.material]->getMaterial());
                                    if (mat)
                                    {
                                       Resource<GBitmap> diffuseTex = getOrCreateTexture(mat->mDiffuseMapFilename[0]);
                                       if (diffuseTex != NULL)
                                       {
                                          voxel_color = diffuseTex->sampleTexel(uv.x, uv.y)*mat->mDiffuse[0];
                                       } else {
                                          voxel_color = mat->mDiffuse[0];
                                       }

                                       emissive = mat->mEmissive[0];
                                    }
                                 }

                                 U32 voxelIdx = getVoxelIndex(x, y, z);
                                 mGeometryGrid[voxelIdx].color = voxel_color;
                              }
                              else
                              {
                                 //first, test if any of the tri edges intersect the box
                                 F32 collideAPos;
                                 Point3F collideANorm;
                                 bool collideA = voxBox.collideLine(vertA, vertB, &collideAPos, &collideANorm);

                                 F32 collideBPos;
                                 Point3F collideBNorm;
                                 bool collideB = voxBox.collideLine(vertB, vertC, &collideBPos, &collideBNorm);

                                 F32 collideCPos;
                                 Point3F collideCNorm;
                                 bool collideC = voxBox.collideLine(vertC, vertA, &collideCPos, &collideCNorm);

                                 if (collideA || collideB || collideC)
                                 {
                                    voxel_color = LinearColorF(255, 255, 255, 255);
                                    if ( poly.material > -1 )
                                    {
                                       Point2F uv;
                                       if ( collideA ) uv = uvA + (collideAPos * (uvB - uvA));
                                       if ( collideB ) uv = uvB + (collideBPos * (uvC - uvB));
                                       if ( collideC ) uv = uvC + (collideCPos * (uvA - uvC));

                                       Material* mat = dynamic_cast<Material*>(polyLists[i].mMaterialList[poly.material]->getMaterial());
                                       if (mat)
                                       {
                                          Resource<GBitmap> diffuseTex = getOrCreateTexture(mat->mDiffuseMapFilename[0]);
                                          if (diffuseTex != NULL)
                                          {
                                             voxel_color = diffuseTex->sampleTexel(uv.x, uv.y)*mat->mDiffuse[0];
                                          } else {
                                             voxel_color = mat->mDiffuse[0];
                                          }

                                          emissive = mat->mEmissive[0];
                                       }
                                    }

                                    //indeed it does
                                    U32 voxelIdx = getVoxelIndex(x, y, z);
                                    mGeometryGrid[voxelIdx].color = voxel_color;
                                    mGeometryGrid[voxelIdx].emissive = emissive;
                                 }
                                 else
                                 {
                                    //it doesn't, so we have to test if the voxel intersects the tri(like in cases where the triangle is larger than the voxel)
                                    //we do this second because it requires more tests and is thus slower.
                                    for (U32 e = 0; e < BoxBase::Edges::NUM_EDGES; e++)
                                    {
                                       BoxBase::Points a, b;
                                       voxBox.getEdgePointIndices((BoxBase::Edges)e, a, b);

                                       Point3F edgeA = voxBox.computeVertex(a);
                                       Point3F edgeB = voxBox.computeVertex(b);

                                       Point3F intersect;

                                       if (triPlane.clipSegment(edgeA, edgeB, intersect))
                                          //if (MathUtils::mLineTriangleCollide(edgeA, edgeB, vertA, vertB, vertC, NULL, &t))
                                       {
                                          if (intersect == edgeA || intersect == edgeB)
                                             continue;

                                          voxel_color = LinearColorF(255, 255, 255, 255);
                                          if ( poly.material > -1 )
                                          {
                                             VectorF f1 = vertA - intersect;
                                             VectorF f2 = vertB - intersect;
                                             VectorF f3 = vertC - intersect;

                                             F32 a =  mCross(vertA - vertB, vertA - vertC).magnitudeSafe();
                                             F32 a1 = mCross(f2, f3).magnitudeSafe() / a;
                                             F32 a2 = mCross(f3, f1).magnitudeSafe() / a;
                                             F32 a3 = mCross(f1, f2).magnitudeSafe() / a;

                                             Point2F uv = (uvA * a1) + (uvB * a2) + (uvC * a3);

                                             Material* mat = dynamic_cast<Material*>(polyLists[i].mMaterialList[poly.material]->getMaterial());
                                             if (mat)
                                             {
                                                Resource<GBitmap> diffuseTex = getOrCreateTexture(mat->mDiffuseMapFilename[0]);
                                                if (diffuseTex != NULL)
                                                {
                                                   voxel_color = diffuseTex->sampleTexel(uv.x, uv.y)*mat->mDiffuse[0];
                                                } else {
                                                   voxel_color = mat->mDiffuse[0];
                                                }

                                                emissive = mat->mEmissive[0];
                                             }
                                          }

                                          U32 voxelIdx = getVoxelIndex(x, y, z);
                                          mGeometryGrid[voxelIdx].color = voxel_color;
                                          mGeometryGrid[voxelIdx].emissive = emissive;
                                          break;
                                       }
                                    }
                                 }
                              }
                           }
                        }
                     }
                  }
               }
            }

            processedTris++;

            if (statusBarGuiCtrl)
            {
               char buff[256];
               F32 percetile = processedTris / totalTris;
               dSprintf(buff, 256, "Voxelizing Static Geometry. %g % complete.", percetile);
               statusBarGuiCtrl->setText(buff);
            }
         }
      }
   }

   if (statusBarGuiCtrl)
   {
       statusBarGuiCtrl->setText(statusBarGuiText);
   }

   mTextureCache.clear();
   //_rebuildDebugVoxels();
}

Point3I OfflineLPV::getVoxel(Point3F position)
{
   Box3F worldBox = getWorldBox();
   Point3F center = worldBox.getCenter();
   Point3F bottom_corner = worldBox.minExtents;
   Point3F top_corner = worldBox.maxExtents;

   Point3F offset = position - bottom_corner;

   Point3I index = Point3I(mAbs(offset.x / mVoxelSize), mAbs(offset.y / mVoxelSize), mAbs(offset.z / mVoxelSize));

   Box3F voxBox = Box3F(bottom_corner + Point3F(mVoxelSize * index.x, mVoxelSize * index.y, mVoxelSize * index.z),
      bottom_corner + Point3F(mVoxelSize * (index.x + 1), mVoxelSize * (index.y + 1), mVoxelSize * (index.z + 1)));

   bool correct = voxBox.isContained(position);

   if (!voxBox.isContained(position))
      return Point3I(-1, -1, -1);

   return index;
}

//--- Light Injection ---

Point3F OfflineLPV::getNearestFaceNormals(Point3F dir, bool nonOccludedOnly, U32 x, U32 y, U32 z)
{
   Point3F face_normals[6] = {Point3F(1, 0, 0), Point3F(-1, 0, 0), Point3F(0, 1, 0), Point3F(0, -1, 0), Point3F(0, 0, 1), Point3F(0, 0, -1)};

   F32 lowest_value = 0;
   S32 chosen_face = -1;
   for(U32 i = 0; i < 6; ++i)
   {
      F32 dp = mDot(dir, face_normals[i]);
      if ( dp <= lowest_value )
      {
         if ( nonOccludedOnly )
         {
            Point3I voxelCount = getVoxelCount();
            U32 sampleOffset = (voxelCount.x * voxelCount.y * (z + face_normals[i].z)) + (voxelCount.x * (y + face_normals[i].y)) + (x + face_normals[i].x);
            if ( sampleOffset < (voxelCount.x * voxelCount.y * voxelCount.z) )
            {
               if ( mGeometryGrid[sampleOffset].color.red > 0.0f || 
                    mGeometryGrid[sampleOffset].color.green > 0.0f || 
                    mGeometryGrid[sampleOffset].color.blue > 0.0f )
                  continue;
            }
         }

         chosen_face = i;
         lowest_value = dp;
      }
   }

   if ( chosen_face == -1 )
   {
      return Point3F(0, 0, 0);
   }

   return face_normals[chosen_face];
}

Point3F OfflineLPV::reflect(Point3F v, Point3F n)
{
   // R = V - 2N(V dot N)
   F32 vDotN = mDot(v, n);
   return v - (2 * (n * vDotN));
}

bool OfflineLPV::_setInjectLights( void *object, const char *index, const char *data )
{
   OfflineLPV* volume = reinterpret_cast< OfflineLPV* >( object );
   volume->mInjectLights = true;
   return false;
}

void OfflineLPV::injectLights()
{
   if ( !mGeometryGrid || !mLightGrid ) return;

   Box3F worldBox          = getWorldBox();
   Point3F bottom_corner   = worldBox.minExtents;
   Point3F top_corner      = worldBox.maxExtents;
   Point3F wsVoxelSize     = getWorldSpaceVoxelSize();
   Point3I voxelCount      = getVoxelCount();

   // Inject lights from the scene into the light grid.
   U32 pos = 0;
   for ( U32 z = 0; z < voxelCount.z; z++ )
   {
      for ( U32 y = 0; y < voxelCount.y; y++ )
      {
         for ( U32 x = 0; x < voxelCount.x; x++ )
         {
            if ( mGeometryGrid[pos].color.alpha > 0.0f )
            {
               Point3F point = bottom_corner + Point3F(wsVoxelSize.x * (x + 1), wsVoxelSize.y * (y + 1), wsVoxelSize.z * (z + 1));
               
               mLightGrid[pos] = calcSHLights(point, mGeometryGrid[pos].color, Point3I(x, y, z), mGeometryGrid[pos].emissive);

               mPropagationStage = 0;
            }
            pos++;
         }
      }
   }

   _rebuildDebugVoxels();
}

OfflineLPV::SHVoxel OfflineLPV::calcSHLights(Point3F position, LinearColorF geometryColor, Point3I voxelPosition, bool emissive)
{
   PROFILE_SCOPE( OfflineLPV_calcLightColor );

   SHVoxel result;

   if ( emissive ) 
   {
      SHVoxel encoded = encodeSH(Point3F(0, 0, 0), geometryColor * mEmissiveIntensity);
      result.red = encoded.red;
      result.green = encoded.green;
      result.blue = encoded.blue;
   } else {
      result.red = LinearColorF::ZERO;
      result.green = LinearColorF::ZERO;
      result.blue = LinearColorF::ZERO;
   }

   if ( !LIGHTMGR )
      return result;

   RayInfo rayInfo;
   SceneContainer* container = getContainer();
   if ( !container ) return result;

   Vector<SceneObject*> lightObjects;
   const U32 lightMask = LightObjectType;
   container->findObjectList( lightMask, &lightObjects );

   Vector<SceneObject*>::iterator iter = lightObjects.begin();
   for ( ; iter != lightObjects.end(); iter++ )
   {
      LightInfo* info = NULL;
      SceneObject *lightObject = (*iter);
      ISceneLight* light = dynamic_cast<ISceneLight*>(lightObject);
      if ( !light ) continue;

      info = light->getLight();
      if ( !info ) continue;
      
      // Sun has special positioning.
      Point3F lightPosition = lightObject->getPosition();
      if ( info->getType() == LightInfo::Type::Vector )
      {
         VectorF sunVector = info->getDirection();
         lightPosition.set(-10000, -10000, -10000);
         lightPosition *= sunVector;
      } 

      bool hit = container->castRay(position, lightPosition, STATIC_COLLISION_TYPEMASK, &rayInfo);
      if ( hit )
      {
         if ( rayInfo.material )
         {
            Material* mat = dynamic_cast<Material*>(rayInfo.material->getMaterial());
            if ( mat && mat->isTranslucent() )
               hit = false;
         }

         if ( hit )
            continue;
      }
      
      if ( !hit )
      {
         Point3F direction = lightPosition - position;
         direction.normalize();
         direction *= -1;

         // Point Light
         if ( info->getType() == LightInfo::Type::Point )
         {
            F32 atten = getAttenuation(info, position);
            if ( atten <= 0 ) continue;

            Point3F normal = getNearestFaceNormals(direction, true, voxelPosition.x, voxelPosition.y, voxelPosition.z);
            Point3F reflected = reflect(direction, normal);
         
            SHVoxel encoded = encodeSH(reflected, geometryColor * info->getColor() * atten * mLightIntensity);
            result.red += encoded.red;
            result.green += encoded.green;
            result.blue += encoded.blue;
            result.normal = normal;
         }

         // Spot Light
         if ( info->getType() == LightInfo::Type::Spot )
         {
            // TODO
         }

         // Sun Light
         if ( info->getType() == LightInfo::Type::Vector )
         {
            Point3F normal = getNearestFaceNormals(direction, true, voxelPosition.x, voxelPosition.y, voxelPosition.z);
            Point3F reflected = reflect(direction, normal);

            SHVoxel encoded = encodeSH(reflected, geometryColor * info->getColor() * info->getBrightness() * mLightIntensity);
            result.red += encoded.red;
            result.green += encoded.green;
            result.blue += encoded.blue;
            result.normal = normal;
         }
      }
   }

   return result;
}

F32 OfflineLPV::getAttenuation(LightInfo* lightInfo, Point3F position)
{
   // Get the attenuation falloff ratio and normalize it.
   Point3F attenRatio = lightInfo->getExtended<ShadowMapParams>()->attenuationRatio;
   F32 total = attenRatio.x + attenRatio.y + attenRatio.z;
   if ( total > 0.0f )
      attenRatio /= total;

   const F32 radius = lightInfo->getRange().x;
   const F32 dist = Point3F(lightInfo->getPosition() - position).len();

   Point2F attenParams( ( 1.0f / radius ) * attenRatio.y,
                        ( 1.0f / ( radius * radius ) ) * attenRatio.z );

   return 1.0 - mDot( attenParams, Point2F( dist, dist * dist ) );
}

//--- Light Propagation ---

Vector4F OfflineLPV::getClampedCosineSHCoeffs(Point3F dir)
{
   // Spherical Harmonic Constants from GPU Pro 4
   //F32 SHConstant0 = M_PI_F / (2.0 * mSqrt( M_PI_F ));
   //F32 SHConstant1 = -((2.0 * M_PI_F)/3.0) * mSqrt( 3 / (4.0 * M_PI_F) );
   //F32 SHConstant2 =  ((2.0 * M_PI_F)/3.0) * mSqrt( 3 / (4.0 * M_PI_F) );
   //F32 SHConstant3 = -((2.0 * M_PI_F)/3.0) * mSqrt( 3 / (4.0 * M_PI_F) );

   // Spherical Harmonic Constants from Crytek LPV Paper
   //F32 SHConstant0 =         1.0f / (2.0f * mSqrt( M_PI_F ));
   //F32 SHConstant1 = -mSqrt(3.0f) / (2.0f * mSqrt( M_PI_F ));
   //F32 SHConstant2 =  mSqrt(3.0f) / (2.0f * mSqrt( M_PI_F ));
   //F32 SHConstant3 = -mSqrt(3.0f) / (2.0f * mSqrt( M_PI_F ));

   // From: http://blog.blackhc.net/2010/07/light-propagation-volumes/
   F32 SHConstant0 = 0.886226925f;
   F32 SHConstant1 = -1.02332671f;
   F32 SHConstant2 = 1.02332671f;
   F32 SHConstant3 = -1.02332671f;

   Vector4F coeffs(SHConstant0, SHConstant1, SHConstant2, SHConstant3);
   coeffs.w *= dir.x;
   coeffs.y *= dir.y;
   coeffs.z *= dir.z;
    
   return coeffs;
}

OfflineLPV::SHVoxel OfflineLPV::encodeSH(Point3F dir, LinearColorF color)
{
   SHVoxel result;
   Vector4F coeffs = getClampedCosineSHCoeffs(dir);
   coeffs = coeffs / M_PI_F;
   result.red.set(coeffs.x * color.red, coeffs.y * color.red, coeffs.z * color.red, coeffs.w * color.red);
   result.green.set(coeffs.x * color.green, coeffs.y * color.green, coeffs.z * color.green, coeffs.w * color.green);
   result.blue.set(coeffs.x * color.blue, coeffs.y * color.blue, coeffs.z * color.blue, coeffs.w * color.blue);

   return result;
}

LinearColorF OfflineLPV::decodeSH(Point3F dir, SHVoxel sh)
{
   Vector4F coeffs = getClampedCosineSHCoeffs(dir); 
   LinearColorF color( 0, 0, 0 );
    
   color.red = mDot(sh.red, coeffs);
   color.green = mDot(sh.green, coeffs);
   color.blue = mDot(sh.blue, coeffs);
   return color;
}

bool OfflineLPV::_setPropagateLights( void *object, const char *index, const char *data )
{
   OfflineLPV* volume = reinterpret_cast< OfflineLPV* >( object );
   volume->mPropagateLights = true;
   return false;
}

void OfflineLPV::propagateLights(SHVoxel* source, SHVoxel* dest, bool sampleFromGeometry)
{
   if ( !mGeometryGrid || !source || !dest ) return;

   Point3I voxelCount = getVoxelCount();
   Point3F sample_directions[6] = {Point3F(1, 0, 0), Point3F(-1, 0, 0), Point3F(0, 1, 0), Point3F(0, -1, 0), Point3F(0, 0, 1), Point3F(0, 0, -1)};

   U32 pos = 0;
   for ( U32 z = 0; z < voxelCount.z; z++ )
   {
      for ( U32 y = 0; y < voxelCount.y; y++ )
      {
         for ( U32 x = 0; x < voxelCount.x; x++ )
         {
            /* Spherical Harmonic Propagation */

            // Clear the cell, since this could be a reused grid.
            dest[pos].red = LinearColorF::ZERO;
            dest[pos].green = LinearColorF::ZERO;
            dest[pos].blue = LinearColorF::ZERO;

            U32 blends = 0;
            // Sample from 6 faces of the cube and sum the attenuated results
            for ( U32 i = 0; i < 6; i++ )
            {
               S32 zSample = z + sample_directions[i].z;
               S32 ySample = y + sample_directions[i].y;
               S32 xSample = x + sample_directions[i].x;

               if (zSample < 0 || zSample >= voxelCount.z ||
                  ySample < 0 || ySample >= voxelCount.y ||
                  xSample < 0 || xSample >= voxelCount.x)
                  continue;

               // Determine which voxel we'll be sampling from.
               U32 sampleOffset = (voxelCount.x * voxelCount.y * zSample) + (voxelCount.x * ySample) + xSample;

               // Decode the color from that voxel facing the opposite direction we sampled from.
               LinearColorF decoded_color = decodeSH(sample_directions[i] * -1, source[sampleOffset]);

               // Encode that color to be added into our grid.
               SHVoxel encoded_color = encodeSH(sample_directions[i] * -1, decoded_color * mPropagationMultiplier);

               // Add spherical harmonics.
               dest[pos].red     += encoded_color.red;
               dest[pos].green   += encoded_color.green;
               dest[pos].blue    += encoded_color.blue;
               blends++;
            }
            dest[pos].red /= blends;
            dest[pos].green /= blends;
            dest[pos].blue /= blends;

            pos++;
         }
      }
   }

   _rebuildDebugVoxels();
}

//--- Export from CPU to GPU for rendering ---

bool OfflineLPV::_setExportPropagated( void *object, const char *index, const char *data )
{
   OfflineLPV* volume = reinterpret_cast< OfflineLPV* >( object );
   volume->mExportPropagated = true;
   return false;
}

// Exports mPropagatedLightGrid to the actual volume texture on the graphics card.
// Note: for some reason the volume texture insists on being BGRA.
void OfflineLPV::exportPropagatedLight(LinearColorF* pSource, Point3I* pSize)
{
   if ( !mPropagatedLightGrid && !pSource ) return;
   Point3I size = pSize ? *pSize : getVoxelCount();

   GFXLockedRect* locked_rect = mPropagatedTexture->lock();
   if ( locked_rect )
   {
      U8* buffer = new U8[size.x * size.y * size.z * 4];
      U32 pos = 0;
      U32 bufPos = 0;
      for(U32 z = 0; z < size.z; z++)
      {
         for(U32 y = 0; y < size.y; y++)
         {
            for(U32 x = 0; x < size.x; x++)
            {
               ColorI cell_color = LinearColorF(pSource[pos] * mGIMultiplier).toColorI();
               pos++;

               buffer[bufPos]     = cell_color.blue;    // Blue
               buffer[bufPos + 1] = cell_color.green;   // Green
               buffer[bufPos + 2] = cell_color.red;     // Red
               buffer[bufPos + 3] = 255;                // Alpha
               bufPos += 4;
            }
         }
      }
      dMemcpy(locked_rect->bits, buffer, size.x * size.y * size.z * 4 * sizeof(U8));
      mPropagatedTexture->unlock();
      SAFE_DELETE(buffer);
   }
}

void OfflineLPV::exportPropagatedLight(SHVoxel* pSource, Point3I* pSize)
{
   if ( !mPropagatedLightGrid && !pSource ) return;
   Point3I size = pSize ? *pSize : getVoxelCount();

   GFXLockedRect* locked_rect = mPropagatedTexture->lock();
   if ( locked_rect )
   {
      U8* buffer = new U8[size.x * size.y * size.z * 4];
      U32 pos = 0;
      U32 bufPos = 0;
      for(U32 z = 0; z < size.z; z++)
      {
         for(U32 y = 0; y < size.y; y++)
         {
            for(U32 x = 0; x < size.x; x++)
            {
               ColorI cell_color = LinearColorF(decodeSH(Point3F(0, 0, 0), pSource[pos]) * mGIMultiplier).toColorI();
               pos++;

               buffer[bufPos]     = cell_color.blue;    // Blue
               buffer[bufPos + 1] = cell_color.green;   // Green
               buffer[bufPos + 2] = cell_color.red;     // Red
               buffer[bufPos + 3] = 255;                // Alpha
               bufPos += 4;
            }
         }
      }
      dMemcpy(locked_rect->bits, buffer, size.x * size.y * size.z * 4 * sizeof(U8));
      mPropagatedTexture->unlock();
      SAFE_DELETE(buffer);
   }
}

bool OfflineLPV::_setExportDirectLight( void *object, const char *index, const char *data )
{
   OfflineLPV* volume = reinterpret_cast< OfflineLPV* >( object );
   volume->mExportDirectLight = true;
   return false;
}

// Exports mLightGrid to the actual volume texture on the graphics card.
// Note: for some reason the volume texture insists on being BGRA.
void OfflineLPV::exportDirectLight(LinearColorF* pSource, Point3I* pSize)
{
   if ( !mLightGrid && !pSource ) return;
   Point3I size = pSize ? *pSize : getVoxelCount();

   GFXLockedRect* locked_rect = mDirectLightTexture->lock();
   if ( locked_rect )
   {
      U8* buffer = new U8[size.x * size.y * size.z * 4];
      U32 pos = 0;
      U32 bufPos = 0;
      for(U32 z = 0; z < size.z; z++)
      {
         for(U32 y = 0; y < size.y; y++)
         {
            for(U32 x = 0; x < size.x; x++)
            {
               ColorI cell_color = pSource[pos].toColorI();
               pos++;

               buffer[bufPos]     = cell_color.blue;    // Blue
               buffer[bufPos + 1] = cell_color.green;   // Green
               buffer[bufPos + 2] = cell_color.red;     // Red
               buffer[bufPos + 3] = 255;                // Alpha
               bufPos += 4;
            }
         }
      }
      dMemcpy(locked_rect->bits, buffer, size.x * size.y * size.z * 4 * sizeof(U8));
      mDirectLightTexture->unlock();
      SAFE_DELETE(buffer);
   }
}

void OfflineLPV::exportDirectLight(SHVoxel* pSource, Point3I* pSize)
{
   if ( !mLightGrid && !pSource ) return;
   Point3I size = pSize ? *pSize : getVoxelCount();

   GFXLockedRect* locked_rect = mDirectLightTexture->lock();
   if ( locked_rect )
   {
      U8* buffer = new U8[size.x * size.y * size.z * 4];
      U32 pos = 0;
      U32 bufPos = 0;
      for(U32 z = 0; z < size.z; z++)
      {
         for(U32 y = 0; y < size.y; y++)
         {
            for(U32 x = 0; x < size.x; x++)
            {
               ColorI cell_color = LinearColorF(decodeSH(Point3F(0, 0, 0), pSource[pos])).toColorI();
               pos++;

               buffer[bufPos]     = cell_color.blue;    // Blue
               buffer[bufPos + 1] = cell_color.green;   // Green
               buffer[bufPos + 2] = cell_color.red;     // Red
               buffer[bufPos + 3] = 255;                // Alpha
               bufPos += 4;
            }
         }
      }
      dMemcpy(locked_rect->bits, buffer, size.x * size.y * size.z * 4 * sizeof(U8));
      mDirectLightTexture->unlock();
      SAFE_DELETE(buffer);
   }
}

//--- Final Volume Rendering Into LightBuffer ---

void OfflineLPV::_initVolumeTextures(Point3I volumeSize)
{
   // We only perform this function if the voxel count changes in any direction.
   if ( mCachedVolumeSize == volumeSize )
      return;

   // Initialize the shaders if they're null.
   if ( mPropagatedShader.isNull() || mReflectShader.isNull() )
      _initShaders();

   // Initialize the volume texture with all black to begin with.
   U8* buffer = new U8[volumeSize.x * volumeSize.y * volumeSize.z * 4];
   U32 pos = 0;
   for(U32 x = 0; x < volumeSize.x; x++)
   {
      for(U32 y = 0; y < volumeSize.y; y++)
      {
         for(U32 z = 0; z < volumeSize.z; z++)
         {
            buffer[pos]     = 0;   // Blue
            buffer[pos + 1] = 0;   // Green
            buffer[pos + 2] = 0;   // Red
            buffer[pos + 3] = 0;   // Alpha
            pos += 4;
         }
      }
   }

   // 
   mPropagatedTexture.set(volumeSize.x, volumeSize.y, volumeSize.z, &buffer[0], GFXFormat::GFXFormatR8G8B8A8, &LPVProfile, "OfflineLPV_Propagated");
   mDirectLightTexture.set(volumeSize.x, volumeSize.y, volumeSize.z, &buffer[0], GFXFormat::GFXFormatR8G8B8A8, &LPVProfile, "OfflineLPV_DirectLight");

   // Clean up buffer.
   SAFE_DELETE(buffer);
}

void OfflineLPV::_initShaders()
{
   mPropagatedShader       = NULL;
   mPropagatedShaderConsts = NULL;
   mReflectShader          = NULL;
   mReflectShaderConsts    = NULL;
   mPrepassTarget          = NULL;

   // Need depth from pre-pass, so get the macros
   Vector<GFXShaderMacro> macros;

   if ( !mPrepassTarget )
      mPrepassTarget = NamedTexTarget::find( "deferred" );

   if ( mPrepassTarget )
      mPrepassTarget->getShaderMacros( &macros );

   ShaderData *shaderData;

   // Load Propagated Display Shader
   if ( !Sim::findObject( "OfflineLPVPropagatedShaderData", shaderData ) )
   {
      Con::warnf( "OfflineLPV::_initShader - failed to locate shader OfflineLPVPropagatedShaderData!" );
      return;
   }

   // SSAO MASK
   if ( AdvancedLightBinManager::smUseSSAOMask )
      macros.push_back( GFXShaderMacro( "USE_SSAO_MASK" ) );

   mPropagatedShader = shaderData->getShader( macros );
   if ( !mPropagatedShader )
      return;
   
   mPropagatedShaderConsts = mPropagatedShader->allocConstBuffer();
   mEyePosWorldPropSC      = mPropagatedShader->getShaderConstHandle( "$eyePosWorld" );
   mRTParamsPropSC         = mPropagatedShader->getShaderConstHandle( "$rtParams0" );
   mVolumeStartPropSC      = mPropagatedShader->getShaderConstHandle( "$volumeStart" );
   mVolumeSizePropSC       = mPropagatedShader->getShaderConstHandle( "$volumeSize" );

   // Load Reflection Shader
   if ( !Sim::findObject( "OfflineLPVReflectShaderData", shaderData ) )
   {
      Con::warnf( "OfflineLPV::_initShader - failed to locate shader OfflineLPVReflectShaderData!" );
      return;
   }

   mReflectShader = shaderData->getShader( macros );
   if ( !mReflectShader )
      return ;

   mReflectShaderConsts    = mReflectShader->allocConstBuffer();
   mInverseViewReflectSC   = mReflectShader->getShaderConstHandle( "$invViewMat" );
   mEyePosWorldReflectSC   = mReflectShader->getShaderConstHandle( "$eyePosWorld" );
   mRTParamsReflectSC      = mReflectShader->getShaderConstHandle( "$rtParams0" );
   mVolumeStartReflectSC   = mReflectShader->getShaderConstHandle( "$volumeStart" );
   mVolumeSizeReflectSC    = mReflectShader->getShaderConstHandle( "$volumeSize" );
}

void OfflineLPV::_handleBinEvent(   RenderBinManager *bin,                           
                  const SceneRenderState* sceneState,
                  bool isBinStart )
{

   // We require a bin name to process effects... without
   // it we can skip the bin entirely.
   String binName( bin->getName() );
   if ( binName.isEmpty() )
      return;

   if ( !isBinStart && binName.equal("AL_LightBinMgr") )
   {
      _renderPropagated(sceneState);

      if ( mRenderReflection )
         _renderReflect(sceneState);
   }
}

void OfflineLPV::_renderPropagated(const SceneRenderState* state)
{
   if ( !mPropagatedTexture || !isClientObject() ) 
      return;

   // -- Setup Render Target --
   if ( !mRenderTarget )
      mRenderTarget = GFX->allocRenderToTextureTarget();
         
   if ( !mRenderTarget ) return;

   if ( !mLightInfoTarget )
      mLightInfoTarget = NamedTexTarget::find( "lightinfo" );

   GFXTextureObject *texObject = mLightInfoTarget->getTexture();
   if ( !texObject ) return;

   mRenderTarget->attachTexture( GFXTextureTarget::Color0, texObject );

   // We also need to sample from the depth buffer.
   if ( !mPrepassTarget )
      mPrepassTarget = NamedTexTarget::find( "deferred" );

   GFXTextureObject *prepassTexObject = mPrepassTarget->getTexture();
   if ( !prepassTexObject ) return;

   // -- Setup screenspace quad to render (postfx) --
   Frustum frustum = state->getCameraFrustum();
   GFXVertexBufferHandle<PFXVertex> vb;
   _updateScreenGeometry( frustum, &vb );

   // -- State Block --
   GFXStateBlockDesc desc;
   desc.setZReadWrite( false, false );
   desc.setBlend( true, GFXBlendOne, GFXBlendOne );
   desc.setFillModeSolid();

   // Point sampling is useful for debugging to see the exact color values in each cell.
   //desc.samplers[0] = GFXSamplerStateDesc::getClampPoint();
   //desc.samplersDefined = true;

   // Camera position, used to calculate World Space position from depth buffer.
   const Point3F &camPos = state->getCameraPosition();
   mPropagatedShaderConsts->setSafe( mEyePosWorldPropSC, camPos );

   // Volume position, used to calculate UV sampling.
   Box3F worldBox = getWorldBox();
   Point3F bottom_corner = worldBox.minExtents;
   Point3F top_corner = worldBox.maxExtents;
   Point3F volume_size = (top_corner - bottom_corner);
   mPropagatedShaderConsts->setSafe(mVolumeStartPropSC, bottom_corner);
   mPropagatedShaderConsts->setSafe(mVolumeSizePropSC, volume_size);

   // Render Target Parameters.
   const Point3I &targetSz = texObject->getSize();
   const RectI &targetVp = mLightInfoTarget->getViewport();
   Point4F rtParams;
   ScreenSpace::RenderTargetParameters(targetSz, targetVp, rtParams);
   mPropagatedShaderConsts->setSafe(mRTParamsPropSC, rtParams);

   GFX->pushActiveRenderTarget();
   GFX->setActiveRenderTarget( mRenderTarget );
   GFX->setVertexBuffer( vb );
   GFX->setStateBlockByDesc( desc );
   GFX->setShader(mPropagatedShader);
   GFX->setShaderConstBuffer(mPropagatedShaderConsts);

   // Setup Textures
   GFX->setTexture(0, mPropagatedTexture);
   GFX->setTexture(1, prepassTexObject);

   // and SSAO mask
   if ( AdvancedLightBinManager::smUseSSAOMask )
   {
      if ( !mSSAOMaskTarget )
         mSSAOMaskTarget = NamedTexTarget::find( "ssaoMask");

      if ( mSSAOMaskTarget )
      {
         GFXTextureObject *SSAOMaskTexObject = mSSAOMaskTarget->getTexture();
         if ( SSAOMaskTexObject ) 
            GFX->setTexture(2, SSAOMaskTexObject);
      }
   }

   // Draw the screenspace quad.
   GFX->drawPrimitive( GFXTriangleStrip, 0, 2 );

   // Clean Up
   mRenderTarget->resolve();
   GFX->popActiveRenderTarget();
}

void OfflineLPV::_renderReflect(const SceneRenderState* state)
{
   if ( !mDirectLightTexture || !isClientObject() ) 
      return;

   // -- Setup Render Target --
   if ( !mRenderTarget )
      mRenderTarget = GFX->allocRenderToTextureTarget();
         
   if ( !mRenderTarget ) return;

   if ( !mLightInfoTarget )
      mLightInfoTarget = NamedTexTarget::find( "lightinfo" );

   GFXTextureObject *texObject = mLightInfoTarget->getTexture();
   if ( !texObject ) return;

   mRenderTarget->attachTexture( GFXTextureTarget::Color0, texObject );

   // We need to sample from the depth buffer.
   if ( !mPrepassTarget )
      mPrepassTarget = NamedTexTarget::find( "prepass" );

   GFXTextureObject *prepassTexObject = mPrepassTarget->getTexture();
   if ( !prepassTexObject ) return;

   // and the material info buffer.
   if ( !mMatInfoTarget )
      mMatInfoTarget = NamedTexTarget::find( "matinfo" );

   GFXTextureObject *matInfoTexObject = mMatInfoTarget->getTexture();
   if ( !matInfoTexObject ) return;

   // -- Setup screenspace quad to render (postfx) --
   Frustum frustum = state->getCameraFrustum();
   GFXVertexBufferHandle<PFXVertex> vb;
   _updateScreenGeometry( frustum, &vb );

   // -- State Block --
   GFXStateBlockDesc desc;
   desc.setZReadWrite( false, false );
   desc.setBlend( true, GFXBlendOne, GFXBlendOne );
   desc.setFillModeSolid();

   // Point sampling is useful for debugging to see the exact color values in each cell.
   //desc.samplers[0] = GFXSamplerStateDesc::getClampPoint();
   //desc.samplersDefined = true;

   // Camera position, used to calculate World Space position from depth buffer.
   const Point3F &camPos = state->getCameraPosition();
   mReflectShaderConsts->setSafe( mEyePosWorldReflectSC, camPos );

   MatrixF inverseViewMatrix = state->getWorldViewMatrix();
   inverseViewMatrix.fullInverse();
   inverseViewMatrix.transpose();
   mReflectShaderConsts->setSafe( mInverseViewReflectSC, inverseViewMatrix );

   // Volume position, used to calculate UV sampling.
   Box3F worldBox = getWorldBox();
   Point3F bottom_corner = worldBox.minExtents;
   Point3F top_corner = worldBox.maxExtents;
   Point3F volume_size = (top_corner - bottom_corner);
   mReflectShaderConsts->setSafe(mVolumeStartReflectSC, bottom_corner);
   mReflectShaderConsts->setSafe(mVolumeSizeReflectSC, volume_size);

   // Render Target Parameters.
   const Point3I &targetSz = texObject->getSize();
   const RectI &targetVp = mLightInfoTarget->getViewport();
   Point4F rtParams;
   ScreenSpace::RenderTargetParameters(targetSz, targetVp, rtParams);
   mReflectShaderConsts->setSafe(mRTParamsReflectSC, rtParams);

   GFX->pushActiveRenderTarget();
   GFX->setActiveRenderTarget( mRenderTarget );
   GFX->setVertexBuffer( vb );
   GFX->setStateBlockByDesc( desc );
   GFX->setShader(mReflectShader);
   GFX->setShaderConstBuffer(mReflectShaderConsts);

   // Setup Textures
   GFX->setTexture(0, mDirectLightTexture);
   GFX->setTexture(1, prepassTexObject);
   GFX->setTexture(2, matInfoTexObject);

   // Draw the screenspace quad.
   GFX->drawPrimitive( GFXTriangleStrip, 0, 2 );

   // Clean Up
   mRenderTarget->resolve();
   GFX->popActiveRenderTarget();
}

void OfflineLPV::_updateScreenGeometry(   const Frustum &frustum,
                                          GFXVertexBufferHandle<PFXVertex> *outVB )
{

   // NOTE: GFXTransformSaver does not save/restore the frustum
   // so we must save it here before we modify it.
   F32 l, r, b, t, n, f;
   bool ortho;
   GFX->getFrustum(&l, &r, &b, &t, &n, &f, &ortho);

   outVB->set( GFX, 4, GFXBufferTypeVolatile );

   const Point3F *frustumPoints = frustum.getPoints();
   const Point3F& cameraPos = frustum.getPosition();

   // Perform a camera offset.  We need to manually perform this offset on the postFx's
   // polygon, which is at the far plane.
   const Point2F& projOffset = frustum.getProjectionOffset();
   Point3F cameraOffsetPos = cameraPos;
   if(!projOffset.isZero())
   {
      // First we need to calculate the offset at the near plane.  The projOffset
      // given above can be thought of a percent as it ranges from 0..1 (or 0..-1).
      F32 nearOffset = frustum.getNearRight() * projOffset.x;

      // Now given the near plane distance from the camera we can solve the right
      // triangle and calcuate the SIN theta for the offset at the near plane.
      // SIN theta = x/y
      F32 sinTheta = nearOffset / frustum.getNearDist();

      // Finally, we can calcuate the offset at the far plane, which is where our sun (or vector)
      // light's polygon is drawn.
      F32 farOffset = frustum.getFarDist() * sinTheta;

      // We can now apply this far plane offset to the far plane itself, which then compensates
      // for the project offset.
      MatrixF camTrans = frustum.getTransform();
      VectorF offset = camTrans.getRightVector();
      offset *= farOffset;
      cameraOffsetPos += offset;
   }

   PFXVertex *vert = outVB->lock();

   vert->point.set( -1.0, -1.0, 0.0 );
   vert->texCoord.set( 0.0f, 1.0f );
   vert->wsEyeRay = frustumPoints[Frustum::FarBottomLeft] - cameraOffsetPos;
   vert++;

   vert->point.set( -1.0, 1.0, 0.0 );
   vert->texCoord.set( 0.0f, 0.0f );
   vert->wsEyeRay = frustumPoints[Frustum::FarTopLeft] - cameraOffsetPos;
   vert++;

   vert->point.set( 1.0, 1.0, 0.0 );
   vert->texCoord.set( 1.0f, 0.0f );
   vert->wsEyeRay = frustumPoints[Frustum::FarTopRight] - cameraOffsetPos;
   vert++;

   vert->point.set( 1.0, -1.0, 0.0 );
   vert->texCoord.set( 1.0f, 1.0f );
   vert->wsEyeRay = frustumPoints[Frustum::FarBottomRight] - cameraOffsetPos;
   vert++;

   outVB->unlock();

   // Restore frustum
   if (!ortho)
      GFX->setFrustum(l, r, b, t, n, f);
   else
      GFX->setOrtho(l, r, b, t, n, f);
}

//--- Final Volume Saving & Loading ---

bool OfflineLPV::_setSaveResults( void *object, const char *index, const char *data )
{
   OfflineLPV* volume = reinterpret_cast< OfflineLPV* >( object );
   volume->mSaveResults = true;
   return false;
}

bool OfflineLPV::save()
{
   if ( mFileName.length() == 0 || !mLightGrid || !mPropagatedLightGrid ) return false;

   FileStream dtsStream;
   if (dtsStream.open(mFileName, Torque::FS::File::Write))
   {
      Point3I voxelCount = getVoxelCount();

      // Resolution
      dtsStream.write(voxelCount.x);
      dtsStream.write(voxelCount.y);
      dtsStream.write(voxelCount.z);

      U32 pos = 0;
      for(U32 z = 0; z < voxelCount.z; z++)
      {
         for(U32 y = 0; y < voxelCount.y; y++)
         {
            for(U32 x = 0; x < voxelCount.x; x++)
            {
               // Direct Light
               LinearColorF decoded_direct = decodeSH(Point3F(0, 0, 0), mLightGrid[pos]);
               dtsStream.write(decoded_direct.red);
               dtsStream.write(decoded_direct.green);
               dtsStream.write(decoded_direct.blue);
               dtsStream.write(decoded_direct.alpha);

               // Propagated Light
               LinearColorF decoded_prop = decodeSH(Point3F(0, 0, 0), mPropagatedLightGrid[pos]);
               dtsStream.write(decoded_prop.red);
               dtsStream.write(decoded_prop.green);
               dtsStream.write(decoded_prop.blue);
               dtsStream.write(decoded_prop.alpha);

               pos++;
            }
         }
      }

      return true;
   }

   return false;
}

bool OfflineLPV::_setLoadResults( void *object, const char *index, const char *data )
{
   OfflineLPV* volume = reinterpret_cast< OfflineLPV* >( object );
   volume->mLoadResults = true;
   return false;
}

bool OfflineLPV::load()
{
   if ( mFileName.length() == 0 ) return false;

   FileStream dtsStream;
   if (dtsStream.open(mFileName, Torque::FS::File::Read))
   {
      // Resolution
      Point3I size;
      dtsStream.read(&size.x);
      dtsStream.read(&size.y);
      dtsStream.read(&size.z);

      LinearColorF* directLightGrid = new LinearColorF[size.x * size.y * size.z];
      LinearColorF* propagatedLightGrid = new LinearColorF[size.x * size.y * size.z];

      U32 pos = 0;
      for(U32 z = 0; z < size.z; z++)
      {
         for(U32 y = 0; y < size.y; y++)
         {
            for(U32 x = 0; x < size.x; x++)
            {
               // Direct Light
               dtsStream.read(&directLightGrid[pos].red);
               dtsStream.read(&directLightGrid[pos].green);
               dtsStream.read(&directLightGrid[pos].blue);
               dtsStream.read(&directLightGrid[pos].alpha);

               // Propagated Light
               dtsStream.read(&propagatedLightGrid[pos].red);
               dtsStream.read(&propagatedLightGrid[pos].green);
               dtsStream.read(&propagatedLightGrid[pos].blue);
               dtsStream.read(&propagatedLightGrid[pos].alpha);

               pos++;
            }
         }
      }

      // Export loaded results.
      _initVolumeTextures(size);
      exportPropagatedLight(propagatedLightGrid, &size);
      exportDirectLight(directLightGrid, &size);

      SAFE_DELETE(propagatedLightGrid);
      SAFE_DELETE(directLightGrid);

      return true;
   }

   return false;
}