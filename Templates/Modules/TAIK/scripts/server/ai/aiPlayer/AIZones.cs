//-----------------------------------------------------------------------------
// Tactical AI Kit
// Copyright (C) Bryce Carrington
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// AIZone.cs
// Contains trigger datablocks and methods for AI zones
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
datablock TriggerData(AIZone)
{
   // The period is value is used to control how often the console
   // onTriggerTick callback is called while there are any objects
   // in the trigger.  The default value is 100 MS.
   tickPeriodMS = 100;
};

//-----------------------------------------------------------------------------
function AIZone::onEnterTrigger(%this,%trigger,%obj)
{
   // This method is called whenever an object enters the %trigger
   // area, the object is passed as %obj.  The default onEnterTrigger
   // method (in the C++ code) invokes the ::onTrigger(%trigger,1) method on
   // every object (whatever it's type) in the same group as the trigger.
   Parent::onEnterTrigger(%this,%trigger,%obj);
   
   if (%obj.getClassName() $= "Player" || %obj.getClassName() $= "AIPlayer")
   {
      %lastZone = %obj.curAIZone;
      
      %obj.lastAIZone = %lastZone;
      error(%obj @ " has entered zone " @ %trigger.getName());
      %obj.curAIZone = %trigger;
   }
}

function AIZone::onLeaveTrigger(%this,%trigger,%obj)
{
   // This method is called whenever an object leaves the %trigger
   // area, the object is passed as %obj.  The default onLeaveTrigger
   // method (in the C++ code) invokes the ::onTrigger(%trigger,0) method on
   // every object (whatever it's type) in the same group as the trigger.
   Parent::onLeaveTrigger(%this,%trigger,%obj);
   
   if (%obj.getClassName() $= "Player" || %obj.getClassName() $= "AIPlayer")
   {
      %curZone = %obj.curAIZone;
      
      // If we are leaving our current zone, we have not activated a newer one.
      // In this case, we are a member of the last zone we were in to avoid confusing
      //   the AI manager.
      if (%trigger == %curZone)
      {
         if (%obj.lastAIZone)
         {
            error(%obj @ " has left their current zone " @ %trigger.getName() @ ", setting them to " @ %obj.lastAIZone.getName());
            %lastZone = %obj.curAIZone;
            %obj.curAIZone = %obj.lastAIZone;
            %obj.lastZone = %lastZone;
         }
      }
      warn(%obj @ " has left zone " @ %trigger.getName());
      //%obj.curAIZone = 0;
   }
}

//-----------------------------------------------------------------------------
// Functions
//-----------------------------------------------------------------------------

// Names the zones
function setUpZones()
{
  if (!isObject(AIZones)) // Make sure the AIZones group exists
  {
    error("Add AIZones group first!");
    return;
  }
  if (AIZones.getCount() < 1) // Make sure there are triggers in the group
  {
    warn("Add triggers to AIZones first.");
    return;
  }
  
  // Name the zones
  for (%i = 0;%i < AIZones.getCount();%i++)
  {
    %zone = AIZones.getObject(%i);
    %zone.setName("Zone_" @ %i);
  }
}

// Set zones in a linear map to be adjacent to their two neighbors
function linearZones()
{
  setUpZones(); // For ease of use
  
  // Iterate through the AIZones list and take care of business
  for (%i = 0;%i < AIZones.getCount(); %i++)
  {
    %curZone = AIZones.getObject(%i);
    %neighbor1 = AIZones.getObject(%i - 1); // The zone behind this zone
    %neighbor2 = AIZones.getObject(%i + 1); // The zone in front of this zone
    if (isObject(%neighbor1))
      %n1 = %neighbor1.getName();
    else
      %n1 = "";
      
    if (isObject(%neighbor2))
      %n2 = %neighbor2.getName();
    else
      %n2 = "";
    
    %curZone.adjacentZones = %n1 SPC %n2;
    warn(%curZone.getName() @ ".AdjacentZones = " @ %curZone.adjacentZones);
  }
}

// Does vertical adjacency. This is temporary, so I should get rid of this.
function downAdj(%zone)
{
  %ray = ContainerRayCast(%zone.getWorldBoxCenter(),VectorAdd(%zone.getWorldboxCenter(),"0 0 -200"),$TypeMasks::TriggerObjectType,%zone);
  if (%ray)
  {
    %obj = getWord(%ray,0);
    %objN = %obj.getName();
    %zone.adjacentZones = %zone.adjacentZones SPC %objN;
    %obj.adjacentZones = %obj.adjacentZones SPC %zone.getName();
    error(%objN @ " is now adjacent to " @ %zone.getName());
  }
  else
  {
    error("Fail :( ");
  }
}

// Resets zone adjacency
function resetZones()
{
  for(%i = 0;%i < AIZones.getCount();%i++)
  {
    %zone = AIZones.getObject(%i);
    echo("Resetting zone " @ %zone);

    %zone.adjacentZones = "";
  }
  error("All " @ %i @ " zones successfully reset.");
}

// Hides all zones
function hideAIZones()
{
  for(%i = 0;%i < AIZones.getCount();%i++)
  {
    %zone = AIZones.getObject(%i);
    
    if (!%zone.isHidden)
    {
      // Move them way up in the sky so they're out of the way of the mission
      %newPos = VectorAdd(%zone.getPosition(),"0 0 10000");
      %zone.setTransform(%newPos);
      %zone.isHidden = true;
    }
  }
}

// Unhides all zones
function unhideAIZones()
{
  for(%i = 0;%i < AIZones.getCount();%i++)
  {
    %zone = AIZones.getObject(%i);
    
    if (%zone.isHidden == true)
    {
      // Move them back down (see the above function)
      %newPos = VectorAdd(%zone.getPosition(),"0 0 -10000");
      %zone.setTransform(%newPos);
      %zone.isHidden = false;
    }
  }
}