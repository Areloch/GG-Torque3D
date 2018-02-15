#pragma once
#include "console/engineAPI.h"

#include "console/simObject.h"

#include "gfx/gfxDevice.h"

#ifndef _TSSHAPEINSTANCE_H_
#include "ts/tsShapeInstance.h"
#endif

#include "scene/sceneManager.h"
#include "scene/sceneCameraState.h"

#include "lighting/shadowManager.h"
#include "lighting/shadowMap/shadowMapPass.h"

class RenderPipeline : public SimObject
{
   typedef SimObject Parent;

   U32 mLastRenderTime;

   /// Name of the model loaded for display.
   String mModelName;

   /// Model being displayed in the view.
   TSShapeInstance* mModel;

protected:
	StringTableEntry mPipelineName;
   StringTableEntry mDescription;

   static RenderPipeline* smRenderPipeline;

   enum NormalStorage
   {
      CartesianXYZ,
      CartesianXY,
      Spherical,
      LambertAzimuthal,
   };

   enum NormalSpace
   {
      WorldSpace,
      ViewSpace,
   };

	struct GBuffer
	{
		enum RenderTargets
		{
			TARGET0,
			TARGET1,
			TARGET2,
			TARGET3,
			TARGET4,
			TARGET5
		};

		enum TargetChannels
		{
			R2 = 0,
         R8 = 0 << 1,
         R16 = 0 << 2,
         R32 = 0 << 3,
         G2 = 0 << 4,
         G8 = 0 << 5,
         G16 = 0 << 6,
         G32 = 0 << 7,
         B2 = 0 << 8,
         B8 = 0 << 9,
         B16 = 0 << 10,
         B32 = 0 << 11,
         A2 = 0 << 12,
         A8 = 0 << 13,
         A16 = 0 << 14,
         A32 = 0 << 15,
		};

		struct RenderTargetData
		{
			RenderTargets id;
			U32 channelsInUse;
		};

		struct Target
		{
			U32 channels;
			StringTableEntry targetName;
			RenderTargets renderTarget;
		};

		Vector<RenderTargetData> rtData;
		Vector<Target> targets;

		U32 getTargetSize(U32 targetIndex);
		U32 getGBufferSize();

		//Used to validate our targets so we don't have multiple channels overlapped
		bool setBuffer(StringTableEntry _bufferName, RenderTargets _renderTarget, GFXFormat _targetFormat);
	};

	bool mSupportGBuffer;
	GBuffer mGBuffer;

	//Lighting settings
	bool mSupportSSR;
	bool mSupportSSAO;
	bool mSupportHDR;

public:
   //
   Vector< SceneObject* > mBatchQueryList;

   //A cache list of objects that made it through culling, so we don't have to attempt to re-test
   //visibility of objects later.
   Vector< SceneObject* > mRenderedObjectsList;

   LinearColorF mAmbientLightColor;

   SceneRenderState* mCurrentRenderState;

   //
   //
   //Lighting
   ShadowMapPass *mShadowMapPass;
   LightShadowMap *mCurrentShadowMap;
   LightShadowMap *mCurrentDynamicShadowMap;

public:
	RenderPipeline();
   DECLARE_CONOBJECT(RenderPipeline);

   static void initPersistFields();

   bool onAdd();
   void onRemove();

	virtual void render();
   void renderFrame(GFXTextureTargetRef* target, MatrixF transform, Frustum frustum, U32 typeMask, ColorI canvasClearColor);

   //void renderScene(SceneRenderState* renderState, U32 objectMask);
   //void _renderScene(SceneRenderState* renderState, U32 objectMask);

   //void renderNonSystems(SceneRenderState* renderState, U32 objectMask);

	virtual void setupBuffers();

   static RenderPipeline* get()
   {
      /*if (smRenderPipeline == nullptr)
      {
         smRenderPipeline = new RenderPipeline();
      }*/

      return smRenderPipeline;
   }

   static bool supportsGBuffer()
   {
      if (smRenderPipeline == nullptr)
      {
         return false;
      }

      return smRenderPipeline->mSupportGBuffer;
   }

   void initialize();
   void shutDown();

   bool setGBufferTarget(String bufferName, String targetName, String formatName);

   RenderPipeline* initPipeline()
   {
      if (smRenderPipeline == nullptr)
      {
         smRenderPipeline = this;
      }
      else
      {
         Con::errorf("Attempted to init the Render Pipeline, but a pipeline was already activated!");
      }

      return smRenderPipeline;
   }

   void shutDownPipeline()
   {
      if (smRenderPipeline != nullptr)
      {
         delete smRenderPipeline;
      }
   }

   //
   //
   //
   //
   /// Render the scene with the default render pass.
   /// @note This uses the current GFX state (transforms, viewport, frustum) to initialize
   ///   the render state.
   static SceneCameraState smLockedDiffuseCamera;

   void renderScene(ScenePassType passType, U32 objectMask = DEFAULT_RENDER_TYPEMASK);

   /// Render the scene with a custom rendering pass.
   void renderScene(SceneRenderState *state, U32 objectMask = DEFAULT_RENDER_TYPEMASK, SceneZoneSpace* baseObject = NULL, U32 baseZone = 0);

   /// Render the scene with a custom rendering pass and no lighting set up.
   void renderSceneNoLights(SceneRenderState *state, U32 objectMask = DEFAULT_RENDER_TYPEMASK, SceneZoneSpace* baseObject = NULL, U32 baseZone = 0);

   void _renderScene(SceneRenderState* state,
      U32 objectMask = (U32)-1,
      SceneZoneSpace* baseObject = NULL,
      U32 baseZone = 0);
};