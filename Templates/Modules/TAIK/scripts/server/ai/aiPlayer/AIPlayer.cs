//-----------------------------------------------------------------------------
// Tactical AI Kit
// Copyright (C) Bryce Carrington
//-----------------------------------------------------------------------------

//------------------------------------------------------------------------------------------------
// AiPlayer.cs
// Basic Functions; Datablock definitions, firing, thinking, callbacks, etc
// Applies to all AI Players
//------------------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------------------
// Global variables
//------------------------------------------------------------------------------------------------

// Thinking
$AIPlayer::ThinkDelay = 400;
$AIPlayer::AsleepDelay = 5000; // Delay between thinking when asleep

// Quick cover search
$AIPlayer::FindQuickCoverDot = 0.7;

// Wandering, running away
$AIPlayer::RunAwayDot = -0.05;

// Firing
$AIPlayer::SuppressEnemyMaxDist = 8; // Suppressing fire should land farther than this from our target, or something is in the way
$AIPlayer::SuppressSelfMaxDist = 8; // Suppressing fire should not land closer than this to ourself
$AIPlayer::CloseShotDist = 1.5; // If we are firing, make sure the bullet does not hit an object closer than this to ourself
$AIPlayer::FireSafeDot = 0.97; // To determine if we are correctly aimed at a regular target
$AIPlayer::FirePreciseDot = 0.995; // To determine if we are correctly aimed when our weapon requires precision before firing for safety (e.g. a rocket launcher)
$AIPlayer::FirePreciseMaxSpeed = 0.1; // If our weapon requires precision (see above), don't fire if we're moving faster than this
$AIPlayer::FireSafeCivilianDot = 0.6; // To determine if we are correctly aimed at a civilian target
$AIPlayer::FireSafeMovingDot = 0.6; // To determine if we are correctly aimed at a target while we are moving

// Throwing grenades
$AIPlayer::GrenadeThrowSpeed = 35;

// Hearing
$AIPlayer::AudFireRange = 500; // Distance that weapon images will look for AI Players upon firing

// Spawning
$AIPlayer::SpawnDelay = 500; // Delay between respawning more than one NPC at a time

// Typemasks for easy access in raycasts
$AIPlayer::SolidMasks = $TypeMasks::InteriorObjectType |
                      $TypeMasks::TerrainObjectType |
                      $TypeMasks::StaticTSObjectType |
                      $TypeMasks::ConvexShapeObjectType |
                      $TypeMasks::StaticShapeObjectType |
                      $TypeMasks::WaterObjectType;

$AIPlayer::CoverMasks = $TypeMasks::InteriorObjectType |
                      $TypeMasks::StaticTSObjectType |
                      $TypeMasks::ConvexShapeObjectType |
                      $TypeMasks::StaticShapeObjectType |
                      $TypeMasks::VehicleObjectType;
                      
$AIPlayer::ProjectileMasks = $TypeMasks::InteriorObjectType |
                      $TypeMasks::TerrainObjectType |
                      $TypeMasks::StaticTSObjectType |
                      $TypeMasks::ConvexShapeObjectType |
                      $TypeMasks::PlayerObjectType |
                      $TypeMasks::StaticShapeObjectType |
                      $TypeMasks::VehicleObjectType;

$AIPlayer::OpaqueMasks = $TypeMasks::InteriorObjectType |
                       $TypeMasks::TerrainObjectType |
                       $TypeMasks::ConvexShapeObjectType |
                       $TypeMasks::StaticTSObjectType |
                       $TypeMasks::StaticShapeObjectType |
                       $TypeMasks::VehicleObjectType |
                       $TypeMasks::WaterObjectType;

//------------------------------------------------------------------------------------------------
// Scripting functions
//------------------------------------------------------------------------------------------------

// for easy ai script compilation
function execAI()
{
  // Exec's all AI functions
  exec("./Brain1.cs");
  
  exec("./aiPaths.cs");
  exec("./squad1.cs");
  exec("./aiSpawn.cs");
  exec("./aiZones.cs");
  exec("./aiManager.cs");
  exec("./aimPatterns.cs");
  
  new ArrayObject(WanderArray){};
  new ArrayObject(PlayerArray){};
}

// Spawns enemy AI Players at every spawn point in the mission
function fillWithEnemies()
{
  SpawnMultipleAI(0,0,Spawn1.getCount(),2);
}

// starts an endless AI match
function AIMatch(%enemyType)
{
  if (%enemyType $= "")
    %enemyType = 3;
  
  $endlessai=1;
  $noaimanage=1;
  $corpsetimeoutvalue=10000;
  spawnmultipleai(1,0,6,1,1);
  spawnmultipleai(0,0,6,%enemyType,1);
}

// resets an AI match
function resetAIMatch()
{
  $endlessai=0;
  $noaimanage=0;
  $corpsetimeoutvalue = 22*1000;
}

// Creates a test NPC hooked up to global variable $A
function testAI()
{
  $a.delete();
  $a = spawnai(0,0,2);
}

// kills all NPCs
//  if %doDelete is set to true, they are deleted.
function killAll(%doDelete)
{
  for($i=0;$i<playerarray.count();$i++) 
  { 
    $obj = playerarray.getkey($i); 
    if (!$obj.client) 
    {
      if (!%doDelete)
        $obj.kill();
      else
        $obj.schedule(500,delete);
    }
  }
}

//------------------------------------------------------------------------------------------------
// Spawn functions
//------------------------------------------------------------------------------------------------

function AIPlayer::spawn(%name, %spawnPoint, %path, %block)
{
  // Create the demo player object
  %player = new AIPlayer() {
    dataBlock = %block; // Uses datablock set by %block
    team = 0; // No team yet
    mountVehicle = 0; // Cannot mount vehicles
    
    isCivilian = 0;
    
    aimPatternType = -1;
    aimPatternIndex = 0;
    targetPathDestination = 0;
  };
  
  %player.patharray = new ArrayObject();
  %player.pathNodeArray = new ArrayObject();
  %player.enemyArray = new ArrayObject();
  
  MissionCleanup.add(%player);
  MissionCleanup.add(%player.patharray);
  MissionCleanup.add(%player.pathNodeArray);
  MissionCleanup.add(%player.enemyArray);

  %player.setTransform(%spawnPoint);
  //%player.rotation = getWords(%spawnPoint,3,6);
  %player.setSquadWait(true);
  return %player;
}

function AIPlayer::spawnOnPath(%name,%path,%index,%block,%marker)
{
  if (!isObject(%path))
    return;
  
  if(%index $= "" || %index < 0 || %index >= %path.getCount() )
    %index = mFloor(getRandom(0.1, %path.getCount() - 0.1));
  
  %node = %path.getObject(%index);
  %player = AIPlayer::spawn(%name, %node.getTransform(), %path, %block);

  %player.spawnNode = %node;
  %player.spawnNodeIdx = %index;

  return %player;
}

// Respawn an AI Player
function AIPlayer::Respawn(%this,%markerIdx,%marker,%aiType,%noVisCheck,%alert,%threatType,%threatPos)
{
  InitContainerRadiusSearch(%marker.getPosition(),400,$TypeMasks::PlayerObjectType);

  while((%obj = ContainerSearchNext()) != 0)
  {
    if (%obj.team == %this.team)
      continue;

    // Two things can prevent an AI player from respawning:
    //  The first thing is if the marker has line of sight to an opposing team member. They shouldn't see the respawn.
    //  The second thing is if the last AI player was killed while standing on their marker. This could suggest that they were killed by
    // a grenade or other explosive; if this happens, the killer should be able to assume that area is "flushed out", and shouldn't
    // have any respawners sneaking up on them.
    if (VectorDist(%this.getPosition(),%marker.getPosition()) < 1 && !%noVisCheck)
    {
      error("Can't respawn at " @ %marker @ ", area flushed out");
      %marker.respawns = %marker.numRespawns; // Area is flushed out, don't respawn there
        return;
    }
    
    %ray = ContainerRayCast(%marker.getPosition(),%obj.getEyePoint(),$AIPlayer::OpaqueMasks);
    if (!%ray && !%noVisCheck)
    {
      error(%obj @ " sees marker " @ %marker @ ", disabling respawns");
      %marker.respawns = %marker.numRespawns; // A player on another team has seen this marker, don't respawn here anymore
        return;
    }
  }
  if (%marker.spawnInc $= "" || !%marker.spawnInc)
  {
    %spawnInc = 1;
  }
  else
  {
    %spawnInc = %marker.spawnInc;
  }
  %spawnTime = 500;
  for (%spawnIndex = 0;%spawnIndex < %spawnInc;%spawnIndex++)
  {
    echo("SPAWN: " @ %spawnIndex @ "; TIME: " @ %spawnTime);
    schedule(%spawnTime,0,spawnai,%this.team,%markerIdx,%aiType,%alert,%threatType,%threatPos);
    %spawnTime += $AIPlayer::SpawnDelay;
  }
}

// Spawns the AIPlayers
//   The last three arguments (alert, threatType, threatPos) are optional
function spawnAI(%team,%marker,%aiType,%alert,%threatType,%threatPos)
{
   switch$(%team)
   {
      case 0:
        %path = "MissionGroup/Spawn1";

      case 1:
        if (isObject(Spawn2) && Spawn2.getCount() > 0)
          %path = "MissionGroup/Spawn2";
        else
          %path = "MissionGroup/Spawn1";
          
      case 2:
        if (isObject(Spawn2) && Spawn2.getCount() > 0)
          %path = "MissionGroup/Spawn2";
        else
          %path = "MissionGroup/Spawn1";

      default:
        %path = "MissionGroup/Spawn1";
   }
   
   // If the marker is disabled, try to spawn the AI somewhere else
   if (%path.getObject(%marker).isDisabled == true)
   {
     warn("Marker in spawn folder " @ %path.getName() @ " at index " @ %marker @ " is disabled. Spawning somewhere else.");
     schedule(1000,0,SpawnAI,%team,-1,%aiType);
     return;
   }

   switch$(%aiType)
   {
     case 1:
       %plr = AIPlayer::spawnOnPath("V",%path, %marker, "SpecOpsRifleMan");
       %brain = Brain1::create(%plr);
       %plr.team = %team;

     case 2:
       %plr = AIPlayer::spawnOnPath("V",%path, %marker, "OpForRifleman");
       %brain = Brain1::create(%plr);
       %plr.team = %team;

     default:
       %plr = AIPlayer::spawnOnPath("V",%path, %marker, "SpecOpsRifleMan");
       %brain = Brain1::create(%plr);
       %plr.team = %team;
       %aiType = 1;
   }

   // wake the bot and start thinking
   %plr.init(%brain, %team);
   %brain.aiType = %aiType;
   %plr.aiType = %aiType;
   %plr.thinkDelay = $AIPlayer::ThinkDelay;
   %plr.enteredSpawnIdx = %marker;
   %brain.wake();
   %plr.schedule(50, think);
   %plr.schedule(50, thinkExplosions);
   %plr.schedule(50, thinkAud);
   %plr.schedule(50, checkLineOfFire);
   
   %plr.setShapeName("");
   
   if (%alert !$= "" && %alert > 1 && %threatType !$= "" && %threatPos !$= "")
   {
      %brain.setAlertness(%alert);
      %brain.threatType = %threatType;
      %brain.threatPosition = %threatPos;
   }
   
   %brain.bumpOutOfCover();
   
   if (%team == 2)
     %plr.isCivilian = true;

   echo("AI player spawned. Id: " @ %plr @ ". Team: " @ %team @ ". Brain: " @ %brain);
     return %plr;
}

// Spawn multiple AI Players
function spawnMultipleAI(%team,%start,%end,%aiType,%rand)
{
  for(%i=%start;%i<%end;%i++)
  {
    if (%rand)
      spawnai(%team,-1,%aiType);
    else
      spawnai(%team,%i,%aiType);
  }
}

// Randomly spawns AI players
function randomAI(%num)
{
  for (%i=0;%i<%num;%i++)
  {
    spawnai(0,-1,3);
  }
}

//------------------------------------------------------------------------------------------------
// Initialization
//------------------------------------------------------------------------------------------------

// Initializes the AIPlayer
function AIPlayer::init(%this, %brain, %team )
{
  %this.brain = %brain;
  %this.team = %team;
  %this.noThinking = 0; // Thinking's okay!
}

//------------------------------------------------------------------------------------------------
// Navigation functions
//------------------------------------------------------------------------------------------------

function AIPlayer::SetOnPath(%this,%destination,%stealth)
{
   if (VectorDist(%destination,%this.targetPathDestination) < 1 && %this.isStopped() != true)
     return 2;
   
   //This empties the bots current path
   %this.patharray.empty();
   %this.pathNodeArray.empty();

   %this.curNode = -1;
   %this.endNode = -1;
   if (%stealth == true)
   {
     %path = AIPaths.findPath(%this.getPosition(), %destination, ASTAR, 3); // hack, add stealth pathfinding type!
   }
   else
   {
     %path = AiPaths.findpath(%this.getPosition(), %destination, ASTAR, 4);
   }
   %this.targetPathDestination = %destination;

   // If no path found
   if(%path == -1)
   {
     %this.patharray.add(%this.getPosition(),0);
     %this.endNode = -1;
     %this.curNode = -1;
     //%this.targetPathDestination = "";
       return -1;
   }
   
   if (!isObject(%this.pathArray))
     return -1;

   %count=getWordCount(%path);
   for (%i=0; %i<%count; %i++)
   {
     %this.pathArray.add(getWord(%path, %i).getPosition(), %i);
     %this.pathNodeArray.add(getWord(%path,%i),%i);
   }
   %this.patharray.add(%destination, %count);
  
   %this.endNode=%count;
   %this.curNode=0;

   // See if we can go ahead and skip to the second node on the path
   %skipRay1 = ContainerRayCast(%this.getPosition(),%this.pathArray.getKey(%this.curNode + 1),$AIPlayer::SolidMasks);
   %skipRay2 = ContainerRayCast(%this.getWorldBoxCenter(),%this.pathArray.getKey(%this.curNode + 1),$AIPlayer::SolidMasks);
   %skipRay3 = ContainerRayCast(%this.getTrueEyePoint(),%this.pathArray.getKey(%this.curNode + 1),$AIPlayer::SolidMasks);
   %clearToNode = !%skipRay1 && !%skipRay2 && !%skipRay3;
   
   %skipDist = VectorDist(%this.getPosition(),%this.pathArray.getKey(%this.curNode + 1));
   %skipHeightDifference = getWord(%this.getPosition(),2) - getWord(%this.pathArray.getKey(%this.curNode + 1),2);

   if (%clearToNode == true && %skipDist <= 10 && %skipHeightDifference < 2000) //HACK
   {
     %this.curNode++;
     %this.moveToNode(%this.curNode);
   }
   else
   {
     %this.moveToNode(%this.curNode);
   }
   return 1;
}
// Get the approximate path distance to a location
function AIPlayer::getPathDistance(%this,%position1,%position2)
{
  %path = AIPaths.findPath(%position1,%position2,ASTAR,0);
  %linearDistance = VectorDist(%position1,%position2);
  if (%path == -1)
    return 100000000000;

  %count = getWordCount(%path);
  %lastNode = firstWord(%path);
  %distance=0;
  
  for (%i=1;%i < %count;%i++)
  {
    %curNode = getWord(%path,%i);
    %distance += VectorDist(%curNode.getPosition(),%lastNode.getPosition());
    %lastNode = %curNode;
  }
  return %distance;
}

// Wander to a position between %minDist and %maxDist away
function AIPlayer::wander(%this,%minDist,%maxDist,%fromPos)
{
  if (%this.brain.canWander == false && isObject(%this.brain))
  {
    warn(%this @ ": wander disabled.");
    return;
  }
  if (%fromPos)
    %wPos = %fromPos;
  else
    %wPos = %this.getPosition();

  %randomPos = %this.getRandomPosition(%wPos,%minDist,%maxDist,0,false,true);
  if (%randomPos != -1 && getWordCount(%randomPos) == 3)
  {
    %this.setOnPath(%randomPos);
    //%this.setMoveDestination(%randomPos);
    
    //%this.setWanderAllowed(false);
    //%this.schedule(3000,setWanderAllowed,true);
  }
  else
  {
    %this.setWanderAllowed(false);
    %this.schedule(3000,setWanderAllowed,true);
  }
  return;
}

function AIPlayer::RunAwayFrom(%this,%minDist,%maxDist,%avoidPos)
{
  if (%this.brain.canWander == false && isObject(%this.brain))
  {
    warn(%this @ ": runaway disabled.");
    return;
  }
  %randomPos = %this.getRandomPosition(%this.getPosition(),%minDist,%maxDist,%avoidPos,false,true);
  if (%randomPos != -1 && getWordCount(%randomPos) == 3)
  {
    %this.setOnPath(%randomPos);
    //%this.setMoveDestination(%randomPos);
    
    //%this.setWanderAllowed(false);
    //%this.schedule(3000,setWanderAllowed,true);
  }
  else
  {
    %this.wander(%minDist,%maxDist);
    //%cover = %this.findQuickCover(%avoidPos,%maxDist,true,false);
    //if (%cover)
    //{
      //%this.setOnPath(%cover.getPosition());
    //}
  }
  return;
}

// Finds a random cover node, and returns its position
function AIPlayer::getRandomPosition(%this,%origin,%minDist,%maxDist,%avoidPos,%directOnly,%useAIPaths)
{
  if (%useAIPaths == true)
    %group = AIPaths.getId();
  else
    %group = CoverPoints.getId();

  // Iterate through %group, to find nodes between minDist and maxDist
  %startIndex = getRandom(0,%group.getCount()-10);
  for(%i = 0;%i < %group.getCount();%i++)
  {
    %loc = getRandom(0,%group.getCount() - 1);
    %node = %group.getObject(%loc);
    %dist = VectorDist(%origin,%node.getPosition());
    if (%dist > %minDist && %dist < %maxDist)
    {
      if (%avoidPos)
      {
        %vecToNode = VectorNormalize(VectorSub(%this.getPosition(),%node.getPosition())); // Get the vector to the node
        %vecToAPos = VectorNormalize(VectorSub(%this.getPosition(),%avoidPos)); // The vector to the position to avoid
        // Flatten the vectors Z-axis (vertically)
        %vecToNode = setWord(%vecToNode, 2, 0);
        %vecToAPos = setWord(%vecToAPos, 2, 0);
        %dot = VectorDot(%vecToAPos,%vecToNode);
        if (%dot < $AIPlayer::RunAwayDot)
        {
          if (%directOnly)
          {
            %h1 = getWord(%this.getPosition(),2);
            %h2 = getWord(%node.getPosition(),2);
            %hDif = mAbs(%h2 - %h1);
            if (%hDif < 2 && !ContainerRayCast(%this.getPosition(),%node.getPosition(),$AIPlayer::SolidMasks))
              return %node.getPosition();
          }
          else
            return %node.getPosition();
        }
      }
      else
      {
        if (%directOnly)
        {
          if (!ContainerRayCast(%this.getWorldBoxCenter(),%node.getPosition(),$AIPlayer::SolidMasks))
            return %node.getPosition();
        }
        else
          return %node.getPosition();
      }
      //%distArray.add(%node,%i);
      //if (%distArray.count() > 2) // Only analyze 2 nodes
        //break;
      //else
        //continue;
    }
    else
      continue;
  }
  
  // We never found anything, so return -1
  return -1;
}

// Picks a random navigation node, returns its position
function AIPlayer::getRandomNodePosition(%this,%origin,%minDist,%maxDist)
{
  // Iterate thru the AIPaths group, to find nodes between minDist and maxDist
  %distArray = new ArrayObject();
    MissionCleanup.add(%distArray);
  %startIndex = getRandom(0,AIPaths.getCount()-10);
  for(%i=%startIndex;%i<AIPaths.getCount();%i++)
  {
    %node = AIPaths.getObject(%i);
    %dist = VectorDist(%origin,%node.getPosition());
    if (%dist > %minDist && %dist < %maxDist)
    {
      %distArray.add(%node,%i);
      if (%distArray.count() > 2) // Only analyze 2 nodes
        break;
      else
        continue;
    }
    else
      continue;
  }
  if (%distArray.count() < 1)
    return;
  // Now we have an array with nodes that are within the given distance. Find a random one we can path to.
  //for(%rand=0;%rand<(%distArray.count()*1.5);%rand++)
  for(%rand=0;%rand<%distArray.count();%rand++)
  {
    %count = %distArray.count();
    %rNode = %distArray.getKey(getRandom(0,%count-1));
    %pathDist = %this.getPathDistance(%origin,%rNode.getPosition());
    if (%pathDist > 1000) // The path failed, so try another node
      continue;
    else
    {
      return %rNode.getPosition(); // The final random position
    }
  }
  
  // We never found anything, so return -1
  return -1;
}

// Find the best node to move to to attack a target
function AIPlayer::getBestAttackNode(%this,%attackPos,%attackObj,%maxSearchDist,%preferredMinDist,%preferredMaxDist)
{
  // Iterate thru the AIPaths group, to find nodes within %maxSearchDist
  %nodeArray = new ArrayObject();
    MissionCleanup.add(%nodeArray);  
  for(%i=0;%i<CoverPoints.getCount();%i++)
  {
    %node = CoverPoints.getObject(%i);
    %dist = VectorDist(%this.getPosition(),%node.getPosition());
    if (%dist < %maxSearchDist)
    {
      %nodeArray.add(%node,%i);
      %nodeArray.setValue(0,%i);
    }
  }
  if (%nodeArray.count() < 5) // HACK: if less than 5 nodes within maxsearchdist, we are isolated and should not do the work below
    return;

  // Weigh each of the nodes for being within the preferred distance
  for(%pD=0;%pD<%nodeArray.count();%pD++)
  {
    %pDNode = %nodeArray.getKey(%pD);
    %distToAO = VectorDist(%attackPos,%pDNode.position);
    if (%distToAO < %preferredMaxDist && %distToAO > %preferredMinDist)
    {
      %curCost = %nodeArray.getValue(%pD);
      %nodeArray.setValue(%curCost + 0,%pD);
    }
    else
    {
      // Not within preferred fighting range, so raise the node's cost
      %curCost = %nodeArray.getValue(%pD);
      %nodeArray.setValue(%curCost + (%distToAO*5),%pD);
    }
  }

  // Weigh each node for line of sight to %attackPos
  for (%los=0;%los<%nodeArray.count();%los++)
  {
    %losNode = %nodeArray.getKey(%los);
    %ray = ContainerRayCast(VectorAdd(AIPlayer::getGroundPos(%losNode.position),"0 0 1.5"),%attackPos,$AIPlayer::SolidMasks);
    if (!%ray)
    {
      %curCost = %nodeArray.getValue(%los);
      %nodeArray.setValue(%curCost + 0,%los);
    }
    else
    {
      // No line-of-sight, raise the cost
      %curCost = %nodeArray.getValue(%los);
      %nodeArray.setValue(%curCost + 70,%los);
    }
  }
  
  // Weigh each node based on LOS to a secondary threat
  //  We want to try to remain hidden from them while attacking the primary threat
  %secondary = %this.brain.getSecondaryThreat(%attackObj);
  if (%secondary != 0)
  {
    // Weight each node for line of sight to the secondary threat
    for (%secLos=0;%secLos<%nodeArray.count();%secLos++)
    {
      %secNode = %nodeArray.getKey(%secLos);
      %secRay = ContainerRayCast(%secondary.getWorldBoxCenter(),%secNode.getPosition(),$AIPlayer::SolidMasks);
      if (!%secRay)
      {
  %curCost = %nodeArray.getValue(%secLos);
  %nodeArray.setValue(%curCost + 40,%secLos);
      }
      else
      {
        %curCost = %nodeArray.getValue(%secLos);
        %nodeArray.setValue(%curCost + 0,%secLos);
      }
    }
  }

  // Look through our nodeArray and find the node with the lowest cost
  %lowestWeight = 1000000;
  %lowestWeightNode = -1;
  for (%best=0;%best<%nodeArray.count();%best++)
  {
    %checkNode = %nodeArray.getKey(%best);
    %cost = %nodeArray.getValue(%best);
    if (%cost <= %lowestWeight) // && %this.getPathDistance(%this.getPosition(),%checkNode.getPosition()) < 1000)
    {
      %lowestWeight = %cost;
      %lowestWeightNode = %checkNode;
    }
  }

  if (%lowestWeightNode)
  {
    return %lowestWeightNode;
  }
  else
  {
    return 0;
  }
}

// Returns the best node to throw a grenade to damage an object at position %targetPos, within radius %blastRadius
function AIPlayer::getBestGrenadeNode(%this,%targetPos,%blastRadius)
{
  // Iterate thru the AIPaths group, to find nodes within %blastRadius of %targetPos
  %nodeArray = new ArrayObject();
    MissionCleanup.add(%nodeArray);
  for(%i=0;%i<AIPaths.getCount();%i++)
  {
    %node = AIPaths.getObject(%i);
    %dist = VectorDist(%targetPos,%node.getPosition());
    if (%dist <= (%blastRadius))
    {
      %nodeArray.add(%node,%i);
      %nodeArray.setValue(%dist,%i);
    }
  }

  // Weigh each node for line of sight to %targetPos (able to damage whoever is standing there)
  for (%los=0;%los<%nodeArray.count();%los++)
  {
    %losGood = false;
    %lofGood = false;
    %losNode = %nodeArray.getKey(%los);
    %losRay = ContainerRayCast(AIPlayer::getGroundPos(%losNode.position),%targetPos,$AIPlayer::SolidMasks,%losNode);
    if (!%losRay)
    {
      %losGood = true;
    }
    else
    {
      continue;
    }

    %lofNode = %nodeArray.getKey(%lof);
    %lofRay = ContainerRayCast(AIPlayer::getGroundPos(%lofNode.position),%this.getTrueMuzzlePoint(0),$AIPlayer::SolidMasks,%lofNode);
    if (!%lofRay)
    {
      %lofGood = true;
    }
    else
    {
      continue;
    }
    
    // If this is a good node, go with it
    if (%lofGood == true && %losGood == true)
    {
      return %losNode;
    }
  }
  
  return -1;
}

function AIPlayer::followPath(%this,%path,%node)
{   
   // Start the player following a path
   %this.stopThread(0);
   if (!isObject(%path)) {
      %this.path = "";
      return;
   }
   if (%node > %path.getWordCount() - 1)
      %this.targetNode = %path.getWordCount() - 1;
   else
      %this.targetNode = %node;
   if (%this.path $= %path)
      %this.moveToNode(%this.currentNode);
   else {
      %this.path = %path;
      %this.moveToNode(0);
   }
}

function AIPlayer::moveToNextNode(%this)
{
   //G.Notman Begin
   if (isObject(AiPaths))
   {
      %this.moveToNode(%this.CurNode + 1);
      return;
   }
   //G.Notman End
   
   if (%this.targetNode < 0 || %this.currentNode < %this.targetNode) {
      if (%this.currentNode < %this.path.getCount() - 1)
         %this.moveToNode(%this.currentNode + 1);
      else
         %this.moveToNode(0);
   }
   else
      if (%this.currentNode == 0)
         %this.moveToNode(%this.path.getCount() - 1);
      else
         %this.moveToNode(%this.currentNode - 1);
}

function AIPlayer::moveToNode(%this,%index)
{
   //G.Notman Begin
   if (isObject(AiPaths))
   {
       // Make sure that this node does not lead us into an inactive zone
       //if (MissionGroup.useAIZones)
       //{
         //if (%this.isInactiveZoneNode(%this.pathNodeArray.getKey(%this.CurNode)))
         //{
           //%this.stop();
         //}
       //}
      %this.CurNode=%index;
      %pos = %this.patharray.getkey(%this.CurNode);
      if (%pos)
        %this.SetMoveDestination(%pos, false);
      return;
   }
   //G.Notman End
}

//------------------------------------------------------------------------------------------------
// Path FOLLOWING
//   This is different from path-finding navigation, these functions are for following pre-placed
// paths in the mission
//   These functions are implemented on the Brain, so these are just here to provide hooks to
// those functions so I can use the schedule function.
//------------------------------------------------------------------------------------------------

function AIPlayer::MoveToMarker(%this,%path,%useNavigation)
{
  %this.brain.moveToMarker(%path,%useNavigation);
}

function AIPlayer::MoveToNextMarker(%this,%path)
{
  %this.brain.moveToNextMarker(%path);
}

//------------------------------------------------------------------------------------------------
// Thinking
//------------------------------------------------------------------------------------------------

function AIPlayer::think(%this)
{
  if (%this.getState() !$= "Dead")
  {
    // Think
    if (!%this.noThinking && %this.brain)
    {
      %this.brain.think();
    }

    if (%this.brain) // think if we still have a brain
    {
      if (%this.isAsleep == false)
      {
        %time = %this.thinkDelay + getRandom(0,%this.thinkDelay) / 3;
        %this.brainSched = %this.schedule(%time, think);
      }
    }
    else
    {
      error(%this @ ": Brain has been erased.");
      %this.noThinking = true;
    }
  }
  else
  {
    if (!%this.brain)
      return;

    if (%this.squad && %this.memberIndex $= 0)
    {
      %this.squad.changeLeader();
      error(%this @ ": changing my squad (" @ %this.squad @ ")'s leader to the member below me");
    }
    //if (%this.isAsleep == true)
      //%this.setSleeping(false);
    %this.brain.coverMarker.isTaken = 0;
    %this.brain.coverMarker = 0;
    %this.attackObj = 0;
    %this.coverObj = 0;
    %this.state = "";

    error(%this @ " has been pwn3d");

    %marker = %this.spawnNode;
    %markerIdx = %this.spawnNodeIdx;
    error("Entered spawn idx " @ %this.enteredSpawnIdx);
    %type = %this.aiType;
    echo("marker: " @ %marker);
    echo("markeridx: " @ %markerIdx);
    echo("aiType: " @ %type);

    if ($endlessai == true && !%this.noRespawn)
    {
      if (!%this.squad)
      {
        //%this.schedule(2000,respawn,%markerIdx,%marker,%this.aiType,true);
        %this.respawn(%this.enteredSpawnIdx,%marker,%this.aiType,true,%this.brain.alertness,%this.brain.threatType,%this.brain.threatPosition);
        %this.noRespawn = true;
        error("individual respawn");
      }
      else
      {
        if (%this.squad.getLivingMemberCount() == 0 && %this.memberIndex == 0)
        {
          // Respawn the squad
          %this.squad.respawn(%markerIdx);
          warn("squad respawn");
        }
        else
        {
          return;
        }
      }
    }

    if (%marker.respawns < %marker.numRespawns && !%this.noRespawn)
    {
      %this.respawn(%markerIdx,%marker,%this.aiType,false,%this.brain.alertness,%this.brain.threatType,%this.brain.threatPosition);
        %marker.respawns++;
    }
    %this.brain = 0;
    return;
  }
}

function AIPlayer::thinkAud(%this)
{
  if (!%this.brain)
    return;
}

//------------------------------------------------------------------------------------------------
// Sleeping/Waking up
//   The AIManager uses these functions to enable/disable NPCs that should not be a part of
//     game play at a given moment and location.
//------------------------------------------------------------------------------------------------

function AIPlayer::setSleeping(%this,%bool)
{
  if (%bool == true) // Setting the NPC to sleep
  {
    %this.isAsleep = true;
    %this.setRendering(false);
    %this.lastMoveSpeed = %this.getMoveSpeed();
    %this.setMoveSpeed(0.0);
    
    cancel(%this.brainSched);
    cancel(%this.brainAudSched);
    cancel(%this.lineOfFireSched);
    //cancel(%this.hideSched);
    //%this.hideSched = %this.schedule(500,setHidden,true);

    //%this.startFade(1,0,1);
  }
  else // Waking up the NPC
  {
    %this.setRendering(true);
    if (!isEventPending(%this.brainSched) || %this.isAsleep == true) //%this.isAsleep == true)
    {
      //%this.setHidden(false);
      %this.isAsleep = false;
      cancel(%this.brainSched);
      cancel(%this.brainAudSched);
      cancel(%this.lineOfFireSched);
      //cancel(%this.hideSched);
      
      %this.brainSched = %this.schedule(500,think); // Start thinking again
      %this.brainAudSched = %this.schedule(500,thinkAud);
      %this.lineOfFireSched = %this.schedule(500,checkLineOfFire);

      if (%this.lastMoveSpeed !$= "")
        %this.setMoveSpeed(%this.lastMoveSpeed);
      else
        %this.setMoveSpeed(1.0);
    }
    //%this.startFade(1,0,0);
    %this.isAsleep = false;
  }
}

//------------------------------------------------------------------------------------------------
// Misc functions
//------------------------------------------------------------------------------------------------

// This function alerts AI Players of a dangerous event, such as an exploding projectile or grenade
//   %position is where the event takes place, radius is the distance that NPCs will be affected,
//   %stress is the amount the NPC's stress should go up, and the %threatType and %alertness fields
//   will allow the NPC to enter their react state, if necessary.
// For example, a large explosion could use AlertAIPlayers(%e.getPosition(),100,50,"Explosion",3);
//   to acheive the desired reaction from the NPCs
// Optional: %sourceObject
//   This specifies whoever created this event, like the user of a weapon, so that they are not alerted
//   by their own shots.
// Optional: %exempt
//   Specifiy an object that should not be alerted
// Optional: %manualThreatPosition
//   If this isn't specified, defaults to %position.
//   This is useful for AI Players who exchange information. %position is the location of an NPC with
//   threat information, and %manualThreatPosition would specify the position of the threat
// Optional: %exemptTeam
//   Objects on this team will not be alerted (NPCs should not think that a bullet fired by a teammate
//   was from an enemy sniper)
function AlertAIPlayers(%position,%radius,%stress,%threatType,%alertLevel,%sourceObject,%exempt,%manualThreatPosition,%exemptTeam,%alertThisTeam,%fromSquad)
{
  if (%manualThreatPosition)
    %alertPos = %manualThreatPosition;
  else
    %alertPos = %position;
    
  if (%threatType $= "Player" && %sourceObject)
    return;
    
  if (%threatType $= "Annoyance" && %sourceObject)
    return;

  // Initialize a radius search to find AI Players within %radius from %position
  InitContainerRadiusSearch(%position,%radius,$TypeMasks::PlayerObjectType | $TypeMasks::AIPlayerObjectType);
  while((%obj = ContainerSearchNext()) != 0)
  {
    // If %fromSquad is specified, somebody in a squad is causing this alert, and other squadmates should not
    //   be affected by this. Otherwise, they tend to get stuck in their react state.
    if (%fromSquad == true && %obj.squad)
      continue; // Skip.
      
    // Make sure that they might actually care about this
    if (%obj.brain.alertness && %alertLevel !$= "")
    {
      if (%obj.brain.alertness > %alertLevel)
        continue; // Skip.
    }

    // Make sure that it is an AI Player and it is not exempt
    if (isObject(%obj.brain) && %obj != %exempt)
    {
      // If %alertThisTeam is specified, make sure that this NPC is on that team
      if (%alertThisTeam !$= "" && %alertThisTeam != -1)
      {
        if (%obj.team != %alertThisTeam)
          continue; // Skip over this NPC
      }
      // If %obj is a member of %exemptTeam, skip over him
      if (%exemptTeam !$= "" && %exemptTeam != -1 && %obj.team == %exemptTeam)
      {
        continue; // Skip.
      }

      // Set the alert level
      if (%alertLevel !$= "")
      {
        %obj.schedule(300,setAlertness,%alertLevel);
        //%obj.brain.setAlertness(%alertLevel);
      }

      // See if the bullet may have been fired by %obj
      if (%threatType $= "Fire")
      {
        if (%sourceObject.team !$= "" && %sourceObject.team == %obj.team)
          continue; // Shooter's team is the same as %obj's team, skip.

        if (VectorDist(%position,%obj.getTrueEyePoint()) < 2.0)
          if (%obj.isImageFiring(0))
            continue; // It's their own bullet, skip.
      }
      
      // See if %obj is the source object, so that they aren't alerted unnecessarily
      if (%obj == %sourceObject)
      {
        continue;
      }
      // Raise the AI Player's stress
      %obj.brain.raiseStress(%stress);
      // Set the ThreatType and ThreatPosition, if provided
      if (%threatType !$= "")
      {
        %objThreatType = %obj.brain.threatType;
				%obj.brain.threatPosition = %alertPos;
				%obj.brain.threatType = %threatType;
      }
    }
  }
}

// Find out if there is a player within a distance (%dist) from a location
// Useful for seeing if somebody is occupying cover
function AIPlayer::isPlayerNearPosition(%this,%position,%dist)
{
  InitContainerRadiusSearch(%position, %dist, $TypeMasks::PlayerObjectType);

  while((%obj = ContainerSearchNext()) != 0)
  {
    if (%obj != %plr)
    {
      %foundPlayer = true;
      break;
    }
  }
  if (%foundPlayer $= true)
  {
    return true;
  }
  else
  {
    return false;
  }
}

// Adds the height of a location on a player to %pos
function PlayerPointZMod(%pos, %point)
{
   switch$(%point)
   {
      case "Foot":
      %heightOnPlr = 0.0119934; // Distance from ground to player's foot

      case "Waist":
      %heightOnPlr = 1.16199; // Distance from ground to player's waist

      case "Muzzle":
      %heightOnPlr = 1.95299; // Distance from ground to player's weapon

      case "Eye":
      %heightOnPlr = 2.26999; // Distance from ground to player's eye (when standing)

      default:
      error("PlayerPointZMod: %point " @ %point @ " is invalid. Must use either Foot, Waist, Muzzle, or Eye.");
      return %pos;
   }
   %xy = getWords(%pos,0,1);
   %z = getWord(%pos,2);
   %z = %z + %heightOnPlr;

   %mod = %xy SPC %z;
      return %mod;
}

// Gets the distance between two points on the XY plane
function AIPlayer::getXYDistance(%p1,%p2)
{
  setWord(%p1,2,0);
  setWord(%p2,2,0);

  return VectorDist(%p1,%p2);
}

// Figures out where our actual eye point is. The original getEyePoint() doesn't factor in stance
// This works for all players, not just ai.
function Player::getTrueEyePoint(%this)
{
  %feet = %this.getPosition();
  %boxCtr = %this.getWorldBoxCenter();
  %feetZ = getWord(%feet, 2);
  %boxCtrZ = getWord(%boxCtr, 2);
  %height = %boxCtrZ - %feetZ; // How far apart are the worldBoxCenter and feet along the Z-axis?
  %eyeZ = %boxCtrZ + %height;
  %eye = getWords(%boxCtr,0,1) SPC %eyeZ;
  if (%this.getPlayerPosition() $= "2")
  {
    %eye = %this.getWorldBoxCenter();
  }
  else
  {
    %eye = %this.getEyePoint();
  }

  %vecPosToCtr = VectorSub(%this.getWorldBoxCenter(),%this.getPosition());
  %eye = VectorAdd(%vecPosToCtr,%this.getWorldBoxCenter());
  %eye = VectorAdd(%eye,"0 0 -0.25");

  return %eye;
}

// Gets the muzzle point of a player, returning the eye point if no weapon is mounted
//   to avoid a crash
function Player::getTrueMuzzlePoint(%this,%slot)
{
  if (%this.getMountedImage(%slot))
    return %this.getMuzzlePoint(%slot);
  else
    return %this.getTrueEyePoint();
}

// Gets the muzzle vector of a player, returning the eye vector if no weapon is mounted
//   to avoid a crash
function Player::getTrueMuzzleVector(%this,%slot)
{
  if (%this.getMountedImage(%slot))
    return %this.getMuzzleVector(%slot);
  else
    return %this.getEyeVector();
}

// Get the position of the ground below a position
// This is useful for judging where to stand to take cover
//   %dist is optional, it specifies how far down the raycast should go
function AIPlayer::getGroundPos(%position,%dist)
{
  if (%dist $= "")
    %dist = 10;

  %startPos = VectorAdd(%position,"0 0 0.0");
  %endPos = VectorAdd(%position,"0 0 " @ -%dist);
  //%ray = ContainerRayCast(%startPos,%endPos,$TypeMasks::InteriorObjectType | $TypeMasks::TerrainObjectType | $TypeMasks::TSStaticObjectType);
  %ray = ContainerRayCast(%startPos,%endPos,$AIPlayer::SolidMasks);
  if (%ray)
  {
    %intersect = getWords(%ray,1,3);
      return VectorAdd(%intersect,"0 0 0.05"); // was 0.08
  }
  else
  {
    return %startPos;
  }
}

function AIPlayer::setAlertness(%this,%lvl)
{
  if (%this.brain)
    %this.brain.setAlertness(%lvl);
}

function AIPlayer::setIsReloading(%this, %bool)
{
   %this.brain.reloading = %bool;
}

function AIPlayer::setIsMelee(%this,%bool)
{
  %this.brain.isMelee = %bool;
}

function AIPlayer::setThreatPosition(%this,%where)
{
  %this.brain.threatPosition = %where;
}

// Checks if a point is inside an ellipse
function isPointInEllipse(%width,%length,%centerX,%centerY,%posX,%posY)
{
  if (%posX >= %centerX && %posX <= (%width / 2))
  {
    if (%posY >= %centerY && %posY <= (%length / 2))
    {
      return true;
    }
  }
  if (%posX <= %centerX && %posX >= (%width / 2))
  {
    if (%posY <= %centerY && %posY >= (%length / 2))
    {
      return true;
    }
  }
  return false;
}

function AIPlayer::checkLineOfFire(%this)
{
  if (!%this.brain)
    return;

	cancel(%this.lineOfFireSched);
	%this.brain.checkLineOfFire();                    
}

// See if anybody is standing on cover marker %obj
function AIPlayer::checkCoverMarkerTaken(%this,%obj)
{
  %pos = %obj.getPosition();
  for(%i = 0;%i < PlayerArray.count();%i++)
  {
    %plr = PlayerArray.getKey(%i);
    %dist = VectorDist(%plr.getPosition(),AIPlayer::GetGroundPos(%pos));
    if (%dist < 3 && %plr.team == %this.team)
      return true;
  }
  return false;
}

// Switches weapons for the AI Player
function AIPlayer::switchWeapons(%this,%which)
{
  if (%this.brain.curWeapon $= %which)
    return; // We don't need to change weapons if we already have that weapon out

  // %which = 0 switches to the primary weapon, %which = 1 switches to the secondary weapon
  %w1 = %this.weapon1;
  %w2 = %this.weapon2;
  %a1 = %this.weapon1ammo;
  %a2 = %this.weapon2ammo;

  switch$(%which)
  {
    case 0: // Switch to the primary weapon
    %this.unmountimage(0);
    %this.mountimage(%w1,$WeaponSlot);
    %this.brain.weapon = %w1;
    %this.brain.ammo = %a1;
    %this.weapon = %w1;
    %this.ammo = %a1;
    %this.brain.curWeapon = 0;
    echo("SWITCHING TO THE PRIMARY WEAPON");

    case 1: // Switch to the secondary weapon
    %this.unmountimage(0);
    %this.mountimage(%w2,$WeaponSlot);
    %this.brain.weapon = %w2;
    %this.brain.ammo = %a2;
    %this.weapon = %w2;
    %this.ammo = %a2;
    %this.brain.curWeapon = 1;
    echo("SWITCHING TO THE SECONDARY WEAPON");

    default: // Toggle Weapons
    if (%this.brain.curWeapon $= 0)
    {
      %this.unmountimage(0);
      %this.mountimage(%w2,$WeaponSlot);
      %this.brain.weapon = %w2;
      %this.brain.ammo = %a2;
      %this.weapon = %w2;
      %this.ammo = %a2;
      %this.brain.curWeapon = 1;
      error("SWITCHING TO THE SECONDARY WEAPON");
      echo("%w1: " @ %w1 @ "; %a1: " @ %a1 @ "; Mounted image: " @ %this.getMountedImage(0).item.getName());
    }
    else
    {
      %this.unmountimage(0);
      %this.mountimage(%w1,$WeaponSlot);
      %this.brain.weapon = %w1;
      %this.brain.ammo = %a1;
      %this.weapon = %w1;
      %this.ammo = %a1;
      %this.brain.curWeapon = 0;
      error("SWITCHING TO THE PRIMARY WEAPON");
      echo("%w1: " @ %w1 @ "; %a1: " @ %a1 @ "; Mounted image: " @ %this.getMountedImage(0).item.getName());
    }
  }
}

// Switches to the most appropriate weapon for a scenario
// This needs work.
function AIPlayer::PickBestWeapon(%this,%distance,%enemyHealth)
{
  %w1 = %this.weapon1;
  %w2 = %this.weapon2;

  %w1RangeScore = -(10/%w1.range)*%distance+10;
  %w1DamageScore = -(10/%w1.damage)*%enemyHealth+10;
  %w1FireRateScore = -10*%w1.fireRate+10;
  %w1MobilityScore = 0.222*%w1.weight+10;
  %w1MagazineScore = 0.125*%this.getInventory(%w1.ammo)/%w1.clipSize-2.5;

  %w2RangeScore = -(10/%w2.range)*%distance+10;
  %w2DamageScore = -(10/%w2.damage)*%enemyHealth+10;
  %w2FireRateScore = -10*%w2.fireRate+10;
  %w2MobilityScore = 0.222*%w2.weight+10;
  %w2MagazineScore = 0.125*%this.getInventory(%w2.ammo)/%w2.clipSize-2.5;

  %w1Score = (%w1RangeScore+%w1DamageScore+%w1FireRateScore+%w1MobilityScore+%w1MagazineScore)/5;
  %w2Score = (%w2RangeScore+%w2DamageScore+%w2FireRateScore+%w2MobilityScore+%w2MagazineScore)/5;

  if (%w1Score < %w2Score) // The secondary weapon is best
    %this.switchWeapons(1);
  else                     // The primary weapon is best
    %this.switchWeapons(0);
}
  
function AIPlayer::isClearTo(%this,%pos)
{
  %ray = ContainerRayCast(VectorAdd("0 0 1",%this.getPosition()),%pos,$AIPlayer::SolidMasks,%this);
  if (!%ray)
  {
    return 1;
  }
  else
  {
    %intersect = getWords(%ray,1,3);
    return %intersect;
  }
}

// Allows/Forbids wandering
function AIPlayer::setWanderAllowed(%this,%bool)
{
  if (%bool == true)
    %this.brain.canWander = true;
  else
    %this.brain.canWander = false;
}

// Allows/Forbids squad waiting (waiting for members to be regrouped)
function AIPlayer::setSquadWait(%this,%bool)
{
  if (%bool == true)
    %this.squadWait = true;
  else
    %this.squadWait = false;
}

// Allows/Forbids cover searching
function AIPlayer::SetCoverSearching(%this,%bool)
{
  if (%bool == true)
     %this.brain.noCoverSearching = false;
  else
     %this.brain.noCoverSearching = true;
}

// Allows/Forbids thinking
function AIPlayer::SetThinking(%this,%bool)
{
  if (%bool $= true)
  {
     %this.noThinking = false;
  }
  else
  {
     %this.noThinking = true;
  }
}

function AIPlayer::SetAstarSearching(%this,%bool)
{
  if (%bool $= true)
  {
     %this.brain.noAstarSearching = false;
  }
  else
  {
     %this.brain.noAstarSearching = true;
  }
}

//------------------------------------------------------------------------------------------------
// Aiming and firing
//------------------------------------------------------------------------------------------------

// Determine where to aim to shoot a moving target
// Borrowed from the Killer Kork Resource, by Devans
// http://www.garagegames.com/community/resources/view/10278

function AIPlayer::getLeadAimPos(%this, %aimObj, %projV)
{
  %plr = %this.player;
  %dist = VectorDist(%this.getPosition(),%aimObj.getTrueEyePoint());
  %velocity = %aimObj.getVelocity();
  
  //if (%dist < 6)
    //return %aimObj.getTrueEyePoint(); // Don't lead aim for close opponents
    
  // figure out where we will be standing on the next think tick
 // %myVel = %this.getVelocity();
 // %myVelSc = VectorScale(%myVel, %this.thinkDelay / 1000);
 // %myPos = VectorAdd(%this.getTrueMuzzlePoint(0),%myVelSc);
 %myPos = %this.getTrueMuzzlePoint(0);

  // vector to the target
  %vec = VectorSub(%aimObj.getPosition(),%myPos);
  %vecn = VectorNormalize(%vec);
  
  if (!%projV)
    %vel = %this.getmountedimage(0).projectile.muzzleVelocity;
  else
    %vel = %projV;

  // Projectile velocity of the current weapon
  %pVel = %vel * 0.5;
  
  // target position at impact
  %closingSpeed = %pVel - VectorDot(%velocity, %vecn);
  %time = %this.thinkDelay / 1000 + %dist / %closingSpeed;
  %posAim = VectorAdd(%aimObj.getTrueEyePoint(), VectorScale(%velocity, %time));

  // aiming error correction
  %vecRot = VectorCross(%vecn, %this.getVelocity());
  %vecErr = VectorScale(VectorCross(%vecn, %vecRot), 0.13);
  %posAim = VectorAdd(%posAim, %vecErr);
  
  return %posAim; 
}

// Aim with random innacuracy
function AIPlayer::AimAt(%this,%pos,%spread)
{
  // Portions borrowed from the Daniel Neilsen's projectile spread resource (http://www.garagegames.com/community/resources/view/1714)
  if (%pos $= "")
  {
    error("INVALID AIM POS IN AIPLAYER::AIMAT");
    return -1;
  }

  if (%spread $= "")
    %spread = %this.brain.spread;

  // See if AI players can use their weapon to fire at a nearby object to damage their opponent
  if (%this.getMountedImage(0).useRadiusDamage == true && %this.getInventory(%this.brain.ammo) > 0)
  {
    %radPos = %this.getRadiusDamageAimPoint(%pos,%this.getMountedImage(0).projectile.damageRadius);
    // see if this position is safe to shoot (no friendlies in the area, including self)
    if (VectorDist(%radPos,%this.getPosition()) <= %this.getMountedImage(0).projectile.damageRadius || !%this.isSafeToRadiusDamage(%pos,%this.getMountedImage(0).projectile.damageRadius))
    {
      error(%this @ ": not safe to fire!");
      %this.setAimLocation(%pos);
      %this.switchWeapons();
      return -1; // Tells the fire code not to shoot the weapon
    }
    %pos = %radPos;
  }
  
  // See if this weapon requires us to compensate for gravity
  if (%this.getMountedImage(0).isBallisticWeapon == true)
  {
    %bCorr = %this.CorrectBallisticAimOffset(%pos,%this.getMountedImage(0).projectile.muzzleVelocity);
    if (%bCorr $= "-1")
    {
      warn(%this @ ": not possible to fire, corr is " @ %bCorr);
      %this.setAimLocation(%pos);
      %this.switchWeapons();
      return -1; // Tells the fire code not to shoot the weapon
    }
    %pos = %bCorr;
  }

  if (VectorDist(%this.getTrueMuzzlePoint(0),%pos) < 7) // HACK
  {
    // The position is close enough, so don't worry about spread.
    // There is a weird bug when trying to spread fire close targets
    // The NPC starts aiming in completely wrong directions, so just aiming straight at
      // the position should do
    %this.setAimLocation(%pos);
    return;
  }
  
  if (%this.getMountedImage(0).usePreciseAim == true)
  {
    // If using precise aim, don't do spread calculations
    %this.setAimLocation(%pos);
    return %pos;
  }

  // Get the direction we need to fire
  %vector = VectorSub(%pos,%this.getTrueMuzzlePoint(0));
  //%vector = VectorScale(%vector,1000);
  //%vector = VectorNormalize(%vector);
  
  // Determine scaled vector. This is still in a straight line as
  // per the default example
  %velocity = VectorScale(%vector, VectorDist(%this.getPosition(),%pos));

  // Determine our random x, y and z points in our spread circle and create
  // a spread matrix.
  %x = (getRandom() - 0.5) * 2 * 3.1415926 * %spread;
  %y = (getRandom() - 0.5) * 2 * 3.1415926 * %spread;
  %z = (getRandom() - 0.5) * 2 * 3.1415926 * %spread;
  %mat = MatrixCreateFromEuler(%x @ " " @ %y @ " " @ %z);
  
  // Alter our projectile vector with our spread matrix
  %aimPoint = VectorAdd(MatrixMulVector(%mat, %velocity),%this.getTrueMuzzlePoint(0));

  %this.setAimLocation(%aimPoint);
    return %aimPoint;
}

// Finds the position to aim at to hit a target with a ballistic weapon.
//  Currently works only if the NPC and the target are at the same height.
function AIPlayer::CorrectBallisticAimOffset(%this,%pos,%roundVel)
{
  %posFlat = setWord(%pos,2,getWord(%this.getPosition(),2));
  %x = VectorDist(%this.getPosition(),%posFlat);
  %y = getWord(%pos,2) - getWord(%this.getPosition(),2);
  error("X delta: " @ %x @ " -- Y delta: " @ %y); 

  %r1 = mSqrt(mPow(%roundVel,4) - 9.82 * (9.82 * (%x * %x) + ( (2 * %y) * (%roundVel * %roundVel))));
  // if r1 is a real number, need to add a check for that
  %a1 = ((%roundVel*%roundVel) - %r1) / (9.82 * %x);
  %a1 = mASin(%a1 / mSqrt((%a1 * %a1) + 1));
  %angleOfReach = mRadToDeg(%a1);
  if ($mortarAim) // debug hack, just for fun
    %angleOfReach = 90 - %angleOfReach;
  error("Angle of reach is " @ %angleOfReach);

  %offsetHeight = mTan(mDegToRad(%angleOfReach)) * %x;

  return VectorAdd(%posFlat,"0 0 " @ %offsetHeight);
}

// Figures out where to fire a radius-damage weapon to hurt an opponent that is not visible
function AIPlayer::getRadiusDamageAimPoint(%this,%pos,%radius)
{
  if (%radius $= "")
    %radius = 10;

  for(%i = 0; %i < 50; %i++)
  {
    %x = getRandom(-%radius, %radius);
    %x = firstWord(%pos) + %x;
    %y = getRandom(-%radius, %radius);
    %y = getWord(%pos, 1) + %y;
    %z = getRandom(-%radius, %radius);
    %z = getWord(%pos, 2) + %z;
    %location = %x SPC %y SPC %z;
    %surfaceRay = ContainerRayCast(%pos,%location,$TypeMasks::TerrainObjectType | $TypeMasks::InteriorObjectType);
    if (%surfaceRay) // It hit something, meaning there is something we can use to reflect the damage
    {
      %intersect = getWords(%surfaceRay,1,3); // So where did it hit?
      %distance = VectorDist(%this.getTrueMuzzlePoint(0),%intersect);
      %vectorTo = VectorSub(%intersect,%this.getTrueMuzzlePoint(0));
      %vectorTo = VectorNormalize(%vectorTo);
      %vectorTo = VectorScale(%vectorTo,%distance - 0.2);
      %location = VectorAdd(%vectorTo,%this.getTrueMuzzlePoint(0));

      %clearShotRay = ContainerRayCast(%this.getTrueMuzzlePoint(0),%location,$TypeMasks::TerrainObjectType | $TypeMasks::InteriorObjectType);
      if (!%clearShotRay) // If there's nothing in the way of the shot
      {
        if (VectorDist(%location,%intersect) <= %radius) // And if it's close enough
        {
          return %location; // Aim here to hurt the target
        }
      }
    }
  }
  return %pos;
}

// Figures out where our target will be on our next think tick, so when he hides behind a wall we know where he probably is
function AIPlayer::getSuppressPos(%this,%obj)
{
  %vel = %obj.getVelocity();
  %vel = VectorScale(%vel,0.5); // Hack for now
  %suppressPos = VectorAdd(%obj.getWorldBoxCenter(),%vel); // His new position
  %suppressPos = VectorAdd(%suppressPos,"0 0 -0.5");
    return %suppressPos;
}

// Evaluates if a projectile we fire will hit our target, rather than an object we are standing behind
function AIPlayer::shouldFire(%this,%pos)
{
  if (!%this.getMountedImage(0).useRadiusDamage) // && %this.getPlayerPosition() != 2)
    return true; // This is an epic hack

  %aimVec = %this.getTrueMuzzleVector(0);
  %aimVec = VectorNormalize(%aimVec);
  %aimVec = VectorScale(%aimVec,$AIPlayer::CloseShotDist*1.5);
  %aimPos = VectorAdd(%this.getTrueMuzzlePoint(0),%aimVec);
  if (!%aimPos)
  {
    error(%this @ ": cannot fire without an aim location");
    return false;
  }
  %ray = ContainerRayCast(%this.getTrueMuzzlePoint(0),%aimPos,$AIPlayer::SolidMasks);
  if (%ray)
  {
    // The ray hit something, so make sure it isn't an object that is too close
    %hitPos = getWords(%ray,1,3);
    %distToAimPos = VectorDist(%this.getTrueMuzzlePoint(0),%aimPos);
    %distToHitPos = VectorDist(%this.getTrueMuzzlePoint(0),%hitPos);
    if (%distToHitPos <= $AIPlayer::CloseShotDist)
    {
      if (%distToHitPos > %distToAimPos)
      {
        return false;
      }
    }
  }
  return true;
}

// Evaluates if suppressing a target will be effective by looking at where a bullet will actually hit
function AIPlayer::shouldSuppress(%this,%pos)
{
  // Don't suppress with precise fire weapons
  if (%this.getMountedImage(0).usePreciseAim == true)
  {
    error(%this @ ": cannot suppress with this weapon");
    return false;
  }

  %muzP = %this.getTrueMuzzlePoint($weaponSlot);
  %muzV = %this.getTrueMuzzleVector($weaponSlot);
  %muzV = VectorNormalize(%muzV);
  %vAhead = VectorScale(%muzV,1000);
  %pAhead = VectorAdd(%muzP,%vAhead);
  %ray = ContainerRayCast(%muzP,%pAhead,$AIPlayer::SolidMasks);
  if (%ray)
  {
    %intersect = getWords(%ray,1,3);
    %distEnemy = VectorDist(%pos,%intersect);
    %distSelf = VectorDist(%muzP,%intersect);
    if (%distEnemy <= $AIPlayer::SuppressEnemyMaxDist && %distSelf >= $AIPlayer::SuppressSelfMaxDist)
    {
      return 1;
    }
    else
    {
      return 0;
    }
  }
  else
  {
    return 0;
  }
}

// Check to see if there are any friendlies in the AIPlayer's line of fire; My squad formations keep getting cut down by the guy in the back that doesn't see his friends in front of him.
function AIPlayer::isFriendlyInLOS(%this)
{
  %start = %this.getTrueMuzzlePoint($WeaponSlot);
  %muzzleVector = %this.getTrueMuzzleVector($WeaponSlot);
  %muzzleVectorNorm = VectorNormalize(%muzzleVector);
  %muzzleVectorCheck = VectorScale(%muzzleVectorNorm,1000); // Look out about 1 kilometer
  %end = VectorAdd(%start,%muzzleVectorCheck);
  // Raycast forward
  %ray = ContainerRayCast(%start,%end,$TypeMasks::PlayerObjectType |
                                      $TypeMasks::InteriorObjectType |
                                      $TypeMasks::StaticTSObjectType |
                                      $TypeMasks::TerrainObjectType, %this);
  if (%ray) // The ray hit something
  {
    %obj = firstWord(%ray); // The first word of what the raycast returns is the object
    if (%obj.getClassName() $= "Player" || %obj.getClassName() $= "AIPlayer")
    {
      if (%obj.team == %this.team)
      {
        return true; // There's a friendly in front of me
      }
      else
      {
        return false; // He's an enemy, fire at will
      }
    }
  }
  return false;
}

// Raytraces the trajectory of a projectile moving at %vec. If it hits something,
//   this function returns the position where the hit occured.
function AIPlayer::TraceTrajectory(%this,%vec,%getHalf)
{
  %maxTime = 5; // Track it for up to 5 seconds of flight
  %step = 0.075;
  %xPos = 0;
  %yPos = 0;
  %oldPos = %this.getTrueMuzzlePoint(0);
  %zStart = getWord(%this.getTrueMuzzlePoint(0),2);
  %i = 0;
  %yVel = getWord(%vec,2);
  %xVel = getWord(%vec,1);
  while (%i < (%maxTime / %step)) 
  {
    %i++;
    %t = %step * %i;
    // Get the height
    %newHeight = -4.6 * mPow(%t,2) + %yVel * %t + 0;
    
    // Scale the vector down
    %scVec = VectorScale(%vec,%step);
    
    // Find the new position
    %newPos = VectorAdd(%oldPos,%scVec);
    %newPos = setWord(%newPos,2,%zStart + %newHeight);
    %yDif = getWord(%newPos,2) - getWord(%oldPos,2);

    %ray = ContainerRayCast(%oldPos,%newPos,$AIPlayer::ProjectileMasks,%this);
    if (%ray)
    {
      %int = getWords(%ray,1,3);
      echo(%this @ ": trace trajectory hit " @ %int);
      
      if (%yDif > 0)
      {
      	echo("Trace hit at positive velocity");
      	if (%getHalf)
      	  return -1;
      }
      
      return VectorAdd(%int,"0 0 0.5");
    }
    %oldPos = %newPos;
  }
}

// Checks to see if launching a grenade at %vec will actually land the round
//   at %target.
function AIPlayer::WillGrenadeLaunchSucceed(%this,%vec,%target,%getHalf)
{
  %trace = %this.traceTrajectory(%vec,%getHalf);
  
  if (%getHalf == true) // If we're only tracing to the top of the trajectory
  {
  	if (%trace == -1)
  	{
			// We do this for throwing grenades. We don't want it bouncing back at us.
			warn("Cannot use grenade");
			return false;
  	}
  	else
  	{
  		return true;
  	}
  }
  
  %dist = VectorDist(%trace,%target);
  if (%dist <= %this.getMountedImage(0).launcherProjectile.damageRadius)
  {
    return true;
  }
  return false;
}

// Checks if it is safe to set off a radius damage weapon (rocket, grenade, etc)
//   at %pos with damage radius %radius
function AIPlayer::IsSafeToRadiusDamage(%this,%pos,%radius)
{
  InitContainerRadiusSearch(%pos,%radius,$TypeMasks::PlayerObjectType);
  while((%obj = ContainerSearchNext()) != 0)
  {
    if (%obj.team == %this.team)
    {
      // Teammate, or self, in the damage area. Not safe to perform this attack.
      return false;
    }
  }
  return true; // We made it this far. Area is clear of friendlies, safe to fire.
}

// Checks to see if we are correctly aimed at our target
function AIPlayer::AimedAtTarget(%this,%dot)
{
	%vecToAimPos = VectorNormalize(VectorSub(%this.getAimLocation(),%this.getTrueMuzzlePoint(0)));
	%muzzleVec = VectorNormalize(%this.getTrueMuzzleVector(0));
	%dot = VectorDot(%vecToAimPos,%muzzleVec);
  
  // Problem?
  if (%this.getAimLocation() $= "0 0 0" || !%this.getAimLocation())
    return false;

  if (%this.brain.attackObj.isCivilian)
  {
    // Attacking a civilian
    if (%dot > $AIPlayer::FireSafeCivilianDot)
    {
      return true;
    }
  }
  else
  {
    // Precise aim required
    if (%this.getMountedImage(0).usePreciseAim == true)
    {
      if (%dot > $AIPlayer::FirePreciseDot && VectorLen(%this.getVelocity()) <= $AIPlayer::FirePreciseMaxSpeed)
      {
        if (!isObject(%this.brain.attackObj) || !%this.brain.isObjVisible(%this.brain.attackObj))
        {
          return false;
        }
        return true;
      }
      else
      {
        return false;
      }
    }
    // If not moving
    if (VectorLen(%this.getVelocity()) < 1)
    {
      if (%dot > $AIPlayer::FireSafeDot)
      {
        return true;
      }
    }
    else // if moving
    {
      if (%dot > $AIPlayer::FireSafeMovingDot)
      {
        return true;
      }
    }
  }
      
  return false;
}

// When the melee weapon contacts the %obj; do damage, play sound, etc
function AIPlayer::MeleeHit(%this,%obj)
{
  if (%this.getState() $= "Dead")
    return;
    
  %start = %this.getTrueMuzzlePoint(0);
  %vec = %this.getTrueMuzzleVector(0);
  %vec = VectorNormalize(%vec);
  %vec = VectorScale(%vec,10);
  %end = VectorAdd(%start,%vec);
  %ray = ContainerRayCast(%start,%end,$TypeMasks::PlayerObjectType,%this);
  %int = firstWord(%ray);
  echo(%start);
  warn(%end);
  error(%ray);
  if (%int != %obj)
  {
    error(%this @ ": " @ %obj @ " dodged the hit!");
    return;
  }

  serverPlay3D("Melee_Punch1",%obj.getTrueEyePoint());
  %obj.damage(%this,%obj.position,getRandom($Brain1::MeleeDamageMin,$Brain1::MeleeDamageMax),"Melee");
  error(%this @ ":    contact!");
}

// Just pull the trigger, simple as that
function AIPlayer::pullTrigger(%this)
{
	%this.schedule(%this.getMountedImage(0).fireDelay,setImageTrigger,0,1);
	%this.schedule(%this.getMountedImage(0).fireDelay,setImageTrigger,0,0);
}

// One aimed shot
function AIPlayer::fire(%this,%where,%checkSuppress,%aimObj)
{	
  if (%this.getState() $= "Dead") // You're dead, don't shoot
     return;
  if (%checkSuppress == 1 && !%this.shouldSuppress(%where))
    return;
  if (isObject(%this.brain) && %this.brain.reloading)
  {
    %this.setImageTrigger(0,0);
    return;
  }

  // If %aimObj is given, figure out where to aim to compensate for their movement
  if (%aimObj && VectorLen(%aimObj.getVelocity()) > 1 && !%this.getMountedImage(0).useRadiusDamage)
  {
    %where = %this.getLeadAimPos(%aimObj);
  }
  %aimed = %this.aimAt(%where);
  if (%aimed == -1)
    return 0;
  
  %vecToAimPos = VectorNormalize(VectorSub(%aimed,%this.getTrueMuzzlePoint(0)));
  %muzzleVec = VectorNormalize(%this.getTrueMuzzleVector(0));
  %dot = VectorDot(%vecToAimPos,%muzzleVec);
  if (!%this.aimedAtTarget(%dot)) // To prevent firing when not aimed at the target
  {
  	%this.setAimLocation(%where);
    %this.setImageTrigger(0,false);
    return;
  }
  
  if (!%this.isImageFiring(0) && %this.isFriendlyInLOS == false)
  {
    if (%this.shouldFire() == true)
    {
      %this.schedule(%this.getMountedImage(0).fireDelay,setImageTrigger,0,1);
      %this.schedule(%this.getMountedImage(0).fireDelay,setImageTrigger,0,0);
    }
  }
}

// Fire a short burst
function AIPlayer::burstFire(%this,%where,%checkSuppress,%aimObj)
{
  if (%this.getState() $= "Dead") // Youre dead, don't shoot
     return;
  if (%checkSuppress == 1 && !%this.shouldSuppress(%where))
     return;
  if (isObject(%this.brain) && %this.brain.reloading)
  {
    %this.setImageTrigger(0,0);
    return;
  }

  // If %aimObj is given, figure out where to aim to compensate for their movement
  if (%aimObj && VectorLen(%aimObj.getVelocity()) > 1 && !%this.getMountedImage(0).useRadiusDamage)
  {
    %where = %this.getLeadAimPos(%aimObj);
  }

  %aimed = %this.aimAt(%where);
  if (%aimed == -1)
    return 0;
  
  %vecToAimPos = VectorNormalize(VectorSub(%aimed,%this.getTrueMuzzlePoint(0)));
  %muzzleVec = VectorNormalize(%this.getTrueMuzzleVector(0));
  %dot = VectorDot(%vecToAimPos,%muzzleVec);
  if (!%this.aimedAtTarget(%dot)) // To prevent firing when not aimed at the target
  {
    %this.setImageTrigger(0,false);
    return;
  }
  
  if (%this.isFriendlyInLOS == false)
  {
    if (%this.shouldFire() == true)
    {
      %this.schedule(%this.getMountedImage(0).fireDelay,setImageTrigger,0,1);
      %this.schedule((%this.thinkDelay/getRandom(2,3)),setimagetrigger,0,0);
    }
  }
}

// Full auto
function AIPlayer::suppressingFire(%this,%where,%fireAnyway,%aimObj)
{
  if (%this.getState() $= "Dead")
    return;
  if (isObject(%this.brain) && %this.brain.reloading)
  {
    %this.setImageTrigger(0,0);
    return;
  }

  // If %aimObj is given, figure out where to aim to compensate for their movement
  if (%aimObj && VectorLen(%aimObj.getVelocity()) > 1 && !%this.getMountedImage(0).useRadiusDamage)
  {
    %where = %this.getLeadAimPos(%aimObj);
  }
  %aimed = %this.aimAt(%where);
  if (%aimed == -1)
    return 0;

  // hack
  if (%fireAnyway != 1)
    return;
  
  %vecToAimPos = VectorNormalize(VectorSub(%aimed,%this.getTrueMuzzlePoint(0)));
  %muzzleVec = VectorNormalize(%this.getTrueMuzzleVector(0));
  %dot = VectorDot(%vecToAimPos,%muzzleVec);
  if (!%this.aimedAtTarget(%dot)) // To prevent firing when not aimed at the target
  {
    %this.setImageTrigger(0,false);
    return;
  }

  if (!%this.friendlyInLOS && (%fireAnyway == 1 || %this.shouldSuppress(%where)))
  {
    if (%this.isFriendlyInLOS == false)
    {
      if (%this.shouldFire() == true)
      {
        %this.schedule(%this.getMountedImage(0).fireDelay,setImageTrigger,0,1);
        %this.schedule(%this.thinkDelay,setimagetrigger,0,0);
      }
    }
  }
}
//------------------------------------------------------------------------------------------------
// Vectoring/Direction functions
//------------------------------------------------------------------------------------------------

function AIPlayer::getPositionAhead(%this,%dist) // Gets a position ahead of the direction the AIPlayer is moving; This is used to predict enemy locations for cover-to-cover movement
{
  %moveDest = %this.getMoveDestination();
  %plrPos = %this.getPosition();
  %vector = VectorSub(%moveDest,%plrPos); // The vector to the move destination
  if (VectorLen(%vector) < 0) // If negative, something got screwed up somehow
  {
    error("AIPlayer::getPositionAhead()--Error, have negative vector length?");
  }
  %vector = VectorNormalize(%vector);
  %vectorAhead = VectorScale(%vector,%dist);
  %positionAhead = VectorAdd(%plrPos,%vectorAhead);
    return %positionAhead;
}

function AIPlayer::getApproachPosition(%this,%dest,%aprDistance)
{
  %vector = VectorSub(%dest,%this.getPosition()); // Vector to the destination
  //%vector = VectorSub(%this.getPosition(),%dest);
  %distance = VectorLen(%vector); // Distance to the destination
  if (%distance < %aprDistance)
  {
    return %this.position; // We're close enough already
  }
  %vectorN = VectorNormalize(%vector);
  %scaleDistance = %distance - %aprDistance; // How far out we should scale VectorN
  %posVector = VectorScale(%vectorN,%scaleDistance);
  %approachPosition = VectorAdd(%posVector,%this.getPosition());
  return %approachPosition;
}

//------------------------------------------------------------------------------------------------
// Quick method to find cover (using cover without a plan to attack targets)
//------------------------------------------------------------------------------------------------

function AIPlayer::findQuickCover(%this,%position,%searchDist,%ignoreLOS,%ignoreDot)
{
  //if (!%this.brain.canWander)
    //return 0;

  if (!isObject(CoverPoints))
    return 0;
    
  if ($noQuickCover == true)
    return 0;

  if (!%this.brain.coverPos)
  {
    %this.brain.coverSearchCounter = 0;
    InitContainerRadiusSearch(%this.getPosition(), %searchDist, $TypeMasks::StaticShapeObjectType);
      
    while ((%obj = ContainerSearchNext()) != 0 && !%this.brain.coverPos)
    {
      %this.brain.coverSearchCounter++;
      if (%this.brain.coverSearchCounter > CoverPoints.getCount() || !isObject(%obj))
      {
        return 0;
      }
      if (%obj.getGroup().getName() $= "CoverPoints" && %obj.isTaken == 0) // We've found a cover point that belongs to the correct group and there isn't anybody occupying it
      {
        %ray = ContainerRayCast(%obj.position,%position,$AIPlayer::SolidMasks);
        if (%ray || %ignoreLos == true) // Cover is good
        {  
          %coverVec = VectorNormalize(%obj.getForwardVector());
          %toEnemyVec = VectorNormalize(VectorSub(%position,%obj.getPosition())); // The vector from cover to the take-cover-from position
          %dot = VectorDot(%coverVec,%toEnemyVec);
          if (%dot > $AIPlayer::findQuickCoverDot || %ignoreDot == true) // If the object is in front of us, it's good
          {
            if (!%this.isPlayerNearPosition(%obj.getPosition(),1.5))
            {
              return %obj;
            }
          }
          else
          {
            continue;
          }
        }
      }
      else
      {
        continue; // Loop again
      }
    }
  }
  return false;
}


//------------------------------------------------------------------------------------------------
// Math Functions
//
// Much of this is from the Killer Kork resource, made by devans:
//   http://www.garagegames.com/community/resources/view/10278
//------------------------------------------------------------------------------------------------

// Returns angle between two vectors
function AIPlayer::getAngle(%vec1, %vec2)
{
  %vec1n = VectorNormalize(%vec1);
  %vec2n = VectorNormalize(%vec2);

  %vdot = VectorDot(%vec1n, %vec2n);
  %angle = mACos(%vdot);

  // convert to degrees and return
  %degangle = mRadToDeg(%angle);
  return %degangle;
}

// Return azimuthal angle between two vectors (in the XY plane)
function AIPlayer::getAz(%vec1, %vec2)
{
  // normalize
  %vec1n = VectorNormalize(%vec1);
  %vec2n = VectorNormalize(%vec2);

  // project onto XY plane
  %vec1p = setWord(%vec1n, 2, 0);
  %vec2p = setWord(%vec2n, 2, 0);

  // compute angle between
  %vecDot = VectorDot(%vec1p, %vec2p);
  %vecCross = VectorCross(%vec1p, %vec2p);
  %angle = mATan(getWord(%vecCross, 2), %vecDot);
  
  // convert to degrees and return
  %degangle = mRadToDeg(%angle);
  return %degangle;
}

function AIPlayer::getVectorTo(%this,%pos)
{
   return VectorSub(%pos, %this.getEyePoint());
}

// return angle between eye vector and %pos
function AIPlayer::getAngleTo(%this, %pos)
{
   return AIPlayer::getAngle(%this.getVectorTo(%pos), %this.getEyeVector());
}

function AIPlayer::getVisProb(%val, %visFrac, %range, %close, %scale)
{
  if( %visFrac == 0.0 )
    %prob = 0.0;
  else if( %val < %close )
    %prob = 1.0;
  else if( %val > %range )
    %prob = 0.0;
  else
  {
    %base = $AIPlayer::VisBaseMin / mPow(%visFrac, $AIPlayer::VisBasePow);
    %pval = mPow((%val - %close) / %scale, 2);
    %prob = mPow(%base, -1.0 * %pval);
  }
  return %prob;
}

// Rotate a vector along the Z-axis
// (%angle is in degrees)
function AIPlayer::vecRotateAz(%vec, %angle)
{
  %rot = MatrixCreateFromEuler("0 0 " @ mDegToRad(%angle));
  return MatrixMulVector(%rot, %vec);
}

// Rotate a vector around the X axis
// (%angle is in degrees)
function AIPlayer::vecRotateX(%vec, %angle)
{
  %rot = MatrixCreateFromEuler("0 " @ mDegToRad(%angle) @ " 0");
  return MatrixMulVector(%rot, %vec);
}

// Gets the distance from the player to a point, %pos
function AIPlayer::getDistTo(%this, %pos)
{ 
   return VectorDist(%pos, %this.getPosition()); 
}

// Aim in the direction we are moving
function AIPlayer::aimForward(%this)
{
  %vel = %this.getVelocity();
  %len = VectorLen(%vel);
  if (%len < 0.1)
  {
    %this.clearAim();
    return;
  }
  
  %vector = VectorScale(VectorNormalize(%vel),100);
  %aimPos = VectorAdd(%vector,%this.getPosition());
  %this.setAimLocation(%aimPos);
  %this.clearAim();
}

// Spin around %angle degrees
function AIPlayer::doYaw(%this, %angle)
{
  %pos = %this.getTrueMuzzlePoint(0);
  %eye = VectorScale(%this.getTrueMuzzleVector(0),1000);

  // rotate eye vector and add to position
  %vec = AIPlayer::vecRotateAz(%eye, -%angle);
  %posAim = VectorAdd(%pos, VectorScale(%vec, 100));
  
  // set aim location
  %this.setAimLocation(%posAim);
}

// Aim up/down %angle degrees
function AIPlayer::doPitch(%this, %angle)
{
  %pos = %this.getTrueMuzzlePoint(0);
  %eye = VectorScale(%this.getTrueMuzzleVector(0),1000);

  // rotate eye vector and add to position
  %vec = AIPlayer::vecRotateX(%eye, -%angle);
  %posAim = VectorAdd(%pos, VectorScale(%vec, 100));
  
  // set aim location
  %this.setAimLocation(%posAim);
}

//------------------------------------------------------------------------------------------------
// Cover Marker setup
//------------------------------------------------------------------------------------------------

//Key bind
MoveMap.bindCmd(keyboard, ";", "commandToServer(\'DropCPoint\');", "");

//Create a new node function
function ServerCmdDropCPoint(%client)
{
   if (!isObject(CoverPoints))
   {
     new SimGroup(CoverPoints) {};
     MissionGroup.add(coverPoints);
   }
   
   %transform = %client.player.getTransform();
   if (!$makeVisibleCover)
     %data = "CoverPosMarker";
   else
     %data = "CoverPosMarkerVisible";
   %node = new StaticShape() 
   {
      dataBlock = %data;
   };
   %node.setTransform(%transform);
   %newPosition = VectorAdd(%client.player.position,"0 0 1");
   %node.setTransform(%newPosition);
   CoverPoints.add(%node);
}

// Resets the the isTaken status of each cover point in the mission
function resetCoverPoints()
{
   %group = nameToId("MissionGroup/CoverPoints");
   %groupCount = %group.getCount();

   for (%i = 0; %i < %groupCount; %i++)
   {
      %obj = %group.getObject(%i);
      %obj.isTaken = false;
   }
}

// Erases the data saved in each cover point in the mission
function eraseCoverPointData()
{
   %group = nameToId("MissionGroup/CoverPoints");
   %groupCount = %group.getCount();

   for (%i = 0; %i < %groupCount; %i++)
   {
      %obj = %group.getObject(%i);
      %obj.isTaken = false;
      %obj.fireOverThis = false;
      %obj.rightFirePos = "";
      %obj.leftFirePos = "";
   }
}

// Sets up every cover point in the CoverPoints group
function SetUpCoverPoints()
{
  %groupName = "MissionGroup/CoverPoints";
  %group = nameToId(%groupName);
  %count = %group.getCount();
  if (%count)
  {
    for (%index = 0; %index < %count; %index++)
    {
      %obj = %group.getObject(%index);
      %obj.isTaken = false; // Reset the taken status so we can save the mission
      %obj.setUpCoverNode(); // Set up the marker
    }
  }
  else
  {
    error("No cover markers in CoverPoints group");
  }
}

// Set up an individual cover point
function StaticShape::setUpCoverNode(%this)
{
  %fwdDist = 1.5; // Cast a ray this far forward when checking for clear line-of-fire
  %vecFwd = "0" SPC %fwdDist SPC "0";

  %groundPos = AIPlayer::getGroundPos(%this.getPosition());
  %muzzlePos = PlayerPointZMod(%groundPos,"Muzzle");

  %rightPos = %muzzlePos;
  %rightVec = "1 0 0";
  %rightVec = MatrixMulVector(%this.getTransform(), %rightVec);
  %rightPos = VectorAdd(%rightPos, %rightVec);
  %rightTrans = %rightPos SPC getWords(%this.getTransform(),3,6);
  %rightFwdVec = MatrixMulVector(%rightTrans, %vecFwd);
  %endright = VectorAdd(%rightPos, %rightFwdVec);
  
  %leftPos = %muzzlePos;
  %leftVec = "-1 0 0";
  %leftVec = MatrixMulVector(%this.getTransform(), %leftVec);
  %leftPos = VectorAdd(%leftPos, %leftVec);
  %leftTrans = %leftPos SPC getWords(%this.getTransform(),3,6);
  %leftFwdVec = MatrixMulVector(%leftTrans, %vecFwd);
  %endleft = VectorAdd(%leftPos, %leftFwdVec);
  
  //%markerHeight = getWord(%pos,2);
  //%muzzleHeight = %markerHeight + 1.5;
  //%muzzlePos = %this.position;
  //%muzzleVec = "0 0" SPC %muzzleHeight;
  //%muzzleVec = MatrixMulVector(%this.getTransform(), %muzzleVec);
  //%muzzlePos = VectorAdd(%muzzlePos, %muzzleVec);
  //%muzzleTrans = %muzzlePos SPC getWords(%this.getTransform(),3,6);
  //%muzzleFwdVec = MatrixMulVector(%muzzleTrans, %vecFwd);
  //%endmuzzle = VectorAdd(%muzzlePos, %muzzleFwdVec);
  %muzzleVec = %this.getForwardVector();
  %muzzleVec = VectorNormalize(%muzzleVec);
  %muzzleVec = VectorScale(%muzzleVec,%fwdDist);
  %endMuzzle = VectorAdd(%muzzlePos,%muzzleVec);
  
  //echo("rightpos " @ %rightpos);
  //echo("rightend " @ %endright);
  //echo("leftpos " @ %leftpos);
  //echo("leftend " @ %endleft);
  //echo("muzzlepos " @ %muzzlepos);
  //echo("muzzleend " @ %endmuzzle);

  %rayRightFwd = ContainerRayCast(%rightPos, %endRight, $AIPlayer::SolidMasks);
  %rayLeftFwd = ContainerRayCast(%leftPos, %endLeft, $AIPlayer::SolidMasks);
  %rayMuzzleFwd = ContainerRayCast(%muzzlePos, %endMuzzle, $AIPlayer::SolidMasks);

  //warn(%this.getID() @ " rayRightFwd: " @ %rayRightFwd);
  //warn(%this.getID() @ " rayLeftFwd: " @ %rayLeftFwd);
  //warn(%this.getID() @ " rayMuzzleFwd: " @ %rayMuzzleFwd);

  //%this.fireOverThis = false;

  if (!%rayMuzzleFwd)
  {
    %this.fireOverThis = true;
    //echo("fire over this");
      return;
  }

  %this.fireOverThis = false;

  if (!%rayRightFwd || !%rayLeftFwd) // If we cant fire over the cover but either side is clear
  {
    if (!%rayRightFwd)
    {
      //echo(%this @ ": rayrightfwd is clr");
      %this.rightFirePos = %rightPos;
    }
    if (!%rayLeftFwd)
    {
      //echo(%this @ ": rayleftfwd is clr");
      %this.leftFirePos = %leftPos;
    }
  }

  if (%this.rightFirePos || %this.leftFirePos) // We don't fire over this cover
  {
    %this.fireOverThis = false; // So save this variable
    //error(%this @ " USES HORIZONTAL MOVEMENT");
  }
  else // No way to return fire
  {
    //%this.isTaken = 1;
    error(%this @ ": no way to return fire!");
  }
}


//------------------------------------------------------------------------------------------------
// Squads
//------------------------------------------------------------------------------------------------
function AIPlayer::setCurrentOrder(%this,%order)
{
  %this.brain.setCurrentOrder(%order);
}

//------------------------------------------------------------------------------------------------
// Ballistics and grenade throwing
//------------------------------------------------------------------------------------------------

function AIPlayer::setNoGrenades(%this,%bool)
{
	%this.noGrenades = %bool;
}

// Throw or launch a grenade at %pos
function AIPlayer::flushOut(%this,%pos)
{
  %wpn = %this.getMountedImage(0);

  if (%wpn.hasLauncher == true)
  {
    if (%this.isSafeToRadiusDamage(%pos,%wpn.launcherProjectile.damageRadius))
    {
      %radPos = %this.getRadiusDamageAimPoint(%pos,%wpn.launcherProjectile.damageRadius);
      %firePos = %radPos;
      if (%firePos == %pos)
      {
        %gNode = %this.getBestGrenadeNode(%this.brain.suppressPos,GrenadeProjectile.damageRadius);
        if (%gNode != -1)
          %firePos = %gNode.getPosition();
      }
      %this.launchGrenade(%firePos,%wpn.launcherSound,%wpn.launcherProjectile);
    }
  }
  else
  {
    if (%this.isSafeToRadiusDamage(%pos,%wpn.launcherProjectile.damageRadius))
    {
      %gNode = %this.getBestGrenadeNode(%this.brain.suppressPos,GrenadeProjectile.damageRadius);
      if (%gNode != -1)
      {
        %this.throwGrenade(%gNode.getPosition());
      }
    }
  }
}

// Launch a grenade from our weapon's "grenade launcher"
//  Somewhat faked effect. Just plays a sound and creates the projectile. Makes things easier.
function AIPlayer::launchGrenade(%this,%loc,%sound,%pData,%fireAnyway)
{
  // determine the launch vector
  %aimPosition = %this.correctBallisticAimOffset(%loc,%pData.muzzleVelocity);
  %vec = VectorSub(%aimPosition,%this.getTrueMuzzlePoint(0));
  %vec = VectorNormalize(%vec);
  %vec = VectorScale(%vec,%pData.muzzleVelocity);
  
  if (!%this.willGrenadeLaunchSucceed(%vec,%loc) && !%fireAnyway)
    return;

  // create the projectile  
  %p = new Projectile()
  {
    dataBlock = %pData;
    initialVelocity = %vec;
    initialPosition = %this.getTrueMuzzlePoint(0);
    sourceObject = %this;
    sourceSlot = 0;
    //client = %obj.client;
  };
  // play sound
  ServerPlay3d(%sound,%this.getTrueMuzzlePoint(0));
  warn(%this @ ": launching " @ %this.getMountedImage(0).launcherProjectile @ " grenade at vector " @ %vec); 
}

// Toss a frag grenade
function AIPlayer::throwGrenade(%this,%loc)
{
  %ourPos = %this.getTrueMuzzlePoint(0);
  %dist = VectorDist(%ourPos,%loc);
  if (%dist >= 80 || %dist <= 25)
    return 0;
    
  if (%this.noGrenades == true)
    return;

  // determine the launch vector
  %aimPosition = %this.correctBallisticAimOffset(%loc,$AIPlayer::GrenadeThrowSpeed);
  %vec = VectorSub(%aimPosition,%this.getTrueMuzzlePoint(0));
  %vec = VectorNormalize(%vec);
  %vec = VectorScale(%vec,$AIPlayer::GrenadeThrowSpeed); // hack, see above
  
  if (!%this.willGrenadeLaunchSucceed(%vec,%loc,true)) // Also works for grenade throw
  {
  	error("Impossible to throw grenade");
    return;
  }

  // create the projectile  
  %p = new Projectile()
  {
    dataBlock = GrenadeLauncherProjectile;
    initialVelocity = %vec;
    initialPosition = %this.getTrueMuzzlePoint(0);
    sourceObject = %this;
    sourceSlot = 0;
    //client = %obj.client;
  };
  warn(%this @ ": throwing hand grenade at vector " @ %vec); 
  
  %this.setNoGrenades(true);
  %this.schedule(5000,setNoGrenades,false); // hack
}

//------------------------------------------------------------------------------------------------
// FindQuickCover list
//
// This is used to track the cover markers we've used when moving
// cover-to-cover so we don't move to a point we've already visited on
// a path.
//------------------------------------------------------------------------------------------------

function AIPlayer::createCList(%this)
{
  if (%this.cList $= "") // Only make a cList if we don't already have one
  {
    %this.cList = "";
  }
  else
  {
    error(%this @ ": Overwriting current cList to make a new one");
    %this.cList = "";
  }
}
function AIPlayer::clearCList(%this)
{
  %this.cList = "";
}
function AIPlayer::addToCList(%this,%what)
{
  if (%this.cList $= "") // If this is the first entry we're adding
  {
    if (%what $= "" || %what $= 0 || %what $= -1)
    {
      error(%this @ "--addToCList(): invalid entry " @ %what);
        return;
    }
    else
    {
      %this.cList = %what; // then that's all we have on it. If we don't do this,
    }                          // each cList will start with a space.
  }
  else
  {
    %this.cList = %this.cList SPC %what; // There is data on the list, so separate
  }                                         // the new entry with a space
}
function AIPlayer::getCListCount(%this)
{
  if (%this.cList !$= "")
  {
    %count = getWordCount(%this.cList) - 1;
      return %count;
  }
}
function AIPlayer::getCListObject(%this,%index)
{
  if (%this.cList !$= "")
  {
    return getWord(%this.cList,%index);
  }
  else
  {
    error(%this @ "--getCListObject(" @ %index @ "): no cList!");
  }
}
function AIPlayer::isInCList(%this,%what) // Checks to see if %what is part of our cList
{
  if (%this.cList !$= "")
  {
    %count = getWordCount(%this.cList);
    for (%i = 0; %i < %count; %i++)
    {
      %obj = %this.getCListObject(%i);
      if (%obj !$= "" && %obj == %what)
      {
        return 1;
      }
    }
    return 0; // If we found the object, we would have returned 1 by now, so we didn't find it.
  }
}

//------------------------------------------------------------------------------------------------
// Hunt list
//
// This is used to track the nodes we would like to move to to hunt down
//   an enemy
//------------------------------------------------------------------------------------------------

function AIPlayer::createHList(%this)
{
  if (%this.HList $= "") // Only make a HList if we don't already have one
  {
    %this.HList = "";
  }
  else
  {
    error(%this @ ": Overwriting current HList to make a new one");
    %this.HList = "";
  }
}
function AIPlayer::clearHList(%this)
{
  %this.HList = "";
}
function AIPlayer::addToHList(%this,%what)
{
  if (%this.HList $= "") // If this is the first entry we're adding
  {
    if (%what $= "" || %what $= 0 || %what $= -1)
    {
      error(%this @ "--addToHList(): invalid entry " @ %what);
        return;
    }
    else
    {
      %this.HList = %what; // then that's all we have on it. If we don't do this,
    }                          // each HList will start with a space.
  }
  else
  {
    %this.HList = %this.HList SPC %what; // There is data on the list, so separate
  }                                         // the new entry with a space
}
function AIPlayer::getHListCount(%this)
{
  if (%this.HList !$= "")
  {
    %count = getWordCount(%this.HList) - 1;
      return %count;
  }
}
function AIPlayer::getHListObject(%this,%index)
{
  if (%this.HList !$= "")
  {
    return getWord(%this.HList,%index);
  }
  else
  {
    error(%this @ "--getHListObject(" @ %index @ "): no HList!");
  }
}
function AIPlayer::isInHList(%this,%what) // Checks to see if %what is part of our HList
{
  if (%this.HList !$= "")
  {
    %count = getWordCount(%this.HList);
    for (%i = 0; %i < %count; %i++)
    {
      %obj = %this.getHListObject(%i);
      if (%obj !$= "" && %obj == %what)
      {
        return 1;
      }
    }
    return 0; // If we found the object, we would have returned 1 by now, so we didn't find it.
  }
}

//------------------------------------------------------------------------------------------------