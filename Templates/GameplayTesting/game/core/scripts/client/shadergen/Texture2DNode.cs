function Texture2DNode::processPixels(%nodeIdx, %returnType, %modifierFlags)
{
   %textureName = ShaderGen::getNodeName(%nodeIdx);
   %return = "diffuseColor";
   
   if($Shadergen::API $= "D3D9" || $Shadergen::API $= "D3D11")
      %texCoord = "IN.texCoord";
   else
      %texCoord = "IN_texCoord";
      
   if(%modifierFlags & $ShaderGen::CompileFlags::NormalMap)
      $ShaderGenFileStream.writeLine("   vec4 " @ %return @ " = tex2D(" @ %textureName @ ", " @ %texCoord @ ");");
   else
      $ShaderGenFileStream.writeLine("   vec4 " @ %return @ " = toLinear(tex2D(" @ %textureName @ ", " @ %texCoord @ "));");
   
   return %return;
}

function Texture2DNode::getPixelReference(%nodeIdx, %returnType)
{
   return ShaderGen::getNodeName(%nodeIdx);
}