function StateMachineInspector::createNodeField(%this, %parentGroup, %name)
{
   %stack = %parentGroup.getObject(0);
   %guiControl = new GuiControl()
   {
      position = "0 0";
      extent = %stack.extent.x SPC 20;
      
      new GuiTextEditCtrl()
      {
         class="NodeFieldName";
         position = "0 0";
         extent = "50 20";
         internalName = "Name";
         text = "";
      };
      
      new GuiTextEditCtrl()
      {
         class="NodeFieldValue";
         position = "50 0";
         extent = "150 20";
         internalName = "Field";
      };
   };
        
   %stack.add(%guiControl);
   return %guiControl;
}

function NodeFieldName::onReturn(%this)
{
   //StateMachineGraph.setNodeField(StateMachineGraph.selectedNode, "scriptFunction", %this.getText());
}

function NodeFieldValue::onReturn(%this)
{
   //StateMachineGraph.setNodeField(StateMachineGraph.selectedNode, "scriptFunction", %this.getText());
}