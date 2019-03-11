function LegacyContentImporterWindow::onWake(%this)
{
   %this.LegacyContentShapeList = new ArrayObject();
   %this.LegacyContentMaterialList = new ArrayObject();
   %this.LegacyContentImageList = new ArrayObject();
   %this.LegacyContentAnimationList = new ArrayObject();
}

function addLegacyAssetBtn::onClick(%this)
{
   %filter = "Any Files (*.*)|*.*|";
   %assetList = "";
   
   if(legacyContentTabbook.selectedPage == 0) //shapes
   {
      %filter = "Shape Files(*.dae, *.cached.dts)|*.dae;*.cached.dts|" @ %filter;
      %assetList = LegacyContentShapeList;
   }
   else if(legacyContentTabbook.selectedPage == 1) //Materials
   {
      %filter = "Material Definition file(*.cs)|*.cs|" @ %filter;
      %assetList = LegacyContentMaterialList;
   }
   else if(legacyContentTabbook.selectedPage == 2) //Images
   {
      %filter = "Images Files(*.jpg,*.png,*.tga,*.bmp,*.dds)|*.jpg;*.png;*.tga;*.bmp;*.dds|" @ %filter;
      %assetList = LegacyContentImageList;
   }
   else if(legacyContentTabbook.selectedPage == 3) //shapes
   {
      %filter = "Shape Animation Files(*.dae, *.dsq)|*.dae;*.dsq|" @ %filter;
      %assetList = LegacyContentAnimationList;
   }
   
   %dlg = new OpenFileDialog()
   {
      Filters        = %filter;//"Shape Files(*.dae, *.cached.dts)|*.dae;*.cached.dts|Images Files(*.jpg,*.png,*.tga,*.bmp,*.dds)|*.jpg;*.png;*.tga;*.bmp;*.dds|Any Files (*.*)|*.*|";
      DefaultPath    = $Pref::WorldEditor::LastPath;
      DefaultFile    = "";
      ChangePath     = false;
      OverwritePrompt = true;
      MultipleFiles = true;
   };

   %ret = %dlg.Execute();
   
   if ( %ret )
   {
      $Pref::WorldEditor::LastPath = filePath( %dlg.FileName );
      //%fullPath = makeRelativePath( %dlg.FileName, getMainDotCSDir() );
      
      for(%i=0; %i < %dlg.fileCount; %i++)
      {
         %filePath = %dlg.files[%i];
         %file = fileBase(makeRelativePath(%dlg.files[%i], getMainDotCSDir()));
         LegacyContentShapeList.add(%file);
      }
   }   
   
   %dlg.delete();
   
   if ( !%ret )
      return;  
}