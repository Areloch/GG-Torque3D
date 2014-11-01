function BehaviorEditor::createBehaviorRollout(%this, %behavior)
{
   %template = %behavior.template;
   %name = %template.friendlyName;
   
   %rollout = new GuiRolloutCtrl() 
   {
      class = "BehaviorRollout";
      //superclass = LBQuickEditRollout;
      Profile = "EditorBehaviorRolloutProfile";
      HorizSizing = "width";
      VertSizing = "height";
      Position = "0 0";
      Extent = (%this.extent.x - 4) SPC "0";
      Caption = %name;
      Margin = "7 4";
      MinExtent = "30 18";
      DragSizable = false;
      container = true;
      parentRollout = %this;
   };
   
   %contExt = (%rollout.extent.x - 18);
   
   /*%fieldContainer = new GuiStackControl()
   {
      class = "BehaviorFieldStack";
      superclass = "LBQuickEditContent";
      Profile = "EditorContainerProfile";
      HorizSizing = "right";
      VertSizing = "bottom";
      Position = "0 36";
      Extent = (%rollout.extent.x - 18) SPC "4";
      object = %behavior;
      rollout = %rollout;
      padding = 6;
   };
   %rollout.stack = %fieldContainer;*/

   %fieldContainer = new GuiSimpleStackCtrl()
   {
      class = "BehaviorFieldStack";
      superclass = "LBQuickEditContent";
      Profile = "EditorContainerProfile";
      HorizSizing = "right";
      VertSizing = "bottom";
      Position = "0 36";
      Extent = (%rollout.extent.x - 18) SPC "4";
      object = %behavior;
      rollout = %rollout;
      padding = 6;
	  extendParent = false;
   };
   %rollout.stack = %fieldContainer;

   %rollout.add( %fieldContainer );
   
   %button = new GuiIconButtonCtrl()
   {
      class = RemoveBehaviorButton;
      Profile = "EditorButton";
      HorizSizing = "right";
      VertSizing = "bottom";
      Position = "10 5";
      Extent = (%fieldContainer.extent.x - 11) SPC "16";
      iconBitmap = "tools/gui/images/iconDelete.png";
      text = "Remove This Behavior";
      buttonMargin = "4 4";
      iconLocation = "Right";
      sizeIconToButton = "0";
      textLocation = "Center";
      textMargin = "4";
      hovertime = "100";
      tooltip = "Remove this Behavior from the object";
      tooltipProfile = "EditorToolTipProfile";
      object = Inspector.getInspectObject(0);
      behavior = %behavior;
      command = %this@".removeBehavior("@%behavior@");";
   };
   
   %checkBox = new GuiCheckBoxCtrl() {
      canSaveDynamicFields = "0";
      class = BehaviorEnableCheckBox;
      internalName = %accessor @ "CheckBox";
      Profile = "ToolsGuiCheckBoxProfile";
      HorizSizing = "left";
      VertSizing = "bottom";
      Position = (%this.extent.x - 34) SPC "2";
      Extent = "13 13";
      MinExtent = "8 2";
      canSave = "1";
      Visible = "1";
      hovertime = "100";
      text = "";
      tooltip = "Toggles weither this behavior is enabled or not";
      tooltipProfile = "EditorToolTipProfile";
      groupNum = "-1";
      buttonType = "ToggleButton";
      object = Inspector.getInspectObject(0);
      behavior = %behavior;
   };
   %checkBox.setStateOn(%behavior.enabled);
   
   if(getWordCount(%behavior.missingDependencies) > 0)
   {
      for(%i=0; %i < getWordCount(%behavior.missingDependencies); %i++)
      {
         %missing = %missing NL "Behavior missing:" SPC getWord(%behavior.missingDependencies, %i);
      }
      %checkBox.tooltip = "This behavior has been disabled due to a dependency conflict:"NL%missing;
      %checkBox.hovertime = "50"; //make sure this pops up quickly to avoid confusion
   }
   
   //gotoWebPage( GuiEditor.documentationURL );
   %helpBtn = new GuiBitmapButtonCtrl() {
      canSaveDynamicFields = "1";
      internalName = %accessor @ "DocsBtn";
      Enabled = "1";
      isContainer = "0";
      Profile = "GuiDefaultProfile";
      HorizSizing = "left";
      VertSizing = "bottom";
      Position = (%this.extent.x - 18) SPC "1";
      Extent = "14 14";
      MinExtent = "8 2";
      canSave = "0";
      Visible = "1";
      tooltip = "Opens documentation for this behavior (not yet implemented)";
      tooltipProfile = "EditorToolTipProfile";
      hovertime = "100";
      bitmap = "tools/behaviorEditor/gui/images/docs";
      wrap = "0";
      //command = "gotoWebPage( GuiEditor.documentationURL )";
   };
   
   /*%count = %template.getBehaviorFieldCount();
   for( %i = 0; %i < %count; %i++ )
   {
      //we skip the enabled field because we integrate that into the rollout
      %name = getField(%template.getBehaviorField(%i), 0);
      if(%name $= "enabled")
         continue;
         
      BehaviorEditor::createFieldGui(%fieldContainer, %behavior, %i);
   }*/

   //then we check our instance if it has any ones unique to it
   %count = %behavior.getComponentFieldCount();
   for( %i = 0; %i < %count; %i++ )
   {
      //we skip the enabled field because we integrate that into the rollout
      %name = getField(%behavior.getComponentField(%i), 0);
      if(%name $= "enabled")
         continue;
         
      BehaviorEditor::createFieldGui(%fieldContainer, %behavior, %i);
   }

   %fieldContainer.add( %button );
   %rollout.add( %checkBox );
   %rollout.add( %helpBtn );
   
   return %rollout;
}

function BehaviorRollout::onCollapsed( %this )
{
   // Force resize
   //%this.parentRollout.stack.updateStack();
   
   %this.parentRollout.sizeToContents();
   //InspectorBehaviorStack.updateStack();
   InspectorBehaviorGroup.sizeToContents();
}

function BehaviorRollout::onExpanded( %this )
{
   // Force resize
   //%this.parentRollout.stack.updateStack();
   
   %this.parentRollout.sizeToContents();
   //InspectorBehaviorStack.updateStack();
   InspectorBehaviorGroup.sizeToContents();
}

function BehaviorRollout::onChildResized( %this, %child )
{
   %this.sizeToContents();
   InspectorBehaviorGroup.updateStack();
}

function RemoveBehaviorButton::onClick( %this )
{
   //first, check if we would cause any conflicts
   %conflictList = BehaviorManager::checkForDependants( %this.behavior );
   
   if(getFieldCount(%conflictList) > 0)
   {
      for(%i=0; %i < getFieldCount(%conflictList); %i++)
      {
          %conflicts = %conflicts NL "Behavior missing:" SPC getField(%conflictList, %i);
      }
      
      MessageBoxOKCancel( "Dependency Conflict!", 
                  "Removing this behavior will cause dependency conflicts for the following behaviors:\n"
                  @%conflicts
                  @"\n\nIf you continue, the conflicted behaviors will be disabled.",
                           %this@".okRemoveBehavior();",
                           "");
   }
   else
      %this.okRemoveBehavior();
}

function RemoveBehaviorButton::okRemoveBehavior(%this)
{
   ToolManager.getLastWindow().setFirstResponder();

   %undo = new UndoScriptAction()
   {
      actionName = "Removed Behavior";
      class = UndoRemoveBehavior;
      object = %this.behavior.owner;
      behavior = %this.behavior;
   };
   %this.behavior.owner.removeBehavior(%this.behavior, false);
   %undo.addToManager(LevelBuilderUndoManager);
   
   Inspector.schedule( 50, "refresh" );
   EWorldEditor.isDirty = true;
}

function BehaviorEnableCheckBox::onClick( %this )
{
   if(%this.behavior.missingDependencies !$= "")
   {
      %this.setStateOn(0);
      %this.behavior.setEnabled(false);
   }
   else
   {
      %this.behavior.setEnabled(!%this.behavior.isEnabled());
      EWorldEditor.isDirty = true;
   }
}

//==============================================================================
// For behavior field sub-grounds
//==============================================================================
function BehaviorFieldStack::createBehaviorGroup(%this, %groupName, %groupObject)
{
   new GuiRolloutCtrl(%groupObject) 
   {
      class = "BhvrGroupRollout";
      Profile = "EditorBehaviorGroupProfile";
      HorizSizing = "width";
      VertSizing = "height";
      Position = "0 0";
      Extent = (%this.extent.x - 4) SPC "0";
      Caption = %groupName;
      Margin = "7 4";
      MinExtent = "30 18";
      DragSizable = false;
      container = true;
      parentStack = %this;
      clickCollapse = false;
   };
   
   /*%fieldContainer = new GuiStackControl()
   {
      class = "BehaviorFieldStack";
      superclass = "LBQuickEditContent";
      Profile = "EditorContainerProfile";
      HorizSizing = "right";
      VertSizing = "bottom";
      Position = "0 36";
      Extent = (%this.extent.x - 11) SPC "4";
      padding = 4;
   };*/

   %fieldContainer = new GuiSimpleStackCtrl()
   {
	  class = "BehaviorFieldStack";
      superclass = "LBQuickEditContent";
      Profile = "EditorContainerProfile";
      HorizSizing = "right";
      VertSizing = "bottom";
      Position = "0 36";
      Extent = (%this.extent.x - 11) SPC "4";
	  extendParent = false;
      padding = 4;
   };
   
   %groupId = %groupObject.getId();
   %groupObject.stack = %fieldContainer;
   
   /*%checkBox = new GuiCheckBoxCtrl() {
      canSaveDynamicFields = "0";
      class = BehaviorEnableCheckBox;
      internalName = %accessor @ "CheckBox";
      Profile = "EditorCheckBox";
      HorizSizing = "left";
      VertSizing = "bottom";
      Position = (%this.extent.x - 34) SPC "2";
      Extent = "13 13";
      MinExtent = "8 2";
      canSave = "0";
      Visible = "1";
      hovertime = "100";
      text = "";
      tooltip = "Toggles weither this behavior is enabled or not";
      tooltipProfile = "EditorToolTipProfile";
      groupNum = "-1";
      buttonType = "ToggleButton";
      object = Inspector.getInspectObject(0);
      command = %behavior@".enabled = !"@%behavior@".enabled;";
   };
   %checkBox.setStateOn(1);*/

   %groupObject.add( %fieldContainer );
   //%groupObject.add( %checkBox );
   
   return %groupObject;
}

//Collapsing the group rollouts is disabled due to a bug in the rollouts.
//When you have rollouts in rollouts and you try and collapse/expand one
//the whole thing explodes violently.
//This will be fixed eventually, but until then, groups can't collapse.
function BhvrGroupRollout::onCollapsed( %this )
{
   // Force resize
   %this.parentStack.updateStack();
}

function BhvrGroupRollout::onExpanded( %this )
{
   // Force resize
   %this.parentStack.updateStack();
}