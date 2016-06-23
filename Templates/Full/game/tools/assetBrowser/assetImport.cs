
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
   //Canvas.pushDialog(AssetBrowser_importAsset);
   AssetBrowser_importAssetWindow.visible = true;
   AssetBrowser_ImportAssetWindow.refresh();
   AssetBrowser_importAssetWindow.selectWindow();
   
   // Update object library
   GuiFormManager::SendContentMessage($LBCreateSiderBar, %this, "refreshAll 1");
}

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
      %assetImportSuccessful = false;
      
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
         
         %assetImportSuccessful = TAMLWrite(%newAsset, %assetPath @ "/" @ %assetName @ ".asset.taml"); 
         
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
         
         %assetImportSuccessful = TAMLWrite(%newAsset, %assetPath @ "/" @ %assetName @ ".asset.taml"); 
         
         //and copy the file into the relevent directory
         if(!pathCopy(%filePath, %assetFullPath))
         {
            error("Unable to import asset: " @ %filePath);
         }
         
         //now, force-load the file if it's collada
         %fileExt = fileExt(%assetFullPath);
         if(%fileExt $= ".dae" || %fileExt $= ".kmz")
         {
            %tempShape = new TSStatic()
            {
               shapeName = %assetFullPath;
            };
            
            %tempShape.delete();
         }
         
      }
      else if(%assetType $= "Sound")
      {
         
      }
      
      if(%assetImportSuccessful)
      {
         %moduleDef = ModuleDatabase.findModule(%moduleName,1);
         AssetDatabase.addDeclaredAsset(%moduleDef, %assetPath @ "/" @ %assetName @ ".asset.taml");
      }
   }
   
   //force an update of any and all modules so we have an up-to-date asset list
   //AssetBrowser.reloadModules();
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