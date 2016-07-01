function InventoryController::onAdd(%this)
{
   %this.inventoryList = new ArrayObject();
}

function InventoryController::onRemove(%this)
{
   %this.inventoryList.deleteObject();
}

function InventoryController::addItem(%this, %item, %quantity)
{
   //first, see if we have one of these already
   
   %found = %this.isItemInInventory(%item);
   if(%found)
   {
      
   }
}

function InventoryController::throwItem(%this, %item, %quantity)
{
   //first, see if we have one of these already
   
   //%found = %this.inventoryList.
}

function InventoryController::isItemInInventory(%this, %item)
{
   %itemComp = %item.getComponent(ItemComponent);
   
   if(!isObject(%itemComp))
      return false;
      
   //first, see if we have one of these already
   %count = %this.inventoryList.countKey(%itemComp.itemName);
   
   return %count > 0;
}
