#include "renderPipeline.h"
#include "materials/matTextureTarget.h"
#include "materials/materialManager.h"

#include "renderInstance/renderPassManager.h"

#include "gfx/gfxTransformSaver.h"
#include "scene/sceneManager.h"
#include "scene/sceneRenderState.h"

#include "lighting/lightManager.h"
#include "lighting/lightInfo.h"

#include "T3D/systems/render/meshRenderSystem.h"

#include "core/resourceManager.h"

#include "postFx/postEffectManager.h"

#include "gui/core/guiCanvas.h"
#include "gui/core/guiOffscreenCanvas.h"

#include "scene/zones/sceneTraversalState.h"
#include "scene/zones/sceneRootZone.h"
#include "scene/zones/sceneZoneSpace.h"

#include "T3D/gameFunctions.h"

#include "gui/3d//guiTSControl.h"

#include "math/mathUtils.h"

#include "scene/reflectionManager.h"

#include "renderInstance/renderFormatChanger.h"

//#include "lighting/advanced/advancedLightBinManager.h"
#include "lighting/advanced/advancedLightingFeatures.h"
#include "lighting/shadowMap/shadowMapManager.h"

#include "shaderGen/featureMgr.h"
#include "materials/materialFeatureTypes.h"

#include "lighting/advanced/advancedLightManager.h" 

#if defined( TORQUE_OS_WIN )
#  include "lighting/advanced/hlsl/gBufferConditionerHLSL.h"
#  include "lighting/advanced/hlsl/advancedLightingFeaturesHLSL.h"
#endif
#if defined( TORQUE_OPENGL )
#  include "lighting/advanced/glsl/gBufferConditionerGLSL.h"
#  include "lighting/advanced/glsl/advancedLightingFeaturesGLSL.h"
#endif

#include "lighting/advanced/advancedLightBufferConditioner.h"

#define RENDER_PIPELINE

RenderPipeline* RenderPipeline::smRenderPipeline(0);

const StringTableEntry RenderPipeline::ColorBufferName(StringTable->insert("color"));
const StringTableEntry RenderPipeline::NormalBufferName(StringTable->insert("deferred"));
const StringTableEntry RenderPipeline::MaterialInfoBufferName(StringTable->insert("matInfo"));
const StringTableEntry RenderPipeline::LightInfoBufferName(StringTable->insert("lightInfo"));

IMPLEMENT_CONOBJECT(RenderPipeline);

SceneCameraState RenderPipeline::smLockedDiffuseCamera = SceneCameraState(RectI(), Frustum(), MatrixF(), MatrixF());

RenderPipeline::RenderPipeline()
{
   mPipelineName = StringTable->EmptyString();
   mCurrentRenderPass = nullptr;
}

bool RenderPipeline::onAdd()
{
   if (!Parent::onAdd())
      return false;

   if(mPipelineName == StringTable->EmptyString())
      mPipelineName = getName();

   return true;
}

void RenderPipeline::onRemove()
{
   Parent::onRemove();
}

void RenderPipeline::initPersistFields()
{
   addField("pipelineName", TypeString, Offset(mPipelineName, RenderPipeline), "");

   addField("supportGBuffer", TypeBool, Offset(mSupportGBuffer, RenderPipeline), "");
   addField("supportHDR", TypeBool, Offset(mSupportHDR, RenderPipeline), "");
   addField("supportSSAO", TypeBool, Offset(mSupportSSAO, RenderPipeline), "");
   addField("supportSSR", TypeBool, Offset(mSupportSSR, RenderPipeline), "");

   addField("renderPassManager", TypeSimObjectPtr, Offset(mRenderPassManager, RenderPipeline), "");
}

void RenderPipeline::initialize()
{
   //Init the pipeline as the active statically available pipeline
   //If it fails, bail out now
   if (!initPipeline())
      return;

   mCurrentRenderPass = nullptr;

   //Begin the real setup of the pipeline's behavior. This informs the rest of the rendering's configuration
   if (mSupportGBuffer)
   {
      //Inform the shader system that we're deferred
      GFXShader::addGlobalMacro("TORQUE_DEFERRED_LIGHTING");

      // Tell the material manager that deferred is enabled.
      MATMGR->setDeferredEnabled(true);

      // Find a target format that supports blending... 
      // we prefer the floating point format if it works.
      /*Vector<GFXFormat> formats;
      formats.push_back(GFXFormatR16G16B16A16F);
      //formats.push_back( GFXFormatR16G16B16A16 );
      GFXFormat blendTargetFormat = GFX->selectSupportedFormat(&GFXRenderTargetProfile,
         formats,
         true,
         true,
         false);*/

      //If we're doing a gbuffer, we need to operate out of the deferred render manager, naturally
      /*mDeferredRenderManager = new RenderDeferredMgr(true, blendTargetFormat);
      mDeferredRenderManager->assignName("DeferredBin");
      mDeferredRenderManager->registerObject();*/
      //mRenderPassManager->addManager(mDeferredRenderManager);

      /*mLightBinManager = new AdvancedLightBinManager(AdvancedLightManager::getActiveLM(), SHADOWMGR, blendTargetFormat);
      mLightBinManager->assignName("LightBin");
      mLightBinManager->setRenderOrder(mDeferredRenderManager->getRenderOrder() + 0.01f); //make it render JUST after the deferred bin*/
      //mRenderPassManager->addManager(mLightBinManager);
   }

   GFXShader::addGlobalMacro("TORQUE_LINEAR_DEPTH");

   //set up the clearGBuffer shader
   //mClearGBufferShader = NULL;

   setupBuffers();
   setupRenderBins();
   setupPasses();

   registerMaterialFeatures();

   //special-handle our lighting manager and bin for now
   gClientSceneGraph->setLightManager("Advanced Lighting");
   
   StringTableEntry lightBinName = StringTable->insert("LightBin");
   for (U32 i = 0; i < mRenderBins.size(); i++)
   {
      if (mRenderBins[i]->getName() == lightBinName)
      {
         AdvancedLightBinManager* lightBin = dynamic_cast<AdvancedLightBinManager*>(mRenderBins[i]);
         
         AdvancedLightManager* lm = dynamic_cast<AdvancedLightManager*>(LIGHTMGR);
         lightBin->mLightManager = lm;
         lightBin->mShadowManager = SHADOWMGR;

         lm->mLightBinManager = lightBin;
         break;
      }
   }

   mShadowMapPass = new ShadowMapPass(LIGHTMGR);
}

void RenderPipeline::shutDown()
{

}

void RenderPipeline::setupBuffers()
{
   /*if (mSupportGBuffer)
   {
      mNamedTarget.registerWithName(BufferName);
      mColorTarget.registerWithName(ColorBufferName);
      mMatInfoTarget.registerWithName(MatInfoBufferName);
   }*/

   if (isMethod("setupBuffers"))
      Con::executef(this, "setupBuffers");
}

void RenderPipeline::setupRenderBins()
{
   mRenderBins.clear();

   if (isMethod("setupRenderBins"))
      Con::executef(this, "setupRenderBins");
}

void RenderPipeline::setupPasses()
{
   mRenderPasses.clear();

   if (isMethod("setupPasses"))
      Con::executef(this, "setupPasses");
}

void RenderPipeline::addRenderBin(RenderBinManager* renderBinMgr)
{
   mRenderBins.push_back(renderBinMgr);
}

void RenderPipeline::addRenderPass(RenderPassManager* renderPassMgr)
{
   mRenderPasses.push_back(renderPassMgr);
}

void RenderPipeline::registerMaterialFeatures()
{
   if (mSupportGBuffer)
   {
      GBuffer::Buffer* normalBuffer = mGBuffer.findBufferByName(RenderPipeline::NormalBufferName);
      GBuffer::Buffer* lightingBuffer = mGBuffer.findBufferByName(RenderPipeline::LightInfoBufferName);

      AdvancedLightingFeatures::registerFeatures(normalBuffer->targetFormat, lightingBuffer->targetFormat);
   }

   MATMGR->setDeferredEnabled(true);

   //Set up material features
   if (GFX->getAdapterType() == OpenGL)
   {
#if defined( TORQUE_OPENGL ) 
      if (mSupportGBuffer)
      {
         GBuffer::Buffer* buffer = mGBuffer.findBufferByName(RenderPipeline::NormalBufferName);

         //ConditionerFeature *cond = new GBufferConditionerGLSL(GFXFormatR16F, GBufferConditionerGLSL::ViewSpace);
         //FEATUREMGR->registerFeature(MFT_DeferredConditioner, cond);
         FEATUREMGR->registerFeature(MFT_RTLighting, new DeferredRTLightingFeatGLSL());
      }

      FEATUREMGR->registerFeature(MFT_NormalMap, new DeferredBumpFeatGLSL());
      FEATUREMGR->registerFeature(MFT_PixSpecular, new DeferredPixelSpecularGLSL());
      FEATUREMGR->registerFeature(MFT_MinnaertShading, new DeferredMinnaertGLSL());
      FEATUREMGR->registerFeature(MFT_SubSurface, new DeferredSubSurfaceGLSL());
#endif
   }
   else
   {
#if defined( TORQUE_OS_WIN )
      if (mSupportGBuffer)
      {
         GBuffer::Buffer* buffer = mGBuffer.findBufferByName(RenderPipeline::NormalBufferName);

         //ConditionerFeature *cond = new GBufferConditionerHLSL(GFXFormatR16F, GBufferConditionerHLSL::ViewSpace);
         //FEATUREMGR->registerFeature(MFT_DeferredConditioner, cond);
         FEATUREMGR->registerFeature(MFT_RTLighting, new DeferredRTLightingFeatHLSL());
      }

      FEATUREMGR->registerFeature(MFT_NormalMap, new DeferredBumpFeatHLSL());
      FEATUREMGR->registerFeature(MFT_PixSpecular, new DeferredPixelSpecularHLSL());
      FEATUREMGR->registerFeature(MFT_MinnaertShading, new DeferredMinnaertHLSL());
      FEATUREMGR->registerFeature(MFT_SubSurface, new DeferredSubSurfaceHLSL());
#endif
   }
}

const GFXStateBlockDesc & RenderPipeline::getOpaqueStenciWriteDesc(bool lightmappedGeometry /*= true*/)
{
   static bool sbInit = false;
   static GFXStateBlockDesc sOpaqueStaticLitStencilWriteDesc;
   static GFXStateBlockDesc sOpaqueDynamicLitStencilWriteDesc;

   if (!sbInit)
   {
      sbInit = true;

      // Build the static opaque stencil write/test state block descriptions
      sOpaqueStaticLitStencilWriteDesc.stencilDefined = true;
      sOpaqueStaticLitStencilWriteDesc.stencilEnable = true;
      sOpaqueStaticLitStencilWriteDesc.stencilWriteMask = 0x03;
      sOpaqueStaticLitStencilWriteDesc.stencilMask = 0x03;
      sOpaqueStaticLitStencilWriteDesc.stencilRef = OpaqueStaticLitMask;
      sOpaqueStaticLitStencilWriteDesc.stencilPassOp = GFXStencilOpReplace;
      sOpaqueStaticLitStencilWriteDesc.stencilFailOp = GFXStencilOpKeep;
      sOpaqueStaticLitStencilWriteDesc.stencilZFailOp = GFXStencilOpKeep;
      sOpaqueStaticLitStencilWriteDesc.stencilFunc = GFXCmpAlways;

      // Same only dynamic
      sOpaqueDynamicLitStencilWriteDesc = sOpaqueStaticLitStencilWriteDesc;
      sOpaqueDynamicLitStencilWriteDesc.stencilRef = OpaqueDynamicLitMask;
   }

   return (lightmappedGeometry ? sOpaqueStaticLitStencilWriteDesc : sOpaqueDynamicLitStencilWriteDesc);
}
//
/*void RenderPipeline::renderViews()
{
	//Walk through all our active cameras, and init a render from them
	for (;;)
	{
		render();
	}
}*/

//

//================================================================================================
//Render a full frame from a given transform and frustum, and render out to a target
//================================================================================================
void RenderPipeline::renderFrame(GFXTextureTargetRef* target, MatrixF transform, Frustum frustum, U32 typeMask, ColorI canvasClearColor, bool isReflection)
{
   if (!GFX->allowRender() || GFX->canCurrentlyRender())
      return;

   PROFILE_START(RenderPipeline_RenderFrame);

   GFX->setActiveRenderTarget(*target);
   if (!GFX->getActiveRenderTarget())
      return;

   GFXTarget* renderTarget = GFX->getActiveRenderTarget();
   if (renderTarget == NULL)
      return;

   // Make sure the root control is the size of the canvas.
   Point2I size = renderTarget->getSize();
   if (size.x == 0 || size.y == 0)
      return;

   //Now, getting to the meat of it!
#ifdef TORQUE_GFX_STATE_DEBUG
   GFX->getDebugStateManager()->startFrame();
#endif
   RectI targetRect(0, 0, size.x, size.y);

   // Signal the interested parties.
   GuiCanvas::getGuiCanvasFrameSignal().trigger(true);

   GFXTransformSaver saver;

   // Gross hack to make sure we don't end up with advanced lighting and msaa 
   // at the same time, which causes artifacts. At the same time we don't 
   // want to just throw the settings the user has chosen if the light manager 
   // changes at a later time.
   /*GFXVideoMode mode = mPlatformWindow->getVideoMode();
   if (dStricmp(LIGHTMGR->getId(), "ADVLM") == 0 && mode.antialiasLevel > 0)
   {
   const char *pref = Con::getVariable("$pref::Video::mode");
   mode.parseFromString(pref);
   mode.antialiasLevel = 0;
   mPlatformWindow->setVideoMode(mode);

   Con::printf("AntiAliasing has been disabled; it is not compatible with AdvancedLighting.");
   }
   else if (dStricmp(LIGHTMGR->getId(), "BLM") == 0)
   {
   const char *pref = Con::getVariable("$pref::Video::mode");

   U32 prefAA = dAtoi(StringUnit::getUnit(pref, 5, " "));
   if (prefAA != mode.antialiasLevel)
   {
   mode.parseFromString(pref);
   mPlatformWindow->setVideoMode(mode);

   Con::printf("AntiAliasing has been enabled while running BasicLighting.");
   }
   }*/

   //Update our GBuffer targets in the event something's changed
   setGBufferTargetSizes(size);

   // Begin GFX
   PROFILE_START(RenderPipeline_RenderFrame_GFXBeginScene);
   bool beginSceneRes = GFX->beginScene();
   PROFILE_END();

   PROFILE_START(RenderPipeline_RenderFrame_OffscreenCanvases);

   // Render all offscreen canvas objects here since we may need them in the render loop
   if (GuiOffscreenCanvas::sList.size() != 0)
   {
      // Reset the entire state since oculus shit will have barfed it.
      GFX->updateStates(true);

      for (Vector<GuiOffscreenCanvas*>::iterator itr = GuiOffscreenCanvas::sList.begin(); itr != GuiOffscreenCanvas::sList.end(); itr++)
      {
         (*itr)->renderFrame(false, false);
      }

      GFX->setActiveRenderTarget(renderTarget);
   }

   PROFILE_END();

   // Can't render if waiting for device to reset.   
   if (!beginSceneRes)
   {
      // Since we already triggered the signal once for begin-of-frame,
      // we should be consistent and trigger it again for end-of-frame.
      GuiCanvas::getGuiCanvasFrameSignal().trigger(false);

      return;
   }

   // Clear the current viewport area
   GFX->setViewport(targetRect);
   GFX->clear(GFXClearZBuffer | GFXClearStencil | GFXClearTarget, canvasClearColor, 1.0f, 0);

   // Make sure we have a clean matrix state 
   // before we start rendering anything!   
   GFX->setWorldMatrix(MatrixF::Identity);
   GFX->setViewMatrix(MatrixF::Identity);
   GFX->setProjectionMatrix(MatrixF::Identity);

   {
      GFXStateBlockDesc d;

      d.cullDefined = true;
      d.cullMode = GFXCullNone;
      d.zDefined = true;
      d.zEnable = false;

      GFXStateBlockRef mDefaultGuiSB = GFX->createStateBlock(d);

      GFX->setClipRect(targetRect);
      GFX->setStateBlock(mDefaultGuiSB);

      GFXTargetRef origTarget = GFX->getActiveRenderTarget();
      U32 origStyle = GFX->getCurrentRenderStyle();

      // Clear the zBuffer so GUI doesn't hose object rendering accidentally
      GFX->clear(GFXClearZBuffer, ColorI(20, 20, 20), 1.0f, 0);

      GFX->setFrustum(frustum);
      MatrixF mSaveProjection = GFX->getProjectionMatrix();

      // We're going to be displaying this render at size of this control in
      // pixels - let the scene know so that it can calculate e.g. reflections
      // correctly for that final display result.
      gClientSceneGraph->setDisplayTargetResolution(size);

      // Set the GFX world matrix to the world-to-camera transform, but don't 
      // change the cameraMatrix in mLastCameraQuery. This is because 
      // mLastCameraQuery.cameraMatrix is supposed to contain the camera-to-world
      // transform. In-place invert would save a copy but mess up any GUIs that
      // depend on that value.
      CameraQuery camera;
      GameUpdateCameraFov();
      bool camQuery = GameProcessCameraQuery(&camera);

      MatrixF worldToCamera = transform;

      GFX->setWorldMatrix(worldToCamera);

      mSaveProjection = GFX->getProjectionMatrix();
      MatrixF mSaveModelview = GFX->getWorldMatrix();

      Point2F mSaveWorldToScreenScale = GFX->getWorldToScreenScale();
      Frustum mSaveFrustum = GFX->getFrustum();
      mSaveFrustum.setTransform(transform);

      //Do an update on our reflections
      REFLECTMGR->update(1, size, camera);

      // Set the default non-clip projection as some 
      // objects depend on this even in non-reflect cases.
      gClientSceneGraph->setNonClipProjection(mSaveProjection);

      // Give the post effect manager the worldToCamera, and cameraToScreen matrices
      PFXMGR->setFrameMatrices(mSaveModelview, mSaveProjection);

      // GameRenderWorld()
      PROFILE_START(RenderPipeline_RenderFrame_RenderWorld);
      FrameAllocator::setWaterMark(0);

      //gClientSceneGraph->renderScene(SPT_Reflect, typeMask);
      //gClientSceneGraph->renderScene(SPT_Diffuse, typeMask);

      if(isReflection)
         renderScene(SPT_Reflect, typeMask);
      else
         renderScene(SPT_Diffuse, typeMask);

      // renderScene leaves some states dirty, which causes problems if GameTSCtrl is the last Gui object rendered
      GFX->updateStates();

      AssertFatal(FrameAllocator::getWaterMark() == 0,
         "Error, someone didn't reset the water mark on the frame allocator!");
      FrameAllocator::setWaterMark(0);
      PROFILE_END();
      // /GameRenderWorld()
   }

   PROFILE_START(RenderPipeline_RenderFrame_GFXEndScene);
   GFX->endScene();
   PROFILE_END();

#ifdef TORQUE_GFX_STATE_DEBUG
   GFX->getDebugStateManager()->endFrame();
#endif

   saver.restore();

   //Update the GBuffer targets
   //for (U32 i = 0; i < mGBuffer.buffers.size(); i++)
   //   mGBuffer.buffers[i].namedTarget.setTexture(0, mGBuffer.buffers[i].handle);

   PROFILE_END();
}

//
//
//
//
//
//

void RenderPipeline::renderScene(ScenePassType passType, U32 objectMask)
{
   SceneCameraState cameraState = SceneCameraState::fromGFX();

   // Handle frustum locking.
   bool smLockDiffuseFrustum = false;

   const bool lockedFrustum = (smLockDiffuseFrustum && passType == SPT_Diffuse);
   if (lockedFrustum)
      cameraState = smLockedDiffuseCamera;
   else if (passType == SPT_Diffuse)
   {
      // Store the camera state so if we lock, this will become the
      // locked state.
      smLockedDiffuseCamera = cameraState;
   }

   // Create the render state.

   SceneRenderState renderState(gClientSceneGraph, passType, cameraState);

   // If we have locked the frustum, reset the view transform
   // on the render pass which the render state has just set
   // to the view matrix corresponding to the locked frustum.  For
   // rendering, however, we need the true view matrix from the
   // GFX state.

   if (lockedFrustum)
   {
      RenderPassManager* rpm = renderState.getRenderPass();
      rpm->assignSharedXform(RenderPassManager::View, GFX->getWorldMatrix());
   }

   // Render.

   renderScene(&renderState, objectMask);
}

//-----------------------------------------------------------------------------

void RenderPipeline::renderScene(SceneRenderState* renderState, U32 objectMask, SceneZoneSpace* baseObject, U32 baseZone)
{
   PROFILE_SCOPE(RenderPipeline_SG_renderScene);

   // Get the lights for rendering the scene.

   PROFILE_START(RenderPipeline_SG_registerLights);
   LIGHTMGR->registerGlobalLights(&renderState->getCullingFrustum(), false);
   PROFILE_END();

   // If its a diffuse pass, update the current ambient light level.
   // To do that find the starting zone and determine whether it has a custom
   // ambient light color.  If so, pass it on to the ambient light manager.
   // If not, use the ambient light color of the sunlight.
   //
   // Note that we retain the starting zone information here and pass it
   // on to renderSceneNoLights so that we don't need to look it up twice.

   if (renderState->isDiffusePass())
   {
      if (!baseObject && gClientSceneGraph->getZoneManager())
      {
         gClientSceneGraph->getZoneManager()->findZone(renderState->getCameraPosition(), baseObject, baseZone);
         AssertFatal(baseObject != NULL, "SceneManager::renderScene - findZone() did not return an object");
      }

      LinearColorF zoneAmbient;
      if (baseObject && baseObject->getZoneAmbientLightColor(baseZone, zoneAmbient))
         mAmbientLightColor = zoneAmbient;
      else
      {
         const LightInfo* sunlight = LIGHTMGR->getSpecialLight(LightManager::slSunLightType);
         if (sunlight)
            mAmbientLightColor = sunlight->getAmbient();
      }

      renderState->setAmbientLightColor(mAmbientLightColor);
   }

   // Trigger the pre-render signal.

   PROFILE_START(RenderPipeline_SG_preRenderSignal);
   mCurrentRenderState = renderState;
   //getPreRenderSignal().trigger(this, renderState);

   if (mShadowMapPass && renderState->isDiffusePass())
      mShadowMapPass->render(renderState, (U32)-1);

   mCurrentRenderState = NULL;
   PROFILE_END();

   // Render the scene.
   if (GFX->getCurrentRenderStyle() == GFXDevice::RS_StereoSideBySide)
   {
      // Store previous values
      RectI originalVP = GFX->getViewport();
      MatrixF originalWorld = GFX->getWorldMatrix();
      Frustum originalFrustum = GFX->getFrustum();

      // Save PFX & SceneManager projections
      MatrixF origNonClipProjection = renderState->getSceneManager()->getNonClipProjection();
      PFXFrameState origPFXState = PFXMGR->getFrameState();

      const FovPort *currentFovPort = GFX->getStereoFovPort();
      const MatrixF *worldEyeTransforms = GFX->getInverseStereoEyeTransforms();

      // Render left half of display
      GFX->activateStereoTarget(0);
      GFX->beginField();

      GFX->setWorldMatrix(worldEyeTransforms[0]);

      Frustum gfxFrustum = originalFrustum;
      MathUtils::makeFovPortFrustum(&gfxFrustum, gfxFrustum.isOrtho(), gfxFrustum.getNearDist(), gfxFrustum.getFarDist(), currentFovPort[0]);
      GFX->setFrustum(gfxFrustum);

      SceneCameraState cameraStateLeft = SceneCameraState::fromGFX();
      SceneRenderState renderStateLeft(gClientSceneGraph, renderState->getScenePassType(), cameraStateLeft);
      renderStateLeft.getSceneManager()->setNonClipProjection(GFX->getProjectionMatrix());
      renderStateLeft.setSceneRenderStyle(SRS_SideBySide);
      PFXMGR->setFrameMatrices(GFX->getWorldMatrix(), GFX->getProjectionMatrix());

      renderSceneNoLights(&renderStateLeft, objectMask, baseObject, baseZone); // left

                                                                               // Indicate that we've just finished a field
                                                                               //GFX->clear(GFXClearTarget | GFXClearZBuffer | GFXClearStencil, ColorI(255,0,0), 1.0f, 0);
      GFX->endField();

      // Render right half of display
      GFX->activateStereoTarget(1);
      GFX->beginField();
      GFX->setWorldMatrix(worldEyeTransforms[1]);

      gfxFrustum = originalFrustum;
      MathUtils::makeFovPortFrustum(&gfxFrustum, gfxFrustum.isOrtho(), gfxFrustum.getNearDist(), gfxFrustum.getFarDist(), currentFovPort[1]);
      GFX->setFrustum(gfxFrustum);

      SceneCameraState cameraStateRight = SceneCameraState::fromGFX();
      SceneRenderState renderStateRight(gClientSceneGraph, renderState->getScenePassType(), cameraStateRight);
      renderStateRight.getSceneManager()->setNonClipProjection(GFX->getProjectionMatrix());
      renderStateRight.setSceneRenderStyle(SRS_SideBySide);
      PFXMGR->setFrameMatrices(GFX->getWorldMatrix(), GFX->getProjectionMatrix());

      renderSceneNoLights(&renderStateRight, objectMask, baseObject, baseZone); // right

                                                                                // Indicate that we've just finished a field
                                                                                //GFX->clear(GFXClearTarget | GFXClearZBuffer | GFXClearStencil, ColorI(0,255,0), 1.0f, 0);
      GFX->endField();

      // Restore previous values
      renderState->getSceneManager()->setNonClipProjection(origNonClipProjection);
      PFXMGR->setFrameState(origPFXState);

      GFX->setWorldMatrix(originalWorld);
      GFX->setFrustum(originalFrustum);
      GFX->setViewport(originalVP);
   }
   else
   {
      //renderSceneNoLights(renderState, objectMask, baseObject, baseZone);

      mCurrentRenderState = renderState;

      _renderScene(mCurrentRenderState, objectMask, baseObject, baseZone);

      mCurrentRenderState = NULL;
   }

   // Trigger the post-render signal.

   PROFILE_START(RenderPipeline_SG_postRenderSignal);
   mCurrentRenderState = renderState;
   //gClientSceneGraph->getPostRenderSignal().trigger(gClientSceneGraph, renderState);

   PFXMGR->postRenderPass(renderState);

   mCurrentRenderState = NULL;
   PROFILE_END();

   // Remove the previously registered lights.

   PROFILE_START(RenderPipeline_SG_unregisterLights);
   LIGHTMGR->unregisterAllLights();
   PROFILE_END();
}

//-----------------------------------------------------------------------------

void RenderPipeline::renderSceneNoLights(SceneRenderState* renderState, U32 objectMask, SceneZoneSpace* baseObject, U32 baseZone)
{
   // Set the current state.

   mCurrentRenderState = renderState;

   // Render.

   

#ifdef TORQUE_DEBUG

   // If frustum is locked and this is a diffuse pass, render the culling volumes of
   // zones that are selected (or the volumes of the outdoor zone if no zone is
   // selected).

   //if (gEditingMission && renderState->isDiffusePass() && smLockDiffuseFrustum)
   //   renderState->getCullingState().debugRenderCullingVolumes();

#endif

   mCurrentRenderState = NULL;
}

//-----------------------------------------------------------------------------

void RenderPipeline::_renderScene(SceneRenderState* state, U32 objectMask, SceneZoneSpace* baseObject, U32 baseZone)
{
   //AssertFatal(this == gClientSceneGraph, "SceneManager::_buildSceneGraph - Only the client scenegraph can support this call!");

   PROFILE_SCOPE(RenderPipeline_SG_batchRenderImages);

   // In the editor, override the type mask for diffuse passes.

   //if (gEditingMission && state->isDiffusePass())
   //   objectMask = EDITOR_RENDER_TYPEMASK;

   MeshRenderSystem::render(gClientSceneGraph, state);

   // Update the zoning state and traverse zones.

   if (gClientSceneGraph->getZoneManager())
   {
      // Update.

      gClientSceneGraph->getZoneManager()->updateZoningState();

      // If zone culling isn't disabled, traverse the
      // zones now.

      if (!state->getCullingState().disableZoneCulling())
      {
         // Find the start zone if we haven't already.

         if (!baseObject)
         {
            gClientSceneGraph->getZoneManager()->findZone(state->getCameraPosition(), baseObject, baseZone);
            AssertFatal(baseObject != NULL, "SceneManager::_renderScene - findZone() did not return an object");
         }

         // Traverse zones starting in base object.

         SceneTraversalState traversalState(&state->getCullingState());
         PROFILE_START(RenderPipeline_Scene_traverseZones);
         baseObject->traverseZones(&traversalState, baseZone);
         PROFILE_END();

         // Set the scene render box to the area we have traversed.

         state->setRenderArea(traversalState.getTraversedArea());
      }
   }

   // Set the query box for the container query.  Never
   // make it larger than the frustum's AABB.  In the editor,
   // always query the full frustum as that gives objects
   // the opportunity to render editor visualizations even if
   // they are otherwise not in view.

   if (!state->getCullingFrustum().getBounds().isOverlapped(state->getRenderArea()))
   {
      // This handles fringe cases like flying backwards into a zone where you
      // end up pretty much standing on a zone border and looking directly into
      // its "walls".  In that case the traversal area will be behind the frustum
      // (remember that the camera isn't where visibility starts, it's the near
      // distance).

      return;
   }

   Box3F queryBox = state->getCullingFrustum().getBounds();
   //if (!gEditingMission)
   {
      queryBox.minExtents.setMax(state->getRenderArea().minExtents);
      queryBox.maxExtents.setMin(state->getRenderArea().maxExtents);
   }

   PROFILE_START(RenderPipeline_Scene_cullObjects);

   //TODO: We should split the codepaths here based on whether the outdoor zone has visible space.
   //    If it has, we should use the container query-based path.
   //    If it hasn't, we should fill the object list directly from the zone lists which will usually
   //       include way fewer objects.

   // Gather all objects that intersect the scene render box.

   mBatchQueryList.clear();
   gClientSceneGraph->getContainer()->findObjectList(queryBox, objectMask, &mBatchQueryList);

   // Cull the list.

   U32 numRenderObjects = state->getCullingState().cullObjects(
      mBatchQueryList.address(),
      mBatchQueryList.size(),
      !state->isDiffusePass() ? SceneCullingState::CullEditorOverrides : 0 // Keep forced editor stuff out of non-diffuse passes.
   );

   //HACK: If the control object is a Player and it is not in the render list, force
   // it into it.  This really should be solved by collision bounds being separate from
   // object bounds; only because the Player class is using bounds not encompassing
   // the actual player object is it that we have this problem in the first place.
   // Note that we are forcing the player object into ALL passes here but such
   // is the power of proliferation of things done wrong.

   /*GameConnection* connection = GameConnection::getConnectionToServer();
   if (connection)
   {
      Player* player = dynamic_cast< Player* >(connection->getControlObject());
      if (player)
      {
         mBatchQueryList.setSize(numRenderObjects);
         if (!mBatchQueryList.contains(player))
         {
            mBatchQueryList.push_back(player);
            numRenderObjects++;
         }
      }
   }*/

   PROFILE_END();

   //store our rendered objects into a list we can easily look up against later if required
   mRenderedObjectsList.clear();
   for (U32 i = 0; i < numRenderObjects; ++i)
   {
      mRenderedObjectsList.push_back(mBatchQueryList[i]);
   }

   // Render the remaining objects.
   //this basically fills out the elements to be rendered into the bins for our passes below
   PROFILE_START(RenderPipeline_Scene_renderObjects);
   state->renderObjects(mBatchQueryList.address(), numRenderObjects);
   PROFILE_END();

   //finally render our passes
   //iterate the bins and sort the elements, first
   for (U32 i = 0; i < mRenderBins.size(); i++)
   {
      mRenderBins[i]->sort();
   }

   for (U32 i = 0; i < mRenderPasses.size(); i++)
   {
      //cache the current render pass so the bins know what they're supposed to be referencing
      mCurrentRenderPass = mRenderPasses[i];

      _onPassPreRender(state);

      //Tell them to do their work
      mRenderPasses[i]->render(state);

      _onPassPostRender();
   }

   //we can finally clear the render insts
   for (U32 i = 0; i < mRenderBins.size(); i++)
   {
      mRenderBins[i]->clear();
   }

   // Render bounding boxes, if enabled.

   /*if (smRenderBoundingBoxes && state->isDiffusePass())
   {
      GFXDEBUGEVENT_SCOPE(Scene_renderBoundingBoxes, ColorI::WHITE);

      GameBase* cameraObject = 0;
      if (connection)
         cameraObject = connection->getCameraObject();

      GFXStateBlockDesc desc;
      desc.setFillModeWireframe();
      desc.setZReadWrite(true, false);

      for (U32 i = 0; i < numRenderObjects; ++i)
      {
         SceneObject* object = mBatchQueryList[i];

         // Skip global bounds object.
         if (object->isGlobalBounds())
            continue;

         // Skip camera object as we're viewing the scene from it.
         if (object == cameraObject)
            continue;

         const Box3F& worldBox = object->getWorldBox();
         GFX->getDrawUtil()->drawObjectBox(
            desc,
            Point3F(worldBox.len_x(), worldBox.len_y(), worldBox.len_z()),
            worldBox.getCenter(),
            MatrixF::Identity,
            ColorI::WHITE
         );
      }
   }*/
}

//
//
//
//
//
//

GFXFormat RenderPipeline::parseFormatString(StringTableEntry formatStr)
{
   if (formatStr == StringTable->insert("GFXFormatR8G8B8"))
      return GFXFormatR8G8B8;
   else if (formatStr == StringTable->insert("GFXFormatR8G8B8A8"))
      return GFXFormatR8G8B8A8;
   else if (formatStr == StringTable->insert("GFXFormatR8G8B8A8_SRGB"))
      return GFXFormatR8G8B8A8_SRGB;
   else if (formatStr == StringTable->insert("GFXFormatR8G8B8X8"))
      return GFXFormatR8G8B8X8;
   else if (formatStr == StringTable->insert("GFXFormatR32F"))
      return GFXFormatR32F;
   else if (formatStr == StringTable->insert("GFXFormatR5G6B5"))
      return GFXFormatR5G6B5;
   else if (formatStr == StringTable->insert("GFXFormatR5G5B5A1"))
      return GFXFormatR5G5B5A1;
   else if (formatStr == StringTable->insert("GFXFormatR5G5B5X1"))
      return GFXFormatR5G5B5X1;
   else if (formatStr == StringTable->insert("GFXFormatA4L4"))
      return GFXFormatA4L4;
   else if (formatStr == StringTable->insert("GFXFormatA8L8"))
      return GFXFormatA8L8;
   else if (formatStr == StringTable->insert("GFXFormatA8"))
      return GFXFormatA8;
   else if (formatStr == StringTable->insert("GFXFormatL8"))
      return GFXFormatL8;
   else if (formatStr == StringTable->insert("GFXFormatBC1"))
      return GFXFormatBC1;
   else if (formatStr == StringTable->insert("GFXFormatBC2"))
      return GFXFormatBC2;
   else if (formatStr == StringTable->insert("GFXFormatBC3"))
      return GFXFormatBC3;
   else if (formatStr == StringTable->insert("GFXFormatBC4"))
      return GFXFormatBC4;
   else if (formatStr == StringTable->insert("GFXFormatBC5"))
      return GFXFormatBC5;
   else if (formatStr == StringTable->insert("GFXFormatD24X8"))
      return GFXFormatD24X8;
   else if (formatStr == StringTable->insert("GFXFormatD24S8"))
      return GFXFormatD24S8;
   else if (formatStr == StringTable->insert("GFXFormatD24FS8"))
      return GFXFormatD24FS8;
   else if (formatStr == StringTable->insert("GFXFormatD16"))
      return GFXFormatD16;
   else if (formatStr == StringTable->insert("GFXFormatR32G32B32A32F"))
      return GFXFormatR32G32B32A32F;
   else if (formatStr == StringTable->insert("GFXFormatR16G16B16A16F"))
      return GFXFormatR16G16B16A16F;
   else if (formatStr == StringTable->insert("GFXFormatL16"))
      return GFXFormatL16;
   else if (formatStr == StringTable->insert("GFXFormatR16G16B16A16"))
      return GFXFormatR16G16B16A16;
   else if (formatStr == StringTable->insert("GFXFormatR16G16"))
      return GFXFormatR16G16;
   else if (formatStr == StringTable->insert("GFXFormatR16F"))
      return GFXFormatR16F;
   else if (formatStr == StringTable->insert("GFXFormatR16G16F"))
      return GFXFormatR16G16F;
   else if (formatStr == StringTable->insert("GFXFormatR10G10B10A2"))
      return GFXFormatR10G10B10A2;

   return GFXFormat::GFXFormat_UNKNOWNSIZE;
}

bool RenderPipeline::setGBufferTargetSizes(Point2I targetSize)
{
   for (U32 i = 0; i < mGBuffer.buffers.size(); ++i)
   {
      mGBuffer.buffers[i].namedTarget.setViewport(RectI(Point2I::Zero, targetSize));

      // Update the target size
      mGBuffer.buffers[i].namedTarget.release();

      mGBuffer.buffers[i].targetRef = GFX->allocRenderToTextureTarget();

      if (mGBuffer.buffers[i].handle.getWidthHeight() != targetSize ||
         mGBuffer.buffers[i].handle.getFormat() != mGBuffer.buffers[i].targetFormat)
      {
         mGBuffer.buffers[i].handle = GFXTexHandle(targetSize.x, targetSize.y, mGBuffer.buffers[i].targetFormat,
            &GFXRenderTargetProfile, avar("%s() - (line %d)", __FUNCTION__, __LINE__), 1, GFXTextureManager::AA_MATCH_BACKBUFFER);

         mGBuffer.buffers[i].targetRef->attachTexture(GFXTextureTarget::RenderSlot(GFXTextureTarget::Color0), mGBuffer.buffers[i].handle);

         mGBuffer.buffers[i].namedTarget.setTexture(0, mGBuffer.buffers[i].handle);
      }
   }

   return true;
}

bool RenderPipeline::addRenderTarget(StringTableEntry bufferName, StringTableEntry formatName, Point2I targetSize)
{
   //Example of the diffuse buffer getting set up
   /*if (mColorTex.getFormat() != colorFormat || mColorTex.getWidthHeight() != mTargetSize || GFX->recentlyReset())
   {
      mColorTarget.release();
      mColorTex.set(mTargetSize.x, mTargetSize.y, colorFormat,
         &GFXRenderTargetSRGBProfile, avar("%s() - (line %d)", __FUNCTION__, __LINE__),
         1, GFXTextureManager::AA_MATCH_BACKBUFFER);
      mColorTarget.setTexture(mColorTex);

      for (U32 i = 0; i < mTargetChainLength; i++)
         mTargetChain[i]->attachTexture(GFXTextureTarget::Color1, mColorTarget.getTexture());
   }*/

   bool ret = true;

   //if (!mSupportGBuffer)
      //return false;

   GBuffer::Buffer* buffer = mGBuffer.findBufferByName(bufferName);

   //If it doesn't already exist
   if (!buffer)
   {
      //set it up now
      mGBuffer.buffers.increment();

      buffer = &mGBuffer.buffers.last();

      buffer->bufferName = bufferName;
      buffer->targetFormat = parseFormatString(formatName);

      buffer->targetSize = targetSize;

      if (buffer->targetFormat == GFXFormat::GFXFormat_UNKNOWNSIZE)
         return false;
   }

   //Find target's name, if it exists already
   buffer->namedTarget.release();

   buffer->namedTarget = NamedTexTarget();

   buffer->targetRef = GFX->allocRenderToTextureTarget();

   //buffer->handle = GFXTexHandle(256,256, buffer->targetFormat, &GFXRenderTargetProfile, avar("%s() - (line %d)", __FUNCTION__, __LINE__), 1, GFXTextureManager::AA_MATCH_BACKBUFFER);
   //buffer->targetRef->attachTexture(GFXTextureTarget::RenderSlot(GFXTextureTarget::Color0), buffer->handle);

   //Set up the conditioner for the buffer. 
   /*GBuffer::Conditioners* cond = mGBuffer.findConditioner(buffer->targetFormat);
   if (!cond)
   {
      GBuffer::Conditioners newCond;
      newCond.format = buffer->targetFormat;
      newCond.conditioner = 
      mGBuffer.mConditioners.push_back(newCond);

      cond = &mGBuffer.mConditioners.last();
   }*/

   if (buffer->bufferName == RenderPipeline::NormalBufferName)
      buffer->conditioner = new GBufferConditioner(buffer->targetFormat, GBufferConditioner::RGB);
   else if (buffer->bufferName == RenderPipeline::LightInfoBufferName)
      buffer->conditioner = new AdvancedLightBufferConditioner(buffer->targetFormat, AdvancedLightBufferConditioner::RGB);
   else
      buffer->conditioner = nullptr;

   buffer->namedTarget.setSamplerState(GFXSamplerStateDesc::getClampPoint());
   buffer->namedTarget.setConditioner(buffer->conditioner);
   buffer->namedTarget.setViewport(RectI(Point2I::Zero, Point2I(256,256)));

   buffer->namedTarget.registerWithName(bufferName);

   Con::printf("%s : Successfully generated the GBuffer Target: %s of format %s", mPipelineName, bufferName, formatName);

   return true;
}

// Target management
void RenderPipeline::_onPassPreRender(SceneRenderState * state, bool preserve /* = false */)
{
   PROFILE_SCOPE(RenderTexTargetBinManager_onPreRender);

   mCurrentRenderState = state;

/*#ifndef TORQUE_SHIPPING
   AssertFatal(m_NeedsOnPostRender == false, "_onPostRender not called on RenderTexTargetBinManager, or sub-class.");
   m_NeedsOnPostRender = false;
#endif*/

   // Update the render target size
   /*const Point2I &rtSize = GFX->getActiveRenderTarget()->getSize();
   switch (mTargetSizeType)
   {
      case WindowSize:
         setTargetSize(rtSize);
         break;
      case WindowSizeScaled:
      {
         Point2I scaledTargetSize(mFloor(rtSize.x * mTargetScale.x), mFloor(rtSize.y * mTargetScale.y));
         setTargetSize(scaledTargetSize);
         break;
      }
      case FixedSize:
         // No adjustment necessary
         break;
   }

   if (mTargetChainLength == 0)
      return false;*/

   //We need to get and set all the targets that the pass expects.
   //The first one is the one set as the active render target
   //And any others are set as attached textures

   if (!mSupportGBuffer || mCurrentRenderPass->mRenderTargetNames.size() == 0)
   {
      return;
   }

   if (mCurrentRenderPass->mFormatToken == nullptr)
   {
      GBuffer::Buffer* primaryRenderTargetBuffer = mGBuffer.findBufferByName(mCurrentRenderPass->mRenderTargetNames[0]);

      if (primaryRenderTargetBuffer == nullptr || !primaryRenderTargetBuffer->targetRef.isValid())
      {
         Con::errorf("RenderPipeline::_onPassPreRender - Attempted to use an invalid target as the pass' primary render target!");
         return;
      }

      // Attach active depth target texture
      primaryRenderTargetBuffer->targetRef->attachTexture(GFXTextureTarget::DepthStencil, getRenderPass()->getDepthTargetTexture());

      // Preserve contents
      if (preserve)
         GFX->getActiveRenderTarget()->preserve();

      GFX->pushActiveRenderTarget();
      GFX->setActiveRenderTarget(primaryRenderTargetBuffer->targetRef);
      GFX->setViewport(primaryRenderTargetBuffer->namedTarget.getViewport());

      //roll through all our other pass targets and prep them now that the initial target is bound
      for (U32 i = 1; i < mCurrentRenderPass->mRenderTargetNames.size(); i++)
      {
         GBuffer::Buffer* targetBuffer = mGBuffer.findBufferByName(mCurrentRenderPass->mRenderTargetNames[i]);

         //GFX->setViewport(targetBuffer->namedTarget.getViewport());
         primaryRenderTargetBuffer->targetRef->attachTexture(GFXTextureTarget::RenderSlot(GFXTextureTarget::Color0 + i), targetBuffer->namedTarget.getTexture());
      }

   }
   else
   {
      //Forward pass, so we do our backbuffer swap voodoo here
      GBuffer::Buffer* primaryRenderTargetBuffer = mGBuffer.findBufferByName(mCurrentRenderPass->mRenderTargetNames[0]);

      primaryRenderTargetBuffer->namedTarget.setViewport(GFX->getViewport());

      // Update targets
      //_updateTargets();

      // If we have a copy PostEffect then get the active backbuffer copy 
      // now before we swap the render targets.
      GFXTexHandle curBackBuffer;
      if (mCurrentRenderPass->mFormatToken->mCopyPostEffect.isValid())
         curBackBuffer = PFXMGR->getBackBufferTex();

      // Push target
      GFX->pushActiveRenderTarget();
      GFX->setActiveRenderTarget(primaryRenderTargetBuffer->targetRef);

      // Set viewport
      GFX->setViewport(primaryRenderTargetBuffer->namedTarget.getViewport());

      // Clear
      GFX->clear(GFXClearTarget | GFXClearZBuffer | GFXClearStencil, /*gCanvasClearColor*/LinearColorF::BLACK, 1.0f, 0);

      // Set active z target on render pass
      if (mCurrentRenderPass->mFormatToken->mTargetDepthStencilTexture[0].isValid())
      {
         if (mCurrentRenderPass->getDepthTargetTexture() != GFXTextureTarget::sDefaultDepthStencil)
            mCurrentRenderPass->mFormatToken->mStoredPassZTarget = mCurrentRenderPass->getDepthTargetTexture();
         else
            mCurrentRenderPass->mFormatToken->mStoredPassZTarget = NULL;

         mCurrentRenderPass->setDepthTargetTexture(mCurrentRenderPass->mFormatToken->mTargetDepthStencilTexture[0]);
      }

      // Run the PostEffect which copies data into the new target.
      if (mCurrentRenderPass->mFormatToken->mCopyPostEffect.isValid())
         mCurrentRenderPass->mFormatToken->mCopyPostEffect->process(state, curBackBuffer, &mCurrentRenderPass->mFormatToken->mTarget.getViewport());
   }

/*#ifndef TORQUE_SHIPPING
   m_NeedsOnPostRender = true;
#endif*/
}

void RenderPipeline::_onPassPostRender()
{
   PROFILE_SCOPE(RenderTexTargetBinManager_onPostRender);

/*#ifndef TORQUE_SHIPPING
   m_NeedsOnPostRender = false;
#endif*/


   if (!mSupportGBuffer || mCurrentRenderPass->mRenderTargetNames.size() == 0)
   {
      return;
   }

   if (mCurrentRenderPass->mFormatToken == nullptr)
   {
      GBuffer::Buffer* primaryRenderTargetBuffer = mGBuffer.findBufferByName(mCurrentRenderPass->mRenderTargetNames[0]);

      primaryRenderTargetBuffer->targetRef->resolve();

      GFX->popActiveRenderTarget();

   }
   else
   {
      //Forward pass, so do our backbuffer swap voodoo here
      // Pop target
      GBuffer::Buffer* primaryRenderTargetBuffer = mGBuffer.findBufferByName(mCurrentRenderPass->mRenderTargetNames[0]);

      AssertFatal(GFX->getActiveRenderTarget() == primaryRenderTargetBuffer->targetRef, "Render target stack went wrong somewhere");
      primaryRenderTargetBuffer->targetRef->resolve();
      GFX->popActiveRenderTarget();
      primaryRenderTargetBuffer->namedTarget.setTexture(mCurrentRenderPass->mFormatToken->mTargetColorTexture[0]);

      // This is the GFX viewport when we were first processed.
      GFX->setViewport(primaryRenderTargetBuffer->namedTarget.getViewport());

      // Restore active z-target
      if (mCurrentRenderPass->mFormatToken->mTargetDepthStencilTexture[0].isValid())
      {
         mCurrentRenderPass->setDepthTargetTexture(mCurrentRenderPass->mFormatToken->mStoredPassZTarget.getPointer());
         mCurrentRenderPass->mFormatToken->mStoredPassZTarget = NULL;
      }

      // Run the PostEffect which copies data to the backbuffer
      if (mCurrentRenderPass->mFormatToken->mResolvePostEffect.isValid())
      {
         // Need to create a texhandle here, since inOutTex gets assigned during process()
         GFXTexHandle inOutTex = mCurrentRenderPass->mFormatToken->mTargetColorTexture[0];
         mCurrentRenderPass->mFormatToken->mResolvePostEffect->process(mCurrentRenderState, inOutTex, &primaryRenderTargetBuffer->namedTarget.getViewport());
      }
   }

   //for (U32 i = 0; i < mNumRenderTargets; i++)
   //   mNamedTarget.setTexture(i, mTargetChainTextures[mTargetChainIdx][i]);
}
//

//
//GBuffer stuff
RenderPipeline::GBuffer::Buffer* RenderPipeline::GBuffer::findBufferByName(StringTableEntry bufferName)
{
   for (U32 i = 0; i < buffers.size(); ++i)
   {
      if (buffers[i].bufferName == bufferName)
         return &buffers[i];
   }

   return nullptr;
}
DefineEngineFunction(getRenderPipeline, S32, (), , "")
{
   RenderPipeline* rp = RenderPipeline::get();

   if (rp)
      return rp->getId();

   return 0;
}

DefineEngineMethod(RenderPipeline, initialize, void, (), , "")
{
   object->initialize();
}

DefineEngineMethod(RenderPipeline, shutDown, void, (), , "")
{
   object->shutDownPipeline();
}

DefineEngineMethod(RenderPipeline, addRenderTarget, bool, (const char* targetName, const char* formatName, Point2I targetSize), ("", "", Point2I::Zero), "")
{
   bool ret = object->addRenderTarget(StringTable->insert(targetName), StringTable->insert(formatName), targetSize);
   return ret;
}

DefineEngineMethod(RenderPipeline, addRenderBin, void, (RenderBinManager* renderBinMgr), (nullAsType<RenderBinManager*>()), "")
{
   object->addRenderBin(renderBinMgr);
}

DefineEngineMethod(RenderPipeline, addRenderPass, void, (RenderPassManager* renderPassMgr), (nullAsType<RenderPassManager*>()), "")
{
   object->addRenderPass(renderPassMgr);
}
