#pragma once
#include "./ShaderGenNode.h"

class DeferredMaterialNode : public ShaderGenNode
{
public:
   DeferredMaterialNode() {};
   ~DeferredMaterialNode() {};

   U32 inputSourceIdxA; //Diffuse
   U32 inputSourceIdxB; //Normal
   U32 inputSourceIdxC; //Metalness
   U32 inputSourceIdxD; //Roughness
   U32 inputSourceIdxE; //AO

   virtual void processVertex(GenerateSettings* matSettings, ReturnType retType = ReturnFloat);
   virtual String getVertexReference(GenerateSettings* matSettings, ReturnType retType = ReturnFloat) { return ""; }

   virtual void processPixels(GenerateSettings* matSettings, ReturnType retType = ReturnFloat);
   virtual String getPixelReference(GenerateSettings* matSettings, ReturnType retType = ReturnFloat) { return ""; }
};

void DeferredMaterialNode::processVertex(GenerateSettings* matSettings, ReturnType retType)
{
   matSettings->TargetMaterial.addVertexIn("position", "float3", "POSITION");
   matSettings->TargetMaterial.addVertexIn("normal", "float3", "NORMAL");
   matSettings->TargetMaterial.addVertexIn("texCoord", "float2", "TEXCOORD0");
   matSettings->TargetMaterial.addVertexIn("T", "float3", "TANGENT");
   matSettings->TargetMaterial.addVertexIn("tangentW", "float", "TEXCOORD3");

   matSettings->TargetMaterial.addVertexOut("hpos", "float4", "POSITION");
   matSettings->TargetMaterial.addVertexOut("out_texCoord", "float2", "TEXCOORD0");
   matSettings->TargetMaterial.addVertexOut("outViewToTangent", "float3x3", "TEXCOORD1");
   matSettings->TargetMaterial.addVertexOut("reflectVec", "float3", "TEXCOORD4");
   matSettings->TargetMaterial.addVertexOut("wsEyeVec", "float4", "TEXCOORD5");

   matSettings->TargetMaterial.addVertexUniform("modelView", "float4x4", "C0");
   matSettings->TargetMaterial.addVertexUniform("viewToObj", "float4x4", "C4");
   matSettings->TargetMaterial.addVertexUniform("objTrans", "float4x4", "C8");
   matSettings->TargetMaterial.addVertexUniform("eyePosWorld", "float3", "C0");
}

void DeferredMaterialNode::processPixels(GenerateSettings* matSettings, ReturnType retType)
{
   if (matSettings->TargetAPI == String("D3D11") || matSettings->TargetAPI == String("D3D12"))
   {
      String diffuseRet;
      ShaderGenNode* diffuseInput = findNode(matSettings, inputSourceIdxA);
      if (diffuseInput != nullptr)
      {
         diffuseInput->processPixels(matSettings);
         diffuseRet = diffuseInput->getPixelReference(matSettings, ReturnFloat3);
      }

      //get connections for the diffuse field
      matSettings->TargetMaterial.addPixelBodyLine("   OUT.color = %s.rgb;", diffuseRet);
      /*matSettings->TargetMaterial.insertPixelBodyLine("");
      matSettings->TargetMaterial.insertPixelBodyLine("   OUT.norm = float4(1,1,1,1);");
      matSettings->TargetMaterial.insertPixelBodyLine("");
      matSettings->TargetMaterial.insertPixelBodyLine("   OUT.roughness = float4(1,1,1,1);");
      matSettings->TargetMaterial.insertPixelBodyLine("");
      matSettings->TargetMaterial.insertPixelBodyLine("   OUT.metalness = float4(1,1,1,1);");*/
   }
   else
   {
      matSettings->TargetMaterial.addPixelBodyLine("   OUT_color = float4(1,1,1,1);");
   }
}