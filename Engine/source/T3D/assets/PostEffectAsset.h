#pragma once
//-----------------------------------------------------------------------------
// Copyright (c) 2013 GarageGames, LLC
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
#ifndef POSTEFFECT_ASSET_H
#define POSTEFFECT_ASSET_H

#ifndef _ASSET_BASE_H_
#include "assets/assetBase.h"
#endif

#ifndef _ASSET_DEFINITION_H_
#include "assets/assetDefinition.h"
#endif

#ifndef _STRINGUNIT_H_
#include "string/stringUnit.h"
#endif

#ifndef _ASSET_FIELD_TYPES_H_
#include "assets/assetFieldTypes.h"
#endif

#include "postFx/postEffect.h"

//-----------------------------------------------------------------------------
class PostEffectAsset : public AssetBase
{
   typedef AssetBase Parent;

   AssetManager*           mpOwningAssetManager;
   bool                    mAssetInitialized;
   AssetDefinition*        mpAssetDefinition;
   U32                     mAcquireReferenceCount;

   StringTableEntry        mLevelFile;
   StringTableEntry        mLevelDescription;
   StringTableEntry        mPreviewImage;

   //State Block settings

   //Shader settings

   //PostEffect settings
   FileName mTexFilename[PostEffect::NumTextures];
   bool mTexSRGB[PostEffect::NumTextures];

   GFXTexHandle mTextures[PostEffect::NumTextures];

   NamedTexTarget mNamedTarget;
   NamedTexTarget mNamedTargetDepthStencil;

   GFXTextureObject *mActiveTextures[PostEffect::NumTextures];

   NamedTexTarget *mActiveNamedTarget[PostEffect::NumTextures];

   RectI mActiveTextureViewport[PostEffect::NumTextures];

   GFXStateBlockData *mStateBlockData;

   GFXStateBlockRef mStateBlock;

   String mShaderName;

   GFXShaderRef mShader;

   Vector<GFXShaderMacro> mShaderMacros;

   GFXShaderConstBufferRef mShaderConsts;

   GFXShaderConstHandle *mRTSizeSC;
   GFXShaderConstHandle *mOneOverRTSizeSC;

   GFXShaderConstHandle *mTexSizeSC[PostEffect::NumTextures];
   GFXShaderConstHandle *mRenderTargetParamsSC[PostEffect::NumTextures];

   GFXShaderConstHandle *mViewportOffsetSC;

   GFXShaderConstHandle *mTargetViewportSC;

   GFXShaderConstHandle *mFogDataSC;
   GFXShaderConstHandle *mFogColorSC;
   GFXShaderConstHandle *mEyePosSC;
   GFXShaderConstHandle *mMatWorldToScreenSC;
   GFXShaderConstHandle *mMatScreenToWorldSC;
   GFXShaderConstHandle *mMatPrevScreenToWorldSC;
   GFXShaderConstHandle *mNearFarSC;
   GFXShaderConstHandle *mInvNearFarSC;
   GFXShaderConstHandle *mWorldToScreenScaleSC;
   GFXShaderConstHandle *mProjectionOffsetSC;
   GFXShaderConstHandle *mWaterColorSC;
   GFXShaderConstHandle *mWaterFogDataSC;
   GFXShaderConstHandle *mAmbientColorSC;
   GFXShaderConstHandle *mWaterFogPlaneSC;
   GFXShaderConstHandle *mWaterDepthGradMaxSC;
   GFXShaderConstHandle *mScreenSunPosSC;
   GFXShaderConstHandle *mLightDirectionSC;
   GFXShaderConstHandle *mCameraForwardSC;
   GFXShaderConstHandle *mAccumTimeSC;
   GFXShaderConstHandle *mDeltaTimeSC;
   GFXShaderConstHandle *mInvCameraMatSC;

   bool mAllowReflectPass;

   /// If true update the shader.
   bool mUpdateShader;

   GFXTextureTargetRef mTarget;

   String mTargetName;
   GFXTexHandle mTargetTex;

   String mTargetDepthStencilName;
   GFXTexHandle mTargetDepthStencil;

   /// If mTargetSize is zero then this scale is
   /// used to make a relative texture size to the
   /// active render target.
   Point2F mTargetScale;

   /// If non-zero this is used as the absolute
   /// texture target size.
   /// @see mTargetScale
   Point2I mTargetSize;

   GFXFormat mTargetFormat;

   /// The color to prefill the named target when
   /// first created by the effect.
   ColorF mTargetClearColor;

   PFXRenderTime mRenderTime;
   PFXTargetClear mTargetClear;
   PFXTargetViewport mTargetViewport;

   String mRenderBin;

   F32 mRenderPriority;

   /// This is true if the effect has been succesfully
   /// initialized and all requirements are met for use.
   bool mIsValid;

   /// True if the effect has been enabled by the manager.
   bool mEnabled;

   /// Skip processing of this PostEffect and its children even if its parent is enabled. 
   /// Parent and sibling PostEffects in the chain are still processed.
   /// This is intended for debugging purposes.
   bool mSkip;

   bool mOneFrameOnly;
   bool mOnThisFrame;

   U32 mShaderReloadKey;

   PostEffect* mPostEffect;

public:
   PostEffectAsset();
   virtual ~PostEffectAsset();

   /// Engine.
   static void initPersistFields();
   virtual void copyTo(SimObject* object);

   virtual void initializeAsset();

   /// Declare Console Object.
   DECLARE_CONOBJECT(PostEffectAsset);

protected:
   virtual void            onAssetRefresh(void) {}
};

DefineConsoleType(TypePostEffectAssetPtr, PostEffectAsset)

#endif // _ASSET_BASE_H_

