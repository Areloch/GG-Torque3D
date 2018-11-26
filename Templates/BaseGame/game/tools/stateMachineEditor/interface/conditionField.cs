function StateMachineInspector::createConditionField(%this, %parentGroup, %name)
{
   %stack = %parentGroup.getObject(0);
   %guiControl = new GuiControl()
   {
      position = "0 0";
      extent = %stack.extent.x SPC 20;
      transitionIdx = StateMachineGraph.selectedConnection;
      
      new GuiPopUpMenuCtrl()
      {
         class="SMConditionField";
         position = "0 0";
         extent = "70 20";
         internalName = "Field";
         text = %name;
      };
      
      new GuiPopUpMenuCtrl()
      {
         class="SMConditionComparitor";
         position = "70 0";
         extent = "30 20";
         internalName = "Comparitor";
      };
      
      new GuiTextEditCtrl()
      {
         class="SMConditionValue";
         position = "100 0";
         extent = "50 20";
         internalName = "Value";
      };
      
      new GuiBitmapButtonCtrl()
      {
         bitmap = "tools/gui/images/iconDelete";
         position = 150 SPC 0;
         extent = "20 20";
         //command = %this @ ".createSMField(StateMachinePropertiesGroup, \"\");";
      };
   };
   
   %guiControl-->field.refresh();
   %guiControl-->comparitor.refresh();
        
   %stack.add(%guiControl);
   return %guiControl;
}

function SMConditionField::refresh(%this)
{
   %count = StateMachineGraph.stateVarsArray.count();
   
   for(%i=0; %i < %count; %i++)
   {
      %varName = StateMachineGraph.stateVarsArray.getKey(%i);  
      %this.add(%varName);
   }
}

function SMConditionComparitor::refresh(%this)
{
   %this.add( ">" ); 
   %this.add( ">=" ); 
   %this.add( "==" );
   %this.add( "<=" );
   %this.add( "<" );
   %this.add( "!=" );
}

function SMConditionValue::onReturn(%this)
{
   //we need all 3 elements of a rule t0 be valid to store the rule
   %fieldName = %this.getParent()-->field.getText();
   %comparitor = %this.getParent()-->comparitor.getText();
   %value = %this.getText();
   
   if(%fieldName $= "" || %comparitor $= "" || %value $= "")
      return;
      
   %transitionIdx = %this.getParent().transitionIdx;
   %idx = StateMachineGraph.transitions.getIndexFromKey(%transitionIdx);
   
   %ruleList = StateMachineGraph.transitions.getValue(%idx);
   
   //Translate the comparitor
   if(%comparitor $= ">=")
      %comparitor = "GreaterOrEqual";
   else if(%comparitor $= ">")
      %comparitor = "GreaterThan";
   else if(%comparitor $= "==")
      %comparitor = "Equals";
   else if(%comparitor $= "<")
      %comparitor = "LessThan";
   else if(%comparitor $= "<=")
      %comparitor = "LessOrEqual";
   else if(%comparitor $= "!=")
      %comparitor = "DoesNotEqual";
   
   %ruleIdx = %ruleList.count();
   %ruleList.add(%ruleIdx, %fieldName TAB %comparitor TAB %value);
   
   %ruleList.uniquekey();
   
   StateMachineGraph.setConnectionError(%transitionIdx, false);
}