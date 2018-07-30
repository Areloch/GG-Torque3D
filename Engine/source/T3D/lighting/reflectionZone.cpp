#include "platform/platform.h"
#include "T3D/lighting/reflectionZone.h"
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

IMPLEMENT_CO_NETOBJECT_V1(ReflectionZone);

ConsoleDocClass(ReflectionZone,
   "@brief An invisible shape that allow objects within it to be lit with static global illumination.\n\n"

   "ReflectionZone is used to add static global illumination to an area of the scene. It's results are generated\n\n"
   "at runtime based on the settings configured in the editor. \n\n"

   "@ingroup enviroMisc"
);

//-----------------------------------------------------------------------------

ReflectionZone::ReflectionZone()
   : mTransformDirty(true),
   mSilhouetteExtractor(mPolyhedron)
{
   VECTOR_SET_ASSOCIATION(mWSPoints);
   VECTOR_SET_ASSOCIATION(mVolumeQueryList);

   mNetFlags.set(Ghostable | ScopeAlways);
   mObjScale.set(1.f, 1.f, 1.f);
   mObjBox.set(
      Point3F(-0.5f, -0.5f, -0.5f),
      Point3F(0.5f, 0.5f, 0.5f)
   );

   mObjToWorld.identity();
   mWorldToObj.identity();

   mLightInfoTarget = nullptr;
   mIndirectLightTarget = nullptr;
   mDeferredTarget = nullptr;
   mMatInfoTarget = nullptr;
   mZoneShader = nullptr;
   mZoneShaderConsts = nullptr;
   mRenderTarget = nullptr;

   mEyePosWorldPropSC = nullptr;

   resetWorldBox();

   RenderPassManager::getRenderBinSignal().notify(this, &ReflectionZone::_handleBinEvent);
}

ReflectionZone::~ReflectionZone()
{
}

void ReflectionZone::initPersistFields()
{
   Parent::initPersistFields();
}

//-----------------------------------------------------------------------------

void ReflectionZone::consoleInit()
{
   // Disable rendering of occlusion volumes by default.
   getStaticClassRep()->mIsRenderEnabled = false;
}

//-----------------------------------------------------------------------------

bool ReflectionZone::onAdd()
{
   if (!Parent::onAdd())
      return false;

   // Set up the silhouette extractor.
   mSilhouetteExtractor = SilhouetteExtractorType(mPolyhedron);

   return true;
}

void ReflectionZone::onRemove()
{
   if (isClientObject())
   {

   }

   Parent::onRemove();
}

//-----------------------------------------------------------------------------
// Renders the visualization of the volume in the editor.
void ReflectionZone::_renderObject(ObjectRenderInst* ri, SceneRenderState* state, BaseMatInstance* overrideMat)
{
   Parent::_renderObject(ri, state, overrideMat);

   GFXStateBlockDesc desc;
   desc.setZReadWrite(true, true);
   desc.setBlend(false);
   desc.setFillModeSolid();
}

//-----------------------------------------------------------------------------

void ReflectionZone::setTransform(const MatrixF& mat)
{
   Parent::setTransform(mat);
   mTransformDirty = true;
}

//-----------------------------------------------------------------------------

void ReflectionZone::buildSilhouette(const SceneCameraState& cameraState, Vector< Point3F >& outPoints)
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
      SilhouetteExtractorOrtho< PolyhedronType > extractor(mPolyhedron);
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
      const PolyhedronType::PointType* points = getPolyhedron().getPoints();

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

U32 ReflectionZone::packUpdate(NetConnection *connection, U32 mask, BitStream *stream)
{
   U32 retMask = Parent::packUpdate(connection, mask, stream);

   return retMask;
}

void ReflectionZone::unpackUpdate(NetConnection *connection, BitStream *stream)
{
   Parent::unpackUpdate(connection, stream);
}

//-----------------------------------------------------------------------------

void ReflectionZone::inspectPostApply()
{
   Parent::inspectPostApply();
   setMaskBits(U32(-1));
}

void ReflectionZone::_initShaders()
{
   mZoneShader = NULL;
   mZoneShaderConsts = NULL;
   mDeferredTarget = NULL;

   // Need depth from pre-pass, so get the macros
   Vector<GFXShaderMacro> macros;

   if (!mDeferredTarget)
      mDeferredTarget = NamedTexTarget::find("deferred");

   if (mDeferredTarget)
      mDeferredTarget->getShaderMacros(&macros);

   ShaderData *shaderData;

   // Load Propagated Display Shader
   if (!Sim::findObject("ReflectionZoneShaderData", shaderData))
   {
      Con::warnf("ReflectionZone::_initShader - failed to locate shader ReflectionZoneShaderData!");
      return;
   }

   // SSAO MASK
   //if (AdvancedLightBinManager::smUseSSAOMask)
    //  macros.push_back(GFXShaderMacro("USE_SSAO_MASK"));

   mZoneShader = shaderData->getShader(macros);
   if (!mZoneShader)
      return;

   mZoneShaderConsts = mZoneShader->allocConstBuffer();
   mEyePosWorldPropSC = mZoneShader->getShaderConstHandle("$eyePosWorld");
   mRTParamsPropSC = mZoneShader->getShaderConstHandle("$rtParams0");
   mVolumeStartPropSC = mZoneShader->getShaderConstHandle("$volumeStart");
   mVolumeSizePropSC = mZoneShader->getShaderConstHandle("$volumeSize");
}

void ReflectionZone::_handleBinEvent(RenderBinManager *bin,
   const SceneRenderState* sceneState,
   bool isBinStart)
{

   // We require a bin name to process effects... without
   // it we can skip the bin entirely.
   String binName(bin->getName());
   if (binName.isEmpty())
      return;

   if (!isBinStart && binName.equal("AL_LightBinMgr"))
   {
      _renderProbes(sceneState);
   }
}

void ReflectionZone::_renderProbes(const SceneRenderState* state)
{
   if (/*!mPropagatedTexture || */!isClientObject())
      return;

   // -- Setup Render Target --
   if (!mRenderTarget)
      mRenderTarget = GFX->allocRenderToTextureTarget();

   if (!mRenderTarget) 
      return;

   if (!mLightInfoTarget)
      mLightInfoTarget = NamedTexTarget::find("directLighting");

   if (!mIndirectLightTarget)
      mIndirectLightTarget = NamedTexTarget::find("indirectLighting");

   GFXTextureObject *texObject = mIndirectLightTarget->getTexture();
   if (!texObject) 
      return;

   mRenderTarget->attachTexture(GFXTextureTarget::Color0, texObject);

   // We also need to sample from the depth buffer.
   if (!mDeferredTarget)
      mDeferredTarget = NamedTexTarget::find("deferred");

   GFXTextureObject *deferredTexObject = mDeferredTarget->getTexture();
   if (!deferredTexObject) 
      return;

   if (mEyePosWorldPropSC == nullptr)
      return;

   // -- Setup screenspace quad to render (postfx) --
   Frustum frustum = state->getCameraFrustum();
   GFXVertexBufferHandle<PFXVertex> vb;
   _updateScreenGeometry(frustum, &vb);

   // -- State Block --
   GFXStateBlockDesc desc;
   desc.setZReadWrite(false, false);
   desc.setBlend(true, GFXBlendOne, GFXBlendOne);
   desc.setFillModeSolid();

   // Point sampling is useful for debugging to see the exact color values in each cell.
   //desc.samplers[0] = GFXSamplerStateDesc::getClampPoint();
   //desc.samplersDefined = true;

   // Camera position, used to calculate World Space position from depth buffer.
   const Point3F &camPos = state->getCameraPosition();
   mZoneShaderConsts->setSafe(mEyePosWorldPropSC, camPos);

   // Volume position, used to calculate UV sampling.
   Box3F worldBox = getWorldBox();
   Point3F bottom_corner = worldBox.minExtents;
   Point3F top_corner = worldBox.maxExtents;
   Point3F volume_size = (top_corner - bottom_corner);
   mZoneShaderConsts->setSafe(mVolumeStartPropSC, bottom_corner);
   mZoneShaderConsts->setSafe(mVolumeSizePropSC, volume_size);

   // Render Target Parameters.
   const Point3I &targetSz = texObject->getSize();
   const RectI &targetVp = mLightInfoTarget->getViewport();
   Point4F rtParams;
   ScreenSpace::RenderTargetParameters(targetSz, targetVp, rtParams);
   mZoneShaderConsts->setSafe(mRTParamsPropSC, rtParams);

   GFX->pushActiveRenderTarget();
   GFX->setActiveRenderTarget(mRenderTarget);
   GFX->setVertexBuffer(vb);
   GFX->setStateBlockByDesc(desc);
   GFX->setShader(mZoneShader);
   GFX->setShaderConstBuffer(mZoneShaderConsts);

   // Setup Textures
   //GFX->setTexture(0, mPropagatedTexture);
   GFX->setTexture(1, deferredTexObject);

   // and SSAO mask
   /*if (AdvancedLightBinManager::smUseSSAOMask)
   {
      if (!mSSAOMaskTarget)
         mSSAOMaskTarget = NamedTexTarget::find("ssaoMask");

      if (mSSAOMaskTarget)
      {
         GFXTextureObject *SSAOMaskTexObject = mSSAOMaskTarget->getTexture();
         if (SSAOMaskTexObject)
            GFX->setTexture(2, SSAOMaskTexObject);
      }
   }*/

   // Draw the screenspace quad.
   GFX->drawPrimitive(GFXTriangleList, 0, 2);

   // Clean Up
   mRenderTarget->resolve();
   GFX->popActiveRenderTarget();
}

void ReflectionZone::_updateScreenGeometry(const Frustum &frustum,
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