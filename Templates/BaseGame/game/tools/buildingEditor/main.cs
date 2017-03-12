//-----------------------------------------------------------------------------
// Copyright (C) Sickhead Games, LLC
//-----------------------------------------------------------------------------

function initializebuildingEditor()
{
   echo(" % - Initializing Building Editor");
     
   exec( "./buildingEditor.cs" );
   exec( "./buildingEditorGui.gui" );
   exec( "./buildingEditorToolbar.gui");
   exec( "./buildingEditorGui.cs" );
   exec( "./buildingEditorPropGui.gui" );
   
   buildingEditorGui.setVisible( false );  
   buildingEditorOptionsWindow.setVisible( false );  
   buildingEditorToolbar.setVisible( false ); 
   buildingEditorTreeWindow.setVisible( false ); 
   
   //prop editor
   LibWindow.setVisible( false );   
   //prop editor
   
   EditorGui.add( buildingEditorGui );
   EditorGui.add( buildingEditorOptionsWindow );
   EditorGui.add( buildingEditorToolbar );
   EditorGui.add( buildingEditorTreeWindow );
   
   //prop editor
    EditorGui.add(LibWindow);
   //prop editor
      
   new ScriptObject( buildingEditorPlugin )
   {
      superClass = "EditorPlugin";
      editorGui = buildingEditorGui;
   };
   
   %map = new ActionMap();
   %map.bindCmd( keyboard, "backspace", "buildingEditorGui.deleteNode();", "" );
   %map.bindCmd( keyboard, "1", "ToolsPaletteArray->BuildingEditorSelectMode.performClick();", "" );  
   %map.bindCmd( keyboard, "2", "ToolsPaletteArray->buildingEditorMoveMode.performClick();", "" );  
   %map.bindCmd( keyboard, "3", "ToolsPaletteArray->buildingEditorRotateMode.performClick();", "" );  
   %map.bindCmd( keyboard, "4", "ToolsPaletteArray->buildingEditorScaleMode.performClick();", "" );  
   %map.bindCmd( keyboard, "5", "ToolsPaletteArray->buildingEditorAddRoadMode.performClick();", "" );  
   %map.bindCmd( keyboard, "-", "ToolsPaletteArray->buildingEditorInsertPointMode.performClick();", "" );  
   %map.bindCmd( keyboard, "=", "ToolsPaletteArray->buildingEditorRemovePointMode.performClick();", "" );
   %map.bindCmd( keyboard, "8", "ToolsPaletteArray->BuildingEditorInsertPointMode.performClick();", "" );
   %map.bindCmd( keyboard, "9", "ToolsPaletteArray->BuildingEditorDeletePointMode.performClick();", "" );
     
   %map.bindCmd( keyboard, "z", "buildingEditorShowSplineBtn.performClick();", "" );  
   %map.bindCmd( keyboard, "x", "buildingEditorWireframeBtn.performClick();", "" );  
   %map.bindCmd( keyboard, "v", "buildingEditorShowRoadBtn.performClick();", "" );  
   buildingEditorPlugin.map = %map;
   
   buildingEditorPlugin.initSettings();
}

function destroybuildingEditor()
{
}

function buildingEditorPlugin::onWorldEditorStartup( %this )
{     
   // Add ourselves to the window menu.
   %accel = EditorGui.addToEditorsMenu( "Building Editor", "", buildingEditorPlugin );
   
   // Add ourselves to the ToolsToolbar
   %tooltip = "Building Editor (" @ %accel @ ")";   
   EditorGui.addToToolsToolbar( "buildingEditorPlugin", "buildingEditorPalette", expandFilename("tools/worldEditor/images/toolbar/mesh-road-editor"), %tooltip );

   //connect editor windows
   GuiWindowCtrl::attach( buildingEditorOptionsWindow, buildingEditorTreeWindow);
   
   // Add ourselves to the Editor Settings window
   exec( "./buildingEditorSettingsTab.gui" );
   ESettingsWindow.addTabPage( EbuildingEditorSettingsPage );
}

function buildingEditorPlugin::onActivated( %this )
{
   %this.readSettings();
   
   ToolsPaletteArray->buildingEditorAddRoadMode.performClick();
   EditorGui.bringToFront( buildingEditorGui );
   buildingEditorGui.setVisible( true );
   buildingEditorGui.makeFirstResponder( true );
   buildingEditorOptionsWindow.setVisible( true );
   buildingEditorToolbar.setVisible( true );  
   buildingEditorTreeWindow.setVisible( true );
   buildingTreeView.open(ServerbuildingSet,true);
   %this.map.push();
   
   //intitilize it to not show the prop editor
   $propEd = false;
   EArchEditor.setVisible( false );  
   LibWindow.setVisible( false ); 
   
   // Store this on a dynamic field
   // in order to restore whatever setting
   // the user had before.
   %this.prevGizmoAlignment = GlobalGizmoProfile.alignment;
   
   // The DecalEditor always uses Object alignment.
   GlobalGizmoProfile.alignment = "Object";
   
   // Set the status bar here until all tool have been hooked up
   EditorGuiStatusBar.setInfo("Building editor.");
   EditorGuiStatusBar.setSelection("");
   
   Parent::onActivated(%this);
}

function buildingEditorPlugin::onDeactivated( %this )
{   
   %this.writeSettings();
   
   buildingEditorGui.setVisible( false );
   buildingEditorOptionsWindow.setVisible( false );
   buildingEditorToolbar.setVisible( false );  
   buildingEditorTreeWindow.setVisible( false );
   %this.map.pop();
   
   $propEd = false;
   EArchEditor.setVisible( false );  
   LibWindow.setVisible( false ); 
   
   // Restore the previous Gizmo
   // alignment settings.
   GlobalGizmoProfile.alignment = %this.prevGizmoAlignment; 
   
   Parent::onDeactivated(%this);  
}

function buildingEditorPlugin::toggleMenus( %this, %editor )
{
   if(%editor $= "buildingEditor")
   {
      //disable the prop editor
      LibWindow.setVisible( false ); 
     
      buildingEditorOptionsWindow.setVisible( true );
      buildingEditorToolbar.setVisible( true );  
      buildingEditorTreeWindow.setVisible( true );
   }
   else if(%editor $= "propEditor")
   {
      //disable the prop editor
      LibWindow.setVisible( true ); 
     
      buildingEditorOptionsWindow.setVisible( false );
      buildingEditorToolbar.setVisible( false );  
      buildingEditorTreeWindow.setVisible( false );
   }
}

function buildingEditorPlugin::onEditMenuSelect( %this, %editMenu )
{
   %hasSelection = false;
   
   if( isObject( buildingEditorGui.road ) )
      %hasSelection = true;
      
   %editMenu.enableItem( 3, false ); // Cut
   %editMenu.enableItem( 4, false ); // Copy
   %editMenu.enableItem( 5, false ); // Paste  
   %editMenu.enableItem( 6, %hasSelection ); // Delete
   %editMenu.enableItem( 8, false ); // Deselect   
}

function buildingEditorPlugin::handleDelete( %this )
{
   buildingEditorGui.deleteNode();
}

function buildingEditorPlugin::handleEscape( %this )
{
   return buildingEditorGui.onEscapePressed();  
}

function buildingEditorPlugin::isDirty( %this )
{
   return buildingEditorGui.isDirty;
}

function buildingEditorPlugin::onSaveMission( %this, %missionFile )
{
   if( buildingEditorGui.isDirty )
   {
      MissionGroup.save( %missionFile );
      buildingEditorGui.isDirty = false;
   }
}

//-----------------------------------------------------------------------------
// Settings
//-----------------------------------------------------------------------------

function buildingEditorPlugin::initSettings( %this )
{
   EditorSettings.beginGroup( "buildingEditor", true );
   
   EditorSettings.setDefaultValue(  "DefaultWidth",         "10" );
   EditorSettings.setDefaultValue(  "DefaultDepth",         "5" );
   EditorSettings.setDefaultValue(  "DefaultNormal",        "0 0 1" );
   EditorSettings.setDefaultValue(  "HoverSplineColor",     "255 0 0 255" );
   EditorSettings.setDefaultValue(  "SelectedSplineColor",  "0 255 0 255" );
   EditorSettings.setDefaultValue(  "HoverNodeColor",       "255 255 255 255" ); //<-- Not currently used
   EditorSettings.setDefaultValue(  "TopMaterialName",      "DefaultRoadMaterialTop" );
   EditorSettings.setDefaultValue(  "BottomMaterialName",   "DefaultRoadMaterialOther" );
   EditorSettings.setDefaultValue(  "SideMaterialName",     "DefaultRoadMaterialOther" );
   
   EditorSettings.endGroup();
}

function buildingEditorPlugin::readSettings( %this )
{
   EditorSettings.beginGroup( "buildingEditor", true );
   
   buildingEditorGui.DefaultWidth         = EditorSettings.value("DefaultWidth");
   buildingEditorGui.DefaultDepth         = EditorSettings.value("DefaultDepth");
   buildingEditorGui.DefaultNormal        = EditorSettings.value("DefaultNormal");
   buildingEditorGui.HoverSplineColor     = EditorSettings.value("HoverSplineColor");
   buildingEditorGui.SelectedSplineColor  = EditorSettings.value("SelectedSplineColor");
   buildingEditorGui.HoverNodeColor       = EditorSettings.value("HoverNodeColor");
   buildingEditorGui.topMaterialName      = EditorSettings.value("TopMaterialName");
   buildingEditorGui.bottomMaterialName   = EditorSettings.value("BottomMaterialName");
   buildingEditorGui.sideMaterialName     = EditorSettings.value("SideMaterialName");
   
   EditorSettings.endGroup();  
}

function buildingEditorPlugin::writeSettings( %this )
{
   EditorSettings.beginGroup( "buildingEditor", true );
   
   EditorSettings.setValue( "DefaultWidth",           buildingEditorGui.DefaultWidth );
   EditorSettings.setValue( "DefaultDepth",           buildingEditorGui.DefaultDepth );
   EditorSettings.setValue( "DefaultNormal",          buildingEditorGui.DefaultNormal );
   EditorSettings.setValue( "HoverSplineColor",       buildingEditorGui.HoverSplineColor );
   EditorSettings.setValue( "SelectedSplineColor",    buildingEditorGui.SelectedSplineColor );
   EditorSettings.setValue( "HoverNodeColor",         buildingEditorGui.HoverNodeColor );
   EditorSettings.setValue( "TopMaterialName",        buildingEditorGui.topMaterialName );
   EditorSettings.setValue( "BottomMaterialName",     buildingEditorGui.bottomMaterialName );
   EditorSettings.setValue( "SideMaterialName",       buildingEditorGui.sideMaterialName );
   
   EditorSettings.endGroup();
}