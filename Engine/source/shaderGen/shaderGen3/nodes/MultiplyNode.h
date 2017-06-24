#pragma once
#include "./ShaderGenNode.h"

class MultipleNode : public ShaderGenNode
{
public:
   MultipleNode() {};
   ~MultipleNode() {};

   StringTableEntry mUniformName;
   F32 mValue;

   U32 mInputNodeIdxA;
   U32 mInputNodeIdxB;

   virtual void processVertex(GenerateSettings* matSettings, ReturnType retType = ReturnFloat);
   virtual String getVertexReference(GenerateSettings* matSettings, ReturnType retType = ReturnFloat);

   virtual void processPixels(GenerateSettings* matSettings, ReturnType retType = ReturnFloat);
   virtual String getPixelReference(GenerateSettings* matSettings, ReturnType retType = ReturnFloat);
};

void MultipleNode::processVertex(GenerateSettings* matSettings, ReturnType retType)
{

}

String MultipleNode::getVertexReference(GenerateSettings* matSettings, ReturnType retType)
{
   char ret[64];
   dSprintf(ret, 64, "%f", mValue);

   return ret;
}

void MultipleNode::processPixels(GenerateSettings* matSettings, ReturnType retType)
{
   ShaderGenNode* inputA = findNode(matSettings, mInputNodeIdxA);
   ShaderGenNode* inputB = findNode(matSettings, mInputNodeIdxB);

   if (inputA != nullptr && inputB != nullptr)
   {
      inputA->processPixels(matSettings, retType);
      inputB->processPixels(matSettings, retType);

      /*switch (retType)
      {
      case ReturnFloat:

      }*/
   }

   if (mUniformName != String::EmptyString)
   {
      matSettings->TargetMaterial.addPixelUniform(mUniformName, "float");
   }
}

String MultipleNode::getPixelReference(GenerateSettings* matSettings, ReturnType retType)
{
   char ret[64];
   dSprintf(ret, 64, "%f", mValue);

   return ret;
}
