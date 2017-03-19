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

//------------------------------------------------------------------------------
// Shape Editor
//------------------------------------------------------------------------------

function initializeGameObjectEditor()
{
   echo(" % - Initializing GameObject Editor");

   exec("./gui/Profiles.ed.cs");

   exec("./gui/gameObjectEdPreviewWindow.ed.gui");
   exec("./gui/gameObjectEdAnimWindow.ed.gui");
   exec("./gui/gameObjectEdAdvancedWindow.ed.gui");
   exec("./gui/gameObjectEditorToolbar.ed.gui");
   exec("./gui/gameObjectEdSelectWindow.ed.gui");
   exec("./gui/gameObjectEdPropWindow.ed.gui");

   exec("./scripts/gameObjectEditor.ed.cs");
   exec("./scripts/gameObjectEditorHints.ed.cs");
   exec("./scripts/gameObjectEditorActions.ed.cs");

   // Add windows to editor gui
   GameObjectEdPreviewGui.setVisible(false);
   GameObjectEdAnimWindow.setVisible(false);

   GameObjectEditorToolbar.setVisible(false);
   GameObjectEdSelectWindow.setVisible(false);
   GameObjectEdPropWindow.setVisible(false);

   EditorGui.add(GameObjectEdPreviewGui);
   EditorGui.add(GameObjectEdAnimWindow);
   EditorGui.add(GameObjectEdAdvancedWindow);

   EditorGui.add(GameObjectEditorToolbar);
   EditorGui.add(GameObjectEdSelectWindow);
   EditorGui.add(GameObjectEdPropWindow);

   new ScriptObject(GameObjectEditorPlugin)
   {
      superClass = "EditorPlugin";
      editorGui = GameObjectEdShapeView;
   };

   %map = new ActionMap();
   %map.bindCmd( keyboard, "escape", "ToolsToolbarArray->WorldEditorInspectorPalette.performClick();", "" );
   %map.bindCmd( keyboard, "1", "GameObjectEditorNoneModeBtn.performClick();", "" );
   %map.bindCmd( keyboard, "2", "GameObjectEditorMoveModeBtn.performClick();", "" );
   %map.bindCmd( keyboard, "3", "GameObjectEditorRotateModeBtn.performClick();", "" );
   //%map.bindCmd( keyboard, "4", "GameObjectEditorScaleModeBtn.performClick();", "" ); // not needed for the shape editor
   %map.bindCmd( keyboard, "n", "GameObjectEditorToolbar->showNodes.performClick();", "" );
   %map.bindCmd( keyboard, "t", "GameObjectEditorToolbar->ghostMode.performClick();", "" );
   %map.bindCmd( keyboard, "r", "GameObjectEditorToolbar->wireframeMode.performClick();", "" );
   %map.bindCmd( keyboard, "f", "GameObjectEditorToolbar->fitToShapeBtn.performClick();", "" );
   %map.bindCmd( keyboard, "g", "GameObjectEditorToolbar->showGridBtn.performClick();", "" );
   %map.bindCmd( keyboard, "h", "GameObjectEdSelectWindow->tabBook.selectPage( 2 );", "" ); // Load help tab
   %map.bindCmd( keyboard, "l", "GameObjectEdSelectWindow->tabBook.selectPage( 1 );", "" ); // load Library Tab
   %map.bindCmd( keyboard, "j", "GameObjectEdSelectWindow->tabBook.selectPage( 0 );", "" ); // load scene object Tab
   %map.bindCmd( keyboard, "SPACE", "GameObjectEdAnimWindow.togglePause();", "" );
   %map.bindCmd( keyboard, "i", "GameObjectEdSequences.onEditSeqInOut(\"in\", GameObjectEdSeqSlider.getValue());", "" );
   %map.bindCmd( keyboard, "o", "GameObjectEdSequences.onEditSeqInOut(\"out\", GameObjectEdSeqSlider.getValue());", "" );
   %map.bindCmd( keyboard, "shift -", "GameObjectEdSeqSlider.setValue(GameObjectEdAnimWindow-->seqIn.getText());", "" );
   %map.bindCmd( keyboard, "shift =", "GameObjectEdSeqSlider.setValue(GameObjectEdAnimWindow-->seqOut.getText());", "" );
   %map.bindCmd( keyboard, "=", "GameObjectEdAnimWindow-->stepFwdBtn.performClick();", "" );
   %map.bindCmd( keyboard, "-", "GameObjectEdAnimWindow-->stepBkwdBtn.performClick();", "" );

   GameObjectEditorPlugin.map = %map;

   GameObjectEditorPlugin.initSettings();
}

function destroyGameObjectEditor()
{
}

function SetToggleButtonValue(%ctrl, %value)
{
   if ( %ctrl.getValue() != %value )
      %ctrl.performClick();
}

// Replace the command field in an Editor PopupMenu item (returns the original value)
function GameObjectEditorPlugin::replaceMenuCmd(%this, %menuTitle, %id, %newCmd)
{
   %menu = EditorGui.findMenu( %menuTitle );
   %cmd = getField( %menu.item[%id], 2 );
   %menu.setItemCommand( %id, %newCmd );

   return %cmd;
}

function GameObjectEditorPlugin::onWorldEditorStartup(%this)
{
   // Add ourselves to the window menu.
   %accel = EditorGui.addToEditorsMenu("GameObject Editor", "", GameObjectEditorPlugin);

   // Add ourselves to the ToolsToolbar
   %tooltip = "GameObject Editor (" @ %accel @ ")";
   EditorGui.addToToolsToolbar( "GameObjectEditorPlugin", "GameObjectEditorPalette", expandFilename("tools/worldEditor/images/toolbar/shape-editor"), %tooltip );

   // Add ourselves to the Editor Settings window
   exec( "./gui/GameObjectEditorSettingsTab.gui" );
   ESettingsWindow.addTabPage( EGameObjectEditorSettingsPage );

   GuiWindowCtrl::attach(GameObjectEdPropWindow, GameObjectEdSelectWindow);
   GameObjectEdAnimWindow.resize( -1, 526, 593, 53 );
   
   // Initialise gui
   GameObjectEdSeqNodeTabBook.selectPage(0);
   GameObjectEdAdvancedWindow-->tabBook.selectPage(0);
   GameObjectEdSelectWindow-->tabBook.selectPage(0);
   GameObjectEdSelectWindow.navigate("");

   SetToggleButtonValue( GameObjectEditorToolbar-->orbitNodeBtn, 0 );
   SetToggleButtonValue( GameObjectEditorToolbar-->ghostMode, 0 );

   // Initialise hints menu
   GameObjectEdHintMenu.clear();
   %count = ShapeHintGroup.getCount();
   for (%i = 0; %i < %count; %i++)
   {
      %hint = ShapeHintGroup.getObject(%i);
      GameObjectEdHintMenu.add(%hint.objectType, %hint);
   }
}

function GameObjectEditorPlugin::openShapeAsset(%this, %assetId)
{
   %asset = AssetDatabase.acquireAsset(%assetId);
   %this.open(%asset.fileName);
}

function GameObjectEditorPlugin::open(%this, %filename)
{
   if ( !%this.isActivated )
   {
      // Activate the Shape Editor
      EditorGui.setEditor( %this, true );

      // Get editor settings (note the sun angle is not configured in the settings
      // dialog, so apply the settings here instead of in readSettings)
      %this.readSettings();
      GameObjectEdShapeView.sunAngleX = EditorSettings.value("GameObjectEditor/SunAngleX");
      GameObjectEdShapeView.sunAngleZ = EditorSettings.value("GameObjectEditor/SunAngleZ");
      EWorldEditor.forceLoadDAE = EditorSettings.value("forceLoadDAE");

      $wasInWireFrameMode = $gfx::wireframe;
      GameObjectEditorToolbar-->wireframeMode.setStateOn($gfx::wireframe);

      if ( GlobalGizmoProfile.getFieldValue(alignment) $= "Object" )
         GameObjectEdNodes-->objectTransform.setStateOn(1);
      else
         GameObjectEdNodes-->worldTransform.setStateOn(1);

      // Initialise and show the shape editor
      GameObjectEdShapeTreeView.open(MissionGroup);
      GameObjectEdShapeTreeView.buildVisibleTree(true);

      GameObjectEdPreviewGui.setVisible(true);
      GameObjectEdSelectWindow.setVisible(true);
      GameObjectEdPropWindow.setVisible(true);
      GameObjectEdAnimWindow.setVisible(true);
      GameObjectEdAdvancedWindow.setVisible(GameObjectEditorToolbar-->showAdvanced.getValue());
      GameObjectEditorToolbar.setVisible(true);
      EditorGui.bringToFront(GameObjectEdPreviewGui);

      ToolsPaletteArray->WorldEditorMove.performClick();
      %this.map.push();

      // Switch to the GameObjectEditor UndoManager
      %this.oldUndoMgr = Editor.getUndoManager();
      Editor.setUndoManager( GameObjectEdUndoManager );

      GameObjectEdShapeView.setDisplayType( EditorGui.currentDisplayType );
      %this.initStatusBar();

      // Customise menu bar
      %this.oldCamFitCmd = %this.replaceMenuCmd( "Camera", 8, "GameObjectEdShapeView.fitToShape();" );
      %this.oldCamFitOrbitCmd = %this.replaceMenuCmd( "Camera", 9, "GameObjectEdShapeView.fitToShape();" );

      Parent::onActivated(%this);
   }

   // Select the new shape
   if (isObject(GameObjectEditor.shape) && (GameObjectEditor.shape.baseShape $= %filename))
   {
      // Shape is already selected => re-highlight the selected material if necessary
      GameObjectEdMaterials.updateSelectedMaterial(GameObjectEdMaterials-->highlightMaterial.getValue());
   }
   else if (%filename !$= "")
   {
      GameObjectEditor.selectShape(%filename, GameObjectEditor.isDirty());

      // 'fitToShape' only works after the GUI has been rendered, so force a repaint first
      Canvas.repaint();
      GameObjectEdShapeView.fitToShape();
   }
}

function GameObjectEditorPlugin::onActivated(%this)
{
   %this.open("");

   // Try to start with the shape selected in the world editor
   %count = EWorldEditor.getSelectionSize();
   for (%i = 0; %i < %count; %i++)
   {
      %obj = EWorldEditor.getSelectedObject(%i);
      %shapeFile = GameObjectEditor.getObjectShapeFile(%obj);
      if (%shapeFile !$= "")
      {
         if (!isObject(GameObjectEditor.shape) || (GameObjectEditor.shape.baseShape !$= %shapeFile))
         {
            // Call the 'onSelect' method directly if the object is not in the
            // MissionGroup tree (such as a Player or Projectile object).
            GameObjectEdShapeTreeView.clearSelection();
            if (!GameObjectEdShapeTreeView.selectItem(%obj))
               GameObjectEdShapeTreeView.onSelect(%obj);

            // 'fitToShape' only works after the GUI has been rendered, so force a repaint first
            Canvas.repaint();
            GameObjectEdShapeView.fitToShape();
         }
         break;
      }
   }
}

function GameObjectEditorPlugin::initStatusBar(%this)
{
   EditorGuiStatusBar.setInfo("GameObject editor ( Shift Click ) to speed up camera.");
   EditorGuiStatusBar.setSelection( GameObjectEditor.shape.baseShape );
}

function GameObjectEditorPlugin::onDeactivated(%this)
{
   %this.writeSettings();

   // Notify game objects if shape has been modified
   if ( GameObjectEditor.isDirty() )
      GameObjectEditor.shape.notifyShapeChanged();

   $gfx::wireframe = $wasInWireFrameMode;

   GameObjectEdMaterials.updateSelectedMaterial(false);
   GameObjectEditorToolbar.setVisible(false);

   GameObjectEdPreviewGui.setVisible(false);
   GameObjectEdSelectWindow.setVisible(false);
   GameObjectEdPropWindow.setVisible(false);
   GameObjectEdAnimWindow.setVisible(false);
   GameObjectEdAdvancedWindow.setVisible(false);
   
   if( EditorGui-->MatEdPropertiesWindow.visible )
   {
      GameObjectEdMaterials.editSelectedMaterialEnd( true );
   }

   %this.map.pop();

   // Restore the original undo manager
   Editor.setUndoManager( %this.oldUndoMgr );

   // Restore menu bar
   %this.replaceMenuCmd( "Camera", 8, %this.oldCamFitCmd );
   %this.replaceMenuCmd( "Camera", 9, %this.oldCamFitOrbitCmd );

   Parent::onDeactivated(%this);
}

function GameObjectEditorPlugin::onExitMission( %this )
{
   // unselect the current shape
   GameObjectEdShapeView.setModel( "" );
   if (GameObjectEditor.shape != -1)
      GameObjectEditor.shape.delete();
   GameObjectEditor.shape = 0;
   GameObjectEdUndoManager.clearAll();
   GameObjectEditor.setDirty( false );

   GameObjectEdSequenceList.clear();
   GameObjectEdNodeTreeView.removeItem( 0 );
   GameObjectEdPropWindow.update_onNodeSelectionChanged( -1 );
   GameObjectEdDetailTree.removeItem( 0 );
   GameObjectEdMaterialList.clear();

   GameObjectEdMountWindow-->mountList.clear();
   GameObjectEdThreadWindow-->seqList.clear();
   GameObjectEdThreadList.clear();
}

function GameObjectEditorPlugin::openShape( %this, %path, %discardChangesToCurrent )
{   
   EditorGui.setEditor( GameObjectEditorPlugin );
   
   if( GameObjectEditor.isDirty() && !%discardChangesToCurrent )
   {
      MessageBoxYesNo( "Save Changes?",
         "Save changes to current shape?",
         "GameObjectEditor.saveChanges(); GameObjectEditorPlugin.openShape(\"" @ %path @ "\");",
         "GameObjectEditorPlugin.openShape(\"" @ %path @ "\");" );
      return;
   }
   
   GameObjectEditor.selectShape( %path );
   GameObjectEdShapeView.fitToShape();
}

function GameObjectEditorWireframeMode()
{
   $gfx::wireframe = !$gfx::wireframe;
   GameObjectEditorToolbar-->wireframeMode.setStateOn($gfx::wireframe);
}

//-----------------------------------------------------------------------------
// Settings
//-----------------------------------------------------------------------------

function GameObjectEditorPlugin::initSettings( %this )
{
   EditorSettings.beginGroup( "GameObjectEditor", true );

   // Display options
   EditorSettings.setDefaultValue( "BackgroundColor",    "0 0 0 100" );
   EditorSettings.setDefaultValue( "HighlightMaterial", 1 );
   EditorSettings.setDefaultValue( "ShowNodes", 1 );
   EditorSettings.setDefaultValue( "ShowBounds", 0 );
   EditorSettings.setDefaultValue( "ShowObjBox", 1 );
   EditorSettings.setDefaultValue( "RenderMounts", 1 );
   EditorSettings.setDefaultValue( "RenderCollision", 0 );

   // Grid
   EditorSettings.setDefaultValue( "ShowGrid", 1 );
   EditorSettings.setDefaultValue( "GridSize", 0.1 );
   EditorSettings.setDefaultValue( "GridDimension", "40 40" );

   // Sun
   EditorSettings.setDefaultValue( "SunDiffuseColor",    "255 255 255 255" );
   EditorSettings.setDefaultValue( "SunAmbientColor",    "180 180 180 255" );
   EditorSettings.setDefaultValue( "SunAngleX",          "45" );
   EditorSettings.setDefaultValue( "SunAngleZ",          "135" );

   // Sub-windows
   EditorSettings.setDefaultValue( "AdvancedWndVisible",   "1" );

   EditorSettings.endGroup();
}

function GameObjectEditorPlugin::readSettings( %this )
{
   EditorSettings.beginGroup( "GameObjectEditor", true );

   // Display options
   GameObjectEdPreviewGui-->previewBackground.color = ColorIntToFloat( EditorSettings.value("BackgroundColor") );
   SetToggleButtonValue( GameObjectEdMaterials-->highlightMaterial, EditorSettings.value( "HighlightMaterial" ) );
   SetToggleButtonValue( GameObjectEditorToolbar-->showNodes, EditorSettings.value( "ShowNodes" ) );
   SetToggleButtonValue( GameObjectEditorToolbar-->showBounds, EditorSettings.value( "ShowBounds" ) );
   SetToggleButtonValue( GameObjectEditorToolbar-->showObjBox, EditorSettings.value( "ShowObjBox" ) );
   SetToggleButtonValue( GameObjectEditorToolbar-->renderColMeshes, EditorSettings.value( "RenderCollision" ) );
   SetToggleButtonValue( GameObjectEdMountWindow-->renderMounts, EditorSettings.value( "RenderMounts" ) );

   // Grid
   SetToggleButtonValue( GameObjectEditorToolbar-->showGridBtn, EditorSettings.value( "ShowGrid" ) );
   GameObjectEdShapeView.gridSize = EditorSettings.value( "GridSize" );
   GameObjectEdShapeView.gridDimension = EditorSettings.value( "GridDimension" );

   // Sun
   GameObjectEdShapeView.sunDiffuse = EditorSettings.value("SunDiffuseColor");
   GameObjectEdShapeView.sunAmbient = EditorSettings.value("SunAmbientColor");

   // Sub-windows
   SetToggleButtonValue( GameObjectEditorToolbar-->showAdvanced, EditorSettings.value( "AdvancedWndVisible" ) );

   EditorSettings.endGroup();
}

function GameObjectEditorPlugin::writeSettings( %this )
{
   EditorSettings.beginGroup( "GameObjectEditor", true );

   // Display options
   EditorSettings.setValue( "BackgroundColor",     ColorFloatToInt( GameObjectEdPreviewGui-->previewBackground.color ) );
   EditorSettings.setValue( "HighlightMaterial",   GameObjectEdMaterials-->highlightMaterial.getValue() );
   EditorSettings.setValue( "ShowNodes",           GameObjectEditorToolbar-->showNodes.getValue() );
   EditorSettings.setValue( "ShowBounds",          GameObjectEditorToolbar-->showBounds.getValue() );
   EditorSettings.setValue( "ShowObjBox",          GameObjectEditorToolbar-->showObjBox.getValue() );
   EditorSettings.setValue( "RenderCollision",     GameObjectEditorToolbar-->renderColMeshes.getValue() );
   EditorSettings.setValue( "RenderMounts",        GameObjectEdMountWindow-->renderMounts.getValue() );

   // Grid
   EditorSettings.setValue( "ShowGrid",            GameObjectEditorToolbar-->showGridBtn.getValue() );
   EditorSettings.setValue( "GridSize",            GameObjectEdShapeView.gridSize );
   EditorSettings.setValue( "GridDimension",       GameObjectEdShapeView.gridDimension );

   // Sun
   EditorSettings.setValue( "SunDiffuseColor",     GameObjectEdShapeView.sunDiffuse );
   EditorSettings.setValue( "SunAmbientColor",     GameObjectEdShapeView.sunAmbient );
   EditorSettings.setValue( "SunAngleX",           GameObjectEdShapeView.sunAngleX );
   EditorSettings.setValue( "SunAngleZ",           GameObjectEdShapeView.sunAngleZ );

   // Sub-windows
   EditorSettings.setValue( "AdvancedWndVisible",    GameObjectEditorToolbar-->showAdvanced.getValue() );

   EditorSettings.endGroup();
}
