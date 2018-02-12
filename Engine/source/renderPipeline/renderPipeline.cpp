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

RenderPipeline* RenderPipeline::smRenderPipeline(0);

IMPLEMENT_CONOBJECT(RenderPipeline);

RenderPipeline::RenderPipeline()
{
   mPipelineName = StringTable->EmptyString();
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

   addField("shapeFile", TypeStringFilename, Offset(mModelName, RenderPipeline),
      "The object model shape file to show in the view.");
}

void RenderPipeline::initialize()
{
   //Init the pipeline as the active statically available pipeline
   //If it fails, bail out now
   if (!initPipeline())
      return;

   //Begin the real setup of the pipeline's behavior. This informs the rest of the rendering's configuration
   if (mSupportGBuffer)
   {
      //Inform the shader system that we're deferred
      GFXShader::addGlobalMacro("TORQUE_DEFERRED_LIGHTING");

      // Tell the material manager that deferred is enabled.
      MATMGR->setDeferredEnabled(true);
   }

   GFXShader::addGlobalMacro("TORQUE_LINEAR_DEPTH");

   //set up the clearGBuffer shader
   //mClearGBufferShader = NULL;

   // Find a target format that supports blending... 
   // we prefer the floating point format if it works.
   Vector<GFXFormat> formats;
   formats.push_back(GFXFormatR16G16B16A16F);
   //formats.push_back( GFXFormatR16G16B16A16 );
   GFXFormat blendTargetFormat = GFX->selectSupportedFormat(&GFXRenderTargetProfile,
      formats,
      true,
      true,
      false);

   setupBuffers();
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

//
/*void RenderPipeline::renderViews()
{
	//Walk through all our active cameras, and init a render from them
	for (;;)
	{
		render();
	}
}*/

void RenderPipeline::render()
{
   //Process our gbuffer condition if it's used
   // Skip deferred features, and use forward shading instead
   /*if (mSupportGBuffer)
   {
      // Pull in the uncondition method for the light info buffer
      NamedTexTarget *texTarget = NamedTexTarget::find(AdvancedLightBinManager::smBufferName);
      if (texTarget && texTarget->getConditioner())
      {
         ConditionerMethodDependency *unconditionMethod = texTarget->getConditioner()->getConditionerMethodDependency(ConditionerFeature::UnconditionMethod);
         unconditionMethod->createMethodMacro(String::ToLower(AdvancedLightBinManager::smBufferName) + "Uncondition", macros);
         addDependency(unconditionMethod);
      }
   }*/

	//Step 1 process all our render systems to draw the scene
	//RenderStaticMeshSytem::render();
	//RenderMeshSystem::render();

	//render any special snowflake non-e/c/s based classes
	//Scene::render();

	//Now that we've written all the visible geometry to our gBuffer, render lights
	//LightManager::renderLights();

	//Initiate our post process effects, starting with the deferred combine(as this mockup is currently written as Deferred)
	//PostFX::render();

	//Frame complete, ensure targets are resolved and that cleanup happens!
	//cleanup();

   //
   //
   //
   GFXTransformSaver _saveTransforms;

   // Determine the camera position, and store off render state.
   MatrixF modelview;
   MatrixF mv;
   Point3F cp;

   modelview = GFX->getWorldMatrix();

   mv = modelview;
   mv.inverse();
   mv.getColumn(3, &cp);

   RenderPassManager* renderPass = gClientSceneGraph->getDefaultRenderPass();

   U32 time = Platform::getVirtualMilliseconds();
   U32 dt = time - mLastRenderTime;
   mLastRenderTime = time;

   //our impromptu sun
   LightInfo* mLight = LIGHTMGR->createLightInfo();

   mLight->setColor(ColorI(255,255,255));
   mLight->setAmbient(ColorI(0,0,0));
   mLight->setDirection(Point3F(0,0,1));

   LIGHTMGR->unregisterAllLights();
   LIGHTMGR->setSpecialLight(LightManager::slSunLightType, mLight);

   GFXStateBlockDesc d;

   d.cullDefined = true;
   d.cullMode = GFXCullNone;
   d.zDefined = true;
   d.zEnable = false;

   GFXStateBlock* stateBlock = GFX->createStateBlock(d);

   GFX->setStateBlock(stateBlock);

   F32 left, right, top, bottom, nearPlane, farPlane;
   bool isOrtho;
   GFX->getFrustum(&left, &right, &bottom, &top, &nearPlane, &farPlane, &isOrtho);

   Frustum frust(false, left, right, top, bottom, nearPlane, farPlane, MatrixF::Identity);

   SceneRenderState state
   (
      gClientSceneGraph,
      SPT_Diffuse,
      SceneCameraState(GFX->getViewport(), frust, MatrixF::Identity, GFX->getProjectionMatrix()),
      renderPass,
      true
   );

   // Set up our TS render state here.   
   TSRenderState rdata;
   rdata.setSceneState(&state);

   // We might have some forward lit materials
   // so pass down a query to gather lights.
   LightQuery query;
   query.init(SphereF(Point3F::Zero, 1.0f));
   rdata.setLightQuery(&query);

   if (mModelName.isEmpty())
      return;

   Resource< TSShape > model = ResourceManager::get().load(mModelName);
   if (!model)
   {
      Con::warnf("GuiObjectView::setObjectModel - Failed to load model '%s'", mModelName.c_str());
      return;
   }

   // Instantiate it.

   mModel = new TSShapeInstance(model, true);

   // Render primary model.

   if (mModel)
   {
      mModel->render(rdata);
   }

   //MeshRenderSystem::render(nullptr, state);

   // Render mounted model.

   /*if (mMountedModel && mMountNode != -1)
   {
      GFX->pushWorldMatrix();
      GFX->multWorld(mModel->mNodeTransforms[mMountNode]);
      GFX->multWorld(mMountTransform);

      mMountedModel->render(rdata);

      GFX->popWorldMatrix();
   }*/

   renderPass->renderPass(&state);

   // Make sure to remove our fake sun.
   LIGHTMGR->unregisterAllLights();

}
//

//================================================================================================
//Render a full frame from a given transform and frustum, and render out to a target
//================================================================================================
void RenderPipeline::renderFrame(GFXTextureTargetRef* target, MatrixF transform, Frustum frustum, U32 typeMask, ColorI canvasClearColor)
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

      // Set the default non-clip projection as some 
      // objects depend on this even in non-reflect cases.
      gClientSceneGraph->setNonClipProjection(mSaveProjection);

      // Give the post effect manager the worldToCamera, and cameraToScreen matrices
      PFXMGR->setFrameMatrices(mSaveModelview, mSaveProjection);

      // GameRenderWorld()
      PROFILE_START(RenderPipeline_RenderFrame_RenderWorld);
      FrameAllocator::setWaterMark(0);

      //gClientSceneGraph->renderScene(SPT_Reflect, typeMask);
      gClientSceneGraph->renderScene(SPT_Diffuse, typeMask);

      SceneCameraState cameraState = SceneCameraState::fromGFX();
      SceneRenderState renderState(gClientSceneGraph, SPT_Diffuse, cameraState);

      //renderScene(&renderState, typeMask);

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

   PROFILE_END();
}

//
void RenderPipeline::renderScene(SceneRenderState* renderState, U32 objectMask)
{
   PROFILE_SCOPE(RenderPipeline_renderScene);

   // Get the lights for rendering the scene.

   PROFILE_START(RenderPipeline_registerLights);
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
      SceneZoneSpace* baseObject = nullptr;
      U32 baseZone;

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
            //mAmbientLightColor.setTargetValue(sunlight->getAmbient());
            mAmbientLightColor = sunlight->getAmbient();
      }

      renderState->setAmbientLightColor(mAmbientLightColor);
   }

   // Trigger the pre-render signal.

   //NOTE:
   //This is pretty much just used for some debug rendering stuff in the editors
   //Re-establish in a better setup later

   //Turns out this part is mandatory for ensuring lighting is properly set up.
   //without it, we get the dreaded shadowSoftness error in vectorLight's shader
   PROFILE_START(RenderPipeline_preRenderSignal);
   //mCurrentRenderState = renderState;
   gClientSceneGraph->getPreRenderSignal().trigger(gClientSceneGraph, renderState);
   //mCurrentRenderState = NULL;
   PROFILE_END();

   // Render the scene.

   //TODO: add a support flag for SBS rendering in the renderPipeline so we know if we even want to support it
   /*if (GFX->getCurrentRenderStyle() == GFXDevice::RS_StereoSideBySide)
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
      SceneRenderState renderStateLeft(this, renderState->getScenePassType(), cameraStateLeft);
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
      SceneRenderState renderStateRight(this, renderState->getScenePassType(), cameraStateRight);
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
   {*/
      //To clarify on an otherwise potentially confusing naming convention
      //This function basically renders the guts of the scene
      //In a deferred context, this will populate our buffer targets that the post process step afterwards will use in deferred/combine shaders to assemble into our final target
      //In a forward contex it's basically where the render magic actually happens.
      //In other words, this name is stupid and I'll be changing it, yes
      _renderScene(renderState, objectMask);
   //}

   // Trigger the post-render signal.

   //NOTE
   //This is basically the post effects and the physics debug renderer
   //Post effects manager basically meaning "this is where all the actual final deferred and combine rendering magic happens
   //Everything before this was just filling the buffer targets for this stage to actually utilize for the final
   //Well, at least, in a deferred renderer context
   //In a non-deferred context, all the magic just happened and why are you even here?
   PROFILE_START(RenderPipeline_postRenderSignal);
   //mCurrentRenderState = renderState;
   gClientSceneGraph->getPostRenderSignal().trigger(gClientSceneGraph, renderState);
   //mCurrentRenderState = NULL;
   PROFILE_END();

   // Remove the previously registered lights.

   PROFILE_START(RenderPipeline_unregisterLights);
   LIGHTMGR->unregisterAllLights();
   PROFILE_END();
}

void RenderPipeline::_renderScene(SceneRenderState* renderState, U32 objectMask)
{
   renderNonSystems(renderState, objectMask);

   MeshRenderSystem::render(nullptr, renderState);
}

void RenderPipeline::renderNonSystems(SceneRenderState* renderState, U32 objectMask)
{
   // Set the current state.

   //mCurrentRenderState = renderState;

   SceneZoneSpace* baseObject = nullptr;
   U32 baseZone;

   // Render.

   //AssertFatal(this == gClientSceneGraph, "SceneManager::_buildSceneGraph - Only the client scenegraph can support this call!");

   PROFILE_SCOPE(RenderPipeline_renderNonSystems);

   // In the editor, override the type mask for diffuse passes.

   //if (gEditingMission && renderState->isDiffusePass())
   //   objectMask = EDITOR_RENDER_TYPEMASK;

   // Update the zoning state and traverse zones.

   //NOTE
   //Systems will deal with the rendering of objects, this is unneeded here
   if (gClientSceneGraph->getZoneManager())
   {
      // Update.

      gClientSceneGraph->getZoneManager()->updateZoningState();

      // If zone culling isn't disabled, traverse the
      // zones now.

      if (!renderState->getCullingState().disableZoneCulling())
      {
         // Find the start zone if we haven't already.

         if (!baseObject)
         {
            gClientSceneGraph->getZoneManager()->findZone(renderState->getCameraPosition(), baseObject, baseZone);
            AssertFatal(baseObject != NULL, "SceneManager::_renderScene - findZone() did not return an object");
         }

         // Traverse zones starting in base object.

         SceneTraversalState traversalState(&renderState->getCullingState());
         PROFILE_START(RenderPipeline_traverseZones);
         baseObject->traverseZones(&traversalState, baseZone);
         PROFILE_END();

         // Set the scene render box to the area we have traversed.

         renderState->setRenderArea(traversalState.getTraversedArea());
      }
   }

   // Set the query box for the container query.  Never
   // make it larger than the frustum's AABB.  In the editor,
   // always query the full frustum as that gives objects
   // the opportunity to render editor visualizations even if
   // they are otherwise not in view.

   if (!renderState->getCullingFrustum().getBounds().isOverlapped(renderState->getRenderArea()))
   {
      // This handles fringe cases like flying backwards into a zone where you
      // end up pretty much standing on a zone border and looking directly into
      // its "walls".  In that case the traversal area will be behind the frustum
      // (remember that the camera isn't where visibility starts, it's the near
      // distance).

      return;
   }

   Box3F queryBox = renderState->getCullingFrustum().getBounds();
   //if (!gEditingMission)
   {
      queryBox.minExtents.setMax(renderState->getRenderArea().minExtents);
      queryBox.maxExtents.setMin(renderState->getRenderArea().maxExtents);
   }

   PROFILE_START(RenderPipeline_cullObjects);

   //TODO: We should split the codepaths here based on whether the outdoor zone has visible space.
   //    If it has, we should use the container query-based path.
   //    If it hasn't, we should fill the object list directly from the zone lists which will usually
   //       include way fewer objects.

   // Gather all objects that intersect the scene render box.

   mBatchQueryList.clear();
   gClientSceneGraph->getContainer()->findObjectList(queryBox, objectMask, &mBatchQueryList);

   // Cull the list.

   U32 numRenderObjects = renderState->getCullingState().cullObjects(
      mBatchQueryList.address(),
      mBatchQueryList.size(),
      !renderState->isDiffusePass() ? SceneCullingState::CullEditorOverrides : 0 // Keep forced editor stuff out of non-diffuse passes.
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
      Player* player = dynamic_cast<Player*>(connection->getControlObject());
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

   PROFILE_START(RenderPipeline_renderNonSystemObjects);
   renderState->renderObjects(mBatchQueryList.address(), numRenderObjects);
   PROFILE_END();

   // Render bounding boxes, if enabled.

   /*if (smRenderBoundingBoxes && renderState->isDiffusePass())
   {
      GFXDEBUGEVENT_SCOPE(RenderPipeline_renderBoundingBoxes, ColorI::WHITE);

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

   #ifdef TORQUE_DEBUG

   // If frustum is locked and this is a diffuse pass, render the culling volumes of
   // zones that are selected (or the volumes of the outdoor zone if no zone is
   // selected).

   //if (gEditingMission && renderState->isDiffusePass() && smLockDiffuseFrustum)
   //   renderState->getCullingState().debugRenderCullingVolumes();

   #endif

   //mCurrentRenderState = NULL;
}


/*bool RenderPipeline::GBuffer::setTargetChannels(Target *_target, RenderPipeline::GBuffer::RenderTargets _renderTarget, U32 _channels)
{
	//find the RTData if it's already had channels assigned
	bool found = false;
	RenderTargetData* data = nullptr;

	for (U32 i = 0; i < rtData.size(); ++i)
	{
		if (rtData[i].id == _renderTarget)
			data = &rtData[i];
	}

	if (!found)
	{
		RenderTargetData newData;
		newData.id = _renderTarget;
		rtData.push_back(newData);

		data = &newData;
	}

	//Now that we have our data, find out if we have any overlap in the channels requested, if so, fail out
	if (data->channelsInUse & _channels)
	{
		//yep, failure
		return false;
	}
	else
	{
		data->channelsInUse |= _channels;

		_target->channels = _channels;
		_target->renderTarget = _renderTarget;

		return true;
	}
}*/

bool RenderPipeline::setGBufferTarget(String bufferName, String targetName, String formatName)
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

   return true;
}

DefineEngineFunction(getRenderPipeline, S32, (), , "")
{
   RenderPipeline* rp = RenderPipeline::getRenderPipeline();

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

DefineEngineMethod(RenderPipeline, setGBufferTarget, bool, (String bufferName, String targetName, String formatName), ("", "", ""), "")
{
   return object->setGBufferTarget(bufferName, targetName, formatName);
}

