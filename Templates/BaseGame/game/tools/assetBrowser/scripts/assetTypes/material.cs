function AssetBrowser::createMaterialAsset(%this)
{
   %assetName = AssetBrowser.newAssetSettings.assetName;
   
   %moduleName = AssetBrowser.newAssetSettings.moduleName;
   %modulePath = "data/" @ %moduleName;
   
   %tamlpath = %modulePath @ "/materials/" @ %assetName @ ".asset.taml";
   %sgfPath = %modulePath @ "/materials/" @ %assetName @ ".sgf";
   
   %asset = new MaterialAsset()
   {
      AssetName = %assetName;
      versionId = 1;
      shaderData = "";
      shaderGraph = %sgfPath;
   };
   
   TamlWrite(%asset, %tamlpath);
   
   %moduleDef = ModuleDatabase.findModule(%moduleName, 1);
	AssetDatabase.addDeclaredAsset(%moduleDef, %tamlpath);

	AssetBrowser.loadFilters();
	
	%treeItemId = AssetBrowserFilterTree.findItemByName(%moduleName);
	%smItem = AssetBrowserFilterTree.findChildItemByName(%treeItemId, "Materials");
	
	AssetBrowserFilterTree.onSelect(%smItem);
   
	return %tamlpath;
}

function AssetBrowser::buildMaterialAssetPreview(%this, %assetDef, %previewData)
{
   %previewData.assetName = %assetDef.materialDefinitionName;
   %previewData.assetPath = %assetDef.scriptFile;

   //Lotta prepwork
   %previewData.doubleClickCommand = %assetDef@".materialDefinitionName.reload(); "
                                   @ "$Tools::materialEditorList = \"\";"
                                   @ "EWorldEditor.clearSelection();"
                                   @ "MaterialEditorGui.currentObject = 0;"
                                   @ "MaterialEditorGui.currentMode = \"asset\";"
                                   @ "MaterialEditorGui.currentMaterial = "@%assetDef@".materialDefinitionName;"
                                   @ "MaterialEditorGui.setActiveMaterial( "@%assetDef@".materialDefinitionName );"
                                   @ "EditorGui.setEditor(MaterialEditorPlugin); "
                                   @ "AssetBrowser.hideDialog();";
   
   if(isFile(%assetDef.materialDefinitionName.diffuseMap[0]))
      %previewData.previewImage = %assetDef.materialDefinitionName.diffuseMap[0];
   else
      %previewData.previewImage = "tools/assetBrowser/art/materialIcon";
   
   %previewData.assetFriendlyName = %assetDef.assetName;
   %previewData.assetDesc = %assetDef.description;
   %previewData.tooltip = %assetDef.friendlyName @ "\n" @ %assetDef;
}