#include "shaderGen/shaderGen3/shaderGen3.h"
#include "core/strings/stringUnit.h"
#include "shaderGen/shaderGen3/ShaderGenMaterialSettings.h"
#include "shaderGen/shaderGen3/nodes/DeferredMaterialNode.h"
#include "shaderGen/shaderGen3/nodes/FloatNode.h"
#include "shaderGen/shaderGen3/nodes/Float3Node.h"
//#include "shaderGen/shaderGen3/nodes/MultiplyNode.h"

IMPLEMENT_CONOBJECT(ShaderGen3);

ShaderGen3::ShaderGen3()
{
   /*mSettings.TargetAPIs.push_back("D3D11");
   mSettings.TargetAPIs.push_back("OpenGL");

   mSettings.TargetShaders.push_back("VertexShader");
   mSettings.TargetShaders.push_back("PixelShader");*/
}

ShaderGen3::~ShaderGen3()
{

}

//This is super hacky, but whatever, cleanup later
ShaderGenNode* ShaderGen3::lazyGetNodeObj(ShaderGen3::Nodes* node)
{
   if(node->type.equal("DeferredMaterialNode"))
   {
      DeferredMaterialNode* newNode = new DeferredMaterialNode();
      sgNodes.push_back(newNode);
      return newNode;
   }
   else if (node->type.equal("Texture2DNode"))
   {
      return nullptr;
   }
   else if (node->type.equal("FloatNode"))
   {
      FloatNode* newNode = new FloatNode();
      sgNodes.push_back(newNode);
      newNode->mUniformName = node->name;
      return newNode;
   }
   else if (node->type.equal("Float3Node"))
   {
      Float3Node* newNode = new Float3Node();
      sgNodes.push_back(newNode);
      newNode->mUniformName = node->name;
      return newNode;
   }
   else //if (node->type.equal("MultiplyNode"))
   {
      return nullptr;
   }
}

//assetdatabase.acquireasset("testmodule:newmaterial")

void ShaderGen3::loadGraph(String fileName)
{
   if (!mXMLReader)
   {
      SimXMLDocument *xmlrdr = new SimXMLDocument();
      xmlrdr->registerObject();

      mXMLReader = xmlrdr;
   }

   mIns.clear();
   mOuts.clear();
   mUniforms.clear();
   mNodes.clear();
   mConnections.clear();

   if (Platform::isFile(fileName))
   {
      //use our xml reader to parse the file!
      SimXMLDocument *reader = mXMLReader.getObject();
      if (!reader->loadFile(fileName))
         Con::errorf("Could not load shader graph file: %s", fileName.c_str());

      reader->pushChildElement(0);

      //Nodes
      U32 nodeCount = 0;
      reader->pushFirstChildElement("Nodes");
      while (reader->pushChildElement(nodeCount))
      {
         Nodes newNode;

         newNode.type = reader->elementValue();
         newNode.name = reader->attribute("name");
         newNode.UID = dAtoi(reader->attribute("UID"));
         newNode.position.x = dAtoi(StringUnit::getUnit(reader->attribute("pos"), 0, " "));
         newNode.position.y = dAtoi(StringUnit::getUnit(reader->attribute("pos"), 1, " "));
         nodeCount++;

         mNodes.push_back(newNode);

         reader->popElement();
      }

      reader->popElement();

      //Connections
      U32 connCount = 0;
      reader->pushFirstChildElement("Connections");
      while (reader->pushChildElement(connCount))
      {
         Connections newConn;

         newConn.startNodeUID = dAtoi(reader->attribute("startNodeUid"));
         newConn.endNodeUID = dAtoi(reader->attribute("endNodeUid"));
         newConn.startSocketId = dAtoi(reader->attribute("startSocket"));
         newConn.endSocketId = dAtoi(reader->attribute("endSocket"));
         connCount++;

         mConnections.push_back(newConn);

         reader->popElement();
      }

      reader->popElement();

      reader->popElement();
   }
}

void ShaderGen3::compileShaderGraph(String fileName)
{
   loadGraph(fileName);

   Vector<String> targetAPIs;
   targetAPIs.push_back("D3D11");

   Torque::Path graphFile = fileName;

   String outputFilePath = graphFile.getPath();
   String outputFileName = graphFile.getFileName();
   String scriptFileExt = "cs";

   GenerateSettings matSettings;
   matSettings.outputScriptFile = outputFilePath + "/" + outputFileName + "." + scriptFileExt;
   matSettings.doubleSided = true;
   matSettings.translucent = true;
   matSettings.blendOp = "AddAlpha";
   matSettings.translucentBlendOp = "AddAlpha";
   matSettings.Name = outputFileName;

   for (U32 i = 0; i < targetAPIs.size(); ++i)
   {
      String shaderFileExt = targetAPIs[i].equal("D3D11") ? "hlsl" : "glsl";
      
      matSettings.TargetAPI = targetAPIs[i];
      matSettings.TargetMaterial = ProtoMaterial();
      
      matSettings.outputVertexFile = outputFilePath + "/" + outputFileName + "V." + shaderFileExt;
      matSettings.outputPixelFile = outputFilePath + "/" + outputFileName + "P." + shaderFileExt;

      ProtoMaterial::Ins tempIn;
      tempIn.name = "Testaroo";
      tempIn.type = "Sampler2D";
      tempIn.target = "Register0";
      matSettings.TargetMaterial.mPixelInputs.push_back(tempIn);

      processGraph(&matSettings);

      //Vertex Shader
      outPutStream = new FileStream();
      if (!outPutStream->open(matSettings.outputVertexFile, Torque::FS::File::Write))
      {
         AssertFatal(false, "Failed to open Shader Stream");
         return;
      }

      writeHeader(&matSettings, "D3D11");
      writeDependencies(&matSettings, "D3D11");
      writeINData(&matSettings, "D3D11", "VertexShader");
      writeOUTData(&matSettings, "D3D11", "VertexShader");
      writeUNIFORMSData(&matSettings, "D3D11", "VertexShader");
      writeMain(&matSettings, "D3D11", "VertexShader");

      outPutStream->close();

      //Pixel Shader
      outPutStream = new FileStream();
      if (!outPutStream->open(matSettings.outputPixelFile, Torque::FS::File::Write))
      {
         AssertFatal(false, "Failed to open Shader Stream");
         return;
      }

      writeHeader(&matSettings, "D3D11");
      writeDependencies(&matSettings, "D3D11");
      writeINData(&matSettings, "D3D11", "PixelShader");
      writeOUTData(&matSettings, "D3D11", "PixelShader");
      writeUNIFORMSData(&matSettings, "D3D11", "PixelShader");
      writeMain(&matSettings, "D3D11", "PixelShader");

      outPutStream->close();
   }

   writeScriptFile(fileName, &matSettings);
}

void ShaderGen3::writeHeader(GenerateSettings* matSettings, String targetAPI)
{
   if (targetAPI == String("D3D11"))
   {
      outPutStream->writeLine((const U8*)"//*****************************************************************************");
      outPutStream->writeLine((const U8*)"// Torque 3D -- HLSL shader");
      outPutStream->writeLine((const U8*)"//*****************************************************************************");
      outPutStream->writeLine((const U8*)"");
   }
   else if (targetAPI == String("OpenGL"))
   {
      outPutStream->writeLine((const U8*)"//*****************************************************************************");
      outPutStream->writeLine((const U8*)"// Torque 3D -- GLSL shader");
      outPutStream->writeLine((const U8*)"//*****************************************************************************");
      outPutStream->writeLine((const U8*)"");
   }
}

void ShaderGen3::writeDependencies(GenerateSettings* matSettings, String targetAPI)
{
   if (targetAPI == String("D3D11"))
   {
      outPutStream->writeLine((const U8*)"// Dependencies:");
      outPutStream->writeLine((const U8*)"#include \"shaders/common/torque.hlsl\"");
      outPutStream->writeLine((const U8*)"");
   }
   else if (targetAPI == String("OpenGL"))
   {
      outPutStream->writeLine((const U8*)"// Dependencies:");
      outPutStream->writeLine((const U8*)"#include \"shaders/common/glsl.hlsl\"");
      outPutStream->writeLine((const U8*)"");
   }
}

void ShaderGen3::writeINData(GenerateSettings* matSettings, String targetAPI, String targetShaderType)
{
   if (targetAPI == String("D3D11"))
   {
      outPutStream->writeLine((const U8*)"struct ConnectData");
      outPutStream->writeLine((const U8*)"{");

      if (targetShaderType.equal("VertexShader"))
      {
         /*for (U32 i = 0; i < matSettings->TargetMaterial.mVertexInputs.size(); i++)
         {
            StringBuilder inString;
            inString.format("   %s %s   : %s;", mIns[i].type.c_str(), mIns[i].name.c_str(), mIns[i].target.c_str());

            outPutStream->writeLine((const U8*)inString.data());
         }*/
      }
      else if (targetShaderType.equal("PixelShader"))
      {
         for (U32 i = 0; i < matSettings->TargetMaterial.mPixelInputs.size(); i++)
         {
            StringBuilder inString;
            inString.format("   %s %s   : %s;", matSettings->TargetMaterial.mPixelInputs[i].type.c_str(), 
               matSettings->TargetMaterial.mPixelInputs[i].name.c_str(), matSettings->TargetMaterial.mPixelInputs[i].target.c_str());

            outPutStream->writeLine((const U8*)inString.data());
         }
      }

      outPutStream->writeLine((const U8*)"};");
      outPutStream->writeLine((const U8*)"");
   }
   else if (targetAPI == String("OpenGL"))
   {
      for (U32 i = 0; i < mIns.size(); i++)
      {
         StringBuilder inString;
         inString.format("in %s IN_%s;", mIns[i].type.c_str(), mIns[i].name.c_str(), mIns[i].target.c_str());

         outPutStream->writeLine((const U8*)inString.data());
      }
      outPutStream->writeLine((const U8*)"");
   }
}

void ShaderGen3::writeOUTData(GenerateSettings* matSettings, String targetAPI, String targetShaderType)
{
   if (targetAPI == String("D3D11"))
   {
      outPutStream->writeLine((const U8*)"struct Fragout");
      outPutStream->writeLine((const U8*)"{");

      for (U32 i = 0; i < mOuts.size(); i++)
      {
         StringBuilder outString;
         outString.format("   %s %s   : %s;", mOuts[i].type.c_str(), mOuts[i].name.c_str(), mOuts[i].target.c_str());

         outPutStream->writeLine((const U8*)outString.data());
      }

      outPutStream->writeLine((const U8*)"};");
      outPutStream->writeLine((const U8*)"");
   }
   else if (targetAPI == String("OpenGL"))
   {
      for (U32 i = 0; i < mOuts.size(); i++)
      {
         StringBuilder outString;
         outString.format("out %s OUT_%s;", mOuts[i].type.c_str(), mOuts[i].name.c_str(), mOuts[i].target.c_str());

         outPutStream->writeLine((const U8*)outString.data());
      }
      outPutStream->writeLine((const U8*)"");
   }
}

void ShaderGen3::writeUNIFORMSData(GenerateSettings* matSettings, String targetAPI, String targetShaderType)
{
   if (targetAPI == String("D3D11"))
   {
      if (targetShaderType.equal("VertexShader"))
      {
         for (U32 i = 0; i < matSettings->TargetMaterial.mVertexUniforms.size(); i++)
         {
            StringBuilder uniString;
            uniString.format("uniform %s %s;", matSettings->TargetMaterial.mVertexUniforms[i].type.c_str(), matSettings->TargetMaterial.mVertexUniforms[i].name.c_str());
            outPutStream->writeLine((const U8*)uniString.data());
         }
      }
      else if (targetShaderType.equal("PixelShader"))
      {
         for (U32 i = 0; i < matSettings->TargetMaterial.mPixelUniforms.size(); i++)
         {
            StringBuilder uniString;

            if (String::ToLower(matSettings->TargetMaterial.mPixelUniforms[i].type).equal("sampler1d"))
            {
               uniString.format("TORQUE_UNIFORM_SAMPLER1D(%s, %s);", matSettings->TargetMaterial.mPixelUniforms[i].name.c_str(), matSettings->TargetMaterial.mPixelUniforms[i].registerName.c_str());
            }
            else if (String::ToLower(matSettings->TargetMaterial.mPixelUniforms[i].type).equal("sampler2d"))
            {
               uniString.format("TORQUE_UNIFORM_SAMPLER2D(%s, %s);", matSettings->TargetMaterial.mPixelUniforms[i].name.c_str(), matSettings->TargetMaterial.mPixelUniforms[i].registerName.c_str());
            }
            else if (String::ToLower(matSettings->TargetMaterial.mPixelUniforms[i].type).equal("sampler3d"))
            {
               uniString.format("TORQUE_UNIFORM_SAMPLER3D(%s, %s);", matSettings->TargetMaterial.mPixelUniforms[i].name.c_str(), matSettings->TargetMaterial.mPixelUniforms[i].registerName.c_str());
            }
            else if (String::ToLower(matSettings->TargetMaterial.mPixelUniforms[i].type).equal("samplercube"))
            {
               uniString.format("TORQUE_UNIFORM_SAMPLERCUBELOD(%s, %s);", matSettings->TargetMaterial.mPixelUniforms[i].name.c_str(), matSettings->TargetMaterial.mPixelUniforms[i].registerName.c_str());
            }
            else
            {
               uniString.format("uniform %s %s;", matSettings->TargetMaterial.mPixelUniforms[i].type.c_str(), matSettings->TargetMaterial.mPixelUniforms[i].name.c_str());
            }

            outPutStream->writeLine((const U8*)uniString.data());
         }
      }
      outPutStream->writeLine((const U8*)"");
   }
   /*else if (targetAPI == String("OpenGL"))
   {
      for (U32 i = 0; i < mUniforms.size(); i++)
      {
         StringBuilder uniString;
         uniString.format("out %s OUT_%s;", mUniforms[i].type, mUniforms[i].name, mUniforms[i].target);

         outPutStream->writeLine(uniString.data);
      }
      outPutStream->writeLine("");
   }*/
}

void ShaderGen3::writeMain(GenerateSettings* matSettings, String targetAPI, String targetShaderType)
{
   outPutStream->writeLine((const U8*)"//*****************************************************************************");
   outPutStream->writeLine((const U8*)"// Main");
   outPutStream->writeLine((const U8*)"//*****************************************************************************");
   outPutStream->writeLine((const U8*)"");

   if (targetAPI == String("D3D11"))
   {
      outPutStream->writeLine((const U8*)"float4 main( ConnectData IN )");
      outPutStream->writeLine((const U8*)"{");
      outPutStream->writeLine((const U8*)"   Fragout OUT;");
      outPutStream->writeLine((const U8*)"");
   }

   //get the root node, and process it
   ShaderGenNode* rootNode = lazyGetNodeObj(&mNodes[0]);

   //String outPut = rootNode->processPixels(&matSettings);

   //outPutStream->write(outPut);

   outPutStream->writeLine((const U8*)"}");
}

void ShaderGen3::processGraph(GenerateSettings* matSettings)
{
   ShaderGenNode* rootNode = lazyGetNodeObj(&mNodes[0]);
   rootNode->processPixels(matSettings);

   ShaderGenNode* rootNode2 = lazyGetNodeObj(&mNodes[1]);
   rootNode2->processPixels(matSettings);
}

void ShaderGen3::writeScriptFile(String fileName, GenerateSettings* matSettings)
{
   FileStream* fs = new FileStream();
   if (!fs->open(matSettings->outputScriptFile, Torque::FS::File::Write))
   {
      AssertFatal(false, "Failed to open script file stream");
      return;
   }

   StringBuilder shaderDataLine;
   shaderDataLine.format("singleton ShaderData(%sShader)", matSettings->Name.c_str());

   fs->writeLine((const U8*)shaderDataLine.data());
   fs->writeLine((const U8*)"{");

   StringBuilder dxVShaderLine;
   dxVShaderLine.format("   DXVertexShaderFile = \"%s\";", matSettings->outputVertexFile.c_str());
   fs->writeLine((const U8*)dxVShaderLine.data());
   StringBuilder dxPShaderLine;
   dxPShaderLine.format("   DXPixelShaderFile = \"%s\";", matSettings->outputPixelFile.c_str());
   fs->writeLine((const U8*)dxPShaderLine.data());
   fs->writeLine((const U8*)"");
   fs->writeLine((const U8*)"   OGLVertexShaderFile = \"\";");
   fs->writeLine((const U8*)"   OGLPixelShaderFile = \"\";");
   fs->writeLine((const U8*)"");

   for (U32 i = 0; i < matSettings->TargetMaterial.mPixelInputs.size(); i++)
   {
      if (matSettings->TargetMaterial.mPixelInputs[i].type == String("Sampler1D") ||
         matSettings->TargetMaterial.mPixelInputs[i].type == String("Sampler2D") ||
         matSettings->TargetMaterial.mPixelInputs[i].type == String("Sampler3D") ||
         matSettings->TargetMaterial.mPixelInputs[i].type == String("SamplerCube"))
      {
         StringBuilder samplerLine;
         samplerLine.format("   samplerNames[%i] = \"$%s\";", i, matSettings->TargetMaterial.mPixelInputs[i].name.c_str());
         fs->writeLine((const U8*)samplerLine.data());
      }
   }

   fs->writeLine((const U8*)"};");
   fs->writeLine((const U8*)"");

   StringBuilder matDataLine;
   matDataLine.format("singleton CustomMaterial(%s)", matSettings->Name.c_str());
 
   fs->writeLine((const U8*)matDataLine.data());
   fs->writeLine((const U8*)"{");

   StringBuilder targetShaderDataLine;
   targetShaderDataLine.format("   shader = %sShader;", matSettings->Name.c_str(), "data/blah.png");

   fs->writeLine((const U8*)targetShaderDataLine.data());
   if(matSettings->doubleSided)
      fs->writeLine((const U8*)"   doubleSided = true;");
   if (matSettings->translucent)
   {
      fs->writeLine((const U8*)"   translucent = true;");
      fs->writeLine((const U8*)"   blendOp = BlendOp;");
      fs->writeLine((const U8*)"   translucentBlendOp = BlendOp;");
   }

   fs->writeLine((const U8*)"");

   for (U32 i = 0; i < matSettings->TargetMaterial.mPixelInputs.size(); i++)
   {
      if (matSettings->TargetMaterial.mPixelInputs[i].type == String("Sampler1D") ||
         matSettings->TargetMaterial.mPixelInputs[i].type == String("Sampler2D") ||
         matSettings->TargetMaterial.mPixelInputs[i].type == String("Sampler3D") ||
         matSettings->TargetMaterial.mPixelInputs[i].type == String("SamplerCube"))
      {
         StringBuilder samplerLine;
         samplerLine.format("   sampler[\"%s\"] = \"%s\";", matSettings->TargetMaterial.mPixelInputs[i].name.c_str(), "data/blah.png");
         fs->writeLine((const U8*)samplerLine.data());
      }
   }
   fs->writeLine((const U8*)"};");

   fs->close();
}
DefineConsoleFunction(getShaderGen, S32, (), ,
   "Adds two rotations together.\n"
   "@param a Rotation one."
   "@param b Rotation two."
   "@returns v sum of both rotations."
   "@ingroup Math")
{
   ShaderGen3* sg;
   if (!Sim::findObject("TheShaderGen", sg))
   {
      sg = new ShaderGen3();
      sg->registerObject("TheShaderGen");
   }

   return sg->getId();
   return 0;
}

DefineConsoleMethod(ShaderGen3, loadShaderGraph, bool, (String fileName), (""), "")
{
   if (fileName == String::EmptyString)
   {
      Con::errorf("ShaderGen::loadShaderGraph: invalid graph file!");
      return false;
   }

   object->loadGraph(fileName);
   return true;
}

DefineConsoleMethod(ShaderGen3, compileShaderGraph, bool, (String fileName), (""), "")
{
   if (fileName == String::EmptyString)
   {
      Con::errorf("ShaderGen::compileShaderGraph: invalid graph file!");
      return false;
   }

   object->compileShaderGraph(fileName);
   return true;
}