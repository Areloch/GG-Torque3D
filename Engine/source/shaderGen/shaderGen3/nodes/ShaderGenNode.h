#pragma once
#include "console/stringStack.h"
#include "shaderGen/shaderGen3/shaderGen3.h"
#include "shaderGen/shaderGen3/ShaderGenMaterialSettings.h"

class ShaderGen3;

class ShaderGenNode
{
   char mReturnBuffer[128];
   bool isRootNode;

public:
   ShaderGenNode() 
   {
      mReturnBuffer[0] = '\0';
      isRootNode = false;
   };
   ~ShaderGenNode() {};

   enum ReturnType
   {
      ReturnFloat,
      ReturnFloat2,
      ReturnFloat3,
      ReturnFloat4,
   };

   virtual void processVertex(GenerateSettings* matSettings, ReturnType retType = ReturnFloat) {}
   virtual String getVertexReference(GenerateSettings* matSettings, ReturnType retType = ReturnFloat) { return ""; }

   virtual void processPixels( GenerateSettings* matSettings, ReturnType retType = ReturnFloat) {}
   virtual String getPixelReference(GenerateSettings* matSettings, ReturnType retType = ReturnFloat) { return ""; }
   
   virtual ShaderGenNode* findNode(GenerateSettings* matSettings, U32 nodeId) { return nullptr; }
};