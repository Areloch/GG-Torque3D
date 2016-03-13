#include "platform/platform.h"
#include "T3D/AmbientLightProbe.h"
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

#include "scene/reflectionManager.h"
#include "gfx/gfxDebugEvent.h"
#include "gfx/gfxTransformSaver.h"
#include "math/mathUtils.h"
#include "math/util/frustum.h"
#include "gfx/screenshot.h"
#include "gui/3d/guiTSControl.h"

#include "gfx/gfxDrawUtil.h"
#include "math/util/sphereMesh.h"

#include "gui/controls/guiTextCtrl.h"

#include "collision/optimizedPolyList.h"

#include "gfx/sim/cubemapData.h"

#include "gfx/gfxDrawUtil.h"
#include "math/util/sphereMesh.h"

//Render manager stuff
#include "core/module.h"
#include "scene/sceneManager.h"
#include "windowManager/platformWindowMgr.h"
#include "console/engineAPI.h"
#include "gui/core/guiCanvas.h"
//

#include "renderInstance/renderPrePassMgr.h"
#include "gfx/bitmap/pngUtils.h"
#include "gfx/bitmap/ddsUtils.h"

extern ColorI gCanvasClearColor;
static SphereMesh gSphere;

GFX_ImplementTextureProfile(AmbientLightProbeProfile,
   GFXTextureProfile::DiffuseMap,
   GFXTextureProfile::RenderTarget | GFXTextureProfile::KeepBitmap,
   GFXTextureProfile::NONE);

IMPLEMENT_CO_NETOBJECT_V1(AmbientLightProbe);

ConsoleDocClass(AmbientLightProbe,
   "@brief An invisible shape that allow objects within it to be lit with static global illumination.\n\n"

   "AmbientLightProbe is used to add static global illumination to an area of the scene. It's results are generated\n\n"
   "at runtime based on the settings configured in the editor. \n\n"

   "@ingroup enviroMisc"
   );

//-----------------------------------------------------------------------------
AmbientLightProbe::AmbientLightProbe()
   : mTransformDirty(true),
   mSilhouetteExtractor(mPolyhedron)
{
   VECTOR_SET_ASSOCIATION(mWSPoints);

   mNetFlags.set(Ghostable | ScopeAlways);
   mObjScale.set(1.f, 1.f, 1.f);
   mObjBox.set(
      Point3F(-0.5f, -0.5f, -0.5f),
      Point3F(0.5f, 0.5f, 0.5f)
      );

   mTypeMask |= LightObjectType;

   mObjToWorld.identity();
   mWorldToObj.identity();

   mLightInfoTarget = NULL;
   mPrepassTarget = NULL;
   mMatInfoTarget = NULL;
   mProbeShader = NULL;
   mProbeShaderConsts = NULL;
   mRenderTarget = NULL;

   resetWorldBox();

   RenderPassManager::getRenderBinSignal().notify(this, &AmbientLightProbe::_handleBinEvent);

   mOverrideSkyColor = false;
   mSkyColor = ColorF(0.5f, 0.5f, 1.0f, 1.0f);
   mGroundColor = ColorF(0.8f, 0.7f, 0.5f, 1.0f);
   mIntensity = 0.1f;

   mUseCubemap = false;
   mCubemap = NULL;

   mUseDynamicReflection = false;
   reflectorDesc = NULL;
   mCubeDescName = "";
}

AmbientLightProbe::~AmbientLightProbe()
{
   mLightInfoTarget = NULL;
   mPrepassTarget = NULL;
   mMatInfoTarget = NULL;
   mProbeShader = NULL;
   mProbeShaderConsts = NULL;
   mRenderTarget = NULL;
}

void AmbientLightProbe::initPersistFields()
{
   //
   addGroup("HorizonColorProbe");
   addField("OverrideSkyColor", TypeBool, Offset(mOverrideSkyColor, AmbientLightProbe), "Path of file to save and load results.");
   addField("SkyColor", TypeColorF, Offset(mSkyColor, AmbientLightProbe), "Path of file to save and load results.");
   addField("GroundColor", TypeColorF, Offset(mGroundColor, AmbientLightProbe), "Path of file to save and load results.");
   addField("Intensity", TypeF32, Offset(mIntensity, AmbientLightProbe), "Path of file to save and load results.");
   endGroup("HorizonColorProbe");

   addGroup("StaticProbe");
   addField("useCubemap", TypeBool, Offset(mUseCubemap, AmbientLightProbe), "Cubemap used instead of reflection texture if fullReflect is off.");
   addField("cubemap", TypeCubemapName, Offset(mCubemapName, AmbientLightProbe), "Cubemap used instead of reflection texture if fullReflect is off.");
   endGroup("StaticProbe");

   addGroup("DynamicProbe");
   addField("useDynamicReflection", TypeBool, Offset(mUseDynamicReflection, AmbientLightProbe), "Cubemap used instead of reflection texture if fullReflect is off.");
   addField("cubeReflectorDesc", TypeRealString, Offset(mCubeDescName, AmbientLightProbe),
      "References a ReflectorDesc datablock that defines performance and quality properties for dynamic reflections.\n");
   endGroup("DynamicProbe");

   Parent::initPersistFields();
}

//-----------------------------------------------------------------------------

void AmbientLightProbe::consoleInit()
{
   // Disable rendering of occlusion volumes by default.
   getStaticClassRep()->mIsRenderEnabled = false;
}

//-----------------------------------------------------------------------------

bool AmbientLightProbe::onAdd()
{
   if (!Parent::onAdd())
      return false;

   if (isClientObject())
   {
      _initShaders();
      
      mProbeShaderConsts->setSafe(mSkyColorSC, mSkyColor);
      mProbeShaderConsts->setSafe(mGroundColorSC, mGroundColor);
      mProbeShaderConsts->setSafe(mIntensitySC, mIntensity);

      if (mCubemapName.isNotEmpty())
         Sim::findObject(mCubemapName, mCubemap);

      if (mUseDynamicReflection)
      {
         if (mCubeDescName.isNotEmpty())
         {
            Sim::findObject(mCubeDescName, reflectorDesc);
         }
         else if (cubeDescId > 0)
         {
            Sim::findObject(cubeDescId, reflectorDesc);
         }
      }

      mPolyhedron.buildBox(MatrixF::Identity, this->getObjBox());
   }

   // Set up the silhouette extractor.
   mSilhouetteExtractor = SilhouetteExtractorType(mPolyhedron);

   return true;
}

void AmbientLightProbe::onRemove()
{
   if (isClientObject())
   {
      mCubeReflector.unregisterReflector();
   }

   Parent::onRemove();
}

//-----------------------------------------------------------------------------
void AmbientLightProbe::_renderObject(ObjectRenderInst* ri, SceneRenderState* state, BaseMatInstance* overrideMat)
{
   if (mCubeReflector.isRendering())
      return;

   Parent::_renderObject(ri, state, overrideMat);
}

void AmbientLightProbe::setTransform(const MatrixF& mat)
{
   Parent::setTransform(mat);
   mTransformDirty = true;
}

//-----------------------------------------------------------------------------

void AmbientLightProbe::buildSilhouette(const SceneCameraState& cameraState, Vector< Point3F >& outPoints)
{
   // Extract the silhouette of the polyhedron.  This works differently
   // depending on whether we project orthogonally or in perspective.

   TempAlloc< U32 > indices(mPolyhedron.getNumPoints());
   U32 numPoints;

   if (cameraState.getFrustum().isOrtho())
   {
      // Transform the view direction into object space.
      Point3F osViewDir;
      getWorldTransform().mulV(cameraState.getViewDirection(), &osViewDir);

      // And extract the silhouette.
      SilhouetteExtractorOrtho< ScenePolyhedralSpace::PolyhedronType > extractor(mPolyhedron);
      numPoints = extractor.extractSilhouette(osViewDir, indices, indices.size);
   }
   else
   {
      // Create a transform to go from view space to object space.
      MatrixF camView(true);
      camView.scale(Point3F(1.0f / getScale().x, 1.0f / getScale().y, 1.0f / getScale().z));
      camView.mul(getRenderWorldTransform());
      camView.mul(cameraState.getViewWorldMatrix());

      // Do a perspective-correct silhouette extraction.
      numPoints = mSilhouetteExtractor.extractSilhouette(
         camView,
         indices, indices.size);
   }

   // If we haven't yet, transform the polyhedron's points
   // to world space.
   if (mTransformDirty)
   {
      const U32 numPoints = mPolyhedron.getNumPoints();
      const ScenePolyhedralSpace::PolyhedronType::PointType* points = mPolyhedron.getPoints();

      mWSPoints.setSize(numPoints);
      for (U32 i = 0; i < numPoints; ++i)
      {
         Point3F p = points[i];
         p.convolve(getScale());
         getTransform().mulP(p, &mWSPoints[i]);
      }

      mTransformDirty = false;
   }

   // Now store the points.
   outPoints.setSize(numPoints);
   for (U32 i = 0; i < numPoints; ++i)
      outPoints[i] = mWSPoints[indices[i]];
}

//-----------------------------------------------------------------------------

U32 AmbientLightProbe::packUpdate(NetConnection *connection, U32 mask, BitStream *stream)
{
   U32 retMask = Parent::packUpdate(connection, mask, stream);

   stream->writeFlag(mOverrideSkyColor);
   stream->write(mSkyColor);
   stream->write(mGroundColor);
   stream->writeFloat(mIntensity, 7);

   stream->writeFlag(mUseCubemap);
   stream->write(mCubemapName);

   stream->writeFlag(mUseDynamicReflection);
   stream->writeString(mCubeDescName);

   return retMask;
}

void AmbientLightProbe::unpackUpdate(NetConnection *connection, BitStream *stream)
{
   Parent::unpackUpdate(connection, stream);

   mOverrideSkyColor = stream->readFlag();
   stream->read(&mSkyColor);
   stream->read(&mGroundColor);
   mIntensity = stream->readFloat(7);

   mUseCubemap = stream->readFlag();
   stream->read(&mCubemapName);

   if (mCubemapName.isNotEmpty())
      Sim::findObject(mCubemapName, mCubemap);

   mUseDynamicReflection = stream->readFlag();

   char buf[256];
   stream->readString(buf);
   mCubeDescName = buf;

   if (mUseDynamicReflection && mCubeDescName.isNotEmpty())
   {
      Sim::findObject(mCubeDescName, reflectorDesc);

      

      if (reflectorDesc)
         mCubeReflector.registerReflector(this, reflectorDesc);
   }
   else
   {
      mCubeReflector.unregisterReflector();
   }
   
}

//-----------------------------------------------------------------------------

void AmbientLightProbe::inspectPostApply()
{
   Parent::inspectPostApply();
   setMaskBits(U32(-1));
}

//--- Final Volume Rendering Into LightBuffer ---
void AmbientLightProbe::_initShaders()
{
   mProbeShader = NULL;
   mProbeShaderConsts = NULL;
   mPrepassTarget = NULL;

   // Need depth from pre-pass, so get the macros
   Vector<GFXShaderMacro> macros;

   if (!mPrepassTarget)
      mPrepassTarget = NamedTexTarget::find("prepass");

   if (mPrepassTarget)
      mPrepassTarget->getShaderMacros(&macros);

   ShaderData *shaderData;

   // Load Propagated Display Shader
   if (!Sim::findObject("AmbientLightProbeShaderData", shaderData))
   {
      Con::warnf("AmbientLightProbe::_initShader - failed to locate shader AmbientLightProbeShaderData!");
      return;
   }

   mProbeShader = shaderData->getShader(macros);
   if (!mProbeShader)
      return;

   mProbeShaderConsts = mProbeShader->allocConstBuffer();
   mEyePosWorldPropSC = mProbeShader->getShaderConstHandle("$eyePosWorld");
   mRTParamsPropSC = mProbeShader->getShaderConstHandle("$rtParams0");
   mVolumeStartPropSC = mProbeShader->getShaderConstHandle("$volumeStart");
   mVolumeSizePropSC = mProbeShader->getShaderConstHandle("$volumeSize");

   mInverseViewSC = mProbeShader->getShaderConstHandle("$invViewMat");

   mSkyColorSC = mProbeShader->getShaderConstHandle("$SkyColor");
   mGroundColorSC = mProbeShader->getShaderConstHandle("$GroundColor");
   mIntensitySC = mProbeShader->getShaderConstHandle("$Intensity");

   mUseCubemapSC = mProbeShader->getShaderConstHandle("$useCubemap");

   mCubemapSC = mProbeShader->getShaderConstHandle("$cubeMap");
}

void AmbientLightProbe::_handleBinEvent(RenderBinManager *bin,
   const SceneRenderState* sceneState, bool isBinStart)
{
   // We require a bin name to process effects... without
   // it we can skip the bin entirely.
   String binName(bin->getName());
   if (binName.isEmpty())
      return;

   if (!isBinStart && binName.equal("AL_LightBinMgr"))
   {
      if (!isClientObject())
         return;

      // -- Setup Render Target --
      if (!mRenderTarget)
         mRenderTarget = GFX->allocRenderToTextureTarget();

      if (!mRenderTarget) return;

      if (!mLightInfoTarget)
         mLightInfoTarget = NamedTexTarget::find("lightinfo");

      GFXTextureObject *texObject = mLightInfoTarget->getTexture();
      if (!texObject) return;

      mRenderTarget->attachTexture(GFXTextureTarget::Color0, texObject);

      //
      //MatInfo
      mMatInfoTarget = NamedTexTarget::find(RenderPrePassMgr::MatInfoBufferName);
      if (!mMatInfoTarget)
         return;
      GFXTextureObject *matTexObject = mMatInfoTarget->getTexture();
      if (!matTexObject) return;

      mRenderTarget->attachTexture(GFXTextureTarget::Color2, matTexObject);

      // We also need to sample from the depth buffer.
      if (!mPrepassTarget)
         mPrepassTarget = NamedTexTarget::find("prepass");

      GFXTextureObject *prepassTexObject = mPrepassTarget->getTexture();
      if (!prepassTexObject) return;

      // -- Setup screenspace quad to render (postfx) --
      Frustum frustum = sceneState->getCameraFrustum();
      GFXVertexBufferHandle<PFXVertex> vb;
      _updateScreenGeometry(frustum, &vb);

      // -- State Block --
      GFXStateBlockDesc desc;
      desc.setZReadWrite(false, false);
      desc.setBlend(true, GFXBlendOne, GFXBlendOne);
      desc.setFillModeSolid();

      // Camera position, used to calculate World Space position from depth buffer.
      const Point3F &camPos = sceneState->getCameraPosition();
      mProbeShaderConsts->setSafe(mEyePosWorldPropSC, camPos);

      MatrixF inverseViewMatrix = sceneState->getWorldViewMatrix();
      inverseViewMatrix.fullInverse();
      inverseViewMatrix.transpose();
      mProbeShaderConsts->setSafe(mInverseViewSC, inverseViewMatrix);

      // Volume position, used to calculate UV sampling.
      Box3F worldBox = getWorldBox();
      Point3F bottom_corner = worldBox.minExtents;
      Point3F top_corner = worldBox.maxExtents;
      Point3F volume_size = (top_corner - bottom_corner);
      mProbeShaderConsts->setSafe(mVolumeStartPropSC, bottom_corner);
      mProbeShaderConsts->setSafe(mVolumeSizePropSC, volume_size);

      //the lighting info
      if (!mOverrideSkyColor)
      {
         LightInfo *lightinfo = LIGHTMGR->getSpecialLight(LightManager::slSunLightType);
         const ColorF &sunlight = sceneState->getAmbientLightColor();

         ColorF ambientColor(sunlight.red, sunlight.green, sunlight.blue);
         mProbeShaderConsts->setSafe(mSkyColorSC, ambientColor);
      }
      else
      {
         mProbeShaderConsts->setSafe(mSkyColorSC, mSkyColor);
      }

      //next, set the ground color
      mProbeShaderConsts->setSafe(mGroundColorSC, mGroundColor);
      mProbeShaderConsts->setSafe(mIntensitySC, mIntensity);

      if (mUseCubemap && mCubemap)
      {
         mProbeShaderConsts->setSafe(mUseCubemapSC, 1.0f);

         if (!mCubemap->mCubemap)
            mCubemap->createMap();

         if (mUseDynamicReflection && mCubeReflector.isEnabled())
            GFX->setCubeTexture(1, mCubeReflector.getCubemap());
         else
            GFX->setCubeTexture(1, mCubemap->mCubemap);
      }
      else
      {
         GFX->setCubeTexture(1, NULL);
         mProbeShaderConsts->setSafe(mUseCubemapSC, 0.0f);
      }

      // Render Target Parameters.
      const Point3I &targetSz = texObject->getSize();
      const RectI &targetVp = mLightInfoTarget->getViewport();
      Point4F rtParams;
      ScreenSpace::RenderTargetParameters(targetSz, targetVp, rtParams);
      mProbeShaderConsts->setSafe(mRTParamsPropSC, rtParams);


      GFX->pushActiveRenderTarget();
      GFX->setActiveRenderTarget(mRenderTarget);
      GFX->setVertexBuffer(vb);
      GFX->setStateBlockByDesc(desc);
      GFX->setShader(mProbeShader);
      GFX->setShaderConstBuffer(mProbeShaderConsts);

      // Setup Textures
      GFX->setTexture(0, prepassTexObject);

      GFX->setTexture(2, matTexObject);

      // Draw the screenspace quad.
      GFX->drawPrimitive(GFXTriangleFan, 0, 2);

      // Clean Up
      mRenderTarget->resolve();
      GFX->popActiveRenderTarget();
   }
}

void AmbientLightProbe::_updateScreenGeometry(const Frustum &frustum,
   GFXVertexBufferHandle<PFXVertex> *outVB)
{

   // NOTE: GFXTransformSaver does not save/restore the frustum
   // so we must save it here before we modify it.
   F32 l, r, b, t, n, f;
   bool ortho;
   GFX->getFrustum(&l, &r, &b, &t, &n, &f, &ortho);

   outVB->set(GFX, 4, GFXBufferTypeVolatile);

   const Point3F *frustumPoints = frustum.getPoints();
   const Point3F& cameraPos = frustum.getPosition();

   // Perform a camera offset.  We need to manually perform this offset on the postFx's
   // polygon, which is at the far plane.
   const Point2F& projOffset = frustum.getProjectionOffset();
   Point3F cameraOffsetPos = cameraPos;
   if (!projOffset.isZero())
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

   vert->point.set(-1.0, -1.0, 0.0);
   vert->texCoord.set(0.0f, 1.0f);
   vert->wsEyeRay = frustumPoints[Frustum::FarBottomLeft] - cameraOffsetPos;
   vert++;

   vert->point.set(-1.0, 1.0, 0.0);
   vert->texCoord.set(0.0f, 0.0f);
   vert->wsEyeRay = frustumPoints[Frustum::FarTopLeft] - cameraOffsetPos;
   vert++;

   vert->point.set(1.0, 1.0, 0.0);
   vert->texCoord.set(1.0f, 0.0f);
   vert->wsEyeRay = frustumPoints[Frustum::FarTopRight] - cameraOffsetPos;
   vert++;

   vert->point.set(1.0, -1.0, 0.0);
   vert->texCoord.set(1.0f, 1.0f);
   vert->wsEyeRay = frustumPoints[Frustum::FarBottomRight] - cameraOffsetPos;
   vert++;

   outVB->unlock();

   // Restore frustum
   if (!ortho)
      GFX->setFrustum(l, r, b, t, n, f);
   else
      GFX->setOrtho(l, r, b, t, n, f);
}

void AmbientLightProbe::bakeFace(U32 faceidx)
{
   GFXDEBUGEVENT_SCOPE(AmbientLightProbe_BakeFace, ColorI::WHITE);

   // store current matrices
   GFXTransformSaver saver;

   // set projection to 90 degrees vertical and horizontal
   F32 left, right, top, bottom;
   F32 nearPlane = 0.1;
   F32 farDist = 1000;

   MathUtils::makeFrustum(&left, &right, &top, &bottom, M_HALFPI_F, 1.0f, nearPlane);
   GFX->setFrustum(left, right, bottom, top, nearPlane, farDist);

   // We don't use a special clipping projection, but still need to initialize 
   // this for objects like SkyBox which will use it during a reflect pass.
   gClientSceneGraph->setNonClipProjection(GFX->getProjectionMatrix());

   // Standard view that will be overridden below.
   VectorF vLookatPt(0.0f, 0.0f, 0.0f), vUpVec(0.0f, 0.0f, 0.0f), vRight(0.0f, 0.0f, 0.0f);

   switch (faceidx)
   {
   case 0: // D3DCUBEMAP_FACE_POSITIVE_X:
      vLookatPt = VectorF(1.0f, 0.0f, 0.0f);
      vUpVec = VectorF(0.0f, 1.0f, 0.0f);
      break;
   case 1: // D3DCUBEMAP_FACE_NEGATIVE_X:
      vLookatPt = VectorF(-1.0f, 0.0f, 0.0f);
      vUpVec = VectorF(0.0f, 1.0f, 0.0f);
      break;
   case 2: // D3DCUBEMAP_FACE_POSITIVE_Y:
      vLookatPt = VectorF(0.0f, 1.0f, 0.0f);
      vUpVec = VectorF(0.0f, 0.0f, -1.0f);
      break;
   case 3: // D3DCUBEMAP_FACE_NEGATIVE_Y:
      vLookatPt = VectorF(0.0f, -1.0f, 0.0f);
      vUpVec = VectorF(0.0f, 0.0f, 1.0f);
      break;
   case 4: // D3DCUBEMAP_FACE_POSITIVE_Z:
      vLookatPt = VectorF(0.0f, 0.0f, 1.0f);
      vUpVec = VectorF(0.0f, 1.0f, 0.0f);
      break;
   case 5: // D3DCUBEMAP_FACE_NEGATIVE_Z:
      vLookatPt = VectorF(0.0f, 0.0f, -1.0f);
      vUpVec = VectorF(0.0f, 1.0f, 0.0f);
      break;
   }

   // create camera matrix
   VectorF cross = mCross(vUpVec, vLookatPt);
   cross.normalizeSafe();

   MatrixF matView(true);
   matView.setColumn(0, cross);
   matView.setColumn(1, vLookatPt);
   matView.setColumn(2, vUpVec);
   matView.setPosition(getPosition());
   matView.inverse();

   GFX->setWorldMatrix(matView);

   Point2I destSize = Point2I(1024, 1024);
   GFXTexHandle blendTex;
   blendTex.set(destSize.x, destSize.y, GFXFormatR8G8B8A8, &GFXDefaultRenderTargetProfile, "");

   GFXTextureTargetRef mBaseTarget = GFX->allocRenderToTextureTarget();
   mBaseTarget->attachTexture(GFXTextureTarget::Color0, blendTex);
   GFX->setActiveRenderTarget(mBaseTarget);

   GFX->clear(GFXClearStencil | GFXClearTarget | GFXClearZBuffer, gCanvasClearColor, 1.0f, 0);

   SceneRenderState reflectRenderState
      (
      gClientSceneGraph,
      SPT_Reflect,
      SceneCameraState::fromGFX()
      );

   reflectRenderState.getMaterialDelegate().bind(REFLECTMGR, &ReflectionManager::getReflectionMaterial);
   reflectRenderState.setDiffuseCameraTransform(matView);

   // render scene
   LIGHTMGR->registerGlobalLights(&reflectRenderState.getCullingFrustum(), false);
   gClientSceneGraph->renderScene(&reflectRenderState, -1);
   LIGHTMGR->unregisterAllLights();

   mBaseTarget->resolve();

   GFXTextureObject *faceTexture;

   char fileName[256];
   dSprintf(fileName, 256, "bakeTest%i.png", faceidx);

   FileStream stream;
   if (!stream.open(fileName, Torque::FS::File::Write))
   {
      return;
   }

   GBitmap bitmap(blendTex->getWidth(), blendTex->getHeight(), false, GFXFormatR8G8B8);
   blendTex->copyToBmp(&bitmap);
   bitmap.writeBitmap("png", stream);
}

DefineEngineMethod(AmbientLightProbe, Bake, void, (), ,
   "@brief returns true if control object is inside the fog\n\n.")
{
   for (U32 i = 0; i < 6; i++)
      object->bakeFace(i);
}