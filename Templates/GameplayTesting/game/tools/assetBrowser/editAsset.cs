function AssetBrowser_editAsset::saveAsset(%this)
{
   %file = AssetDatabase.getAssetFilePath(%this.editedAssetId);
   %success = TamlWrite(AssetBrowser_editAsset.editedAsset, %file);

   Canvas.popDialog(AssetBrowser_editAsset);
}