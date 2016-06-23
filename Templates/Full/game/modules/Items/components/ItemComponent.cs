function ItemComponent::onAdd(%this)
{
   %this.addComponentField(itemName, "Name of the item.", "String", "", "");
   %this.addComponentField(maxInventoryCount, "Max number of these items that can be held in inventory.", "int", "-1", "");
}

function ItemComponent::onInteracted(%this, %user)
{
   %this.owner.hidden = true;
   
   %inventoryControl = %user.owner.getComponent("InventoryController");
   
   %inventoryControl.addItem(%this.owner, 1);
}

