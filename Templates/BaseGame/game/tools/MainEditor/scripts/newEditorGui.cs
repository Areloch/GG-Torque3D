function NewEditorGui::AddWindow(%this)
{
   %page = new GuiTabPageCtrl()
   {
      fitBook = "1";
      text = "Object Viewer";
      maxLength = "1024";
      margin = "0 0 0 0";
      padding = "0 0 0 0";
      anchorTop = "1";
      anchorBottom = "0";
      anchorLeft = "1";
      anchorRight = "0";
      position = "0 0";
      extent = "1024 768";
      minExtent = "8 2";
      horizSizing = "width";
      vertSizing = "height";
      profile = "GuiTabPageProfile";
      visible = "1";
      active = "1";
      tooltipProfile = "GuiToolTipProfile";
      hovertime = "1000";
      isContainer = "1";
      canSave = "1";
      canSaveDynamicFields = "1";
   };
   
   %page.add(ObjectViewer);
   
   MainEditorBasePanel.add(%page);
   
   //Ensure the sidebar is spaced sanely
   %pos = MainEditorBasePanel.extent.x * 0.8;
   ObjectViewer-->splitContainer.splitPoint.x = MainEditorBasePanel.extent.x * 0.8;
   
}