
/*$Pref::AssetBrowser::CurrentStaticFilter = "MaterialFilterAllArray";
$Pref::AssetBrowser::CurrentFilter = ""; //ALL
$Pref::AssetBrowser::ThumbnailCountIndex = 0;

new PersistenceManager(AssetBrowserPerMan);

new ArrayObject(UnlistedMaterials);
UnlistedMaterials.add( "unlistedMaterials", WarningMaterial );
UnlistedMaterials.add( "unlistedMaterials", materialEd_previewMaterial );
UnlistedMaterials.add( "unlistedMaterials", notDirtyMaterial );
UnlistedMaterials.add( "unlistedMaterials", materialEd_cubemapEd_cubeMapPreview );
UnlistedMaterials.add( "unlistedMaterials", matEdCubeMapPreviewMat );
UnlistedMaterials.add( "unlistedMaterials", materialEd_justAlphaMaterial );
UnlistedMaterials.add( "unlistedMaterials", materialEd_justAlphaShader ); */

new SimGroup(AssetBrowserPreviewCache);

//AssetBrowser.addToolbarButton
function AssetBrowser::addToolbarButton(%this)
{
	%filename = expandFilename("tools/worldEditor/images/toolbar/matterial-editor");
	%button = new GuiBitmapButtonCtrl() {
		canSaveDynamicFields = "0";
		internalName = AssetBrowserBtn;
		Enabled = "1";
		isContainer = "0";
		Profile = "ToolsGuiButtonProfile";
		HorizSizing = "right";
		VertSizing = "bottom";
		position = "180 0";
		Extent = "25 19";
		MinExtent = "8 2";
		canSave = "1";
		Visible = "1";
		Command = "AssetBrowser.ShowDialog();";
		tooltipprofile = "ToolsGuiToolTipProfile";
		ToolTip = "Asset Browser";
		hovertime = "750";
		bitmap = %filename;
		buttonType = "RadioButton";
		groupNum = "0";
		useMouseEvents = "0";
	};
	ToolsToolbarArray.add(%button);
	EWToolsToolbar.setExtent((25 + 8) * (ToolsToolbarArray.getCount()) + 12 SPC "33");
}
//

function AssetBrowser::onWake(%this)
{
   %this.importAssetListArray = new ArrayObject();
}

//Drag-Drop functionality

function AssetBrowser::onBeginDropFiles( %this, %fileCount )
{   
   error("% DragDrop - Beginning file dropping of" SPC %fileCount SPC " files.");
   %this.importAssetListArray.empty();
}
function AssetBrowser::onDropFile( %this, %filePath )
{
   if(!%this.isVisible())
      return;
      
   %fileExt = fileExt( %filePath );
   //add it to our array!
   if( (%fileExt $= ".png") || (%fileExt $= ".jpg") || (%fileExt $= ".bmp") || (%fileExt $= ".dds") )
      %this.importAssetListArray.add("Image", %filePath);
   else if( (%fileExt $= ".dae") || (%fileExt $= ".dts"))
      %this.importAssetListArray.add("Model", %filePath);
   else if( (%fileExt $= ".ogg") || (%fileExt $= ".wav") || (%fileExt $= ".mp3"))
      %this.importAssetListArray.add("Sound", %filePath);
      
   // Check imagemap extension
   /*%fileExt = fileExt( %filePath );
   if( (%fileExt $= ".png") || (%fileExt $= ".jpg") || (%fileExt $= ".bmp") )
      %this.onDropImageFile(%filePath);
   
   else if (%fileExt $= ".zip")
      %this.onDropZipFile(%filePath);*/
}

function AssetBrowser::onDropZipFile(%this, %filePath)
{
   if(!%this.isVisible())
      return;
      
   %zip = new ZipObject();
   %zip.openArchive(%filePath);
   %count = %zip.getFileEntryCount();
   for (%i = 0; %i < %count; %i++)
   {
      %fileEntry = %zip.getFileEntry(%i);
      %fileFrom = getField(%fileEntry, 0);
      
      // For now, if it's a .cs file, we'll assume it's a behavior.
      if (fileExt(%fileFrom) !$= ".cs")
         continue;
      
      %fileTo = expandFilename("^game/behaviors/") @ fileName(%fileFrom);
      %zip.extractFile(%fileFrom, %fileTo);
      exec(%fileTo);
   }
}

function AssetBrowser::onDropImageFile(%this, %filePath)
{
   if(!%this.isVisible())
      return;
      
   // File Information madness
   %fileName         = %filePath;
   %fileOnlyName     = fileName( %fileName );
   %fileBase         = validateDatablockName(fileBase( %fileName ) @ "ImageMap");
   
   // [neo, 5/17/2007 - #3117]
   // Check if the file being dropped is already in data/images or a sub dir by checking if
   // the file path up to length of check path is the same as check path.
   %defaultPath = EditorSettings.value( "WorldEditor/defaultMaterialsPath" );

   %checkPath    = expandFilename( "^"@%defaultPath );
   %fileOnlyPath = expandFileName( %filePath ); //filePath( expandFileName( %filePath ) );
   %fileBasePath = getSubStr( %fileOnlyPath, 0, strlen( %checkPath ) );
   
   if( %checkPath !$= %fileBasePath )
   {
      // No match so file is from outside images directory and we need to copy it in
      %fileNewLocation = expandFilename("^"@%defaultPath) @ "/" @ fileBase( %fileName ) @ fileExt( %fileName );
   
      // Move to final location
      if( !pathCopy( %filePath, %fileNewLocation ) )
         return;
   }
   else 
   {  
      // Already in images path somewhere so just link to it
      %fileNewLocation = %filePath;
   }
   
   addResPath( filePath( %fileNewLocation ) );

   %matName = fileBase( %fileName );
      
   // Create Material
   %imap = new Material(%matName)
   {
	  mapTo = fileBase( %matName );
	  diffuseMap[0] = %defaultPath @ "/" @ fileBase( %fileName ) @ fileExt( %fileName );
   };
   //%imap.setName( %fileBase );
   //%imap.imageName = %fileNewLocation;
   //%imap.imageMode = "FULL";
   //%imap.filterPad = false;
   //%imap.compile();

   %diffusecheck = %imap.diffuseMap[0];
         
   // Bad Creation!
   if( !isObject( %imap ) )
      return;
      
   %this.addDatablock( %fileBase, false );
}

function AssetBrowser::onEndDropFiles( %this, %fileCount )
{
   //error("% DragDrop - Completed file dropping");
   if(!%this.isVisible())
      return;
      
   //%this.persistToDisk( true, false, false );
   
   //we have assets to import, so go ahead and display the window for that now
   AssetBrowser_importAssetWindow.visible = true;
   AssetBrowser_ImportAssetWindow.refresh();
   AssetBrowser_importAssetWindow.selectWindow();
   
   // Update object library
   GuiFormManager::SendContentMessage($LBCreateSiderBar, %this, "refreshAll 1");
}

function AssetBrowser::selectMaterial( %this, %material )
{
   %name = "";
   
   if( AssetBrowser.terrainMaterials )
   {
      %name = %material;
      %material = TerrainMaterialSet.findObjectByInternalName( %material );
   }
   else
   {
      %name = %material.getName();
   }
   
   // The callback function should be ready to intake the returned material
   //eval("materialEd_previewMaterial." @ %propertyField @ " = " @ %value @ ";");
   if( AssetBrowser.returnType $= "name" )
      eval( "" @ AssetBrowser.selectCallback @ "(" @ %name  @ ");");
   else if( AssetBrowser.returnType $= "index" )
   {
      %index = -1;
      if( AssetBrowser.terrainMaterials )
      {
         // Obtain the index into the terrain's material list
         %mats = ETerrainEditor.getMaterials();
         for(%i = 0; %i < getRecordCount( %mats ); %i++)
         {
            %matInternalName = getRecord( %mats, %i );
            if( %matInternalName $= %name )
            {
               %index = %i;
               break;
            }
         }
      }
      else
      {
         // Obtain the index into the material set
         for(%i = 0; %i < materialSet.getCount(); %i++)
         {
            %obj = materialSet.getObject(%i);
            if( %obj.getName() $= %name )
            {
               %index = %i;
               break;
            }
         }
      }
      
      eval( "" @ AssetBrowser.selectCallback @ "(" @ %index  @ ");");
   }
   else
      eval( "" @ AssetBrowser.selectCallback @ "(" @ %material.getId()  @ ");");
   AssetBrowser.hideDialog();
}

function AssetBrowser::showDialog( %this, %AssetTypeFilter, %selectCallback, %targetObj, %fieldName, %returnType)
{
   //if( AssetBrowser.isVisible() )
   //   return;
      
   %this.showDialogBase(%AssetTypeFilter, %selectCallback, %targetObj, %fieldName, %returnType, false);
}

function AssetBrowser::showTerrainDialog( %this, %selectCallback, %returnType)
{
   %this.showDialogBase(%selectCallback, %returnType, true);
}

function AssetBrowser::showDialogBase( %this, %AssetTypeFilter, %selectCallback, %targetObj, %fieldName, %returnType)
{
   // Set the select callback
   AssetBrowser.selectCallback = %selectCallback;
   AssetBrowser.returnType = %returnType;
   AssetBrowser.assetTypeFilter = %AssetTypeFilter;
   AssetBrowser.fieldTargetObject = %targetObj;
   AssetBrowser.fieldTargetName = %fieldName;
   
   //Canvas.pushDialog(AssetBrowser);
   //EWTreeWindow.getParent().add(AssetBrowser);
   //AssetBrowser.setVisible(1);
   //AssetBrowser.buildStaticFilters();   
   
   //AssetBrowser.selectedMaterial = "";
   Canvas.add(AssetBrowser);
   AssetBrowser.selectWindow();
   AssetBrowser.setVisible(1);
   AssetBrowserWindow.setVisible(1);
   AssetBrowser_addPackageWindow.setVisible(0);
   AssetBrowser_importAssetWindow.setVisible(0);
   AssetBrowser.loadFilters();
}

function AssetBrowser::hideDialog( %this )
{
   //AssetBrowser.breakdown();
   
   AssetBrowser.setVisible(1);
   AssetBrowserWindow.setVisible(1);
   AssetBrowser_addPackageWindow.setVisible(0);
   AssetBrowser_importAssetWindow.setVisible(0);
   
   Canvas.popDialog(AssetBrowser);
}

function AssetBrowser::buildPreviewArray( %this, %asset, %moduleName )
{
   %assetName = %asset;
   %previewImage = "core/art/warnmat";
      
   // it may seem goofy why the checkbox can't be instanciated inside the container
   // reason being its because we need to store the checkbox ctrl in order to make changes
   // on it later in the function. 

   %previewSize = "80 80";
   %previewBounds = 20;
   
   %container = new GuiControl(){
      profile = "ToolsGuiDefaultProfile";
      Position = "0 0";
      Extent = %previewSize.x + %previewBounds SPC %previewSize.y + %previewBounds + 24;
      HorizSizing = "right";
      VertSizing = "bottom";
      isContainer = "1";
      assetName = %assetName;
      moduleName = %moduleName;
      
      new GuiTextCtrl(){
         position = 0 SPC %previewSize.y + %previewBounds - 16;
         profile = "ToolsGuiTextCenterProfile";
         extent = %previewSize.x + %previewBounds SPC 16;
         text = %assetName;
      };
   };
   
   %previewButton = new GuiBitmapButtonCtrl(){
      internalName = %assetName;
      HorizSizing = "right";
      VertSizing = "bottom";
      profile = "ToolsGuiButtonProfile";
      position = "10 4";
      extent = %previewSize;
      buttonType = "PushButton";
      bitmap = "";
      Command = "";
      text = "Loading...";
      useStates = false;
      
      new GuiBitmapButtonCtrl(){
            HorizSizing = "right";
            VertSizing = "bottom";
            profile = "ToolsGuiButtonProfile";
            position = "0 0";
            extent = %previewSize;
            Variable = "";
            buttonType = "toggleButton";
            bitmap = "tools/materialEditor/gui/cubemapBtnBorder";
            groupNum = "0";
            text = "";
         }; 
   }; 

   /*%previewButton = new GuiShapeEdPreview(){
      internalName = %matName;
      HorizSizing = "right";
      VertSizing = "bottom";
      Profile = "ToolsGuiDefaultProfile";
      position = "7 4";
      extent = %previewSize;
      MinExtent = "8 8";
     canSave = "1";
     Visible = "1";
     tooltipprofile = "ToolsGuiToolTipProfile";
     hovertime = "1000";
     Margin = "0 0 0 0";
     Padding = "0 0 0 0";
     AnchorTop = "1";
     AnchorBottom = "0";
     AnchorLeft = "1";
     AnchorRight = "0";
     renderMissionArea = "0";
     GizmoProfile = "GlobalGizmoProfile";
     cameraZRot = "0";
     forceFOV = "0";
     gridColor = "0 0 0 0";
     renderNodes = "0";
     renderObjBox = "0";
     renderMounts = "0";
     renderColMeshes = "0";
     selectedNode = "-1";
     sunDiffuse = "255 255 255 255";
     sunAmbient = "180 180 180 255";
     timeScale = "1.0";
     fixedDetail = "0";
        orbitNode = "0";
      
      new GuiBitmapButtonCtrl(){
            HorizSizing = "right";
            VertSizing = "bottom";
            profile = "ToolsGuiButtonProfile";
            position = "0 0";
            extent = %previewSize;
            Variable = "";
            buttonType = "toggleButton";
            bitmap = "tools/materialEditor/gui/cubemapBtnBorder";
            groupNum = "0";
            text = "";
         }; 
   };
   */
   %previewBorder = new GuiButtonCtrl(){
         internalName = %assetName@"Border";
         HorizSizing = "right";
         VertSizing = "bottom";
         profile = "ToolsGuiThumbHighlightButtonProfile";
         position = "0 0";
         extent = %previewSize.x + %previewBounds SPC %previewSize.y + 24;
         Variable = "";
         buttonType = "toggleButton";
         tooltip = %assetName;
         Command = "AssetBrowser.updateSelection( $ThisControl.getParent().assetName, $ThisControl.getParent().moduleName );"; 
		   altCommand = "EditorGui.setEditor(ShapeEditorPlugin); ShapeEditorPlugin.openShapeAsset(" @ %moduleName @":"@ %assetName @");";
         groupNum = "0";
         text = "";
   };

   //%previewButton.setModel("art/DoubleDoor.dae");
   //%previewButton.fitToShape();
   
   %container.add(%previewButton);  
   %container.add(%previewBorder); 
   // add to the gui control array
   AssetBrowser-->materialSelection.add(%container);
   
   // add to the array object for reference later
   MatEdPreviewArray.add( %previewButton, %previewImage );
}

function AssetBrowser::loadImages( %this, %materialNum )
{
   // this will save us from spinning our wheels in case we don't exist
   if( !AssetBrowser.visible )
      return;
   
   // this schedule is here to dynamically load images
   %previewButton = MatEdPreviewArray.getKey(%materialNum);
   %previewImage = MatEdPreviewArray.getValue(%materialNum);
   
   %previewButton.setBitmap(%previewImage);
   %previewButton.setText("");
   
   %materialNum++;
   
   if( %materialNum < MatEdPreviewArray.count() )
   {
      %tempSchedule = %this.schedule(64, "loadImages", %materialNum);
      MatEdScheduleArray.add( %tempSchedule, %materialNum );
   }
}

function AssetBrowser::clearMaterialFilters( %this )
{
   for( %i = AssetBrowser.staticFilterObjects; %i < AssetBrowser-->filterArray.getCount(); %i++ )
      AssetBrowser-->filterArray.getObject(%i).getObject(0).setStateOn(0);
      
   AssetBrowser.loadFilter( "", "" );
}

function AssetBrowser::loadFilters( %this )
{
   AssetBrowser-->filterTree.clear();

   AssetBrowser-->filterTree.buildIconTable(":tools/classIcons/prefab");

   AssetBrowser-->filterTree.insertItem(0, "Assets");

   %assetQuery = new AssetQuery();
   %numAssetsFound = AssetDatabase.findAllAssets(%assetQuery);
   
   for( %i=0; %i < %numAssetsFound; %i++)
   {
	    %assetId = %assetQuery.getAsset(%i);
		
		//first, get the asset's module, as our major categories
		%module = AssetDatabase.getAssetModule(%assetId);
		
		%moduleName = %module.moduleId;
		
		//check that we don't re-add it
		%moduleItemId = AssetBrowser-->filterTree.findItemByName(%moduleName);
		
		if(%moduleItemId == -1 || %moduleItemId == 0)
			%moduleItemId = AssetBrowser-->filterTree.insertItem(1, %module.moduleId, "", "", 1, 1);
			
		//now, add the asset's category
		%assetType = AssetDatabase.getAssetCategory(%assetId);
		
		if(%assetType $= "")
		{
		   %assetType = AssetDatabase.getAssetType(%assetId);
		   if(%assetType $= "")
			   %assetType = "Misc";
		}
		
		if(AssetBrowser.assetTypeFilter !$= "" && AssetBrowser.assetTypeFilter !$= %assetType)
		   continue;
			
		%typeFilterItemId = AssetBrowser-->filterTree.findItemByName(%assetType);
		
      //we can have similar asset categories under different package modules, so lets double check that
      %parentItem = AssetBrowser-->filterTree.getParentItem(%typeFilterItemId);
		
		if(%typeFilterItemId == -1 || %typeFilterItemId == 0 
		   || %parentItem != %moduleItemId)
			%typeFilterItemId = AssetBrowser-->filterTree.insertItem(%moduleItemId, %assetType);
   }

   AssetBrowser-->filterTree.buildVisibleTree(true);
}

// create category and update current material if there is one
function AssetBrowser::createFilter( %this, %filter )
{
   if( %filter $= %existingFilters )
   {
      MessageBoxOK( "Error", "Can not create blank filter.");
      return;
   }
      
   for( %i = AssetBrowser.staticFilterObjects; %i < AssetBrowser-->filterArray.getCount() ; %i++ )
   {
      %existingFilters = AssetBrowser-->filterArray.getObject(%i).getObject(0).filter;
      if( %filter $= %existingFilters )
      {
         MessageBoxOK( "Error", "Can not create two filters of the same name.");
         return;
      }
   }
   %container = new GuiControl(){
      profile = "ToolsGuiDefaultProfile";
      Position = "0 0";
      Extent = "128 18";
      HorizSizing = "right";
      VertSizing = "bottom";
      isContainer = "1";
         
      new GuiCheckBoxCtrl(){
         Profile = "ToolsGuiCheckBoxListProfile";
         position = "5 1";
         Extent = "118 18";
         Command = "";
         groupNum = "0";
         buttonType = "ToggleButton";
         text = %filter @ " ( " @ MaterialFilterAllArray.countKey(%filter) @ " )";
         filter = %filter;
         Command = "AssetBrowser.preloadFilter();";
      };
   };
   
   AssetBrowser-->filterArray.add( %container );
   
   // if selection exists, lets reselect it to refresh it
   if( isObject(AssetBrowser.selectedMaterial) )
      AssetBrowser.updateSelection( AssetBrowser.selectedMaterial, AssetBrowser.selectedPreviewImagePath );
   
   // material category text field to blank
   AssetBrowser_addFilterWindow-->tagName.setText("");
}

function AssetBrowser::updateSelection( %this, %asset, %moduleName )
{
   // the material selector will visually update per material information
   // after we move away from the material. eg: if we remove a field from the material,
   // the empty checkbox will still be there until you move fro and to the material again
   
   %isMaterialBorder = 0;
   eval("%isMaterialBorder = isObject(AssetBrowser-->"@%asset@"Border);");
   if( %isMaterialBorder )
   {
      eval( "AssetBrowser-->"@%asset@"Border.setStateOn(1);");
   }
      
   %isMaterialBorderPrevious = 0;
   eval("%isMaterialBorderPrevious = isObject(AssetBrowser-->"@$prevSelectedMaterialHL@"Border);");
   if( %isMaterialBorderPrevious )
   {
      eval( "AssetBrowser-->"@$prevSelectedMaterialHL@"Border.setStateOn(0);");
   }
   
   AssetBrowser-->materialCategories.deleteAllObjects();
   AssetBrowser.selectedMaterial = %asset;
   AssetBrowser.selectedAsset = %moduleName@":"@%asset;
   AssetBrowser.selectedPreviewImagePath = %previewImagePath;
   AssetBrowser-->previewSelectionText.setText( %asset );
   AssetBrowser-->previewSelection.setBitmap( %previewImagePath );
   
   // running through the existing list of categorynames in the left, so yes
   // some might exist on the left only temporary if not given a home
   for( %i = AssetBrowser.staticFilterObjects; %i < AssetBrowser-->filterArray.getCount() ; %i++ )
   {
      %filter = AssetBrowser-->filterArray.getObject(%i).getObject(0).filter;
      
      %checkbox = new GuiCheckBoxCtrl(){
         materialName = %material.name;
         Profile = "ToolsGuiCheckBoxListProfile";
         position = "5 2";
         Extent = "118 18";
         Command = "AssetBrowser.updateMaterialTags( $ThisControl.materialName, $ThisControl.getText(), $ThisControl.getValue() );";
         text = %filter;
      };
      
      AssetBrowser-->materialCategories.add( %checkbox );
      // crawl through material for categories in order to check or not
      %filterFound = 0;
      for( %j = 0; %material.getFieldValue("materialTag" @ %j) !$= ""; %j++ )
      {
         %tag = %material.getFieldValue("materialTag" @ %j);
         
         if( %tag  $= %filter )
         {
            %filterFound = 1;
            break;
         }
      }
      
      if( %filterFound  )
         %checkbox.setStateOn(1);
      else
         %checkbox.setStateOn(0);
   }
   
   $prevSelectedMaterialHL = %material;
}

//needs to be deleted with the persistence manager and needs to be blanked out of the matmanager
//also need to update instances... i guess which is the tricky part....
function AssetBrowser::showDeleteDialog( %this )
{
   %material = AssetBrowser.selectedMaterial;
   %secondFilter = "MaterialFilterMappedArray";
   %secondFilterName = "Mapped";
   
   for( %i = 0; %i < MaterialFilterUnmappedArray.count(); %i++ )
   {
      if( MaterialFilterUnmappedArray.getValue(%i) $= %material )
      {
         %secondFilter = "MaterialFilterUnmappedArray";
         %secondFilterName = "Unmapped";
         break;
      }
   }
   
   if( isObject( %material ) )
   {
      MessageBoxYesNoCancel("Delete Material?", 
         "Are you sure you want to delete<br><br>" @ %material.getName() @ "<br><br> Material deletion won't take affect until the engine is quit.", 
         "AssetBrowser.deleteMaterial( " @ %material @ ", " @ %secondFilter @ ", " @ %secondFilterName @" );", 
         "", 
         "" );
   }
}

function AssetBrowser::deleteMaterial( %this, %materialName, %secondFilter, %secondFilterName )
{
   if( !isObject( %materialName ) )
      return;
   
   for( %i = 0; %i <= MaterialFilterAllArray.countValue( %materialName ); %i++)
   {
      %index = MaterialFilterAllArray.getIndexFromValue( %materialName );
      MaterialFilterAllArray.erase( %index );
   }
   MaterialFilterAllArrayCheckbox.setText("All ( " @ MaterialFilterAllArray.count() - 1 @ " ) ");
   
   %checkbox = %secondFilter @ "Checkbox";
   for( %k = 0; %k <= %secondFilter.countValue( %materialName ); %k++)
   {
      %index = %secondFilter.getIndexFromValue( %materialName );
      %secondFilter.erase( %index );
   }
   %checkbox.setText( %secondFilterName @ " ( " @ %secondFilter.count() - 1 @ " ) ");
   
   for( %i = 0; %materialName.getFieldValue("materialTag" @ %i) !$= ""; %i++ )
   {
      %materialTag = %materialName.getFieldValue("materialTag" @ %i);
         
         for( %j = AssetBrowser.staticFilterObjects; %j < AssetBrowser-->filterArray.getCount() ; %j++ )
         {
            if( %materialTag $= AssetBrowser-->filterArray.getObject(%j).getObject(0).filter )
            {
               %count = getWord( AssetBrowser-->filterArray.getObject(%j).getObject(0).getText(), 2 );
               %count--;
               AssetBrowser-->filterArray.getObject(%j).getObject(0).setText( %materialTag @ " ( "@ %count @ " )");
            }
         }
      
   }
   
   UnlistedMaterials.add( "unlistedMaterials", %materialName );
   
   if( %materialName.getFilename() !$= "" && 
         %materialName.getFilename() !$= "tools/gui/AssetBrowser.ed.gui" &&
         %materialName.getFilename() !$= "tools/materialEditor/scripts/materialEditor.ed.cs" )
   {
      AssetBrowserPerMan.removeObjectFromFile(%materialName);
      AssetBrowserPerMan.saveDirty();
   }
      
   AssetBrowser.preloadFilter();
   //AssetBrowser.selectMaterial( "WarningMaterial" );
}

function AssetBrowser::thumbnailCountUpdate(%this)
{
   $Pref::AssetBrowser::ThumbnailCountIndex = AssetBrowser-->materialPreviewCountPopup.getSelected();
   AssetBrowser.LoadFilter( AssetBrowser.currentFilter, AssetBrowser.currentStaticFilter );
}

function AssetBrowser::buildPagesButtons(%this, %currentPage, %totalPages)
{
   // We don't want any more than 8 pages at a time.
   if( %totalPages > 8 )
   {
      // We attempt to display up to 2 pages before the current page
      %start = %currentPage - 2;
      if( %start <= 0 )
      {
         %start = 0;
         %startbracket = false;
      }
      else
      {
         %startbracket = true;
      }
      
      if( (%totalPages - %start) < 8 )
      {
         // Move %start closer to the beginning to maintain 8 pages
         %start = %totalPages - 8;
      }
      
      %end = %start + 8;
      if( %end >= %totalPages )
      {
         %end = %totalPages;
         %endbracket = false;
      }
      else
      {
         %endbracket = true;
      }
   }
   else
   {
      %start = 0;
      %end = %totalPages;
      %startbracket = false;
      %endbracket = false;
   }
   
   if( %startbracket )
   {
      %control =  new GuiTextCtrl(){
                     profile = "ToolsGuiTextCenterProfile";
                     HorizSizing = "right";
                     VertSizing = "bottom";
                     position = "0 2";
                     extent = "14 16";
                     MinExtent = "8 8";
                     text = "...";
                  };
      AssetBrowser-->materialPreviewPagesStack.add( %control );
   }
   
   for( %i = %start; %i < %end; %i++ )
   {
      if( %i != %currentPage )
      {
         %control =   new GuiButtonCtrl() {
                        canSaveDynamicFields = "0";
                        Enabled = "1";
                        isContainer = "0";
                        Profile = "ToolsGuiTextCenterProfile";
                        HorizSizing = "right";
                        VertSizing = "bottom";
                        Position = "0 0";
                        Extent = "14 16";
                        MinExtent = "8 8";
                        canSave = "1";
                        isDecoy = "0";
                        Visible = "1";
                        Command = "AssetBrowser.schedule(0, selectPage, " @ %i @ ");";
                        tooltipprofile = "ToolsGuiToolTipProfile";
                        hovertime = "1000";
                        text = %i+1;
                        groupNum = "-1";
                        buttonType = "PushButton";
                        useMouseEvents = "1";
                     };
      }
      else
      {
         %control =  new GuiTextCtrl(){
                        profile = "ToolsGuiTextBoldCenterProfile";
                        HorizSizing = "right";
                        VertSizing = "bottom";
                        position = "0 2";
                        extent = "14 16";
                        MinExtent = "8 8";
                        text = %i+1;
                     };
      }

      AssetBrowser-->materialPreviewPagesStack.add( %control );
   }
   
   if( %endbracket )
   {
      %control =  new GuiTextCtrl(){
                     profile = "ToolsGuiTextCenterProfile";
                     HorizSizing = "right";
                     VertSizing = "bottom";
                     position = "0 2";
                     extent = "14 16";
                     MinExtent = "8 8";
                     text = "...";
                  };
      AssetBrowser-->materialPreviewPagesStack.add( %control );
   }
}

function AssetBrowser::toggleTagFilterPopup(%this)
{
	if(TagFilterWindow.visible)
		TagFilterWindow.visible = false;
	else
		TagFilterWindow.visible = true;
		
	return;
   %assetQuery = new AssetQuery();
   %numAssetsFound = AssetDatabase.findAllAssets(%assetQuery);
   
   for( %i=0; %i < %numAssetsFound; %i++)
   {
	    %assetId = %assetQuery.getAsset(%i);
		
		//first, get the asset's module, as our major categories
		%module = AssetDatabase.getAssetModule(%assetId);
		
		%moduleName = %module.moduleId;
		
		//check that we don't re-add it
		%moduleItemId = AssetBrowser-->filterTree.findItemByName(%moduleName);
		
		if(%moduleItemId == -1 || %moduleItemId == 0)
			%moduleItemId = AssetBrowser-->filterTree.insertItem(1, %module.moduleId, "", "", 1, 1);
			
		//now, add the asset's category
		%assetType = AssetDatabase.getAssetCategory(%assetId);
		
		%checkBox = new GuiCheckBoxCtrl()
		{
			canSaveDynamicFields = "0";
			isContainer = "0";
			Profile = "ToolsGuiCheckBoxListProfile";
			HorizSizing = "right";
			VertSizing = "bottom";
			Position = "0 0";
			Extent = (%textLength * 4) @ " 18";
			MinExtent = "8 2";
			canSave = "1";
			Visible = "1";
			Variable = %var;
			tooltipprofile = "ToolsGuiToolTipProfile";
			hovertime = "1000";
			text = %text;
			groupNum = "-1";
			buttonType = "ToggleButton";
			useMouseEvents = "0";
			useInactiveState = "0";
			Command = %cmd;
		};
		
		TagFilterList.add(%checkBox);
   }	
}

function AssetBrowser::changeAsset(%this)
{
   //alright, we've selectd an asset for a field, so time to set it!
   %cmd = %this.fieldTargetObject @ "." @ %this.fieldTargetName @ "=\"" @ %this.selectedAsset @ "\";";
   echo("Changing asset via the " @ %cmd @ " command");
   eval(%cmd);
}

//
//
//
function AssetBrowserFilterTree::onSelect(%this, %itemId)
{
	if(%itemId == 1)
		//can't select root
		return;
	
	//alright, we have a module or sub-filter selected, so now build our asset list based on that filter!
	echo("Asset Browser Filter Tree selected filter #:" @ %itemId);
	
	// manage schedule array properly
   if(!isObject(MatEdScheduleArray))
      new ArrayObject(MatEdScheduleArray);
	
	// if we select another list... delete all schedules that were created by 
   // previous load
   for( %i = 0; %i < MatEdScheduleArray.count(); %i++ )
      cancel(MatEdScheduleArray.getKey(%i));
	
	// we have to empty out the list; so when we create new schedules, these dont linger
   MatEdScheduleArray.empty();
   
   // manage preview array
   if(!isObject(MatEdPreviewArray))
      new ArrayObject(MatEdPreviewArray);
      
   // we have to empty out the list; so when we create new guicontrols, these dont linger
   MatEdPreviewArray.empty();
   AssetBrowser-->materialSelection.deleteAllObjects();
   AssetBrowser-->materialPreviewPagesStack.deleteAllObjects();

   %assetArray = new ArrayObject();

   %previewsPerPage = AssetBrowser-->materialPreviewCountPopup.getTextById( AssetBrowser-->materialPreviewCountPopup.getSelected() );

   //First, Query for our assets
   %assetQuery = new AssetQuery();
   %numAssetsFound = AssetDatabase.findAllAssets(%assetQuery);
   
	//module name per our selected filter:
	%moduleItemId = %this.getParentItem(%itemId);
	
	//check if we've selected a package
	if(%moduleItemId == 1)
	{
	   %FilterModuleName = %this.getItemText(%itemId);
	}
	else
	{
	   %FilterModuleName = %this.getItemText(%moduleItemId);
	}
   
    //now, we'll iterate through, and find the assets that are in this module, and this category
    for( %i=0; %i < %numAssetsFound; %i++)
    {
	    %assetId = %assetQuery.getAsset(%i);
		
		//first, get the asset's module, as our major categories
		%module = AssetDatabase.getAssetModule(%assetId);
		
		%moduleName = %module.moduleId;
		
		if(%FilterModuleName $= %moduleName)
		{
			//it's good, so test that the category is right!
			%assetType = AssetDatabase.getAssetCategory(%assetId);
			if(%assetType $= "")
			{
			   %assetType = AssetDatabase.getAssetType(%assetId);
			}
			
			if(%this.getItemText(%itemId) $= %assetType || (%assetType $= "" && %this.getItemText(%itemId) $= "Misc")
			   || %moduleItemId == 1)
			{
				//stop adding after previewsPerPage is hit
				%assetName = AssetDatabase.getAssetName(%assetId);
				
				%searchText = AssetBrowserSearchFilter.getText();
				if(%searchText !$= "\c2Filter...")
				{
					if(strstr(strlwr(%assetName), strlwr(%searchText)) != -1)
						%assetArray.add( %moduleName, %assetName);
				}
				else
				{
					//got it.	
					%assetArray.add( %moduleName, %assetName );
				}
			}
		}
   }

	AssetBrowser.currentPreviewPage = 0;
	AssetBrowser.totalPages = 1;

	// Build out the pages buttons
    AssetBrowser.buildPagesButtons( AssetBrowser.currentPreviewPage, AssetBrowser.totalPages );
	
	for(%i=0; %i < %assetArray.count(); %i++)
		AssetBrowser.buildPreviewArray( %assetArray.getValue(%i), %assetArray.getKey(%i) );
   
    AssetBrowser.loadImages( 0 );
}

//
//
function AssetBrowserSearchFilterText::onWake( %this )
{
   %filter = %this.treeView.getFilterText();
   if( %filter $= "" )
      %this.setText( "\c2Filter..." );
   else
      %this.setText( %filter );
}

//---------------------------------------------------------------------------------------------

function AssetBrowserSearchFilterText::onGainFirstResponder( %this )
{
   %this.selectAllText();
}

//---------------------------------------------------------------------------------------------

// When Enter is pressed in the filter text control, pass along the text of the control
// as the treeview's filter.
function AssetBrowserSearchFilterText::onReturn( %this )
{
   %text = %this.getText();
   if( %text $= "" )
      %this.reset();
   else
   {
      //%this.treeView.setFilterText( %text );
	  %curItem = AssetBrowserFilterTree.getSelectedItem();
	  AssetBrowserFilterTree.onSelect(%curItem);
   }
}

//---------------------------------------------------------------------------------------------

function AssetBrowserSearchFilterText::reset( %this )
{
   %this.setText( "\c2Filter..." );
   //%this.treeView.clearFilterText();
   %curItem = AssetBrowserFilterTree.getSelectedItem();
   AssetBrowserFilterTree.onSelect(%curItem);
}

//---------------------------------------------------------------------------------------------

function AssetBrowserSearchFilterText::onClick( %this )
{
   %this.textCtrl.reset();
}

//
//
//
function AssetBrowser_ImportAssetWindow::onWake(%this)
{
   //We've woken, meaning we're trying to import assets
   //Lets refresh our list
   if(!AssetBrowser_ImportAssetWindow.isVisible())
      return;
   
   //%this.refresh();
}

function AssetBrowser_ImportAssetWindow::refresh(%this)
{
   ImportingAssetList.clear();
   
   %assetCount = AssetBrowser.importAssetListArray.count();
   
   for(%i=0; %i < %assetCount; %i++)
   {
      %assetType = %filePath = AssetBrowser.importAssetListArray.getKey(%i);
      %filePath = AssetBrowser.importAssetListArray.getValue(%i);
      %assetName = fileBase(%filePath);
      
      //create!
      %width = mRound(ImportingAssetList.extent.x / 2);
      %height = 20;
      
      %importEntry = new GuiControl()
      {
         position = "0 0";
         extent = ImportingAssetList.extent.x SPC %height;
         
         new GuiTextCtrl()
         {
           Text = %assetName; 
           position = "0 0";
           extent = %width SPC %height;
           internalName = "AssetName";
         };
         
         new GuiTextCtrl()
         {
           Text = %assetType; 
           position = %width SPC "0";
           extent = %width SPC %height;
           internalName = "AssetType";
         };
      };
      
      ImportingAssetList.add(%importEntry);
   }
}

function AssetBrowser_ImportAssetWindow::ImportAssets(%this)
{
   //do the actual importing, now!
   %assetCount = AssetBrowser.importAssetListArray.count();
   
   //get the selected module data
   %moduleName = ImportAssetPackageList.getText();
   
   %module = ModuleDatabase.findModule(%moduleName);
   
   for(%i=0; %i < %assetCount; %i++)
   {
      %assetType = AssetBrowser.importAssetListArray.getKey(%i);
      %filePath = AssetBrowser.importAssetListArray.getValue(%i);
      %assetName = fileBase(%filePath);
      
      if(%assetType $= "Image")
      {
         %assetPath = "modules/" @ %moduleName @ "/Images";
         %assetFullPath = %assetPath @ "/" @ fileName(%filePath);
         
         %newAsset = new ImageAsset()
         {
            assetName = %assetName;
            versionId = 1;
            imageFile = %assetFullPath;
         };
         
         TAMLWrite(%newAsset, %assetPath @ "/" @ %assetName @ ".asset.taml"); 
         
         //and copy the file into the relevent directory
         if(!pathCopy(%filePath, %assetFullPath))
         {
            error("Unable to import asset: " @ %filePath);
         }
      }
      else if(%assetType $= "Model")
      {
         %assetPath = "modules/" @ %moduleName @ "/Shapes";
         %assetFullPath = %assetPath @ "/" @ fileName(%filePath);
         
         %newAsset = new ShapeAsset()
         {
            assetName = %assetName;
            versionId = 1;
            fileName = %assetFullPath;
         };
         
         TAMLWrite(%newAsset, %assetPath @ "/" @ %assetName @ ".asset.taml"); 
         
         //and copy the file into the relevent directory
         if(!pathCopy(%filePath, %assetFullPath))
         {
            error("Unable to import asset: " @ %filePath);
         }
      }
      else if(%assetType $= "Sound")
      {
         
      }
   }
   
   //force an update of any and all modules so we have an up-to-date asset list
   AssetBrowser.reloadModules();
   AssetBrowser.loadFilters();
   AssetBrowser_ImportAssetWindow.visible = false;
}

function ImportAssetPackageList::onWake(%this)
{
   %this.refresh();
}

function ImportAssetPackageList::refresh(%this)
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

//
//
//
function AssetBrowser_addPackageWindow::CreateNewPackage(%this)
{
   %newPackageName = %this-->packageName.getText();
   
   if(%newPackageName $= "")
      return;
      
   echo("LET IT BEGIIIIIIIIIIIIIIIN with a new package named: " @ %newPackageName);
   
   %moduleFilePath = "modules/" @ %newPackageName;
   %moduleScriptFilePath = %moduleFilePath @ "/" @ %newPackageName @ ".cs";
   
   %newPackage = new ModuleDefinition()
   {
      ModuleId = %newPackageName;
      versionId = 1;
      ScriptFile = %moduleScriptFilePath;
      Group = "Game"; //todo, change from hardcode
      CreateFunction="Create";
	   DestroyFunction="Destroy";
      
      new DeclaredAssets()
      {
         Extension = "asset.taml";
         Recurse = true;
      };
   };
   
   TAMLWrite(%newPackage, %moduleFilePath @ "/" @ %newPackageName @ ".module.taml"); 
   
   //Now generate the script file for it
   %file = new FileObject();
	
	if(%file.openForWrite(%moduleScriptFilePath))
	{
		%file.writeline("function " @ %newPackageName @ "::onCreate(%this)\n{\n\n}\n");
		%file.writeline("function " @ %newPackageName @ "::onDestroy(%this)\n{\n\n}\n");
		
		//todo, pre-write any event functions of interest
		
		%file.close();
	}
   
   //force a refresh of our modules list
   AssetBrowser.reloadModules();
   ImportAssetPackageList.refresh();
   
   AssetBrowser_addPackageWindow.visible = false;
}

//
//
//
function AssetBrowser::reloadModules(%this)
{
   ModuleDatabase.unloadGroup("Game");
   
   %modulesList = ModuleDatabase.findModules();
   
   %count = getWordCount(%modulesList);
   
   for(%i=0; %i < %count; %i++)
   {
      %moduleId = getWord(%modulesList, %i).ModuleId;
      ModuleDatabase.unloadExplicit(%moduleId);
   }

   ModuleDatabase.scanModules();
   
   %modulesList = ModuleDatabase.findModules();
   
   %count = getWordCount(%modulesList);
   
   for(%i=0; %i < %count; %i++)
   {
      %moduleId = getWord(%modulesList, %i).ModuleId;
      ModuleDatabase.loadExplicit(%moduleId);
   }
   
   //ModuleDatabase.loadGroup("Game");
}