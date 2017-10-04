//-----------------------------------------------------------------------------
// Tactical AI Kit
// Copyright (C) Bryce Carrington
//-----------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Key bindings
//------------------------------------------------------------------------------

// Forward slash ("/") drops spawn points
MoveMap.bindCmd(keyboard, "/", "commandToServer(\'DropSpawn\');", "");

//------------------------------------------------------------------------------
// Functions
//------------------------------------------------------------------------------

// Creates a new spawn node for team $spawnTeam
function ServerCmdDropSpawn(%client)
{
  if ($spawnTeam $= "")
    %team = 0;
  else
    %team = $spawnteam;

   if (!isObject(Spawn1))
   {
     new SimGroup(Spawn1) {};
     MissionGroup.add(Spawn1);
   }
   if (!isObject(Spawn2))
   {
     new SimGroup(Spawn2) {};
     MissionGroup.add(Spawn2);
   }
  
   %position=VectorAdd(%client.player.position, "0 0 0.1");
   %node = new StaticShape() 
   {
      datablock = "SpawnMarker";
      position = %position;
   };
   %node.setTransform(%client.player.getTransform());
   if (%team == 0)
     Spawn1.add(%node);
   else
     Spawn2.add(%node);

   if ($markerAddMoveDest !$= "")
     %node.moveDestination = $markerAddMoveDest;
   if ($markerAddMoveDest !$= "")
     %node.moveDestination = $markerAddMoveDest;
   if ($markerAddPath !$= "")
     %node.path = $markerAddPath;
   if ($markeraddrespawncount !$= "")
     %node.numRespawns = $markeraddrespawncount;
   if ($markeraddspawnInc !$= "")
     %node.spawnInc = $markeraddspawnInc;
   if ($markerAddHighPriority == true)
     %node.highPriorityMove = true;
     
   $lastSpawnNode = %node;
   schedule(500,0,setLastSpawnText);
}

// Sets up all mission spawn points
function setUpSpawnPoints()
{
  %set1 = nameToId("MissionGroup/Spawn1");
  %set1Count = %set1.getCount();
  %set2 = nameToId("MissionGroup/Spawn2");
  %set2Count = %set2.getCount();

  // Set up the markers for the first set
  for (%i = 0; %i < %set1Count; %i++)
  {
    %marker = %set1.getObject(%i);
    %marker.setName(%i);
    if (%marker.moveDestination $= "")
    {
      %marker.moveDestination = %marker.position; // The map developer can set this to something else from the editor
    }
    if (%marker.numRespawns $= "")
    {
      %marker.respawns = 0;
      %marker.numRespawns = 0;
    }
  }
  // Set up the markers for the second set
  for (%i = 0; %i < %set2Count; %i++)
  {
    %marker = %set2.getObject(%i);
    %marker.setName(%i);
    if (%marker.moveDestination $= "")
    {
      %marker.moveDestination = %marker.position; // The map developer can set this to something else if he chooses.
        continue;
    }
    if (%marker.numRespawns $= "")
    {
      %marker.respawns = 0;
      %marker.numRespawns = 0;
    }
  }
}

// Resets all saved spawn point fields
function resetSpawnPoints()
{
  %set1 = nameToId("MissionGroup/Spawn1");
  %set1Count = %set1.getCount();
  %set2 = nameToId("MissionGroup/Spawn2");
  %set2Count = %set2.getCount();

  for (%i = 0; %i < %set1Count; %i++)
  {
    %marker = %set1.getObject(%i);
    if (%marker.moveDestination !$= "")
    {
      %marker.moveDestination = "";
      %marker.respawns = 0;
      %marker.numRespawns = 0;
      %marker.setName("");
        continue;
    }
    else
    {

    }
    if (%marker.numRespawns !$= "")
    {
      %marker.respawns = 0;
      %marker.numRespawns = 0;
    }
  }
  for (%i = 0; %i < %set2Count; %i++)
  {
    %marker = %set2.getObject(%i);
    if (%marker.moveDestination !$= "")
    {
      %marker.moveDestination = "";
      %marker.setName("");
        continue;
    }
    else
    {

    }
    if (%marker.numRespawns !$= "")
    {
      %marker.respawns = 0;
      %marker.numRespawns = 0;
    }
  }
}

// Resets the respawn count for each spawn marker
function resetRespawns()
{
  %set1 = nameToId("MissionGroup/Spawn1");
  %set1Count = %set1.getCount();
  %set2 = nameToId("MissionGroup/Spawn2");
  %set2Count = %set2.getCount();

  for (%i = 0; %i < %set1Count; %i++)
  {
    %marker = %set1.getObject(%i);
    %marker.respawns = 0;
  }
  for (%i = 0; %i < %set2Count; %i++)
  {
    %marker = %set2.getObject(%i);
    %marker.respawns = 0;
  }
}

// Disables all respawns for each spawn marker
function disableRespawns()
{
  %set1 = nameToId("MissionGroup/Spawn1");
  %set1Count = %set1.getCount();
  %set2 = nameToId("MissionGroup/Spawn2");
  %set2Count = %set2.getCount();

  for (%i = 0; %i < %set1Count; %i++)
  {
    %marker = %set1.getObject(%i);
    %marker.respawns = 100;
  }
  for (%i = 0; %i < %set2Count; %i++)
  {
    %marker = %set2.getObject(%i);
    %marker.respawns = 100;
  }
}

//------------------------------------------------------------------------------
// Ease of use functions
//------------------------------------------------------------------------------

function RenameSpawns()
{
  Path1.setName("Spawn1");
  Path2.setName("Spawn2");
}

function CleanUpOldPaths()
{
  Paths.delete();
}

// Replaces all path markers in the mission with the StaticShape markers
function ReplaceSpawnPoints()
{
  RenameSpawns();
  
  %p1Count = Spawn1.getCount();
  %p2Count = Spawn2.getCount();
  
  %newSpawn1 = new SimGroup() {};
  %newSpawn2 = new SimGroup() {};

  for (%i = 0; %i < Spawn1.getCount(); %i++)
  {
    %marker = Spawn1.getObject(%i);
    error("Marker " @ %marker @ ", index " @ %i);
    if (!isObject(%marker))
      break;
    %pos = %marker.getPosition();
    %rot = getWords(%marker.getTransform(),3,6);
    %mDest = %marker.moveDestination;
    %r = %marker.respawns;
    %nR = %marker.numRespawns;
    %sI = %marker.spawnInc;
    %n = %marker.getName();
    
    // Replace the path marker with a static shape
    %newMarker = new StaticShape(%n) {
      datablock = "SpawnMarker";
      position = %pos;
      rotation = %rot;
      moveDestination = %mDest;
      respawns = %r;
      numRespawns = %nR;
      spawnInc = %sI;
    };
    %newMarker.setTransform(%marker.getTransform());
    %newSpawn1.add(%newMarker);
  }
  
  // Now for Spawn2
  for (%i2 = 0; %i2 < Spawn2.getCount(); %i2++)
  {
    %marker = Spawn2.getObject(%i2);
    error("Marker " @ %marker @ ", index " @ %i2);
    if (!isObject(%marker))
      break;
    %pos = %marker.getPosition();
    %rot = getWords(%marker.getTransform(),3,6);
    %mDest = %marker.moveDestination;
    %r = %marker.respawns;
    %nR = %marker.numRespawns;
    %sI = %marker.spawnInc;
    %n = %marker.getName();
    
    // Replace the path marker with a static shape
    %newMarker = new StaticShape(%n) {
      datablock = "SpawnMarker";
      position = %pos;
      rotation = %rot;
      moveDestination = %mDest;
      respawns = %r;
      numRespawns = %nR;
      spawnInc = %sI;
    };
    %newMarker.setTransform(%marker.getTransform());
    %newSpawn2.add(%newMarker);
  }
  schedule(2000,0,cleanUpOldPaths);
  %newSpawn1.setName("Spawn1");
  %newSpawn2.setName("Spawn2");
}

// Sets each spawn marker to not have move destinations
function resetMoveDestinations(%spawnTeam)
{
  %group = "Spawn" @ %spawnTeam;
  for (%i = 0;%i<%group.getCount();%i++)
  {
    %obj = %group.getObject(%i);
    %obj.moveDestination = %obj.getPosition();
  }
}

// Adds a number of spawn points on random path nodes for Spawn1
function addSpawnPoints(%num)
{
  for (%i = 0;%i < %num;%i++)
  {
    %aPCount = AIPaths.getCount();
    %rand = getRandom(0,%aPCount);
    %obj = AIPaths.getObject(%rand);
    
    %marker = new StaticShape(%i) {
      datablock = "SpawnMarker";
      position = %obj.getPosition();
    };
    Spawn1.add(%marker);
  }
  SetUpSpawnPoints();
  echo("Successfully placed " @ %num @ " spawn points");
}
