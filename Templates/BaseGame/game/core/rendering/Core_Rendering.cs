
function Core_Rendering::onCreate(%this)
{
   $Core::MissingTexturePath = "core/rendering/images/missingTexture";
   $Core::UnAvailableTexturePath = "core/rendering/images/unavailable";
   $Core::WarningTexturePath = "core/rendering/images/warnMat";
   $Core::CommonShaderPath = "core/rendering/shaders";
   
   exec("./scripts/renderManager.cs");
   exec("./scripts/gfxData/clouds.cs");
   exec("./scripts/gfxData/commonMaterialData.cs");
   exec("./scripts/gfxData/scatterSky.cs");
   exec("./scripts/gfxData/shaders.cs");
   exec("./scripts/gfxData/terrainBlock.cs");
   exec("./scripts/gfxData/water.cs");
   
   exec("./scripts/shadergen/Shadergen.cs");
   exec("./scripts/shadergen/Texture2DNode.cs");
   exec("./scripts/shadergen/floatNode.cs");
   exec("./scripts/shadergen/MultiplyNode.cs");   
   exec("./scripts/shadergen/DeferredMaterialNode.cs");
}

function Core_Rendering::onDestroy(%this)
{
}