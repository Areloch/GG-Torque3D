function AssetBrowser::buildImageAssetPreview(%this, %assetDef, %previewData)
{
   %previewData.assetName = %assetDef.assetName;
   %previewData.assetPath = %assetDef.scriptFile;
   //%previewData.doubleClickCommand = "EditorOpenFileInTorsion( "@%previewData.assetPath@", 0 );";
   
   if(isFile(%assetDef.imageFile))
      %previewData.previewImage = %assetDef.imageFile;
   else
      %previewData.previewImage = "core/rendering/images/unavailable";
   
   %previewData.assetFriendlyName = %assetDef.assetName;
   %previewData.assetDesc = %assetDef.description;
   %previewData.tooltip = %assetDef.friendlyName @ "\n" @ %assetDef;
}

function GuiInspectorTypeImageAssetPtr::onControlDropped( %this, %payload, %position )
{
   Canvas.popDialog(EditorDragAndDropLayer);
   
   // Make sure this is a color swatch drag operation.
   if( !%payload.parentGroup.isInNamespaceHierarchy( "AssetPreviewControlType_AssetDrop" ) )
      return;

   %assetType = %payload.dragSourceControl.parentGroup.assetType;
   
   if(%assetType $= "ImageAsset")
   {
      echo("DROPPED A IMAGE ON AN IMAGE ASSET COMPONENT FIELD!");  
   }
   
   EWorldEditor.isDirty = true;
}