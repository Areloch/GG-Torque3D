#pragma once
#ifndef RENDER_PIPELINE
#include "console/engineAPI.h"

#include "console/simObject.h"

#include "gfx/gfxDevice.h"

#ifndef _MATTEXTURETARGET_H_
#include "materials/matTextureTarget.h"
#endif
#ifndef _CONDITIONER_BASE_H_
#include "shaderGen/conditionerFeature.h"
#endif

#include "scene/sceneManager.h"
#include "scene/sceneCameraState.h"

#include "lighting/shadowManager.h"
#include "lighting/shadowMap/shadowMapPass.h"

#include "renderInstance/renderDeferredMgr.h"
#include "lighting/advanced/advancedLightBinManager.h"

class GBufferConditioner : public ConditionerFeature
{
   typedef ConditionerFeature Parent;

public:
   enum ColorFormat
   {
      RGB,
      LUV
   };

public:

   GBufferConditioner(const GFXFormat bufferFormat, const ColorFormat colorFormat)
      : Parent(bufferFormat), mColorFormat(colorFormat)
   {

   }

   virtual ~GBufferConditioner();

   virtual String getName()
   {
      return String("GBuffer Conditioner ") + String(mColorFormat == RGB ? "[RGB]" : "[LUV]");
   }

protected:
   ColorFormat mColorFormat;

   virtual Var *_conditionOutput(Var *unconditionedOutput, MultiLine *meta);
   virtual Var *_unconditionInput(Var *conditionedInput, MultiLine *meta);
   virtual Var *printMethodHeader(MethodType methodType, const String &methodName, Stream &stream, MultiLine *meta);
   virtual void printMethodFooter(MethodType methodType, Var *retVar, Stream &stream, MultiLine *meta);
};

class RenderPipeline : public SimObject
{
   typedef SimObject Parent;

   U32 mLastRenderTime;

public:
   //placeholders for ease of material hook-ins
   static const StringTableEntry ColorBufferName;
   static const StringTableEntry NormalBufferName;
   static const StringTableEntry MaterialInfoBufferName;
   static const StringTableEntry LightInfoBufferName;

protected:
	StringTableEntry mPipelineName;
   StringTableEntry mDescription;

   static RenderPipeline* smRenderPipeline;

public:
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

   enum TargetSizeType
   {
      WindowSize = 0,
      WindowSizeScaled,
      FixedSize
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

		struct Buffer
		{
			StringTableEntry bufferName;
			RenderTargets renderTarget;
         NamedTexTarget namedTarget;
         GFXFormat targetFormat;
         GFXTextureTargetRef targetRef;
         GBufferConditioner* conditioner;
         GFXTexHandle handle;

         //RectI mTargetViewport;
         TargetSizeType mTargetSizeType;

         U32 mTargetChainLength;
         U32 mTargetChainIdx;
         U32 mNumRenderTargets;
         GFXTextureTargetRef *mTargetChain;
         GFXTexHandle **mTargetChainTextures;
		};

		Vector<Buffer> buffers;

		U32 getTargetSize(U32 targetIndex);
		U32 getGBufferSize();

		//Used to validate our targets so we don't have multiple channels overlapped
		bool setBuffer(StringTableEntry _bufferName, RenderTargets _renderTarget, GFXFormat _targetFormat);

      Buffer* findBufferByName(StringTableEntry bufferName);

      GBuffer()
      {
         buffers.clear();
      }
	};

   Vector<RenderBinManager*> mRenderBins;

   Vector<RenderPassManager*> mRenderPasses;

   RenderPassManager* mCurrentRenderPass;

   RenderPassManager* getRenderPass() { return mCurrentRenderPass; }

protected:

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

   RenderPassManager* mRenderPassManager;

   //
   //
   //Lighting
   ShadowMapPass *mShadowMapPass;
   LightShadowMap *mCurrentShadowMap;
   LightShadowMap *mCurrentDynamicShadowMap;

   GBuffer* getGBuffer() { return &mGBuffer; }

   static const U32 OpaqueStaticLitMask = BIT(1);     ///< Stencil mask for opaque, lightmapped pixels
   static const U32 OpaqueDynamicLitMask = BIT(0);    ///< Stencil mask for opaque, dynamic lit pixels

   AdvancedLightBinManager* mLightBinManager;
   RenderDeferredMgr *mDeferredRenderManager;

public:
	RenderPipeline();
   DECLARE_CONOBJECT(RenderPipeline);

   static void initPersistFields();

   bool onAdd();
   void onRemove();


   void renderFrame(GFXTextureTargetRef* target, MatrixF transform, Frustum frustum, U32 typeMask, ColorI canvasClearColor, bool isReflection = false);

   //void renderScene(SceneRenderState* renderState, U32 objectMask);
   //void _renderScene(SceneRenderState* renderState, U32 objectMask);

   //void renderNonSystems(SceneRenderState* renderState, U32 objectMask);

	virtual void setupBuffers();
   virtual void setupRenderBins();
   virtual void setupPasses();

   void addRenderBin(RenderBinManager* renderBinMgr);
   void addRenderPass(RenderPassManager* renderPassMgr);

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

   void registerMaterialFeatures();

   static const GFXStateBlockDesc & getOpaqueStenciWriteDesc(bool lightmappedGeometry = true);

   bool setGBufferTargetSizes(Point2I targetSize);
   bool setGBufferTarget(StringTableEntry bufferName, StringTableEntry targetName, StringTableEntry formatName);

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

   GFXFormat parseFormatString(StringTableEntry formatStr);

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

#endif