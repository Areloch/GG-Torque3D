function ShaderGen::init()
{
   $Shadergen::targetShaderFile = "";
   $ShaderGen = new SimObject();
   
   $ShaderGen.inCount = 0;
   $ShaderGen.outCount = 0;
   $ShaderGen.uniformCount = 0;
   $ShaderGen.nodeCount = 0;
   $ShaderGen.connCount = 0;
}

function ShaderGen::getConnection(%nodeIdx, %inSocket)
{
   for(%i = 0; %i < $ShaderGen.connCount; %i++)
	{
	   %startNode = getField($ShaderGen.CONNECTIONS[%i], 0);
	   %endNode = getField($ShaderGen.CONNECTIONS[%i], 1);
	   %startSocket = getField($ShaderGen.CONNECTIONS[%i], 2);
	   %endSocket = getField($ShaderGen.CONNECTIONS[%i], 3);
	   
	   if(%endNode == %nodeIdx && %inSocket == %endSocket)
	   {
	      return %startNode SPC %startSocket;
	   }
	}
	
	return -1;
}

function ShaderGen::getNodeType(%nodeIdx)
{
   if($ShaderGen.nodeCount >= %nodeIdx && %nodeIdx >= 0)
      return getField($ShaderGen.NODES[%nodeIdx], 0);
      
   return "";
}

function ShaderGen::getNodeName(%nodeIdx)
{
   if($ShaderGen.nodeCount >= %nodeIdx && %nodeIdx >= 0)
      return getField($ShaderGen.NODES[%nodeIdx], 1);
      
   return "";
}

function ShaderGen::processPixels(%nodeIdx, %returnType)
{
   if($ShaderGen.nodeCount >= %nodeIdx && %nodeIdx >= 0)
   {
      %type = getField($ShaderGen.NODES[%nodeIdx], 0);
      %return = eval(%type @ "::processPixels(" @ %nodeIdx @ ", \"" @ %returnType @ "\");");
   }
   
   return %return;
}

function ShaderGen::getPixelReference(%nodeIdx, %returnType)
{
   if($ShaderGen.nodeCount >= %nodeIdx && %nodeIdx >= 0)
   {
      %type = getField($ShaderGen.NODES[%nodeIdx], 0);
      %return = eval(%type @ "::getPixelReference(" @ %nodeIdx @ ", \"" @ %returnType @ "\");");
   }
   
   return %return;
}

function ShaderGen::addVertexFeature()
{
   
}

function ShaderGen::addVertexIN(%inElementName)
{
   $Shadergen.VERTEXIN[$Shadergen.vertexInCount] = %inElementName;
   $Shadergen.vertexInCount++;
}

function ShaderGen::addVertexOut(%outElementName)
{
   $Shadergen.VERTEXOUT[$Shadergen.vertexOutCount] = %outElementName;
   $Shadergen.vertexOutCount++;
}

function ShaderGen::loadGraph(%file)
{
   if(!isObject($ShaderGen::FileReader))
      $ShaderGen::FileReader = new SimXMLDocument();
      
	if(!$ShaderGen::FileReader.loadFile(%file))
	{
		error("ShaderGen: Unable to read shader graph file!");
		return;
	}
	
	$ShaderGen::FileReader.pushChildElement(0);
	
	//Figure out what kind of graph this is
	$ShaderGen::FileReader.pushChildElement(0);
	
	%graphType = $ShaderGen::FileReader.elementValue();
	
	if(%graphType $= "PixelShader")
	{
	   //get our in params
	   $ShaderGen::FileReader.pushFirstChildElement("In");
	   
	   %inCount = 0;
	   while($ShaderGen::FileReader.pushChildElement(%inCount))
	   {
	      %inType = $ShaderGen::FileReader.elementValue();
	      %inName = $ShaderGen::FileReader.attribute("name");
	      %inTarget = $ShaderGen::FileReader.attribute("target");
	      
	      $ShaderGen.IN[%inCount] = %inType TAB %inName TAB %inTarget;
	      
	      $ShaderGen::FileReader.popElement();
	      %inCount++;      
	   }
	   $ShaderGen.inCount = %inCount;
	   
	   $ShaderGen::FileReader.popElement();
	   
	   //now, our outs
	   $ShaderGen::FileReader.pushFirstChildElement("Out");
	   
	   %outCount = 0;
	   while($ShaderGen::FileReader.pushChildElement(%outCount))
	   {
	      %outType = $ShaderGen::FileReader.elementValue();
	      %outName = $ShaderGen::FileReader.attribute("name");
	      %outTarget = $ShaderGen::FileReader.attribute("target");
	      
	      $ShaderGen.OUT[%outCount] = %outType TAB %outName TAB %outTarget;
	      
	      $ShaderGen::FileReader.popElement();
	      %outCount++;      
	   }
	   $ShaderGen.outCount = %outCount;
	   
	   $ShaderGen::FileReader.popElement();
	   
	   //Next, our uniforms
	   $ShaderGen::FileReader.pushFirstChildElement("Uniforms");
	   
	   %uniformCount = 0;
	   while($ShaderGen::FileReader.pushChildElement(%uniformCount))
	   {
	      %uniType = $ShaderGen::FileReader.elementValue();
	      %uniName = $ShaderGen::FileReader.attribute("name");
	      %uniRegister = $ShaderGen::FileReader.attribute("register");
	      
	      $ShaderGen.UNIFORM[%uniformCount] = %uniType TAB %uniName TAB %uniRegister;
	      
	      $ShaderGen::FileReader.popElement();
	      %uniformCount++;      
	   }
	   $ShaderGen.uniformCount = %uniformCount;
	   
	   $ShaderGen::FileReader.popElement();
	   
	   //Our nodes
	   $ShaderGen::FileReader.pushFirstChildElement("Nodes");
	   
	   %nodeCount = 0;
	   while($ShaderGen::FileReader.pushChildElement(%nodeCount))
	   {
	      %nodeType = $ShaderGen::FileReader.elementValue();
	      %nodeName = $ShaderGen::FileReader.attribute("name");
	      %nodeUID = $ShaderGen::FileReader.attribute("uid");
	      %nodePos = $ShaderGen::FileReader.attribute("pos");
	      
	      $ShaderGen.NODES[%nodeCount] = %nodeType TAB %nodeName TAB %nodeUID TAB %nodePos;
	      
	      //catch the end-node
	      if(%nodeType $= "DeferredMaterialNode")
	         $Shadergen.endNode = %nodeCount;
	      
	      $ShaderGen::FileReader.popElement();
	      %nodeCount++;      
	   }
	   $ShaderGen.nodeCount = %nodeCount;
	   
	   $ShaderGen::FileReader.popElement();
	   
	   //And lastly, our connections
	   $ShaderGen::FileReader.pushFirstChildElement("Connections");
	   
	   %connCount = 0;
	   while($ShaderGen::FileReader.pushChildElement(%connCount))
	   {
	      %connStartNodeUID = $ShaderGen::FileReader.attribute("startNodeUid");
	      %connEndNodeUID = $ShaderGen::FileReader.attribute("endNodeUid");
	      %connStartSoc = $ShaderGen::FileReader.attribute("startSocket");
	      %connEndSoc = $ShaderGen::FileReader.attribute("endSocket");
	      
	      $ShaderGen.CONNECTIONS[%connCount] = %connStartNodeUID TAB %connEndNodeUID TAB %connStartSoc TAB %connEndSoc;
	      
	      $ShaderGen::FileReader.popElement();
	      %connCount++;
	   }
	   $ShaderGen.connCount = %connCount;
	   
	   $ShaderGen::FileReader.popElement();
	}	
	
	$ShaderGen::FileReader.popElement();
	
	$ShaderGen::FileReader.popElement();
	
	//Now, add nodes and populate the graph
	/*for(%i = 0; %i < $ShaderGen.nodeCount; %i++)
	{
	   %type = getField($ShaderGen.NODES[%i], 0);
	   %pos = getField($ShaderGen.NODES[%i], 3);
	   
	   %pos.x = mRound(%this.position.x + %this.extent.x / 2 + %pos.x);
	   %pos.y = mRound(%this.position.y + %this.extent.y / 2 + %pos.y);
	   
	   if(%type $= "Texture2DNode")
	   {
	      %newNode = %this.addTextureNode();
	      %this.setNodePosition(%i, %pos);
	   }
	   else if(%type $= "floatNode")
	   {
	      %newNode = %this.addFloatNode();
	      %this.setNodePosition(%i, %pos);
	   }
	   else if(%type $= "DeferredMaterialNode")
	   {
	      %newNode = %this.addPBRMatEndNode();
	      %this.setNodePosition(%i, %pos);
	      %this.endNode = %i;
	   }
	}
	
	for(%i = 0; %i < $ShaderGen.connCount; %i++)
	{
	   %startNode = getField($ShaderGen.CONNECTIONS[%i], 0);
	   %endNode = getField($ShaderGen.CONNECTIONS[%i], 1);
	   %startSocket = getField($ShaderGen.CONNECTIONS[%i], 2);
	   %endSocket = getField($ShaderGen.CONNECTIONS[%i], 3);
	   
	   %this.addConnection(%startNode, %startSocket, %endNode, %endSocket);
	}*/
}

function ShaderGen::compileShader(%shaderGraphFile)
{
   if(!isObject($ShaderGenFileStream))
      $ShaderGenFileStream = new FileStreamObject();
   
   //HLSL
   $Shadergen::API = "D3D9";
   //Vertex Shader
   $ShaderGenFileStream.open(%shaderGraphFile @ "V.hlsl", "Write");
   
   ShaderGen::writeHeader();
   ShaderGen::writeDependencies();
   ShaderGen::writeVertINData();
   ShaderGen::writeVertOUTData();
   ShaderGen::writeVertUNIFORMSData();
   ShaderGen::writeVertSecondaryFunctions();
   ShaderGen::writeVertMain();
   
   $ShaderGenFileStream.close();
   
   //Pixel Shader
   $ShaderGenFileStream.open(%shaderGraphFile @ "P.hlsl", "Write");
   
   ShaderGen::writeHeader();
   ShaderGen::writeDependencies();
   ShaderGen::writeINData();
   ShaderGen::writeOUTData();
   ShaderGen::writeUNIFORMSData();
   ShaderGen::writeSecondaryFunctions();
   ShaderGen::writeMain();
   
   $ShaderGenFileStream.close();
   
   $Shadergen::API = "OpenGL";
   $ShaderGenFileStream.open(%shaderGraphFile @ "P.glsl", "Write");
   
   ShaderGen::writeHeader();
   ShaderGen::writeDependencies();
   ShaderGen::writeINData();
   ShaderGen::writeOUTData();
   ShaderGen::writeUNIFORMSData();
   ShaderGen::writeSecondaryFunctions();
   ShaderGen::writeMain();
   
   $ShaderGenFileStream.close();
}

function ShaderGen::writeHeader(%this)
{
   $ShaderGenFileStream.writeLine("//*****************************************************************************");
   
   if($Shadergen::API $= "D3D9" || $Shadergen::API $= "D3D11")
      $ShaderGenFileStream.writeLine("// Torque -- HLSL procedural shader");
   else
      $ShaderGenFileStream.writeLine("// Torque -- GLSL procedural shader");
      
   $ShaderGenFileStream.writeLine("//*****************************************************************************");
   $ShaderGenFileStream.writeLine("");
}

function ShaderGen::writeDependencies(%this)
{
   $ShaderGenFileStream.writeLine("// Dependencies:");
   
   if($Shadergen::API $= "D3D9" || $Shadergen::API $= "D3D11")
   {
      $ShaderGenFileStream.writeLine("#include \"shaders/common/torque.hlsl\"");
   }
   else
   {
      $ShaderGenFileStream.writeLine("#include \"shaders/common/torque.glsl\"");
   }
   
   $ShaderGenFileStream.writeLine("");
}

function ShaderGen::writeINData(%this)
{
   if($Shadergen::API $= "D3D9" || $Shadergen::API $= "D3D11")
   {
      $ShaderGenFileStream.writeLine("struct ConnectData");
      $ShaderGenFileStream.writeLine("{");
      
      for(%i=0; %i < $ShaderGen.inCount; %i++)
      {
         %type = getField($ShaderGen.IN[%i], 0);
         %name = getField($ShaderGen.IN[%i], 1);
         %target = getField($ShaderGen.IN[%i], 2);
         
         $ShaderGenFileStream.writeLine("   " @ %type SPC %name TAB ":" SPC %target @ ";");
      }
      
      $ShaderGenFileStream.writeLine("};");
      $ShaderGenFileStream.writeLine("");
   }
   else
   {
      for(%i=0; %i < $ShaderGen.inCount; %i++)
      {
         %type = getField($ShaderGen.IN[%i], 0);
         %name = getField($ShaderGen.IN[%i], 1);
         %target = getField($ShaderGen.IN[%i], 2);
         
         $ShaderGenFileStream.writeLine("in " @ %type @ " IN_" @ %name @ ";");
      }

      $ShaderGenFileStream.writeLine("");
   }
}

function ShaderGen::writeOUTData(%this)
{
   if($Shadergen::API $= "D3D9" || $Shadergen::API $= "D3D11")
   {
      $ShaderGenFileStream.writeLine("struct Fragout");
      $ShaderGenFileStream.writeLine("{");
      
      for(%i=0; %i < $ShaderGen.outCount; %i++)
      {
         %type = getField($ShaderGen.OUT[%i], 0);
         %name = getField($ShaderGen.OUT[%i], 1);
         %target = getField($ShaderGen.OUT[%i], 2);
         
         $ShaderGenFileStream.writeLine("   " @ %type SPC %name TAB ":" SPC %target @ ";");
      }
      
      $ShaderGenFileStream.writeLine("};");
      $ShaderGenFileStream.writeLine("");
   }
   else
   {
      for(%i=0; %i < $ShaderGen.outCount; %i++)
      {
         %type = getField($ShaderGen.OUT[%i], 0);
         %name = getField($ShaderGen.OUT[%i], 1);
         %target = getField($ShaderGen.OUT[%i], 2);
         
         $ShaderGenFileStream.writeLine("out " @ %type SPC "OUT_" @ %name @ ";");
      }

      $ShaderGenFileStream.writeLine("");
   }
}

function ShaderGen::writeUNIFORMSData(%this)
{
   for(%i=0; %i < $ShaderGen.uniformCount; %i++)
   {
      %type = getField($ShaderGen.UNIFORM[%i], 0);
      %name = getField($ShaderGen.UNIFORM[%i], 1);
      %target = getField($ShaderGen.UNIFORM[%i], 2);
      
      if($Shadergen::API $= "D3D9" || $Shadergen::API $= "D3D11")
      {
         if(%type $= "Sampler1D" || %type $= "Sampler2D" || %type $= "Sampler3D" || %type $= "SamplerCube" )
            $ShaderGenFileStream.writeLine("TORQUE_UNIFORM_SAMPLER2D("@%name@","@%target@");");
         else
            $ShaderGenFileStream.writeLine("uniform " @ %type SPC %name @ ";");
      }
      else
      {
         $ShaderGenFileStream.writeLine("uniform " @ %type SPC %name @ ";");
      }
   }

   $ShaderGenFileStream.writeLine("");
}

function ShaderGen::writeMain(%this)
{
   $ShaderGenFileStream.writeLine("//-----------------------------------------------------------------------------");
   $ShaderGenFileStream.writeLine("// Main");
   $ShaderGenFileStream.writeLine("//-----------------------------------------------------------------------------");
   
   if($Shadergen::API $= "D3D9" || $Shadergen::API $= "D3D11")
   {
      $ShaderGenFileStream.writeLine("float4 main( ConnectData IN )");
   }
   else
   {
      $ShaderGenFileStream.writeLine("void main( )");
   }
   
   $ShaderGenFileStream.writeLine("{");
   
   if($Shadergen::API $= "D3D9" || $Shadergen::API $= "D3D11")
   {
      //We have to declare our out in d3d
      $ShaderGenFileStream.writeLine("   Fragout OUT;");
      $ShaderGenFileStream.writeLine("");
   }
   
   //compiiiiiile!
   //first, get our endNode
   ShaderGen::processNode($Shadergen.endNode, $Shadergen::API, "");
   
   $ShaderGenFileStream.writeLine("}");
}

//
// Vertex Shaders
//

function ShaderGen::writeVertINData(%this)
{
   $ShaderGenFileStream.writeLine("struct VertexData");
   $ShaderGenFileStream.writeLine("{");
   
   //This needs proper fleshing out, but a combination of pixel shader hooks + flagged features should do it
   //We pretty much will always have a few of these though:
   
   if($Shadergen::API $= "D3D9" || $Shadergen::API $= "D3D11")
   {
      $ShaderGenFileStream.writeLine("   float3 position : POSITION;");
      $ShaderGenFileStream.writeLine("   float3 normal : NORMAL;");
      $ShaderGenFileStream.writeLine("   float3 texCoord : TEXCOORD0;");
      
      //Additional T and W vert features
      $ShaderGenFileStream.writeLine("   float3 T : TANGENT;");
      $ShaderGenFileStream.writeLine("   float3 tangentW : TEXCOORD3;");
      $ShaderGenFileStream.writeLine("};");
   }
   else
   {
      $ShaderGenFileStream.writeLine("   vec3 position;");
      $ShaderGenFileStream.writeLine("   vec3 normal;");
      $ShaderGenFileStream.writeLine("   vec3 texCoord;");
      
      //Additional T and W vert features
      $ShaderGenFileStream.writeLine("   vec3 T;");
      $ShaderGenFileStream.writeLine("   vec3 tangentW;");
      $ShaderGenFileStream.writeLine("} IN;");
   }

   $ShaderGenFileStream.writeLine("");
}

function ShaderGen::writeVertOUTData(%this)
{
   if($Shadergen::API $= "D3D9" || $Shadergen::API $= "D3D11")
   {
      $ShaderGenFileStream.writeLine("struct ConnectData");
      $ShaderGenFileStream.writeLine("{");
      
      for(%i=0; %i < $ShaderGen.inCount; %i++)
      {
         %type = getField($ShaderGen.IN[%i], 0);
         %name = getField($ShaderGen.IN[%i], 1);
         %target = getField($ShaderGen.IN[%i], 2);
         
         $ShaderGenFileStream.writeLine("   " @ %type SPC %name TAB ":" SPC %target @ ";");
      }
      
      $ShaderGenFileStream.writeLine("};");
      $ShaderGenFileStream.writeLine("");
   }
   else
   {
      for(%i=0; %i < $ShaderGen.inCount; %i++)
      {
         %type = getField($ShaderGen.IN[%i], 0);
         %name = getField($ShaderGen.IN[%i], 1);
         %target = getField($ShaderGen.IN[%i], 2);
         
         $ShaderGenFileStream.writeLine("out " @ %type @ " OUT_" @ %name @ ";");
      }

      $ShaderGenFileStream.writeLine("");
   }
}

function ShaderGen::writeVertUNIFORMSData(%this)
{
   for(%i=0; %i < $ShaderGen.uniformCount; %i++)
   {
      %type = getField($ShaderGen.UNIFORM[%i], 0);
      %name = getField($ShaderGen.UNIFORM[%i], 1);
      %target = getField($ShaderGen.UNIFORM[%i], 2);
      
      if($Shadergen::API $= "D3D9" || $Shadergen::API $= "D3D11")
      {
         if(%type $= "Sampler1D" || %type $= "Sampler2D" || %type $= "Sampler3D" || %type $= "SamplerCube" )
            $ShaderGenFileStream.writeLine("TORQUE_UNIFORM_SAMPLER2D("@%name@","@%target@");");
         else
            $ShaderGenFileStream.writeLine("uniform " @ %type SPC %name @ ";");
      }
      else
      {
         $ShaderGenFileStream.writeLine("uniform " @ %type SPC %name @ ";");
      }
   }

   $ShaderGenFileStream.writeLine("");
}

function ShaderGen::writeVertMain(%this)
{
   $ShaderGenFileStream.writeLine("//-----------------------------------------------------------------------------");
   $ShaderGenFileStream.writeLine("// Main");
   $ShaderGenFileStream.writeLine("//-----------------------------------------------------------------------------");
   
   if($Shadergen::API $= "D3D9" || $Shadergen::API $= "D3D11")
   {
      $ShaderGenFileStream.writeLine("float4 main( ConnectData IN )");
   }
   else
   {
      $ShaderGenFileStream.writeLine("void main( )");
   }
   
   $ShaderGenFileStream.writeLine("{");
   
   if($Shadergen::API $= "D3D9" || $Shadergen::API $= "D3D11")
   {
      //We have to declare our out in d3d
      $ShaderGenFileStream.writeLine("   Fragout OUT;");
      $ShaderGenFileStream.writeLine("");
   }
   
   //compiiiiiile!
   //Walk through the features
   $ShaderGenFileStream.writeLine("   //Position");
   $ShaderGenFileStream.writeLine("   OUT.hpos = mul(modelview, float4(IN.position.xyz,1));");
   $ShaderGenFileStream.writeLine("");
   
   $ShaderGenFileStream.writeLine("   //Base texture coords");
   $ShaderGenFileStream.writeLine("   OUT.out_texCoord = (float2)IN.texCoord;");
   $ShaderGenFileStream.writeLine("");
   
   $ShaderGenFileStream.writeLine("   //Normal mapping");
   $ShaderGenFileStream.writeLine("   float3x3 objToTangentSpace;");
   $ShaderGenFileStream.writeLine("   objToTangentSpace[0] = IN.T;");
   $ShaderGenFileStream.writeLine("   objToTangentSpace[1] = cross( IN.T, normalize(IN.normal) ) * IN.tangentW;");
   $ShaderGenFileStream.writeLine("   objToTangentSpace[2] = normalize(IN.normal);");
   $ShaderGenFileStream.writeLine("   float3x3 viewToTangent = mul( objToTangentSpace, (float3x3)viewToObj );");
   $ShaderGenFileStream.writeLine("   OUT.outViewToTangent = viewToTangent;");
   $ShaderGenFileStream.writeLine("");
   
   $ShaderGenFileStream.writeLine("   //Depth");
   $ShaderGenFileStream.writeLine("   float3 depthPos = mul( objTrans, float4( IN.position.xyz, 1 ) ).xyz;");
   $ShaderGenFileStream.writeLine("   OUT.wsEyeVec = float4( depthPos.xyz - eyePosWorld, 1 );");
   $ShaderGenFileStream.writeLine("");
   
   if($Shadergen::VertexFeatures & $Shadergen::VertexFeatures::HWSkin)
   {
      $ShaderGenFileStream.writeLine("");
   }
   
   $ShaderGenFileStream.writeLine("}");
}

//
function ShaderGen::processNode(%nodeId, %shaderType, %returnType)
{
   %nodeType = getField($Shadergen.NODES[%nodeId],0);
   
   %testStr = %nodeType @ "::processPixels( "@ %nodeId @ ", \"\");";
   
   eval(%testStr);
}

function ShaderGen::writeSecondaryFunctions(%this)
{
   $ShaderGenFileStream.writeLine("");
}

function ShaderGen::write(%this)
{
   $ShaderGenFileStream.writeLine("");
}