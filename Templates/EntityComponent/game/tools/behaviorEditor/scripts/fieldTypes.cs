//-----------------------------------------------------------------------------
// Torque Game Builder
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

// Default. Text field.
BehaviorEditor::registerFieldType("default", "createDefaultGui");

function BehaviorFieldStack::createDefaultGui(%this, %behavior, %fieldIndex)
{
   %fieldInfo = %behavior.getComponentField(%fieldIndex);
   %name = getField(%fieldInfo, 0);
   
   %data = %behavior.getFieldValue(%name, 0);
   if(%data $= "")
      %data = getField(%fieldInfo, 2);
      
   %description = %behavior.getBehaviorFieldDescription(%fieldIndex);
   
   %control = %this.createTextEditProperty(%name, "TEXT", %name, %description, %data);
   
   %group = getField(%fieldInfo, 3);
   %this.addControlToStack(%group, %behavior, %control);
      
   %editField = %control.findObjectByInternalName(%name @ "TextEdit");
   %editField.object = %behavior;
}

// Int. Text field validated to a whole number.
BehaviorEditor::registerFieldType("int", "createIntGui");

function BehaviorFieldStack::createIntGui(%this, %behavior, %fieldIndex)
{
   %fieldInfo = %behavior.getComponentField(%fieldIndex);
   %name = getField(%fieldInfo, 0);
   
   %data =  %behavior.getFieldValue(%name, 0);
   if(%data $= "")
      %data = getField(%fieldInfo, 2);
      
   %description = %behavior.getBehaviorFieldDescription(%fieldIndex);
   
   %control = %this.createTextEditProperty(%name, 0, %name, %description, %data);
   
   %group = getField(%fieldInfo, 3);
   %this.addControlToStack(%group, %behavior, %control);
      
   %editField = %control.findObjectByInternalName(%name @ "TextEdit");
   %editField.object = %behavior;
}

// Float. Text field validated to a number with 3 digits of precision.
BehaviorEditor::registerFieldType("float", "createFloatGui");

function BehaviorFieldStack::createFloatGui(%this, %behavior, %fieldIndex)
{
   %fieldInfo = %behavior.getComponentField(%fieldIndex);
   %name = getField(%fieldInfo, 0);
   
   %data =  %behavior.getFieldValue(%name, 0);
   if(%data $= "")
      %data = getField(%fieldInfo, 2);
      
   %description = %behavior.getBehaviorFieldDescription(%fieldIndex);
   
   %control = %this.createTextEditProperty(%name, 3, %name, %description, %data);
   
   %group = getField(%fieldInfo, 3);
   %this.addControlToStack(%group, %behavior, %control);
      
   %editField = %control.findObjectByInternalName(%name @ "TextEdit");
   %editField.object = %behavior;
}

// Point2F. Two text edits validated to a number with 3 digits of precision.
BehaviorEditor::registerFieldType("Point2F", "createPoint2FGui");

function BehaviorFieldStack::createPoint2FGui(%this, %behavior, %fieldIndex)
{
   %fieldInfo = %behavior.getComponentField(%fieldIndex);
   %name = getField(%fieldInfo, 0);
   %description = %behavior.getBehaviorFieldDescription(%fieldIndex);
   
   %control = %this.createTextEdit2(%name, %name, 3, %name, "X", "Y", %description);
   %edit1 = %control.findObjectByInternalName(%name @ "TextEdit0");
   %edit2 = %control.findObjectByInternalName(%name @ "TextEdit1");
   %edit1.isProperty = true;
   %edit2.isProperty = true;
   %edit1.object = %behavior;
   %edit2.object = %behavior;
}

// Bool. Check box.
BehaviorEditor::registerFieldType("bool", "createBoolGui");

function BehaviorFieldStack::createBoolGui(%this, %behavior, %fieldIndex)
{
   %fieldInfo = %behavior.getComponentField(%fieldIndex);
   %name = getField(%fieldInfo, 0);
   %description = %behavior.getBehaviorFieldDescription(%fieldIndex);
   
   %control = %this.createCheckBox(%name, %name, %description, "", "", "", true);
   
   %group = getField(%fieldInfo, 3);
   %this.addControlToStack(%group, %behavior, %control);
      
   %editField = %control.findObjectByInternalName(%name @ "CheckBox");
   %editField.object = %behavior;
}

// Enum. Drop down list.
BehaviorEditor::registerFieldType("enum", "createEnumGui");

function BehaviorFieldStack::createEnumGui(%this, %behavior, %fieldIndex)
{
   %fieldInfo = %behavior.getComponentField(%fieldIndex);
   %name = getField(%fieldInfo, 0);
   %description = %behavior.getBehaviorFieldDescription(%fieldIndex);
   %list = %behavior.getBehaviorFieldUserData(%fieldIndex);
   
   %control = %this.createDropDownList(%name, %name, "", %list, %description, true, true);
   %listCtrl = %control.findObjectByInternalName(%name @ "DropDown");
   %listCtrl.object = %behavior;
}

// Object. Drop down list containing named objects of a specified type.
BehaviorEditor::registerFieldType("Object", "createObjectGui");

function BehaviorFieldStack::createObjectGui(%this, %behavior, %fieldIndex)
{
   %fieldInfo = %behavior.getComponentField(%fieldIndex);
   %name = getField(%fieldInfo, 0);
   %description = %behavior.getBehaviorFieldDescription(%fieldIndex);
   %objectType = %behavior.getBehaviorFieldUserData(%fieldIndex);
   
   // Everything we could possibly want should be contained in either the
   // scenegraph or the managed datablock set.
   
   %scenegraph = ToolManager.getLastWindow().getSceneGraph();
   %count = %scenegraph.getSceneObjectCount();
   %list = "None";
   for( %i = 0; %i < %count; %i++ )
   {
      %sceneObject = %scenegraph.getSceneObject( %i );
      if( !%sceneObject.isMemberOfClass( %objectType ) )
         continue;
      
      if( %sceneObject.getName() $= "" )
         continue;
         
      %list = %list TAB %sceneObject.getName();
   }
   
   %count = $managedDatablockSet.getCount();
   for( %i = 0; %i < %count; %i++ )
   {
      %object = $managedDatablockSet.getObject( %i );
      if( !%object.isMemberOfClass( %objectType ) )
         continue;
      
      if( %object.getName() $= "" )
         continue;
         
      %list = %list TAB %object.getName();
   }
   
   %control = %this.createDropDownList(%name, %name, "", %list, %description, true, true);
   %listCtrl = %control.findObjectByInternalName(%name @ "DropDown");
   %listCtrl.object = %behavior;
}

// Enum. Drop down list.
BehaviorEditor::registerFieldType("keybind", "createKeybindGui");

function BehaviorFieldStack::createKeybindGui(%this, %behavior, %fieldIndex)
{
   %fieldInfo = %behavior.getComponentField(%fieldIndex);
   %name = getField(%fieldInfo, 0);
   %description = %behavior.getBehaviorFieldDescription(%fieldIndex);
   
   %control = %this.createKeybindList(%name, %name, %description, true, true);
   
   %group = getField(%fieldInfo, 3);
   %this.addControlToStack(%group, %behavior, %control);
      
   %listCtrl = %control.findObjectByInternalName(%name @ "DropDown");
   %listCtrl.object = %behavior;
}

// Int. Text field validated to a whole number.
BehaviorEditor::registerFieldType("color", "createColorGui");

function BehaviorFieldStack::createColorGui(%this, %behavior, %fieldIndex)
{
   %fieldInfo = %behavior.getComponentField(%fieldIndex);
   %name = getField(%fieldInfo, 0);
   %description = %behavior.getBehaviorFieldDescription(%fieldIndex);
   
   %control = %this.createColorPicker(%name, %name, %description);
   %editField = %control.findObjectByInternalName("QuickEditColorPicker");
   %editField.object = %behavior;
   %editField.isProperty = true;
}

// Polygon Editor
BehaviorEditor::registerFieldType("polygon", "createPolygonGui");

function BehaviorFieldStack::createPolygonGui(%this, %behavior, %fieldIndex)
{
   %fieldInfo = %behavior.getComponentField(%fieldIndex);
   %name = getField(%fieldInfo, 0);
   %description = %behavior.getBehaviorFieldDescription(%fieldIndex);

   %control = %this.createCommandButton("EditBehaviorPolygon(" @ %behavior.owner @ ", "
                                                               @ %behavior @ ", "
                                                               @ %name @ ");",
                                        %name @ " - Edit Polygon");
}

// Polygon Editor
BehaviorEditor::registerFieldType("localpointlist", "createLocalPointListGui");

function BehaviorFieldStack::createLocalPointListGui(%this, %behavior, %fieldIndex)
{
   %fieldInfo = %behavior.getComponentField(%fieldIndex);
   %name = getField(%fieldInfo, 0);
   %description = %behavior.getBehaviorFieldDescription(%fieldIndex);

   %control = %this.createCommandButton("EditBehaviorLocalPointList(" @ %behavior.owner @ ", "
                                                                      @ %behavior @ ", "
                                                                      @ %name @ ");",
                                        %name @ " - Edit Local Point List");
}

// File. Opens a load file window.
BehaviorEditor::registerFieldType("file", "createFileGui");

function BehaviorFieldStack::createFileGui(%this, %behavior, %fieldIndex)
{
   %fieldInfo = %behavior.getComponentField(%fieldIndex);
   %name = getField(%fieldInfo, 0);
   %description = %behavior.getBehaviorFieldDescription(%fieldIndex);
   
   %data =  %behavior.getFieldValue(%name, 0);
   if(%data $= "")
      %data = getField(%fieldInfo, 2);
   
   %control = %this.createFileProperty(%name, "All", %description, "Load a file", %data);
   
   %group = getField(%fieldInfo, 3);
   %this.addControlToStack(%group, %behavior, %control);
      
   %editField = %control.findObjectByInternalName(%name @ "File");
   %editField.object = %behavior;
}

// Shape File. Opens a load file window with shape file filters.
BehaviorEditor::registerFieldType("imageFile", "createImageFileGui");

function BehaviorFieldStack::createImageFileGui(%this, %behavior, %fieldIndex)
{
   %fieldInfo = %behavior.getComponentField(%fieldIndex);
   %name = getField(%fieldInfo, 0);
   %description = %behavior.getBehaviorFieldDescription(%fieldIndex);
   
   %data =  %behavior.getFieldValue(%name, 0);
   if(%data $= "")
      %data = getField(%fieldInfo, 2);
   
   %control = %this.createFileProperty(%name, "images", %description, "Load a file", %data);
   
   %group = getField(%fieldInfo, 3);
   %this.addControlToStack(%group, %behavior, %control);
      
   %editField = %control.findObjectByInternalName(%name @ "File");
   %editField.object = %behavior;
}

// Shape File. Opens a load file window with shape file filters.
BehaviorEditor::registerFieldType("modelFile", "createModelFileGui");

function BehaviorFieldStack::createModelFileGui(%this, %behavior, %fieldIndex)
{
   %fieldInfo = %behavior.getComponentField(%fieldIndex);
   %name = getField(%fieldInfo, 0);
   %description = %behavior.getBehaviorFieldDescription(%fieldIndex);
   
   %data =  %behavior.getFieldValue(%name, 0);
   if(%data $= "")
      %data = getField(%fieldInfo, 2);
   
   %control = %this.createFileProperty(%name, "models", %description, "Load a file", %data);
   
   %group = getField(%fieldInfo, 3);
   %this.addControlToStack(%group, %behavior, %control);
      
   %editField = %control.findObjectByInternalName(%name @ "File");
   %editField.object = %behavior;
}

// Shape File. Opens a load file window with shape file filters.
BehaviorEditor::registerFieldType("soundFile", "createSoundFileGui");

function BehaviorFieldStack::createSoundFileGui(%this, %behavior, %fieldIndex)
{
   %fieldInfo = %behavior.getComponentField(%fieldIndex);
   %name = getField(%fieldInfo, 0);
   %description = %behavior.getBehaviorFieldDescription(%fieldIndex);
   
   %data =  %behavior.getFieldValue(%name, 0);
   if(%data $= "")
      %data = getField(%fieldInfo, 2);
   
   %control = %this.createFileProperty(%name, "sounds", %description, "Load a file", %data);
   
   %group = getField(%fieldInfo, 3);
   %this.addControlToStack(%group, %behavior, %control);
      
   %editField = %control.findObjectByInternalName(%name @ "File");
   %editField.object = %behavior;
}

BehaviorEditor::registerFieldType("stateMachine", "createStateMachineEditor");
BehaviorEditor::registerFieldType("stateMachineState", "createSMStates");
//==============================================================================
function BehaviorFieldStack::addControlToStack(%this, %group, %behavior, %control)
{
   if(%group !$= "")
   {
      %groupObj = %behavior.getTemplateName()@"_"@%group@"_group";
      if(!isObject(%groupObj)){
         %this.createBehaviorGroup(%group, %groupObj);
         %this.add(%groupObj);
      }
         
      %groupObj.stack.add(%control);
   }
   else
      %this.add(%control);
}

function BehaviorEditorField::apply(%this, %newData)
{
   echo(%this @ " had an update applied! New data is:" SPC %newData);
}


//
//
//
//
//
BehaviorEditor::registerFieldType("fieldButton", "createFieldButton");
function BehaviorFieldStack::createFieldButton(%this, %behavior, %fieldIndex)
{
   %fieldInfo = %behavior.getComponentField(%fieldIndex);
   %name = getField(%fieldInfo, 0);
   
   %button = editorFieldBuilder::buildButtonField("", %name, "0 0", "300 24");

   if(!isObject(ConvexShapeBehaviorEditor))
	  new ConvexShapeBehaviorTool(ConvexShapeBehaviorEditor);
	
	ConvexShapeBehaviorEditor.defaultTemplate = ConvexShapeBhvr;
	
   %button-->data.command = "EWorldEditor.setActiveTool(ConvexShapeBehaviorEditor);";
   %this.add(%button);

	%exitBttn = editorFieldBuilder::buildButtonField("", "Close Convex Editor", "0 0", "300 24");
	%exitBttn-->data.command = "EWorldEditor.setActiveTool();";
	%exitBttn.setName("convexExit");

	//Canvas.pushDialog(%exitBttn);
}

//
BehaviorEditor::registerFieldType("animationList", "createAnimList");
function BehaviorFieldStack::createAnimList(%this, %behavior, %fieldIndex)
{
   %fieldInfo = %behavior.getComponentField(%fieldIndex);
   %name = getField(%fieldInfo, 0);
   %description = %behavior.getBehaviorFieldDescription(%fieldIndex);
   %objectType = %behavior.getBehaviorFieldUserData(%fieldIndex);
   
   // Everything we could possibly want should be contained in either the
   // scenegraph or the managed datablock set.
   
   %shapeBehav = %behavior.owner.getBehavior(RenderShape);

   if(%shapeBehav.shapeName $= "") 
   {
   	  %ctrl = %this.createLabelField(%name, "No Shape", "The entity either doesn't have a RenderShape, or no shape is selected.");
	  %this.add(%ctrl);
	  return;
   }
   else
   {
	  %path = %shapeBehav.shapeName;
   }

   //track down the shape constructor for our shape
   %found = false;
   %count = TSShapeConstructorGroup.getCount();
   for ( %i = 0; %i < %count; %i++ )
   {
      %shape = TSShapeConstructorGroup.getObject( %i );
      if ( %shape.baseShape $= %path )
	  {
		 %found = true;
         break;
	  }
   }

   if(!%found)
   {
      %name = strcapitalise( fileBase( %path ) ) @ strcapitalise( getSubStr( fileExt( %path ), 1, 3 ) );
      %name = strreplace( %name, "-", "_" );
      %name = strreplace( %name, ".", "_" );
      %name = getUniqueName( %name );
      %shape = new TSShapeConstructor( %name ) { baseShape = %path; };
   }

   %container = %this.createDropDownList(%name, %name, %description, true, true);

   %listCtrl = %container.findObjectByInternalName(%name @ "DropDown");
   %listCtrl.object = %behavior;

   %listCtrl.add("None", 0);

   %count = %shape.getSequenceCount();
   for ( %i = 0; %i < %count; %i++ )
   {
      %name = %shape.getSequenceName( %i );

      // Ignore __backup__ sequences (only used by editor)
      if ( !startswith( %name, "__backup__" ) )
		%listCtrl.add(%name, %i+1);
   }

   %this.add(%container);
}

// Shape File. Opens a load file window with shape file filters.
BehaviorEditor::registerFieldType("material", "createMaterialGui");

function BehaviorFieldStack::createMaterialGui(%this, %behavior, %fieldIndex)
{
   %fieldInfo = %behavior.getComponentField(%fieldIndex);
   %name = getField(%fieldInfo, 0);
   //%description = %behavior.getBehaviorFieldDescription(%fieldIndex);
   
   %data =  %behavior.getFieldValue(%name, 0);
   if(%data $= "")
      %data = getField(%fieldInfo, 2);
   
   %control = %this.createMaterialProperty(%name, "Load a file", %behavior, %data);
   
   %group = getField(%fieldInfo, 3);
   %this.addControlToStack(%group, %behavior, %control);
      
   %editField = %control.findObjectByInternalName(%name @ "File");
   %editField.object = %behavior;
}

BehaviorEditor::registerFieldType("fieldButton", "createFieldButton");
function BehaviorFieldStack::createFieldButton(%this, %behavior, %fieldIndex)
{
   %fieldInfo = %behavior.getComponentField(%fieldIndex);
   %name = getField(%fieldInfo, 0);
   
   %button = buildButtonField(%name, "0 0", "300 24");

   if(!isObject(ConvexShapeComponentEditor))
	  new ConvexShapeComponentTool(ConvexShapeComponentEditor);
	
	ConvexShapeComponentEditor.defaultTemplate = ConvexShapeComp;
	
   %button-->data.command = "EWorldEditor.setActiveTool(ConvexShapeComponentEditor);";
   %this.add(%button);

	%exitBttn = buildButtonField("Close Convex Editor", "0 0", "300 24");
	%exitBttn-->data.command = "EWorldEditor.setActiveTool();";
	%exitBttn.setName("convexExit");

	//Canvas.pushDialog(%exitBttn);
}

BehaviorEditor::registerFieldType("tileEditorButton", "createTileEditorButton");
function BehaviorFieldStack::createTileEditorButton(%this, %behavior, %fieldIndex)
{
   %fieldInfo = %behavior.getComponentField(%fieldIndex);
   %name = getField(%fieldInfo, 0);
   
   %button = buildButtonField(%name, "0 0", "300 24");

   if(!isObject(LevelTileComponentEditor))
	  new LevelTileComponentTool(LevelTileComponentEditor);
	
	LevelTileComponentEditor.selectedTile = %behavior;
	
    %button-->data.command = "EWorldEditor.setActiveTool(LevelTileComponentEditor);";
    %this.add(%button);

	%exitBttn = buildButtonField("Close Tile Editor", "0 0", "300 24");
	%exitBttn-->data.command = "EWorldEditor.setActiveTool();";
	%exitBttn.setName("convexExit");

	//Canvas.pushDialog(%exitBttn);
}