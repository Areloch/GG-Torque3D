function AssetBrowser_editAsset::saveAsset(%this)
{
   %file = AssetDatabase.getAssetFilePath(%this.editedAssetId);
   %success = TamlWrite(AssetBrowser_editAsset.editedAsset, %file);
   
   AssetBrowser.loadFilters();

   Canvas.popDialog(AssetBrowser_editAsset);
}

function AssetNameField::onReturn(%this)
{
   //if the name is different to the asset's original name, rename it!
   %newName = %this.getText();
   if(%this.originalAssetName !$= %this.getText())
   {
      %moduleName = AssetBrowser.selectedModule;
      
      //do a rename!
      %success = AssetDatabase.renameDeclaredAsset(%moduleName @ ":" @ %this.originalAssetName, %moduleName @ ":" @ %this.getText());
      
      if(%success)
      {
         %newAssetId = %moduleName @ ":" @ %this.getText();
         %assetPath = AssetDatabase.getAssetFilePath(%newAssetId);
         
         //Rename any associated files as well
         %assetDef = AssetDatabase.acquireAsset(%newAssetId);
         %assetType = %assetDef.getClassName();
         
         //rename the file to match
         %path = filePath(%assetPath);
         
         //%newFile = %path @ "/" @ %this.getText() @ ".asset.taml";
         //pathCopy(%assetPath, %newFile);
         //fileDelete(%assetPath);
         
         if(%assetType $= "ComponentAsset")
         {
            %oldScriptFilePath = %assetDef.scriptFile;
            %scriptFilePath = filePath(%assetDef.scriptFile);
            %scriptExt = fileExt(%assetDef.scriptFile);
            
            %newScriptFileName = %scriptFilePath @ "/" @ %newName @ %scriptExt;
            %newAssetFile = %path @ "/" @ %this.getText() @ ".asset.taml";
            
            %assetDef.componentName = %newName;
            %assetDef.scriptFile = %newScriptFileName;
            
            TamlWrite(%assetDef, %newAssetFile);
            fileDelete(%assetPath);
            
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
                  
                  %editedFileContents = %editedFileContents @ strreplace(%line, %this.originalAssetName, %newName) @ "\n";
		         }
		         
		         %file.close();
            }
            
            if(%editedFileContents !$= "")
            {
               %file.openForWrite(%newScriptFileName);
               
               %file.writeline(%editedFileContents);
               
               %file.close();
            }
            
            exec(%newScriptFileName);
         }
         else if(%assetType $= "StateMachineAsset")
         {
            %oldScriptFilePath = %assetDef.stateMachineFile;
            %scriptFilePath = filePath(%assetDef.stateMachineFile);
            %scriptExt = fileExt(%assetDef.stateMachineFile);
            
            %newScriptFileName = %scriptFilePath @ "/" @ %newName @ %scriptExt;
            %newAssetFile = %path @ "/" @ %this.getText() @ ".asset.taml";
            
            %assetDef.stateMachineFile = %newScriptFileName;
            
            TamlWrite(%assetDef, %newAssetFile);
            fileDelete(%assetPath);
            
            pathCopy(%oldScriptFilePath, %newScriptFileName);
            fileDelete(%oldScriptFilePath);
         }
      }
   }
   
   %this.clearFirstResponder();
   %this.setActive(false);
}

function AssetBrowser::editAsset(%this)
{
   //Find out what type it is
   %assetDef = AssetDatabase.acquireAsset(EditAssetPopup.assetId);
   %assetType = %assetDef.getClassName();
   
   if(%assetType $= "MaterialAsset")
   {
      if(EditorSettings.materialEditMode $= "MaterialEditor")
      {
         %assetDef.materialDefinitionName.reload();
         
         EditorGui.setEditor(MaterialEditorPlugin);
         
         MaterialEditorGui.currentMaterial = %assetDef.materialDefinitionName;
         MaterialEditorGui.setActiveMaterial( %assetDef.materialDefinitionName );
         
         AssetBrowser.hideDialog();
      }
      else
      {
         Canvas.pushDialog(ShaderEditor); 
         ShaderEditorGraph.loadGraph(%assetDef.shaderGraph);
         $ShaderGen::targetShaderFile = filePath(%assetDef.shaderGraph) @"/"@fileBase(%assetDef.shaderGraph);
      }
   }
   else if(%assetType $= "StateMachineAsset")
   {
      eval("AssetBrowser.tempAsset = new " @ %assetDef.getClassName() @ "();");
      AssetBrowser.tempAsset.assignFieldsFrom(%assetDef);
      
      SMAssetEditInspector.inspect(AssetBrowser.tempAsset);  
      AssetBrowser_editAsset.editedAssetId = EditAssetPopup.assetId;
      AssetBrowser_editAsset.editedAsset = AssetBrowser.tempAsset;
      
      //remove some of the groups we don't need:
      for(%i=0; %i < SMAssetEditInspector.getCount(); %i++)
      {
         %caption = SMAssetEditInspector.getObject(%i).caption;
         
         if(%caption $= "Ungrouped" || %caption $= "Object" || %caption $= "Editing" 
            || %caption $= "Persistence" || %caption $= "Dynamic Fields")
         {
            SMAssetEditInspector.remove(SMAssetEditInspector.getObject(%i));
            %i--;
         }
      }
   
      Canvas.pushDialog(StateMachineEditor);
      StateMachineEditor.loadStateMachineAsset(EditAssetPopup.assetId);
      StateMachineEditor-->Window.text = "State Machine Editor ("@EditAssetPopup.assetId@")";
   }
   else if(%assetType $= "ComponentAsset")
   {
      %assetDef = AssetDatabase.acquireAsset(EditAssetPopup.assetId);
      %scriptFile = %assetDef.scriptFile;
      
      EditorOpenFileInTorsion(makeFullPath(%scriptFile), 0);
   }
   else if(%assetType $= "GameObjectAsset")
   {
      %assetDef = AssetDatabase.acquireAsset(EditAssetPopup.assetId);
      %scriptFile = %assetDef.scriptFilePath;
      
      EditorOpenFileInTorsion(makeFullPath(%scriptFile), 0);
   }
   else if(%assetType $= "ScriptAsset")
   {
      %assetDef = AssetDatabase.acquireAsset(EditAssetPopup.assetId);
      %scriptFile = %assetDef.scriptFilePath;
      
      EditorOpenFileInTorsion(makeFullPath(%scriptFile), 0);
   }
   else if(%assetType $= "ShapeAsset")
   {
      %this.hideDialog();
      ShapeEditorPlugin.openShapeAsset(EditAssetPopup.assetId);  
   }
   else if(%assetType $= "ShapeAnimationAsset")
   {
      %this.hideDialog();
      ShapeEditorPlugin.openShapeAsset(EditAssetPopup.assetId);  
   }
   else if(%assetType $= "LevelAsset")
   {
      schedule( 1, 0, "EditorOpenMission", %assetDef.LevelFile);
   }
   else if(%assetType $= "GUIAsset")
   {
      if(!isObject(%assetDef.assetName))
      {
         exec(%assetDef.GUIFilePath);
         exec(%assetDef.mScriptFilePath);
      }
      
      GuiEditContent(%assetDef.assetName);
   }
}

function AssetBrowser::editAssetInfo(%this)
{
   Canvas.pushDialog(AssetBrowser_editAsset); 
   
   %assetDef = AssetDatabase.acquireAsset(EditAssetPopup.assetId);
   
   eval("AssetBrowser.tempAsset = new " @ %assetDef.getClassName() @ "();");
   AssetBrowser.tempAsset.assignFieldsFrom(%assetDef);
   
   AssetEditInspector.inspect(AssetBrowser.tempAsset);  
   AssetBrowser_editAsset.editedAssetId = EditAssetPopup.assetId;
   AssetBrowser_editAsset.editedAsset = AssetBrowser.tempAsset;
   
   //remove some of the groups we don't need:
   for(%i=0; %i < AssetEditInspector.getCount(); %i++)
   {
      %caption = AssetEditInspector.getObject(%i).caption;
      
      if(%caption $= "Ungrouped" || %caption $= "Object" || %caption $= "Editing" 
         || %caption $= "Persistence" || %caption $= "Dynamic Fields")
      {
         AssetEditInspector.remove(AssetEditInspector.getObject(%i));
         %i--;
      }
   }
}

function AssetBrowser::renameAsset(%this)
{
   //Find out what type it is
   %assetDef = AssetDatabase.acquireAsset(EditAssetPopup.assetId);
   
   %curFirstResponder = AssetBrowser.getFirstResponder();
   
   if(%curFirstResponder != 0)
      %curFirstResponder.clearFirstResponder();
   
   AssetBrowser.selectedAssetPreview-->AssetNameLabel.setActive(true);
   AssetBrowser.selectedAssetPreview-->AssetNameLabel.setFirstResponder();
}

function AssetBrowser::duplicateAsset(%this)
{
   %assetDef = AssetDatabase.acquireAsset(EditAssetPopup.assetId);
   
   %this.setupCreateNewAsset(%assetDef.getClassName(), AssetBrowser.selectedModule);
}

function AssetBrowser::deleteAsset(%this)
{
   //Find out what type it is
   %assetDef = AssetDatabase.acquireAsset(EditAssetPopup.assetId);
   %assetType = %assetDef.getClassName();
   
   MessageBoxOKCancel("Warning!", "This will delete the selected asset and the files associated to it, do you wish to continue?", 
      "confirmDeleteAsset", "");
      
   %this.confirmDeleteAsset();
}

function AssetBrowser::confirmDeleteAsset(%this)
{
   AssetDatabase.deleteAsset(EditAssetPopup.assetId, false);
   %this.loadFilters();
}
