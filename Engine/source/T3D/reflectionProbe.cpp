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

#include "T3D/reflectionProbe.h"
#include "math/mathIO.h"
#include "scene/sceneRenderState.h"
#include "console/consoleTypes.h"
#include "core/stream/bitStream.h"
#include "materials/baseMatInstance.h"
#include "console/engineAPI.h"
#include "gfx/gfxDrawUtil.h"
#include "gfx/gfxDebugEvent.h"
#include "gfx/gfxTransformSaver.h"
#include "math/mathUtils.h"
#include "gfx/bitmap/gBitmap.h"
#include "core/stream/fileStream.h"
#include "core/fileObject.h"
#include "core/resourceManager.h"
#include "console/simPersistId.h"
#include <string>
#include "T3D/gameFunctions.h"
#include "postFx/postEffect.h"
#include "renderInstance/renderProbeMgr.h"

#include "math/util/sphereMesh.h"
#include "materials/materialManager.h"
#include "math/util/matrixSet.h"
#include "gfx/bitmap/cubemapSaver.h"

extern bool gEditingMission;
extern ColorI gCanvasClearColor;
bool ReflectionProbe::smRenderReflectionProbes = true;
bool ReflectionProbe::smRenderPreviewProbes = true;

IMPLEMENT_CO_NETOBJECT_V1(ReflectionProbe);

ConsoleDocClass(ReflectionProbe,
   "@brief An example scene object which renders a mesh.\n\n"
   "This class implements a basic SceneObject that can exist in the world at a "
   "3D position and render itself. There are several valid ways to render an "
   "object in Torque. This class implements the preferred rendering method which "
   "is to submit a MeshRenderInst along with a Material, vertex buffer, "
   "primitive buffer, and transform and allow the RenderMeshMgr handle the "
   "actual setup and rendering for you.\n\n"
   "See the C++ code for implementation details.\n\n"
   "@ingroup Examples\n");

ImplementEnumType(ReflectProbeType,
   "Type of mesh data available in a shape.\n"
   "@ingroup gameObjects")
{ ProbeRenderInst::Sphere, "Sphere", "Sphere shaped" },
{ ProbeRenderInst::Box, "Box", "Box shape" }
EndImplementEnumType;

ImplementEnumType(IndrectLightingModeEnum,
   "Type of mesh data available in a shape.\n"
   "@ingroup gameObjects")
{ ReflectionProbe::NoIndirect, "No Lighting", "This probe does not provide any local indirect lighting data" },
{ ReflectionProbe::AmbientColor, "Ambient Color", "Adds a flat color to act as the local indirect lighting" },
{ ReflectionProbe::SphericalHarmonics, "Spherical Harmonics", "Creates spherical harmonics data based off the reflection data" },
   EndImplementEnumType;

ImplementEnumType(ReflectionModeEnum,
   "Type of mesh data available in a shape.\n"
   "@ingroup gameObjects")
{ ReflectionProbe::NoReflection, "No Reflections", "This probe does not provide any local reflection data"},
{ ReflectionProbe::StaticCubemap, "Static Cubemap", "Uses a static CubemapData" },
{ ReflectionProbe::BakedCubemap, "Baked Cubemap", "Uses a cubemap baked from the probe's current position" },
{ ReflectionProbe::DynamicCubemap, "Dynamic Cubemap", "Uses a cubemap baked from the probe's current position, updated at a set rate" },
{ ReflectionProbe::SkyLight, "Skylight", "Captures a cubemap of the sky and far terrain, designed to influence the whole level" }
   EndImplementEnumType;

//-----------------------------------------------------------------------------
// Object setup and teardown
//-----------------------------------------------------------------------------
ReflectionProbe::ReflectionProbe()
{
   // Flag this object so that it will always
   // be sent across the network to clients
   mNetFlags.set(Ghostable | ScopeAlways);

   mTypeMask = LightObjectType | MarkerObjectType;

   mProbeShapeType = ProbeRenderInst::Sphere;

   mIndrectLightingModeType = AmbientColor;
   mAmbientColor = ColorF(1, 1, 1, 1);
   mSphericalHarmonics = ColorF(0,0,0,1);

   mReflectionModeType = BakedCubemap;

   mEnabled = true;
   mBake = false;
   mDirty = false;

   mProbeInfo = new ProbeRenderInst();

   mRadius = 10;
   mIntensity = 1.0f;

   mUseCubemap = false;
   mCubemap = NULL;
   mReflectionPath = "";
   mProbeUniqueID = "";

   mEditorShapeInst = NULL;
   mEditorShape = NULL;

   mRefreshRateMS = 200;
   mDynamicLastBakeMS = 0;

   mMaxDrawDistance = 75;
}

ReflectionProbe::~ReflectionProbe()
{
   if (mEditorShapeInst)
      SAFE_DELETE(mEditorShapeInst);

   if (mCubemap)
      mCubemap->deleteObject();
}

//-----------------------------------------------------------------------------
// Object Editing
//-----------------------------------------------------------------------------
void ReflectionProbe::initPersistFields()
{
   addGroup("Rendering");
      addProtectedField("enabled", TypeBool, Offset(mEnabled, ReflectionProbe),
         &_setEnabled, &defaultProtectedGetFn, "Regenerate Voxel Grid");

      addField("ProbeShape", TypeReflectProbeType, Offset(mProbeShapeType, ReflectionProbe),
         "The type of mesh data to use for collision queries.");
      addField("radius", TypeF32, Offset(mRadius, ReflectionProbe), "The name of the material used to render the mesh.");

      addField("Intensity", TypeF32, Offset(mIntensity, ReflectionProbe), "Path of file to save and load results.");
   endGroup("Rendering");

   addGroup("IndirectLighting");
      addField("IndirectLightMode", TypeIndrectLightingModeEnum, Offset(mIndrectLightingModeType, ReflectionProbe),
         "The type of mesh data to use for collision queries.");

      addField("IndirectLight", TypeColorF, Offset(mAmbientColor, ReflectionProbe), "Path of file to save and load results.");
   endGroup("IndirectLighting");

   addGroup("Reflection");
      addField("ReflectionMode", TypeReflectionModeEnum, Offset(mReflectionModeType, ReflectionProbe),
         "The type of mesh data to use for collision queries.");

      addField("reflectionPath", TypeImageFilename, Offset(mReflectionPath, ReflectionProbe),
         "The type of mesh data to use for collision queries.");

      addField("StaticCubemap", TypeCubemapName, Offset(mCubemapName, ReflectionProbe), "Cubemap used instead of reflection texture if fullReflect is off.");

      addProtectedField("Bake", TypeBool, Offset(mBake, ReflectionProbe),
         &_doBake, &defaultProtectedGetFn, "Regenerate Voxel Grid", AbstractClassRep::FieldFlags::FIELD_ComponentInspectors);
   endGroup("Reflection");

   Con::addVariable("$Light::renderReflectionProbes", TypeBool, &ReflectionProbe::smRenderReflectionProbes,
      "Toggles rendering of light frustums when the light is selected in the editor.\n\n"
      "@note Only works for shadow mapped lights.\n\n"
      "@ingroup Lighting");

   Con::addVariable("$Light::renderPreviewProbes", TypeBool, &ReflectionProbe::smRenderPreviewProbes,
      "Toggles rendering of light frustums when the light is selected in the editor.\n\n"
      "@note Only works for shadow mapped lights.\n\n"
      "@ingroup Lighting");

   // SceneObject already handles exposing the transform
   Parent::initPersistFields();
}

void ReflectionProbe::inspectPostApply()
{
   Parent::inspectPostApply();

   mDirty = true;

   // Flag the network mask to send the updates
   // to the client object
   setMaskBits(-1);
}

bool ReflectionProbe::_setEnabled(void *object, const char *index, const char *data)
{
   ReflectionProbe* probe = reinterpret_cast< ReflectionProbe* >(object);

   probe->mEnabled = dAtob(data);
   probe->setMaskBits(-1);

   return true;
}

bool ReflectionProbe::_doBake(void *object, const char *index, const char *data)
{
   ReflectionProbe* probe = reinterpret_cast< ReflectionProbe* >(object);

   if (probe->mDirty)
      probe->bake(probe->mReflectionPath, 256);

   return false;
}

bool ReflectionProbe::onAdd()
{
   if (!Parent::onAdd())
      return false;

   mObjBox.minExtents.set(-1, -1, -1);
   mObjBox.maxExtents.set(1, 1, 1);
   mObjScale.set(mRadius/2, mRadius/2, mRadius/2);

   // Skip our transform... it just dirties mask bits.
   Parent::setTransform(mObjToWorld);

   resetWorldBox();

   // Add this object to the scene
   addToScene();

   if (isServerObject())
   {
      if (!mPersistentId)
         mPersistentId = getOrCreatePersistentId();

      mProbeUniqueID = std::to_string(mPersistentId->getUUID().getHash()).c_str();
   }

   // Refresh this object's material (if any)
   if (isClientObject())
      updateMaterial();
  
   setMaskBits(-1);

   return true;
}

void ReflectionProbe::onRemove()
{
   // Remove this object from the scene
   removeFromScene();

   Parent::onRemove();
}

void ReflectionProbe::setTransform(const MatrixF & mat)
{
   // Let SceneObject handle all of the matrix manipulation
   Parent::setTransform(mat);

   mDirty = true;

   // Dirty our network mask so that the new transform gets
   // transmitted to the client object
   setMaskBits(TransformMask);
}

U32 ReflectionProbe::packUpdate(NetConnection *conn, U32 mask, BitStream *stream)
{
   // Allow the Parent to get a crack at writing its info
   U32 retMask = Parent::packUpdate(conn, mask, stream);

   // Write our transform information
   if (stream->writeFlag(mask & TransformMask))
   {
      mathWrite(*stream, getTransform());
      mathWrite(*stream, getScale());
   }

   stream->write((U32)mProbeShapeType);

   stream->write((U32)mIndrectLightingModeType);
   stream->write(mAmbientColor);

   stream->write((U32)mReflectionModeType);

   stream->writeFloat(mIntensity, 7);
   stream->write(mRadius);

   stream->write(mReflectionPath);
   stream->write(mProbeUniqueID);

   stream->writeFlag(mEnabled);

   stream->writeFlag(mUseCubemap);
   stream->write(mCubemapName);

   return retMask;
}

void ReflectionProbe::unpackUpdate(NetConnection *conn, BitStream *stream)
{
   // Let the Parent read any info it sent
   Parent::unpackUpdate(conn, stream);

   if (stream->readFlag())  // TransformMask
   {
      mathRead(*stream, &mObjToWorld);
      mathRead(*stream, &mObjScale);

      setTransform(mObjToWorld);
   }

   U32 shapeType = ProbeRenderInst::Sphere;
   stream->read(&shapeType);
   mProbeShapeType = (ProbeRenderInst::ProbeShapeType)shapeType;

   U32 indirectModeType = AmbientColor;
   stream->read(&indirectModeType);
   if ((IndrectLightingModeType)indirectModeType != NoIndirect)
   {
      mIndrectLightingModeType = (IndrectLightingModeType)indirectModeType;
   }

   stream->read(&mAmbientColor);

   U32 reflectModeType = BakedCubemap;
   stream->read(&reflectModeType);
   mReflectionModeType = (ReflectionModeType)reflectModeType;

   mIntensity = stream->readFloat(7);
   stream->read(&mRadius);

   stream->read(&mReflectionPath);
   stream->read(&mProbeUniqueID);

   mEnabled = stream->readFlag();

   mUseCubemap = stream->readFlag();
   stream->read(&mCubemapName);

   if (mCubemapName.isNotEmpty() && mReflectionModeType == ReflectionModeType::StaticCubemap)
      Sim::findObject(mCubemapName, mCubemap);

   createGeometry();
   updateMaterial();
}

void ReflectionProbe::createGeometry()
{
   // Clean up our previous shape
   if (mEditorShapeInst)
      SAFE_DELETE(mEditorShapeInst);
   
   mEditorShape = NULL;
   
   String shapeFile = "tools/resources/ReflectProbeSphere.dae";
   
   // Attempt to get the resource from the ResourceManager
   mEditorShape = ResourceManager::get().load(shapeFile);
   if (mEditorShape)
   {
      mEditorShapeInst = new TSShapeInstance(mEditorShape, isClientObject());
   }
}

//-----------------------------------------------------------------------------
// Object Rendering
//-----------------------------------------------------------------------------

void ReflectionProbe::updateMaterial()
{
   mProbeInfo->setPosition(getPosition());

   if (mReflectionModeType != DynamicCubemap)
   {
      if (!mCubemap)
      {
         mCubemap = new CubemapData();
         mCubemap->registerObject();
      }

      if (!mProbeUniqueID.isEmpty())
      {
         bool validCubemap = true;

         for (U32 i = 0; i < 6; ++i)
         {
            char faceFile[256];
            dSprintf(faceFile, sizeof(faceFile), "%s%s_%i.png", mReflectionPath.c_str(),
               mProbeUniqueID.c_str(), i);

            if (Platform::isFile(faceFile))
               mCubemap->setCubeFaceFile(i, FileName(faceFile));
            else
               validCubemap = false;
         }

         if (validCubemap)
         {
            mCubemap->createMap();
            mCubemap->updateFaces();
         }
      }

      mProbeInfo->mCubemap = &mCubemap->mCubemap;
   }
   else if (mReflectionModeType == DynamicCubemap && !mDynamicCubemap.isNull())
   {
      mProbeInfo->mCubemap = &mDynamicCubemap;
   }

   mProbeInfo->mIntensity = mIntensity;
   mProbeInfo->mRadius = mRadius;

   if (mIndrectLightingModeType == AmbientColor)
   {
      mProbeInfo->mAmbient = mAmbientColor;
   }
   else
   {
      mProbeInfo->mAmbient = ColorF(0, 0, 0, 0);
   }

   mProbeInfo->mProbeShapeType = mProbeShapeType;

   mProbeInfo->mBounds = mWorldBox;

   calculateSHTerms();

   bool tump = true;
}

void ReflectionProbe::prepRenderImage(SceneRenderState *state)
{
   if (!mEnabled || !ReflectionProbe::smRenderReflectionProbes)
      return;

   Point3F distVec = getPosition() - state->getCameraPosition();
   F32 dist = distVec.len();

   //Culling distance. Can be adjusted for performance options considerations via the scalar
   if (dist > mMaxDrawDistance * Con::getFloatVariable("$pref::GI::ProbeDrawDistScale", 1.0))
      return;

   if (mReflectionModeType == DynamicCubemap && mRefreshRateMS < (Platform::getRealMilliseconds() - mDynamicLastBakeMS))
   {
      bake("", 32);
      mDynamicLastBakeMS = Platform::getRealMilliseconds();
   }

   //Submit our probe to actually do the probe action
   // Get a handy pointer to our RenderPassmanager
   RenderPassManager *renderPass = state->getRenderPass();

   // Allocate an MeshRenderInst so that we can submit it to the RenderPassManager
   ProbeRenderInst *probeInst = renderPass->allocInst<ProbeRenderInst>();

   probeInst->set(mProbeInfo);

   probeInst->type = RenderPassManager::RIT_Probes;

   // Submit our RenderInst to the RenderPassManager
   state->getRenderPass()->addInst(probeInst);

   if (ReflectionProbe::smRenderPreviewProbes && gEditingMission && mEditorShapeInst)
   {
      GFXTransformSaver saver;

      // Calculate the distance of this object from the camera
      Point3F cameraOffset;
      getRenderTransform().getColumn(3, &cameraOffset);
      cameraOffset -= state->getDiffuseCameraPosition();
      F32 dist = cameraOffset.len();
      if (dist < 0.01f)
         dist = 0.01f;

      // Set up the LOD for the shape
      F32 invScale = (1.0f / getMax(getMax(mObjScale.x, mObjScale.y), mObjScale.z));

      mEditorShapeInst->setDetailFromDistance(state, dist * invScale);

      // Make sure we have a valid level of detail
      if (mEditorShapeInst->getCurrentDetail() < 0)
         return;

      BaseMatInstance* probePrevMat = mEditorShapeInst->getMaterialList()->getMaterialInst(0);

      setPreviewMatParameters(state, probePrevMat);

      // GFXTransformSaver is a handy helper class that restores
      // the current GFX matrices to their original values when
      // it goes out of scope at the end of the function

      // Set up our TS render state      
      TSRenderState rdata;
      rdata.setSceneState(state);
      rdata.setFadeOverride(1.0f);

      if(mReflectionModeType != DynamicCubemap)
         rdata.setCubemap(mCubemap->mCubemap);
      else
         rdata.setCubemap(mDynamicCubemap);

      // We might have some forward lit materials
      // so pass down a query to gather lights.
      LightQuery query;
      query.init(getWorldSphere());
      rdata.setLightQuery(&query);

      // Set the world matrix to the objects render transform
      MatrixF mat = getRenderTransform();
      mat.scale(Point3F(1, 1, 1));
      GFX->setWorldMatrix(mat);

      // Animate the the shape
      mEditorShapeInst->animate();

      // Allow the shape to submit the RenderInst(s) for itself
      mEditorShapeInst->render(rdata);

      saver.restore();
   }

   // If the light is selected or light visualization
   // is enabled then register the callback.
   const bool isSelectedInEditor = (gEditingMission && isSelected());
   if (isSelectedInEditor)
   {
      ObjectRenderInst *ri = state->getRenderPass()->allocInst<ObjectRenderInst>();
      ri->renderDelegate.bind(this, &ReflectionProbe::_onRenderViz);
      ri->type = RenderPassManager::RIT_Editor;
      state->getRenderPass()->addInst(ri);
   }
}

void ReflectionProbe::_onRenderViz(ObjectRenderInst *ri,
   SceneRenderState *state,
   BaseMatInstance *overrideMat)
{
   if (!ReflectionProbe::smRenderReflectionProbes)
      return;

   GFXDrawUtil *draw = GFX->getDrawUtil();

   GFXStateBlockDesc desc;
   desc.setZReadWrite(true, false);
   desc.setCullMode(GFXCullNone);
   desc.setBlend(true);

   // Base the sphere color on the light color.
   ColorI color = ColorI::WHITE;
   color.alpha = 50;

   if (mProbeShapeType == ProbeRenderInst::Sphere)
   {
      draw->drawSphere(desc, mRadius, getPosition(), color);
   }
   else
   {
      Box3F cube(mRadius);
      draw->drawCube(desc, cube, color);
   }
}

void ReflectionProbe::setPreviewMatParameters(SceneRenderState* renderState, BaseMatInstance* mat)
{
   //Set up the params
   MaterialParameters *matParams = mat->getMaterialParameters();

   //Get the deferred render target
   NamedTexTarget* deferredTexTarget = NamedTexTarget::find("deferred");

   GFXTextureObject *deferredTexObject = deferredTexTarget->getTexture();
   if (!deferredTexObject) 
      return;

   GFX->setTexture(0, deferredTexObject);

   //Set the cubemap
   GFX->setCubeTexture(1, mCubemap->mCubemap);

   //Set the invViewMat
   MatrixSet &matrixSet = renderState->getRenderPass()->getMatrixSet();
   const MatrixF &worldToCameraXfm = matrixSet.getWorldToCamera();

   MaterialParameterHandle *invViewMat = mat->getMaterialParameterHandle("$invViewMat");

   matParams->setSafe(invViewMat, worldToCameraXfm);
}

ColorF ReflectionProbe::decodeSH(Point3F normal)
{
   float x = normal.x;
   float y = normal.y;
   float z = normal.z;

   ColorF l00 = mProbeInfo->mSHTerms[0];

   ColorF l10 = mProbeInfo->mSHTerms[1];
   ColorF l11 = mProbeInfo->mSHTerms[2];
   ColorF l12 = mProbeInfo->mSHTerms[3];

   ColorF l20 = mProbeInfo->mSHTerms[4];
   ColorF l21 = mProbeInfo->mSHTerms[5];
   ColorF l22 = mProbeInfo->mSHTerms[6];
   ColorF l23 = mProbeInfo->mSHTerms[7];
   ColorF l24 = mProbeInfo->mSHTerms[8];

   ColorF result = (
         l00 * mProbeInfo->mSHConstants[0] +

         l12 * mProbeInfo->mSHConstants[1] * x +
         l10 * mProbeInfo->mSHConstants[1] * y +
         l11 * mProbeInfo->mSHConstants[1] * z +

         l20 * mProbeInfo->mSHConstants[2] * x*y +
         l21 * mProbeInfo->mSHConstants[2] * y*z +
         l22 * mProbeInfo->mSHConstants[3] * (3.0*z*z - 1.0) +
         l23 * mProbeInfo->mSHConstants[2] * x*z +
         l24 * mProbeInfo->mSHConstants[4] * (x*x - y*y)
      );

   return ColorF(mMax(result.red, 0), mMax(result.green, 0), mMax(result.blue, 0));
}

MatrixF getSideMatrix(U32 side)
{
   // Standard view that will be overridden below.
   VectorF vLookatPt(0.0f, 0.0f, 0.0f), vUpVec(0.0f, 0.0f, 0.0f), vRight(0.0f, 0.0f, 0.0f);

   switch (side)
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

   MatrixF rotMat(true);
   rotMat.setColumn(0, cross);
   rotMat.setColumn(1, vLookatPt);
   rotMat.setColumn(2, vUpVec);
   //rotMat.inverse();

   return rotMat;
}

F32 ReflectionProbe::harmonics(U32 termId, Point3F normal)
{
   F32 x = normal.x;
   F32 y = normal.y;
   F32 z = normal.z;

   switch(termId)
   {
   case 0:
      return 1.0;
   case 1:
      return y;
   case 2:
      return z;
   case 3:
      return x;
   case 4:
      return x*y;
   case 5:
      return y*z;
   case 6:
      return 3.0*z*z - 1.0;
   case 7:
      return x*z;
   default:
      return x*x - y*y;
   }
}

ColorF ReflectionProbe::sampleSide(U32 termindex, U32 sideIndex)
{
   MatrixF sideRot = getSideMatrix(sideIndex);

   ColorF result = ColorF::ZERO;
   F32 divider = 0;

   for (int y = 0; y<mCubemapResolution; y++)
   {
      for (int x = 0; x<mCubemapResolution; x++)
      {
         Point2F sidecoord = ((Point2F(x, y) + Point2F(0.5, 0.5)) / Point2F(mCubemapResolution, mCubemapResolution))*2.0 - Point2F(1.0, 1.0);
         Point3F normal = Point3F(sidecoord.x, sidecoord.y, -1.0);
         normal.normalize();

         F32 minBrightness = Con::getFloatVariable("$pref::GI::Cubemap_Sample_MinBrightness", 0.001f);

         ColorF texel = mCubeFaceBitmaps[sideIndex]->sampleTexel(y, x);
         texel = ColorF(mMax(texel.red, minBrightness), mMax(texel.green, minBrightness), mMax(texel.blue, minBrightness)) * Con::getFloatVariable("$pref::GI::Cubemap_Gain", 1.5);

         Point3F dir;
         sideRot.mulP(normal, &dir);

         result += texel * harmonics(termindex,dir) * -normal.z;
         divider += -normal.z;
      }
   }

   result /= divider;

   return result;
}

//
//SH Calculations
// From http://sunandblackcat.com/tipFullView.php?l=eng&topicid=32&topic=Spherical-Harmonics-From-Cube-Texture
// With shader decode logic from https://github.com/nicknikolov/cubemap-sh
void ReflectionProbe::calculateSHTerms()
{
   if (!mCubemap || !mCubemap->mCubemap)
      return;

   const VectorF cubemapFaceNormals[6] =
   {
      // D3DCUBEMAP_FACE_POSITIVE_X:
      VectorF(1.0f, 0.0f, 0.0f),
      // D3DCUBEMAP_FACE_NEGATIVE_X:
      VectorF(-1.0f, 0.0f, 0.0f),
      // D3DCUBEMAP_FACE_POSITIVE_Y:
      VectorF(0.0f, 1.0f, 0.0f),
      // D3DCUBEMAP_FACE_NEGATIVE_Y:
      VectorF(0.0f, -1.0f, 0.0f),
      // D3DCUBEMAP_FACE_POSITIVE_Z:
      VectorF(0.0f, 0.0f, 1.0f),
      // D3DCUBEMAP_FACE_NEGATIVE_Z:
      VectorF(0.0f, 0.0f, -1.0f),
   };

   mCubemapResolution = mCubemap->mCubemap->getSize();

   for (U32 i = 0; i < 6; i++)
   {
      mCubeFaceBitmaps[i] = new GBitmap(mCubemapResolution, mCubemapResolution, false, GFXFormatR8G8B8);
   }

   //If we fail to parse the cubemap for whatever reason, we really can't continue
   if (!CubemapSaver::getBitmaps(mCubemap->mCubemap, GFXFormatR8G8B8, mCubeFaceBitmaps))
      return; 

   //Set up our constants
   F32 L0 = Con::getFloatVariable("$pref::GI::SH_Term_L0", 1.0f);
   F32 L1 = Con::getFloatVariable("$pref::GI::SH_Term_L1", 1.8f);
   F32 L2 = Con::getFloatVariable("$pref::GI::SH_Term_L2", 0.83f);
   F32 L2m2_L2m1_L21 = Con::getFloatVariable("$pref::GI::SH_Term_L2m2", 2.9f);
   F32 L20 = Con::getFloatVariable("$pref::GI::SH_Term_L20", 0.58f);
   F32 L22 = Con::getFloatVariable("$pref::GI::SH_Term_L22", 1.1f);

   mProbeInfo->mSHConstants[0] = L0;
   mProbeInfo->mSHConstants[1] = L1;
   mProbeInfo->mSHConstants[2] = L2 * L2m2_L2m1_L21;
   mProbeInfo->mSHConstants[3] = L2 * L20;
   mProbeInfo->mSHConstants[4] = L2 * L22;

   for (U32 i = 0; i < 9; i++)
   {
      //Clear it, just to be sure
      mProbeInfo->mSHTerms[i] = ColorF(0.f, 0.f, 0.f);

      //Now, encode for each side
      mProbeInfo->mSHTerms[i] = sampleSide(i, 0); //POS_X
      mProbeInfo->mSHTerms[i] += sampleSide(i, 1); //NEG_X
      mProbeInfo->mSHTerms[i] += sampleSide(i, 2); //POS_Y
      mProbeInfo->mSHTerms[i] += sampleSide(i, 3); //NEG_Y
      mProbeInfo->mSHTerms[i] += sampleSide(i, 4); //POS_Z
      mProbeInfo->mSHTerms[i] += sampleSide(i, 5); //NEG_Z

      //Average
      mProbeInfo->mSHTerms[i] /= 6;
   }

   for (U32 i = 0; i < 6; i++)
      SAFE_DELETE(mCubeFaceBitmaps[i]);

   bool mExportSHTerms = false;
   if (mExportSHTerms)
   {
      for (U32 f = 0; f < 6; f++)
      {
         char fileName[256];
         dSprintf(fileName, 256, "%s%s_DecodedFaces_%d.png", mReflectionPath.c_str(),
            mProbeUniqueID.c_str(), f);

         ColorF color = decodeSH(cubemapFaceNormals[f]);

         FileStream stream;
         if (stream.open(fileName, Torque::FS::File::Write))
         {
            GBitmap bitmap(mCubemapResolution, mCubemapResolution, false, GFXFormatR8G8B8);

            bitmap.fill(color);

            bitmap.writeBitmap("png", stream);
         }
      }

      for (U32 f = 0; f < 9; f++)
      {
         char fileName[256];
         dSprintf(fileName, 256, "%s%s_SHTerms_%d.png", mReflectionPath.c_str(),
            mProbeUniqueID.c_str(), f);

         ColorF color = mProbeInfo->mSHTerms[f];

         FileStream stream;
         if (stream.open(fileName, Torque::FS::File::Write))
         {
            GBitmap bitmap(mCubemapResolution, mCubemapResolution, false, GFXFormatR8G8B8);

            bitmap.fill(color);

            bitmap.writeBitmap("png", stream);
         }
      }
   }
}

F32 ReflectionProbe::texelSolidAngle(F32 aU, F32 aV, U32 width, U32 height)
{
   // transform from [0..res - 1] to [- (1 - 1 / res) .. (1 - 1 / res)]
   // ( 0.5 is for texel center addressing)
   const F32 U = (2.0 * (aU + 0.5) / width) - 1.0;
   const F32 V = (2.0 * (aV + 0.5) / height) - 1.0;

   // shift from a demi texel, mean 1.0 / size  with U and V in [-1..1]
   const F32 invResolutionW = 1.0 / width;
   const F32 invResolutionH = 1.0 / height;

   // U and V are the -1..1 texture coordinate on the current face.
   // get projected area for this texel
   const F32 x0 = U - invResolutionW;
   const F32 y0 = V - invResolutionH;
   const F32 x1 = U + invResolutionW;
   const F32 y1 = V + invResolutionH;
   const F32 angle = areaElement(x0, y0) - areaElement(x0, y1) - areaElement(x1, y0) + areaElement(x1, y1);

   return angle;
}

F32 ReflectionProbe::areaElement(F32 x, F32 y) 
{
   return mAtan2(x * y, (F32)mSqrt(x * x + y * y + 1.0));
}

DefineEngineMethod(ReflectionProbe, postApply, void, (), ,
   "A utility method for forcing a network update.\n")
{
   object->inspectPostApply();
}

void ReflectionProbe::bake(String outputPath, S32 resolution)
{
   GFXDEBUGEVENT_SCOPE(ReflectionProbe_Bake, ColorI::WHITE);

   PostEffect *preCapture = dynamic_cast<PostEffect*>(Sim::findObject("AL_PreCapture"));
   PostEffect *deferredShading = dynamic_cast<PostEffect*>(Sim::findObject("AL_DeferredShading"));
   if (preCapture)
      preCapture->enable();
   if (deferredShading)
      deferredShading->disable();

   //if (mReflectionModeType == StaticCubemap || mReflectionModeType == BakedCubemap || mReflectionModeType == SkyLight)
   {
      if (!mCubemap)
      {
         mCubemap = new CubemapData();
         mCubemap->registerObject();
      }
   }

   if (mReflectionModeType == DynamicCubemap && mDynamicCubemap.isNull())
   {
      //mCubemap->createMap();
      mDynamicCubemap = GFX->createCubemap();
      mDynamicCubemap->initDynamic(resolution, GFXFormatR8G8B8);
   }
   else if (mReflectionModeType != DynamicCubemap)
   {
      if (mReflectionPath.isEmpty() || !mPersistentId)
      {
         if (!mPersistentId)
            mPersistentId = getOrCreatePersistentId();

         mReflectionPath = outputPath.c_str();

         mProbeUniqueID = std::to_string(mPersistentId->getUUID().getHash()).c_str();
      }
   }

   bool validCubemap = true;

   // Save the current transforms so we can restore
   // it for child control rendering below.
   GFXTransformSaver saver;

   //bool saveEditingMission = gEditingMission;
   //gEditingMission = false;

   //Set this to true to use the prior method where it goes through the SPT_Reflect path for the bake
   bool probeRenderState = ReflectionProbe::smRenderReflectionProbes;
   ReflectionProbe::smRenderReflectionProbes = false;
   for (U32 i = 0; i < 6; ++i)
   {
      GFXTexHandle blendTex;
      blendTex.set(resolution, resolution, GFXFormatR8G8B8A8, &GFXRenderTargetProfile, "");

      GFXTextureTargetRef mBaseTarget = GFX->allocRenderToTextureTarget();

      GFX->clearTextureStateImmediate(0);
      if(mReflectionModeType == DynamicCubemap)
         mBaseTarget->attachTexture(GFXTextureTarget::Color0, mDynamicCubemap, i);
      else
         mBaseTarget->attachTexture(GFXTextureTarget::Color0, blendTex);

      // Standard view that will be overridden below.
      VectorF vLookatPt(0.0f, 0.0f, 0.0f), vUpVec(0.0f, 0.0f, 0.0f), vRight(0.0f, 0.0f, 0.0f);

      switch (i)
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

      // set projection to 90 degrees vertical and horizontal
      F32 left, right, top, bottom;
      F32 nearPlane = 0.01f;
      F32 farDist = 1000.f;

      if (mReflectionModeType == SkyLight)
      {
         nearPlane = 1000.f;
         farDist = 10000.f;
      }

      MathUtils::makeFrustum(&left, &right, &top, &bottom, M_HALFPI_F, 1.0f, nearPlane);
      Frustum frustum(false, left, right, top, bottom, nearPlane, farDist);

      renderFrame(&mBaseTarget, matView, frustum, StaticObjectType | StaticShapeObjectType & EDITOR_RENDER_TYPEMASK, gCanvasClearColor);

      mBaseTarget->resolve();

      if (mReflectionModeType != DynamicCubemap)
      {
         char fileName[256];
         dSprintf(fileName, 256, "%s%s_%i.png", mReflectionPath.c_str(),
            mProbeUniqueID.c_str(), i);

         FileStream stream;
         if (!stream.open(fileName, Torque::FS::File::Write))
         {
            Con::errorf("ReflectionProbe::bake(): Couldn't open cubemap face file fo writing " + String(fileName));
            if (preCapture)
               preCapture->disable();
            if (deferredShading)
               deferredShading->enable();
            return;
         }

         GBitmap bitmap(blendTex->getWidth(), blendTex->getHeight(), false, GFXFormatR8G8B8);
         blendTex->copyToBmp(&bitmap);
         bitmap.writeBitmap("png", stream);

         if (Platform::isFile(fileName) && mCubemap)
            mCubemap->setCubeFaceFile(i, FileName(fileName));
         else
            validCubemap = false;

         bitmap.deleteImage();
      }
   }

   if (mReflectionModeType != DynamicCubemap && validCubemap)
   {
      if (mCubemap->mCubemap)
         mCubemap->updateFaces();
      else
         mCubemap->createMap();

      mDirty = false;
   }

   calculateSHTerms();

   ReflectionProbe::smRenderReflectionProbes = probeRenderState;
   setMaskBits(-1);

   if (preCapture)
      preCapture->disable();
   if (deferredShading)
      deferredShading->enable();
}

DefineEngineMethod(ReflectionProbe, Bake, void, (String outputPath, S32 resolution), ("", 256),
   "@brief returns true if control object is inside the fog\n\n.")
{
   object->bake(outputPath, resolution);
   //object->renderFrame(false);
}