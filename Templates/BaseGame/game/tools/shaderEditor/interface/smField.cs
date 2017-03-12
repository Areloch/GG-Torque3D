function ShaderEditorInspector::createSMField(%this, %parentGroup, %name)
{
   %stack = %parentGroup.getObject(0);
   
   %newFieldIdx = %stack.getCount();
   
   %guiControl = new GuiControl()
   {
      position = "0 0";
      extent = %stack.extent.x SPC 20;
      fieldIdx = %newFieldIdx;
      
      new GuiTextEditCtrl()
      {
         class="SMFieldName";
         position = "0 0";
         extent = "100 20";
         internalName = "Field";
         text = %name;
      };
      
      new GuiTextEditCtrl()
      {
         class="SMFieldValue";
         position = "100 0";
         extent = "50 20";
         internalName = "Value";
      };
      
      new GuiBitmapButtonCtrl()
      {
         class="SMFieldRemoveBtn";
         bitmap = "tools/gui/images/iconDelete";
         position = 150 SPC 0;
         extent = "20 20";
      };
   };
        
   %stack.add(%guiControl);
   return %guiControl;
}

function ShaderEdtiorFieldRemoveBtn::onClick(%this)
{
   %fieldIdx = %this.getParent().fieldIdx - 1;
   StateMachineGraph.stateVarsArray.erase(%fieldIdx);
   %this.getParent().getParent().remove(%this.getParent());
}

function ShaderEdtiorFieldName::onReturn(%this)
{
   if(!isObject(StateMachineGraph.stateVarsArray))
   {
      StateMachineGraph.stateVarsArray = new ArrayObject();
   }
   
   StateMachineGraph.stateVarsArray.add(%this.getText(), "");
   
   //remove duplicates
   StateMachineGraph.stateVarsArray.uniquekey();
   
   if(StateMachineGraph.selectedConnection != -1)
      StateMachineGraph.onConnectionSelected(StateMachineGraph.selectedConnection);
   else if(StateMachineGraph.selectedNode != -1)
      StateMachineGraph.onNodeSelected(StateMachineGraph.selectedNode);
}   

function ShaderEdtiorFieldValue::onReturn(%this)
{
   if(!isObject(StateMachineGraph.stateVarsArray))
   {
      StateMachineGraph.stateVarsArray = new ArrayObject();
   }
   
   %fieldName = %this.getParent()-->field;
   %arrayIdx = StateMachineGraph.stateVarsArray.getIndexFromKey(%fieldName.getText());
   
   if(%arrayIdx == -1)
      return;
   
   %value = %this.getText();
   StateMachineGraph.stateVarsArray.setValue(%value, %arrayIdx);
}