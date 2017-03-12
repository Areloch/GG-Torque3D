function InventoryController::onAdd(%this)
{
   %this.inventoryList = new ArrayObject();
}

function InventoryController::onRemove(%this)
{
}

function InventoryController::hasItem(%this, %itemName)
{
   return (%this.inventoryList.countKey(%itemName) > 0);
}

function InventoryController::addItem(%this, %item)
{
   // Increment the inventory by the given amount.  The return value
   // is the amount actually added, which may be less than the
   // requested amount due to inventory restrictions.
   
   %itemName = %item.getComponent("ItemComponent").itemName;
   
   if(%itemName $= "")
   {
      error("Trying to add an item to our inventory that has no name!");
      return;
   }
   
   %this.inventoryList.add(%itemName, %item);
}

function InventoryController::removeItem(%this, %item)
{
   // Decrement the inventory by the given amount. The return value
   // is the amount actually removed.

   %itemIndex = %this.inventoryList.getIndexFromValue(%item);
   
   %this.inventoryList.erase(%itemIndex);
}

function InventoryController::getItemCount(%this, %itemName)
{
   // Return the current inventory amount
   return %this.inventoryList.countKey(%itemName);
}

function InventoryController::getItem(%this, %itemName)
{
   %index = %this.inventoryList.getIndexFromKey(%itemName);
   return %this.inventoryList.getValue(%index);
}

//-----------------------------------------------------------------------------

function InventoryController::getInventory(%this, %data)
{
   // Return the current inventory amount
   return %this.inventory[%data.getName()];
}

function InventoryController::setInventory(%this, %data, %value)
{
   // Set the inventory amount for this datablock and invoke inventory
   // callbacks.  All changes to inventory go through this single method.

   // Impose inventory limits
   if (%value < 0)
      %value = 0;
   else
   {
      %max = %this.maxInventory[%data];
      if (%value > %max)
         %value = %max;
   }

   // Set the value and invoke object callbacks
   %name = %data.getName();
   if (%this.inventory[%name] != %value)
   {
      %this.inventory[%name] = %value;
      %data.onInventory(%this, %value);
      %this.getDataBlock().onInventory(%data, %value);
   }
   return %value;
}

//
function InventoryController::getMaxInventory(%this, %data)
{
   // Return the current inventory amount
   return %this.inventory[%data.getName()];
}

function InventoryController::setMaxInventory(%this, %data, %value)
{
   // Set the inventory amount for this datablock and invoke inventory
   // callbacks.  All changes to inventory go through this single method.

   // Impose inventory limits
   if (%value < 0)
      %value = 0;
   else
   {
      %max = %this.maxInventory(%data);
      if (%value > %max)
         %value = %max;
   }

   // Set the value and invoke object callbacks
   %name = %data.getName();
   if (%this.inventory[%name] != %value)
   {
      %this.inventory[%name] = %value;
      %data.onInventory(%this, %value);
      %this.getDataBlock().onInventory(%data, %value);
   }
   return %value;
}