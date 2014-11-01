//-----------------------------------------------------------------------------
// Editor Custom Rollout - 'BehaviorsRollout'
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// Register Form Content
//
// For more information on the Form Content and the inner workings of it,
// please refer to editorClasses/script/guiFormContentManager.ed.cs and the
// documentation on the functions.
//
/*GuiFormManager::AddFormContent( 
  "LevelBuilderQuickEditClasses", // Content Library Name
  "t2dSceneObject Behaviors",     // Objects C++ class name or 'class' of object
  "BehaviorsRollout::CreateContent",// Create function
  "BehaviorsRollout::SaveContent",  // Save function
  2 );                            // Deprecated 'magin' option*/
  
                                
//-----------------------------------------------------------------------------
// Define form save function.
//
// This is unused here, but is called on our content right before we
// get deleted.
//
function BehaviorsRollout::SaveContent( %contentCtrl ) 
{
   // Nothing
}

//-----------------------------------------------------------------------------
// Define form create function.
//
// Your standard form create function that creates a base and
// two check boxes that control setFlipX and setFlipY
function GuiInspectorBehaviorGroup::CreateContent(%this)
{
   // It's important to note that the class we give the first 
   // parameter of this function should NOT be the same as the
   // one we're in right now.  We add the QE to the end to 
   // differentiate them
   //%base = %contentCtrl.createBaseStack("BehaviorsRolloutQE", %quickEditObj);
   //%behaviorRollout = %base.createRolloutStack("Behaviors", true);
   %this.margin = "4 8 4 0";
   %this.stack.padding = 4;
   %this.stack.margin = "0 16 0 0";
   //%this.listControl = %this.createAddBehaviorList();
   //%this.stack.add(%this.listControl);
   //%this.add(%listControl);
   
   // Whenever we create form content, we must return it to the base
   //return %base;
   %this.setName("InspectorBehaviorGroup");
}

function GuiInspectorBehaviorGroup::InspectObject( %this, %targetObject )
{
   %this.stack.clear();
   %this.stack.addGuiControl(%this.createAddBehaviorList());
 
   for(%i=0; %i < %targetObject.getBehaviorCount(); %i++)
   {
      %behavior = BehaviorEditor::createBehaviorRollout(%this, %targetObject.getBehaviorByIndex(%i));
      %this.stack.addGuiControl(%behavior);
   }
   
   //%this.stack.updateStack();
   
   //%this.sizeToContents();
   
   //%this.instantCollapse();
   //%this.instantExpand();
}

function BehaviorEditor::registerFieldType(%type, %create)
{
   $BehaviorEditor::fieldTypes[%type] = %create;
}

function BehaviorEditor::createFieldGui(%parent, %behavior, %fieldIndex)
{
   %templatefieldInfo = %behavior.getComponentField(%fieldIndex);
   %fieldInfo = %behavior.getComponentField(%fieldIndex);
   %name = getField(%fieldInfo, 0);
   %type = getField(%fieldInfo, 1);
   %defaultVal = getField(%fieldInfo, 2);
   %group = getField(%fieldInfo, 3);
   
   %create = $BehaviorEditor::fieldTypes[%type];
   if (%create $= "")
      %create = $BehaviorEditor::fieldTypes["Default"];
   
   %parent.call(%create, %behavior, %fieldIndex);
}
