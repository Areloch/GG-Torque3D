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

#ifndef _OFFLINELPV_H_
#define _OFFLINELPV_H_

#ifndef _SCENEPOLYHEDRALSPACE_H_
#include "scene/scenePolyhedralSpace.h"
#endif

#ifndef _MSILHOUETTEEXTRACTOR_H_
#include "math/mSilhouetteExtractor.h"
#endif

#ifndef _GFXDEVICE_H_
#include "gfx/gfxDevice.h"
#endif

#ifndef _POSTEFFECTCOMMON_H_
#include "postFx/postEffectCommon.h"
#endif

#include "materials/matTextureTarget.h"
#include "renderInstance/renderBinManager.h"
#include "core/resourceManager.h"

GFX_DeclareTextureProfile( LPVProfile );

/// A volume in space that blocks visibility.
class OfflineLPV : public ScenePolyhedralSpace
{
   public:

      typedef ScenePolyhedralSpace Parent;

      struct DebugRenderStash
      {
         struct WireMeshData
         {
            struct data
            {
               data()
               {
                  triCount = 0;
               }

               U32 triCount;

               Vector<Point3F> vertA;
               Vector<Point3F> vertB;
               Vector<Point3F> vertC;
            };

            void clear()
            {
               bufferData.clear();
            }

            Vector<data> bufferData;
         };

         WireMeshData wireMeshRender;
      };

      DebugRenderStash mDebugRender;

   protected:

      // Volume Textures and raw data buffer used to copy.
      GFXTexHandle            mPropagatedTexture;
      GFXTexHandle            mDirectLightTexture;

      // We sample from prepass and render to light buffer.
      NamedTexTarget*         mPrepassTarget;
      NamedTexTarget*         mLightInfoTarget;
      NamedTexTarget*         mMatInfoTarget;
      NamedTexTarget*         mSSAOMaskTarget;
      GFXTextureTargetRef     mRenderTarget;

      // Stateblock for shaders
      GFXStateBlockRef        mStateBlock;

      // Propagated Shader
      GFXShaderRef            mPropagatedShader;
      GFXShaderConstBufferRef mPropagatedShaderConsts;
      GFXShaderConstHandle    *mEyePosWorldPropSC;
      GFXShaderConstHandle    *mRTParamsPropSC;
      GFXShaderConstHandle    *mVolumeStartPropSC;
      GFXShaderConstHandle    *mVolumeSizePropSC;

      // Reflection Shader
      GFXShaderRef            mReflectShader;
      GFXShaderConstBufferRef mReflectShaderConsts;
      GFXShaderConstHandle    *mInverseViewReflectSC;
      GFXShaderConstHandle    *mEyePosWorldReflectSC;
      GFXShaderConstHandle    *mRTParamsReflectSC;
      GFXShaderConstHandle    *mVolumeStartReflectSC;
      GFXShaderConstHandle    *mVolumeSizeReflectSC;

      // Configuratable volume resolution
      F32       mVoxelSize;

      // Volume Texture
      Point3I   mCachedVolumeSize;
      void     _initVolumeTextures(Point3I volumeSize);
      void     _initShaders();

      // Geometry voxel with emissive flag
      struct GeometryVoxel
      {
         LinearColorF color;
         bool emissive;

         GeometryVoxel()
         {
            color = LinearColorF::ZERO;
            emissive = false;
         }
      };

      // Geometry Grid
      GeometryVoxel* mGeometryGrid;

      // 2nd Order Spherical Harmonics require storage in (3) float4s.
      struct SHVoxel
      {
         LinearColorF red;
         LinearColorF green;
         LinearColorF blue;

         // For debugging only.
         Point3F normal;

         SHVoxel()
         {
            red = LinearColorF::ZERO;
            green = LinearColorF::ZERO;
            blue = LinearColorF::ZERO;
            normal = Point3F::Zero;
         }
      };

      // Spherical Harmonics functions
      Vector4F getClampedCosineSHCoeffs(Point3F dir);
      SHVoxel  encodeSH(Point3F dir, LinearColorF color);
      LinearColorF   decodeSH(Point3F dir, SHVoxel sh);
      SHVoxel  calcSHLights(Point3F position, LinearColorF geometryColor, Point3I voxelPosition, bool emissive = false);

      // Helper functions
      Point3F  getNearestFaceNormals(Point3F dir, bool nonOccludedOnly = false, U32 x = 0, U32 y = 0, U32 z = 0);
      Point3F  reflect(Point3F v, Point3F n);

      // Directly lit voxel grid. Calculated from real light sources in scene.
      SHVoxel*   mLightGrid;

      // Propagation Grids.
      U32        mPropagationStage;
      SHVoxel*   mPropagatedLightGrid;
      SHVoxel*   mPropagatedLightGridA;
      SHVoxel*   mPropagatedLightGridB;

      // Final Volume Rendering
      void _handleBinEvent( RenderBinManager *bin, const SceneRenderState* sceneState, bool isBinStart );  
      void _renderPropagated(const SceneRenderState* sceneState);
      void _renderReflect(const SceneRenderState* sceneState);
      void _updateScreenGeometry( const Frustum &frustum, GFXVertexBufferHandle<PFXVertex> *outVB );

      // World Editor Visualization.
      typedef SilhouetteExtractorPerspective< PolyhedronType > SilhouetteExtractorType;
      bool mTransformDirty;
      Vector< Point3F > mWSPoints;
      SilhouetteExtractorType mSilhouetteExtractor;
      mutable Vector< SceneObject* > mVolumeQueryList;

      // Debug voxel rendering.
      struct DebugVoxel
      {
         LinearColorF color;
         Point3F position;
         Point3F size;
      };
      Vector<DebugVoxel> mDebugVoxels;
      GFXVertexBufferHandle<GFXVertexPC> mDebugVoxelVB;

      void _rebuildDebugVoxels();
      void _rebuildDebugVoxelsVB();
      virtual void _renderObject( ObjectRenderInst* ri, SceneRenderState* state, BaseMatInstance* overrideMat );

      //texture cacheing
      struct TextureCache
      {
         Resource<GBitmap> mTexture;
         FileName mFileReference;
      };
      Vector<TextureCache> mTextureCache;
      Resource<GBitmap> getOrCreateTexture(FileName textureName);

   public:

      OfflineLPV();
      ~OfflineLPV();

      // SimObject.
      DECLARE_CONOBJECT( OfflineLPV );
      DECLARE_DESCRIPTION( "Offline Light Propogation Volume" );
      DECLARE_CATEGORY( "3D Scene" );

      virtual bool onAdd();
      virtual void onRemove();
      void inspectPostApply();

      Point3I  getVoxelCount();
      Point3F  getWorldSpaceVoxelSize();

      // Saving & Loading
      String   mFileName;
      bool     save();
      bool     load();

      // Settings
      F32  mLightIntensity;
      F32  mEmissiveIntensity;
      F32  mPropagationMultiplier;
      F32  mGIMultiplier;

      // Editor Triggered Flags
      bool mRegenVolume;
      bool mInjectLights;
      bool mPropagateLights;
      bool mShowVoxels;
      bool mShowDirectLight;
      bool mShowPropagated;
      bool mExportPropagated;
      bool mExportDirectLight;
      bool mRenderReflection;
      bool mSaveResults;
      bool mLoadResults;

      // Editor Triggered Functions
      void regenVolume();
      void injectLights();
      void exportPropagatedLight(LinearColorF* pSource, Point3I* pSize = NULL);
      void exportPropagatedLight(SHVoxel* pSource, Point3I* pSize = NULL);
      void exportDirectLight(LinearColorF* pSource, Point3I* pSize = NULL);
      void exportDirectLight(SHVoxel* pSource, Point3I* pSize = NULL);
      F32  getAttenuation(LightInfo* lightInfo, Point3F position);
      void propagateLights(SHVoxel* source, SHVoxel* dest, bool sampleFromGeometry = false);

      Point3I getVoxel(Point3F position);
      S32 getVoxelIndex(U32 x, U32 y, U32 z)
      {
         Point3I voxels = getVoxelCount();
         S32 offset = -1;

         if (x < voxels.x && y < voxels.y && z < voxels.z)
         {
            /*offset = (x*voxels.y*voxels.z + y*voxels.z + z);

            U32 otheroffset = x * (voxels.y * voxels.z) + y * (voxels.z) + z;

            bool hurp = true;*/

            offset = (voxels.x * voxels.y * z) + (voxels.x * y) + x;
         }

         return offset;
      }

      // Network
      U32  packUpdate( NetConnection *, U32 mask, BitStream *stream );
      void unpackUpdate( NetConnection *, BitStream *stream );

      // SceneObject.
      virtual void buildSilhouette( const SceneCameraState& cameraState, Vector< Point3F >& outPoints );
      virtual void setTransform( const MatrixF& mat );

      // Static Functions.
      static void consoleInit();
      static void initPersistFields();

      // Editor Triggered Static Functions
      static bool _setRegenVolume( void *object, const char *index, const char *data );
      static bool _setInjectLights( void *object, const char *index, const char *data );
      static bool _setPropagateLights( void *object, const char *index, const char *data );
      static bool _setExportPropagated( void *object, const char *index, const char *data );
      static bool _setExportDirectLight( void *object, const char *index, const char *data );
      static bool _setSaveResults( void *object, const char *index, const char *data );
      static bool _setLoadResults( void *object, const char *index, const char *data );
};

#endif // !_OFFLINELPV_H_