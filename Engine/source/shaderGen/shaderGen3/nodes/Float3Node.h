#pragma once
#include "./ShaderGenNode.h"

class Float3Node : public ShaderGenNode
{
public:
   Float3Node() {};
   ~Float3Node() {};

   StringTableEntry mUniformName;
   Point3F mValue;

   virtual void processVertex(GenerateSettings* matSettings, ReturnType retType = ReturnFloat);
   virtual String getVertexReference(GenerateSettings* matSettings, ReturnType retType = ReturnFloat);

   virtual void processPixels(GenerateSettings* matSettings, ReturnType retType = ReturnFloat);
   virtual String getPixelReference(GenerateSettings* matSettings, ReturnType retType = ReturnFloat);
};

void Float3Node::processVertex(GenerateSettings* matSettings, ReturnType retType)
{

}

String Float3Node::getVertexReference(GenerateSettings* matSettings, ReturnType retType)
{
   char ret[64];

   if (mUniformName != "")
      dSprintf(ret, 64, "%s.xyz", mUniformName);
   else
      dSprintf(ret, 64, "%f", mValue);

   return ret;
}

void Float3Node::processPixels(GenerateSettings* matSettings, ReturnType retType)
{
   if (mUniformName != String::EmptyString)
   {
      matSettings->TargetMaterial.addPixelUniform(mUniformName, "float3");
   }
}

String Float3Node::getPixelReference(GenerateSettings* matSettings, ReturnType retType)
{
   char ret[64];
   if(mUniformName != "")
      dSprintf(ret, 64, "%s.rgb", mUniformName, mValue);
   else
      dSprintf(ret, 64, "%f", mValue);

   return ret;
}

