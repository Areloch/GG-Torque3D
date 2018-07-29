//-----------------------------------------------------------------------------
// Copyright (c) 2012 GarageGames, LLC
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
#include "renderProbeMgr.h"
#include "console/consoleTypes.h"
#include "scene/sceneObject.h"
#include "materials/materialManager.h"
#include "scene/sceneRenderState.h"

#include "math/util/matrixSet.h"
#include "materials/processedMaterial.h"
#include "renderInstance/renderDeferredMgr.h"
#include "math/mPolyhedron.impl.h"
#include "gfx/gfxTransformSaver.h"
#include "materials/shaderData.h"

IMPLEMENT_CONOBJECT(RenderProbeMgr);

ConsoleDocClass( RenderProbeMgr, 
   "@brief A render bin which uses object callbacks for rendering.\n\n"
   "This render bin gathers object render instances and calls its delegate "
   "method to perform rendering.  It is used infrequently for specialized "
   "scene objects which perform custom rendering.\n\n"
   "@ingroup RenderBin\n" );

S32 QSORT_CALLBACK AscendingReflectProbeInfluence(const void* a, const void* b)
{
   // Debug Profiling.
   PROFILE_SCOPE(AdvancedLightBinManager_AscendingReflectProbeInfluence);

   // Fetch asset definitions.
   /*const ReflectionProbeInterface* pReflectProbeA = static_cast<ReflectionProbeInterface*>(a);
   const ReflectionProbeInterface* pReflectProbeB = static_cast<ReflectionProbeInterface*>(b);

   // Sort.
   //First, immediate check on if either is a skylight. Skylight always gets the highest priority
   //if (pReflectProbeA->mIsSkylight)
   //   return 1;
   //else if (pReflectProbeB->mIsSkylight)
   //   return -1;
   //No? then sort by score
   if (pReflectProbeA->mScore > pReflectProbeB->mScore)
	   return 1;
   else if (pReflectProbeA->mScore < pReflectProbeB->mScore)
	   return -1;*/
   return  0;
}

RenderProbeMgr::RenderProbeMgr()
: RenderBinManager(RenderPassManager::RIT_Probes, 1.0f, 1.0f)
{
}

RenderProbeMgr::RenderProbeMgr(RenderInstType riType, F32 renderOrder, F32 processAddOrder)
 : RenderBinManager(riType, renderOrder, processAddOrder)
{  
}

void RenderProbeMgr::initPersistFields()
{
   Parent::initPersistFields();
}

void RenderProbeMgr::addElement(RenderInst *inst)
{
   // If this instance is translucent handle it in RenderTranslucentMgr
   //if (inst->translucentSort)
      return;

   //AssertFatal(inst->defaultKey != 0, "RenderMeshMgr::addElement() - Got null sort key... did you forget to set it?");

   /*internalAddElement(inst);

   ReflectionProbeInterface* probeInst = static_cast<ReflectionProbeInterface*>(inst);

   if (probeInst->mIsSkylight)
   {
      addSkylightProbe(probeInst);
   }
   else
   {
      if (probeInst->mProbeShapeType == ReflectionProbeInterface::Sphere)
         addSphereReflectionProbe(probeInst);
      else
         addConvexReflectionProbe(probeInst);
   }*/
}

//remove
//Con::setIntVariable("lightMetrics::activeReflectionProbes", mReflectProbeBin.size());
//Con::setIntVariable("lightMetrics::culledReflectProbes", 0/*mNumLightsCulled*/);
//

void RenderProbeMgr::_setupPerFrameParameters(const SceneRenderState *state)
{
   PROFILE_SCOPE(RenderProbeMgr_SetupPerFrameParameters);
   const Frustum &frustum = state->getCameraFrustum();

   MatrixF invCam(frustum.getTransform());
   invCam.inverse();

   const Point3F *wsFrustumPoints = frustum.getPoints();
   const Point3F& cameraPos = frustum.getPosition();

   // Perform a camera offset.  We need to manually perform this offset on the sun (or vector) light's
   // polygon, which is at the far plane.
   Point3F cameraOffsetPos = cameraPos;

   // Now build the quad for drawing full-screen vector light
   // passes.... this is a volatile VB and updates every frame.
   FarFrustumQuadVert verts[4];
   {
      verts[0].point.set(wsFrustumPoints[Frustum::FarTopLeft] - cameraPos);
      invCam.mulP(wsFrustumPoints[Frustum::FarTopLeft], &verts[0].normal);
      verts[0].texCoord.set(-1.0, 1.0);
      verts[0].tangent.set(wsFrustumPoints[Frustum::FarTopLeft] - cameraOffsetPos);

      verts[1].point.set(wsFrustumPoints[Frustum::FarTopRight] - cameraPos);
      invCam.mulP(wsFrustumPoints[Frustum::FarTopRight], &verts[1].normal);
      verts[1].texCoord.set(1.0, 1.0);
      verts[1].tangent.set(wsFrustumPoints[Frustum::FarTopRight] - cameraOffsetPos);

      verts[2].point.set(wsFrustumPoints[Frustum::FarBottomLeft] - cameraPos);
      invCam.mulP(wsFrustumPoints[Frustum::FarBottomLeft], &verts[2].normal);
      verts[2].texCoord.set(-1.0, -1.0);
      verts[2].tangent.set(wsFrustumPoints[Frustum::FarBottomLeft] - cameraOffsetPos);

      verts[3].point.set(wsFrustumPoints[Frustum::FarBottomRight] - cameraPos);
      invCam.mulP(wsFrustumPoints[Frustum::FarBottomRight], &verts[3].normal);
      verts[3].texCoord.set(1.0, -1.0);
      verts[3].tangent.set(wsFrustumPoints[Frustum::FarBottomRight] - cameraOffsetPos);
   }
   mFarFrustumQuadVerts.set(GFX, 4);
   dMemcpy(mFarFrustumQuadVerts.lock(), verts, sizeof(verts));
   mFarFrustumQuadVerts.unlock();

   PlaneF farPlane(wsFrustumPoints[Frustum::FarBottomLeft], wsFrustumPoints[Frustum::FarTopLeft], wsFrustumPoints[Frustum::FarTopRight]);
   PlaneF vsFarPlane(verts[0].normal, verts[1].normal, verts[2].normal);

   MatrixSet &matrixSet = getRenderPass()->getMatrixSet();
   matrixSet.restoreSceneViewProjection();

   const MatrixF &worldToCameraXfm = matrixSet.getWorldToCamera();

   MatrixF inverseViewMatrix = worldToCameraXfm;
   //inverseViewMatrix.fullInverse();
   //inverseViewMatrix.transpose();

   //inverseViewMatrix = MatrixF::Identity;

   // Parameters calculated, assign them to the materials
   ProbeManager::SkylightMaterialInfo* skylightMat = PROBEMGR->getSkylightMaterial();

   if (skylightMat != nullptr && skylightMat->matInstance != nullptr)
   {
	   skylightMat->setViewParameters(frustum.getNearDist(),
         frustum.getFarDist(),
         frustum.getPosition(),
         farPlane,
         vsFarPlane, inverseViewMatrix);
   }

   ProbeManager::ReflectProbeMaterialInfo* reflProbeMat = PROBEMGR->getReflectProbeMaterial();

   if (reflProbeMat != nullptr && reflProbeMat->matInstance != nullptr)
   {
	   reflProbeMat->setViewParameters(frustum.getNearDist(),
         frustum.getFarDist(),
         frustum.getPosition(),
         farPlane,
         vsFarPlane, inverseViewMatrix);
   }
}

//-----------------------------------------------------------------------------
// render objects
//-----------------------------------------------------------------------------
void RenderProbeMgr::render( SceneRenderState *state )
{
   PROFILE_SCOPE(RenderProbeMgr_render);

   // Early out if nothing to draw.
   if (!ReflectionProbeInterface::all.size())
      return;

   if (!ProbeManager::smRenderReflectionProbes)
      return;

   NamedTexTargetRef specularLightingTarget = NamedTexTarget::find("indirectLighting");
   if (specularLightingTarget.isNull())
      return;

   GFXTextureTargetRef probeLightingTargetRef = GFX->allocRenderToTextureTarget();
   if (probeLightingTargetRef.isNull())
      return;

   GFXTransformSaver saver;

   //Handle rendering of zones
   U32 probeZoneCount = PROBEMGR->getProbeZoneCount();
   if (probeZoneCount != 0)
   {

      GFXStateBlockRef irrStateBlock;

      ShaderData *probeZoneShaderData;
      GFXShaderRef probeZoneShader = Sim::findObject("ReflectionProbeZoneShader", probeZoneShaderData) ? probeZoneShaderData->getShader() : NULL;
      if (!probeZoneShader)
      {
         Con::errorf("RenderProbeMgr::render() - could not find ReflectionProbeZoneShader");
         return;
      }
      //Prep probes
      for (U32 i = 0; i < probeZoneCount; ++i)
      {
         ProbeZone* zone = PROBEMGR->getProbeZone(i);

         zone->recalculateChildProbes();
      }

      GFXShaderConstBufferRef probeZoneConsts = probeZoneShader->allocConstBuffer();

      String probeCountName("$inProbeCount");
      String probePositionName("$inProbePos");
      String probeRadiusName("$inProbeRadius");
      String probeBoxMinName("$inProbeBoxMin");
      String probeBoxMaxName("$inProbeBoxMax");
      String probeLocalPosName("$inProbeLocalPos");
      String probeIsSphereName("$inProbeIsSphere");

      GFXShaderConstHandle* mProbeCountSC = probeZoneShader->getShaderConstHandle(probeCountName);

      GFXShaderConstHandle* mProbePositionSC = probeZoneShader->getShaderConstHandle(probePositionName);
      GFXShaderConstHandle* mProbeRadiusSC = probeZoneShader->getShaderConstHandle(probeRadiusName);
      GFXShaderConstHandle* mProbeBoxMinSC = probeZoneShader->getShaderConstHandle(probeBoxMinName);
      GFXShaderConstHandle* mProbeBoxMaxSC = probeZoneShader->getShaderConstHandle(probeBoxMaxName);
      GFXShaderConstHandle* mProbeIsSphereSC = probeZoneShader->getShaderConstHandle(probeIsSphereName);
      GFXShaderConstHandle* mProbeLocalPosSC = probeZoneShader->getShaderConstHandle(probeLocalPosName);

      U32 maxActiveProbeCount = 64;

      static AlignedArray<Point3F> probePositions(maxActiveProbeCount, sizeof(Point3F));
      static AlignedArray<F32> probeRadius(maxActiveProbeCount, sizeof(F32));
      static AlignedArray<Point3F> probeBoxMins(maxActiveProbeCount, sizeof(Point3F));
      static AlignedArray<Point3F> probeBoxMaxs(maxActiveProbeCount, sizeof(Point3F));
      static AlignedArray<Point3F> probeLocalPositions(maxActiveProbeCount, sizeof(Point3F));
      static AlignedArray<F32> probeIsSphere(maxActiveProbeCount, sizeof(F32));

      dMemset(probePositions.getBuffer(), 0, probePositions.getBufferSize());
      dMemset(probeRadius.getBuffer(), 0, probeRadius.getBufferSize());
      dMemset(probeBoxMins.getBuffer(), 0, probeBoxMins.getBufferSize());
      dMemset(probeBoxMaxs.getBuffer(), 0, probeBoxMaxs.getBufferSize());
      dMemset(probeLocalPositions.getBuffer(), 0, probeLocalPositions.getBufferSize());
      dMemset(probeIsSphere.getBuffer(), 0, probeRadius.getBufferSize());

      GFXStateBlockDesc desc;
      desc.zEnable = false;
      desc.samplersDefined = true;
      desc.samplers[0].addressModeU = GFXAddressClamp;
      desc.samplers[0].addressModeV = GFXAddressClamp;
      desc.samplers[0].addressModeW = GFXAddressClamp;
      desc.samplers[0].magFilter = GFXTextureFilterLinear;
      desc.samplers[0].minFilter = GFXTextureFilterLinear;
      desc.samplers[0].mipFilter = GFXTextureFilterLinear;

      irrStateBlock = GFX->createStateBlock(desc);

      GFX->pushActiveRenderTarget();
      GFX->setShader(probeZoneShader);
      GFX->setShaderConstBuffer(probeZoneConsts);
      GFX->setStateBlock(irrStateBlock);

      // -- Setup screenspace quad to render (postfx) --
      Frustum frustum = state->getCameraFrustum();
      GFXVertexBufferHandle<PFXVertex> vb;
      _updateScreenGeometry(frustum, &vb);

      //GFX->setVertexBuffer(NULL);
      GFX->setVertexBuffer(vb);
      //GFX->setCubeTexture(0, cubemap);

      probeLightingTargetRef->attachTexture(GFXTextureTarget::Color0, specularLightingTarget->getTexture());

      GFX->pushActiveRenderTarget();
      GFX->setActiveRenderTarget(probeLightingTargetRef);

      GFX->setViewport(specularLightingTarget->getViewport());

      //Bind our array probe data here

      ProbeZone* rootZone = PROBEMGR->getProbeZone(0);
      U32 probeCount = rootZone->getProbeCount();

      probeZoneConsts->setSafe(mProbeCountSC, (S32)probeCount);

      for (U32 i = 0; i < probeCount; i++)
      {
         ReflectionProbeInterface* probe = rootZone->getProbe(i);

         if (!probe)
            continue;

         // The light positions and spot directions are 
         // in SoA order to make optimal use of the GPU.
         const Point3F &probePos = probe->getPosition();
         probePositions[i].x = probePos.x;
         probePositions[i].y = probePos.y;
         probePositions[i].z = probePos.z;

         probeRadius[i] = probe->mRadius;

         const Point3F &minExt = probe->mBounds.minExtents;
         probeBoxMins[i].x = minExt.x;
         probeBoxMins[i].y = minExt.y;
         probeBoxMins[i].z = minExt.z;

         const Point3F &maxExt = probe->mBounds.maxExtents;
         probeBoxMaxs[i].x = maxExt.x;
         probeBoxMaxs[i].y = maxExt.y;
         probeBoxMaxs[i].z = maxExt.z;

         probeIsSphere[i] = probe->mProbeShapeType == ReflectionProbeInterface::Sphere ? 1.0 : 0.0;

         Point3F localProbePos;
         //worldToCameraXfm.mulP(probe->getPosition(), &localProbePos);
         localProbePos = probe->getPosition();

         probeLocalPositions[i].x = localProbePos.x;
         probeLocalPositions[i].y = localProbePos.y;
         probeLocalPositions[i].z = localProbePos.z;
      }

      GFX->clear(GFXClearTarget, LinearColorF::RED, 1.0f, 0);
      GFX->drawPrimitive(GFXTriangleList, 0, 3);

      probeLightingTargetRef->resolve();

      GFX->popActiveRenderTarget();

      GFX->setVertexBuffer(NULL);
      GFX->setPrimitiveBuffer(NULL);
   }
   else
   {
      //Do a quick pass to update our probes if they're dirty
      PROBEMGR->updateDirtyProbes();

      probeLightingTargetRef->attachTexture(GFXTextureTarget::Color0, specularLightingTarget->getTexture());

      GFX->pushActiveRenderTarget();
      GFX->setActiveRenderTarget(probeLightingTargetRef);

      GFX->setViewport(specularLightingTarget->getViewport());
      //GFX->setViewport(specularLightingTarget->getViewport());

      // Restore transforms
      MatrixSet &matrixSet = getRenderPass()->getMatrixSet();
      matrixSet.restoreSceneViewProjection();

      const MatrixF &worldToCameraXfm = matrixSet.getWorldToCamera();

      // Set up the SG Data
      SceneData sgData;
      sgData.init(state);

      // Initialize and set the per-frame parameters after getting
      // the vector light material as we use lazy creation.
      _setupPerFrameParameters(state);

      //Order the probes by size, biggest to smallest
      //dQsort(mElementList.address(), mElementList.size(), sizeof(const MainSortElem), AscendingReflectProbeInfluence);

      //Specular
      PROFILE_START(RenderProbeManager_ReflectProbeRender);

      ProbeManager::SkylightMaterialInfo* skylightMat = PROBEMGR->getSkylightMaterial();
      ProbeManager::ReflectProbeMaterialInfo* reflProbeMat = PROBEMGR->getReflectProbeMaterial();

      for (U32 i = 0; i < ReflectionProbeInterface::all.size(); i++)
      {
         //ProbeRenderInst *curEntry = static_cast<ProbeRenderInst*>(ReflectionProbeInterface::all[i].inst);

         ReflectionProbeInterface* curEntry = ReflectionProbeInterface::all[i];

         if (!curEntry->mIsEnabled)
            continue;

         if (curEntry->numPrims == 0)
            continue;

         if (curEntry->mIsSkylight && (!skylightMat || !skylightMat->matInstance))
            continue;

         if (!curEntry->mIsSkylight && (!reflProbeMat || !reflProbeMat->matInstance))
            break;

         //Setup
         MatrixF probeTrans = curEntry->getTransform();

         if (!curEntry->mIsSkylight)
         {
            if (curEntry->mProbeShapeType == ReflectionProbeInterface::Sphere)
               probeTrans.scale(curEntry->mRadius * 1.01f);
         }
         else
         {
            probeTrans.scale(10); //force it to be big enough to surround the camera
         }

         sgData.objTrans = &probeTrans;

         if (curEntry->mIsSkylight)
            skylightMat->setSkylightParameters(curEntry, state, worldToCameraXfm);
         else
            reflProbeMat->setProbeParameters(curEntry, state, worldToCameraXfm);

         // Set geometry
         GFX->setVertexBuffer(curEntry->vertBuffer);
         GFX->setPrimitiveBuffer(curEntry->primBuffer);

         if (curEntry->mIsSkylight)
         {
            while (skylightMat->matInstance->setupPass(state, sgData))
            {
               // Set transforms
               matrixSet.setWorld(*sgData.objTrans);
               skylightMat->matInstance->setTransforms(matrixSet, state);
               skylightMat->matInstance->setSceneInfo(state, sgData);

               GFX->drawPrimitive(GFXTriangleList, 0, curEntry->numPrims);
            }
         }
         else
         {
            while (reflProbeMat->matInstance->setupPass(state, sgData))
            {
               // Set transforms
               matrixSet.setWorld(*sgData.objTrans);
               reflProbeMat->matInstance->setTransforms(matrixSet, state);
               reflProbeMat->matInstance->setSceneInfo(state, sgData);

               GFX->drawPrimitive(GFXTriangleList, 0, curEntry->numPrims);
            }
         }
      }

      probeLightingTargetRef->resolve();
      GFX->popActiveRenderTarget();

      PROFILE_END();

      GFX->setVertexBuffer(NULL);
      GFX->setPrimitiveBuffer(NULL);

      // Fire off a signal to let others know that light-bin rendering is ending now
      //getRenderSignal().trigger(state, this);
   }

   GFX->updateStates();
}

void RenderProbeMgr::_updateScreenGeometry(const Frustum &frustum, GFXVertexBufferHandle<PFXVertex> *outVB)
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

//
//
/*ProbeRenderInst::ProbeRenderInst()
   : mTransform(true),
   mAmbient(0.0f, 0.0f, 0.0f, 1.0f),
   mPriority(1.0f),
   mScore(0.0f),
   mDebugRender(false),
   mCubemap(NULL),
   mRadius(1.0f),
   mIntensity(1.0f)
{
}

ProbeRenderInst::~ProbeRenderInst()
{
   SAFE_DELETE(mCubemap);
}

void ProbeRenderInst::set(const ProbeRenderInst *probeInfo)
{
   mTransform = probeInfo->mTransform;
   mAmbient = probeInfo->mAmbient;
   mCubemap = probeInfo->mCubemap;
   mIrradianceCubemap = probeInfo->mIrradianceCubemap;
   mBRDFTexture = probeInfo->mBRDFTexture;
   mRadius = probeInfo->mRadius;
   mIntensity = probeInfo->mIntensity;
   mProbeShapeType = probeInfo->mProbeShapeType;
   numPrims = probeInfo->numPrims;
   numVerts = probeInfo->numVerts;
   numIndicesForPoly = probeInfo->numIndicesForPoly;
   mBounds = probeInfo->mBounds;
   mScore = probeInfo->mScore;
   mIsSkylight = probeInfo->mIsSkylight;

   for (U32 i = 0; i < 9; i++)
   {
      mSHTerms[i] = probeInfo->mSHTerms[i];
   }

   for (U32 i = 0; i < 5; i++)
   {
      mSHConstants[i] = probeInfo->mSHConstants[i];
   }
}

void ProbeRenderInst::set(const ReflectionProbeInterface *probeInfo)
{
   mTransform = probeInfo->mTransform;
   mAmbient = probeInfo->mAmbient;
   mCubemap = probeInfo->mCubemap;
   mIrradianceCubemap = probeInfo->mIrradianceCubemap;
   mBRDFTexture = probeInfo->mBRDFTexture;
   mRadius = probeInfo->mRadius;
   mIntensity = probeInfo->mIntensity;
   mProbeShapeType = probeInfo->mProbeShapeType;
   numPrims = probeInfo->numPrims;
   numVerts = probeInfo->numVerts;
   numIndicesForPoly = probeInfo->numIndicesForPoly;
   mBounds = probeInfo->mBounds;
   mScore = probeInfo->mScore;
   mIsSkylight = probeInfo->mIsSkylight;

   for (U32 i = 0; i < 9; i++)
   {
      mSHTerms[i] = probeInfo->mSHTerms[i];
   }

   for (U32 i = 0; i < 5; i++)
   {
      mSHConstants[i] = probeInfo->mSHConstants[i];
   }
}

void ReflectionProbeInterface::getWorldToLightProj(MatrixF *outMatrix) const
{
   *outMatrix = getTransform();
   outMatrix->inverse();
}*/
