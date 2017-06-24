#pragma once
#ifndef SHADERGEN3
#define SHADERGEN3

#include "console/engineAPI.h"
#include "console/stringStack.h"

#ifndef _XMLDOC_H_
#include "console/SimXMLDocument.h"
#endif

#include "math/mMath.h"
#include "core/stream/fileStream.h"
#include "shaderGen/shaderGen3/nodes/ShaderGenNode.h"

class ShaderGen3 : public SimObject
{
   typedef Parent SimObject;

   SimObjectPtr<SimXMLDocument> mXMLReader;

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

   struct Connections
   {
      U32 startNodeUID;
      U32 endNodeUID;
      U32 startSocketId;
      U32 endSocketId;
   };

   Vector<Ins> mIns;
   Vector<Outs> mOuts;
   Vector<Uniforms> mUniforms;
   Vector<Nodes> mNodes;
   Vector<Connections> mConnections;

   FileStream* outPutStream;

   Vector<ShaderGenNode*> sgNodes;

public:
   

   enum APIs
   {
      D3D11 = BIT(0),
      D3D12 = BIT(1),
      OpenGL = BIT(2),
      Metal = BIT(3),
      Vulkan = BIT(4)
   };

   enum ShaderType
   {
      VertexShader = BIT(0),
      PixelShader = BIT(1),
      ComputeShader = BIT(2),
      ShellShader = BIT(3),
      TessShader = BIT(4),
   };

public:
   ShaderGen3();
   ~ShaderGen3();

   void loadGraph(String fileName);
   void compileShaderGraph(String fileName);

   void processGraph(GenerateSettings* matSettings);

   void writeHeader(GenerateSettings* matSettings, String targetAPI);
   void writeDependencies(GenerateSettings* matSettings, String targetAPI);
   void writeINData(GenerateSettings* matSettings, String targetAPI, String targetShaderType);
   void writeOUTData(GenerateSettings* matSettings, String targetAPI, String targetShaderType);
   void writeUNIFORMSData(GenerateSettings* matSettings, String targetAPI, String targetShaderType);
   void writeMain(GenerateSettings* matSettings, String targetAPI, String targetShaderType);

   //This outputs to our script file which contains the ShaderData and CustomMaterial definitions
   void writeScriptFile(String fileName, GenerateSettings* matSettings);

   ShaderGenNode* lazyGetNodeObj(ShaderGen3::Nodes* node);

   DECLARE_CONOBJECT(ShaderGen3);
};

#endif // !SHADERGEN3