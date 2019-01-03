//-----------------------------------------------------------------------------
// Copyright (c) 2012 GarageGames, LLC
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.
//-----------------------------------------------------------------------------

//
//
function isImageFormat(%fileExt)
{
   if( (%fileExt $= ".png") || (%fileExt $= ".jpg") || (%fileExt $= ".bmp") || (%fileExt $= ".dds") || (%fileExt $= ".tif"))
      return true;
      
   return false;
}

function isShapeFormat(%fileExt)
{
   if( (%fileExt $= ".dae") || (%fileExt $= ".dts") || (%fileExt $= ".fbx") || (%fileExt $= ".obj") || (%fileExt $= ".blend"))
      return true;
      
   return false;
}

function isSoundFormat(%fileExt)
{
   if( (%fileExt $= ".ogg") || (%fileExt $= ".wav") || (%fileExt $= ".mp3"))
      return true;
      
   return false;
}

function getImageInfo(%file)
{
   //we're going to populate a GuiTreeCtrl with info of the inbound image file
}

//This lets us go and look for a image at the importing directory as long as it matches the material name
function findImageFile(%path, %materialName, %type)
{
   
   if(isFile(%path @ "/" @ %materialName @ ".jpg"))
      return %path @ "/" @ %materialName @ ".jpg";
   else if(isFile(%path @ "/" @ %materialName @ ".png"))
      return %path @ "/" @ %materialName @ ".png";
   else if(isFile(%path @ "/" @ %materialName @ ".dds"))
      return %path @ "/" @ %materialName @ ".dds";
   else if(isFile(%path @ "/" @ %materialName @ ".tif"))
      return %path @ "/" @ %materialName @ ".tif";
}

function AssetBrowser::onBeginDropFiles( %this )
{   
   error("% DragDrop - Beginning files dropping.");
   %this.importAssetNewListArray.empty();
   %this.importAssetUnprocessedListArray.empty();
   %this.importAssetFinalListArray.empty();
}

function AssetBrowser::onDropFile( %this, %filePath )
{
   if(!%this.isVisible())
      return;
      
   %fileExt = fileExt( %filePath );
   //add it to our array!
   if(isImageFormat(%fileExt))
      %this.addImportingAsset("Image", %filePath);
   else if( isShapeFormat(%fileExt))
      %this.addImportingAsset("Model", %filePath);
   else if( isSoundFormat(%fileExt))
      %this.addImportingAsset("Sound", %filePath);
   else if( %fileExt $= ".cs" || %fileExt $= ".cs.dso" )
      %this.addImportingAsset("Script", %filePath);
   else if( %fileExt $= ".gui" || %fileExt $= ".gui.dso" )
      %this.addImportingAsset("GUI", %filePath);
   else if (%fileExt $= ".zip")
      %this.onDropZipFile(%filePath);
}

function AssetBrowser::onDropZipFile(%this, %filePath)
{
   if(!%this.isVisible())
      return;
      
   %zip = new ZipObject();
   %zip.openArchive(%filePath);
   %count = %zip.getFileEntryCount();
   
   echo("Dropped in a zip file with" SPC %count SPC "files inside!");
   
   return;
   for (%i = 0; %i < %count; %i++)
   {
      %fileEntry = %zip.getFileEntry(%i);
      %fileFrom = getField(%fileEntry, 0);
      
      //First, we wanna scan to see if we have modules to contend with. If we do, we'll just plunk them in wholesale
      //and not process their contents.
      
      //If not modules, it's likely an art pack or other mixed files, so we'll import them as normal
      if( (%fileExt $= ".png") || (%fileExt $= ".jpg") || (%fileExt $= ".bmp") || (%fileExt $= ".dds") )
         %this.importAssetListArray.add("Image", %filePath);
      else if( (%fileExt $= ".dae") || (%fileExt $= ".dts"))
         %this.importAssetListArray.add("Model", %filePath);
      else if( (%fileExt $= ".ogg") || (%fileExt $= ".wav") || (%fileExt $= ".mp3"))
         %this.importAssetListArray.add("Sound", %filePath);
      else if( (%fileExt $= ".gui") || (%fileExt $= ".gui.dso"))
         %this.importAssetListArray.add("GUI", %filePath);
      //else if( (%fileExt $= ".cs") || (%fileExt $= ".dso"))
      //   %this.importAssetListArray.add("Script", %filePath);
      else if( (%fileExt $= ".mis"))
         %this.importAssetListArray.add("Level", %filePath);
         
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

function AssetBrowser::onDropSoundFile(%this, %filePath)
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

function AssetBrowser::onEndDropFiles( %this )
{
   if(!%this.isVisible())
      return;
   
   //we have assets to import, so go ahead and display the window for that now
   Canvas.pushDialog(AssetImportCtrl);
   ImportAssetWindow.visible = true;
   //ImportAssetWindow.validateAssets();
   ImportAssetWindow.refresh();
   ImportAssetWindow.selectWindow();
   
   // Update object library
   GuiFormManager::SendContentMessage($LBCreateSiderBar, %this, "refreshAll 1");
   
   if(ImportAssetWindow.importConfigsList.count() == 0)
   {
      MessageBoxOK( "Warning", "No base import config. Please create an import configuration set to simplify asset importing.");
   }
}
//
//
//

function AssetBrowser::ImportTemplateModules(%this)
{
   //AssetBrowser_ImportModule
   Canvas.pushDialog(AssetBrowser_ImportModuleTemplate);
   AssetBrowser_ImportModuleTemplateWindow.visible = true;   
   
   AssetBrowser_ImportModuleTemplateList.clear();
   
   //ModuleDatabase.scanModules("../../../../../../Templates/Modules/");
   
   %pattern = "../../../../../../Templates/Modules//*//*.module";   
   %file = findFirstFile( %pattern );

   while( %file !$= "" )
   {      
      echo("FOUND A TEMPLATE MODULE! " @ %file);
      %file = findNextFile( %pattern );
   }
   
   /*%moduleCheckbox = new GuiCheckBoxCtrl()
   {
      text = "Testadoo";
      moduleId = "";
   };
   
   AssetBrowser_ImportModuleTemplateList.addRow("0", "Testaroooooo");
   AssetBrowser_ImportModuleTemplateList.addRow("1", "Testadoooooo");*/
}

function AssetBrowser_ImportModuleTemplateList::onSelect(%this, %selectedRowIdx, %text)
{
   echo("Selected row: " @ %selectedRowIdx @ " " @ %text);
}

function AssetBrowser::addImportingAsset( %this, %assetType, %filePath, %parentAssetItem, %assetNameOverride )
{
   //In some cases(usually generated assets on import, like materials) we'll want to specifically define the asset name instead of peeled from the filePath
   if(%assetNameOverride !$= "")
      %assetName = %assetNameOverride;
   else
      %assetName = fileBase(%filePath);
      
   //We don't get a file path at all if we're a generated entry, like materials
   //if we have a file path, though, then sanitize it
   if(%filePath !$= "")
      %filePath = filePath(%filePath) @ "/" @ fileBase(%filePath) @ fileExt(%filePath);
   
   %moduleName = AssetBrowser.SelectedModule;
   ImportAssetModuleList.text = %moduleName;
   
   //Add to our main list
   %assetItem = new ScriptObject()
   {
      assetType = %assetType;
      filePath = %filePath;
      assetName = %assetName;
      cleanAssetName = %assetName; 
      moduleName = %moduleName;
      dirty  = true;
      parentAssetItem = %parentAssetItem;
      status = "";
      statusType = "";
      statusInfo = "";
      skip = false;
   };
   
   //little bit of interception here
   if(%assetItem.assetType $= "Model")
   {
      %fileExt = fileExt(%assetItem.filePath);
      if(%fileExt $= ".dae")
      {
         %shapeInfo = new GuiTreeViewCtrl();
         enumColladaForImport(%assetItem.filePath, %shapeInfo, false);  
      }
      else
      {
         %shapeInfo = GetShapeInfo(%assetItem.filePath);
      }
      
      %assetItem.shapeInfo = %shapeInfo;
      
      %shapeItem = %assetItem.shapeInfo.findItemByName("Shape");
      %shapeCount = %assetItem.shapeInfo.getItemValue(%shapeItem);
      
      %animItem = %assetItem.shapeInfo.findItemByName("Animations");
      %animCount = %assetItem.shapeInfo.getItemValue(%animItem);
      
      //If the model has shapes AND animations, then it's a normal shape with embedded animations
      //if it has shapes and no animations it's a regular static mesh
      //if it has no shapes and animations, it's a special case. This means it's a shape animation only file so it gets flagged as special
      if(%shapeCount == 0 && %animCount != 0)
      {
         %assetItem.assetType = "Animation";
      }
      else if(%shapeCount == 0 && %animCount == 0)
      {
         //either it imported wrong or it's a bad file we can't read. Either way, don't try importing it
         error("Error - attempted to import a model file with no shapes or animations! Model in question was: " @ %filePath);
         
         %assetItem.delete();
         return 0;
      }
   }
   
   if(%parentAssetItem $= "")
   {
      %assetItem.parentDepth = 0;
      %this.importAssetNewListArray.add(%assetItem);
      %this.importAssetUnprocessedListArray.add(%assetItem);
   }
   else
   {
      %assetItem.parentDepth = %parentAssetItem.parentDepth + 1;  
      %parentIndex = %this.importAssetUnprocessedListArray.getIndexFromKey(%parentAssetItem);
      
      %parentAssetItem.dependencies = %parentAssetItem.dependencies SPC %assetItem;
      trim(%parentAssetItem.dependencies);
      
      %this.importAssetUnprocessedListArray.insert(%assetItem, "", %parentIndex + 1);
   }
   
   return %assetItem;
}

function AssetBrowser::importLegacyGame(%this)
{
   
}

function AssetBrowser::importNewAssetFile(%this)
{
   %dlg = new OpenFileDialog()
   {
      Filters        = "Shape Files(*.dae, *.cached.dts)|*.dae;*.cached.dts|Images Files(*.jpg,*.png,*.tga,*.bmp,*.dds)|*.jpg;*.png;*.tga;*.bmp;*.dds|Any Files (*.*)|*.*|";
      DefaultPath    = $Pref::WorldEditor::LastPath;
      DefaultFile    = "";
      ChangePath     = false;
      OverwritePrompt = true;
      forceRelativePath = false;
      //MultipleFiles = true;
   };

   %ret = %dlg.Execute();
   
   if ( %ret )
   {
      $Pref::WorldEditor::LastPath = filePath( %dlg.FileName );
      %fullPath = %dlg.FileName;
      %file = fileBase( %fullPath );
   }   
   
   %dlg.delete();
   
   if ( !%ret )
      return;
      
   AssetBrowser.onBeginDropFiles();
   AssetBrowser.onDropFile(%fullPath);
   AssetBrowser.onEndDropFiles();
}

//
function ImportAssetButton::onClick(%this)
{
   ImportAssetsPopup.showPopup(Canvas);
}
//

//
function ImportAssetWindow::onWake(%this)
{
   //We've woken, meaning we're trying to import assets
   //Lets refresh our list
   if(!ImportAssetWindow.isVisible())
      return;
      
   $AssetBrowser::importConfigsFile = "tools/assetBrowser/assetImportConfigs.xml";
   
   %this.reloadImportOptionConfigs();
}

function ImportAssetWindow::reloadImportOptionConfigs(%this)
{
   ImportAssetWindow.importConfigsList = new ArrayObject();
   ImportAssetConfigList.clear();
   
   %xmlDoc = new SimXMLDocument();
   if(%xmlDoc.loadFile($AssetBrowser::importConfigsFile))
   {
      //StateMachine element
      %xmlDoc.pushFirstChildElement("AssetImportConfigs");
      
      //Configs
      %configCount = 0;
      while(%xmlDoc.pushChildElement(%configCount))
      {
         %configObj = new ScriptObject(){};
         
         %configObj.Name = %xmlDoc.attribute("Name");

         %xmlDoc.pushFirstChildElement("Mesh");
            %configObj.ImportMesh = %xmlDoc.attribute("ImportMesh");
            %configObj.DoUpAxisOverride = %xmlDoc.attribute("DoUpAxisOverride");
            %configObj.UpAxisOverride = %xmlDoc.attribute("UpAxisOverride");
            %configObj.DoScaleOverride = %xmlDoc.attribute("DoScaleOverride");
            %configObj.ScaleOverride = %xmlDoc.attribute("ScaleOverride");
            %configObj.IgnoreNodeScale = %xmlDoc.attribute("IgnoreNodeScale");
            %configObj.AdjustCenter = %xmlDoc.attribute("AdjustCenter");
            %configObj.AdjustFloor = %xmlDoc.attribute("AdjustFloor");
            %configObj.CollapseSubmeshes = %xmlDoc.attribute("CollapseSubmeshes");       
            %configObj.LODType = %xmlDoc.attribute("LODType");
            %configObj.ImportedNodes = %xmlDoc.attribute("ImportedNodes");
            %configObj.IgnoreNodes = %xmlDoc.attribute("IgnoreNodes");
            %configObj.ImportMeshes = %xmlDoc.attribute("ImportMeshes");
            %configObj.IgnoreMeshes = %xmlDoc.attribute("IgnoreMeshes");
         %xmlDoc.popElement();
         
         %xmlDoc.pushFirstChildElement("Materials");
            %configObj.ImportMaterials = %xmlDoc.attribute("ImportMaterials");
            %configObj.CreateComposites = %xmlDoc.attribute("CreateComposites");
            %configObj.UseDiffuseSuffixOnOriginImg = %xmlDoc.attribute("UseDiffuseSuffixOnOriginImg");
            %configObj.UseExistingMaterials = %xmlDoc.attribute("UseExistingMaterials");
         %xmlDoc.popElement();
         
         %xmlDoc.pushFirstChildElement("Animations");
            %configObj.ImportAnimations = %xmlDoc.attribute("ImportAnimations");
            %configObj.SeparateAnimations = %xmlDoc.attribute("SeparateAnimations");
            %configObj.SeparateAnimationPrefix = %xmlDoc.attribute("SeparateAnimationPrefix");
         %xmlDoc.popElement();
         
         %xmlDoc.pushFirstChildElement("Collisions");
            %configObj.GenerateCollisions = %xmlDoc.attribute("GenerateCollisions");
            %configObj.GenCollisionType = %xmlDoc.attribute("GenCollisionType");
            %configObj.CollisionMeshPrefix = %xmlDoc.attribute("CollisionMeshPrefix");
            %configObj.GenerateLOSCollisions = %xmlDoc.attribute("GenerateLOSCollisions");
            %configObj.GenLOSCollisionType = %xmlDoc.attribute("GenLOSCollisionType");
            %configObj.LOSCollisionMeshPrefix = %xmlDoc.attribute("LOSCollisionMeshPrefix");
         %xmlDoc.popElement();
         
         %xmlDoc.pushFirstChildElement("Images");
            %configObj.ImageType = %xmlDoc.attribute("ImageType");
            %configObj.DiffuseTypeSuffixes = %xmlDoc.attribute("DiffuseTypeSuffixes");
            %configObj.NormalTypeSuffixes = %xmlDoc.attribute("NormalTypeSuffixes");
            %configObj.SpecularTypeSuffixes = %xmlDoc.attribute("SpecularTypeSuffixes");
            %configObj.MetalnessTypeSuffixes = %xmlDoc.attribute("MetalnessTypeSuffixes");
            %configObj.RoughnessTypeSuffixes = %xmlDoc.attribute("RoughnessTypeSuffixes");
            %configObj.SmoothnessTypeSuffixes = %xmlDoc.attribute("SmoothnessTypeSuffixes");
            %configObj.AOTypeSuffixes = %xmlDoc.attribute("AOTypeSuffixes");
            %configObj.CompositeTypeSuffixes = %xmlDoc.attribute("CompositeTypeSuffixes");
            %configObj.TextureFilteringMode = %xmlDoc.attribute("TextureFilteringMode");
            %configObj.UseMips = %xmlDoc.attribute("UseMips");
            %configObj.IsHDR = %xmlDoc.attribute("IsHDR");
            %configObj.Scaling = %xmlDoc.attribute("Scaling");
            %configObj.Compressed = %xmlDoc.attribute("Compressed");
            %configObj.GenerateMaterialOnImport = %xmlDoc.attribute("GenerateMaterialOnImport");
            %configObj.PopulateMaterialMaps = %xmlDoc.attribute("PopulateMaterialMaps");
         %xmlDoc.popElement();
         
         %xmlDoc.pushFirstChildElement("Sounds");
            %configObj.VolumeAdjust = %xmlDoc.attribute("VolumeAdjust");
            %configObj.PitchAdjust = %xmlDoc.attribute("PitchAdjust");
            %configObj.Compressed = %xmlDoc.attribute("Compressed");
         %xmlDoc.popElement();
         
         %xmlDoc.popElement();
         %configCount++;
         
         ImportAssetWindow.importConfigsList.add(%configObj);
      }
      
      %xmlDoc.popElement();
   }
   
   for(%i = 0; %i < ImportAssetWindow.importConfigsList.count(); %i++)
   {
      %configObj = ImportAssetWindow.importConfigsList.getKey(%i);
      ImportAssetConfigList.add(%configObj.Name);
   }
   
   %importConfigIdx = ImportAssetWindow.activeImportConfigIndex;
   if(%importConfigIdx $= "")
      %importConfigIdx = 0;
      
   ImportAssetConfigList.setSelected(%importConfigIdx);
}

function ImportAssetWindow::setImportOptions(%this, %optionsObj)
{
   //Todo, editor + load from files for preconfigs
   
   //Meshes
   %optionsObj.ImportMesh = true;
   %optionsObj.UpAxisOverride = "Z_AXIS";
   %optionsObj.OverrideScale = 1.0;
   %optionsObj.IgnoreNodeScale = false;
   %optionsObj.AdjustCenter = false;
   %optionsObj.AdjustFloor = false;
   %optionsObj.CollapseSubmeshes = false;
   %optionsObj.LODType = "TrailingNumber";
   %optionsObj.TrailingNumber = 2;
   %optionsObj.ImportedNodes = "";
   %optionsObj.IgnoreNodes = "";
   %optionsObj.ImportMeshes = "";
   %optionsObj.IgnoreMeshes = "";
   
   //Materials
   %optionsObj.ImportMaterials = true;
   %optionsObj.CreateComposites = true;
   
   //Animations
   %optionsObj.ImportAnimations = true;
   %optionsObj.SeparateAnimations = true;
   %optionsObj.SeparateAnimationPrefix = "";
   
   //Collision
   %optionsObj.GenerateCollisions = true;
   %optionsObj.GenCollisionType = "CollisionMesh";
   %optionsObj.CollisionMeshPrefix = "Collision";
   %optionsObj.GenerateLOSCollisions = true;
   %optionsObj.GenLOSCollisionType = "CollisionMesh";
   %optionsObj.LOSCollisionMeshPrefix = "LOS";
   
   //Images
   %optionsObj.ImageType = "Diffuse";
   %optionsObj.DiffuseTypeSuffixes = "_ALBEDO,_DIFFUSE,_ALB,_DIF,_COLOR,_COL";
   %optionsObj.NormalTypeSuffixes = "_NORMAL,_NORM";
   %optionsObj.SpecularTypeSuffixes = "_SPECULAR,_SPEC";
   %optionsObj.MetalnessTypeSuffixes = "_METAL,_MET,_METALNESS,_METALLIC";
   %optionsObj.RoughnessTypeSuffixes = "_ROUGH,_ROUGHNESS";
   %optionsObj.SmoothnessTypeSuffixes = "_SMOOTH,_SMOOTHNESS";
   %optionsObj.AOTypeSuffixes = "_AO,_AMBIENT,_AMBIENTOCCLUSION";
   %optionsObj.CompositeTypeSuffixes = "_COMP,_COMPOSITE";
   %optionsObj.TextureFilteringMode = "Bilinear";
   %optionsObj.UseMips = true;
   %optionsObj.IsHDR = false;
   %optionsObj.Scaling = 1.0;
   %optionsObj.Compressed = true;
   
   //Sounds
   %optionsObj.VolumeAdjust = 1.0;
   %optionsObj.PitchAdjust = 1.0;
   %optionsObj.Compressed = false;
}

//
function ImportAssetWindow::processNewImportAssets(%this)
{
   %unprocessedCount = AssetBrowser.importAssetUnprocessedListArray.count();
   while(AssetBrowser.importAssetUnprocessedListArray.count() > 0)
   {
      %assetItem = AssetBrowser.importAssetUnprocessedListArray.getKey(0);  
      
      %assetConfigObj = ImportAssetWindow.activeImportConfig.clone();
      %assetConfigObj.assetIndex = %i;

      //sanetize before modifying our asset name(suffix additions, etc)      
      if(%assetItem.assetName !$= %assetItem.cleanAssetName)
         %assetItem.assetName = %assetItem.cleanAssetName;
         
      %assetConfigObj.assetName = %assetItem.assetName;
      %assetItem.importConfig = %assetConfigObj;
      
      if(%assetItem.assetType $= "Model")
      {
         %fileExt = fileExt(%assetItem.filePath);
         if(%fileExt $= ".dae")
         {
            %shapeInfo = new GuiTreeViewCtrl();
            enumColladaForImport(%assetItem.filePath, %shapeInfo, false);  
         }
         else
         {
            %shapeInfo = GetShapeInfo(%assetItem.filePath);
         }
         
         %assetItem.shapeInfo = %shapeInfo;
      
         %shapeItem = %assetItem.shapeInfo.findItemByName("Shape");
         %shapeCount = %assetItem.shapeInfo.getItemValue(%shapeItem);
         
         if(%assetConfigObj.ImportMesh == 1 && %shapeCount > 0)
         {
            
         }
         
         %animItem = %assetItem.shapeInfo.findItemByName("Animations");
         %animCount = %assetItem.shapeInfo.getItemValue(%animItem);
         
         if(%assetConfigObj.ImportAnimations == 1 && %animCount > 0)
         {
            %animationItem = %assetItem.shapeInfo.getChild(%animItem);
            
            %animName = %assetItem.shapeInfo.getItemText(%animationItem);
            //%animName = %assetItem.shapeInfo.getItemValue(%animationItem);
            
            AssetBrowser.addImportingAsset("Animation", %animName, %assetItem);
            
            %animationItem = %assetItem.shapeInfo.getNextSibling(%animationItem);
            while(%animationItem != 0)
            {
               %animName = %assetItem.shapeInfo.getItemText(%animationItem);
               //%animName = %assetItem.shapeInfo.getItemValue(%animationItem);
               
               AssetBrowser.addImportingAsset("Animation", %animName, %assetItem);
                  
               %animationItem = %shapeInfo.getNextSibling(%animationItem);
            }
         }
         
         %matItem = %assetItem.shapeInfo.findItemByName("Materials");
         %matCount = %assetItem.shapeInfo.getItemValue(%matItem);
         
         if(%assetConfigObj.importMaterials == 1 && %matCount > 0)
         {
            %materialItem = %assetItem.shapeInfo.getChild(%matItem);
            
            %matName = %assetItem.shapeInfo.getItemText(%materialItem);
            
            %filePath = %assetItem.shapeInfo.getItemValue(%materialItem);
            if(%filePath !$= "")
            {
               AssetBrowser.addImportingAsset("Material", %filePath, %assetItem);
            }
            else
            {
               //we need to try and find our material, since the shapeInfo wasn't able to find it automatically
               %filePath = findImageFile(filePath(%assetItem.filePath), %matName);
               if(%filePath !$= "")
                  AssetBrowser.addImportingAsset("Material", %filePath, %assetItem);
               else
                  AssetBrowser.addImportingAsset("Material", %matName, %assetItem);
            }
            
            %materialItem = %assetItem.shapeInfo.getNextSibling(%materialItem);
            while(%materialItem != 0)
            {
               %matName = %assetItem.shapeInfo.getItemText(%materialItem);
               %filePath = %assetItem.shapeInfo.getItemValue(%materialItem);
               if(%filePath !$= "")
               {
                  AssetBrowser.addImportingAsset("Material", %filePath, %assetItem);
               }
               else
               {
                  //we need to try and find our material, since the shapeInfo wasn't able to find it automatically
                  %filePath = findImageFile(filePath(%assetItem.filePath), %matName);
                  if(%filePath !$= "")
                     AssetBrowser.addImportingAsset("Material", %filePath, %assetItem);
                  else
                     AssetBrowser.addImportingAsset("Material", %matName, %assetItem);
               }
                  
               %materialItem = %shapeInfo.getNextSibling(%materialItem);
            }
         }
      }
      else if(%assetItem.assetType $= "Animation")
      {
         //if we don't have our own file, that means we're gunna be using our parent shape's file so reference that
         if(!isFile(%assetItem.filePath))
         {
            %assetItem.filePath = %assetItem.parentAssetItem.filePath;
         }
      }
      else if(%assetItem.assetType $= "Material")
      {
         //Iterate over to find appropriate images for
         
         //Fetch just the fileBase name
         %fileDir = filePath(%assetItem.filePath);
         %filename = fileBase(%assetItem.filePath);
         %fileExt = fileExt(%assetItem.filePath);
         
         if(%assetItem.importConfig.PopulateMaterialMaps == 1)
         {
            if(%assetItem.diffuseImageAsset $= "")
            {
               //First, load our diffuse map, as set to the material in the shape
               //We're going to presume(for now) that the specifically-mentioned file for a given material is the diffuse/albedo
               %diffuseImagePath = %fileDir @ "/" @ %filename @ %fileExt;
               
               %diffuseImageSuffix = %this.parseImagePathSuffixes(%diffuseImagePath);
               
               if(%assetItem.importConfig.UseDiffuseSuffixOnOriginImg == 1 && %diffuseImageSuffix $= "")
               {
                  %diffuseToken = getToken(%assetItem.importConfig.DiffuseTypeSuffixes, ",", 0);
                  
                  %diffuseAsset = AssetBrowser.addImportingAsset("Image", %diffuseImagePath, %assetItem, %filename @ %diffuseToken);
               }
               else
               {
                  %diffuseAsset = AssetBrowser.addImportingAsset("Image", %diffuseImagePath, %assetItem);
               }
               
               %assetItem.diffuseImageAsset = %diffuseAsset;
            }
            
            if(%assetItem.normalImageAsset $= "")
            {
               //Now, iterate over our comma-delimited suffixes to see if we have any matches. We'll use the first match in each case, if any.
               //First, normal map
               %listCount = getTokenCount(%assetItem.importConfig.NormalTypeSuffixes, ",");
         
               %foundFile = 0;
               for(%i=0; %i < %listCount; %i++)
               {
                  %entryText = getToken(%assetItem.importConfig.NormalTypeSuffixes, ",", %i);
                  
                  %targetFilePath = %fileDir @ "/" @ %filename @ %entryText @ %fileExt;
                  %foundFile = isFile(%targetFilePath);
                  
                  if(%foundFile)
                  {
                     %normalAsset = AssetBrowser.addImportingAsset("Image", %targetFilePath, %assetItem);
                     %assetItem.normalImageAsset = %normalAsset;
                     break;  
                  }
               }
            }
            if(%assetItem.specularImageAsset $= "")
            {
               //Specular
               %listCount = getTokenCount(%assetItem.importConfig.SpecularTypeSuffixes, ",");
         
               %foundFile = 0;
               for(%i=0; %i < %listCount; %i++)
               {
                  %entryText = getToken(%assetItem.importConfig.SpecularTypeSuffixes, ",", %i);
                  
                  %targetFilePath = %fileDir @ "/" @ %filename @ %entryText @ %fileExt;
                  %foundFile = isFile(%targetFilePath);
                  
                  if(%foundFile)
                  {
                     %specularAsset = AssetBrowser.addImportingAsset("Image", %targetFilePath, %assetItem);
                     %assetItem.specularImageAsset = %specularAsset;
                     break;  
                  }
               }
            }
            
            if(%assetItem.metalImageAsset $= "")
            {
               //Metal
               %listCount = getTokenCount(%assetItem.importConfig.MetalnessTypeSuffixes, ",");
         
               %foundFile = 0;
               for(%i=0; %i < %listCount; %i++)
               {
                  %entryText = getToken(%assetItem.importConfig.MetalnessTypeSuffixes, ",", %i);
                  
                  %targetFilePath = %fileDir @ "/" @ %filename @ %entryText @ %fileExt;
                  %foundFile = isFile(%targetFilePath);
                  
                  if(%foundFile)
                  {
                     %metalAsset = AssetBrowser.addImportingAsset("Image", %targetFilePath, %assetItem);
                     %assetItem.metalImageAsset = %metalAsset;
                     break;  
                  }
               }
            }
            
            if(%assetItem.roughnessImageAsset $= "")
            {
               //Roughness
               %listCount = getTokenCount(%assetItem.importConfig.RoughnessTypeSuffixes, ",");
         
               %foundFile = 0;
               for(%i=0; %i < %listCount; %i++)
               {
                  %entryText = getToken(%assetItem.importConfig.RoughnessTypeSuffixes, ",", %i);
                  
                  %targetFilePath = %fileDir @ "/" @ %filename @ %entryText @ %fileExt;
                  %foundFile = isFile(%targetFilePath);
                  
                  if(%foundFile)
                  {
                     %roughnessAsset = AssetBrowser.addImportingAsset("Image", %targetFilePath, %assetItem);
                     %assetItem.roughnessImageAsset = %roughnessAsset;
                     break;  
                  }
               }
            }
            
            if(%assetItem.smoothnessImageAsset $= "")
            {
               //Smoothness
               %listCount = getTokenCount(%assetItem.importConfig.SmoothnessTypeSuffixes, ",");
         
               %foundFile = 0;
               for(%i=0; %i < %listCount; %i++)
               {
                  %entryText = getToken(%assetItem.importConfig.SmoothnessTypeSuffixes, ",", %i);
                  
                  %targetFilePath = %fileDir @ "/" @ %filename @ %entryText @ %fileExt;
                  %foundFile = isFile(%targetFilePath);
                  
                  if(%foundFile)
                  {
                     %smoothnessAsset = AssetBrowser.addImportingAsset("Image", %targetFilePath, %assetItem);
                     %assetItem.SmoothnessImageAsset = %smoothnessAsset;
                     break;  
                  }
               }
            }
            
            if(%assetItem.AOImageAsset $= "")
            {
               //AO
               %listCount = getTokenCount(%assetItem.importConfig.AOTypeSuffixes, ",");
         
               %foundFile = 0;
               for(%i=0; %i < %listCount; %i++)
               {
                  %entryText = getToken(%assetItem.importConfig.AOTypeSuffixes, ",", %i);
                  
                  %targetFilePath = %fileDir @ "/" @ %filename @ %entryText @ %fileExt;
                  %foundFile = isFile(%targetFilePath);
                  
                  if(%foundFile)
                  {
                     %AOAsset = AssetBrowser.addImportingAsset("Image", %targetFilePath, %assetItem);
                     %assetItem.AOImageAsset = %AOAsset;
                     break;  
                  }
               }
            }
            
            if(%assetItem.compositeImageAsset $= "")
            {
               //Composite
               %listCount = getTokenCount(%assetItem.importConfig.CompositeTypeSuffixes, ",");
         
               %foundFile = 0;
               for(%i=0; %i < %listCount; %i++)
               {
                  %entryText = getToken(%assetItem.importConfig.CompositeTypeSuffixes, ",", %i);
                  
                  %targetFilePath = %fileDir @ "/" @ %filename @ %entryText @ %fileExt;
                  %foundFile = isFile(%targetFilePath);
                  
                  if(%foundFile)
                  {
                     %compositeAsset = AssetBrowser.addImportingAsset("Image", %targetFilePath, %assetItem);
                     %assetItem.compositeImageAsset = %compositeAsset;
                     break;  
                  }
               }
            }
         }
      } 
      else if(%assetItem.assetType $= "Image")
      {
         if(%assetConfigObj.GenerateMaterialOnImport == 1 && %assetItem.parentAssetItem $= "")
         {
            //First, see if this already has a suffix of some sort based on our import config logic. Many content pipeline tools like substance automatically appends them
            %foundSuffixType = %this.parseImageSuffixes(%assetItem);
            
            if(%foundSuffixType $= "")
            {
               %noSuffixName = %assetItem.AssetName;
            }
            else
            {
               %suffixPos = strpos(strlwr(%assetItem.AssetName), strlwr(%assetItem.imageSuffixType), 0);
               %noSuffixName = getSubStr(%assetItem.AssetName, 0, %suffixPos);
            }
         
            //Check if our material already exists
            //First, lets double-check that we don't already have an
            %materialAsset = %this.findImportingAssetByName(%noSuffixName);
            if(%materialAsset == 0)
            {
               %filePath = %assetItem.filePath;
               if(%filePath !$= "")
                  %materialAsset = AssetBrowser.addImportingAsset("Material", "", %assetItem.parentAssetItem, %noSuffixName);
            }
            
            //%materialIndex = %this.importAssetUnprocessedListArray.getIndexFromKey(%materialAsset);
            //%assetIndex = %this.importAssetUnprocessedListArray.getIndexFromKey(%assetItem);
            
            //Organize the layout so the material is the parent of the inbound images
            %assetItem.parentDepth = %materialAsset.parentDepth + 1;  
      
            %materialAsset.dependencies = %materialAsset.dependencies SPC %assetItem;
            trim(%materialAsset.dependencies);
   
            //Lets do some cleverness here. If we're generating a material we can parse like assets being imported(similar file names) but different suffixes
            //if we find these, we'll just populate into the original's material
            
            //If we need to append the diffuse suffix and indeed didn't find a suffix on the name, do that here
            if(%assetConfigObj.UseDiffuseSuffixOnOriginImg == 1 && %foundSuffixType $= "")
            {
               %diffuseToken = getToken(%assetItem.importConfig.DiffuseTypeSuffixes, ",", 0);
               %assetItem.AssetName = %assetItem.AssetName @ %diffuseToken;
               
               if(%assetItem.importConfig.PopulateMaterialMaps == 1)
                  %materialAsset.diffuseImageAsset = %assetItem;
            }
            else if(%foundSuffixType !$= "")
            {
               //otherwise, if we have some sort of suffix, we'll want to figure out if we've already got an existing material, and should append to it  
               
               if(%assetItem.importConfig.PopulateMaterialMaps == 1)
               {
                  if(%foundSuffixType $= "diffuse")
                     %materialAsset.diffuseImageAsset = %assetItem;
                  else if(%foundSuffixType $= "normal")
                     %materialAsset.normalImageAsset = %assetItem;
                  else if(%foundSuffixType $= "metalness")
                     %materialAsset.metalnessImageAsset = %assetItem;
                  else if(%foundSuffixType $= "roughness")
                     %materialAsset.roughnessImageAsset = %assetItem;
                     else if(%foundSuffixType $= "specular")
                     %materialAsset.specularImageAsset = %assetItem;
                  else if(%foundSuffixType $= "AO")
                     %materialAsset.AOImageAsset = %assetItem;
                  else if(%foundSuffixType $= "composite")
                     %materialAsset.compositeImageAsset = %assetItem;
               }
            }
         }
      }
      
      AssetBrowser.importAssetUnprocessedListArray.erase(0);    
      //Been processed, so add it to our final list
      AssetBrowser.importAssetFinalListArray.add(%assetItem);
   }
}

function ImportAssetWindow::findImportingAssetByName(%this, %assetName)
{
   %unprocessedCount = AssetBrowser.importAssetUnprocessedListArray.count();
   for(%i=0; %i < %unprocessedCount; %i++)
   {
      %asset = AssetBrowser.importAssetUnprocessedListArray.getKey(%i);
      
      if(%asset.cleanAssetName $= %assetName)
      {
         return %asset;
      }
   }
   
   %processedCount = AssetBrowser.importAssetFinalListArray.count();
   for(%i=0; %i < %processedCount; %i++)
   {
      %asset = AssetBrowser.importAssetFinalListArray.getKey(%i);
      
      if(%asset.cleanAssetName $= %assetName)
      {
         return %asset;
      }
   }
   
   return 0;
}

function ImportAssetWindow::parseImageSuffixes(%this, %assetItem)
{
   //diffuse
   %suffixCount = getTokenCount(%assetItem.importConfig.DiffuseTypeSuffixes, ",;");
   for(%sfx = 0; %sfx < %suffixCount; %sfx++)
   {
      %suffixToken = getToken(%assetItem.importConfig.DiffuseTypeSuffixes, ",;", %sfx);
      if(strIsMatchExpr("*"@%suffixToken, %assetItem.AssetName))
      {
         %assetItem.imageSuffixType = %suffixToken;
         return "diffuse";
      }
   }
   
   //normal
   %suffixCount = getTokenCount(%assetItem.importConfig.NormalTypeSuffixes, ",;");
   for(%sfx = 0; %sfx < %suffixCount; %sfx++)
   {
      %suffixToken = getToken(%assetItem.importConfig.NormalTypeSuffixes, ",;", %sfx);
      if(strIsMatchExpr("*"@%suffixToken, %assetItem.AssetName))
      {
         %assetItem.imageSuffixType = %suffixToken;
         return "normal";
      }
   }
   
   //roughness
   %suffixCount = getTokenCount(%assetItem.importConfig.RoughnessTypeSuffixes, ",;");
   for(%sfx = 0; %sfx < %suffixCount; %sfx++)
   {
      %suffixToken = getToken(%assetItem.importConfig.RoughnessTypeSuffixes, ",;", %sfx);
      if(strIsMatchExpr("*"@%suffixToken, %assetItem.AssetName))
      {
         %assetItem.imageSuffixType = %suffixToken;
         return "roughness";
      }
   }
   
   //Ambient Occlusion
   %suffixCount = getTokenCount(%assetItem.importConfig.AOTypeSuffixes, ",;");
   for(%sfx = 0; %sfx < %suffixCount; %sfx++)
   {
      %suffixToken = getToken(%assetItem.importConfig.AOTypeSuffixes, ",;", %sfx);
      if(strIsMatchExpr("*"@%suffixToken, %assetItem.AssetName))
      {
         %assetItem.imageSuffixType = %suffixToken;
         return "AO";
      }
   }
   
   //metalness
   %suffixCount = getTokenCount(%assetItem.importConfig.MetalnessTypeSuffixes, ",;");
   for(%sfx = 0; %sfx < %suffixCount; %sfx++)
   {
      %suffixToken = getToken(%assetItem.importConfig.MetalnessTypeSuffixes, ",;", %sfx);
      if(strIsMatchExpr("*"@%suffixToken, %assetItem.AssetName))
      {
         %assetItem.imageSuffixType = %suffixToken;
         return "metalness";
      }
   }
   
   //composite
   %suffixCount = getTokenCount(%assetItem.importConfig.CompositeTypeSuffixes, ",;");
   for(%sfx = 0; %sfx < %suffixCount; %sfx++)
   {
      %suffixToken = getToken(%assetItem.importConfig.CompositeTypeSuffixes, ",;", %sfx);
      if(strIsMatchExpr("*"@%suffixToken, %assetItem.AssetName))
      {
         %assetItem.imageSuffixType = %suffixToken;
         return "composite";
      }
   }
   
   //specular
   %suffixCount = getTokenCount(%assetItem.importConfig.SpecularTypeSuffixes, ",;");
   for(%sfx = 0; %sfx < %suffixCount; %sfx++)
   {
      %suffixToken = getToken(%assetItem.importConfig.SpecularTypeSuffixes, ",;", %sfx);
      if(strIsMatchExpr("*"@%suffixToken, %assetItem.AssetName))
      {
         %assetItem.imageSuffixType = %suffixToken;
         return "specular";
      }
   }
   
   return "";
}

function ImportAssetWindow::parseImagePathSuffixes(%this, %filePath)
{
   //diffuse
   %suffixCount = getTokenCount(%assetItem.importConfig.DiffuseTypeSuffixes, ",");
   for(%sfx = 0; %sfx < %suffixCount; %sfx++)
   {
      %suffixToken = getToken(%assetItem.importConfig.DiffuseTypeSuffixes, ",", %sfx);
      if(strIsMatchExpr("*"@%suffixToken, %filePath))
      {
         return "diffuse";
      }
   }
   
   //normal
   %suffixCount = getTokenCount(%assetItem.importConfig.NormalTypeSuffixes, ",");
   for(%sfx = 0; %sfx < %suffixCount; %sfx++)
   {
      %suffixToken = getToken(%assetItem.importConfig.NormalTypeSuffixes, ",", %sfx);
      if(strIsMatchExpr("*"@%suffixToken, %filePath))
      {
         return "normal";
      }
   }
   
   //roughness
   %suffixCount = getTokenCount(%assetItem.importConfig.RoughnessTypeSuffixes, ",");
   for(%sfx = 0; %sfx < %suffixCount; %sfx++)
   {
      %suffixToken = getToken(%assetItem.importConfig.RoughnessTypeSuffixes, ",", %sfx);
      if(strIsMatchExpr("*"@%suffixToken, %filePath))
      {
         return "roughness";
      }
   }
   
   //Ambient Occlusion
   %suffixCount = getTokenCount(%assetItem.importConfig.AOTypeSuffixes, ",");
   for(%sfx = 0; %sfx < %suffixCount; %sfx++)
   {
      %suffixToken = getToken(%assetItem.importConfig.AOTypeSuffixes, ",", %sfx);
      if(strIsMatchExpr("*"@%suffixToken, %filePath))
      {
         return "AO";
      }
   }
   
   //metalness
   %suffixCount = getTokenCount(%assetItem.importConfig.MetalnessTypeSuffixes, ",");
   for(%sfx = 0; %sfx < %suffixCount; %sfx++)
   {
      %suffixToken = getToken(%assetItem.importConfig.MetalnessTypeSuffixes, ",", %sfx);
      if(strIsMatchExpr("*"@%suffixToken, %filePath))
      {
         return "metalness";
      }
   }
   
   //composite
   %suffixCount = getTokenCount(%assetItem.importConfig.CompositeTypeSuffixes, ",");
   for(%sfx = 0; %sfx < %suffixCount; %sfx++)
   {
      %suffixToken = getToken(%assetItem.importConfig.CompositeTypeSuffixes, ",", %sfx);
      if(strIsMatchExpr("*"@%suffixToken, %filePath))
      {
         return "composite";
      }
   }
   
   //specular
   %suffixCount = getTokenCount(%assetItem.importConfig.SpecularTypeSuffixes, ",");
   for(%sfx = 0; %sfx < %suffixCount; %sfx++)
   {
      %suffixToken = getToken(%assetItem.importConfig.SpecularTypeSuffixes, ",", %sfx);
      if(strIsMatchExpr("*"@%suffixToken, %filePath))
      {
         return "specular";
      }
   }
   
   return "";
}

function ImportAssetWindow::refresh(%this)
{
   ImportingAssetList.clear();
   
   //Go through and process any newly, unprocessed assets
   %this.processNewImportAssets();
   
   if(AssetBrowser.importAssetUnprocessedListArray.count() == 0)
   {
      //We've processed them all, prep the assets for actual importing
      //Initial set of assets
      %assetCount = AssetBrowser.importAssetFinalListArray.count();
      
      for(%i=0; %i < %assetCount; %i++)
      {
         %assetItem = AssetBrowser.importAssetFinalListArray.getKey(%i);
         %assetType = %assetItem.assetType;
         %filePath = %assetItem.filePath;
         %assetName = %assetItem.assetName;
         
         //validate
         %this.validateAsset(%assetItem);
         
         //Once validated, attempt any fixes for issues
         %this.resolveIssue(%assetItem);
         
         //Make sure we size correctly
         ImportingAssetList.extent.x = ImportingAssetList.getParent().extent.x - 15;
         
         //create!
         %width = mRound(mRound(ImportingAssetList.extent.x) / 2);
         %height = 20;
         %indent = %assetItem.parentDepth * 16;
         %toolTip = "";
         
         %iconPath = "tools/gui/images/iconInformation";
         %configCommand = "ImportAssetOptionsWindow.editImportSettings(" @ %assetItem @ ");";
         
         if(%assetType $= "Model" || %assetType $= "Animation" || %assetType $= "Image" || %assetType $= "Sound")
         {
            /*if(%assetItem.status $= "Error")
            {
               %iconPath = "tools/gui/images/iconError";
               %configCommand = "ImportAssetOptionsWindow.findMissingFile(" @ %assetItem @ ");";
            }
            else*/
            if(%assetItem.status $= "Warning")
            {
               %iconPath = "tools/gui/images/iconWarn";
               %configCommand = "ImportAssetOptionsWindow.fixIssues(" @ %assetItem @ ");";
               
               if(%assetItem.statusType $= "DuplicateAsset" || %assetItem.statusType $= "DuplicateImportAsset")
                  %assetName = %assetItem.assetName @ " <Duplicate Asset>";
            }
            
            %toolTip = %assetItem.statusInfo;
         }
         else
         {
            if(%assetItem.status $= "Error")
            {
               %iconPath = "tools/gui/images/iconError";
               %configCommand = "";//"ImportAssetOptionsWindow.findMissingFile(" @ %assetItem @ ");";
            }
            else if(%assetItem.status $= "Warning")
            {
               %iconPath = "tools/gui/images/iconWarn";
               %configCommand = "";//"ImportAssetOptionsWindow.fixIssues(" @ %assetItem @ ");";
               
               if(%assetItem.statusType $= "DuplicateAsset" || %assetItem.statusType $= "DuplicateImportAsset")
                  %assetName = %assetItem.assetName @ " <Duplicate Asset>";
            }
         }
         
         %inputCellPos = %indent;
         %inputCellWidth = (ImportingAssetList.extent.x * 0.3) - %indent;
         
         %filePathBtnPos = %inputCellPos + %inputCellWidth - %height;
         
         %assetNameCellPos = %inputCellPos + %inputCellWidth;
         %assetNameCellWidth = ImportingAssetList.extent.x * 0.3;
         
         %assetTypeCellPos = %assetNameCellPos + %assetNameCellWidth;
         %assetTypeCellWidth = ImportingAssetList.extent.x * 0.3;
         
         %configBtnPos = %assetTypeCellPos + %assetTypeCellWidth - (%height * 2);
         %configBtnWidth = %height;
         
         %delBtnPos = %assetTypeCellPos + %assetTypeCellWidth - %height;
         %delBtnWidth = %height;
         
         %inputField = %filePath;
         
         //Check if it's a generated type, like materials
         %inputPathProfile = ToolsGuiTextEditProfile;
         %generatedField = false;
         if(%assetType $= "Material")
         {
            %inputField = "(Generated)";
            %generatedField = true;
         }
         else
         {
            //nope, so check that it's a valid file path. If not, flag it as such
            if(%assetItem.status $= "Error")
            {
               %inputField = "File not found!";
               %inputPathProfile = ToolsGuiTextEditErrorProfile;
            }
         }
         
         %importEntry = new GuiControl()
         {
            position = "0 0";
            extent = ImportingAssetList.extent.x SPC %height;
            horzSizing = "width";
            vertSizing = "bottom";
            
            new GuiTextEditCtrl()
            {
               Text = %inputField; 
               position = %inputCellPos SPC "0";
               extent = %inputCellWidth SPC %height;
               internalName = "InputPath";
               active = false;
               profile = %inputPathProfile;
               horzSizing = "width";
               vertSizing = "bottom";
            };
            
            new GuiButtonCtrl()
            {
               position = %filePathBtnPos SPC "0";
               extent = %height SPC %height;
               command = "ImportAssetOptionsWindow.findMissingFile(" @ %assetItem @ ");";
               text = "...";
               internalName = "InputPathButton";
               tooltip = %toolTip;
               visible = !%generatedField;
               horzSizing = "width";
               vertSizing = "bottom";
            };
            
            new GuiTextEditCtrl()
            {
              Text = %assetName; 
              position = %assetNameCellPos SPC "0";
              extent = %assetNameCellWidth SPC %height;
              internalName = "AssetName";
              horzSizing = "width";
               vertSizing = "bottom";
            };
            
            new GuiTextEditCtrl()
            {
              Text = %assetType; 
              position = %assetTypeCellPos SPC "0";
              extent = %assetTypeCellWidth SPC %height;
              active = false;
              internalName = "AssetType";
              horzSizing = "width";
               vertSizing = "bottom";
            };
            
            new GuiBitmapButtonCtrl()
            {
               position = %configBtnPos SPC "0";
               extent = %height SPC %height;
               command = %configCommand;
               bitmap = %iconPath;
               tooltip = %toolTip;
               horzSizing = "width";
               vertSizing = "bottom";
            };
            new GuiBitmapButtonCtrl()
            {
               position = %delBtnPos SPC "0";
               extent = %height SPC %height;
               command = "ImportAssetOptionsWindow.deleteImportingAsset(" @ %assetItem @ ");";
               bitmap = "tools/gui/images/iconDelete";
               horzSizing = "width";
               vertSizing = "bottom";
            };
         };
         
         ImportingAssetList.add(%importEntry);
      }
   }
   else
   {
      //Continue processing
      %this.refresh();  
   }
}
//

function ImportAssetWindow::validateAssets(%this)
{
   %assetCount = AssetBrowser.importAssetFinalListArray.count();
   %moduleName = ImportAssetModuleList.getText();
   %assetQuery = new AssetQuery();
   
   %hasIssues = false;
   
   //First, check the obvious: name collisions. We should have no asset that shares a similar name.
   //If we do, prompt for it be renamed first before continuing
   
   for(%i=0; %i < %assetCount; %i++)
   {
      %assetItemA = AssetBrowser.importAssetFinalListArray.getKey(%i);
      
      //First, check our importing assets for name collisions
      for(%j=0; %j < %assetCount; %j++)
      {
         %assetItemB = AssetBrowser.importAssetFinalListArray.getKey(%j);
         if( (%assetItemA.assetName $= %assetItemB.assetName) && (%i != %j) )
         {
            //yup, a collision, prompt for the change and bail out
            /*MessageBoxOK( "Error!", "Duplicate asset names found with importing assets!\nAsset \"" @ 
               %assetItemA.assetName @ "\" of type \"" @ %assetItemA.assetType @ "\" and \"" @
               %assetItemB.assetName @ "\" of type \"" @ %assetItemB.assetType @ "\" have matching names.\nPlease rename one of them and try again!");*/
               
            %assetItemA.status = "Warning";
            %assetItemA.statusType = "DuplicateImportAsset";
            %assetItemA.statusInfo = "Duplicate asset names found with importing assets!\nAsset \"" @ 
               %assetItemA.assetName @ "\" of type \"" @ %assetItemA.assetType @ "\" and \"" @
               %assetItemB.assetName @ "\" of type \"" @ %assetItemB.assetType @ "\" have matching names.\nPlease rename one of them and try again!";
               
            %hasIssues = true;
         }
      }
      
      //No collisions of for this name in the importing assets. Now, check against the existing assets in the target module
      if(!AssetBrowser.isAssetReImport)
      {
         %numAssetsFound = AssetDatabase.findAllAssets(%assetQuery);

         %foundCollision = false;
         for( %f=0; %f < %numAssetsFound; %f++)
         {
            %assetId = %assetQuery.getAsset(%f);
             
            //first, get the asset's module, as our major categories
            %module = AssetDatabase.getAssetModule(%assetId);
            
            %testModuleName = %module.moduleId;
            
            //These are core, native-level components, so we're not going to be messing with this module at all, skip it
            if(%moduleName !$= %testModuleName)
               continue;

            %testAssetName = AssetDatabase.getAssetName(%assetId);
            
            if(%testAssetName $= %assetItemA.assetName)
            {
               %foundCollision = true;
               
               %assetItemA.status = "Warning";
               %assetItemA.statusType = "DuplicateAsset";
               %assetItemA.statusInfo = "Duplicate asset names found with the target module!\nAsset \"" @ 
                  %assetItemA.assetName @ "\" of type \"" @ %assetItemA.assetType @ "\" has a matching name.\nPlease rename it and try again!";
                  
               break;            
            }
         }
         
         if(%foundCollision == true)
         {
            %hasIssues = true;
            
            //yup, a collision, prompt for the change and bail out
            /*MessageBoxOK( "Error!", "Duplicate asset names found with the target module!\nAsset \"" @ 
               %assetItemA.assetName @ "\" of type \"" @ %assetItemA.assetType @ "\" has a matching name.\nPlease rename it and try again!");*/
               
            //%assetQuery.delete();
            //return false;
         }
      }
      
      //Check if we were given a file path(so not generated) but somehow isn't a valid file
      if(%assetItemA.filePath !$= "" && !isFile(%assetItemA.filePath))
      {
         %hasIssues = true;  
         %assetItemA.status = "error";
         %assetItemA.statusType = "MissingFile";
         %assetItemA.statusInfo = "Unable to find file to be imported. Please select asset file.";
      }
   }
   
   //Clean up our queries
   %assetQuery.delete();
   
   if(%hasIssues)
      return false;
   else
      return true;
}

function ImportAssetWindow::ImportAssets(%this)
{
   //do the actual importing, now!
   %assetCount = AssetBrowser.importAssetFinalListArray.count();
   
   //get the selected module data
   %moduleName = ImportAssetModuleList.getText();
   
   %module = ModuleDatabase.findModule(%moduleName, 1);
   
   if(!isObject(%module))
   {
      MessageBoxOK( "Error!", "No module selected. You must select or create a module for the assets to be added to.");
      return;
   }
   
   /*if(!%this.validateAssets())
   {
      //Force a refresh, as some things may have changed, such as errors and failure info!
      refresh();
      
      return;
   }*/
   
   for(%i=0; %i < %assetCount; %i++)
   {
      %assetItem = AssetBrowser.importAssetFinalListArray.getKey(%i);
      %assetType = %assetItem.AssetType;
      %filePath = %assetItem.filePath;
      %assetName = %assetItem.assetName;
      %assetImportSuccessful = false;
      %assetId = %moduleName@":"@%assetName;
      
      if(%assetType $= "Image")
      {
         %assetPath = "data/" @ %moduleName @ "/Images";
         %assetFullPath = %assetPath @ "/" @ fileName(%filePath);
         
         %newAsset = new ImageAsset()
         {
            assetName = %assetName;
            versionId = 1;
            imageFile = %assetFullPath;
            originalFilePath = %filePath;
         };
         
         %assetImportSuccessful = TAMLWrite(%newAsset, %assetPath @ "/" @ %assetName @ ".asset.taml"); 
         
         //and copy the file into the relevent directory
         %doOverwrite = !AssetBrowser.isAssetReImport;
         if(!pathCopy(%filePath, %assetFullPath, %doOverwrite))
         {
            error("Unable to import asset: " @ %filePath);
         }
      }
      else if(%assetType $= "Model")
      {
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
         
         %dependencyCount = getWordCount(%assetItem.dependencies);
         for(%d=0; %d < %dependencyCount; %d++)
         {
            %dependencyAssetItem = getWord(%assetItem.dependencies, %d);
            
            %depAssetType = %dependencyAssetItem.assetType;
            if(%depAssetType $= "Material")
            {
               %matSet = "%newAsset.materialSlot"@%d@"=\"@Asset="@%moduleName@":"@%dependencyAssetItem.assetName@"\";";
               eval(%matSet);
            }
            if(%depAssetType $= "Animation")
            {
               %matSet = "%newAsset.animationSequence"@%d@"=\"@Asset="@%moduleName@":"@%dependencyAssetItem.assetName@"\";";
               eval(%matSet);
            }
         }
         
         %assetImportSuccessful = TAMLWrite(%newAsset, %assetPath @ "/" @ %assetName @ ".asset.taml"); 
         
         //and copy the file into the relevent directory
         %doOverwrite = !AssetBrowser.isAssetReImport;
         if(!pathCopy(%filePath, %assetFullPath, %doOverwrite))
         {
            error("Unable to import asset: " @ %filePath);
         }
         
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
      }
      else if(%assetType $= "Animation")
      {
         %assetPath = "data/" @ %moduleName @ "/ShapeAnimations";
         %assetFullPath = %assetPath @ "/" @ fileName(%filePath);
         
         %newAsset = new ShapeAnimationAsset()
         {
            assetName = %assetName;
            versionId = 1;
            fileName = %assetFullPath;
            originalFilePath = %filePath;
            animationFile = %assetFullPath;
            animationName = %assetName;
            startFrame = 0;
            endFrame = -1;
            padRotation = false;
            padTransforms = false;
         };

         %assetImportSuccessful = TAMLWrite(%newAsset, %assetPath @ "/" @ %assetName @ ".asset.taml"); 
         
         //and copy the file into the relevent directory
         %doOverwrite = !AssetBrowser.isAssetReImport;
         if(!pathCopy(%filePath, %assetFullPath, %doOverwrite))
         {
            error("Unable to import asset: " @ %filePath);
         }
      }
      else if(%assetType $= "Sound")
      {
         %assetPath = "data/" @ %moduleName @ "/Sounds";
         %assetFullPath = %assetPath @ "/" @ fileName(%filePath);
         
         %newAsset = new SoundAsset()
         {
            assetName = %assetName;
            versionId = 1;
            fileName = %assetFullPath;
            originalFilePath = %filePath;
         };
         
         %assetImportSuccessful = TAMLWrite(%newAsset, %assetPath @ "/" @ %assetName @ ".asset.taml"); 
         
         //and copy the file into the relevent directory
         %doOverwrite = !AssetBrowser.isAssetReImport;
         if(!pathCopy(%filePath, %assetFullPath, %doOverwrite))
         {
            error("Unable to import asset: " @ %filePath);
         }
      }
      else if(%assetType $= "Material")
      {
         %assetPath = "data/" @ %moduleName @ "/materials";
         %tamlpath = %assetPath @ "/" @ %assetName @ ".asset.taml";
         %sgfPath = %assetPath @ "/" @ %assetName @ ".sgf";
         %scriptPath = %assetPath @ "/" @ %assetName @ ".cs";
         
         %newAsset = new MaterialAsset()
         {
            assetName = %assetName;
            versionId = 1;
            shaderGraph = %sgfPath;
            scriptFile = %scriptPath;
            originalFilePath = %filePath;
            materialDefinitionName = %assetName;
         };
         
         %dependencyCount = getWordCount(%assetItem.dependencies);
         for(%d=0; %d < %dependencyCount; %d++)
         {
            %dependencyAssetItem = getWord(%assetItem.dependencies, %d);
            
            %depAssetType = %dependencyAssetItem.assetType;
            if(%depAssetType $= "Image")
            {
               %matSet = "%newAsset.imageMap"@%d@"=\"@Asset="@%moduleName@":"@%dependencyAssetItem.assetName@"\";";
               eval(%matSet);
            }
         }
         
         %assetImportSuccessful = TamlWrite(%newAsset, %tamlpath);
         
         %file = new FileObject();
   
         if(%file.openForWrite(%scriptPath))
         {
            %file.writeline("//--- OBJECT WRITE BEGIN ---");
            %file.writeline("singleton Material(" @ %assetName @ ") {");
            
            //TODO: pass along the shape's target material for this just to be sure
            %file.writeLine("   mapTo = \"" @ %assetName @ "\";"); 
            
            if(%assetItem.diffuseImageAsset !$= "")
            {
               %diffuseAssetPath = "data/" @ %moduleName @ "/Images/" @ fileName(%assetItem.diffuseImageAsset.filePath);
               %file.writeline("   DiffuseMap[0] = \"" @ %diffuseAssetPath @"\";");
               %file.writeline("   DiffuseMapAsset[0] = \"" @ %moduleName @ ":" @ %assetItem.diffuseImageAsset.assetName @"\";");
            }
            if(%assetItem.normalImageAsset)
            {
               %normalAssetPath = "data/" @ %moduleName @ "/Images/" @ fileName(%assetItem.normalImageAsset.filePath);
               %file.writeline("   NormalMap[0] = \"" @ %normalAssetPath @"\";");
               %file.writeline("   NormalMapAsset[0] = \"" @ %moduleName @ ":" @ %assetItem.normalImageAsset.assetName @"\";");
            }
            /*if(%assetItem.specularImageAsset)
            {
               %file.writeline("   SpecularMap[0] = \"" @ %assetItem.specularImageAsset.filePath @"\";");
               %file.writeline("   SpecularMapAsset[0] = \"" @ %moduleName @ ":" @ %assetItem.specularImageAsset.assetName @"\";");
            }*/
            if(%assetItem.roughnessImageAsset)
            {
               %file.writeline("   RoughMap[0] = \"" @ %assetItem.roughnessImageAsset.filePath @"\";");
               %file.writeline("   RoughMapAsset[0] = \"" @ %moduleName @ ":" @ %assetItem.roughnessImageAsset.assetName @"\";");
            }
            if(%assetItem.smoothnessImageAsset)
            {
               %file.writeline("   SmoothnessMap[0] = \"" @ %assetItem.smoothnessImageAsset.filePath @"\";");
               %file.writeline("   SmoothnessMapAsset[0] = \"" @ %moduleName @ ":" @ %assetItem.smoothnessImageAsset.assetName @"\";");
            }
            if(%assetItem.metalnessImageAsset)
            {
               %file.writeline("   MetalMap[0] = \"" @ %assetItem.metalnessImageAsset.filePath @"\";");
               %file.writeline("   MetalMapAsset[0] = \"" @ %moduleName @ ":" @ %assetItem.metalnessImageAsset.assetName @"\";");
            }
            if(%assetItem.AOImageAsset)
            {
               %file.writeline("   AOMap[0] = \"" @ %assetItem.AOImageAsset.filePath @"\";");
               %file.writeline("   AOMapAsset[0] = \"" @ %moduleName @ ":" @ %assetItem.AOImageAsset.assetName @"\";");
            }
            if(%assetItem.compositeImageAsset)
            {
               %file.writeline("   CompositeMap[0] = \"" @ %assetItem.compositeImageAsset.filePath @"\";");
               %file.writeline("   CompositeMapAsset[0] = \"" @ %moduleName @ ":" @ %assetItem.compositeImageAsset.assetName @"\";");
            }
            %file.writeline("};");
            %file.writeline("//--- OBJECT WRITE END ---");
            
            %file.close();
         }
      }
      else if(%assetType $= "Script")
      {
         %assetPath = "data/" @ %moduleName @ "/Scripts";
         %assetFullPath = %assetPath @ "/" @ fileName(%filePath);
         
         %newAsset = new ScriptAsset()
         {
            assetName = %assetName;
            versionId = 1;
            scriptFilePath = %assetFullPath;
            isServerSide = true;
            originalFilePath = %filePath;
         };
         
         %assetImportSuccessful = TAMLWrite(%newAsset, %assetPath @ "/" @ %assetName @ ".asset.taml"); 
         
         //and copy the file into the relevent directory
         %doOverwrite = !AssetBrowser.isAssetReImport;
         if(!pathCopy(%filePath, %assetFullPath, %doOverwrite))
         {
            error("Unable to import asset: " @ %filePath);
         }
      }
      else if(%assetType $= "GUI")
      {
         %assetPath = "data/" @ %moduleName @ "/GUIs";
         %assetFullPath = %assetPath @ "/" @ fileName(%filePath);
         
         %newAsset = new GUIAsset()
         {
            assetName = %assetName;
            versionId = 1;
            GUIFilePath = %assetFullPath;
            scriptFilePath = "";
            originalFilePath = %filePath;
         };
         
         %assetImportSuccessful = TAMLWrite(%newAsset, %assetPath @ "/" @ %assetName @ ".asset.taml"); 
         
         //and copy the file into the relevent directory
         %doOverwrite = !AssetBrowser.isAssetReImport;
         if(!pathCopy(%filePath, %assetFullPath, %doOverwrite))
         {
            error("Unable to import asset: " @ %filePath);
         }
      }
      
      if(%assetImportSuccessful)
      {
         %moduleDef = ModuleDatabase.findModule(%moduleName,1);
         
         if(!AssetBrowser.isAssetReImport)
            AssetDatabase.addDeclaredAsset(%moduleDef, %assetPath @ "/" @ %assetName @ ".asset.taml");
         else
            AssetDatabase.refreshAsset(%assetId);
      }
   }
   
   //force an update of any and all modules so we have an up-to-date asset list
   AssetBrowser.loadFilters();
   AssetBrowser.refreshPreviews();
   Canvas.popDialog(AssetImportCtrl);
   AssetBrowser.isAssetReImport = false;
}

//
function ImportAssetWindow::validateAsset(%this, %assetItem)
{
   %assetCount = AssetBrowser.importAssetFinalListArray.count();
   %moduleName = ImportAssetModuleList.getText();
   
   %hasIssues = false;
   
   //First, check the obvious: name collisions. We should have no asset that shares a similar name.
   //If we do, prompt for it be renamed first before continuing
   
   for(%i=0; %i < %assetCount; %i++)
   {
      %assetItemA = AssetBrowser.importAssetFinalListArray.getKey(%i);
      
      if( (%assetItemA.assetName $= %assetItem.assetName) && (%assetItemA.getId() != %assetItem.getId()) )
      {
         //yup, a collision, prompt for the change and bail out
         /*MessageBoxOK( "Error!", "Duplicate asset names found with importing assets!\nAsset \"" @ 
            %assetItemA.assetName @ "\" of type \"" @ %assetItemA.assetType @ "\" and \"" @
            %assetItemB.assetName @ "\" of type \"" @ %assetItemB.assetType @ "\" have matching names.\nPlease rename one of them and try again!");*/
            
         %assetItem.status = "Warning";
         %assetItem.statusType = "DuplicateImportAsset";
         %assetItem.statusInfo = "Duplicate asset names found with importing assets!\nAsset \"" @ 
            %assetItemA.assetName @ "\" of type \"" @ %assetItemA.assetType @ "\" and \"" @
            %assetItem.assetName @ "\" of type \"" @ %assetItem.assetType @ "\" have matching names.\nPlease rename one of them and try again!";
            
         %hasIssues = true;
         return false;
      }
   }

   //No collisions of for this name in the importing assets. Now, check against the existing assets in the target module
   if(!AssetBrowser.isAssetReImport)
   {
      %assetQuery = new AssetQuery();
      
      %numAssetsFound = AssetDatabase.findAllAssets(%assetQuery);

      %foundCollision = false;
      for( %f=0; %f < %numAssetsFound; %f++)
      {
         %assetId = %assetQuery.getAsset(%f);
          
         //first, get the asset's module, as our major categories
         %module = AssetDatabase.getAssetModule(%assetId);
         
         %testModuleName = %module.moduleId;
         
         //These are core, native-level components, so we're not going to be messing with this module at all, skip it
         if(%moduleName !$= %testModuleName)
            continue;

         %testAssetName = AssetDatabase.getAssetName(%assetId);
         
         if(%testAssetName $= %assetItem.assetName)
         {
            %foundCollision = true;
            
            %assetItem.status = "Warning";
            %assetItem.statusType = "DuplicateAsset";
            %assetItem.statusInfo = "Duplicate asset names found with the target module!\nAsset \"" @ 
               %assetItem.assetName @ "\" of type \"" @ %assetItem.assetType @ "\" has a matching name.\nPlease rename it and try again!";
               
            //Clean up our queries
            %assetQuery.delete();
      
            return false;            
         }
      }
      
      if(%foundCollision == true)
      {
         %hasIssues = true;
         
         //yup, a collision, prompt for the change and bail out
         /*MessageBoxOK( "Error!", "Duplicate asset names found with the target module!\nAsset \"" @ 
            %assetItemA.assetName @ "\" of type \"" @ %assetItemA.assetType @ "\" has a matching name.\nPlease rename it and try again!");*/
            
         //%assetQuery.delete();
         //return false;
      }
      
      //Clean up our queries
      %assetQuery.delete();
   }
      
   //Check if we were given a file path(so not generated) but somehow isn't a valid file
   if(%assetItem.filePath !$= "" && !isFile(%assetItem.filePath))
   {
      %hasIssues = true;  
      %assetItem.status = "error";
      %assetItem.statusType = "MissingFile";
      %assetItem.statusInfo = "Unable to find file to be imported. Please select asset file.";
      
      return false;
   }
   
   return true;
}

function ImportAssetWindow::resolveIssue(%this, %assetItem)
{
   if(%assetItem.status !$= "Warning")
      return;
      
   //Ok, we actually have a warning, so lets resolve
   if(%assetItem.statusType $= "DuplicateImportAsset" || %assetItem.statusType $= "DuplicateAsset")
   {
      
   }
   else if(%assetItem.statusType $= "MissingFile")
   {
      %this.findMissingFile(%assetItem);
   }
}
//

//
function ImportAssetModuleList::onWake(%this)
{
   %this.refresh();
}

function ImportAssetModuleList::refresh(%this)
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
