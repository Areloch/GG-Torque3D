//-----------------------------------------------------------------------------
// Torque Game Builder
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

singleton VisibilityTriggerComponent(VisibilityTrigger);

function VisibilityTrigger::onAdd(%this) 
{
   Parent::onBehaviorAdd(%this);

   %clientCount = ClientGroup.getCount();

   for(%i=0; %i < %clientCount; %i++)
   {
	   %this.addClient(ClientGroup.getObject(%i));
   }
}

/*function VisibilityTrigger::onRemove(%this)
{
   %clientID = %this.getClientID();
   if(%clientID)
      %clientID.clearCameraObject();
}*/

function VisibilityTrigger::onClientConnect(%this, %client)
{
   %this.addClient(%client);
}

function VisibilityTrigger::onClientDisconnect(%this, %client)
{
   Parent::onClientDisconnect(%this, %client);
   
   %this.removeClient(%client);
}