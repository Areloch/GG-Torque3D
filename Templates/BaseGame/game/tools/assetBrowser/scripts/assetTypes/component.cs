function AssetBrowser::createComponentAsset(%this)
{
   %moduleName = AssetBrowser.newAssetSettings.moduleName;
   %modulePath = "data/" @ %moduleName;
      
   %assetName = AssetBrowser.newAssetSettings.assetName;
   
   %tamlpath = %modulePath @ "/components/" @ %assetName @ ".asset.taml";
   %scriptPath = %modulePath @ "/components/" @ %assetName @ ".cs";
   
   %asset = new ComponentAsset()
   {
      AssetName = %assetName;
      versionId = 1;
      componentName = %assetName;
      componentClass = AssetBrowser.newAssetSettings.parentClass;
      friendlyName = AssetBrowser.newAssetSettings.friendlyName;
      componentType = AssetBrowser.newAssetSettings.componentGroup;
      description = AssetBrowser.newAssetSettings.description;
      scriptFile = %scriptPath;
   };
   
   TamlWrite(%asset, %tamlpath);
   
   %file = new FileObject();
	
	if(%file.openForWrite(%scriptPath))
	{
		//TODO: enable ability to auto-embed a header for copyright or whatnot
	   %file.writeline("//onAdd is called when the component is created and then added to it's owner entity.\n");
	   %file.writeline("//You would also add any script-defined component fields via addComponentField().\n");
		%file.writeline("function " @ %assetName @ "::onAdd(%this)\n{\n\n}\n");
		%file.writeline("//onAdd is called when the component is removed and deleted from it's owner entity.");
		%file.writeline("function " @ %assetName @ "::onRemove(%this)\n{\n\n}\n");
		%file.writeline("//onClientConnect is called any time a new client connects to the server.");
		%file.writeline("function " @ %assetName @ "::onClientConnect(%this, %client)\n{\n\n}\n");
		%file.writeline("//onClientDisconnect is called any time a client disconnects from the server.");
		%file.writeline("function " @ %assetName @ "::onClientDisonnect(%this, %client)\n{\n\n}\n");
		%file.writeline("//update is called when the component does an update tick.\n");
		%file.writeline("function " @ %assetName @ "::Update(%this)\n{\n\n}\n");
		
		%file.close();
	}
	
	Canvas.popDialog(AssetBrowser_newComponentAsset);
	
	%moduleDef = ModuleDatabase.findModule(%moduleName, 1);
	AssetDatabase.addDeclaredAsset(%moduleDef, %tamlpath);

	AssetBrowser.loadFilters();
	
	%treeItemId = AssetBrowserFilterTree.findItemByName(%moduleName);
	%smItem = AssetBrowserFilterTree.findChildItemByName(%treeItemId, "Components");
	
	AssetBrowserFilterTree.onSelect(%smItem);
	
	return %tamlpath;
}

function AssetBrowser::editComponentAsset(%this, %assetId)
{
   %assetDef = AssetDatabase.acquireAsset(EditAssetPopup.assetId);
   %scriptFile = %assetDef.scriptFile;
   
   EditorOpenFileInTorsion(makeFullPath(%scriptFile), 0);
}

function AssetBrowser::duplicateComponentAsset(%this, %assetId)
{
   
}

//not used
function AssetBrowser::importComponentAsset(%this, %assetId)
{
   
}

function AssetBrowser::buildComponentAssetPreview(%this, %assetDef, %previewData)
{
   %previewData.assetName = %assetDef.assetName;
   %previewData.assetPath = %assetDef.scriptFile;
   %previewData.doubleClickCommand = "EditorOpenFileInTorsion( "@%previewData.assetPath@", 0 );";
   
   %previewData.previewImage = "tools/assetBrowser/art/componentIcon";
   
   %previewData.assetFriendlyName = %assetDef.friendlyName;
   %previewData.assetDesc = %assetDef.description;
   %previewData.tooltip = %assetDef.friendlyName @ "\n" @ %assetDef;
}