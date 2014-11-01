//-----------------------------------------------------------------------------
// Torque Game Builder
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

singleton Component(ItemRotateBehavior)
{
   friendlyName = "Item Rotation";
   behaviorType = "Game";
   description  = "Rotates the entity around the z axis, like an item pickup";

   networked = true;
};

ItemRotateBehavior.addBehaviorField(rotationsPerMinute, "Number of rotations per minute", float, "5.0");
ItemRotateBehavior.addBehaviorField(forward, "Rotate forward or backwards", bool, "1");
ItemRotateBehavior.addBehaviorField(horizontal, "Rotate horizontal or verticle, true for horizontal", bool, "1");


function ItemRotateBehavior::Update(%this)
{
	//Rotations per second is calculated based on a standard update tick being 32ms. So we scale by the tick speed, then add that to our rotation to 
	//get a nice rotation speed.
	if(%this.horizontal)
	{
		if(%this.forward)
			%this.owner.rotation.z += ( ( 360 * %this.rotationsPerMinute ) / 60 ) * 0.032;
		else
			%this.owner.rotation.z -= ( ( 360 * %this.rotationsPerMinute ) / 60 ) * 0.032;
	}
	else
	{
		%this.owner.rotation.x += ( ( 360 * %this.rotationsPerMinute ) / 60 ) * 0.032;
	}
}