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

#include "platform/platform.h"
#include "shaderGen/GLSL/shaderFeatureGLSL.h"
#include "shaderGen/GLSL/materialDamageGLSL.h"

#include "shaderGen/langElement.h"
#include "shaderGen/shaderOp.h"
#include "shaderGen/shaderGenVars.h"
#include "gfx/gfxDevice.h"
#include "materials/matInstance.h"
#include "materials/processedMaterial.h"
#include "materials/materialFeatureTypes.h"
#include "core/util/autoPtr.h"

#include "lighting/advanced/advancedLightBinManager.h"

//****************************************************************************
// Damage Texture -Albedo
//****************************************************************************
U32 AlbedoDamageFeatGLSL::getOutputTargets(const MaterialFeatureData &fd) const
{
   return fd.features[MFT_isDeferred] ? ShaderFeature::RenderTarget1 : ShaderFeature::DefaultTarget;
}

void AlbedoDamageFeatGLSL::processPix(Vector<ShaderComponent*> &componentList,
   const MaterialFeatureData &fd)
{
   //determine output target
   Var* targ = (Var*)LangElement::find(getOutputTargetVarName(ShaderFeature::DefaultTarget));
   if (fd.features[MFT_isDeferred])
   {
      targ = (Var*)LangElement::find(getOutputTargetVarName(ShaderFeature::RenderTarget1));
   }

   // Get the texture coord.
   Var *texCoord = getInTexCoord("texCoord", "vec2", true, componentList);

   Var *damage = new Var("materialDamage", "float");
   damage->uniform = true;
   damage->constSortPos = cspPrimitive;

   // create texture var
   Var *albedoDamage = new Var;
   albedoDamage->setType("sampler2D");
   albedoDamage->setName("albedoDamageMap");
   albedoDamage->uniform = true;
   albedoDamage->sampler = true;
   albedoDamage->constNum = Var::getTexUnitNum();     // used as texture unit num here

   Var *floor = (Var*)LangElement::find("materialDamageMin");
   if (!floor){
      floor = new Var("materialDamageMin", "float");
      floor->uniform = true;
      floor->constSortPos = cspPrimitive;
   }

   MultiLine * meta = new MultiLine;
   Var *damageResult = (Var*)LangElement::find("damageResult");
   if (!damageResult){
      damageResult = new Var("damageResult", "float");
      meta->addStatement(new GenOp("   @ = max(@,@);\r\n", new DecOp(damageResult), floor, damage));
   }
   else
      meta->addStatement(new GenOp("   @ = max(@,@);\r\n", damageResult, floor, damage));


   LangElement *statement = NULL;
   if (fd.features[MFT_Imposter])
   {
      statement = new GenOp("texture(@, @)", albedoDamage, texCoord);
   }
   else
   {
      statement = new GenOp("toLinear(texture(@, @))", albedoDamage, texCoord);
   }

   meta->addStatement(new GenOp("   @ = mix(@,@,@);\r\n", targ, targ, statement, damageResult));
   output = meta;
}

ShaderFeature::Resources AlbedoDamageFeatGLSL::getResources(const MaterialFeatureData &fd)
{
   Resources res;
   res.numTex = 1;
   res.numTexReg = 1;

   return res;
}

void AlbedoDamageFeatGLSL::setTexData(Material::StageData &stageDat,
   const MaterialFeatureData &fd,
   RenderPassData &passData,
   U32 &texIndex)
{
   GFXTextureObject *tex = stageDat.getTex(MFT_AlbedoDamage);
   if (tex)
   {
      passData.mTexType[texIndex] = Material::Standard;
      passData.mSamplerNames[texIndex] = "albedoDamageMap";
      passData.mTexSlot[texIndex++].texObject = tex;
   }
}

void AlbedoDamageFeatGLSL::processVert(Vector<ShaderComponent*> &componentList,
   const MaterialFeatureData &fd)
{
   MultiLine *meta = new MultiLine;
   getOutTexCoord("texCoord",
      "vec2",
      true,
      fd.features[MFT_TexAnim],
      meta,
      componentList);
   output = meta;
}

//****************************************************************************
// Damage Texture -Composite
//****************************************************************************
void CompositeDamageFeatGLSL::processPix(Vector<ShaderComponent*> &componentList,
   const MaterialFeatureData &fd)
{
   // Get the texture coord.
   Var *texCoord = getInTexCoord("texCoord", "vec2", true, componentList);

   Var *damage = (Var*)LangElement::find("materialDamage");
   if (!damage){
      damage = new Var("materialDamage", "float");
      damage->uniform = true;
      damage->constSortPos = cspPrimitive;
   }

   // create texture var
   Var *damageCMap = new Var;
   damageCMap->setType("sampler2D");
   damageCMap->setName("compositeDamageMap");
   damageCMap->uniform = true;
   damageCMap->sampler = true;
   damageCMap->constNum = Var::getTexUnitNum();     // used as texture unit num here

   Var *damageComposite = new Var;
   damageComposite->setType("float4");
   damageComposite->setName("compositeDamageColor");

   //uniforms
   bool declareSpec, declareMetal, declareSmooth;
   declareSpec = declareMetal = declareSmooth = false;

   Var *specularColor = (Var*)LangElement::find("specularColor");
   if (!specularColor) {
      specularColor = new Var("specularColor", "vec4");
      declareSpec = true;
   };

   Var *metalness = (Var*)LangElement::find("metalness");
   if (!metalness){
      metalness = new Var("metalness", "float");
      declareMetal = true;
   }

   Var *smoothness = (Var*)LangElement::find("smoothness");
   if (!smoothness){
      smoothness = new Var("smoothness", "float");
      declareSmooth = true;
   }

   MultiLine * meta = new MultiLine;

   Var *floor = (Var*)LangElement::find("materialDamageMin");
   if (!floor){
      floor = new Var("materialDamageMin", "float");
      floor->uniform = true;
      floor->constSortPos = cspPrimitive;
   }

   Var *damageResult = (Var*)LangElement::find("damageResult");
   if (!damageResult){
      damageResult = new Var("damageResult", "float");
      meta->addStatement(new GenOp("   @ = max(@,@);\r\n", new DecOp(damageResult), floor, damage));
   }
   else
      meta->addStatement(new GenOp("   @ = max(@,@);\r\n", damageResult, floor, damage));

   meta->addStatement(new GenOp("   @ = texture(@, @);\r\n",
      new DecOp(damageComposite), damageCMap, texCoord));
   if (declareSmooth)
      meta->addStatement(new GenOp("   @ = mix(0.0,@.r,@);\r\n", new DecOp(smoothness), damageComposite, damageResult));
   else
      meta->addStatement(new GenOp("   @ = mix(@,@.r,@);\r\n", smoothness, smoothness, damageComposite, damageResult));

   if (declareSpec)
      meta->addStatement(new GenOp("   @ = mix(float4(1.0,1.0,1.0,1.0),@.ggga,@);\r\n", new DecOp(specularColor), damageComposite, damageResult));
   else
      meta->addStatement(new GenOp("   @ = mix(@,@.ggga,@);\r\n", specularColor, specularColor, damageComposite, damageResult));

   if (declareMetal)
      meta->addStatement(new GenOp("   @ = mix(0.0,@.b,@);\r\n", new DecOp(metalness), damageComposite, damageResult));
   else
      meta->addStatement(new GenOp("   @ = mix(@,@.b,@);\r\n", metalness, metalness, damageComposite, damageResult));

   if (fd.features.hasFeature(MFT_isDeferred))
   {
      // search for material var
      Var *material = (Var*)LangElement::find(getOutputTargetVarName(ShaderFeature::RenderTarget2));
      if (!material)
      {
         // create material var
         material = new Var;
         material->setType("fragout");
         material->setName(getOutputTargetVarName(ShaderFeature::RenderTarget2));
         material->setStructName("OUT");
      }
      
      meta->addStatement(new GenOp("   @.bga = vec3(@,@.g,@);\r\n", material, smoothness, specularColor, metalness));
   }
   output = meta;
}

ShaderFeature::Resources CompositeDamageFeatGLSL::getResources(const MaterialFeatureData &fd)
{
   Resources res;
   res.numTex = 1;
   res.numTexReg = 1;

   return res;
}

void CompositeDamageFeatGLSL::setTexData(Material::StageData &stageDat,
   const MaterialFeatureData &fd,
   RenderPassData &passData,
   U32 &texIndex)
{
   GFXTextureObject *tex = stageDat.getTex(MFT_CompositeDamage);
   if (tex)
   {
      passData.mTexType[texIndex] = Material::Standard;
      passData.mSamplerNames[texIndex] = "compositeDamageMap";
      passData.mTexSlot[texIndex++].texObject = tex;
   }
}

void CompositeDamageFeatGLSL::processVert(Vector<ShaderComponent*> &componentList,
   const MaterialFeatureData &fd)
{
   MultiLine *meta = new MultiLine;
   getOutTexCoord("texCoord",
      "vec2",
      true,
      fd.features[MFT_TexAnim],
      meta,
      componentList);
   output = meta;
}