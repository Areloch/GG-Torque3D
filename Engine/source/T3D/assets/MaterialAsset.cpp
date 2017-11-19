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

#ifndef MATERIALASSET_H
#include "MaterialAsset.h"
#endif

#ifndef _ASSET_MANAGER_H_
#include "assets/assetManager.h"
#endif

#ifndef _CONSOLETYPES_H_
#include "console/consoleTypes.h"
#endif

#ifndef _TAML_
#include "persistence/taml/taml.h"
#endif

#ifndef _ASSET_PTR_H_
#include "assets/assetPtr.h"
#endif

#include "T3D/assets/ImageAsset.h"

#include "materials/shaderData.h"

#include "gfx/gfxTextureHandle.h"

//#include "shaderGen/shaderGen3/shaderGen3.h"

// Script bindings.
//#include "assets/assetBase_ScriptBinding.h"

// Debug Profiling.
#include "platform/profiler.h"

#include "core/fileObject.h"

#include "shaderGen/shaderGenVars.h"

static U32 execDepth = 0;
static U32 journalDepth = 1;

//-----------------------------------------------------------------------------

IMPLEMENT_CONOBJECT(MaterialAsset);

ConsoleType(TestAssetPtr, TypeMaterialAssetPtr, MaterialAsset, ASSET_ID_FIELD_PREFIX)

//-----------------------------------------------------------------------------

ConsoleGetType(TypeMaterialAssetPtr)
{
   // Fetch asset Id.
   return (*((AssetPtr<MaterialAsset>*)dptr)).getAssetId();
}

//-----------------------------------------------------------------------------

ConsoleSetType(TypeMaterialAssetPtr)
{
   // Was a single argument specified?
   if (argc == 1)
   {
      // Yes, so fetch field value.
      const char* pFieldValue = argv[0];

      // Fetch asset pointer.
      AssetPtr<MaterialAsset>* pAssetPtr = dynamic_cast<AssetPtr<MaterialAsset>*>((AssetPtrBase*)(dptr));

      // Is the asset pointer the correct type?
      if (pAssetPtr == NULL)
      {
         // No, so fail.
         //Con::warnf("(TypeTextureAssetPtr) - Failed to set asset Id '%d'.", pFieldValue);
         return;
      }

      // Set asset.
      pAssetPtr->setAssetId(pFieldValue);

      return;
   }

   // Warn.
   Con::warnf("(TypeTextureAssetPtr) - Cannot set multiple args to a single asset.");
}

//-----------------------------------------------------------------------------

MaterialAsset::MaterialAsset()
{
   mShaderGraphFile = "";
   mScriptFile = "";
   mMatDefinitionName = "";

   //mShaderFile = StringTable->lookup("");
   //mShaderGraphFile = StringTable->lookup("");
   //mDiffuseText = StringTable->lookup("");

   //mMaterial = NULL;
   //mMatInstance = NULL;
}

//-----------------------------------------------------------------------------

MaterialAsset::~MaterialAsset()
{
   // If the asset manager does not own the asset then we own the
   // asset definition so delete it.
   if (!getOwned())
      delete mpAssetDefinition;
}

//-----------------------------------------------------------------------------

void MaterialAsset::initPersistFields()
{
   // Call parent.
   Parent::initPersistFields();

   addField("shaderGraph", TypeRealString, Offset(mShaderGraphFile, MaterialAsset), "");
   addField("scriptFile", TypeRealString, Offset(mScriptFile, MaterialAsset), "");
   addField("materialDefinitionName", TypeRealString, Offset(mMatDefinitionName, MaterialAsset), "");
   //addField("shaderGraph", TypeRealString, Offset(mShaderGraphFile, MaterialAsset), "");

   //addField("diffuseTexture", TypeImageFilename, Offset(mDiffuseText, MaterialAsset), "");
   //addField("diffuseTexture", TypeAssetId, Offset(mDiffuseTexture, MaterialAsset), "");
}

void MaterialAsset::initializeAsset()
{
   // Call parent.
   Parent::initializeAsset();

   compileShader();

   if (Platform::isFile(mScriptFile))
      Con::executeFile(mScriptFile, false, false);

   return;

/*   if (String::isEmpty(mShaderFile) || String::isEmpty(mDiffuseText))
      return;

   //
   //mDiffuseTextureAsset.setAssetId(mDiffuseTexture);
   //

   //if we have a shadergraph file, use that, otherwise if we have a shaderData definition, use that

   SAFE_DELETE(mMaterial);

   mMaterial = new CustomMaterial();

   ShaderData* shdr = new ShaderData();
   shdr->registerObject();
   shdr->mDXPixelShaderName = "shaders/common/MeshAssetP.hlsl";
   shdr->mDXVertexShaderName = "shaders/common/MeshAssetV.hlsl";
   shdr->mPixVersion = 2.0;

   mMaterial->registerObject();
   mMaterial->mShaderData = shdr;
   //mMaterial->mSamplerNames[0] = "diffuseMap";
   //mMaterial->mTexFilename[0] = "core/art/grids/512_orange";

   const char* samplerName = "";
   for (U32 i = 0; i < dStrcmp(samplerName = getDataField(avar("samplers%d", i), NULL), "") != 0; ++i)
   {
      mMaterial->mSamplerNames[i] = samplerName;
   }

   const char* textureName = "";
   for (U32 i = 0; i < dStrcmp(textureName = getDataField(avar("textures%d", i), NULL), "") != 0; ++i)
   {
      //get the image asset and assign it to the sampler name
      ImageAsset* imgAst = mpOwningAssetManager->acquireAsset<ImageAsset>(textureName);
      if (imgAst)
      {
         mMaterial->mTexFilename[i] = imgAst->getImageFileName();
      }
   }

   mMaterial->mOutputTarget = getDataField("outputTarget", NULL);*/

   //
   //
   //

   //mMaterial->mSamplerNames[0]

   /*mShader = NULL;
   mShaderConstBuffer = NULL;

   //mDeferredTarget = NULL;

   // Need depth from pre-pass, so get the macros
   Vector<GFXShaderMacro> macros;

   if (!mDeferredTarget)
      mDeferredTarget = NamedTexTarget::find("deferred");

   if (mDeferredTarget)
      mDeferredTarget->getShaderMacros(&macros);

   ShaderData *shaderData;

   // Load Propagated Display Shader
   if (!Sim::findObject(mShaderFile, shaderData))
   {
      Con::warnf("OfflineLPV::_initShader - failed to locate shader OfflineLPVPropagatedShaderData!");
      return;
   }

   // DEF example
   /*if (AdvancedLightBinManager::smUseSSAOMask)
      macros.push_back(GFXShaderMacro("USE_SSAO_MASK"));

   mPropagatedShader = shaderData->getShader(macros);
   if (!mPropagatedShader)
      return;*/

   /*mShaderConstBuffer = mShader->allocConstBuffer();

   mShader->getSha

   constHandle handle = constHandle();
   handle.handleName = "$eyePosWorld";
   handle.handle = mShader->getShaderConstHandle(handle.handleName);
   handle.mType = constHandle::Float3;

   mConstHandlesList.push_back(handle);*/

   //mEyePosWorldPropSC = mShader->getShaderConstHandle("$eyePosWorld");
   //mRTParamsPropSC = mShader->getShaderConstHandle("$rtParams0");
   //mVolumeStartPropSC = mShader->getShaderConstHandle("$volumeStart");
   //mVolumeSizePropSC = mShader->getShaderConstHandle("$volumeSize");
}

void MaterialAsset::onAssetRefresh()
{
   if (Platform::isFile(mScriptFile))
      Con::executeFile(mScriptFile, false, false);

   if (!mMatDefinitionName.isEmpty())
   {
      Material* matDef;
      if (!Sim::findObject(mMatDefinitionName.c_str(), matDef))
      {
         Con::errorf("MaterialAsset: Unable to find the Material %s", mMatDefinitionName.c_str());
         return;
      }

      matDef->reload();
   }
}

/*void MaterialAsset::parseShaderData()
{
   //we have a shaderdata declaration, so we'll parse that to build our fields and constants
   //load the shader file and parse for uniforms!

   /*Torque::Path shaderFile = mShader->getPixelShaderFile();

   FileObject f;
   f.readMemory(shaderFile.getFullPath());

   while (!f.isEOF())
   {
      String line = (const char *)f.readLine();

      if (line.find("$diffuseMaterialColor") != 0)
      {

      }
      else if (line.find(ShaderGenVars::texMat) != 0)
      {

      }
      else if (line.find(ShaderGenVars::toneMap) != 0)
      {

      }
      else if (line.find(ShaderGenVars::specularColor) != 0)
      {

      }
      else if (line.find(ShaderGenVars::specularPower) != 0)
      {

      }
      else if (line.find(ShaderGenVars::specularStrength) != 0)
      {

      }
      else if (line.find("$accuScale") != 0)
      {

      }
      else if (line.find("$accuDirection") != 0)
      {

      }
      else if (line.find("$accuStrength") != 0)
      {

      }
      else if (line.find("$accuCoverage") != 0)
      {

      }
      else if (line.find("$accuSpecular") != 0)
      {

      }
      else if (line.find("$parallaxInfo") != 0)
      {

      }
      else if (line.find(ShaderGenVars::fogData) != 0)
      {

      }
   }

   //next, parse for any standard constants we'd want to use, such as worldSpaceEyeVector or the like
   GFXShaderConstBuffer* shaderConsts = _getShaderConstBuffer(pass);
   ShaderConstHandles* handles = _getShaderConstHandles(pass);
   U32 stageNum = getStageFromPass(pass);

   // First we do all the constants which are not
   // controlled via the material... we have to
   // set these all the time as they could change.

   if (handles->mFogDataSC->isValid())
   {
      Point3F fogData;
      fogData.x = sgData.fogDensity;
      fogData.y = sgData.fogDensityOffset;
      fogData.z = sgData.fogHeightFalloff;
      shaderConsts->set(handles->mFogDataSC, fogData);
   }

   shaderConsts->setSafe(handles->mFogColorSC, sgData.fogColor);

   if (handles->mOneOverFarplane->isValid())
   {
      const F32 &invfp = 1.0f / state->getFarPlane();
      Point4F oneOverFP(invfp, invfp, invfp, invfp);
      shaderConsts->set(handles->mOneOverFarplane, oneOverFP);
   }

   shaderConsts->setSafe(handles->mAccumTimeSC, MATMGR->getTotalTime());

   // If the shader constants have not been lost then
   // they contain the content from a previous render pass.
   //
   // In this case we can skip updating the material constants
   // which do not change frame to frame.
   //
   // NOTE: This assumes we're not animating material parameters
   // in a way that doesn't cause a shader reload... this isn't
   // being done now, but it could change in the future.
   // 
   if (!shaderConsts->wasLost())
      return;

   shaderConsts->setSafe(handles->mSpecularColorSC, mMaterial->mSpecular[stageNum]);
   shaderConsts->setSafe(handles->mSpecularPowerSC, mMaterial->mSpecularPower[stageNum]);
   shaderConsts->setSafe(handles->mSpecularStrengthSC, mMaterial->mSpecularStrength[stageNum]);

   shaderConsts->setSafe(handles->mParallaxInfoSC, mMaterial->mParallaxScale[stageNum]);
   shaderConsts->setSafe(handles->mMinnaertConstantSC, mMaterial->mMinnaertConstant[stageNum]);

   if (handles->mSubSurfaceParamsSC->isValid())
   {
      Point4F subSurfParams;
      dMemcpy(&subSurfParams, &mMaterial->mSubSurfaceColor[stageNum], sizeof(ColorF));
      subSurfParams.w = mMaterial->mSubSurfaceRolloff[stageNum];
      shaderConsts->set(handles->mSubSurfaceParamsSC, subSurfParams);
   }

   if (handles->mRTSizeSC->isValid())
   {
      const Point2I &resolution = GFX->getActiveRenderTarget()->getSize();
      Point2F pixelShaderConstantData;

      pixelShaderConstantData.x = resolution.x;
      pixelShaderConstantData.y = resolution.y;

      shaderConsts->set(handles->mRTSizeSC, pixelShaderConstantData);
   }

   if (handles->mOneOverRTSizeSC->isValid())
   {
      const Point2I &resolution = GFX->getActiveRenderTarget()->getSize();
      Point2F oneOverTargetSize(1.0f / (F32)resolution.x, 1.0f / (F32)resolution.y);

      shaderConsts->set(handles->mOneOverRTSizeSC, oneOverTargetSize);
   }

   // set detail scale
   shaderConsts->setSafe(handles->mDetailScaleSC, mMaterial->mDetailScale[stageNum]);
   shaderConsts->setSafe(handles->mDetailBumpStrength, mMaterial->mDetailNormalMapStrength[stageNum]);

   // MFT_ImposterVert
   if (handles->mImposterUVs->isValid())
   {
      U32 uvCount = getMin(mMaterial->mImposterUVs.size(), 64); // See imposter.hlsl   
      AlignedArray<Point4F> imposterUVs(uvCount, sizeof(Point4F), (U8*)mMaterial->mImposterUVs.address(), false);
      shaderConsts->set(handles->mImposterUVs, imposterUVs);
   }
   shaderConsts->setSafe(handles->mImposterLimits, mMaterial->mImposterLimits);

   // Diffuse
   shaderConsts->setSafe(handles->mDiffuseColorSC, mMaterial->mDiffuse[stageNum]);

   shaderConsts->setSafe(handles->mAlphaTestValueSC, mClampF((F32)mMaterial->mAlphaRef / 255.0f, 0.0f, 1.0f));

   if (handles->mDiffuseAtlasParamsSC)
   {
      Point4F atlasParams(1.0f / mMaterial->mCellLayout[stageNum].x, // 1 / num_horizontal
         1.0f / mMaterial->mCellLayout[stageNum].y, // 1 / num_vertical
         mMaterial->mCellSize[stageNum],            // tile size in pixels
         getBinLog2(mMaterial->mCellSize[stageNum]));    // pow of 2 of tile size in pixels 2^9 = 512, 2^10=1024 etc
      shaderConsts->setSafe(handles->mDiffuseAtlasParamsSC, atlasParams);
   }

   if (handles->mBumpAtlasParamsSC)
   {
      Point4F atlasParams(1.0f / mMaterial->mCellLayout[stageNum].x, // 1 / num_horizontal
         1.0f / mMaterial->mCellLayout[stageNum].y, // 1 / num_vertical
         mMaterial->mCellSize[stageNum],            // tile size in pixels
         getBinLog2(mMaterial->mCellSize[stageNum]));    // pow of 2 of tile size in pixels 2^9 = 512, 2^10=1024 etc
      shaderConsts->setSafe(handles->mBumpAtlasParamsSC, atlasParams);
   }

   if (handles->mDiffuseAtlasTileSC)
   {
      // Sanity check the wrap flags
      //AssertWarn(mMaterial->mTextureAddressModeU == mMaterial->mTextureAddressModeV, "Addresing mode mismatch, texture atlasing will be confused");
      Point4F atlasTileParams(mMaterial->mCellIndex[stageNum].x, // Tile co-ordinate, ie: [0, 3]
         mMaterial->mCellIndex[stageNum].y,
         0.0f, 0.0f); // TODO: Wrap mode flags?
      shaderConsts->setSafe(handles->mDiffuseAtlasTileSC, atlasTileParams);
   }

   if (handles->mBumpAtlasTileSC)
   {
      // Sanity check the wrap flags
      //AssertWarn(mMaterial->mTextureAddressModeU == mMaterial->mTextureAddressModeV, "Addresing mode mismatch, texture atlasing will be confused");
      Point4F atlasTileParams(mMaterial->mCellIndex[stageNum].x, // Tile co-ordinate, ie: [0, 3]
         mMaterial->mCellIndex[stageNum].y,
         0.0f, 0.0f); // TODO: Wrap mode flags?
      shaderConsts->setSafe(handles->mBumpAtlasTileSC, atlasTileParams);
   }

   if (handles->mAccuScaleSC->isValid())
      shaderConsts->set(handles->mAccuScaleSC, mMaterial->mAccuScale[stageNum]);
   if (handles->mAccuDirectionSC->isValid())
      shaderConsts->set(handles->mAccuDirectionSC, mMaterial->mAccuDirection[stageNum]);
   if (handles->mAccuStrengthSC->isValid())
      shaderConsts->set(handles->mAccuStrengthSC, mMaterial->mAccuStrength[stageNum]);
   if (handles->mAccuCoverageSC->isValid())
      shaderConsts->set(handles->mAccuCoverageSC, mMaterial->mAccuCoverage[stageNum]);
   if (handles->mAccuSpecularSC->isValid())
      shaderConsts->set(handles->mAccuSpecularSC, mMaterial->mAccuSpecular[stageNum]);*/
/*}

void MaterialAsset::parseShaderGraph()
{

}*/
//------------------------------------------------------------------------------

void MaterialAsset::compileShader()
{
   /*if (mShaderGraphFile.isEmpty())
   {
      Con::errorf("MaterialAsset:initializeAsset: No valid Shader Graph file!");
      return;
   }

   ShaderGen3* sg;
   if (!Sim::findObject("ShaderGen", sg))
   {
      sg = new ShaderGen3();
      sg->registerObject("ShaderGen");
   }

   sg->compileShaderGraph(mShaderGraphFile);*/
}

void MaterialAsset::copyTo(SimObject* object)
{
   // Call to parent.
   Parent::copyTo(object);
}

ConsoleMethod(MaterialAsset, compileShader, void, 2, 2, "() - Gets a field description by index\n"
   "@param index The index of the behavior\n"
   "@return Returns a string representing the description of this field\n")
{
   object->compileShader();
}