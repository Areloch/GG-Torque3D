function AssetBrowser::createShapeAsset(%this)
{
   %moduleName = AssetBrowser.newAssetSettings.moduleName;
   %modulePath = "data/" @ %moduleName;
      
   %assetName = AssetBrowser.newAssetSettings.assetName;
   
   %tamlpath = %modulePath @ "/shapes/" @ %assetName @ ".asset.taml";
   %shapeFilePath = %modulePath @ "/shapes/" @ %assetName @ ".dae";
   
   %asset = new ShapeAsset()
   {
      AssetName = %assetName;
      versionId = 1;
      friendlyName = AssetBrowser.newAssetSettings.friendlyName;
      description = AssetBrowser.newAssetSettings.description;
      fileName = %shapeFilePath;
   };
   
   TamlWrite(%asset, %tamlpath);
   
   Canvas.popDialog(AssetBrowser_newComponentAsset);
	
	%moduleDef = ModuleDatabase.findModule(%moduleName, 1);
	AssetDatabase.addDeclaredAsset(%moduleDef, %tamlpath);

	AssetBrowser.loadFilters();
	
	%treeItemId = AssetBrowserFilterTree.findItemByName(%moduleName);
	%smItem = AssetBrowserFilterTree.findChildItemByName(%treeItemId, "ShapeAsset");
	
	AssetBrowserFilterTree.onSelect(%smItem);
	
	return %tamlpath;
}

function AssetBrowser::editShapeAsset(%this, %assetDef)
{
   %this.hideDialog();
   ShapeEditorPlugin.openShapeAsset(%assetDef);    
}

function AssetBrowser::importShapeAsset(%this, %assetItem)
{
   %moduleName = ImportAssetModuleList.getText();
   
   %assetType = %assetItem.AssetType;
   %filePath = %assetItem.filePath;
   %assetName = %assetItem.assetName;
   %assetImportSuccessful = false;
   %assetId = %moduleName@":"@%assetName;
   
   %assetPath = "data/" @ %moduleName @ "/Shapes";
   %assetFullPath = %assetPath @ "/" @ fileName(%filePath);
   
   %newAsset = new ShapeAsset()
   {
      assetName = %assetName;
      versionId = 1;
      fileName = %assetFullPath;
      originalFilePath = %filePath;
      isNewShape = true;
   };
   
   //check dependencies
   %importItem = ImportAssetTree.findItemByObjectId(%assetItem);
   if(ImportAssetTree.isParentItem(%importItem))
   {
        %matSlotId = 0;
        %childId = ImportAssetTree.getChild(%importItem);
        while(%childId > 0)
        {
            %dependencyAssetItem = ImportAssetTree.getItemObject(%childId);
            
            %depAssetType = %dependencyAssetItem.assetType;
            if(%depAssetType $= "Material")
            {
               %matSet = "%newAsset.materialSlot"@%matSlotId@"=\"@Asset="@%moduleName@":"@%dependencyAssetItem.assetName@"\";";
               eval(%matSet);
            }
            if(%depAssetType $= "Animation")
            {
               %matSet = "%newAsset.animationSequence"@%matSlotId@"=\"@Asset="@%moduleName@":"@%dependencyAssetItem.assetName@"\";";
               eval(%matSet);
            }
            
            %childId = ImportAssetTree.getNextSibling(%childId);  
            %matSlotId++;
        }
   }
   
   %assetImportSuccessful = TAMLWrite(%newAsset, %assetPath @ "/" @ %assetName @ ".asset.taml"); 
   
   //and copy the file into the relevent directory
   %doOverwrite = !AssetBrowser.isAssetReImport;
   if(!pathCopy(%filePath, %assetFullPath, %doOverwrite))
   {
      error("Unable to import asset: " @ %filePath);
   }
   
   %constructor = ShapeEditor.findConstructor( %assetFullPath );
   
   if(!isObject(%constructor))
      %constructor = ShapeEditor.createConstructor(%assetFullPath);
   
   //We'll update any relevent bits to the ShapeConstructor here
   $TSShapeConstructor::neverImportMat = "";
   
   if(ImportAssetWindow.activeImportConfig.IgnoreMaterials !$= "")
   {
      %ignoredMatNamesCount = getTokenCount(ImportAssetWindow.activeImportConfig.IgnoreMaterials, ",;");
      for(%i=0; %i < %ignoredMatNamesCount; %i++)
      {
         if(%i==0)
            $TSShapeConstructor::neverImportMat = getToken(ImportAssetWindow.activeImportConfig.IgnoreMaterials, ",;", %i);
         else
            $TSShapeConstructor::neverImportMat = $TSShapeConstructor::neverImportMat TAB getToken(ImportAssetWindow.activeImportConfig.IgnoreMaterials, ",;", %i);
      }
   }  
   
   %constructor.neverImportMat = $TSShapeConstructor::neverImportMat;
   ShapeEditor.saveConstructor( %constructor );
   
   //now, force-load the file if it's collada
   %fileExt = fileExt(%assetFullPath);
   if(isSupportedFormat(getSubStr(%fileExt,1)))
   {
      %tempShape = new TSStatic()
      {
         shapeName = %assetFullPath;
      };
      
      %tempShape.delete();
   }
   
   %moduleDef = ModuleDatabase.findModule(%moduleName,1);
         
   if(!AssetBrowser.isAssetReImport)
      AssetDatabase.addDeclaredAsset(%moduleDef, %assetPath @ "/" @ %assetName @ ".asset.taml");
   else
      AssetDatabase.refreshAsset(%assetId);
}

function GuiInspectorTypeShapeAssetPtr::onControlDropped( %this, %payload, %position )
{
   Canvas.popDialog(EditorDragAndDropLayer);
   
   // Make sure this is a color swatch drag operation.
   if( !%payload.parentGroup.isInNamespaceHierarchy( "AssetPreviewControlType_AssetDrop" ) )
      return;

   %assetType = %payload.dragSourceControl.parentGroup.assetType;
   
   if(%assetType $= "ShapeAsset")
   {
      echo("DROPPED A SHAPE ON A SHAPE ASSET COMPONENT FIELD!");  
      
      %module = %payload.dragSourceControl.parentGroup.moduleName;
      %asset = %payload.dragSourceControl.parentGroup.assetName;
      
      %targetComponent = %this.ComponentOwner;
      %targetComponent.MeshAsset = %module @ ":" @ %asset;
      
      //Inspector.refresh();
   }
   
   EWorldEditor.isDirty= true;
}