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
   ImportAssetWindow.refresh();
   ImportAssetWindow.selectWindow();
   
   // Update object library
   GuiFormManager::SendContentMessage($LBCreateSiderBar, %this, "refreshAll 1");
   
   if(ImportAssetWindow.importConfigsList.count() == 0)
   {
      MessageBoxOK( "Warning", "No base import config. Please create an import configuration set to simplify asset importing.");
   }
}

function AssetBrowser::addImportingAsset( %this, %assetType, %filePath, %parentAssetItem )
{
   %assetName = fileBase(%filePath);
   %filePath = filePath(%filePath) @ "/" @ fileBase(%filePath) @ fileExt(%filePath); //sanitize the file path
   
   //Add to our main list
   %assetItem = new ScriptObject()
   {
      assetType = %assetType;
      filePath = %filePath;
      assetName = %assetName;
      dirty  = true;
      parentAssetItem = %parentAssetItem;
   };
   
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
      
      %this.importAssetUnprocessedListArray.insert(%assetItem, "", %parentIndex + 1);
   }
   
   return %assetItem;
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
   
   ImportAssetConfigList.setSelected(0);
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

function ImportAssetWindow::refresh(%this)
{
   ImportingAssetList.clear();
   
   %unprocessedCount = AssetBrowser.importAssetUnprocessedListArray.count();
   while(AssetBrowser.importAssetUnprocessedListArray.count() > 0)
   {
      %assetItem = AssetBrowser.importAssetUnprocessedListArray.getKey(0);  
      
      %assetConfigObj = ImportAssetWindow.activeImportConfig.clone();
      %assetConfigObj.assetIndex = %i;
      %assetConfigObj.assetName = %assetItem.assetName;
      %assetItem.importConfig = %assetConfigObj;
      
      if(%assetItem.assetType $= "Model")
      {
         %shapeInfo = GetShapeInfo(%assetItem.filePath);
         %matItem = %shapeInfo.findItemByName("Materials");
         %matCount = %shapeInfo.getItemValue(%matItem);
         
         if(%assetConfigObj.importMaterials == 1 && %matCount > 0)
         {
            %materialItem = %shapeInfo.getChild(%matItem);
            
            %matName = %shapeInfo.getItemText(%materialItem);
            
            %filePath = %shapeInfo.getItemValue(%materialItem);
            if(%filePath !$= "")
               AssetBrowser.addImportingAsset("Material", %filePath, %assetItem);
            
            %materialItem = %shapeInfo.getNextSibling(%materialItem);
            while(%materialItem != 0)
            {
               %matName = %shapeInfo.getItemText(%materialItem);
               %filePath = %shapeInfo.getItemValue(%materialItem);
               if(%filePath !$= "")
                  AssetBrowser.addImportingAsset("Material", %filePath, %assetItem);
                  
               %materialItem = %shapeInfo.getNextSibling(%materialItem);
            }
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
               %diffuseAsset = AssetBrowser.addImportingAsset("Image", %fileDir @ "/" @ %filename @ %fileExt, %assetItem);
               %assetItem.diffuseImageAsset = %diffuseAsset;
               
               if(%assetItem.importConfig.UseDiffuseSuffixOnOriginImg == 1)
               {
                  %diffuseToken = getToken(%assetItem.importConfig.DiffuseTypeSuffixes, ",", 0);
                  %diffuseAsset.AssetName = %diffuseAsset.AssetName @ %diffuseToken;
               }
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
            %filePath = %assetItem.filePath;
            if(%filePath !$= "")
               %materialAsset = AssetBrowser.addImportingAsset("Material", %filePath, %assetItem);
            
            %materialAsset.diffuseImageAsset = %assetItem;
            
            if(%assetConfigObj.UseDiffuseSuffixOnOriginImg == 1)
            {
               %diffuseToken = getToken(%assetItem.importConfig.DiffuseTypeSuffixes, ",", 0);
               %assetItem.AssetName = %assetItem.AssetName @ %diffuseToken;
            }
         }
      }
      
      AssetBrowser.importAssetUnprocessedListArray.erase(0);    
      //Been processed, so add it to our final list
      AssetBrowser.importAssetFinalListArray.add(%assetItem);
   }
   
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
         
         //%assetConfigObj = ImportAssetWindow.activeImportConfig.clone();
         //%assetConfigObj.assetIndex = %i;
         //%assetConfigObj.assetName = %assetName;
         //%this.importAssetConfigListArray.add(%assetName, %assetConfigObj);
         
         //Make sure we size correctly
         ImportingAssetList.extent.x = ImportingAssetList.getParent().extent.x;
         
         //create!
         %width = mRound(ImportingAssetList.extent.x / 2);
         %height = 20;
         %indent = %assetItem.parentDepth * 16;
         
         %importEntry = new GuiControl()
         {
            position = "0 0";
            extent = ImportingAssetList.extent.x SPC %height;
            
            new GuiTextCtrl()
            {
              Text = %assetName; 
              position = %indent SPC "0";
              extent = %width - %indent SPC %height;
              internalName = "AssetName";
            };
            
            new GuiTextCtrl()
            {
              Text = %assetType; 
              position = %width SPC "0";
              extent = %width - %height - %height SPC %height;
              internalName = "AssetType";
            };
            
            new GuiBitmapButtonCtrl()
            {
               position = ImportingAssetList.extent.x - %height - %height SPC "0";
               extent = %height SPC %height;
               command = "ImportAssetOptionsWindow.editImportSettings(" @ %assetItem @ ");";
               bitmap = "tools/gui/images/iconInformation";
            };
            new GuiBitmapButtonCtrl()
            {
               position = ImportingAssetList.extent.x - %height SPC "0";
               extent = %height SPC %height;
               command = "ImportAssetOptionsWindow.deleteImportingAsset(" @ %assetItem @ ");";
               bitmap = "tools/gui/images/iconDelete";
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

function ImportAssetWindow::validateAssets(%this)
{
   %assetCount = AssetBrowser.importAssetFinalListArray.count();
   %moduleName = ImportAssetPackageList.getText();
   %assetQuery = new AssetQuery();
   
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
            MessageBoxOK( "Error!", "Duplicate asset names found with importing assets!\nAsset \"" @ 
               %assetItemA.assetName @ "\" of type \"" @ %assetItemA.assetType @ "\" and \"" @
               %assetItemB.assetName @ "\" of type \"" @ %assetItemB.assetType @ "\" have matching names.\nPlease rename one of them and try again!");
               
            %assetQuery.delete();
            return false;
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
               break;            
            }
         }
         
         if(%foundCollision == true)
         {
            //yup, a collision, prompt for the change and bail out
            MessageBoxOK( "Error!", "Duplicate asset names found with the target module!\nAsset \"" @ 
               %assetItemA.assetName @ "\" of type \"" @ %assetItemA.assetType @ "\" has a matching name.\nPlease rename it and try again!");
               
            %assetQuery.delete();
            return false;
         }
      }
   }
   
   //Clean up our queries
   %assetQuery.delete();
   return true;
}

function ImportAssetConfigList::onSelect( %this, %id, %text )
{
   //Apply our settings to the assets
   echo("Changed our import config!");
   AssetBrowser.importAssetUnprocessedListArray.empty();
   AssetBrowser.importAssetUnprocessedListArray.duplicate(AssetBrowser.importAssetNewListArray);
   AssetBrowser.importAssetFinalListArray.empty();
   
   ImportAssetWindow.activeImportConfigIndex = %id;
   ImportAssetWindow.activeImportConfig = ImportAssetWindow.importConfigsList.getKey(%id);
   ImportAssetWindow.refresh();
}

function ImportAssetWindow::ImportAssets(%this)
{
   //do the actual importing, now!
   %assetCount = AssetBrowser.importAssetFinalListArray.count();
   
   //get the selected module data
   %moduleName = ImportAssetPackageList.getText();
   
   %module = ModuleDatabase.findModule(%moduleName, 1);
   
   if(!isObject(%module))
   {
      MessageBoxOK( "Error!", "No module selected. You must select or create a module for the assets to be added to.");
      return;
   }
   
   if(!%this.validateAssets())
      return;
   
   for(%i=0; %i < %assetCount; %i++)
   {
      %assetItem = AssetBrowser.importAssetFinalListArray.getKey(%i);
      %assetType = %assetItem.AssetType;
      %filePath = %assetItem.filePath;
      %assetName = %assetItem.assetName;
      %assetImportSuccessful = false;
      
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
         
         TamlWrite(%newAsset, %tamlpath);
         
         %file = new FileObject();
   
         if(%file.openForWrite(%scriptPath))
         {
            %file.writeline("//--- OBJECT WRITE BEGIN ---");
            %file.writeline("singleton Material(" @ %assetName @ ") {");
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
      
      if(%assetImportSuccessful)
      {
         %moduleDef = ModuleDatabase.findModule(%moduleName,1);
         if(!AssetBrowser.isAssetReImport)
            AssetDatabase.addDeclaredAsset(%moduleDef, %assetPath @ "/" @ %assetName @ ".asset.taml");
         else
            AssetDatabase.refreshAsset(AssetBrowser.reImportingAssetId);
      }
   }
   
   //force an update of any and all modules so we have an up-to-date asset list
   //AssetBrowser.reloadModules();
   AssetBrowser.loadFilters();
   Canvas.popDialog(AssetImportCtrl);
   AssetBrowser.isAssetReImport = false;
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
function ImportAssetButton::onClick(%this)
{
   %dlg = new OpenFileDialog()
   {
      Filters        = "Shape Files(*.dae, *.cached.dts)|*.dae;*.cached.dts|Images Files(*.jpg,*.png,*.tga,*.bmp,*.dds)|*.jpg;*.png;*.tga;*.bmp;*.dds|Any Files (*.*)|*.*|";
      DefaultPath    = $Pref::WorldEditor::LastPath;
      DefaultFile    = "";
      ChangePath     = false;
      OverwritePrompt = true;
      //MultipleFiles = true;
   };

   %ret = %dlg.Execute();
   
   if ( %ret )
   {
      $Pref::WorldEditor::LastPath = filePath( %dlg.FileName );
      %fullPath = makeRelativePath( %dlg.FileName, getMainDotCSDir() );
      %file = fileBase( %fullPath );
   }   
   
   %dlg.delete();
   
   if ( !%ret )
      return;
      
   AssetBrowser.importAssetListArray.empty();
   
   %fileExt = fileExt( %fullPath );
   //add it to our array!
   if( (%fileExt $= ".png") || (%fileExt $= ".jpg") || (%fileExt $= ".bmp") || (%fileExt $= ".dds") )
      AssetBrowser.importAssetListArray.add("Image", %fullPath);
   else if( (%fileExt $= ".dae") || (%fileExt $= ".dts"))
      AssetBrowser.importAssetListArray.add("Model", %fullPath);
   else if( (%fileExt $= ".ogg") || (%fileExt $= ".wav") || (%fileExt $= ".mp3"))
      AssetBrowser.importAssetListArray.add("Sound", %fullPath);
   else if (%fileExt $= ".zip")
      AssetBrowser.onDropZipFile(%fullPath);
      
   ImportAssetConfigWindow.visible = true;
   ImportAssetConfigWindow.refresh();
   ImportAssetConfigWindow.selectWindow();
}
//

//
function ImportAssetOptionsWindow::editImportSettings(%this, %assetItem)
{
   ImportAssetOptionsWindow.setVisible(1);
   ImportAssetOptionsWindow.selectWindow();
   
   ImportOptionsList.clearFields();
   
   %assetType = %assetItem.assetType;
   %filePath = %assetItem.filePath;
   %assetName = %assetItem.assetName;
   %assetConfigObj = %assetItem.importConfig;
   
   ImportOptionsList.startGroup("Asset");
   ImportOptionsList.addField("AssetName", "Asset Name", "string", "", "NewAsset", "", %assetItem);
   ImportOptionsList.endGroup();
   
   if(%assetType $= "Model")
   {
      //Get the shape info, so we know what we're doing with the mesh
      %shapeInfo = GetShapeInfo(%filePath);
      %meshItem = %shapeInfo.findItemByName("Meshes");
      %matItem = %shapeInfo.findItemByName("Materials");
      
      %meshCount = %shapeInfo.getItemValue(%meshItem);
      %matCount = %shapeInfo.getItemValue(%matItem);
      
      %firstMat = %shapeInfo.getChild(%matItem);
      echo("Mesh's first material texture path is: " @ %shapeInfo.getItemValue(%firstMat));
            
      if(%meshCount > 0)
      {
         ImportOptionsList.startGroup("Mesh");
         ImportOptionsList.addField("AutogenCollisions", "Auto-gen Collisions", "bool", "", "0", "", %assetConfigObj);
         ImportOptionsList.addField("CollapseSubmeshes", "Collapse Submeshes", "bool", "", "0", "", %assetConfigObj);
         ImportOptionsList.addField("UpAxisOverride", "Up-Axis Override", "list", "", "Z_AXIS", "Z_AXIS,Y_AXIS,X_AXIS", %assetConfigObj);
         ImportOptionsList.addField("OverrideScale", "Override Scale", "float", "", "1.0", "", %assetConfigObj);
         ImportOptionsList.addField("IgnoreNodeScale", "IgnoreNodeScaling", "bool", "", "0", "", %assetConfigObj);
         ImportOptionsList.addField("AdjustCenter", "Adjust Center", "bool", "", "0", "", %assetConfigObj);
         ImportOptionsList.addField("CollapseSubmeshes", "Collapse Submeshes", "bool", "", "0", "", %assetConfigObj);
         ImportOptionsList.addField("AdjustFloor", "Adjust Floor", "bool", "", "0", "", %assetConfigObj);
         ImportOptionsList.addField("LODType", "LOD Type", "list", "", "TrailingNumber", "TrailingNumber,DetectDTS", %assetConfigObj);
         ImportOptionsList.endGroup();
      }
      
      if(%matItem > 0)
      {
         ImportOptionsList.startGroup("Material");
         ImportOptionsList.addCallbackField("ImportMaterials", "Import Materials", "bool", "", "1", "", "ImportMaterialsChanged", %assetConfigObj);
         ImportOptionsList.addField("UseExistingMaterials", "Use Existing Materials", "bool", "", "1", "", %assetConfigObj);
         ImportOptionsList.endGroup();
      }
   }
   else if(%assetType $= "Material")
   {
      ImportOptionsList.startGroup("Material");
      ImportOptionsList.addField("CreateComposites", "Create Composite Textures", "bool", "", "1", "", %assetConfigObj);
      ImportOptionsList.endGroup();
   }
   else if(%assetType $= "Image")
   {
      ImportOptionsList.startGroup("Formatting");
      ImportOptionsList.addField("ImageType", "Image Type", "string", "", "Diffuse", "", %assetConfigObj);
      ImportOptionsList.addField("TextureFiltering", "Texture Filtering", "list", "", "Bilinear", "None,Bilinear,Trilinear", %assetConfigObj);
      ImportOptionsList.addField("UseMips", "Use Mips", "bool", "", "1", "", %assetConfigObj);
      ImportOptionsList.addField("IsHDR", "Is HDR", "bool", "", "0", "", %assetConfigObj);
      ImportOptionsList.endGroup();
      
      ImportOptionsList.startGroup("Scaling");
      ImportOptionsList.addField("Scaling", "Scaling", "float", "", "1.0", "", %assetConfigObj);
      ImportOptionsList.endGroup();
      
      ImportOptionsList.startGroup("Compression");
      ImportOptionsList.addField("IsCompressed", "Is Compressed", "bool", "", "1", "", %assetConfigObj);
      ImportOptionsList.endGroup();
      
      ImportOptionsList.startGroup("Material");
      ImportOptionsList.addField("GenerateMaterialOnImport", "Generate Material On Import", "bool", "", "1", "", %optionsObj);
      ImportOptionsList.addField("PopulateMaterialMaps", "Populate Material Maps", "bool", "", "1", "", %optionsObj);
      ImportOptionsList.addField("UseDiffuseSuffixOnOriginImg", "Use Diffuse Suffix for Origin Image", "bool", "", "1", "", %optionsObj);
      ImportOptionsList.addField("UseExistingMaterials", "Use Existing Materials", "bool", "", "1", "", %optionsObj);
      ImportOptionsList.endGroup();
   }
   else if(%assetType $= "Sound")
   {
      ImportOptionsList.startGroup("Adjustment");
      ImportOptionsList.addField("VolumeAdjust", "VolumeAdjustment", "float", "", "1.0", "", %assetConfigObj);
      ImportOptionsList.addField("PitchAdjust", "PitchAdjustment", "float", "", "1.0", "", %assetConfigObj);
      ImportOptionsList.endGroup();
      
      ImportOptionsList.startGroup("Compression");
      ImportOptionsList.addField("IsCompressed", "Is Compressed", "bool", "", "1", "", %assetConfigObj);
      ImportOptionsList.endGroup();
   }
}

function ImportAssetOptionsWindow::deleteImportingAsset(%this, %assetItem)
{
   %assetIndex = AssetBrowser.importAssetNewListArray.getIndexFromKey(%assetItem);
   AssetBrowser.importAssetNewListArray.erase(%assetIndex);
   
   //check if we have any child assets and remove them as well
   for(%i=0; %i < AssetBrowser.importAssetNewListArray.count(); %i++)
   {
      %asset = AssetBrowser.importAssetNewListArray.getKey(%i);  
      if(%asset.ParentAssetItem == %assetItem)
      {
         AssetBrowser.importAssetNewListArray.erase(%i);
         %i--;
      }
   }
   
   %assetIndex = AssetBrowser.importAssetFinalListArray.getIndexFromKey(%assetItem);
   AssetBrowser.importAssetFinalListArray.erase(%assetIndex);
   
   //check if we have any child assets and remove them as well
   for(%i=0; %i < AssetBrowser.importAssetFinalListArray.count(); %i++)
   {
      %asset = AssetBrowser.importAssetFinalListArray.getKey(%i);  
      if(%asset.ParentAssetItem == %assetItem)
      {
         AssetBrowser.importAssetFinalListArray.erase(%i);
         %i--;
      }
   }
   
   ImportAssetWindow.refresh();
   ImportAssetOptionsWindow.setVisible(0);
}

function ImportAssetOptionsWindow::saveAssetOptions(%this)
{
   ImportAssetWindow.refresh();
   ImportAssetOptionsWindow.setVisible(0);   
}

function ImportOptionsList::ImportMaterialsChanged(%this, %fieldName, %newValue, %ownerObject)
{
   echo("CHANGED IF OUR IMPORTED MATERIALS WERE HAPPENING!");
}

function ImportAssetConfigEditorWindow::populateConfigList(%this, %optionsObj)
{
   AssetImportConfigName.setText(%optionsObj.Name);
   
   ImportOptionsConfigList.clear();
   
   ImportOptionsConfigList.startGroup("Mesh");
   ImportOptionsConfigList.addCallbackField("ImportMesh", "Import Mesh", "bool", "", "1", "", "ToggleImportMesh", %optionsObj);
   ImportOptionsConfigList.addField("DoUpAxisOverride", "Do Up-axis Override", "bool", "", "0", "", %optionsObj);
   ImportOptionsConfigList.addField("UpAxisOverride", "Up-axis Override", "list", "", "Z_AXIS", "X_AXIS,Y_AXIS,Z_AXIS", %optionsObj);
   ImportOptionsConfigList.addField("DoScaleOverride", "Do Scale Override", "bool", "", "0", "", %optionsObj);
   ImportOptionsConfigList.addField("ScaleOverride", "Scale Override", "float", "", "1", "", %optionsObj);
   ImportOptionsConfigList.addField("IgnoreNodeScale", "Ignore Node Scale", "bool", "", "0", "", %optionsObj);
   ImportOptionsConfigList.addField("AdjustCenter", "Adjust Center", "bool", "", "0", "", %optionsObj);
   ImportOptionsConfigList.addField("AdjustFloor", "Adjust Floor", "bool", "", "0", "", %optionsObj);
   ImportOptionsConfigList.addField("CollapseSubmeshes", "Collapse Submeshes", "bool", "", "0", "", %optionsObj);
   ImportOptionsConfigList.addField("LODType", "LOD Type", "list", "", "TrailingNumber", "TrailingNumber,DetectDTS", %optionsObj);
   //ImportOptionsConfigList.addField("TrailingNumber", "Trailing Number", "float", "", "2", "", %optionsObj, "Mesh");
   ImportOptionsConfigList.addField("ImportedNodes", "Imported Nodes", "string", "", "", "", %optionsObj);
   ImportOptionsConfigList.addField("IgnoreNodes", "Ignore Nodes", "string", "", "", "", %optionsObj);
   ImportOptionsConfigList.addField("ImportMeshes", "Import Meshes", "string", "", "", "", %optionsObj);
   ImportOptionsConfigList.addField("IgnoreMeshes", "Imported Meshes", "string", "", "", "", %optionsObj);
   ImportOptionsConfigList.endGroup();
   
   //Materials
   ImportOptionsConfigList.startGroup("Material");
   ImportOptionsConfigList.addField("ImportMaterials", "Import Materials", "bool", "", "1", "", %optionsObj);
   ImportOptionsConfigList.addField("CreateComposites", "Create Composites", "bool", "", "1", "", %optionsObj);
   ImportOptionsConfigList.addField("UseDiffuseSuffixOnOriginImg", "Use Diffuse Suffix for Origin Image", "bool", "", "1", "", %optionsObj);
   ImportOptionsConfigList.addField("UseExistingMaterials", "Use Existing Materials", "bool", "", "1", "", %optionsObj);
   ImportOptionsConfigList.endGroup();
   
   //Animations
   ImportOptionsConfigList.startGroup("Animations");
   ImportOptionsConfigList.addField("ImportAnimations", "Import Animations", "bool", "", "1", "", %optionsObj);
   ImportOptionsConfigList.addField("SeparateAnimations", "Separate Animations", "bool", "", "1", "", %optionsObj);
   ImportOptionsConfigList.addField("SeparateAnimationPrefix", "Separate Animation Prefix", "string", "", "", "", %optionsObj);
   ImportOptionsConfigList.endGroup();
   
   //Collision
   ImportOptionsConfigList.startGroup("Collision");
   ImportOptionsConfigList.addField("GenerateCollisions", "Generate Collisions", "bool", "", "1", "", %optionsObj);
   ImportOptionsConfigList.addField("GenCollisionType", "Generate Collision Type", "list", "", "CollisionMesh", "CollisionMesh,ConvexHull", %optionsObj);
   ImportOptionsConfigList.addField("CollisionMeshPrefix", "CollisionMesh Prefix", "string", "", "Col", "", %optionsObj);
   ImportOptionsConfigList.addField("GenerateLOSCollisions", "Generate LOS Collisions", "bool", "", "1", "", %optionsObj);
   ImportOptionsConfigList.addField("GenLOSCollisionType", "Generate LOS Collision Type", "list", "", "CollisionMesh", "CollisionMesh,ConvexHull", %optionsObj);
   ImportOptionsConfigList.addField("LOSCollisionMeshPrefix", "LOS CollisionMesh Prefix", "string", "", "LOS", "", %optionsObj);
   ImportOptionsConfigList.endGroup();
   
   //Images
   ImportOptionsConfigList.startGroup("Image");
   ImportOptionsConfigList.addField("ImageType", "Image Type", "list", "", "N/A", "N/A,Diffuse,Normal,Specular,Metalness,Roughness,AO,Composite,GUI", %optionsObj);
   ImportOptionsConfigList.addField("DiffuseTypeSuffixes", "Diffuse Type Suffixes", "string", "", "_ALBEDO,_DIFFUSE,_ALB,_DIF,_COLOR,_COL", "", %optionsObj);
   ImportOptionsConfigList.addField("NormalTypeSuffixes", "Normal Type Suffixes", "string", "", "_NORMAL,_NORM", "", %optionsObj);
   ImportOptionsConfigList.addField("SpecularTypeSuffixes", "Specular Type Suffixes", "string", "", "_SPECULAR,_SPEC", "", %optionsObj);
   ImportOptionsConfigList.addField("MetalnessTypeSuffixes", "Metalness Type Suffixes", "string", "", "_METAL,_MET,_METALNESS,_METALLIC", "", %optionsObj);
   ImportOptionsConfigList.addField("RoughnessTypeSuffixes", "Roughness Type Suffixes", "string", "", "_ROUGH,_ROUGHNESS", "", %optionsObj);
   ImportOptionsConfigList.addField("SmoothnessTypeSuffixes", "Smoothness Type Suffixes", "string", "", "_SMOOTH,_SMOOTHNESS", "", %optionsObj);
   ImportOptionsConfigList.addField("AOTypeSuffixes", "AO Type Suffixes", "string", "", "_AO,_AMBIENT,_AMBIENTOCCLUSION", "", %optionsObj);
   ImportOptionsConfigList.addField("CompositeTypeSuffixes", "Composite Type Suffixes", "string", "", "_COMP,_COMPOSITE", "", %optionsObj);
   ImportOptionsConfigList.addField("TextureFilteringMode", "Texture Filtering Mode", "list", "", "Bilinear", "None,Bilinear,Trilinear", %optionsObj);
   ImportOptionsConfigList.addField("UseMips", "Use Mipmaps", "bool", "", "1", "", %optionsObj);
   ImportOptionsConfigList.addField("IsHDR", "Is HDR", "bool", "", "0", "", %optionsObj);
   ImportOptionsConfigList.addField("Scaling", "Scaling", "float", "", "1.0", "", %optionsObj);
   ImportOptionsConfigList.addField("Compressed", "Is Compressed", "bool", "", "1", "", %optionsObj);
   ImportOptionsConfigList.addField("GenerateMaterialOnImport", "Generate Material On Import", "bool", "", "1", "", %optionsObj);
   ImportOptionsConfigList.addField("PopulateMaterialMaps", "Populate Material Maps", "bool", "", "1", "", %optionsObj);
   ImportOptionsConfigList.endGroup();
   
   //Sounds
   ImportOptionsConfigList.startGroup("Sound");
   ImportOptionsConfigList.addField("VolumeAdjust", "Volume Adjustment", "float", "", "1.0", "", %optionsObj);
   ImportOptionsConfigList.addField("PitchAdjust", "Pitch Adjustment", "float", "", "1.0", "", %optionsObj);
   ImportOptionsConfigList.addField("Compressed", "Is Compressed", "bool", "", "0", "", %optionsObj);
   ImportOptionsConfigList.endGroup();
}

function ImportAssetConfigEditorWindow::addNewConfig(%this)
{
   ImportAssetConfigEditorWindow.setVisible(1);
   ImportAssetConfigEditorWindow.selectWindow();
   
   %optionsObj = new ScriptObject(){};
   
   ImportAssetWindow.importConfigsList.add(%optionsObj); 
   
   //Initial, blank configuration
   %optionsObj.ImportMesh = true;
   %optionsObj.DoUpAxisOverride = false;
   %optionsObj.UpAxisOverride = "Z_AXIS";
   %optionsObj.DoScaleOverride = false;
   %optionsObj.ScaleOverride = 1.0;
   %optionsObj.IgnoreNodeScale = false;
   %optionsObj.AdjustCenter = false;
   %optionsObj.AdjustFloor = false;
   %optionsObj.CollapseSubmeshes = false;
   %optionsObj.LODType = "TrailingNumber";
   //%optionsObj.TrailingNumber = 2;
   %optionsObj.ImportedNodes = "";
   %optionsObj.IgnoreNodes = "";
   %optionsObj.ImportMeshes = "";
   %optionsObj.IgnoreMeshes = "";
   
   //Materials
   %optionsObj.ImportMaterials = true;
   %optionsObj.CreateComposites = true;
   %optionsObj.UseDiffuseSuffixOnOriginImg = true;
   %optionsObj.UseExistingMaterials = true;
   
   //Animations
   %optionsObj.ImportAnimations = true;
   %optionsObj.SeparateAnimations = true;
   %optionsObj.SeparateAnimationPrefix = "";
   
   //Collision
   %optionsObj.GenerateCollisions = true;
   %optionsObj.GenCollisionType = "CollisionMesh";
   %optionsObj.CollisionMeshPrefix = "Col";
   %optionsObj.GenerateLOSCollisions = true;
   %optionsObj.GenLOSCollisionType = "CollisionMesh";
   %optionsObj.LOSCollisionMeshPrefix = "LOS";
   
   //Images
   %optionsObj.ImageType = "N/A";
   %optionsObj.DiffuseTypeSuffixes = "_ALBEDO,_DIFFUSE,_ALB,_DIF,_COLOR,_COL,_BASECOLOR,_BASE_COLOR";
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
   %optionsObj.GenerateMaterialOnImport = true;
   %optionsObj.PopulateMaterialMaps = true;
   
   //Sounds
   %optionsObj.VolumeAdjust = 1.0;
   %optionsObj.PitchAdjust = 1.0;
   %optionsObj.Compressed = false;
   
   //Hook in the UI
   %this.populateConfigList(%optionsObj);
}

function ImportAssetConfigEditorWindow::editConfig(%this)
{
   ImportAssetConfigEditorWindow.setVisible(1);
   ImportAssetConfigEditorWindow.selectWindow();
   
   %this.populateConfigList(ImportAssetWindow.activeImportConfig);
}

function ImportAssetConfigEditorWindow::deleteConfig(%this)
{
   ImportAssetWindow.importConfigsList.erase(ImportAssetWindow.activeImportConfigIndex);
   ImportAssetConfigList.setSelected(0); //update it
   
   ImportAssetConfigEditorWindow.saveAssetOptionsConfig();
}

function ImportAssetConfigEditorWindow::saveAssetOptionsConfig(%this)
{
   %xmlDoc = new SimXMLDocument();
   
   %xmlDoc.pushNewElement("AssetImportConfigs");
      
      for(%i = 0; %i < ImportAssetWindow.importConfigsList.count(); %i++)
      {
         %configObj = ImportAssetWindow.importConfigsList.getKey(%i);         
         
         %xmlDoc.pushNewElement("Config");
         
         if(%configObj.Name $= "")
            %configObj.Name = AssetImportConfigName.getText();
            
         %xmlDoc.setAttribute("Name", %configObj.Name); 
         
            %xmlDoc.pushNewElement("Mesh");
               %xmlDoc.setAttribute("ImportMesh", %configObj.ImportMesh);
               %xmlDoc.setAttribute("DoUpAxisOverride", %configObj.DoUpAxisOverride);
               %xmlDoc.setAttribute("UpAxisOverride", %configObj.UpAxisOverride);
               %xmlDoc.setAttribute("DoScaleOverride", %configObj.DoScaleOverride);
               %xmlDoc.setAttribute("ScaleOverride", %configObj.ScaleOverride);
               %xmlDoc.setAttribute("IgnoreNodeScale", %configObj.IgnoreNodeScale);
               %xmlDoc.setAttribute("AdjustCenter", %configObj.AdjustCenter);
               %xmlDoc.setAttribute("AdjustFloor", %configObj.AdjustFloor);
               %xmlDoc.setAttribute("CollapseSubmeshes", %configObj.CollapseSubmeshes);         
               %xmlDoc.setAttribute("LODType", %configObj.LODType);
               %xmlDoc.setAttribute("ImportedNodes", %configObj.ImportedNodes);
               %xmlDoc.setAttribute("IgnoreNodes", %configObj.IgnoreNodes);
               %xmlDoc.setAttribute("ImportMeshes", %configObj.ImportMeshes);
               %xmlDoc.setAttribute("IgnoreMeshes", %configObj.IgnoreMeshes);
            %xmlDoc.popElement();
            
            %xmlDoc.pushNewElement("Materials");
               %xmlDoc.setAttribute("ImportMaterials", %configObj.ImportMaterials);
               %xmlDoc.setAttribute("CreateComposites", %configObj.CreateComposites);
               %xmlDoc.setAttribute("UseDiffuseSuffixOnOriginImg", %configObj.UseDiffuseSuffixOnOriginImg);
               %xmlDoc.setAttribute("UseExistingMaterials", %configObj.UseExistingMaterials);
            %xmlDoc.popElement();
            
            %xmlDoc.pushNewElement("Animations");
               %xmlDoc.setAttribute("ImportAnimations", %configObj.ImportAnimations);
               %xmlDoc.setAttribute("SeparateAnimations", %configObj.SeparateAnimations);
               %xmlDoc.setAttribute("SeparateAnimationPrefix", %configObj.SeparateAnimationPrefix);
            %xmlDoc.popElement();
            
            %xmlDoc.pushNewElement("Collisions");
               %xmlDoc.setAttribute("GenerateCollisions", %configObj.GenerateCollisions);
               %xmlDoc.setAttribute("GenCollisionType", %configObj.GenCollisionType);
               %xmlDoc.setAttribute("CollisionMeshPrefix", %configObj.CollisionMeshPrefix);
               %xmlDoc.setAttribute("GenerateLOSCollisions", %configObj.GenerateLOSCollisions);
               %xmlDoc.setAttribute("GenLOSCollisionType", %configObj.GenLOSCollisionType);
               %xmlDoc.setAttribute("LOSCollisionMeshPrefix", %configObj.LOSCollisionMeshPrefix);
            %xmlDoc.popElement();
            
            %xmlDoc.pushNewElement("Images");
               %xmlDoc.setAttribute("ImageType", %configObj.ImageType);
               %xmlDoc.setAttribute("DiffuseTypeSuffixes", %configObj.DiffuseTypeSuffixes);
               %xmlDoc.setAttribute("NormalTypeSuffixes", %configObj.NormalTypeSuffixes);
               %xmlDoc.setAttribute("SpecularTypeSuffixes", %configObj.SpecularTypeSuffixes);
               %xmlDoc.setAttribute("MetalnessTypeSuffixes", %configObj.MetalnessTypeSuffixes);
               %xmlDoc.setAttribute("RoughnessTypeSuffixes", %configObj.RoughnessTypeSuffixes);
               %xmlDoc.setAttribute("SmoothnessTypeSuffixes", %configObj.SmoothnessTypeSuffixes);
               %xmlDoc.setAttribute("AOTypeSuffixes", %configObj.AOTypeSuffixes);
               %xmlDoc.setAttribute("CompositeTypeSuffixes", %configObj.CompositeTypeSuffixes);
               %xmlDoc.setAttribute("TextureFilteringMode", %configObj.TextureFilteringMode);
               %xmlDoc.setAttribute("UseMips", %configObj.UseMips);
               %xmlDoc.setAttribute("IsHDR", %configObj.IsHDR);
               %xmlDoc.setAttribute("Scaling", %configObj.Scaling);
               %xmlDoc.setAttribute("Compressed", %configObj.Compressed);
               %xmlDoc.setAttribute("GenerateMaterialOnImport", %configObj.GenerateMaterialOnImport);
               %xmlDoc.setAttribute("PopulateMaterialMaps", %configObj.PopulateMaterialMaps);
            %xmlDoc.popElement();
            
            %xmlDoc.pushNewElement("Sounds");
               %xmlDoc.setAttribute("VolumeAdjust", %configObj.VolumeAdjust);
               %xmlDoc.setAttribute("PitchAdjust", %configObj.PitchAdjust);
               %xmlDoc.setAttribute("Compressed", %configObj.Compressed);
            %xmlDoc.popElement();
         
         %xmlDoc.popElement();
      }
      
   %xmlDoc.popElement();
   
   %xmlDoc.saveFile($AssetBrowser::importConfigsFile);
   
   ImportAssetConfigEditorWindow.setVisible(0);
   ImportAssetWindow.reloadImportOptionConfigs();
}

function ImportOptionsConfigList::ToggleImportMesh(%this, %fieldName, %newValue, %ownerObject)
{
   %this.setFieldEnabled("DoUpAxisOverride", %newValue);
   %this.setFieldEnabled("UpAxisOverride", %newValue);
   %this.setFieldEnabled("DoScaleOverride", %newValue);
   %this.setFieldEnabled("ScaleOverride", %newValue);
   %this.setFieldEnabled("IgnoreNodeScale", %newValue);
   %this.setFieldEnabled("AdjustCenter", %newValue);
   %this.setFieldEnabled("AdjustFloor", %newValue);
   %this.setFieldEnabled("CollapseSubmeshes", %newValue);
   %this.setFieldEnabled("LODType", %newValue);   
   %this.setFieldEnabled("ImportedNodes", %newValue);
   %this.setFieldEnabled("IgnoreNodes", %newValue);
   %this.setFieldEnabled("ImportMeshes", %newValue);
   %this.setFieldEnabled("IgnoreMeshes", %newValue);
}