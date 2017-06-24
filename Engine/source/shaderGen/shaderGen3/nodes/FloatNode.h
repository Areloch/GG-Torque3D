#pragma once
#include "./ShaderGenNode.h"

class FloatNode : public ShaderGenNode
{
public:
   FloatNode() {};
   ~FloatNode() {};

   StringTableEntry mUniformName;
   F32 mValue;

   virtual void processVertex(GenerateSettings* matSettings, ReturnType retType = ReturnFloat);
   virtual String getVertexReference(GenerateSettings* matSettings, ReturnType retType = ReturnFloat);

   virtual void processPixels(GenerateSettings* matSettings, ReturnType retType = ReturnFloat);
   virtual String getPixelReference(GenerateSettings* matSettings, ReturnType retType = ReturnFloat);
};

void FloatNode::processVertex(GenerateSettings* matSettings, ReturnType retType)
{

}

String FloatNode::getVertexReference(GenerateSettings* matSettings, ReturnType retType)
{
   char ret[64];

   if (mUniformName != "")
      dSprintf(ret, 64, "%s", mUniformName);
   else
      dSprintf(ret, 64, "%f", mValue);

   return ret;
}

void FloatNode::processPixels(GenerateSettings* matSettings, ReturnType retType)
{
   if (mUniformName != String::EmptyString)
   {
      matSettings->TargetMaterial.addPixelUniform(mUniformName, "float");
   }
}

String FloatNode::getPixelReference(GenerateSettings* matSettings, ReturnType retType)
{
   char ret[64];

   if(mUniformName != "")
      dSprintf(ret, 64, "%s", mUniformName);
   else
      dSprintf(ret, 64, "%f", mValue);

   return ret;
}

