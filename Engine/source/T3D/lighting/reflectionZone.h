#pragma once

#ifndef REFLECTION_ZONE_H
#define REFLECTION_ZONE_H

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

/// A volume in space that blocks visibility.
class ReflectionZone : public ScenePolyhedralSpace
{
public:
   typedef ScenePolyhedralSpace Parent;

private:
   // We sample from prepass and render to light buffer.
   NamedTexTarget*         mDeferredTarget;
   NamedTexTarget*         mLightInfoTarget;
   NamedTexTarget*         mIndirectLightTarget;
   NamedTexTarget*         mMatInfoTarget;
   NamedTexTarget*         mSSAOMaskTarget;
   GFXTextureTargetRef     mRenderTarget;

   // Stateblock for shaders
   GFXStateBlockRef        mStateBlock;

   GFXShaderRef            mZoneShader;
   GFXShaderConstBufferRef mZoneShaderConsts;
   GFXShaderConstHandle    *mEyePosWorldPropSC;
   GFXShaderConstHandle    *mRTParamsPropSC;
   GFXShaderConstHandle    *mVolumeStartPropSC;
   GFXShaderConstHandle    *mVolumeSizePropSC;

protected:
   void     _initVolumeTextures(Point3I volumeSize);
   void     _initShaders();

   // Final Volume Rendering
   void _handleBinEvent(RenderBinManager *bin, const SceneRenderState* sceneState, bool isBinStart);

   void _renderProbes(const SceneRenderState* state);

   void _updateScreenGeometry(const Frustum &frustum, GFXVertexBufferHandle<PFXVertex> *outVB);

   // World Editor Visualization.
   typedef SilhouetteExtractorPerspective< PolyhedronType > SilhouetteExtractorType;
   bool mTransformDirty;
   Vector< Point3F > mWSPoints;
   SilhouetteExtractorType mSilhouetteExtractor;
   mutable Vector< SceneObject* > mVolumeQueryList;

   virtual void _renderObject(ObjectRenderInst* ri, SceneRenderState* state, BaseMatInstance* overrideMat);

public:

   ReflectionZone();
   ~ReflectionZone();

   // SimObject.
   DECLARE_CONOBJECT(ReflectionZone);
   DECLARE_DESCRIPTION("Reflection Zone");
   DECLARE_CATEGORY("3D Scene");

   virtual bool onAdd();
   virtual void onRemove();
   void inspectPostApply();

   // Network
   U32  packUpdate(NetConnection *, U32 mask, BitStream *stream);
   void unpackUpdate(NetConnection *, BitStream *stream);

   // SceneObject.
   virtual void buildSilhouette(const SceneCameraState& cameraState, Vector< Point3F >& outPoints);
   virtual void setTransform(const MatrixF& mat);

   // Static Functions.
   static void consoleInit();
   static void initPersistFields();
};

#endif // !_ReflectionZone_H_