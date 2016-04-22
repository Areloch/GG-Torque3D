function initProfiler()
{
   new GuiControl(ProfilerGUI) {
      profile = "GuiModelessDialogProfile";
      horizSizing = "right";
      vertSizing = "bottom";
      position = "0 0";
      extent = "640 480";
      minExtent = "8 8";
      visible = "True";
      setFirstResponder = "True";
      modal = "false";
      helpTag = "0";
      noCursor = true;
      
      new GuiWindowCollapseCtrl(ProfilerWindow) {
         canSaveDynamicFields = "0";
         internalName = "ProfilerWindow";
         Enabled = "1";
         isContainer = "1";
         Profile = "ToolsGuiWindowProfile";
         Position = firstWord($pref::Video::mode) - 209
            SPC getWord(EditorGuiToolbar.extent, 1) -1;
         Extent = "210 324";
         MinExtent = "210 140";
         HorizSizing = "windowRelative";
         VertSizing = "windowRelative";
         canSave = "1";
         Visible = "1";
         hovertime = "1000";
         Margin = "8 8 8 8";
         Padding = "0 0 0 0";
         AnchorTop = "1";
         AnchorBottom = "0";
         AnchorLeft = "1";
         AnchorRight = "0";
         resizeWidth = "1";
         resizeHeight = "1";
         canMove = "1";
         canClose = "1";
         canMinimize = "0";
         canMaximize = "0";
         minSize = "50 50";
         closeCommand = "endProfiler();";
         EdgeSnap = "1";
         text = "Profiler";
         
         new GuiTextEditCtrl( ProfilerTreeFilter ) {
            position = "2 25";
            extent = "175 18";
            profile = "ToolsGuiTextEditProfile";
            horizSizing = "width";
            vertSizing = "bottom";
            class = "GuiTreeViewFilterText";
            treeView = EditorTree;
         };
         
         new GuiBitmapButtonCtrl() {
            bitmap = "tools/gui/images/clear-icon";
            groupNum = "-1";
            buttonType = "PushButton";
            useMouseEvents = "0";
            isContainer = "0";
            Profile = "ToolsGuiDefaultProfile";
            HorizSizing = "left";
            VertSizing = "bottom";
            position = "180 26";
            Extent = "17 17";
            MinExtent = "8 2";
            canSave = "1";
            Visible = "1";
            tooltipprofile = "ToolsGuiToolTipProfile";
            hovertime = "1000";
            canSaveDynamicFields = "0";
            class = "GuiTreeViewFilterClearButton";
            textCtrl = EditorTreeFilter;
         };
                           
         new GuiScrollCtrl() {
            canSaveDynamicFields = "0";
            Enabled = "1";
            isContainer = "1";
            Profile = "ToolsGuiScrollProfile";
            HorizSizing = "width";
            VertSizing = "height";
            Position = "0 45";
            Extent = "197 246";
            MinExtent = "8 8";
            canSave = "1";
            Visible = "1";
            hovertime = "1000";
            willFirstRespond = "1";
            hScrollBar = "dynamic";
            vScrollBar = "dynamic";
            lockHorizScroll = "false";
            lockVertScroll = "false";
            constantThumbHeight = "0";
            childMargin = "0 0";

            new GuiTreeViewCtrl(ProfilerTreeView) {
               canSaveDynamicFields = "0";
               Enabled = "1";
               isContainer = "1";
               Profile = "ToolsGuiTreeViewProfile";
               HorizSizing = "right";
               VertSizing = "bottom";
               Position = "1 1";
               Extent = "193 21";
               MinExtent = "8 8";
               canSave = "1";
               Visible = "1";
               hovertime = "1000";
               tabSize = "16";
               textOffset = "2";
               fullRowSelect = "0";
               itemHeight = "21";
               destroyTreeOnSleep = "1";
               MouseDragging = "1";
               MultipleSelections = "1";
               DeleteObjectAllowed = "1";
               DragToItemAllowed = "1";
               showRoot = "1";
               useInspectorTooltips = "1";
               tooltipOnWidthOnly = "1";
               showObjectIds = "0";
               showClassNames = "0";
               showObjectNames = "1";
               showInternalNames = "1";
               showClassNameForUnnamedObjects = "1";
            };
         };
      };
   };
   
   return %profilerGUI;
}

function startProfiler()
{
   initProfiler();
   Canvas.pushDialog(ProfilerGUI);
   
   $runProfiler = true;
   profilerEnable(true);
   schedule(1000, 0, "updateProfiler");
}

function updateProfiler()
{
   if($runProfiler == true)
   {
      schedule(1000, 0, "updateProfiler");
      profilerDumpToScript();
   }
}

function endProfiler()
{
   $runProfiler = false;
   profilerEnable(false);
   Canvas.popDialog(ProfilerGUI);
}