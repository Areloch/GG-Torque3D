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

function initializeTilesetEditor()
{
   echo(" % - Initializing Tileset Editor");
   
   new TilesetObjectEditorTool(TilesetEditor){}; 
   
   exec("./gui/TilesetEditorSidebar.gui");  
            
   new ScriptObject( TilesetEditorPlugin )
   {
      superClass = "EditorPlugin";
      editorTool = TilesetEditor;
   };
   
   exec("./script/TilesetManagement.cs"); 
   
   %map = new ActionMap();
   %map.bindCmd( keyboard, "shift 1", "TilesetEditor.setActionMode(\"Select\");", "" ); // Select
   %map.bindCmd( keyboard, "shift 2", "TilesetEditor.setActionMode(\"Paint\");", "" ); // Select
   %map.bindCmd( keyboard, "shift 3", "TilesetEditor.setActionMode(\"Erase\");", "" ); // Select
   %map.bindCmd( keyboard, "shift 4", "TilesetEditor.setActionMode(\"PaintBucket\");", "" ); // Select
   
   %map.bindCmd( keyboard, "1", "TilesetEditor.setElementMode(\"Floor\");", "" ); // Select
   %map.bindCmd( keyboard, "2", "TilesetEditor.setElementMode(\"Wall\");", "" );   // Move
   %map.bindCmd( keyboard, "3", "TilesetEditor.setElementMode(\"Ceiling\");", "" ); // Rotate
   %map.bindCmd( keyboard, "4", "TilesetEditor.setElementMode(\"FullTile\");", "" );  // Scale
   
   %map.bindCmd( keyboard, "up", "TilesetEditor.riseFloor();", "" );
   %map.bindCmd( keyboard, "down", "TilesetEditor.lowerFloor();", "" );
   
   %map.bindCmd( keyboard, "shift up", "TilesetEditor.nextFloor();", "" );
   %map.bindCmd( keyboard, "shift down", "TilesetEditor.prevFloor();", "" );
   TilesetEditorPlugin.map = %map;   
}

function destroyTilesetEditor()
{
}

function TilesetEditorPlugin::onWorldEditorStartup( %this )
{       
}

function TilesetEditorPlugin::onWorldEditorShutdown( %this )
{
}

function TilesetEditorPlugin::toggleActive(%this)
{
   %currentEditorTool = EWorldEditor.getActiveEditorTool();
   if(isObject(%currentEditorTool) && %currentEditorTool == TilesetEditor.getId())
   {
      %this.onDeactivated(); 
   }
   else
   {
      %this.onActivated();
   }
}

function TilesetEditorPlugin::onActivated( %this )
{
   %this.map.push();
   //Parent::onActivated(%this); 
   
   EWorldEditor.setEditorTool(TilesetEditorPlugin.editorTool); 
   
   Canvas.pushDialog(TilesetEditorContainer); 
   
   TilesetEditorSettingsWindow.Position = EWorldEditor.extent.x - TilesetEditorSettingsWindow.Extent.x SPC EditorGuiToolbar.getGlobalPosition().y + EditorGuiToolbar.extent.y;
   
   EWToolsPaletteWindow.Visible = false;
   EWTreeWindow.Visible = false; 
   EWInspectorWindow.Visible = false;
   EWToolsToolbar.Visible = false;
   
   TilesetEditorSettingsWindow-->MainBook.selectPage(0);
   TilesetEditorSettingsWindow-->TilesetManagementBook.selectPage(0);
   
   TilesetEditorSettingsWindow-->ListBook.selectPage(0);
   TilesetEditorSettingsWindow-->SettingsBook.selectPage(0);
   
   TilesetEditor_SelectModeBtn.setStateOn(true);
   TilesetEditor_FloorModeBtn.setStateOn(true);
   
   TilesetEditor_RestrictWallstoFlooredBtn.setStateOn(true);
   
   %this.populateTilesetObjectsList();
   
   %this.loadTilesInfo();
}

function TilesetEditorPlugin::onDeactivated( %this )
{  
   %this.map.pop();
   
   Canvas.popDialog(TilesetEditorContainer);
   
   EWToolsPaletteWindow.Visible = true;
   EWTreeWindow.Visible = true;
   EWInspectorWindow.Visible = true;
   EWToolsToolbar.Visible = true;
   
   Parent::onDeactivated(%this);
   
   EWorldEditor.setEditorTool(0);

}

function TilesetEditorPlugin::populateTilesetObjectsList(%this)
{
   %objs = parseMissionGroupForIds("TilesetObject", "");
   %objsCount = getWordCount(%objs);
   
   TilesetObjListTreeView.clear();
   TilesetObjListTreeView.insertItem(0, "Tileset Objects", "");
   
   for(%i=0; %i < %objsCount; %i++)
   {
      %tilesetObj = getWord(%objs, %i);
      
      TilesetObjListTreeView.insertItem(1, "New Tileset Object", %tilesetObj);
   }
   
   TilesetObjListTreeView.buildVisibleTree(true);
}

function TilesetEditorTool_AddNewTilesetObj::onClick(%this)
{
   %newTileObj = new TilesetObject()
   {
      position = "0 0 0";
      rotation = "0 0 0";
   };
   
   MissionGroup.add(%newTileObj);
   
   TilesetEditorPlugin.populateTilesetObjectsList();
}

function TilesetEditorTool_DelTilesetObj::onClick(%this)
{
   
}

function TilesetObjListTreeView::onSelect(%this, %item)
{
   %tilesetObj = TilesetObjListTreeView.getItemValue(%item);
   
   TilesetEditor.setActiveObject(%tilesetObj);
   
   //populate the elements tree
   TilesetElemListTreeView.clear();
   
   TilesetEditor.getActiveTilesetObjectInfo(TilesetElemListTreeView);
}

function TilesetEditor_SelectModeBtn::onClick(%this)
{
   TilesetEditor.setActionMode("Select");
}

function TilesetEditor_PaintModeBtn::onClick(%this)
{
   TilesetEditor.setActionMode("Paint");
}

function TilesetEditor_EraseModeBtn::onClick(%this)
{
   TilesetEditor.setActionMode("Erase");
}

function TilesetEditor_BoxPaintModeBtn::onClick(%this)
{
   TilesetEditor.setActionMode("BoxPaint");
}

function TilesetEditor_FloorModeBtn::onClick(%this)
{
   TilesetEditor.setElementMode("Floor");
}

function TilesetEditor_WallModeBtn::onClick(%this)
{
   TilesetEditor.setElementMode("Wall");
}

function TilesetEditor_RaiseFloorBtn::onClick(%this)
{
   TilesetEditor.riseFloor();
}

function TilesetEditor_LowerFloorBtn::onClick(%this)
{
   TilesetEditor.lowerFloor();
}

function TilesetEditor_NextFloorBtn::onClick(%this)
{
   TilesetEditor.nextFloor();
}
function TilesetEditor_PrevFloorBtn::onClick(%this)
{
   TilesetEditor.prevFloor();
}