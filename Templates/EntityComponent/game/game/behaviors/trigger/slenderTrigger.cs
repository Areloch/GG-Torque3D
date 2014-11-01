//-----------------------------------------------------------------------------
// Torque Game Builder
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

singleton VisibilityTriggerComponent(SlenderTrigger)
{
	friendlyName = "Slender Trigger";
};

function SlenderTrigger::onAdd(%this) 
{
   Parent::onBehaviorAdd(%this);

   %clientCount = ClientGroup.getCount();

   for(%i=0; %i < %clientCount; %i++)
   {
	   %this.addClient(ClientGroup.getObject(%i));
   }
}

function SlenderTrigger::onClientConnect(%this, %client)
{
   %this.addClient(%client);
}

function SlenderTrigger::onClientDisconnect(%this, %client)
{
   Parent::onClientDisconnect(%this, %client);
   
   %this.removeClient(%client);
}

function SlenderTrigger::onEnterViewCmd(%this, %cameraEntity, %firstTimeSeen)
{
   echo("AIIIIEEEEEEE");
}

function SlenderTrigger::onExitViewCmd(%this, %cameraEntity)
{
   //get the vector between us
   %pos = %this.owner.position;

   %vector = VectorSub(%pos, %cameraEntity.getPosition());

   %distance = VectorLen(%vector);

   %vector = VectorNormalize(%vector);

   //now, get the position along it
   %offset = VectorScale(%vector, %distance*0.2);

   %newPos = VectorSub(%pos, %offset);

   %this.owner.position = %newPos;
}