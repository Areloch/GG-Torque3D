function DeferredMaterialNode::createNode(%shadergen)
{
   %node = %shadergen.addNode();
   
   %node.addInParam("Albedo", "float4");
   %node.addInParam("Normal", "float4");
   %node.addInParam("Metalness", "float4");
   %node.addInParam("AmbientOcclusion", "float4");
   %node.addInParam("Roughness", "float4");
   %node.addInParam("Composite", "float4");
   
   %node.addInParam("WorldPosition", "float3");
}

function DeferredMaterialNode::processVertex(%nodeIdx, %returnType, %materialFlags)
{
   ShaderGen::addVertexIN("position");
   ShaderGen::addVertexIN("normal");
   ShaderGen::addVertexIN("texCoord");
   ShaderGen::addVertexIN("T");
   ShaderGen::addVertexIN("tangentW");
   
   ShaderGen::addVertexOUT("texCoord");
   ShaderGen::addVertexOUT("screenspacePos");
   ShaderGen::addVertexOUT("vpos");
}

function DeferredMaterialNode::processPixels(%nodeIdx, %returnType, %materialFlags)
{
   %shaderOutput = "";
   
   //walk down our list of sockets, and see if we have a connection

   //Albedo map
   %node = ShaderGen::getConnection(%nodeIdx, 0);
   if(%node != -1)
   {
      %return = ShaderGen::processPixels(%node.x);

      $ShaderGenFileStream.writeLine("   OUT_color = " @ %return @ ";");
   }
   else
   {
      $ShaderGenFileStream.writeLine("   OUT_color = float4(1,1,1,1);");
   }
   
   //Normal Map
   %node = ShaderGen::getConnection(%nodeIdx, 1);
   if(%node != -1)
   {      
      %return = ShaderGen::processPixels(%node.x);

      $ShaderGenFileStream.writeLine("   OUT_norm = " @ %return @ ";");
   }
   else
   {
      $ShaderGenFileStream.writeLine("   OUT_norm = float4(1,1,1,1);");
   }
}