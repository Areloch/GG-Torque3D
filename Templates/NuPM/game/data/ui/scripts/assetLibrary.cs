function loadAssetsPage()
{
   getOrUpdateAssetData();
   
   PanelGrid.clear();
   
   parseAssetLibrary();
   
   updateNewPanelBtn(); 
   
   $Mode = "Assets";
}

function getOrUpdateAssetData()
{
   if(!isObject(AssetLibrary))
   {
      %al = new ArrayObject(AssetLibrary);  
   }
   
   %assetLibraryPath = "data/Assets/assetLibrary";
   if(isFile(%assetLibraryPath @ "/Assets.xml"))
   {
      %path = makeFullPath(%assetLibraryPath);
      
      %gitCommand = "git pull \"https://github.com/Torque3D/AssetLibrary\" \"" @ %path @ "\"";
      
      warn("Running git command");
      warn(%gitCommand);
      %result = systemCommand(%gitCommand, "");
      
      if(%result != 0)
      {
         error("Something went wrong when pulling update to Asset Library. Please check terminal log. We got an error code of: " @ %result);  
      }
   }
   else
   {
      //don't have it, so do a clean pull
      if(!isDirectory(%assetLibraryPath))
      {
         if(!createPath(%assetLibraryPath))
         {
            error("Unable to make Assets path. Please review file permissions and try again");
            return;  
         }
      }
      
      %path = makeFullPath(%assetLibraryPath);
      
      %gitCommand = "git clone \"https://github.com/Torque3D/AssetLibrary\" \"" @ %path @ "\"";
      
      warn("Running git command");
      warn(%gitCommand);
      %result = systemCommand(%gitCommand, "");
      
      if(%result != 0)
      {
         error("Something went wrong when getting Asset Library. Please check terminal log. We got an error code of: " @ %result);  
      }
   }
}

function AssetBorderButton::onRightClick(%this)
{
   echo("OH NO, THERE GOES TOKYO");  
}

function parseAssetLibrary()
{
   %assetLibraryPath = "data/Assets/assetLibrary";
   if(!isFile(%assetLibraryPath @ "/Assets.xml"))
   {
      error("No asset library file found");
      return;
   }
   
   %xml = new SimXMLDocument();
   %loaded = %xml.loadFile(%assetLibraryPath @ "/Assets.xml");
   %xml.pushFirstChildElement("AssetLibrary");
   %xml.pushFirstChildElement("Assets");
   
   //Now we iterate out and get our assets list
   %configCount = 0;
   while(%xml.pushChildElement(%configCount))
   {
      %libItem = makeAssetLibraryItem();
      
      %libItem.AssetName = %xml.attribute("Name");
      %libItem.Creator = %xml.attribute("Creator");
      %libItem.assetType = %xml.attribute("AssetType");
      %libItem.tags = %xml.attribute("Tags");
      %libItem.description = %xml.attribute("Description");
      %libItem.repoURL = %xml.attribute("RepoURL");
      %libItem.issuesURL = %xml.attribute("IssuesURL");
      %libItem.PpreviewImageURL = %xml.attribute("PreviewImageURL");
      %libItem.engineVersionTarget = %xml.attribute("EngineVersionTarget");
      
      echo("Found Asset:" SPC %libItem.AssetName SPC %libItem.Creator SPC %libItem.assetType SPC %libItem.tags);
      
      %xml.popElement();
      %configCount++;
   }
   
   %xml.popElement();
   %xml.popElement();
   
   //Now, update our UI listing
   for(%i=0; %i < AssetLibrary.count(); %i++)
   {
      %assetItem = AssetLibrary.getKey(%i);
      
      %btn = buildAssetPanel(%assetItem.AssetName, "Asset");
      
      PanelGrid.add(%btn);
   }
}

function makeAssetLibraryItem()
{
   %libItem = new ScriptObject();
   AssetLibrary.add(%libItem);
   
   return %libItem;
}