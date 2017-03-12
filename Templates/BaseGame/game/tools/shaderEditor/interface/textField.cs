function StateMachineInspector::createTextField(%this, %parentGroup, %name)
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
         position = "50 0";
         extent = "150 20";
         internalName = "Field";
      };
   };
        
   %stack.add(%guiControl);
   return %guiControl;
}