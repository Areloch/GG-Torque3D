function MainMenuGui::onWake(%this)
{
   //return;
   ManagePanel.visible = false;
   
   //Navigation
   %btn = new GuiButtonCtrl()
   {
      text = "Asset Library";
      profile = GuiMenuButtonProfile;
      command="loadAssetsPage();";
   };   
   NavBarStack.add(%btn);
   
   %btn = new GuiButtonCtrl()
   {
      text = "Projects";
      profile = GuiMenuButtonProfile;
      command="loadProjectsPage();";
   };   
   NavBarStack.add(%btn);
   
   %btn = new GuiButtonCtrl()
   {
      text = "Engine Builds";
      profile = GuiMenuButtonProfile;
   };   
   NavBarStack.add(%btn);
   
   %btn = new GuiButtonCtrl()
   {
      text = "Community";
      profile = GuiMenuButtonProfile;
   };   
   NavBarStack.add(%btn);
   
   $Mode = "";
   
   %homePage = PMSettings.value("UI/homePage", "Projects");
   
   for(%i=0; %i < NavBarStack.getCount(); %i++)
   {
      %btn = NavBarStack.getObject(%i);
      if(%btn.text $= %homePage)
      {
         %btn.performClick();
         break;
      }
   }
}

function addNavigationButton(%name)
{
   %btn = new GuiButtonCtrl()
   {
      text = %name;
      profile = GuiMenuButtonProfile;
   };
   
   NavBarStack.add(%btn);
   
   return %btn;
}

function updateNewPanelBtn()
{
   if(!isObject(NewPanelButton))
   {
      %previewSize = $PanelSize - 20 SPC $PanelSize - 20;
      %previewBounds = 20;
      %container = new GuiControl(NewPanelButton){
         profile = "ToolsGuiDefaultProfile";
         Position = "0 0";
         Extent = %previewSize.x + %previewBounds SPC %previewSize.y + %previewBounds + 24;
         HorizSizing = "right";
         VertSizing = "bottom";
         isContainer = "1";
         assetName = %assetName;
         moduleName = %moduleName;
         assetType = %assetType;
         
         new GuiBitmapButtonCtrl()
         {
            internalName = btn;
            HorizSizing = "right";
            VertSizing = "bottom";
            profile = "ToolsGuiButtonProfile";
            position = "0 0";
            extent = %previewSize;
            buttonType = "PushButton";
            bitmap = "data/ui/images/newPanelImage";
            Command = "";
            text = "";
            useStates = false;
            command="handleNewPanelAction();";
         };
      };
   }
   
   //force it to the end
   PanelGrid.remove(NewPanelButton);
   PanelGrid.add(NewPanelButton);
}

function buildAssetPanel(%name, %type)
{
   %previewSize = $PanelSize - 20 SPC $PanelSize - 20;
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
      assetType = %assetType;
   };
   
   %previewButton = new GuiBitmapButtonCtrl()
   {
      className = "AssetPreviewControl";
      internalName = %this.previewData.assetName;
      HorizSizing = "right";
      VertSizing = "bottom";
      profile = "ToolsGuiButtonProfile";
      position = "10 4";
      extent = %previewSize;
      buttonType = "PushButton";
      bitmap = "data/ui/images/assetPanelImage";
      Command = "";
      text = "";
      useStates = false;
      
      new GuiBitmapButtonCtrl()
      {
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
            visible = false;
         }; 
   };  
   
   %previewBorder = new GuiButtonCtrl(){
         class = "AssetPreviewButton";
         internalName = %this.previewData.assetName@"Border";
         HorizSizing = "right";
         VertSizing = "bottom";
         profile = "ToolsGuiThumbHighlightButtonProfile";
         position = "0 0";
         extent = %previewSize.x + %previewBounds SPC %previewSize.y + 24;
         Variable = "";
         buttonType = "radioButton";
         tooltip = %this.previewData.tooltip;
         Command = "AssetBrowser.updateSelection( $ThisControl.getParent().assetName, $ThisControl.getParent().moduleName );"; 
		   altCommand = %this.previewData.doubleClickCommand;
         groupNum = "0";
         useMouseEvents = true;
         text = "";
         visible = false;
         icon = %this.previewData.previewImage;
   };
   
   %previewNameCtrl = new GuiTextEditCtrl(){
      position = 0 SPC %previewSize.y + %previewBounds - 16;
      profile = GuiMenuButtonProfile;
      extent = %previewSize.x + %previewBounds SPC 16;
      text = %name;
      internalName = "AssetNameLabel";
      class = "AssetNameField";
      active = false;
   }; 
   
   %container.add(%previewButton);  
   %container.add(%previewBorder); 
   %container.add(%previewNameCtrl);
   
   return %container;
}

function handleNewPanelAction()
{
   ManagePanel.visible = !ManagePanel.visible;
   
   if(ManagePanel.visible == true)
   {
      NewPanelButton-->btn.bitmap = "data/ui/images/cancelPanelImage";
   }
   else
   {
      NewPanelButton-->btn.bitmap = "data/ui/images/newPanelImage";
   }
   
   switch$($Mode)
   {
      case "Assets":
         makeNewAsset();
         break;
      case "Projects":
         makeNewProject();
         break;
   }
}