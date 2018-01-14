//
function AssetBrowser::CreateNewModule(%this)
{
   Canvas.pushDialog(AssetBrowser_AddModule); 
   AssetBrowser_addModuleWindow.selectWindow();  
   
   AssetBrowser_addModuleWindow.callbackFunction = "AssetBrowser.loadFilters();";
}

function AssetBrowser_editModule::saveModule(%this)
{
   //Check what special actions we may need to do, such as renames
   %moduleDef = ModuleDatabase.findModule(AssetBrowser.selectedModule, 1);
   
   %oldModuleName = %moduleDef.ModuleID;
   
   if(%oldModuleName !$= AssetBrowser.tempModule.ModuleID)
   {      
      //rename the script file and script namespaces
      %oldScriptFilePath = "data/" @ %oldModuleName @ "/" @ %moduleDef.scriptFile;
      %newscriptFilePath = "data/" @ AssetBrowser.tempModule.ModuleID @ "/";
      %scriptExt = fileExt(%moduleDef.scriptFile);
      
      %newScriptFileName = %newscriptFilePath @ "/" @ AssetBrowser.tempModule.ModuleID @ %scriptExt;
      
      %oldModuleFile = "data/" @ %oldModuleName @ "/" @ %oldModuleName @ ".module";
      %newModuleFile = "data/" @ AssetBrowser.tempModule.ModuleID @ "/" @ AssetBrowser.tempModule.ModuleID @ ".module";
      
      //renaming
      ModuleDatabase.renameModule(%moduleDef, AssetBrowser.tempModule.ModuleID);
      
      %moduleDef.ModuleId = AssetBrowser.tempModule.ModuleID;
      %moduleDef.scriptFile = strreplace(%moduleDef.scriptFile, %oldModuleName, AssetBrowser.tempModule.ModuleID);
      
      TamlWrite(%moduleDef, %newModuleFile);
      fileDelete(%oldModuleFile);
      
      pathCopy(%oldScriptFilePath, %newScriptFileName);
      fileDelete(%oldScriptFilePath);
      
      //Go through our scriptfile and replace the old namespace with the new
      %editedFileContents = "";
      
      %file = new FileObject();
      if ( %file.openForRead( %newScriptFileName ) ) 
      {
         while ( !%file.isEOF() ) 
         {
            %line = %file.readLine();
            %line = trim( %line );
            
            %editedFileContents = %editedFileContents @ strreplace(%line, %oldModuleName, AssetBrowser.tempModule.ModuleID) @ "\n";
         }
         
         %file.close();
      }
      
      if(%editedFileContents !$= "")
      {
         %file.openForWrite(%newScriptFileName);
         
         %file.writeline(%editedFileContents);
         
         %file.close();
      }
      
      //Now, relocate all asset files
      for( %file = findFirstFile( "data/"@%oldModuleName@"/*" );
         %file !$= "";
         %file = findNextFile( "data/"@%oldModuleName@"/*" ))
      {
         if(%file $= %oldScriptFilePath || %file $= %oldModuleFile)
            continue;
            
         %path = filePath(%file);
         %base = fileBase(%file);
         %ext = fileExt(%file);
         
         %newPath = strreplace(%path, %oldModuleName, AssetBrowser.tempModule.ModuleID);
            
         %newFile = %newPath @ "/" @ %base @ %ext;
         
         if(!IsDirectory(%newPath))
         {
            createPath(%newFile);
         }

         pathCopy(%file, %newFile);
         fileDelete(%file);
      }
      
      exec(%newScriptFileName);
   }
   
   ModuleDatabase.unloadExplicit(%oldModuleName);
   ModuleDatabase.loadExplicit(AssetBrowser.tempModule.ModuleID);
   
   //Now, update the module file itself
   //%file = ModuleDatabase.getAssetFilePath(%moduleDef.ModuleID);
   //%success = TamlWrite(AssetBrowser_editAsset.editedAsset, %file);
   
   AssetBrowser.loadFilters();

   Canvas.popDialog(AssetBrowser_editModule);
}

function AssetBrowser::editModuleInfo(%this)
{
   Canvas.pushDialog(AssetBrowser_editModule); 
   
   %moduleDef = ModuleDatabase.findModule(AssetBrowser.selectedModule, 1);
   
   AssetBrowser.tempModule = new ModuleDefinition();
   AssetBrowser.tempModule.assignFieldsFrom(%moduleDef);
   
   ModuleEditInspector.inspect(AssetBrowser.tempModule);  
   AssetBrowser_editModule.editedModuleId = AssetBrowser.selectedModule;
   AssetBrowser_editModule.editedModule = AssetBrowser.tempModule;
   
   //remove some of the groups we don't need:
   for(%i=0; %i < ModuleEditInspector.getCount(); %i++)
   {
      %caption = ModuleEditInspector.getObject(%i).caption;
      
      if(%caption $= "BuildId" || %caption $= "type" || %caption $= "Dependencies" || %caption $= "scriptFile" 
         || %caption $= "AssetTagsManifest" || %caption $= "ScopeSet" || %caption $= "ModulePath" 
         || %caption $= "ModuleFile" || %caption $= "ModuleFilePath" || %caption $= "ModuleScriptFilePath"  )
      {
         ModuleEditInspector.remove(ModuleEditInspector.getObject(%i));
         %i--;
      }
   }
}

function AssetBrowser::renameModule(%this)
{
   
}

function AssetBrowser::reloadModule(%this)
{
   ModuleDatabase.unregisterModule(AssetBrowser.SelectedModule, 1);
   ModuleDatabase.loadExplicit(AssetBrowser.SelectedModule);
}

function AssetBrowser::deleteModule(%this)
{
   
}