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

function initializeBrushEditor()
{
   echo(" % - Initializing Brush Editor");
   
   new BrushEditorTool(BrushEditor){}; 
   
   exec("./gui/BrushEditorSidebar.gui");  
            
   new ScriptObject( BrushEditorPlugin )
   {
      superClass = "EditorPlugin";
      editorTool = BrushEditor;
   };
   
   exec("./script/BrushManagement.cs"); 
   
   %map = new ActionMap();
   %map.bindCmd( keyboard, "shift 1", "BrushEditor.setActionMode(\"Select\");", "" ); // Select
   %map.bindCmd( keyboard, "shift 2", "BrushEditor.setActionMode(\"Paint\");", "" ); // Select
   %map.bindCmd( keyboard, "shift 3", "BrushEditor.setActionMode(\"Erase\");", "" ); // Select
   %map.bindCmd( keyboard, "shift 4", "BrushEditor.setActionMode(\"PaintBucket\");", "" ); // Select
   
   %map.bindCmd( keyboard, "1", "BrushEditor.setElementMode(\"Floor\");", "" ); // Select
   %map.bindCmd( keyboard, "2", "BrushEditor.setElementMode(\"Wall\");", "" );   // Move
   %map.bindCmd( keyboard, "3", "BrushEditor.setElementMode(\"Ceiling\");", "" ); // Rotate
   %map.bindCmd( keyboard, "4", "BrushEditor.setElementMode(\"FullTile\");", "" );  // Scale
   
   %map.bindCmd( keyboard, "up", "BrushEditor.riseFloor();", "" );
   %map.bindCmd( keyboard, "down", "BrushEditor.lowerFloor();", "" );
   
   %map.bindCmd( keyboard, "shift up", "BrushEditor.nextFloor();", "" );
   %map.bindCmd( keyboard, "shift down", "BrushEditor.prevFloor();", "" );
   BrushEditorPlugin.map = %map;   
}

function destroyBrushEditor()
{
}

function BrushEditorPlugin::onWorldEditorStartup( %this )
{       
}

function BrushEditorPlugin::onWorldEditorShutdown( %this )
{
}

function BrushEditorPlugin::toggleActive(%this)
{
   %currentEditorTool = EWorldEditor.getActiveEditorTool();
   if(isObject(%currentEditorTool) && %currentEditorTool == BrushEditor.getId())
   {
      %this.onDeactivated(); 
   }
   else
   {
      %this.onActivated();
   }
}

function BrushEditorPlugin::onActivated( %this )
{
   %this.map.push();
   //Parent::onActivated(%this); 
   
   EWorldEditor.setEditorTool(BrushEditorPlugin.editorTool); 
   
   Canvas.pushDialog(BrushEditorContainer); 
   
   BrushEditorSettingsWindow.Position = EWorldEditor.extent.x - BrushEditorSettingsWindow.Extent.x SPC EditorGuiToolbar.getGlobalPosition().y + EditorGuiToolbar.extent.y;
   
   EWToolsPaletteWindow.Visible = false;
   EWTreeWindow.Visible = false; 
   EWInspectorWindow.Visible = false;
   EWToolsToolbar.Visible = false;
   
   BrushEditorSettingsWindow-->MainBook.selectPage(0);
   BrushEditorSettingsWindow-->BrushManagementBook.selectPage(0);
   
   BrushEditorSettingsWindow-->ListBook.selectPage(0);
   BrushEditorSettingsWindow-->SettingsBook.selectPage(0);
   
   BrushEditor_SelectModeBtn.setStateOn(true);
   BrushEditor_FloorModeBtn.setStateOn(true);
   
   BrushEditor_RestrictWallstoFlooredBtn.setStateOn(true);
   
   %this.populateBrushObjectsList();
   
   %this.loadTilesInfo();
}

function BrushEditorPlugin::onDeactivated( %this )
{  
   %this.map.pop();
   
   Canvas.popDialog(BrushEditorContainer);
   
   EWToolsPaletteWindow.Visible = true;
   EWTreeWindow.Visible = true;
   EWInspectorWindow.Visible = true;
   EWToolsToolbar.Visible = true;
   
   Parent::onDeactivated(%this);
   
   EWorldEditor.setEditorTool(0);

}

function BrushEditorPlugin::populateBrushObjectsList(%this)
{
   %objs = parseMissionGroupForIds("BrushObject", "");
   %objsCount = getWordCount(%objs);
   
   BrushObjListTreeView.clear();
   BrushObjListTreeView.insertItem(0, "Brush Objects", "");
   
   for(%i=0; %i < %objsCount; %i++)
   {
      %BrushObj = getWord(%objs, %i);
      
      BrushObjListTreeView.insertItem(1, "New Brush Object", %BrushObj);
   }
   
   BrushObjListTreeView.buildVisibleTree(true);
}

function BrushEditorTool_AddNewBrushObj::onClick(%this)
{
   %newTileObj = new BrushObject()
   {
      position = "0 0 0";
      rotation = "0 0 0";
   };
   
   MissionGroup.add(%newTileObj);
   
   BrushEditorPlugin.populateBrushObjectsList();
}

function BrushEditorTool_DelBrushObj::onClick(%this)
{
   
}

function BrushObjListTreeView::onSelect(%this, %item)
{
   %BrushObj = BrushObjListTreeView.getItemValue(%item);
   
   BrushEditor.setActiveObject(%BrushObj);
   
   //populate the elements tree
   BrushElemListTreeView.clear();
   
   BrushEditor.getActiveBrushObjectInfo(BrushElemListTreeView);
}

function BrushEditor_SelectModeBtn::onClick(%this)
{
   BrushEditor.setActionMode("Select");
}

function BrushEditor_PaintModeBtn::onClick(%this)
{
   BrushEditor.setActionMode("Paint");
}

function BrushEditor_EraseModeBtn::onClick(%this)
{
   BrushEditor.setActionMode("Erase");
}

function BrushEditor_BoxPaintModeBtn::onClick(%this)
{
   BrushEditor.setActionMode("BoxPaint");
}

function BrushEditor_FloorModeBtn::onClick(%this)
{
   BrushEditor.setElementMode("Floor");
}

function BrushEditor_WallModeBtn::onClick(%this)
{
   BrushEditor.setElementMode("Wall");
}

function BrushEditor_RaiseFloorBtn::onClick(%this)
{
   BrushEditor.riseFloor();
}

function BrushEditor_LowerFloorBtn::onClick(%this)
{
   BrushEditor.lowerFloor();
}

function BrushEditor_NextFloorBtn::onClick(%this)
{
   BrushEditor.nextFloor();
}
function BrushEditor_PrevFloorBtn::onClick(%this)
{
   BrushEditor.prevFloor();
}