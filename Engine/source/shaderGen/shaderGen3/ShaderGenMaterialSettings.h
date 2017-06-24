#pragma once
#include "console/stringStack.h"
#include "math/mMath.h"

struct ProtoMaterial
{
public:
//private:
   struct Ins
   {
      String type;
      String name;
      String target;
   };

   struct Outs
   {
      String type;
      String name;
      String target;
   };

   struct Uniforms
   {
      String type;
      String name;
      String registerName;
   };

   struct Nodes
   {
      String type;
      String name;
      U32 UID;
      Point2F position;
   };

   Vector<Ins> mPixelInputs;
   Vector<Outs> mPixelOutputs;
   Vector<Uniforms> mPixelUniforms;

   Vector<Ins> mVertexIns;
   Vector<Outs> mVertexOuts;
   Vector<Uniforms> mVertexUniforms;

   Vector<String> mVertexShaderBody;
   Vector<String> mPixelShaderBody;

public:
   void addVertexBodyLine(const char* format, ...)
   {
      char buff[512];

      va_list args;
      va_start(args, format);
      dSprintf(buff, 512, format, args);
      va_end(args);

      mVertexShaderBody.push_back(buff);
   }

   void addPixelBodyLine(const char* format, ...)
   {
      char buff[512];

      va_list args;
      va_start(args, format);
      dSprintf(buff, 512, format, args);
      va_end(args);

      mPixelShaderBody.push_back(buff);
   }

   void addVertexIn(String name, String type, String target)
   {
      Ins newIn;
      newIn.name = name;
      newIn.type = type;
      newIn.target = target;

      mVertexIns.push_back(newIn);
   }

   void addVertexOut(String name, String type, String target)
   {
      Outs newOut;
      newOut.name = name;
      newOut.type = type;
      newOut.target = target;

      mVertexOuts.push_back(newOut);
   }

   void addVertexUniform(String name, String type, String registerName = "")
   {
      Uniforms newUni;
      newUni.name = name;
      newUni.type = type;
      newUni.registerName = registerName;

      mVertexUniforms.push_back(newUni);
   }

   void addPixelUniform(String name, String type, String registerName = "")
   {
      Uniforms newUni;
      newUni.name = name;
      newUni.type = type;
      newUni.registerName = registerName;

      mPixelUniforms.push_back(newUni);
   }
};

struct GenerateSettings
{
   String TargetAPI;
   String TargetShader;
   ProtoMaterial TargetMaterial;

   String Name;
   String outputScriptFile;
   String outputVertexFile;
   String outputPixelFile;

   bool doubleSided;
   bool translucent;
   String blendOp;
   String translucentBlendOp;
};