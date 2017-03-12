function StateMachineInspector::createNodeNameField(%this, %parentGroup, %name)
{
   %stack = %parentGroup.getObject(0);
   %guiControl = new GuiControl()
   {
      position = "0 0";
      extent = %stack.extent.x SPC 20;
      
      new GuiTextCtrl()
      {
         position = "0 0";
         extent = "50 20";
         internalName = "Label";
         text = %name;
      };
      
      new GuiTextEditCtrl()
      {
         class="SMNodeName";
         position = "50 0";
         extent = "150 20";
         internalName = "Field";
      };
   };
        
   %stack.add(%guiControl);
   return %guiControl;
}

function SMNodeName::onReturn(%this)
{
   StateMachineGraph.setNodeName(StateMachineGraph.selectedNode, %this.getText());
   
   StateMachineGraph.setNodeError(StateMachineGraph.selectedNode, false);
   
   //sanity check the node!
   for(%i=0; %i < StateMachineGraph.getNodeCount(); %i++)
   {
      if(%i == StateMachineGraph.selectedNode)
         continue;
         
      if(%this.getText() $= StateMachineGraph.getNodeName(%i))
      {
         StateMachineGraph.setNodeError(StateMachineGraph.selectedNode, true); 
         StateMachineGraph.setNodeError(%i, true); 
      }
      else
      { 
         StateMachineGraph.setNodeError(%i, false); 
      }
   }
}