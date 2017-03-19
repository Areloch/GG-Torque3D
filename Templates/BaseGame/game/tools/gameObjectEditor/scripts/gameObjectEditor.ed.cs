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

// @todo:
//
// - split node transform editboxes into X Y Z and rot X Y Z with spin controls
//   to allow easier manual editing
// - add groundspeed editing ( use same format as node transform editing )
//
// Known bugs/limitations:
//
// - resizing the GuiTextListCtrl should resize the columns as well
// - modifying the from/in/out properties of a sequence will change the sequence
//   order in the shape ( since it results in remove/add sequence commands )
// - deleting a node should not delete its children as well?
//

//------------------------------------------------------------------------------
// Utility Methods
//------------------------------------------------------------------------------

if ( !isObject( GameObjectEditor ) ) new ScriptObject( GameObjectEditor )
{
   shape = -1;
   deletedCount = 0;
};


// Capitalise the first letter of the input string
function strcapitalise( %str )
{
   %len = strlen( %str );
   return strupr( getSubStr( %str,0,1 ) ) @ getSubStr( %str,1,%len-1 );
}

function GameObjectEditor::getObjectShapeFile( %this, %obj )
{
   // Get the path to the shape file used by the given object (not perfect, but
   // works for the vast majority of object types)
   %path = "";
   if ( %obj.isMemberOfClass( "TSStatic" ) )
      %path = %obj.shapeName;
   else if ( %obj.isMemberOfClass( "PhysicsShape" ) )
      %path = %obj.getDataBlock().shapeName;
   else if ( %obj.isMemberOfClass( "GameBase" ) )
      %path = %obj.getDataBlock().shapeFile;
      
   return %path;
}

// Check if the given name already exists
function GameObjectEditor::nameExists( %this, %type, %name )
{
   if ( GameObjectEditor.shape == -1 )
      return false;

   if ( %type $= "node" )
      return ( GameObjectEditor.shape.getNodeIndex( %name ) >= 0 );
   else if ( %type $= "sequence" )
      return ( GameObjectEditor.shape.getSequenceIndex( %name ) >= 0 );
   else if ( %type $= "object" )
      return ( GameObjectEditor.shape.getObjectIndex( %name ) >= 0 );
}

// Check if the given 'hint' name exists (spaces could also be underscores)
function GameObjectEditor::hintNameExists( %this, %type, %name )
{
   if ( GameObjectEditor.nameExists( %type, %name ) )
      return true;

   // If the name contains spaces, try replacing with underscores
   %name = strreplace( %name, " ", "_" );
   if ( GameObjectEditor.nameExists( %type, %name ) )
      return true;

   return false;
}

// Generate a unique name from a given base by appending an integer
function GameObjectEditor::getUniqueName( %this, %type, %name )
{
   for ( %idx = 1; %idx < 100; %idx++ )
   {
      %uniqueName = %name @ %idx;
      if ( !%this.nameExists( %type, %uniqueName ) )
         break;
   }

   return %uniqueName;
}

function GameObjectEditor::getProxyName( %this, %seqName )
{
   return "__proxy__" @ %seqName;
}

function GameObjectEditor::getUnproxyName( %this, %proxyName )
{
   return strreplace( %proxyName, "__proxy__", "" );
}

function GameObjectEditor::getBackupName( %this, %seqName )
{
   return "__backup__" @ %seqName;
}

// Check if this mesh name is a collision hint
function GameObjectEditor::isCollisionMesh( %this, %name )
{
   return ( startswith( %name, "ColBox" ) ||
            startswith( %name, "ColSphere" ) ||
            startswith( %name, "ColCapsule" ) ||
            startswith( %name, "ColConvex" ) );
}

// 
function GameObjectEditor::getSequenceSource( %this, %seqName )
{
   %source = %this.shape.getSequenceSource( %seqName );

   // Use the sequence name as the source for DTS built-in sequences
   %src0 = getField( %source, 0 );
   %src1 = getField( %source, 1 );
   if ( %src0 $= %src1 )
      %source = setField( %source, 1, "" );
   if ( %src0 $= "" )
      %source = setField( %source, 0, %seqName );

   return %source;
}

// Recursively get names for a node and its children
function GameObjectEditor::getNodeNames( %this, %nodeName, %names, %exclude )
{
   if ( %nodeName $= %exclude )
      return %names;

   %count = %this.shape.getNodeChildCount( %nodeName );
   for ( %i = 0; %i < %count; %i++ )
   {
      %childName = %this.shape.getNodeChildName( %nodeName, %i );
      %names = %this.getNodeNames( %childName, %names, %exclude );
   }

   %names = %names TAB %nodeName;

   return trim( %names );
}

// Get the list of meshes for a particular object
function GameObjectEditor::getObjectMeshList( %this, %name )
{
   %list = "";
   %count = %this.shape.getMeshCount( %name );
   for ( %i = 0; %i < %count; %i++ )
      %list = %list TAB %this.shape.getMeshName( %name, %i );
   return trim( %list );
}

// Get the list of meshes for a particular detail level
function GameObjectEditor::getDetailMeshList( %this, %detSize )
{
   %list = "";
   %objCount = GameObjectEditor.shape.getObjectCount();
   for ( %i = 0; %i < %objCount; %i++ )
   {
      %objName = GameObjectEditor.shape.getObjectName( %i );
      %meshCount = GameObjectEditor.shape.getMeshCount( %objName );
      for ( %j = 0; %j < %meshCount; %j++ )
      {
         %size = GameObjectEditor.shape.getMeshSize( %objName, %j );
         if ( %size == %detSize )
            %list = %list TAB %this.shape.getMeshName( %objName, %j );
      }
   }
   return trim( %list );
}

function GameObjectEditor::isDirty( %this )
{
   return ( isObject( %this.shape ) && GameObjectEdPropWindow-->saveBtn.isActive() );
}

function GameObjectEditor::setDirty( %this, %dirty )
{
   if ( %dirty )
      GameObjectEdSelectWindow.text = "Shapes *";
   else
      GameObjectEdSelectWindow.text = "Shapes";

   GameObjectEdPropWindow-->saveBtn.setActive( %dirty );
}

function GameObjectEditor::saveChanges( %this )
{
   if ( isObject( GameObjectEditor.shape ) )
   {
      GameObjectEditor.saveConstructor( GameObjectEditor.shape );
      GameObjectEditor.shape.writeChangeSet();
      GameObjectEditor.shape.notifyShapeChanged();      // Force game objects to reload shape
      GameObjectEditor.setDirty( false );
   }
}

//------------------------------------------------------------------------------
// Shape Selection
//------------------------------------------------------------------------------

function GameObjectEditor::findConstructor( %this, %path )
{
   %count = TSShapeConstructorGroup.getCount();
   for ( %i = 0; %i < %count; %i++ )
   {
      %obj = TSShapeConstructorGroup.getObject( %i );
      if ( %obj.baseShape $= %path )
         return %obj;
   }
   return -1;
}

function GameObjectEditor::createConstructor( %this, %path )
{
   %name = strcapitalise( fileBase( %path ) ) @ strcapitalise( getSubStr( fileExt( %path ), 1, 3 ) );
   %name = strreplace( %name, "-", "_" );
   %name = strreplace( %name, ".", "_" );
   %name = getUniqueName( %name );
   return new TSShapeConstructor( %name ) { baseShape = %path; };
}

function GameObjectEditor::saveConstructor( %this, %constructor )
{
   %savepath = filePath( %constructor.baseShape ) @ "/" @ fileBase( %constructor.baseShape ) @ ".cs";
   new PersistenceManager( GameObjectEd_perMan );
   GameObjectEd_perMan.setDirty( %constructor, %savepath );
   GameObjectEd_perMan.saveDirtyObject( %constructor );
   GameObjectEd_perMan.delete();
}

// Handle a selection in the shape selector list
function GameObjectEdSelectWindow::onSelect( %this, %path )
{
   // Prompt user to save the old shape if it is dirty
   if ( GameObjectEditor.isDirty() )
   {
      %cmd = "ColladaImportDlg.showDialog( \"" @ %path @ "\", \"GameObjectEditor.selectShape( \\\"" @ %path @ "\\\", ";
      MessageBoxYesNoCancel( "Shape Modified", "Would you like to save your changes?", %cmd @ "true );\" );", %cmd @ "false );\" );" );
   }
   else
   {
      %cmd = "GameObjectEditor.selectShape( \"" @ %path @ "\", false );";
      ColladaImportDlg.showDialog( %path, %cmd );
   }
}

function GameObjectEditor::selectShape( %this, %path, %saveOld )
{
   GameObjectEdShapeView.setModel( "" );

   if ( %saveOld )
   {
      // Save changes to a TSShapeConstructor script
      %this.saveChanges();
   }
   else if ( GameObjectEditor.isDirty() )
   {
      // Purge all unsaved changes
      %oldPath = GameObjectEditor.shape.baseShape;
      GameObjectEditor.shape.delete();
      GameObjectEditor.shape = 0;

      reloadResource( %oldPath );   // Force game objects to reload shape
   }

   // Initialise the shape preview window
   if ( !GameObjectEdShapeView.setModel( %path ) )
   {
      MessageBoxOK( "Error", "Failed to load '" @ %path @ "'. Check the console for error messages." );
      return;
   }
   GameObjectEdShapeView.fitToShape();

   GameObjectEdUndoManager.clearAll();
   GameObjectEditor.setDirty( false );

   // Get ( or create ) the TSShapeConstructor object for this shape
   GameObjectEditor.shape = GameObjectEditor.findConstructor( %path );
   if ( GameObjectEditor.shape <= 0 )
   {
      GameObjectEditor.shape = %this.createConstructor( %path );
      if ( GameObjectEditor.shape <= 0 )
      {
         error( "GameObjectEditor: Error - could not select " @ %path );
         return;
      }
   }

   // Initialise the editor windows
   GameObjectEdAdvancedWindow.update_onShapeSelectionChanged();
   GameObjectEdMountWindow.update_onShapeSelectionChanged();
   GameObjectEdThreadWindow.update_onShapeSelectionChanged();
   GameObjectEdColWindow.update_onShapeSelectionChanged();
   GameObjectEdPropWindow.update_onShapeSelectionChanged();
   GameObjectEdShapeView.refreshShape();

   // Update object type hints
   GameObjectEdSelectWindow.updateHints();

   // Update editor status bar
   EditorGuiStatusBar.setSelection( %path );
}

// Handle a selection in the MissionGroup shape selector
function GameObjectEdShapeTreeView::onSelect( %this, %obj )
{
   %path = GameObjectEditor.getObjectShapeFile( %obj );
   if ( %path !$= "" )
      GameObjectEdSelectWindow.onSelect( %path );

   // Set the object type (for required nodes and sequences display)
   %objClass = %obj.getClassName();
   %hintId = -1;

   %count = ShapeHintGroup.getCount();
   for ( %i = 0; %i < %count; %i++ )
   {
      %hint = ShapeHintGroup.getObject( %i );
      if ( %objClass $= %hint.objectType )
      {
         %hintId = %hint;
         break;
      }
      else if ( isMemberOfClass( %objClass, %hint.objectType ) )
      {
         %hintId = %hint;
      }
   }
   GameObjectEdHintMenu.setSelected( %hintId );
}

// Find all DTS or COLLADA models. Note: most of this section was shamelessly
// stolen from creater.ed.cs => great work whoever did the original!
function GameObjectEdSelectWindow::navigate( %this, %address )
{
   // Freeze the icon array so it doesn't update until we've added all of the
   // icons
   %this-->shapeLibrary.frozen = true;
   %this-->shapeLibrary.clear();
   GameObjectEdSelectMenu.clear();

   %filePatterns = getFormatExtensions();
   %fullPath = findFirstFileMultiExpr( %filePatterns );

   while ( %fullPath !$= "" )
   {
      // Ignore cached DTS files
      if ( endswith( %fullPath, "cached.dts" ) )
      {
         %fullPath = findNextFileMultiExpr( %filePatterns );
         continue;
      }

      // Ignore assets in the tools folder
      %fullPath = makeRelativePath( %fullPath, getMainDotCSDir() );
      %splitPath = strreplace( %fullPath, " ", "_" );
      %splitPath = strreplace( %splitPath, "/", " " );
      if ( getWord( %splitPath, 0 ) $= "tools" )
      {
         %fullPath = findNextFileMultiExpr( %filePatterns );
         continue;
      }

      %dirCount = getWordCount( %splitPath ) - 1;
      %pathFolders = getWords( %splitPath, 0, %dirCount - 1 );

      // Add this file's path ( parent folders ) to the
      // popup menu if it isn't there yet.
      %temp = strreplace( %pathFolders, " ", "/" );
      %temp = strreplace( %temp, "_", " " );
      %r = GameObjectEdSelectMenu.findText( %temp );
      if ( %r == -1 )
         GameObjectEdSelectMenu.add( %temp );

      // Is this file in the current folder?
      if ( stricmp( %pathFolders, %address ) == 0 )
      {
         %this.addShapeIcon( %fullPath );
      }
      // Then is this file in a subfolder we need to add
      // a folder icon for?
      else
      {
         %wordIdx = 0;
         %add = false;

         if ( %address $= "" )
         {
            %add = true;
            %wordIdx = 0;
         }
         else
         {
            for ( ; %wordIdx < %dirCount; %wordIdx++ )
            {
               %temp = getWords( %splitPath, 0, %wordIdx );
               if ( stricmp( %temp, %address ) == 0 )
               {
                  %add = true;
                  %wordIdx++;
                  break;
               }
            }
         }

         if ( %add == true )
         {
            %folder = getWord( %splitPath, %wordIdx );

            // Add folder icon if not already present
            %ctrl = %this.findIconCtrl( %folder );
            if ( %ctrl == -1 )
               %this.addFolderIcon( %folder );
         }
      }

      %fullPath = findNextFileMultiExpr( %filePatterns );
   }

   %this-->shapeLibrary.sort( "alphaIconCompare" );
   for ( %i = 0; %i < %this-->shapeLibrary.getCount(); %i++ )
      %this-->shapeLibrary.getObject( %i ).autoSize = false;

   %this-->shapeLibrary.frozen = false;
   %this-->shapeLibrary.refresh();
   %this.address = %address;

   GameObjectEdSelectMenu.sort();

   %str = strreplace( %address, " ", "/" );
   %r = GameObjectEdSelectMenu.findText( %str );
   if ( %r != -1 )
      GameObjectEdSelectMenu.setSelected( %r, false );
   else
      GameObjectEdSelectMenu.setText( %str );
}

function GameObjectEdSelectWindow::navigateDown( %this, %folder )
{
   if ( %this.address $= "" )
      %address = %folder;
   else
      %address = %this.address SPC %folder;

   // Because this is called from an IconButton::onClick command
   // we have to wait a tick before actually calling navigate, else
   // we would delete the button out from under itself.
   %this.schedule( 1, "navigate", %address );
}

function GameObjectEdSelectWindow::navigateUp( %this )
{
   %count = getWordCount( %this.address );

   if ( %count == 0 )
      return;

   if ( %count == 1 )
      %address = "";
   else
      %address = getWords( %this.address, 0, %count - 2 );

   %this.navigate( %address );
}

function GameObjectEdSelectWindow::findIconCtrl( %this, %name )
{
   for ( %i = 0; %i < %this-->shapeLibrary.getCount(); %i++ )
   {
      %ctrl = %this-->shapeLibrary.getObject( %i );
      if ( %ctrl.text $= %name )
         return %ctrl;
   }
   return -1;
}

function GameObjectEdSelectWindow::createIcon( %this )
{
   %ctrl = new GuiIconButtonCtrl()
   {
      profile = "GuiCreatorIconButtonProfile";
      iconLocation = "Left";
      textLocation = "Right";
      extent = "348 19";
      textMargin = 8;
      buttonMargin = "2 2";
      autoSize = false;
      sizeIconToButton = true;
      makeIconSquare = true;
      buttonType = "radioButton";
      groupNum = "-1";   
   };

   return %ctrl;
}

function GameObjectEdSelectWindow::addFolderIcon( %this, %text )
{
   %ctrl = %this.createIcon();

   %ctrl.altCommand = "GameObjectEdSelectWindow.navigateDown( \"" @ %text @ "\" );";
   %ctrl.iconBitmap = "tools/gui/images/folder.png";
   %ctrl.text = %text;
   %ctrl.tooltip = %text;
   %ctrl.class = "CreatorFolderIconBtn";
   
   %ctrl.buttonType = "radioButton";
   %ctrl.groupNum = "-1";
   
   %this-->shapeLibrary.addGuiControl( %ctrl );
}

function GameObjectEdSelectWindow::addShapeIcon( %this, %fullPath )
{
   %ctrl = %this.createIcon();

   %ext = fileExt( %fullPath );
   %file = fileBase( %fullPath );
   %fileLong = %file @ %ext;
   %tip = %fileLong NL
          "Size: " @ fileSize( %fullPath ) / 1000.0 SPC "KB" NL
          "Date Created: " @ fileCreatedTime( %fullPath ) NL
          "Last Modified: " @ fileModifiedTime( %fullPath );

   %ctrl.altCommand = "GameObjectEdSelectWindow.onSelect( \"" @ %fullPath @ "\" );";
   %ctrl.iconBitmap = ( ( %ext $= ".dts" ) ? EditorIconRegistry::findIconByClassName( "TSStatic" ) : "tools/gui/images/iconCollada" );
   %ctrl.text = %file;
   %ctrl.class = "CreatorStaticIconBtn";
   %ctrl.tooltip = %tip;
   
   %ctrl.buttonType = "radioButton";
   %ctrl.groupNum = "-1";

   // Check if a shape specific icon is available
   %formats = ".png .jpg .dds .bmp .gif .jng .tga";
   %count = getWordCount( %formats );
   for ( %i = 0; %i < %count; %i++ )
   {
      %ext = getWord( %formats, %i );
      if ( isFile( %fullPath @ %ext ) )
      {
         %ctrl.iconBitmap = %fullPath @ %ext;
         break;
      }
   }

   %this-->shapeLibrary.addGuiControl( %ctrl );
}

function GameObjectEdSelectMenu::onSelect( %this, %id, %text )
{
   %split = strreplace( %text, "/", " " );
   GameObjectEdSelectWindow.navigate( %split );
}

// Update the GUI in response to the shape selection changing
function GameObjectEdPropWindow::update_onShapeSelectionChanged( %this )
{
   // --- NODES TAB ---
   GameObjectEdNodeTreeView.removeItem( 0 );
   %rootId = GameObjectEdNodeTreeView.insertItem( 0, "<root>", 0, "" );
   %count = GameObjectEditor.shape.getNodeCount();
   for ( %i = 0; %i < %count; %i++ )
   {
      %name = GameObjectEditor.shape.getNodeName( %i );
      if ( GameObjectEditor.shape.getNodeParentName( %name ) $= "" )
         GameObjectEdNodeTreeView.addNodeTree( %name );
   }
   %this.update_onNodeSelectionChanged( -1 );    // no node selected

   // --- SEQUENCES TAB ---
   GameObjectEdSequenceList.clear();
   GameObjectEdSequenceList.addRow( -1, "Name" TAB "Cyclic" TAB "Blend" TAB "Frames" TAB "Priority" );
   GameObjectEdSequenceList.setRowActive( -1, false );
   GameObjectEdSequenceList.addRow( 0, "<rootpose>" TAB "" TAB "" TAB "" TAB "" );

   %count = GameObjectEditor.shape.getSequenceCount();
   for ( %i = 0; %i < %count; %i++ )
   {
      %name = GameObjectEditor.shape.getSequenceName( %i );

      // Ignore __backup__ sequences (only used by editor)
      if ( !startswith( %name, "__backup__" ) )
         GameObjectEdSequenceList.addItem( %name );
   }
   GameObjectEdThreadWindow.onAddThread();        // add thread 0

   // --- DETAILS TAB ---
   // Add detail levels and meshes to tree
   GameObjectEdDetailTree.clearSelection();
   GameObjectEdDetailTree.removeItem( 0 );
   %root = GameObjectEdDetailTree.insertItem( 0, "<root>", "", "" );
   %objCount = GameObjectEditor.shape.getObjectCount();
   for ( %i = 0; %i < %objCount; %i++ )
   {
      %objName = GameObjectEditor.shape.getObjectName( %i );
      %meshCount = GameObjectEditor.shape.getMeshCount( %objName );
      for ( %j = 0; %j < %meshCount; %j++ )
      {
         %meshName = GameObjectEditor.shape.getMeshName( %objName, %j );
         GameObjectEdDetailTree.addMeshEntry( %meshName, 1 );
      }
   }

   // Initialise object node list
   GameObjectEdDetails-->objectNode.clear();
   GameObjectEdDetails-->objectNode.add( "<root>" );
   %nodeCount = GameObjectEditor.shape.getNodeCount();
   for ( %i = 0; %i < %nodeCount; %i++ )
      GameObjectEdDetails-->objectNode.add( GameObjectEditor.shape.getNodeName( %i ) );

   // --- MATERIALS TAB ---
   GameObjectEdMaterials.updateMaterialList();
}

//------------------------------------------------------------------------------
// Shape Hints
//------------------------------------------------------------------------------

function GameObjectEdHintMenu::onSelect( %this, %id, %text )
{
   GameObjectEdSelectWindow.updateHints();
}

function GameObjectEdSelectWindow::updateHints( %this )
{
   %objectType = GameObjectEdHintMenu.getText();

   GameObjectEdSelectWindow-->nodeHints.freeze( true );
   GameObjectEdSelectWindow-->sequenceHints.freeze( true );

   // Move all current hint controls to a holder SimGroup
   for ( %i = GameObjectEdSelectWindow-->nodeHints.getCount()-1; %i >= 0; %i-- )
      ShapeHintControls.add( GameObjectEdSelectWindow-->nodeHints.getObject( %i ) );
   for ( %i = GameObjectEdSelectWindow-->sequenceHints.getCount()-1; %i >= 0; %i-- )
      ShapeHintControls.add( GameObjectEdSelectWindow-->sequenceHints.getObject( %i ) );

   // Update node and sequence hints, modifying and/or creating gui controls as needed
   for ( %i = 0; %i < ShapeHintGroup.getCount(); %i++ )
   {
      %hint = ShapeHintGroup.getObject( %i );
      if ( ( %objectType $= %hint.objectType ) || isMemberOfClass( %objectType, %hint.objectType ) )
      {
         for ( %idx = 0; %hint.node[%idx] !$= ""; %idx++ )
            GameObjectEdHintMenu.processHint( "node", %hint.node[%idx] );

         for ( %idx = 0; %hint.sequence[%idx] !$= ""; %idx++ )
            GameObjectEdHintMenu.processHint( "sequence", %hint.sequence[%idx] );
      }
   }

   GameObjectEdSelectWindow-->nodeHints.freeze( false );
   GameObjectEdSelectWindow-->nodeHints.updateStack();
   GameObjectEdSelectWindow-->sequenceHints.freeze( false );
   GameObjectEdSelectWindow-->sequenceHints.updateStack();

}

function GameObjectEdHintMenu::processHint( %this, %type, %hint )
{
   %name = getField( %hint, 0 );
   %desc = getField( %hint, 1 );

   // check for arrayed names (ending in 0-N or 1-N)
   %pos = strstr( %name, "0-" );
   if ( %pos == -1 )
      %pos = strstr( %name, "1-" );

   if ( %pos > 0 )
   {
      // arrayed name => add controls for each name in the array, but collapse
      // consecutive indices where possible. eg.  if the model only has nodes
      // mount1-3, we should create: mount0 (red), mount1-3 (green), mount4-31 (red)
      %base = getSubStr( %name, 0, %pos );      // array name
      %first = getSubStr( %name, %pos, 1 );     // first index
      %last = getSubStr( %name, %pos+2, 3 );    // last index

      // get the state of the first element
      %arrayStart = %first;
      %prevPresent = GameObjectEditor.hintNameExists( %type, %base @ %first );

      for ( %j = %first + 1; %j <= %last; %j++ )
      {
         // if the state of this element is different to the previous one, we
         // need to add a hint
         %present = GameObjectEditor.hintNameExists( %type, %base @ %j );
         if ( %present != %prevPresent )
         {
            GameObjectEdSelectWindow.addObjectHint( %type, %base, %desc, %prevPresent, %arrayStart, %j-1 );
            %arrayStart = %j;
            %prevPresent = %present;
         }
      }

      // add hint for the last group
      GameObjectEdSelectWindow.addObjectHint( %type, %base, %desc, %prevPresent, %arrayStart, %last );
   }
   else
   {
      // non-arrayed name
      %present = GameObjectEditor.hintNameExists( %type, %name );
      GameObjectEdSelectWindow.addObjectHint( %type, %name, %desc, %present );
   }
}

function GameObjectEdSelectWindow::addObjectHint( %this, %type, %name, %desc, %present, %start, %end )
{
   // Get a hint gui control (create one if needed)
   if ( ShapeHintControls.getCount() == 0 )
   {
      // Create a new hint gui control
      %ctrl = new GuiIconButtonCtrl()
      {
         profile = "GuiCreatorIconButtonProfile";
         iconLocation = "Left";
         textLocation = "Right";
         extent = "348 19";
         textMargin = 8;
         buttonMargin = "2 2";
         autoSize = true;
         buttonType = "radioButton";
         groupNum = "-1";
         iconBitmap = "tools/editorClasses/gui/images/iconCancel";
         text = "hint";
         tooltip = "";
      };

      ShapeHintControls.add( %ctrl );
   }
   %ctrl = ShapeHintControls.getObject( 0 );

   // Initialise the control, then add it to the appropriate list
   %name = %name @ %start;
   if ( %end !$= %start )
      %ctrl.text = %name @ "-" @ %end;
   else
      %ctrl.text = %name;

   %ctrl.tooltip = %desc;
   %ctrl.setBitmap( "tools/editorClasses/gui/images/" @ ( %present ? "iconAccept" : "iconCancel" ) );
   %ctrl.setStateOn( false );
   %ctrl.resetState();

   switch$ ( %type )
   {
      case "node":
         %ctrl.altCommand = %present ? "" : "GameObjectEdNodes.onAddNode( \"" @ %name @ "\" );";
         GameObjectEdSelectWindow-->nodeHints.addGuiControl( %ctrl );
      case "sequence":
         %ctrl.altCommand = %present ? "" : "GameObjectEdSequences.onAddSequence( \"" @ %name @ "\" );";
         GameObjectEdSelectWindow-->sequenceHints.addGuiControl( %ctrl );
   }
}

//------------------------------------------------------------------------------

function GameObjectEdSeqNodeTabBook::onTabSelected( %this, %name, %index )
{
   %this.activePage = %name;

   switch$ ( %name )
   {
      case "Seq":
         GameObjectEdPropWindow-->newBtn.ToolTip = "Add new sequence";
         GameObjectEdPropWindow-->newBtn.Command = "GameObjectEdSequences.onAddSequence();";
         GameObjectEdPropWindow-->newBtn.setActive( true );
         GameObjectEdPropWindow-->deleteBtn.ToolTip = "Delete selected sequence (cannot be undone)";
         GameObjectEdPropWindow-->deleteBtn.Command = "GameObjectEdSequences.onDeleteSequence();";
         GameObjectEdPropWindow-->deleteBtn.setActive( true );

      case "Node":
         GameObjectEdPropWindow-->newBtn.ToolTip = "Add new node";
         GameObjectEdPropWindow-->newBtn.Command = "GameObjectEdNodes.onAddNode();";
         GameObjectEdPropWindow-->newBtn.setActive( true );
         GameObjectEdPropWindow-->deleteBtn.ToolTip = "Delete selected node (cannot be undone)";
         GameObjectEdPropWindow-->deleteBtn.Command = "GameObjectEdNodes.onDeleteNode();";
         GameObjectEdPropWindow-->deleteBtn.setActive( true );

      case "Detail":
         GameObjectEdPropWindow-->newBtn.ToolTip = "";
         GameObjectEdPropWindow-->newBtn.Command = "";
         GameObjectEdPropWindow-->newBtn.setActive( false );
         GameObjectEdPropWindow-->deleteBtn.ToolTip = "Delete the selected mesh or detail level (cannot be undone)";
         GameObjectEdPropWindow-->deleteBtn.Command = "GameObjectEdDetails.onDeleteMesh();";
         GameObjectEdPropWindow-->deleteBtn.setActive( true );

      case "Mat":
         GameObjectEdPropWindow-->newBtn.ToolTip = "";
         GameObjectEdPropWindow-->newBtn.Command = "";
         GameObjectEdPropWindow-->newBtn.setActive( false );
         GameObjectEdPropWindow-->deleteBtn.ToolTip = "";
         GameObjectEdPropWindow-->deleteBtn.Command = "";
         GameObjectEdPropWindow-->deleteBtn.setActive( false );

         // For some reason, the header is not resized correctly until the Materials tab has been
         // displayed at least once, so resize it here too
         GameObjectEdMaterials-->materialListHeader.setExtent( getWord( GameObjectEdMaterialList.extent, 0 ) SPC "19" );
   }
}

//------------------------------------------------------------------------------
// Node Editing
//------------------------------------------------------------------------------

// Update the GUI in response to the node selection changing
function GameObjectEdPropWindow::update_onNodeSelectionChanged( %this, %id )
{
   if ( %id > 0 )
   {
      // Enable delete button and edit boxes
      if ( GameObjectEdSeqNodeTabBook.activePage $= "Node" )
         GameObjectEdPropWindow-->deleteBtn.setActive( true );
      GameObjectEdNodes-->nodeName.setActive( true );
      GameObjectEdNodes-->nodePosition.setActive( true );
      GameObjectEdNodes-->nodeRotation.setActive( true );

      // Update the node inspection data
      %name = GameObjectEdNodeTreeView.getItemText( %id );

      GameObjectEdNodes-->nodeName.setText( %name );

      // Node parent list => ancestor and sibling nodes only (can't re-parent to a descendent)
      GameObjectEdNodeParentMenu.clear();
      %parentNames = GameObjectEditor.getNodeNames( "", "<root>", %name );
      %count = getWordCount( %parentNames );
      for ( %i = 0; %i < %count; %i++ )
         GameObjectEdNodeParentMenu.add( getWord(%parentNames, %i), %i );

      %pName = GameObjectEditor.shape.getNodeParentName( %name );
      if ( %pName $= "" )
         %pName = "<root>";
      GameObjectEdNodeParentMenu.setText( %pName );

      if ( GameObjectEdNodes-->worldTransform.getValue() )
      {
         // Global transform
         %txfm = GameObjectEditor.shape.getNodeTransform( %name, 1 );
         GameObjectEdNodes-->nodePosition.setText( getWords( %txfm, 0, 2 ) );
         GameObjectEdNodes-->nodeRotation.setText( getWords( %txfm, 3, 6 ) );
      }
      else
      {
         // Local transform (relative to parent)
         %txfm = GameObjectEditor.shape.getNodeTransform( %name, 0 );
         GameObjectEdNodes-->nodePosition.setText( getWords( %txfm, 0, 2 ) );
         GameObjectEdNodes-->nodeRotation.setText( getWords( %txfm, 3, 6 ) );
      }

      GameObjectEdShapeView.selectedNode = GameObjectEditor.shape.getNodeIndex( %name );
   }
   else
   {
      // Disable delete button and edit boxes
      if ( GameObjectEdSeqNodeTabBook.activePage $= "Node" ) 
         GameObjectEdPropWindow-->deleteBtn.setActive( false );
      GameObjectEdNodes-->nodeName.setActive( false );
      GameObjectEdNodes-->nodePosition.setActive( false );
      GameObjectEdNodes-->nodeRotation.setActive( false );

      GameObjectEdNodes-->nodeName.setText( "" );
      GameObjectEdNodes-->nodePosition.setText( "" );
      GameObjectEdNodes-->nodeRotation.setText( "" );

      GameObjectEdShapeView.selectedNode = -1;
   }
}

// Update the GUI in response to a node being added
function GameObjectEdPropWindow::update_onNodeAdded( %this, %nodeName, %oldTreeIndex )
{
   // --- MISC ---
   GameObjectEdShapeView.refreshShape();
   GameObjectEdShapeView.updateNodeTransforms();
   GameObjectEdSelectWindow.updateHints();

   // --- MOUNT WINDOW ---
   if ( GameObjectEdMountWindow.isMountableNode( %nodeName ) )
   {
      GameObjectEdMountWindow-->mountNode.add( %nodeName );
      GameObjectEdMountWindow-->mountNode.sort();
   }

   // --- NODES TAB ---
   %id = GameObjectEdNodeTreeView.addNodeTree( %nodeName );
   if ( %oldTreeIndex <= 0 )
   {
      // This is a new node => make it the current selection
      if ( %id > 0 )
      {
         GameObjectEdNodeTreeView.clearSelection();
         GameObjectEdNodeTreeView.selectItem( %id );
      }
   }
   else
   {
      // This node has been un-deleted. Inserting a new item puts it at the
      // end of the siblings, but we want to restore the original order as
      // if the item was never deleted, so move it up as required.
      %childIndex = GameObjectEdNodeTreeView.getChildIndexByName( %nodeName );
      while ( %childIndex > %oldTreeIndex )
      {
         GameObjectEdNodeTreeView.moveItemUp( %id );
         %childIndex--;
      }
   }

   // --- DETAILS TAB ---
   GameObjectEdDetails-->objectNode.add( %nodeName );
}

// Update the GUI in response to a node(s) being removed
function GameObjectEdPropWindow::update_onNodeRemoved( %this, %nameList, %nameCount )
{
   // --- MISC ---
   GameObjectEdShapeView.refreshShape();
   GameObjectEdShapeView.updateNodeTransforms();
   GameObjectEdSelectWindow.updateHints();

   // Remove nodes from the mountable list, and any shapes mounted to the node
   for ( %i = 0; %i < %nameCount; %i++ )
   {
      %nodeName = getField( %nameList, %i );
      GameObjectEdMountWindow-->mountNode.clearEntry( GameObjectEdMountWindow-->mountNode.findText( %nodeName ) );

      for ( %j = GameObjectEdMountWindow-->mountList.rowCount()-1; %j >= 1; %j-- )
      {
         %text = GameObjectEdMountWindow-->mountList.getRowText( %j );
         if ( getField( %text, 1 ) $= %nodeName )
         {
            GameObjectEdShapeView.unmountShape( %j-1 );
            GameObjectEdMountWindow-->mountList.removeRow( %j );
         }
      }
   }

   // --- NODES TAB ---
   %lastName = getField( %nameList, %nameCount-1 );
   %id = GameObjectEdNodeTreeView.findItemByName( %lastName );   // only need to remove the parent item
   if ( %id > 0 )
   {
      GameObjectEdNodeTreeView.removeItem( %id );
      if ( GameObjectEdNodeTreeView.getSelectedItem() <= 0 )
         GameObjectEdPropWindow.update_onNodeSelectionChanged( -1 );
   }

   // --- DETAILS TAB ---
   for ( %i = 0; %i < %nameCount; %i++ )
   {
      %nodeName = getField( %nameList, %i );
      GameObjectEdDetails-->objectNode.clearEntry( GameObjectEdDetails-->objectNode.findText( %nodeName ) );
   }
}

// Update the GUI in response to a node being renamed
function GameObjectEdPropWindow::update_onNodeRenamed( %this, %oldName, %newName )
{
   // --- MISC ---
   GameObjectEdSelectWindow.updateHints();

   // --- MOUNT WINDOW ---
   // Update entries for any shapes mounted to this node
   %rowCount = GameObjectEdMountWindow-->mountList.rowCount();
   for ( %i = 1; %i < %rowCount; %i++ )
   {
      %text = GameObjectEdMountWindow-->mountList.getRowText( %i );
      if ( getField( %text, 1 ) $= %oldName )
      {
         %text = setField( %text, 1, %newName );
         GameObjectEdMountWindow-->mountList.setRowById( GameObjectEdMountWindow-->mountList.getRowId( %i ), %text );
      }
   }

   // Update list of mountable nodes
   GameObjectEdMountWindow-->mountNode.clearEntry( GameObjectEdMountWindow-->mountNode.findText( %oldName ) );
   if ( GameObjectEdMountWindow.isMountableNode( %newName ) )
   {
      GameObjectEdMountWindow-->mountNode.add( %newName );
      GameObjectEdMountWindow-->mountNode.sort();
   }

   // --- NODES TAB ---
   %id = GameObjectEdNodeTreeView.findItemByName( %oldName );
   GameObjectEdNodeTreeView.editItem( %id, %newName, 0 );
   if ( GameObjectEdNodeTreeView.getSelectedItem() == %id )
      GameObjectEdNodes-->nodeName.setText( %newName );

   // --- DETAILS TAB ---
   %id = GameObjectEdDetails-->objectNode.findText( %oldName );
   if ( %id != -1 )
   {
      GameObjectEdDetails-->objectNode.clearEntry( %id );
      GameObjectEdDetails-->objectNode.add( %newName, %id );
      GameObjectEdDetails-->objectNode.sortID();
      if ( GameObjectEdDetails-->objectNode.getText() $= %oldName )
         GameObjectEdDetails-->objectNode.setText( %newName );
   }
}

// Update the GUI in response to a node's parent being changed
function GameObjectEdPropWindow::update_onNodeParentChanged( %this, %nodeName )
{
   // --- MISC ---
   GameObjectEdShapeView.updateNodeTransforms();

   // --- NODES TAB ---
   %isSelected = 0;
   %id = GameObjectEdNodeTreeView.findItemByName( %nodeName );
   if ( %id > 0 )
   {
      %isSelected = ( GameObjectEdNodeTreeView.getSelectedItem() == %id );
      GameObjectEdNodeTreeView.removeItem( %id );
   }
   GameObjectEdNodeTreeView.addNodeTree( %nodeName );
   if ( %isSelected )
      GameObjectEdNodeTreeView.selectItem( GameObjectEdNodeTreeView.findItemByName( %nodeName ) );
}

function GameObjectEdPropWindow::update_onNodeTransformChanged( %this, %nodeName )
{
   // Default to the selected node if none is specified
   if ( %nodeName $= "" )
   {
      %id = GameObjectEdNodeTreeView.getSelectedItem();
      if ( %id > 0 )
         %nodeName = GameObjectEdNodeTreeView.getItemText( %id );
      else
         return;
   }

   // --- MISC ---
   GameObjectEdShapeView.updateNodeTransforms();
   if ( GameObjectEdNodes-->objectTransform.getValue() )
      GlobalGizmoProfile.setFieldValue(alignment, Object);
   else
      GlobalGizmoProfile.setFieldValue(alignment, World);

   // --- NODES TAB ---
   // Update the node transform fields if necessary
   %id = GameObjectEdNodeTreeView.getSelectedItem();
   if ( ( %id > 0 ) && ( GameObjectEdNodeTreeView.getItemText( %id ) $= %nodeName ) )
   {
      %isWorld = GameObjectEdNodes-->worldTransform.getValue();
      %transform = GameObjectEditor.shape.getNodeTransform( %nodeName, %isWorld );
      GameObjectEdNodes-->nodePosition.setText( getWords( %transform, 0, 2 ) );
      GameObjectEdNodes-->nodeRotation.setText( getWords( %transform, 3, 6 ) );
   }
}

function GameObjectEdNodeTreeView::onClearSelection( %this )
{
   GameObjectEdPropWindow.update_onNodeSelectionChanged( -1 );
}

function GameObjectEdNodeTreeView::onSelect( %this, %id )
{
   // Update the node name and transform controls
   GameObjectEdPropWindow.update_onNodeSelectionChanged( %id );

   // Update orbit position if orbiting the selected node
   if ( GameObjectEdShapeView.orbitNode )
   {
      %name = %this.getItemText( %id );
      %transform = GameObjectEditor.shape.getNodeTransform( %name, 1 );
      GameObjectEdShapeView.setOrbitPos( getWords( %transform, 0, 2 ) );
   }
}

function GameObjectEdShapeView::onNodeSelected( %this, %index )
{
   GameObjectEdNodeTreeView.clearSelection();
   if ( %index > 0 )
   {
      %name = GameObjectEditor.shape.getNodeName( %index );
      %id = GameObjectEdNodeTreeView.findItemByName( %name );
      if ( %id > 0 )
         GameObjectEdNodeTreeView.selectItem( %id );
   }
}

function GameObjectEdNodes::onAddNode( %this, %name )
{
   // Add a new node, using the currently selected node as the initial parent
   if ( %name $= "" )
      %name = GameObjectEditor.getUniqueName( "node", "myNode" );

   %id = GameObjectEdNodeTreeView.getSelectedItem();
   if ( %id <= 0 )
      %parent = "";
   else
      %parent = GameObjectEdNodeTreeView.getItemText( %id );

   GameObjectEditor.doAddNode( %name, %parent, "0 0 0 0 0 1 0" );
}

function GameObjectEdNodes::onDeleteNode( %this )
{
   // Remove the node and all its children from the shape
   %id = GameObjectEdNodeTreeView.getSelectedItem();
   if ( %id > 0 )
   {
      %name = GameObjectEdNodeTreeView.getItemText( %id );
      GameObjectEditor.doRemoveShapeData( "Node", %name );
   }
}

// Determine the index of a node in the tree relative to its parent
function GameObjectEdNodeTreeView::getChildIndexByName( %this, %name )
{
   %id = %this.findItemByName( %name );
   %parentId = %this.getParentItem( %id );
   %childId = %this.getChild( %parentId );
   if ( %childId <= 0 )
      return 0;   // bad!

   %index = 0;
   while ( %childId != %id )
   {
      %childId = %this.getNextSibling( %childId );
      %index++;
   }

   return %index;
}

// Add a node and its children to the node tree view
function GameObjectEdNodeTreeView::addNodeTree( %this, %nodeName )
{
   // Abort if already added => something dodgy has happened and we'd end up
   // recursing indefinitely
   if ( %this.findItemByName( %nodeName ) )
   {
      error( "Recursion error in GameObjectEdNodeTreeView::addNodeTree" );
      return 0;
   }

   // Find parent and add me to it
   %parentName = GameObjectEditor.shape.getNodeParentName( %nodeName );
   if ( %parentName $= "" )
      %parentName = "<root>";

   %parentId = %this.findItemByName( %parentName );
   %id = %this.insertItem( %parentId, %nodeName, 0, "" );

   // Add children
   %count = GameObjectEditor.shape.getNodeChildCount( %nodeName );
   for ( %i = 0; %i < %count; %i++ )
      %this.addNodeTree( GameObjectEditor.shape.getNodeChildName( %nodeName, %i ) );

   return %id;
}

function GameObjectEdNodes::onEditName( %this )
{
   %id = GameObjectEdNodeTreeView.getSelectedItem();
   if ( %id > 0 )
   {
      %oldName = GameObjectEdNodeTreeView.getItemText( %id );
      %newName = %this-->nodeName.getText();
      if ( %newName !$= "" )
         GameObjectEditor.doRenameNode( %oldName, %newName );
   }
}

function GameObjectEdNodeParentMenu::onSelect( %this, %id, %text )
{
   %id = GameObjectEdNodeTreeView.getSelectedItem();
   if ( %id > 0 )
   {
      %name = GameObjectEdNodeTreeView.getItemText( %id );
      GameObjectEditor.doSetNodeParent( %name, %text );
   }
}

function GameObjectEdNodes::onEditTransform( %this )
{
   %id = GameObjectEdNodeTreeView.getSelectedItem();
   if ( %id > 0 )
   {
      %name = GameObjectEdNodeTreeView.getItemText( %id );

      // Get the node transform from the gui
      %pos = %this-->nodePosition.getText();
      %rot = %this-->nodeRotation.getText();
      %txfm = %pos SPC %rot;
      %isWorld = GameObjectEdNodes-->worldTransform.getValue();

      // Do a quick sanity check to avoid setting wildly invalid transforms
      for ( %i = 0; %i < 7; %i++ )    // "x y z aa.x aa.y aa.z aa.angle"
      {
         if ( getWord( %txfm, %i ) $= "" )
            return;
      }

      GameObjectEditor.doEditNodeTransform( %name, %txfm, %isWorld, -1 );
   }
}

function GameObjectEdShapeView::onEditNodeTransform( %this, %node, %txfm, %gizmoID )
{
   GameObjectEditor.doEditNodeTransform( %node, %txfm, 1, %gizmoID );
}

//------------------------------------------------------------------------------
// Sequence Editing
//------------------------------------------------------------------------------

function GameObjectEdPropWindow::onWake( %this )
{
   GameObjectEdTriggerList.triggerId = 1;

   GameObjectEdTriggerList.addRow( -1, "-1" TAB "Frame" TAB "Trigger" TAB "State" );
   GameObjectEdTriggerList.setRowActive( -1, false );
}

function GameObjectEdPropWindow::update_onSeqSelectionChanged( %this )
{
   // Sync the Thread window sequence selection
   %row = GameObjectEdSequenceList.getSelectedRow();
   if ( GameObjectEdThreadWindow-->seqList.getSelectedRow() != ( %row-1 ) )
   {
      GameObjectEdThreadWindow-->seqList.setSelectedRow( %row-1 );
      return;  // selecting a sequence in the Thread window will re-call this function
   }

   GameObjectEdSeqFromMenu.clear();
   GameObjectEdSequences-->blendSeq.clear();

   // Clear the trigger list
   GameObjectEdTriggerList.removeAll();

   // Update the active sequence data
   %seqName = GameObjectEdSequenceList.getSelectedName();
   if ( %seqName !$= "" )
   {
      // Enable delete button and edit boxes
      if ( GameObjectEdSeqNodeTabBook.activePage $= "Seq" ) 
         GameObjectEdPropWindow-->deleteBtn.setActive( true );
      GameObjectEdSequences-->seqName.setActive( true );
      GameObjectEdSequences-->blendFlag.setActive( true );
      GameObjectEdSequences-->cyclicFlag.setActive( true );
      GameObjectEdSequences-->priority.setActive( true );
      GameObjectEdSequences-->addTriggerBtn.setActive( true );
      GameObjectEdSequences-->deleteTriggerBtn.setActive( true );

      // Initialise the sequence properties
      %blendData = GameObjectEditor.shape.getSequenceBlend( %seqName );
      GameObjectEdSequences-->seqName.setText( %seqName );
      GameObjectEdSequences-->cyclicFlag.setValue( GameObjectEditor.shape.getSequenceCyclic( %seqName ) );
      GameObjectEdSequences-->blendFlag.setValue( getField( %blendData, 0 ) );
      GameObjectEdSequences-->priority.setText( GameObjectEditor.shape.getSequencePriority( %seqName ) );

      // 'From' and 'Blend' sequence menus
      GameObjectEdSeqFromMenu.add( "Browse..." );
      %count = GameObjectEdSequenceList.rowCount();
      for ( %i = 2; %i < %count; %i++ )  // skip header row and <rootpose>
      {
         %name = GameObjectEdSequenceList.getItemName( %i );
         if ( %name !$= %seqName )
         {
            GameObjectEdSeqFromMenu.add( %name );
            GameObjectEdSequences-->blendSeq.add( %name );
         }
      }
      GameObjectEdSequences-->blendSeq.setText( getField( %blendData, 1 ) );
      GameObjectEdSequences-->blendFrame.setText( getField( %blendData, 2 ) );

      %this.syncPlaybackDetails();

      // Triggers (must occur after syncPlaybackDetails is called so the slider range is correct)
      %count = GameObjectEditor.shape.getTriggerCount( %seqName );
      for ( %i = 0; %i < %count; %i++ )
      {
         %trigger = GameObjectEditor.shape.getTrigger( %seqName, %i );
         GameObjectEdTriggerList.addItem( getWord( %trigger, 0 ), getWord( %trigger, 1 ) );
      }
   }
   else
   {
      // Disable delete button and edit boxes
      if ( GameObjectEdSeqNodeTabBook.activePage $= "Seq" ) 
         GameObjectEdPropWindow-->deleteBtn.setActive( false );
      GameObjectEdSequences-->seqName.setActive( false );
      GameObjectEdSequences-->blendFlag.setActive( false );
      GameObjectEdSequences-->cyclicFlag.setActive( false );
      GameObjectEdSequences-->priority.setActive( false );
      GameObjectEdSequences-->addTriggerBtn.setActive( false );
      GameObjectEdSequences-->deleteTriggerBtn.setActive( false );

      // Clear sequence properties
      GameObjectEdSequences-->seqName.setText( "" );
      GameObjectEdSequences-->cyclicFlag.setValue( 0 );
      GameObjectEdSequences-->blendSeq.setText( "" );
      GameObjectEdSequences-->blendFlag.setValue( 0 );
      GameObjectEdSequences-->priority.setText( 0 );

      %this.syncPlaybackDetails();
   }

   %this.onTriggerSelectionChanged();

   GameObjectEdSequences-->sequenceListHeader.setExtent( getWord( GameObjectEdSequenceList.extent, 0 ) SPC "19" );

   // Reset current frame
   //GameObjectEdAnimWindow.setKeyframe( GameObjectEdAnimWindow-->seqIn.getText() );
}

// Update the GUI in response to a sequence being added
function GameObjectEdPropWindow::update_onSequenceAdded( %this, %seqName, %oldIndex )
{
   // --- MISC ---
   GameObjectEdSelectWindow.updateHints();

   // --- SEQUENCES TAB ---
   if ( %oldIndex == -1 )
   {
      // This is a brand new sequence => add it to the list and make it the
      // current selection
      %row = GameObjectEdSequenceList.insertItem( %seqName, GameObjectEdSequenceList.rowCount() );
      GameObjectEdSequenceList.scrollVisible( %row );
      GameObjectEdSequenceList.setSelectedRow( %row );
   }
   else
   {
      // This sequence has been un-deleted => add it back to the list at the
      // original position
      GameObjectEdSequenceList.insertItem( %seqName, %oldIndex );
   }
}

function GameObjectEdPropWindow::update_onSequenceRemoved( %this, %seqName )
{
   // --- MISC ---
   GameObjectEdSelectWindow.updateHints();

   // --- SEQUENCES TAB ---
   %isSelected = ( GameObjectEdSequenceList.getSelectedName() $= %seqName );
   GameObjectEdSequenceList.removeItem( %seqName );
   if ( %isSelected )
      GameObjectEdPropWindow.update_onSeqSelectionChanged();

   // --- THREADS WINDOW ---
   GameObjectEdShapeView.refreshThreadSequences();
}

function GameObjectEdPropWindow::update_onSequenceRenamed( %this, %oldName, %newName )
{
   // --- MISC ---
   GameObjectEdSelectWindow.updateHints();

   // Rename the proxy sequence as well
   %oldProxy = GameObjectEditor.getProxyName( %oldName );
   %newProxy = GameObjectEditor.getProxyName( %newName );
   if ( GameObjectEditor.shape.getSequenceIndex( %oldProxy ) != -1 )
      GameObjectEditor.shape.renameSequence( %oldProxy, %newProxy );

   // --- SEQUENCES TAB ---
   GameObjectEdSequenceList.editColumn( %oldName, 0, %newName );
   if ( GameObjectEdSequenceList.getSelectedName() $= %newName )
      GameObjectEdSequences-->seqName.setText( %newName );

   // --- THREADS WINDOW ---
   // Update any threads that use this sequence
   %active = GameObjectEdShapeView.activeThread;
   for ( %i = 0; %i < GameObjectEdShapeView.getThreadCount(); %i++ )
   {
      GameObjectEdShapeView.activeThread = %i;
      if ( GameObjectEdShapeView.getThreadSequence() $= %oldName )
         GameObjectEdShapeView.setThreadSequence( %newName, 0, GameObjectEdShapeView.threadPos, 0 );
      else if ( GameObjectEdShapeView.getThreadSequence() $= %oldProxy )
         GameObjectEdShapeView.setThreadSequence( %newProxy, 0, GameObjectEdShapeView.threadPos, 0 );
   }
   GameObjectEdShapeView.activeThread = %active;
}

function GameObjectEdPropWindow::update_onSequenceCyclicChanged( %this, %seqName, %cyclic )
{
   // --- MISC ---
   // Apply the same transformation to the proxy animation if necessary
   %proxyName = GameObjectEditor.getProxyName( %seqName );
   if ( GameObjectEditor.shape.getSequenceIndex( %proxyName ) != -1 )
      GameObjectEditor.shape.setSequenceCyclic( %proxyName, %cyclic );

   // --- SEQUENCES TAB ---
   GameObjectEdSequenceList.editColumn( %seqName, 1, %cyclic ? "yes" : "no" );
   if ( GameObjectEdSequenceList.getSelectedName() $= %seqName )
      GameObjectEdSequences-->cyclicFlag.setStateOn( %cyclic );
}

function GameObjectEdPropWindow::update_onSequenceBlendChanged( %this, %seqName, %blend,
                              %oldBlendSeq, %oldBlendFrame, %blendSeq, %blendFrame )
{
   // --- MISC ---
   // Apply the same transformation to the proxy animation if necessary
   %proxyName = GameObjectEditor.getProxyName( %seqName );
   if ( GameObjectEditor.shape.getSequenceIndex( %proxyName ) != -1 )
   {
      if ( %blend && %oldBlend )
         GameObjectEditor.shape.setSequenceBlend( %proxyName, false, %oldBlendSeq, %oldBlendFrame );
      GameObjectEditor.shape.setSequenceBlend( %proxyName, %blend, %blendSeq, %blendFrame );
   }
   GameObjectEdShapeView.updateNodeTransforms();

   // --- SEQUENCES TAB ---
   GameObjectEdSequenceList.editColumn( %seqName, 2, %blend ? "yes" : "no" );
   if ( GameObjectEdSequenceList.getSelectedName() $= %seqName )
   {
      GameObjectEdSequences-->blendFlag.setStateOn( %blend );
      GameObjectEdSequences-->blendSeq.setText( %blendSeq );
      GameObjectEdSequences-->blendFrame.setText( %blendFrame );
   }
}

function GameObjectEdPropWindow::update_onSequencePriorityChanged( %this, %seqName )
{
   // --- SEQUENCES TAB ---
   %priority = GameObjectEditor.shape.getSequencePriority( %seqName );
   GameObjectEdSequenceList.editColumn( %seqName, 4, %priority );
   if ( GameObjectEdSequenceList.getSelectedName() $= %seqName )
      GameObjectEdSequences-->priority.setText( %priority );
}

function GameObjectEdPropWindow::update_onSequenceGroundSpeedChanged( %this, %seqName )
{
   // nothing to do yet
}

function GameObjectEdPropWindow::syncPlaybackDetails( %this )
{
   %seqName = GameObjectEdSequenceList.getSelectedName();
   if ( %seqName !$= "" )
   {
      // Show sequence in/out bars
      GameObjectEdAnimWindow-->seqInBar.setVisible( true );
      GameObjectEdAnimWindow-->seqOutBar.setVisible( true );

      // Sync playback controls
      %sourceData = GameObjectEditor.getSequenceSource( %seqName );
      %seqFrom = rtrim( getFields( %sourceData, 0, 1 ) );
      %seqStart = getField( %sourceData, 2 );
      %seqEnd = getField( %sourceData, 3 );
      %seqFromTotal = getField( %sourceData, 4 );

      // Display the original source for edited sequences
      if ( startswith( %seqFrom, "__backup__" ) )
      {
         %backupData = GameObjectEditor.getSequenceSource( getField( %seqFrom, 0 ) );
         %seqFrom = rtrim( getFields( %backupData, 0, 1 ) );
      }

      GameObjectEdSeqFromMenu.setText( %seqFrom );
      GameObjectEdSeqFromMenu.tooltip = GameObjectEdSeqFromMenu.getText();   // use tooltip to show long names
      GameObjectEdSequences-->startFrame.setText( %seqStart );
      GameObjectEdSequences-->endFrame.setText( %seqEnd );

      %val = GameObjectEdSeqSlider.getValue() / getWord( GameObjectEdSeqSlider.range, 1 );
      GameObjectEdSeqSlider.range = "0" SPC ( %seqFromTotal-1 );
      GameObjectEdSeqSlider.setValue( %val * getWord( GameObjectEdSeqSlider.range, 1 ) );
      GameObjectEdThreadSlider.range = GameObjectEdSeqSlider.range;
      GameObjectEdThreadSlider.setValue( GameObjectEdSeqSlider.value );

      GameObjectEdAnimWindow.setSequence( %seqName );
      GameObjectEdAnimWindow.setPlaybackLimit( "in", %seqStart );
      GameObjectEdAnimWindow.setPlaybackLimit( "out", %seqEnd );
   }
   else
   {
      // Hide sequence in/out bars
      GameObjectEdAnimWindow-->seqInBar.setVisible( false );
      GameObjectEdAnimWindow-->seqOutBar.setVisible( false );

      GameObjectEdSeqFromMenu.setText( "" );
      GameObjectEdSeqFromMenu.tooltip = "";
      GameObjectEdSequences-->startFrame.setText( 0 );
      GameObjectEdSequences-->endFrame.setText( 0 );

      GameObjectEdSeqSlider.range = "0 1";
      GameObjectEdSeqSlider.setValue( 0 );
      GameObjectEdThreadSlider.range = GameObjectEdSeqSlider.range;
      GameObjectEdThreadSlider.setValue( GameObjectEdSeqSlider.value );
      GameObjectEdAnimWindow.setPlaybackLimit( "in", 0 );
      GameObjectEdAnimWindow.setPlaybackLimit( "out", 1 );
      GameObjectEdAnimWindow.setSequence( "" );
   }
}

function GameObjectEdSequences::onEditSeqInOut( %this, %type, %val )
{
   %frameCount = getWord( GameObjectEdSeqSlider.range, 1 );

   // Force value to a frame index within the slider range
   %val = mRound( %val );
   if ( %val < 0 )
      %val = 0;
   if ( %val > %frameCount )
      %val = %frameCount;

   // Enforce 'in' value must be < 'out' value
   if ( %type $= "in" )
   {
      if ( %val >= %this-->endFrame.getText() )
         %val = %this-->endFrame.getText() - 1;
      %this-->startFrame.setText( %val );
   }
   else
   {
      if ( %val <= %this-->startFrame.getText() )
         %val = %this-->startFrame.getText() + 1;
      %this-->endFrame.setText( %val );
   }

   %this.onEditSequenceSource( "" );
}

function GameObjectEdSequences::onEditSequenceSource( %this, %from )
{
   // ignore for shapes without sequences
   if (GameObjectEditor.shape.getSequenceCount() == 0)
      return;

   %start = %this-->startFrame.getText();
   %end = %this-->endFrame.getText();

   if ( ( %start !$= "" ) && ( %end !$= "" ) )
   {
      %seqName = GameObjectEdSequenceList.getSelectedName();
      %oldSource = GameObjectEditor.getSequenceSource( %seqName );

      if ( %from $= "" )
         %from = rtrim( getFields( %oldSource, 0, 0 ) );

      if ( getFields( %oldSource, 0, 3 ) !$= ( %from TAB "" TAB %start TAB %end ) )
         GameObjectEditor.doEditSeqSource( %seqName, %from, %start, %end );
   }
}

function GameObjectEdSequences::onToggleCyclic( %this )
{
   %seqName = GameObjectEdSequenceList.getSelectedName();
   if ( %seqName !$= "" )
   {
      %cyclic = %this-->cyclicFlag.getValue();
      GameObjectEditor.doEditCyclic( %seqName, %cyclic );
   }
}

function GameObjectEdSequences::onEditPriority( %this )
{
   %seqName = GameObjectEdSequenceList.getSelectedName();
   if ( %seqName !$= "" )
   {
      %newPriority = %this-->priority.getText();
      if ( %newPriority !$= "" )
         GameObjectEditor.doEditSequencePriority( %seqName, %newPriority );
   }
}

function GameObjectEdSequences::onEditBlend( %this )
{
   %seqName = GameObjectEdSequenceList.getSelectedName();
   if ( %seqName !$= "" )
   {
      // Get the blend flags (current and new)
      %oldBlendData = GameObjectEditor.shape.getSequenceBlend( %seqName );
      %oldBlend = getField( %oldBlendData, 0 );
      %blend = %this-->blendFlag.getValue();

      // Ignore changes to the blend reference for non-blend sequences
      if ( !%oldBlend && !%blend )
         return;

      // OK - we're trying to change the blend properties of this sequence. The
      // new reference sequence and frame must be set.
      %blendSeq = %this-->blendSeq.getText();
      %blendFrame = %this-->blendFrame.getText();
      if ( ( %blendSeq $= "" ) || ( %blendFrame $= "" ) )
      {
         MessageBoxOK( "Blend reference not set", "The blend reference sequence and " @
            "frame must be set before changing the blend flag or frame." );
         GameObjectEdSequences-->blendFlag.setStateOn( %oldBlend );
         return;
      }

      // Get the current blend properties (use new values if not specified)
      %oldBlendSeq = getField( %oldBlendData, 1 );
      if ( %oldBlendSeq $= "" )
         %oldBlendSeq = %blendSeq;
      %oldBlendFrame = getField( %oldBlendData, 2 );
      if ( %oldBlendFrame $= "" )
         %oldBlendFrame = %blendFrame;

      // Check if there is anything to do
      if ( ( %oldBlend TAB %oldBlendSeq TAB %oldBlendFrame ) !$= ( %blend TAB %blendSeq TAB %blendFrame ) )
         GameObjectEditor.doEditBlend( %seqName, %blend, %blendSeq, %blendFrame );
   }
}

function GameObjectEdSequences::onAddSequence( %this, %name )
{
   if ( %name $= "" )
      %name = GameObjectEditor.getUniqueName( "sequence", "mySequence" );

   // Use the currently selected sequence as the base
   %from = GameObjectEdSequenceList.getSelectedName();
   %row = GameObjectEdSequenceList.getSelectedRow();
   if ( ( %row < 2 ) && ( GameObjectEdSequenceList.rowCount() > 2 ) )
      %row = 2;
   if ( %from $= "" )
   {
      // No sequence selected => open dialog to browse for one
      getLoadFormatFilename( %this @ ".onAddSequenceFromBrowse", GameObjectEdFromMenu.lastPath );
      return;
   }
   else
   {
      // Add the new sequence
      %start = GameObjectEdSequences-->startFrame.getText();
      %end = GameObjectEdSequences-->endFrame.getText();
      GameObjectEditor.doAddSequence( %name, %from, %start, %end );
   }
}

function GameObjectEdSequences::onAddSequenceFromBrowse( %this, %path )
{
   // Add a new sequence from the browse path
   %path = makeRelativePath( %path, getMainDotCSDir() );
   GameObjectEdFromMenu.lastPath = %path;

   %name = GameObjectEditor.getUniqueName( "sequence", "mySequence" );
   GameObjectEditor.doAddSequence( %name, %path, 0, -1 );
}

// Delete the selected sequence
function GameObjectEdSequences::onDeleteSequence( %this )
{
   %row = GameObjectEdSequenceList.getSelectedRow();
   if ( %row != -1 )
   {
      %seqName = GameObjectEdSequenceList.getItemName( %row );
      GameObjectEditor.doRemoveShapeData( "Sequence", %seqName );
   }
}

// Get the name of the currently selected sequence
function GameObjectEdSequenceList::getSelectedName( %this )
{
   %row = %this.getSelectedRow();
   return ( %row > 1 ) ? %this.getItemName( %row ) : "";    // ignore header row
}

// Get the sequence name from the indexed row
function GameObjectEdSequenceList::getItemName( %this, %row )
{
   return getField( %this.getRowText( %row ), 0 );
}

// Get the index in the list of the sequence with the given name
function GameObjectEdSequenceList::getItemIndex( %this, %name )
{
   for ( %i = 1; %i < %this.rowCount(); %i++ )  // ignore header row
   {
      if ( %this.getItemName( %i ) $= %name )
         return %i;
   }
   return -1;
}

// Change one of the fields in the sequence list
function GameObjectEdSequenceList::editColumn( %this, %name, %col, %text )
{
   %row = %this.getItemIndex( %name );
   %rowText = setField( %this.getRowText( %row ), %col, %text );

   // Update the Properties and Thread sequence lists
   %id = %this.getRowId( %row );
   if ( %col == 0 )
      GameObjectEdThreadWindow-->seqList.setRowById( %id, %text );   // Sync name in Thread window
   %this.setRowById( %id, %rowText );
}

function GameObjectEdSequenceList::addItem( %this, %name )
{
   return %this.insertItem( %name, %this.rowCount() );
}

function GameObjectEdSequenceList::insertItem( %this, %name, %index )
{
   %cyclic = GameObjectEditor.shape.getSequenceCyclic( %name ) ? "yes" : "no";
   %blend = getField( GameObjectEditor.shape.getSequenceBlend( %name ), 0 ) ? "yes" : "no";
   %frameCount = GameObjectEditor.shape.getSequenceFrameCount( %name );
   %priority = GameObjectEditor.shape.getSequencePriority( %name );

   // Add the item to the Properties and Thread sequence lists
   %this.seqId++; // use this to keep the row IDs synchronised
   GameObjectEdThreadWindow-->seqList.addRow( %this.seqId, %name, %index-1 );   // no header row
   return %this.addRow( %this.seqId, %name TAB %cyclic TAB %blend TAB %frameCount TAB %priority, %index );
}

function GameObjectEdSequenceList::removeItem( %this, %name )
{
   %index = %this.getItemIndex( %name );
   if ( %index >= 0 )
   {
      %this.removeRow( %index );
      GameObjectEdThreadWindow-->seqList.removeRow( %index-1 );   // no header row
   }
}

function GameObjectEdSeqFromMenu::onSelect( %this, %id, %text )
{
   if ( %text $= "Browse..." )
   {
      // Reset menu text
      %seqName = GameObjectEdSequenceList.getSelectedName();
      %seqFrom = rtrim( getFields( GameObjectEditor.getSequenceSource( %seqName ), 0, 1 ) );
      %this.setText( %seqFrom );

      // Allow the user to browse for an external source of animation data
      getLoadFormatFilename( %this @ ".onBrowseSelect", %this.lastPath );
   }
   else
   {
      GameObjectEdSequences.onEditSequenceSource( %text );
   }
}

function GameObjectEdSeqFromMenu::onBrowseSelect( %this, %path )
{
   %path = makeRelativePath( %path, getMainDotCSDir() );
   %this.lastPath = %path;
   %this.setText( %path );
   GameObjectEdSequences.onEditSequenceSource( %path );
}

//------------------------------------------------------------------------------
// Threads and Animation
//------------------------------------------------------------------------------

function GameObjectEdThreadWindow::onWake( %this )
{
   %this-->useTransitions.setValue( 1 );
   %this-->transitionTime.setText( "0.5" );

   %this-->transitionTo.clear();
   %this-->transitionTo.add( "synched position", 0 );
   %this-->transitionTo.add( "slider position", 1 );
   %this-->transitionTo.setSelected( 0 );

   %this-->transitionTarget.clear();
   %this-->transitionTarget.add( "plays during transition", 0 );
   %this-->transitionTarget.add( "pauses during transition", 1 );
   %this-->transitionTarget.setSelected( 0 );
}

// Update the GUI in response to the shape selection changing
function GameObjectEdThreadWindow::update_onShapeSelectionChanged( %this )
{
   GameObjectEdThreadList.clear();
   %this-->seqList.clear();
   %this-->seqList.addRow( 0, "<rootpose>" );
}

function GameObjectEdAnimWIndow::threadPosToKeyframe( %this, %pos )
{
   if ( %this.usingProxySeq )
   {
      %start = getWord( GameObjectEdSeqSlider.range, 0 );
      %end = getWord( GameObjectEdSeqSlider.range, 1 );
   }
   else
   {
      %start = GameObjectEdAnimWindow.seqStartFrame;
      %end = GameObjectEdAnimWindow.seqEndFrame;
   }

   return %start + ( %end - %start ) * %pos;
}

function GameObjectEdAnimWindow::keyframeToThreadPos( %this, %frame )
{
   if ( %this.usingProxySeq )
   {
      %start = getWord( GameObjectEdSeqSlider.range, 0 );
      %end = getWord( GameObjectEdSeqSlider.range, 1 );
   }
   else
   {
      %start = GameObjectEdAnimWindow.seqStartFrame;
      %end = GameObjectEdAnimWindow.seqEndFrame;
   }

   return ( %frame - %start ) / ( %end - %start );
}

function GameObjectEdAnimWindow::setKeyframe( %this, %frame )
{
   GameObjectEdSeqSlider.setValue( %frame );
   if ( GameObjectEdThreadWindow-->transitionTo.getText() $= "synched position" )
      GameObjectEdThreadSlider.setValue( %frame );

   // Update the position of the active thread => if outside the in/out range,
   // need to switch to the proxy sequence
   if ( !%this.usingProxySeq )
   {
      if ( ( %frame < %this.seqStartFrame ) || ( %frame > %this.seqEndFrame) )
      {
         %this.usingProxySeq = true;
         %proxyName = GameObjectEditor.getProxyName( GameObjectEdShapeView.getThreadSequence() );
         GameObjectEdShapeView.setThreadSequence( %proxyName, 0, 0, false );
      }
   }

   GameObjectEdShapeView.threadPos = %this.keyframeToThreadPos( %frame );
}

function GameObjectEdAnimWindow::setNoProxySequence( %this )
{
   // no need to use the proxy sequence during playback
   if ( %this.usingProxySeq )
   {
      %this.usingProxySeq = false;
      %seqName = GameObjectEditor.getUnproxyName( GameObjectEdShapeView.getThreadSequence() );
      GameObjectEdShapeView.setThreadSequence( %seqName, 0, 0, false );
      GameObjectEdShapeView.threadPos = %this.keyframeToThreadPos( GameObjectEdSeqSlider.getValue() );
   }
}

function GameObjectEdAnimWindow::togglePause( %this )
{
   if ( %this-->pauseBtn.getValue() == 0 )
   {
      %this.lastDirBkwd = %this-->playBkwdBtn.getValue();
      %this-->pauseBtn.performClick();
   }
   else
   {
      %this.setNoProxySequence();
      if ( %this.lastDirBkwd )
         %this-->playBkwdBtn.performClick();
      else
         %this-->playFwdBtn.performClick();
   }
}

function GameObjectEdAnimWindow::togglePingPong( %this )
{
   GameObjectEdShapeView.threadPingPong = %this-->pingpong.getValue();
   if ( %this-->playFwdBtn.getValue() )
      %this-->playFwdBtn.performClick();
   else if ( %this-->playBkwdBtn.getValue() )
      %this-->playBkwdBtn.performClick();
}

function GameObjectEdSeqSlider::onMouseDragged( %this )
{
   // Pause the active thread when the slider is dragged
   if ( GameObjectEdAnimWindow-->pauseBtn.getValue() == 0 )
      GameObjectEdAnimWindow-->pauseBtn.performClick();

   GameObjectEdAnimWindow.setKeyframe( %this.getValue() );
}

function GameObjectEdThreadSlider::onMouseDragged( %this )
{
   if ( GameObjectEdThreadWindow-->transitionTo.getText() $= "synched position" )
   {
      // Pause the active thread when the slider is dragged
      if ( GameObjectEdAnimWindow-->pauseBtn.getValue() == 0 )
         GameObjectEdAnimWindow-->pauseBtn.performClick();

      GameObjectEdAnimWindow.setKeyframe( %this.getValue() );
   }
}

function GameObjectEdShapeView::onThreadPosChanged( %this, %pos, %inTransition )
{
   // Update sliders
   %frame = GameObjectEdAnimWindow.threadPosToKeyframe( %pos );
   GameObjectEdSeqSlider.setValue( %frame );

   if ( GameObjectEdThreadWindow-->transitionTo.getText() $= "synched position" )
   {
      GameObjectEdThreadSlider.setValue( %frame );

      // Highlight the slider during transitions
      if ( %inTransition )
         GameObjectEdThreadSlider.profile = GuiGameObjectEdTransitionSliderProfile;
      else
         GameObjectEdThreadSlider.profile = ToolsGuiSliderProfile;
   }
}

// Set the direction of the current thread (-1: reverse, 0: paused, 1: forward)
function GameObjectEdAnimWindow::setThreadDirection( %this, %dir )
{
   // Update thread direction
   GameObjectEdShapeView.threadDirection = %dir;

   // Sync the controls in the thread window
   switch ( %dir )
   {
      case -1: GameObjectEdThreadWindow-->playBkwdBtn.setStateOn( 1 );
      case 0:  GameObjectEdThreadWindow-->pauseBtn.setStateOn( 1 );
      case 1:  GameObjectEdThreadWindow-->playFwdBtn.setStateOn( 1 );
   }
}

// Set the sequence to play
function GameObjectEdAnimWindow::setSequence( %this, %seqName )
{
   %this.usingProxySeq = false;

   if ( GameObjectEdThreadWindow-->useTransitions.getValue() )
   {
      %transTime = GameObjectEdThreadWindow-->transitionTime.getText();
      if ( GameObjectEdThreadWindow-->transitionTo.getText() $= "synched position" )
         %transPos = -1;
      else
         %transPos = %this.keyframeToThreadPos( GameObjectEdThreadSlider.getValue() );
      %transPlay = ( GameObjectEdThreadWindow-->transitionTarget.getText() $= "plays during transition" );
   }
   else
   {
      %transTime = 0;
      %transPos = 0;
      %transPlay = 0;
   }

   // No transition when sequence is not changing
   if ( %seqName $= GameObjectEdShapeView.getThreadSequence() )
      %transTime = 0;

   if ( %seqName !$= "" )
   {
      // To be able to effectively scrub through the animation, we need to have all
      // frames available, even if it was added with only a subset. If that is the
      // case, then create a proxy sequence that has all the frames instead.
      %sourceData = GameObjectEditor.getSequenceSource( %seqName );
      %from = rtrim( getFields( %sourceData, 0, 1 ) );
      %startFrame = getField( %sourceData, 2 );
      %endFrame = getField( %sourceData, 3 );
      %frameCount = getField( %sourceData, 4 );

      if ( ( %startFrame != 0 ) || ( %endFrame != ( %frameCount-1 ) ) )
      {
         %proxyName = GameObjectEditor.getProxyName( %seqName );
         if ( GameObjectEditor.shape.getSequenceIndex( %proxyName ) != -1 )
         {
            GameObjectEditor.shape.removeSequence( %proxyName );
            GameObjectEdShapeView.refreshThreadSequences();
         }
         GameObjectEditor.shape.addSequence( %from, %proxyName );

         // Limit the transition position to the in/out range
         %transPos = mClamp( %transPos, 0, 1 );
      }
   }

   GameObjectEdShapeView.setThreadSequence( %seqName, %transTime, %transPos, %transPlay );
}

function GameObjectEdAnimWindow::getTimelineBitmapPos( %this, %val, %width )
{
   %frameCount = getWord( GameObjectEdSeqSlider.range, 1 );
   %pos_x = getWord( GameObjectEdSeqSlider.getPosition(), 0 );
   %len_x = getWord( GameObjectEdSeqSlider.getExtent(), 0 ) - %width;
   return %pos_x + ( ( %len_x * %val / %frameCount ) );
}

// Set the in or out sequence limit
function GameObjectEdAnimWindow::setPlaybackLimit( %this, %limit, %val )
{
   // Determine where to place the in/out bar on the slider
   %thumbWidth = 8;    // width of the thumb bitmap
   %pos_x = %this.getTimelineBitmapPos( %val, %thumbWidth );

   if ( %limit $= "in" )
   {
      %this.seqStartFrame = %val;
      %this-->seqIn.setText( %val );
      %this-->seqInBar.setPosition( %pos_x, 0 );
   }
   else
   {
      %this.seqEndFrame = %val;
      %this-->seqOut.setText( %val );
      %this-->seqOutBar.setPosition( %pos_x, 0 );
   }
}

function GameObjectEdThreadWindow::onAddThread( %this )
{
   GameObjectEdShapeView.addThread();
   GameObjectEdThreadList.addRow( %this.threadID++, GameObjectEdThreadList.rowCount() );
   GameObjectEdThreadList.setSelectedRow( GameObjectEdThreadList.rowCount()-1 );
}

function GameObjectEdThreadWindow::onRemoveThread( %this )
{
   if ( GameObjectEdThreadList.rowCount() > 1 )
   {
      // Remove the selected thread
      %row = GameObjectEdThreadList.getSelectedRow();
      GameObjectEdShapeView.removeThread( %row );
      GameObjectEdThreadList.removeRow( %row );

      // Update list (threads are always numbered 0-N)
      %rowCount = GameObjectEdThreadList.rowCount();
      for ( %i = %row; %i < %rowCount; %i++ )
         GameObjectEdThreadList.setRowById( GameObjectEdThreadList.getRowId( %i ), %i );

      // Select the next thread
      if ( %row >= %rowCount )
         %row = %rowCount - 1;

      GameObjectEdThreadList.setSelectedRow( %row );
   }
}

function GameObjectEdThreadList::onSelect( %this, %row, %text )
{
   GameObjectEdShapeView.activeThread = GameObjectEdThreadList.getSelectedRow();

   // Select the active thread's sequence in the list
   %seqName = GameObjectEdShapeView.getThreadSequence();
   if ( %seqName $= "" )
      %seqName = "<rootpose>";
   else if ( startswith( %seqName, "__proxy__" ) )
      %seqName = GameObjectEditor.getUnproxyName( %seqName );

   %seqIndex = GameObjectEdSequenceList.getItemIndex( %seqName );
   GameObjectEdSequenceList.setSelectedRow( %seqIndex );

   // Update the playback controls
   switch ( GameObjectEdShapeView.threadDirection )
   {
      case -1: GameObjectEdAnimWindow-->playBkwdBtn.performClick();
      case 0:  GameObjectEdAnimWindow-->pauseBtn.performClick();
      case 1:  GameObjectEdAnimWindow-->playFwdBtn.performClick();
   }
   SetToggleButtonValue( GameObjectEdAnimWindow-->pingpong, GameObjectEdShapeView.threadPingPong );
}

//------------------------------------------------------------------------------
// Trigger Editing
//------------------------------------------------------------------------------

function GameObjectEdPropWindow::onTriggerSelectionChanged( %this )
{
   %row = GameObjectEdTriggerList.getSelectedRow();
   if ( %row > 0 )  // skip header row
   {
      %text = GameObjectEdTriggerList.getRowText( %row );

      GameObjectEdSequences-->triggerFrame.setActive( true );
      GameObjectEdSequences-->triggerNum.setActive( true );
      GameObjectEdSequences-->triggerOnOff.setActive( true );

      GameObjectEdSequences-->triggerFrame.setText( getField( %text, 1 ) );
      GameObjectEdSequences-->triggerNum.setText( getField( %text, 2 ) );
      GameObjectEdSequences-->triggerOnOff.setValue( getField( %text, 3 ) $= "on" );
   }
   else
   {
      // No trigger selected
      GameObjectEdSequences-->triggerFrame.setActive( false );
      GameObjectEdSequences-->triggerNum.setActive( false );
      GameObjectEdSequences-->triggerOnOff.setActive( false );

      GameObjectEdSequences-->triggerFrame.setText( "" );
      GameObjectEdSequences-->triggerNum.setText( "" );
      GameObjectEdSequences-->triggerOnOff.setValue( 0 );
   }
}

function GameObjectEdSequences::onEditName( %this )
{
   %seqName = GameObjectEdSequenceList.getSelectedName();
   if ( %seqName !$= "" )
   {
      %newName = %this-->seqName.getText();
      if ( %newName !$= "" )
         GameObjectEditor.doRenameSequence( %seqName, %newName );
   }
}

function GameObjectEdPropWindow::update_onTriggerAdded( %this, %seqName, %frame, %state )
{
   // --- SEQUENCES TAB ---
   // Add trigger to list if this sequence is selected
   if ( GameObjectEdSequenceList.getSelectedName() $= %seqName )
      GameObjectEdTriggerList.addItem( %frame, %state );
}

function GameObjectEdPropWindow::update_onTriggerRemoved( %this, %seqName, %frame, %state )
{
   // --- SEQUENCES TAB ---
   // Remove trigger from list if this sequence is selected
   if ( GameObjectEdSequenceList.getSelectedName() $= %seqName )
      GameObjectEdTriggerList.removeItem( %frame, %state );
}

function GameObjectEdTriggerList::getTriggerText( %this, %frame, %state )
{
   // First column is invisible and used only for sorting
   %sortKey = ( %frame * 1000 ) + ( mAbs( %state ) * 10 ) + ( ( %state > 0 ) ? 1 : 0 );
   return %sortKey TAB %frame TAB mAbs( %state ) TAB ( ( %state > 0 ) ? "on" : "off" );
}

function GameObjectEdTriggerList::addItem( %this, %frame, %state )
{
   // Add to text list
   %row = %this.addRow( %this.triggerId, %this.getTriggerText( %frame, %state ) );
   %this.sortNumerical( 0, true );

   // Add marker to animation timeline
   %pos = GameObjectEdAnimWindow.getTimelineBitmapPos( GameObjectEdAnimWindow-->seqIn.getText() + %frame, 2 );
   %ctrl = new GuiBitmapCtrl()
   {
      internalName = "trigger" @ %this.triggerId;
      Profile = "ToolsGuiDefaultProfile";
      HorizSizing = "right";
      VertSizing = "bottom";
      position = %pos SPC "0";
      Extent = "2 12";
      bitmap = "tools/GameObjectEditor/images/trigger_marker";
   };
   GameObjectEdAnimWindow.getObject(0).addGuiControl( %ctrl );
   %this.triggerId++;
}

function GameObjectEdTriggerList::removeItem( %this, %frame, %state )
{
   // Remove from text list
   %row = %this.findTextIndex( %this.getTriggerText( %frame, %state ) );
   if ( %row > 0 )
   {
      eval( "GameObjectEdAnimWindow-->trigger" @ %this.getRowId( %row ) @ ".delete();" );
      %this.removeRow( %row );
   }
}

function GameObjectEdTriggerList::removeAll( %this )
{
   %count = %this.rowCount();
   for ( %row = %count-1; %row > 0; %row-- )
   {
      eval( "GameObjectEdAnimWindow-->trigger" @ %this.getRowId( %row ) @ ".delete();" );
      %this.removeRow( %row );
   }
}

function GameObjectEdTriggerList::updateItem( %this, %oldFrame, %oldState, %frame, %state )
{
   // Update text list entry
   %oldText = %this.getTriggerText( %oldFrame, %oldState );
   %row = %this.getSelectedRow();
   if ( ( %row <= 0 ) || ( %this.getRowText( %row ) !$= %oldText ) )
      %row = %this.findTextIndex( %oldText );
   if ( %row > 0 )
   {
      %updatedId = %this.getRowId( %row );
      %newText = %this.getTriggerText( %frame, %state );
      %this.setRowById( %updatedId, %newText );

      // keep selected row the same
      %selectedId = %this.getSelectedId();
      %this.sortNumerical( 0, true );
      %this.setSelectedById( %selectedId );

      // Update animation timeline marker
      if ( %frame != %oldFrame )
      {
         %pos = GameObjectEdAnimWindow.getTimelineBitmapPos( GameObjectEdAnimWindow-->seqIn.getText() + %frame, 2 );
         eval( "%ctrl = GameObjectEdAnimWindow-->trigger" @ %updatedId @ ";" );
         %ctrl.position = %pos SPC "0";
      }
   }
}

function GameObjectEdSequences::onAddTrigger( %this )
{
    // Can only add triggers if a sequence is selected
    %seqName = GameObjectEdSequenceList.getSelectedName();
    if ( %seqName !$= "" )
    {
        // Add a new trigger at the current frame
        %frame = mRound( GameObjectEdSeqSlider.getValue() ) - %this-->startFrame.getText();
        if ((%frame < 0) || (%frame > %this-->endFrame.getText() - %this-->startFrame.getText()))
        {
            MessageBoxOK( "Error", "Trigger out of range of the selected animation." );
        }
        else
        {
        %state = GameObjectEdTriggerList.rowCount() % 30;
        GameObjectEditor.doAddTrigger( %seqName, %frame, %state );
        }
    }
}

function GameObjectEdTriggerList::onDeleteSelection( %this )
{
   // Can only delete a trigger if a sequence and trigger are selected
   %seqName = GameObjectEdSequenceList.getSelectedName();
   if ( %seqName !$= "" )
   {
      %row = %this.getSelectedRow();
      if ( %row > 0 )
      {
         %text = %this.getRowText( %row );
         %frame = getWord( %text, 1 );
         %state = getWord( %text, 2 );
         %state *= ( getWord( %text, 3 ) $= "on" ) ? 1 : -1;
         GameObjectEditor.doRemoveTrigger( %seqName, %frame, %state );
      }
   }
}

function GameObjectEdTriggerList::onEditSelection( %this )
{
   // Can only edit triggers if a sequence and trigger are selected
   %seqName = GameObjectEdSequenceList.getSelectedName();
   if ( %seqName !$= "" )
   {
      %row = GameObjectEdTriggerList.getSelectedRow();
      if ( %row > 0 )
      {
         %text = %this.getRowText( %row );
         %oldFrame = getWord( %text, 1 );
         %oldState = getWord( %text, 2 );
         %oldState *= ( getWord( %text, 3 ) $= "on" ) ? 1 : -1;

         %frame = mRound( GameObjectEdSequences-->triggerFrame.getText() );
         %state = mRound( mAbs( GameObjectEdSequences-->triggerNum.getText() ) );
         %state *= GameObjectEdSequences-->triggerOnOff.getValue() ? 1 : -1;

         if ( ( %frame >= 0 ) && ( %state != 0 ) )
            GameObjectEditor.doEditTrigger( %seqName, %oldFrame, %oldState, %frame, %state );
      }
   }
}

//------------------------------------------------------------------------------
// Material Editing
//------------------------------------------------------------------------------

function GameObjectEdMaterials::updateMaterialList( %this )
{
   // --- MATERIALS TAB ---
   GameObjectEdMaterialList.clear();
   GameObjectEdMaterialList.addRow( -2, "Name" TAB "Mapped" );
   GameObjectEdMaterialList.setRowActive( -2, false );
   GameObjectEdMaterialList.addRow( -1, "<none>" );
   %count = GameObjectEditor.shape.getTargetCount();
   for ( %i = 0; %i < %count; %i++ )
   {
      %matName = GameObjectEditor.shape.getTargetName( %i );
      %mapped = getMaterialMapping( %matName );
      if ( %mapped $= "" )
         GameObjectEdMaterialList.addRow( WarningMaterial.getID(), %matName TAB "unmapped" );
      else
         GameObjectEdMaterialList.addRow( %mapped.getID(), %matName TAB %mapped );
   }

   GameObjectEdMaterials-->materialListHeader.setExtent( getWord( GameObjectEdMaterialList.extent, 0 ) SPC "19" );
}

function GameObjectEdMaterials::updateSelectedMaterial( %this, %highlight )
{
   // Remove the highlight effect from the old selection
   if ( isObject( %this.selectedMaterial ) )
   {
      %this.selectedMaterial.diffuseMap[1] = %this.savedMap;
      %this.selectedMaterial.reload();
   }

   // Apply the highlight effect to the new selected material
   %this.selectedMapTo = getField( GameObjectEdMaterialList.getRowText( GameObjectEdMaterialList.getSelectedRow() ), 0 );
   %this.selectedMaterial = GameObjectEdMaterialList.getSelectedId();
   %this.savedMap = %this.selectedMaterial.diffuseMap[1];
   if ( %highlight && isObject( %this.selectedMaterial ) )
   {
      %this.selectedMaterial.diffuseMap[1] = "tools/GameObjectEditor/images/highlight_material";
      %this.selectedMaterial.reload();
   }
}

function GameObjectEdMaterials::editSelectedMaterial( %this )
{
   if ( isObject( %this.selectedMaterial ) )
   {
      // Remove the highlight effect from the selected material, then switch
      // to the Material Editor
      %this.updateSelectedMaterial( false );

      // Create a temporary TSStatic so the MaterialEditor can query the model's
      // materials.
      pushInstantGroup();
      %this.tempShape = new TSStatic() {
         shapeName = GameObjectEditor.shape.baseShape;
         collisionType = "None";
      };
      popInstantGroup();

      MaterialEditorGui.currentMaterial = %this.selectedMaterial;
      MaterialEditorGui.currentObject = $Tools::materialEditorList = %this.tempShape;

      GameObjectEdSelectWindow.setVisible( false );
      GameObjectEdPropWindow.setVisible( false );
      
      EditorGui-->MatEdPropertiesWindow.setVisible( true );
      EditorGui-->MatEdPreviewWindow.setVisible( true );
      
      MatEd_phoBreadcrumb.setVisible( true );
      MatEd_phoBreadcrumb.command = "GameObjectEdMaterials.editSelectedMaterialEnd();";
      
      advancedTextureMapsRollout.Expanded = false;
      materialAnimationPropertiesRollout.Expanded = false;
      materialAdvancedPropertiesRollout.Expanded = false;
   
      MaterialEditorGui.open();
      MaterialEditorGui.setActiveMaterial( %this.selectedMaterial );

      %id = SubMaterialSelector.findText( %this.selectedMapTo );
      if( %id != -1 )
         SubMaterialSelector.setSelected( %id );
   }
}

function GameObjectEdMaterials::editSelectedMaterialEnd( %this, %closeEditor )
{   
   MatEd_phoBreadcrumb.setVisible( false );
   MatEd_phoBreadcrumb.command = "";
   
   MaterialEditorGui.quit();
   EditorGui-->MatEdPropertiesWindow.setVisible( false );
   EditorGui-->MatEdPreviewWindow.setVisible( false );

   // Delete the temporary TSStatic
   %this.tempShape.delete();

   if( !%closeEditor )
   {
      GameObjectEdSelectWindow.setVisible( true );
      GameObjectEdPropWindow.setVisible( true );
   }
}

//------------------------------------------------------------------------------
// Detail/Mesh Editing
//------------------------------------------------------------------------------

function GameObjectEdDetails::onWake( %this )
{
   // Initialise popup menus
   %this-->bbType.clear();
   %this-->bbType.add( "None", 0 );
   %this-->bbType.add( "Billboard", 1 );
   %this-->bbType.add( "Z Billboard", 2 );

   %this-->addGeomTo.clear();
   %this-->addGeomTo.add( "current detail", 0 );
   %this-->addGeomTo.add( "new detail", 1 );
   %this-->addGeomTo.setSelected( 0, false );

   GameObjectEdDetailTree.onDefineIcons();
}

function GameObjectEdDetailTree::onDefineIcons(%this)
{
   // Set the tree view icon indices and texture paths
   %this._imageNone = 0;
   %this._imageHidden = 1;

   %icons = ":" @                                        // no icon
            "tools/gui/images/visible_i:";               // hidden

   %this.buildIconTable( %icons );
}

// Return true if the item in the details tree view is a detail level (false if
// a mesh)
function GameObjectEdDetailTree::isDetailItem( %this, %id )
{
   return ( %this.getParentItem( %id ) == 1 );
}

// Get the detail level index from the ID of an item in the details tree view
function GameObjectEdDetailTree::getDetailLevelFromItem( %this, %id )
{
   if ( %this.isDetailItem( %id ) )
      %detSize = %this.getItemValue( %id );
      
   else
      %detSize = %this.getItemValue( %this.getParentItem( %id ) );
   return GameObjectEditor.shape.getDetailLevelIndex( %detSize );
}

function GameObjectEdDetailTree::addMeshEntry( %this, %name, %noSync )
{
   // Add new detail level if required
   %size = getTrailingNumber( %name );
   %detailID = %this.findItemByValue( %size );
   if ( %detailID <= 0 )
   {
      %dl = GameObjectEditor.shape.getDetailLevelIndex( %size );
      %detName = GameObjectEditor.shape.getDetailLevelName( %dl );
      %detailID = GameObjectEdDetailTree.insertItem( 1, %detName, %size, "" );

      // Sort details by decreasing size
      for ( %sibling = GameObjectEdDetailTree.getPrevSibling( %detailID );
            ( %sibling > 0 ) && ( GameObjectEdDetailTree.getItemValue( %sibling ) < %size );
            %sibling = GameObjectEdDetailTree.getPrevSibling( %detailID ) )
         GameObjectEdDetailTree.moveItemUp( %detailID );

      if ( !%noSync )
         GameObjectEdDetails.update_onDetailsChanged();
   }
   return %this.insertItem( %detailID, %name, "", "" );
}

function GameObjectEdDetailTree::removeMeshEntry( %this, %name, %size )
{
   %size = getTrailingNumber( %name );
   %id = GameObjectEdDetailTree.findItemByName( %name );
   if ( GameObjectEditor.shape.getDetailLevelIndex( %size ) < 0 )
   {
      // Last mesh of a detail level has been removed => remove the detail level
      %this.removeItem( %this.getParentItem( %id ) );
      GameObjectEdDetails.update_onDetailsChanged();
   }
   else
      %this.removeItem( %id );
}

function GameObjectEdAdvancedWindow::update_onShapeSelectionChanged( %this )
{
   GameObjectEdShapeView.currentDL = 0;
   GameObjectEdShapeView.onDetailChanged();
}

function GameObjectEdPropWindow::update_onDetailRenamed( %this, %oldName, %newName )
{
   // --- DETAILS TAB ---
   // Rename detail entry
   %id = GameObjectEdDetailTree.findItemByName( %oldName );
   if ( %id > 0 )
   {
      %size = GameObjectEdDetailTree.getItemValue( %id );
      GameObjectEdDetailTree.editItem( %id, %newName, %size );

      // Sync text if item is selected
      if ( GameObjectEdDetailTree.isItemSelected( %id ) &&
           ( GameObjectEdDetails-->meshName.getText() !$= %newName ) )
         GameObjectEdDetails-->meshName.setText( stripTrailingNumber( %newName ) );
   }
}

function GameObjectEdPropWindow::update_onDetailSizeChanged( %this, %oldSize, %newSize )
{
   // --- MISC ---
   GameObjectEdShapeView.refreshShape();
   %dl = GameObjectEditor.shape.getDetailLevelIndex( %newSize );
   if ( GameObjectEdAdvancedWindow-->detailSize.getText() $= %oldSize )
   {
      GameObjectEdShapeView.currentDL = %dl;
      GameObjectEdAdvancedWindow-->detailSize.setText( %newSize );
      GameObjectEdDetails-->meshSize.setText( %newSize );
   }

   // --- DETAILS TAB ---
   // Update detail entry then resort details by size
   %id = GameObjectEdDetailTree.findItemByValue( %oldSize );
   %detName = GameObjectEditor.shape.getDetailLevelName( %dl );
   GameObjectEdDetailTree.editItem( %id, %detName, %newSize );

   for ( %sibling = GameObjectEdDetailTree.getPrevSibling( %id );
         ( %sibling > 0 ) && ( GameObjectEdDetailTree.getItemValue( %sibling ) < %newSize );
         %sibling = GameObjectEdDetailTree.getPrevSibling( %id ) )
      GameObjectEdDetailTree.moveItemUp( %id );
   for ( %sibling = GameObjectEdDetailTree.getNextSibling( %id );
         ( %sibling > 0 ) && ( GameObjectEdDetailTree.getItemValue( %sibling ) > %newSize );
         %sibling = GameObjectEdDetailTree.getNextSibling( %id ) )
      GameObjectEdDetailTree.moveItemDown( %id );

   // Update size values for meshes of this detail
   for ( %child = GameObjectEdDetailTree.getChild( %id );
         %child > 0;
         %child = GameObjectEdDetailTree.getNextSibling( %child ) )
   {
      %meshName = stripTrailingNumber( GameObjectEdDetailTree.getItemText( %child ) );
      GameObjectEdDetailTree.editItem( %child, %meshName SPC %newSize, "" );
   }
}

function GameObjectEdDetails::update_onDetailsChanged( %this )
{
   %detailCount = GameObjectEditor.shape.getDetailLevelCount();
   GameObjectEdAdvancedWindow-->detailSlider.range = "0" SPC ( %detailCount-1 );
   if ( %detailCount >= 2 )
      GameObjectEdAdvancedWindow-->detailSlider.ticks = %detailCount - 2;
   else
      GameObjectEdAdvancedWindow-->detailSlider.ticks = 0;

   // Initialise imposter settings
   GameObjectEdAdvancedWindow-->bbUseImposters.setValue( GameObjectEditor.shape.getImposterDetailLevel() != -1 );

   // Update detail parameters
   if ( GameObjectEdShapeView.currentDL < %detailCount )
   {
      %settings = GameObjectEditor.shape.getImposterSettings( GameObjectEdShapeView.currentDL );
      %isImposter = getWord( %settings, 0 );

      GameObjectEdAdvancedWindow-->imposterInactive.setVisible( !%isImposter );

      GameObjectEdAdvancedWindow-->bbEquatorSteps.setText( getField( %settings, 1 ) );
      GameObjectEdAdvancedWindow-->bbPolarSteps.setText( getField( %settings, 2 ) );
      GameObjectEdAdvancedWindow-->bbDetailLevel.setText( getField( %settings, 3 ) );
      GameObjectEdAdvancedWindow-->bbDimension.setText( getField( %settings, 4 ) );
      GameObjectEdAdvancedWindow-->bbIncludePoles.setValue( getField( %settings, 5 ) );
      GameObjectEdAdvancedWindow-->bbPolarAngle.setText( getField( %settings, 6 ) );
   }
}

function GameObjectEdPropWindow::update_onObjectNodeChanged( %this, %objName )
{
   // --- MISC ---
   GameObjectEdShapeView.refreshShape();

   // --- DETAILS TAB ---
   // Update the node popup menu if this object is selected
   if ( GameObjectEdDetails-->meshName.getText() $= %objName )
   {
      %nodeName = GameObjectEditor.shape.getObjectNode( %objName );
      if ( %nodeName $= "" )
         %nodeName = "<root>";
      %id = GameObjectEdDetails-->objectNode.findText( %nodeName );
      GameObjectEdDetails-->objectNode.setSelected( %id, false );
   }
}

function GameObjectEdPropWindow::update_onObjectRenamed( %this, %oldName, %newName )
{
   // --- DETAILS TAB ---
   // Rename tree entries for this object
   %count = GameObjectEditor.shape.getMeshCount( %newName );
   for ( %i = 0; %i < %count; %i++ )
   {
      %size = getTrailingNumber( GameObjectEditor.shape.getMeshName( %newName, %i ) );
      %id = GameObjectEdDetailTree.findItemByName( %oldName SPC %size );
      if ( %id > 0 )
      {
         GameObjectEdDetailTree.editItem( %id, %newName SPC %size, "" );

         // Sync text if item is selected
         if ( GameObjectEdDetailTree.isItemSelected( %id ) &&
              ( GameObjectEdDetails-->meshName.getText() !$= %newName ) )
            GameObjectEdDetails-->meshName.setText( %newName );
      }
   }
}

function GameObjectEdPropWindow::update_onMeshAdded( %this, %meshName )
{
   // --- MISC ---
   GameObjectEdShapeView.refreshShape();
   GameObjectEdShapeView.updateNodeTransforms();

   // --- COLLISION WINDOW ---
   // Add object to target list if it does not already exist
   if ( !GameObjectEditor.isCollisionMesh( %meshName ) )
   {
      %objName = stripTrailingNumber( %meshName );
      %id = GameObjectEdColWindow-->colTarget.findText( %objName );
      if ( %id == -1 )
         GameObjectEdColWindow-->colTarget.add( %objName );
   }

   // --- DETAILS TAB ---
   %id = GameObjectEdDetailTree.addMeshEntry( %meshName );
   GameObjectEdDetailTree.clearSelection();
   GameObjectEdDetailTree.selectItem( %id );
}

function GameObjectEdPropWindow::update_onMeshSizeChanged( %this, %meshName, %oldSize, %newSize )
{
   // --- MISC ---
   GameObjectEdShapeView.refreshShape();

   // --- DETAILS TAB ---
   // Move the mesh to the new location in the tree
   %selected = GameObjectEdDetailTree.getSelectedItem();
   %id = GameObjectEdDetailTree.findItemByName( %meshName SPC %oldSize );
   GameObjectEdDetailTree.removeMeshEntry( %meshName SPC %oldSize );
   %newId = GameObjectEdDetailTree.addMeshEntry( %meshName SPC %newSize );

   // Re-select the new entry if it was selected
   if ( %selected == %id )
   {
      GameObjectEdDetailTree.clearSelection();
      GameObjectEdDetailTree.selectItem( %newId );
   }
}

function GameObjectEdPropWindow::update_onMeshRemoved( %this, %meshName )
{
   // --- MISC ---
   GameObjectEdShapeView.refreshShape();

   // --- COLLISION WINDOW ---
   // Remove object from target list if it no longer exists
   %objName = stripTrailingNumber( %meshName );
   if ( GameObjectEditor.shape.getObjectIndex( %objName ) == -1 )
   {
      %id = GameObjectEdColWindow-->colTarget.findText( %objName );
      if ( %id != -1 )
         GameObjectEdColWindow-->colTarget.clearEntry( %id );
   }

   // --- DETAILS TAB ---
   // Determine which item to select next
   %id = GameObjectEdDetailTree.findItemByName( %meshName );
   if ( %id > 0 )
   {
      %nextId = GameObjectEdDetailTree.getPrevSibling( %id );
      if ( %nextId <= 0 )
      {
         %nextId = GameObjectEdDetailTree.getNextSibling( %id );
         if ( %nextId <= 0 )
            %nextId = 2;
      }

      // Remove the entry from the tree
      %meshSize = getTrailingNumber( %meshName );
      GameObjectEdDetailTree.removeMeshEntry( %meshName, %meshSize );

      // Change selection if needed
      if ( GameObjectEdDetailTree.getSelectedItem() == -1 )
         GameObjectEdDetailTree.selectItem( %nextId );
   }
}

function GameObjectEdDetailTree::onSelect( %this, %id )
{
   %name = %this.getItemText( %id );
   %baseName = stripTrailingNumber( %name );
   %size = getTrailingNumber( %name );

   GameObjectEdDetails-->meshName.setText( %baseName );
   GameObjectEdDetails-->meshSize.setText( %size );

   // Select the appropriate detail level
   %dl = %this.getDetailLevelFromItem( %id );
   GameObjectEdShapeView.currentDL = %dl;

   if ( %this.isDetailItem( %id ) )
   {
      // Selected a detail => disable mesh controls
      GameObjectEdDetails-->editMeshInactive.setVisible( true );
      GameObjectEdShapeView.selectedObject = -1;
      GameObjectEdShapeView.selectedObjDetail = 0;
   }
   else
   {
      // Selected a mesh => sync mesh controls
      GameObjectEdDetails-->editMeshInactive.setVisible( false );

      switch$ ( GameObjectEditor.shape.getMeshType( %name ) )
      {
         case "normal":          GameObjectEdDetails-->bbType.setSelected( 0, false );
         case "billboard":       GameObjectEdDetails-->bbType.setSelected( 1, false );
         case "billboardzaxis":  GameObjectEdDetails-->bbType.setSelected( 2, false );
      }

      %node = GameObjectEditor.shape.getObjectNode( %baseName );
      if ( %node $= "" )
         %node = "<root>";
      GameObjectEdDetails-->objectNode.setSelected( GameObjectEdDetails-->objectNode.findText( %node ), false );
      GameObjectEdShapeView.selectedObject = GameObjectEditor.shape.getObjectIndex( %baseName );
      GameObjectEdShapeView.selectedObjDetail = %dl;
   }
}

function GameObjectEdDetailTree::onRightMouseUp( %this, %itemId, %mouse )
{
   // Open context menu if this is a Mesh item
   if ( !%this.isDetailItem( %itemId ) )
   {
      if( !isObject( "GameObjectEdMeshPopup" ) )
      {
         new PopupMenu( GameObjectEdMeshPopup )
         {
            superClass = "MenuBuilder";
            isPopup = "1";

            item[ 0 ] = "Hidden" TAB "" TAB "GameObjectEdDetailTree.onHideMeshItem( %this._objName, !%this._itemHidden );";
            item[ 1 ] = "-";
            item[ 2 ] = "Hide all" TAB "" TAB "GameObjectEdDetailTree.onHideMeshItem( \"\", true );";
            item[ 3 ] = "Show all" TAB "" TAB "GameObjectEdDetailTree.onHideMeshItem( \"\", false );";
         };
      }

      GameObjectEdMeshPopup._objName = stripTrailingNumber( %this.getItemText( %itemId ) );
      GameObjectEdMeshPopup._itemHidden = GameObjectEdShapeView.getMeshHidden( GameObjectEdMeshPopup._objName );

      GameObjectEdMeshPopup.checkItem( 0, GameObjectEdMeshPopup._itemHidden );
      GameObjectEdMeshPopup.showPopup( Canvas );
   }
}

function GameObjectEdDetailTree::onHideMeshItem( %this, %objName, %hide )
{
   if ( %hide )
      %imageId = %this._imageHidden;
   else
      %imageId = %this._imageNone;

   if ( %objName $= "" )
   {
      // Show/hide all
      GameObjectEdShapeView.setAllMeshesHidden( %hide );
      for ( %parent = %this.getChild(%this.getFirstRootItem()); %parent > 0; %parent = %this.getNextSibling(%parent) )
         for ( %child = %this.getChild(%parent); %child > 0; %child = %this.getNextSibling(%child) )
            %this.setItemImages( %child, %imageId, %imageId );
   }
   else
   {
      // Show/hide all meshes for this object
      GameObjectEdShapeView.setMeshHidden( %objName, %hide );
      %count = GameObjectEditor.shape.getMeshCount( %objName );
      for ( %i = 0; %i < %count; %i++ )
      {
         %meshName = GameObjectEditor.shape.getMeshName( %objName, %i );
         %id = GameObjectEdDetailTree.findItemByName( %meshName );
         if ( %id > 0 )
            %this.setItemImages( %id, %imageId, %imageId );
      }
   }
}

function GameObjectEdShapeView::onDetailChanged( %this )
{
   // Update slider
   if ( mRound( GameObjectEdAdvancedWindow-->detailSlider.getValue() ) != %this.currentDL )
      GameObjectEdAdvancedWindow-->detailSlider.setValue( %this.currentDL );
   GameObjectEdAdvancedWindow-->detailSize.setText( %this.detailSize );

   GameObjectEdDetails.update_onDetailsChanged();

   %id = GameObjectEdDetailTree.getSelectedItem();
   if ( ( %id <= 0 ) || ( %this.currentDL != GameObjectEdDetailTree.getDetailLevelFromItem( %id ) ) )
   {
      %id = GameObjectEdDetailTree.findItemByValue( %this.detailSize );
      if ( %id > 0 )
      {
         GameObjectEdDetailTree.clearSelection();
         GameObjectEdDetailTree.selectItem( %id );
      }
   }
}

function GameObjectEdAdvancedWindow::onEditDetailSize( %this )
{
   // Change the size of the current detail level
   %oldSize = GameObjectEditor.shape.getDetailLevelSize( GameObjectEdShapeView.currentDL );
   %detailSize = %this-->detailSize.getText();
   GameObjectEditor.doEditDetailSize( %oldSize, %detailSize );
}

function GameObjectEdDetails::onEditName( %this )
{
   %newName = %this-->meshName.getText();

   // Check if we are renaming a detail or a mesh
   %id = GameObjectEdDetailTree.getSelectedItem();
   %oldName = GameObjectEdDetailTree.getItemText( %id );

   if ( GameObjectEdDetailTree.isDetailItem( %id ) )
   {
      // Rename the selected detail level
      %oldSize = getTrailingNumber( %oldName );
      GameObjectEditor.doRenameDetail( %oldName, %newName @ %oldSize );
   }
   else
   {
      // Rename the selected mesh
      GameObjectEditor.doRenameObject( stripTrailingNumber( %oldName ), %newName );
   }
}

function GameObjectEdDetails::onEditSize( %this )
{
   %newSize = %this-->meshSize.getText();

   // Check if we are changing the size for a detail or a mesh
   %id = GameObjectEdDetailTree.getSelectedItem();
   if ( GameObjectEdDetailTree.isDetailItem( %id ) )
   {
      // Change the size of the selected detail level
      %oldSize = GameObjectEdDetailTree.getItemValue( %id );
      GameObjectEditor.doEditDetailSize( %oldSize, %newSize );
   }
   else
   {
      // Change the size of the selected mesh
      %meshName = GameObjectEdDetailTree.getItemText( %id );
      GameObjectEditor.doEditMeshSize( %meshName, %newSize );
   }
}

function GameObjectEdDetails::onEditBBType( %this )
{
   // This command is only valid for meshes (not details)
   %id = GameObjectEdDetailTree.getSelectedItem();
   if ( !GameObjectEdDetailTree.isDetailItem( %id ) )
   {
      %meshName = GameObjectEdDetailTree.getItemText( %id );
      %bbType = GameObjectEdDetails-->bbType.getText();
      switch$ ( %bbType )
      {
         case "None":         %bbType = "normal";
         case "Billboard":    %bbType = "billboard";
         case "Z Billboard":  %bbType = "billboardzaxis";
      }
      GameObjectEditor.doEditMeshBillboard( %meshName, %bbType );
   }
}

function GameObjectEdDetails::onSetObjectNode( %this )
{
   // This command is only valid for meshes (not details)
   %id = GameObjectEdDetailTree.getSelectedItem();
   if ( !GameObjectEdDetailTree.isDetailItem( %id ) )
   {
      %meshName = GameObjectEdDetailTree.getItemText( %id );
      %objName = stripTrailingNumber( %meshName );
      %node = %this-->objectNode.getText();
      if ( %node $= "<root>" )
         %node = "";
      GameObjectEditor.doSetObjectNode( %objName, %node );
   }
}

function GameObjectEdDetails::onAddMeshFromFile( %this, %path )
{
   if ( %path $= "" )
   {
      getLoadFormatFilename( %this @ ".onAddMeshFromFile", %this.lastPath );
      return;
   }

   %path = makeRelativePath( %path, getMainDotCSDir() );
   %this.lastPath = %path;

   // Determine the detail level to use for the new geometry
   if ( %this-->addGeomTo.getText() $= "current detail" )
   {
      %size = GameObjectEditor.shape.getDetailLevelSize( GameObjectEdShapeView.currentDL );
   }
   else
   {
      // Check if the file has an LODXXX hint at the end of it
      %base = fileBase( %path );
      %pos = strstr( %base, "_LOD" );
      if ( %pos > 0 )
         %size = getSubStr( %base, %pos + 4, strlen( %base ) ) + 0;
      else
         %size = 2;

      // Make sure size is not in use
      while ( GameObjectEditor.shape.getDetailLevelIndex( %size ) != -1 )
         %size++;
   }

   GameObjectEditor.doAddMeshFromFile( %path, %size );
}

function GameObjectEdDetails::onDeleteMesh( %this )
{
   %id = GameObjectEdDetailTree.getSelectedItem();
   if ( GameObjectEdDetailTree.isDetailItem( %id ) )
   {
      %detSize = GameObjectEdDetailTree.getItemValue( %id );
      GameObjectEditor.doRemoveShapeData( "Detail", %detSize );
   }
   else
   {
      %name = GameObjectEdDetailTree.getItemText( %id );
      GameObjectEditor.doRemoveShapeData( "Mesh", %name );
   }
}

function GameObjectEdDetails::onToggleImposter( %this, %useImposter )
{
   %hasImposterDetail = ( GameObjectEditor.shape.getImposterDetailLevel() != -1 );
   if ( %useImposter == %hasImposterDetail )
      return;

   if ( %useImposter )
   {
      // Determine an unused detail size
      for ( %detailSize = 0; %detailSize < 50; %detailSize++ )
      {
         if ( GameObjectEditor.shape.getDetailLevelIndex( %detailSize ) == -1 )
            break;
      }

      // Set some initial values for the imposter
      %bbEquatorSteps = 6;
      %bbPolarSteps = 0;
      %bbDetailLevel = 0;
      %bbDimension = 128;
      %bbIncludePoles = 0;
      %bbPolarAngle = 0;

      // Add a new imposter detail level to the shape
      GameObjectEditor.doEditImposter( -1, %detailSize, %bbEquatorSteps, %bbPolarSteps,
         %bbDetailLevel, %bbDimension, %bbIncludePoles, %bbPolarAngle );
   }
   else
   {
      // Remove the imposter detail level
      GameObjectEditor.doRemoveImposter();
   }
}

function GameObjectEdDetails::onEditImposter( %this )
{
   // Modify the parameters of the current imposter detail level
   %detailSize = GameObjectEditor.shape.getDetailLevelSize( GameObjectEdShapeView.currentDL );
   %bbDimension = GameObjectEdAdvancedWindow-->bbDimension.getText();
   %bbDetailLevel = GameObjectEdAdvancedWindow-->bbDetailLevel.getText();
   %bbEquatorSteps = GameObjectEdAdvancedWindow-->bbEquatorSteps.getText();
   %bbIncludePoles = GameObjectEdAdvancedWindow-->bbIncludePoles.getValue();
   %bbPolarSteps = GameObjectEdAdvancedWindow-->bbPolarSteps.getText();
   %bbPolarAngle = GameObjectEdAdvancedWindow-->bbPolarAngle.getText();

   GameObjectEditor.doEditImposter( GameObjectEdShapeView.currentDL, %detailSize,
      %bbEquatorSteps, %bbPolarSteps, %bbDetailLevel, %bbDimension,
      %bbIncludePoles, %bbPolarAngle );
}


function GameObjectEditor::autoAddDetails( %this, %dest )
{
   // Sets of LOD files are named like:
   //
   // MyShape_LOD200.dae
   // MyShape_LOD64.dae
   // MyShape_LOD2.dae
   //
   // Determine the base name of the input file (MyShape_LOD in the example above)
   // and use that to find any other shapes in the set.
   %base = fileBase( %dest.baseShape );
   %pos = strstr( %base, "_LOD" );
   if ( %pos < 0 )
   {
      echo( "Not an LOD shape file" );
      return;
   }

   %base = getSubStr( %base, 0, %pos + 4 );

   echo( "Base is: " @ %base );

   %filePatterns = filePath( %dest.baseShape ) @ "/" @ %base @ "*" @ fileExt( %dest.baseShape );

   echo( "Pattern is: " @ %filePatterns );

   %fullPath = findFirstFileMultiExpr( %filePatterns );
   while ( %fullPath !$= "" )
   {
      %fullPath = makeRelativePath( %fullPath, getMainDotCSDir() );

      if ( %fullPath !$= %dest.baseShape )
      {
         echo( "Found LOD shape file: " @ %fullPath );

         // Determine the detail size ( number after the base name ), then add the
         // new mesh
         %size = strreplace( fileBase( %fullPath ), %base, "" );
         GameObjectEditor.addLODFromFile( %dest, %fullPath, %size, 0 );
      }

      %fullPath = findNextFileMultiExpr( %filePatterns );
   }

   if ( %this.shape == %dest )
   {
      GameObjectEdShapeView.refreshShape();
      GameObjectEdDetails.update_onDetailsChanged();
   }
}

function GameObjectEditor::addLODFromFile( %this, %dest, %filename, %size, %allowUnmatched )
{
   // Get (or create) a TSShapeConstructor object for the source shape. Need to
   // exec the script manually as the resource may not have been loaded yet
   %csPath = filePath( %filename ) @ "/" @ fileBase( %filename ) @ ".cs";
   if ( isFile( %csPath ) )
      exec( %csPath );

   %source = GameObjectEditor.findConstructor( %filename );
   if ( %source == -1 )
      %source = GameObjectEditor.createConstructor( %filename );
   %source.lodType = "SingleSize";
   %source.singleDetailSize = %size;

   // Create a temporary TSStatic to ensure the resource is loaded
   %temp = new TSStatic() {
      shapeName = %filename;
      collisionType = "None";
   };

   %meshList = "";
   if ( isObject( %temp ) )
   {
      // Add a new mesh for each object in the source shape
      %objCount = %source.getObjectCount();
      for ( %i = 0; %i < %objCount; %i++ )
      {
         %objName = %source.getObjectName( %i );

         echo( "Checking for object " @ %objName );

         if ( %allowUnmatched || ( %dest.getObjectIndex( %objName ) != -1 ) )
         {
            // Add the source object's highest LOD mesh to the destination shape
            echo( "Adding detail size" SPC %size SPC "for object" SPC %objName );
            %srcName = %source.getMeshName( %objName, 0 );
            %destName = %objName SPC %size;
            %dest.addMesh( %destName, %filename, %srcName );
            %meshList = %meshList TAB %destName;
         }
      }

      %temp.delete();
   }

   return trim( %meshList );
}

//------------------------------------------------------------------------------
// Collision editing
//------------------------------------------------------------------------------

function GameObjectEdColWindow::onWake( %this )
{
   %this-->colType.clear();
   %this-->colType.add( "Box" );
   %this-->colType.add( "Sphere" );
   %this-->colType.add( "Capsule" );
   %this-->colType.add( "10-DOP X" );
   %this-->colType.add( "10-DOP Y" );
   %this-->colType.add( "10-DOP Z" );
   %this-->colType.add( "18-DOP" );
   %this-->colType.add( "26-DOP" );
   %this-->colType.add( "Convex Hulls" );
}

function GameObjectEdColWindow::update_onShapeSelectionChanged( %this )
{
   %this.lastColSettings = "" TAB "Bounds";

   // Initialise collision mesh target list
   %this-->colTarget.clear();
   %this-->colTarget.add( "Bounds" );
   %objCount = GameObjectEditor.shape.getObjectCount();
   for ( %i = 0; %i < %objCount; %i++ )
      %this-->colTarget.add( GameObjectEditor.shape.getObjectName( %i ) );

   %this-->colTarget.setSelected( %this-->colTarget.findText( "Bounds" ), false );
}

function GameObjectEdColWindow::update_onCollisionChanged( %this )
{
   // Sync collision settings
   %colData = %this.lastColSettings;

   %typeId = %this-->colType.findText( getField( %colData, 0 ) );
   %this-->colType.setSelected( %typeId, false );

   %targetId = %this-->colTarget.findText( getField( %colData, 1 ) );
   %this-->colTarget.setSelected( %targetId, false );

   if ( %this-->colType.getText() $= "Convex Hulls" )
   {
      %this-->hullInactive.setVisible( false );
      %this-->hullDepth.setValue( getField( %colData, 2 ) );
      %this-->hullDepthText.setText( mFloor( %this-->hullDepth.getValue() ) );
      %this-->hullMergeThreshold.setValue( getField( %colData, 3 ) );
      %this-->hullMergeText.setText( mFloor( %this-->hullMergeThreshold.getValue() ) );
      %this-->hullConcaveThreshold.setValue( getField( %colData, 4 ) );
      %this-->hullConcaveText.setText( mFloor( %this-->hullConcaveThreshold.getValue() ) );
      %this-->hullMaxVerts.setValue( getField( %colData, 5 ) );
      %this-->hullMaxVertsText.setText( mFloor( %this-->hullMaxVerts.getValue() ) );
      %this-->hullMaxBoxError.setValue( getField( %colData, 6 ) );
      %this-->hullMaxBoxErrorText.setText( mFloor( %this-->hullMaxBoxError.getValue() ) );
      %this-->hullMaxSphereError.setValue( getField( %colData, 7 ) );
      %this-->hullMaxSphereErrorText.setText( mFloor( %this-->hullMaxSphereError.getValue() ) );
      %this-->hullMaxCapsuleError.setValue( getField( %colData, 8 ) );
      %this-->hullMaxCapsuleErrorText.setText( mFloor( %this-->hullMaxCapsuleError.getValue() ) );
   }
   else
   {
      %this-->hullInactive.setVisible( true );
   }
}

function GameObjectEdColWindow::editCollision( %this )
{
   // If the shape already contains a collision detail size-1, warn the user
   // that it will be removed
   if ( ( GameObjectEditor.shape.getDetailLevelIndex( -1 ) >= 0 ) &&
        ( getField(%this.lastColSettings, 0) $= "" ) )
   {
      MessageBoxYesNo( "Warning", "Existing collision geometry at detail size " @
         "-1 will be removed, and this cannot be undone. Do you want to continue?",
         "GameObjectEdColWindow.editCollisionOK();", "" );
   }
   else
   {
      %this.editCollisionOK();
   }
}

function GameObjectEdColWindow::editCollisionOK( %this )
{
   %type = %this-->colType.getText();
   %target = %this-->colTarget.getText();
   %depth = %this-->hullDepth.getValue();
   %merge = %this-->hullMergeThreshold.getValue();
   %concavity = %this-->hullConcaveThreshold.getValue();
   %maxVerts = %this-->hullMaxVerts.getValue();
   %maxBox = %this-->hullMaxBoxError.getValue();
   %maxSphere = %this-->hullMaxSphereError.getValue();
   %maxCapsule = %this-->hullMaxCapsuleError.getValue();

   GameObjectEditor.doEditCollision( %type, %target, %depth, %merge, %concavity, %maxVerts,
                                 %maxBox, %maxSphere, %maxCapsule );
}

//------------------------------------------------------------------------------
// Mounted Shapes
//------------------------------------------------------------------------------

function GameObjectEdMountWindow::onWake( %this )
{
   %this-->mountType.clear();
   %this-->mountType.add( "Object", 0 );
   %this-->mountType.add( "Image", 1 );
   %this-->mountType.add( "Wheel", 2 );
   %this-->mountType.setSelected( 1, false );

   %this-->mountSeq.clear();
   %this-->mountSeq.add( "<rootpose>", 0 );
   %this-->mountSeq.setSelected( 0, false );
   %this-->mountPlayBtn.setStateOn( false );

   // Only add the Browse entry the first time so we keep any files the user has
   // set up previously
   if ( GameObjectEdMountShapeMenu.size() == 0 )
   {
      GameObjectEdMountShapeMenu.add( "Browse...", 0 );
      GameObjectEdMountShapeMenu.setSelected( 0, false );
   }
}

function GameObjectEdMountWindow::isMountableNode( %this, %nodeName )
{
   return ( startswith( %nodeName, "mount" ) || startswith( %nodeName, "hub" ) );
}

function GameObjectEdMountWindow::update_onShapeSelectionChanged( %this )
{
   %this.unmountAll();

   // Initialise the dropdown menus
   %this-->mountNode.clear();
   %this-->mountNode.add( "<origin>" );
   %count = GameObjectEditor.shape.getNodeCount();
   for ( %i = 0; %i < %count; %i++ )
   {
      %name = GameObjectEditor.shape.getNodeName( %i );
      if ( %this.isMountableNode( %name ) )
         %this-->mountNode.add( %name );
   }
   %this-->mountNode.sort();
   %this-->mountNode.setFirstSelected();

   %this-->mountSeq.clear();
   %this-->mountSeq.add( "<rootpose>", 0 );
   %this-->mountSeq.setSelected( 0, false );
}

function GameObjectEdMountWindow::update_onMountSelectionChanged( %this )
{
   %row = %this-->mountList.getSelectedRow();
   if ( %row > 0 )
   {
      %text = %this-->mountList.getRowText( %row );
      %shapePath = getField( %text, 0 );

      GameObjectEdMountShapeMenu.setText( %shapePath );
      %this-->mountNode.setText( getField( %text, 2 ) );
      %this-->mountType.setText( getField( %text, 3 ) );

      // Fill in sequence list
      %this-->mountSeq.clear();
      %this-->mountSeq.add( "<rootpose>", 0 );

      %tss = GameObjectEditor.findConstructor( %shapePath );
      if ( !isObject( %tss ) )
         %tss = GameObjectEditor.createConstructor( %shapePath );
      if ( isObject( %tss ) )
      {
         %count = %tss.getSequenceCount();
         for ( %i = 0; %i < %count; %i++ )
            %this-->mountSeq.add( %tss.getSequenceName( %i ) );
      }

      // Select the currently playing sequence
      %slot = %row - 1;
      %seq = GameObjectEdShapeView.getMountThreadSequence( %slot );
      %id = %this-->mountSeq.findText( %seq );
      if ( %id == -1 )
         %id = 0;
      %this-->mountSeq.setSelected( %id, false );

      GameObjectEdMountSeqSlider.setValue( GameObjectEdShapeView.getMountThreadPos( %slot ) );
      %this-->mountPlayBtn.setStateOn( GameObjectEdShapeView.getMountThreadPos( %slot ) != 0 );
   }
}

function GameObjectEdMountWindow::updateSelectedMount( %this )
{
   %row = %this-->mountList.getSelectedRow();
   if ( %row > 0 )
      %this.mountShape( %row-1 );
}

function GameObjectEdMountWindow::setMountThreadSequence( %this )
{
   %row = %this-->mountList.getSelectedRow();
   if ( %row > 0 )
   {
      GameObjectEdShapeView.setMountThreadSequence( %row-1, %this-->mountSeq.getText() );
      GameObjectEdShapeView.setMountThreadDir( %row-1, %this-->mountPlayBtn.getValue() );
   }
}

function GameObjectEdMountSeqSlider::onMouseDragged( %this )
{
   %row = GameObjectEdMountWindow-->mountList.getSelectedRow();
   if ( %row > 0 )
   {
      GameObjectEdShapeView.setMountThreadPos( %row-1, %this.getValue() );

      // Pause the sequence when the slider is dragged
      GameObjectEdShapeView.setMountThreadDir( %row-1, 0 );
      GameObjectEdMountWindow-->mountPlayBtn.setStateOn( false );
   }
}

function GameObjectEdMountWindow::toggleMountThreadPlayback( %this )
{
   %row = %this-->mountList.getSelectedRow();
   if ( %row > 0 )
      GameObjectEdShapeView.setMountThreadDir( %row-1, %this-->mountPlayBtn.getValue() );
}

function GameObjectEdMountShapeMenu::onSelect( %this, %id, %text )
{
   if ( %text $= "Browse..." )
   {
      // Allow the user to browse for an external model file
      getLoadFormatFilename( %this @ ".onBrowseSelect", %this.lastPath );
   }
   else
   {
      // Modify the current mount
      GameObjectEdMountWindow.updateSelectedMount();
   }
}

function GameObjectEdMountShapeMenu::onBrowseSelect( %this, %path )
{
   %path = makeRelativePath( %path, getMainDotCSDir() );
   %this.lastPath = %path;
   %this.setText( %path );

   // Add entry if unique
   if ( %this.findText( %path ) == -1 )
      %this.add( %path );

   GameObjectEdMountWindow.updateSelectedMount();
}

function GameObjectEdMountWindow::mountShape( %this, %slot )
{
   %model = GameObjectEdMountShapeMenu.getText();
   %node = %this-->mountNode.getText();
   %type = %this-->mountType.getText();

   if ( %model $= "Browse..." )
      %model = "core/art/shapes/octahedron.dts";

   if ( GameObjectEdShapeView.mountShape( %model, %node, %type, %slot ) )
   {
      %rowText = %model TAB fileName( %model ) TAB %node TAB %type;
      if ( %slot == -1 )
      {
         %id = %this.mounts++;
         %this-->mountList.addRow( %id, %rowText );
      }
      else
      {
         %id = %this-->mountList.getRowId( %slot+1 );
         %this-->mountList.setRowById( %id, %rowText );
      }

      %this-->mountList.setSelectedById( %id );
   }
   else
   {
      MessageBoxOK( "Error", "Failed to mount \"" @ %model @ "\". Check the console for error messages.", "" );
   }
}

function GameObjectEdMountWindow::unmountShape( %this )
{
   %row = %this-->mountList.getSelectedRow();
   if ( %row > 0 )
   {
      GameObjectEdShapeView.unmountShape( %row-1 );
      %this-->mountList.removeRow( %row );

      // Select the next row (if any)
      %count = %this-->mountList.rowCount();
      if ( %row >= %count )
         %row = %count-1;
      if ( %row > 0 )
         %this-->mountList.setSelectedRow( %row );
   }
}

function GameObjectEdMountWindow::unmountAll( %this )
{
   GameObjectEdShapeView.unmountAll();
   %this-->mountList.clear();
   %this-->mountList.addRow( -1, "FullPath" TAB "Filename" TAB "Node" TAB "Type" );
   %this-->mountList.setRowActive( -1, false );
}

//------------------------------------------------------------------------------
// Shape Preview
//------------------------------------------------------------------------------

function GameObjectEdPreviewGui::updatePreviewBackground( %color )
{
   GameObjectEdPreviewGui-->previewBackground.color = %color;
   GameObjectEditorToolbar-->previewBackgroundPicker.color = %color;
}

function showGameObjectEditorPreview()
{
   %visible = GameObjectEditorToolbar-->showPreview.getValue();
   GameObjectEdPreviewGui.setVisible( %visible );
}
