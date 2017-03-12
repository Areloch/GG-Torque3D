function ItemComponent::onAdd(%this)
{
   %this.addComponentField(itemName, "Name of the item.", "String", " ", "");
   %this.addComponentField(maxInventoryCount, "Max number of these items that can be held in inventory.", "int", "-1", "");
}

function ItemComponent::onInteracted(%this, %user)
{
   %this.owner.hidden = true;
   
   %inventoryControl = %user.owner.getComponent("InventoryController");
   
   %inventoryControl.addItem(%this.owner);
}


function ItemData::onThrow(%this, %user, %amount)
{
   // Remove the object from the inventory
   if (%amount $= "")
      %amount = 1;
   if (%this.maxInventory !$= "")
      if (%amount > %this.maxInventory)
         %amount = %this.maxInventory;
   if (!%amount)
      return 0;
   %user.decInventory(%this,%amount);

   // Construct the actual object in the world, and add it to
   // the mission group so it's cleaned up when the mission is
   // done.  The object is given a random z rotation.
   %obj = new Item()
   {
      datablock = %this;
      rotation = "0 0 1 "@ (getRandom() * 360);
      count = %amount;
   };
   MissionGroup.add(%obj);
   %obj.schedulePop();
   return %obj;
}

function ItemData::onPickup(%this, %obj, %user, %amount)
{
    // Add it to the inventory, this currently ignores the request
    // amount, you get what you get.  If the object doesn't have
    // a count or the datablock doesn't have maxIventory set, the
    // object cannot be picked up.

    // See if the object has a count
    %count = %obj.count;
    if (%count $= "")
    {
       // No, so check the datablock
       %count = %this.count;
       if (%count $= "")
       {
          // No, so attempt to provide the maximum amount
          if (%this.maxInventory !$= "")
          {
             if (!(%count = %this.maxInventory))
                return;
          }
          else
             %count = 1;
       }
    }
    
    %user.incInventory(%this, %count);

    // Inform the client what they got.
    if (%user.client)
       messageClient(%user.client, 'MsgItemPickup', '\c0You picked up %1', %this.pickupName);

    // If the item is a static respawn item, then go ahead and
    // respawn it, otherwise remove it from the world.
    // Anything not taken up by inventory is lost.
    if (%obj.isStatic())
       %obj.respawn();
    else
       %obj.delete();
    return true;
}
