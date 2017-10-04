//-----------------------------------------------------------------------------
// Tactical AI Kit
// Copyright (C) Bryce Carrington
//-----------------------------------------------------------------------------

//------------------------------------------------------------------------------------------------
// AiManager.cs
// Code for the the management of NPCS for performance.
//   The AIManager controls which NPCs in a mission are active at any given time.
// This is most effective in single-player missions, where many NPCs can 
//   significantly slow down game performance.
//------------------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------------------
// Global Variables
//------------------------------------------------------------------------------------------------

// AiManager variables
$AIManager::ThinkDelay = 1000; // Think once every second
$AIManager::CanSleepSquads = false; // If we allow the human player's squad to sleep
$AIManager::ScanDistance = 400; // Look for enemies in this radius
$AIManager::SleepDistance = 150; // NPCs farther than this from their nearest enemy will sleep (if no LOS)

// WanderManager variables
$WanderManager::ThinkDelay = 500;
$WanderManager::NumAllowedPerTick = 2; // Allow this many NPCs to wander per WanderManager think tick

//------------------------------------------------------------------------------------------------
// Think function, which checks up on AI Players
//------------------------------------------------------------------------------------------------

function AIManagerThink()
{
  if ($noAIManage != true)
  {
    if (isObject(AIZones))
      doThinkZoned();
    else
      doThinkBasic();
  }

  $aiManagerSched = schedule($AIManager::ThinkDelay,0,AIManagerThink);
}

//------------------------------------------------------------------------------------------------
// The think function for missions that use AI zones to figure out which NPCs should sleep.
//------------------------------------------------------------------------------------------------
function doThinkZoned()
{
  // See if the AIZones group exists, and that it's not empty
  if (!isObject(AIZones))
  {
    error("AIMANAGER--No AIZones group!");
      return;
  }
  if (AIZones.getCount() < 1)
  {
    warn("AIMANAGER--No triggers in the AIZones group!");
      return;
  }

  // For every human player
  for (%cl = 0;%cl < ClientGroup.getCount();%cl++)
  {
    %humanPlayer = ClientGroup.getObject(%cl).player;
    %hZone = %humanPlayer.curAIZone;
  
    // Figure out which zone the client's player (human-controlled) is standing in
    //%humanPlayer = LocalClientConnection.Player;
    //%hZone = %humanPlayer.curAIZone;
    
    // See if any AI Players are standing in the same zone, or specified adjacent zones.
    // To do this, we iterate through each living AI Player and check to see if he is
    //   within any of these zones. If he is, we keep him awake.
    %pArrayCount = PlayerArray.count();
    for (%ai = 0;%ai < %pArrayCount;%ai++)
    {
      %plr = PlayerArray.getKey(%ai);
      // Check to see if %plr is an AI Player
      if (%plr.client)
        continue; // If not an AI Player or dead, check the next PlayerArray object.
  
      // Otherwise, carry on.

      // Now, we figure out which zones are set as adjacent to the player's zone, and
      //   we see if %plr is in any of them. If so, we wake them up.
      %checkZones = %hZone.adjacentZones; // Make a space-delimited list of the zones to check
      %checkZones = %checkZones SPC %hZone.getName(); // Include the zone the human player is in
      $AIManager::ActiveZones = %checkZones;
      
      // Check to see if the current AI Player is in any of these zones
      %plrZone = %plr.curAIZone; // The zone the current AI Player is standing in
      %shouldSleep = true;
      for (%word = 0;%word < getWordCount(%checkZones);%word++)
      {
        %zone = getWord(%checkZones,%word);
        if (!isObject(%zone))
          continue; // If the zone doesn't exist, look at the next

        %zone = %zone.getId();
        if (%zone.getName() $= %plrZone.getName())
        {
          %hasZone = true;
          %shouldSleep = false; // The AI Player is in one of the zones, so he should be awake.
        }
      }
      // If specified, don't set NPCs with squads to sleep
      if ($AIManager::CanSleepSquads == false && %plr.squad == %humanPlayer.squad && %plr.squad !$= "")
        %shouldSleep = false;
      if (!%plrZone) // If the AI Player is not in any zone, he should not sleep
      {
        %shouldSleep = false;
      }
      if (!%hZone) // If the human player doesn't have a zone, AI should sleep
      {
        %shouldSleep = true;
      }
  
      // Now, either sleep the NPC or wake it up
      if (%shouldSleep == true)
      {
        //%plr.setSleeping(true);
        if (%plr.manageDoSleep $= "")
          %plr.manageDoSleep = true;
      }
      else
      {
        //%plr.setSleeping(false);
        %plr.manageDoSleep = false;
      }
    }
  }
  
  // Sleep them, or wake them up
  for (%sl = 0;%sl < PlayerArray.count();%sl++)
  {
    %aiDecide = PlayerArray.getKey(%sl);
    if (!%aiDecide.client) // Make sure this is an NPC
    {
      if (%aiDecide.manageDoSleep == true)
      {
        %aiDecide.setSleeping(true);
      }
      if (%aiDecide.manageDoSleep == false)
      {
        %aiDecide.setSleeping(false);
      }
        
      %aiDecide.manageDoSleep = ""; // So we can set it again the next zone tick
    }
  }
}

//------------------------------------------------------------------------------------------------
// The think function for sleeping AI Players based on their distances from human players
//------------------------------------------------------------------------------------------------
function doThinkBasic()
{
  if (!isObject(PlayerArray))
    return;

  // For every human player
  for (%cl = 0;%cl < ClientGroup.getCount();%cl++)
  {
    %humanPlayer = ClientGroup.getObject(%cl).player;
    %hZone = %humanPlayer.curAIZone;
  
    // Figure out which zone the client's player (human-controlled) is standing in
    //%humanPlayer = LocalClientConnection.Player;
    //%hZone = %humanPlayer.curAIZone;
    
    // See if any AI Players are standing in the same zone, or specified adjacent zones.
    // To do this, we iterate through each living AI Player and check to see if he is
    //   within any of these zones. If he is, we keep him awake.
    %pArrayCount = PlayerArray.count();
    %sleepNum = 0;
    for (%ai = 0;%ai < %pArrayCount;%ai++)
    {
      %plr = PlayerArray.getKey(%ai);
      // Check to see if %plr is an AI Player
      if (%plr.client)
        continue; // If not an AI Player or dead, check the next PlayerArray object.
  
      // Otherwise, carry on.
      %shouldSleep = true;
      %dist = VectorDist(%plr.getPosition(),%humanPlayer.getPosition());
      if (%dist < theLevelInfo.visibleDistance / 1.25)
        %shouldSleep = false;
      
      // Now, either sleep the NPC or wake it up
      if (%shouldSleep == true)
      {
        //%plr.setSleeping(true);
        if (%plr.manageDoSleep $= "")
          %plr.manageDoSleep = true;
          
        %sleepNum++;
      }
      else
      {
        //%plr.setSleeping(false);
        %plr.manageDoSleep = false;
      }
    }
  }
  
  // Sleep them, or wake them up
  for (%sl = 0;%sl < PlayerArray.count();%sl++)
  {
    %aiDecide = PlayerArray.getKey(%sl);
    if (!%aiDecide.client) // Make sure this is an NPC
    {
      if (%aiDecide.manageDoSleep == true)
      {
        %aiDecide.setSleeping(true);
      }
      if (%aiDecide.manageDoSleep == false)
      {
        %aiDecide.setSleeping(false);
      }
        
      %aiDecide.manageDoSleep = ""; // So we can set it again the next zone tick
    }
  }
  
  //echo("Sleeping " @ %sleepNum);
}
  

//------------------------------------------------------------------------------------------------
// The WanderManager controls how many NPCs are allowed to wander at a time.
// Basically, they wait in line for their turn to wander. This prevents everyone
//  from doing a performance-costing function at the same time.
//------------------------------------------------------------------------------------------------
function WanderManagerThink()
{
  // Make sure that a mission is running and the WanderManager exists
  if (isObject(MissionGroup) && isObject(WanderArray))
  {
    %objectsPermitted = 0;
    // Look through the wander array and allow the first few on the list to wander
    %count = WanderArray.count();
    for(%i = 0;%i < %count;%i++)
    {
      // If we have allowed enough players to wander, quit and we'll process more
      //   players on the next think tick
      if (%objectsPermitted == $WanderManager::NumAllowedPerTick)
        break;
  
      %plr = WanderArray.getKey(%plr);
      // Make sure this player is an awake, living, existing AI Player
      if (!isObject(%plr) || %plr.getState() $= "Dead" || %plr.client || %plr.isAsleep)
      {
        %index = WanderArray.getIndexFromKey(%plr);
        WanderArray.erase(%index);
        continue; // Go to the next player on the list
      }
  
      %index = WanderArray.getIndexFromKey(%plr);
      %string = WanderArray.getValue(%index);
      %minDist = getWord(%string,0);
      %maxDist = getWord(%string,1);
      %avoidPos = getWords(%string,2,4);
      %fromPos = getWords(%string,5,7);
      
      if (%avoidPos !$= "0 0 0") // If we have an avoidPos, the NPC wants to run away
      {
        %plr.runAwayFrom(%minDist,%maxDist,%avoidPos);
        error(%plr @ ": running away");
      }
      else // Otherwise, the NPC wants to wander
      {
        if (%fromPos !$= "0 0 0")
        {
          %plr.wander(%minDist,%maxDist,%fromPos);
        }
        else
        {
          %plr.wander(%minDist,%maxDist);
        }
      }
  
      WanderArray.erase(%index);
      %objectsPermitted++;
    }
  }

  // Schedule the next think tick
  schedule($WanderManager::ThinkDelay,0,WanderManagerThink);
}

//------------------------------------------------------------------------------------------------
// Helper functions
//------------------------------------------------------------------------------------------------

// Adds a wander request for the WanderManager to process
function AIPlayer::RequestWander(%this,%minDist,%maxDist,%avoidPos,%immediate,%fromPos)
{
  // Make sure we are not already on the wander list
  if (WanderArray.countKey(%this) > 0)
  {
    return;
  }
  // Make sure we are not prohibited from wandering
  if (%this.brain.canWander == false)
  {
    return;
  }
  // If this is an immediate request, just carry it out.
  if (%immediate == true)
  {
    if (%avoidPos)
    {
      %this.runAwayFrom(%minDist,%maxDist,%avoidPos);
      echo(%this @ ": immediate run-away request");
    }
    else
    {
      %this.wander(%minDist,%maxDist,%fromPos);
      echo(%this @ ": immediate wander request");
    }
    return;
  }
  
  if (%avoidPos == 0 || %avoidPos $= "")
    %avoidPos = "0 0 0";
  if (%fromPos == 0 || %fromPos $= "")
    %fromPos = "0 0 0";
    
  // Create the value string that provides information about how we want to
  // wander.
  // --Format: minimumDistance SPC maximumDistance SPC positionToAvoid SPC fromPosition
  %string = %minDist SPC %maxDist SPC %avoidPos SPC %fromPos;
  WanderArray.add(%this,%string); // Add ourself to the array and wait our turn.
}

// Prints to the console the number of awake NPCs
function NPCsAwake()
{
  %count = 0;
  %awakeArray = new ArrayObject(); MissionCleanup.add(%awakeArray);
  for(%i=0;%i<PlayerArray.count();%i++)
  {
    %obj = PlayerArray.getKey(%i);
    if (%obj.isAsleep == false && %obj.getState() !$= "Dead" && %obj.brain)
    {
      %count++;
      %awakeArray.add(%obj,%i);
    }
  }
  
  %awakeArray.echo();
  error("Awake NPCs: " @ %count @ " / " @ PlayerArray.count() - 1);
}

// Prints to the console the number of NPCs that are alive, awake or not
function NPCsAlive()
{
  %count = 0;
  %aliveArray = new ArrayObject(); MissionCleanup.add(%aliveArray);
  for(%i=0;%i<PlayerArray.count();%i++)
  {
    %obj = PlayerArray.getKey(%i);
    if (%obj.getState() !$= "Dead" && %obj.brain)
    {
      %count++;
      %aliveArray.add(%obj,%i);
    }
  }
  
  %aliveArray.echo();
  error("Living NPCs: " @ %count @ " / " @ PlayerArray.count() - 1);
}