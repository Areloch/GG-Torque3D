function GameObjectPackageList::onWake(%this)
{
   %this.refresh();
}

function GameObjectPackageList::refresh(%this)
{
   %this.clear();
   
   //First, get our list of modules
   %moduleList = ModuleDatabase.findModules();
   
   %count = getWordCount(%moduleList);
   for(%i=0; %i < %count; %i++)
   {
      %moduleName = getWord(%moduleList, %i);
      %this.add(%moduleName.ModuleId, %i);  
   }
}

function GameObjectCreatorPkgBtn::onClick(%this)
{
   Canvas.pushDialog(AssetBrowser_AddPackage);
   AssetBrowser_addPackageWindow.selectWindow();
}

function GameObjectCreateBtn::onClick(%this)
{
   %className = GameObjectCreatorName.getText();
   
   if(%className $= "")
	{
		error("Attempted to make a new Game Object with no name!");
		Canvas.popDialog(GameObjectCreator);
		return;
	}
	
	//First, find out if this one already exists. If so, we're obviously merely updating it
	//also, exec any components that may exist
   //find all GameObjectAssets
   %assetQuery = new AssetQuery();
   if(!AssetDatabase.findAssetType(%assetQuery, "GameObjectAsset"))
      return; //if we didn't find ANY, just exit
      
   %count = %assetQuery.getCount();
   
   %createNew = true;
   
	for(%i=0; %i < %count; %i++)
	{
	   %assetId = %assetQuery.getAsset(%i);
      
      %gameObjectAsset = AssetDatabase.acquireAsset(%assetId);
      
      if(%gameObjectAsset.gameObjectName $= %className)
      {
         %createNew = false;
         break;
      }
	}
	
	%selectedEntity = GameObjectCreator.selectedEntity;

	%selectedEntity.class = %className;
	%selectedEntity.gameObjectAsset = %className;
	Inspector.inspect(%selectedEntity);
	
	if(%createNew)
	{
      //get the selected module data
      %moduleName = GameObjectPackageList.getText();
      
      %path = "modules/" @ %moduleName @ "/gameObjects/";
      
      %file = new FileObject();
      
      if(%file.openForWrite(%path @ "\\" @ %className @ ".cs"))
      {
         %file.writeline("function " @ %className @ "::onAdd(%this)\n{\n\n}\n");
         %file.writeline("function " @ %className @ "::onRemove(%this)\n{\n\n}\n");
         
         //todo, pre-write any event functions of interest
         
         %file.close();
      }
      
      //set up the paths
      %tamlPath = %path  @ %className @ ".taml";
      %scriptPath = %path  @ %className @ ".cs";
      saveGameObject(%className, %tamlPath, %scriptPath);
      
      %asset = new GameObjectAsset()
      {
         AssetId = %className @ "Asset";
         AssetName = %className;
         VersionId = 1;
      };
      %assetPath = %path  @ %className @ ".asset.taml";
      
      //now, add the script file and a ref to the taml into our SGO manifest so we can readily spawn it later.
      TamlWrite(%selectedEntity, %tamlpath);
      TamlWrite(%asset, %assetPath);

      GameObjectCreator.selectedEntity = "";
      
      Canvas.popDialog(GameObjectCreator);
      
      //Load it
      %moduleDef = ModuleDatabase.findModule(%moduleName,1);
      AssetDatabase.addDeclaredAsset(%moduleDef, %assetPath);
	}
	else
	{
	   %moduleDef = AssetDatabase.getAssetModule(%assetId);
	   %moduleName = %moduleDef.ModuleId;
	   %path = "modules/" @ %moduleName @ "/gameObjects/";
	   
	   %tamlPath = %path  @ %className @ ".taml";
	   TamlWrite(%selectedEntity, %tamlpath);
	   
	   GameObjectCreator.selectedEntity = "";
      
      Canvas.popDialog(GameObjectCreator);
	}
}