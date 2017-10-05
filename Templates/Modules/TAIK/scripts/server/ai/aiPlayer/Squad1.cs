//-----------------------------------------------------------------------------
// Tactical AI Kit
// Copyright (C) Bryce Carrington
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------
// Squad1.cs
// Code for a squad of AI Players
//-----------------------------------------------------------------

//----------------------------------------
// Globals
//----------------------------------------

$Squad1::RegroupedDistance = 12; // If each member is within this distance to the squad leader, we are regrouped
$Squad1::ObjectiveRegroupedDistance = 20; // In stateObjective, allow squad members to spread out up to this distance
$Squad1::IsMoveCompleteDist = 8; // A member at least this close to the ordered move position has finished moving up

//----------------------------------------
// General Functions
//----------------------------------------

function Squad1::Create(%leader,%m1,%m2,%m3) // This squad consists of a leader and three other members
{
  %squad = new ScriptObject() 
  {
  	class = "Squad1";

    numMembers = 4; // Four-person squad
    member[0] = %leader;
    member[1] = %m1;
    member[2] = %m2;
    member[3] = %m3;

    // Formation offsets; The deviation of each member's position from the squad leader's when we are in formation
    offset[0] = "0 0 0";
    offset[1] = "-2 -2 0";
    offset[2] = "2 -2 0";
    offset[3] = "3 -3 0";
    
    objectiveType = "None"; // ObjectiveTypes can be Move, Defend, and None [for now].
    objectiveData = 0; // Depending on the ObjectiveType, this can be either a position or object id
    roe = 0; // Rules of engagement; 0 is fire at will, 1 is return fire only
    
    moveLocation = ""; // A place we should move to that isn't part of the objective. Move towards a sound, release a hostage, whatever.
    orderedMovePosition = 0;
    
    currentOrder = "Regroup"; // Can be Regroup (follow), Hold, Move, and Objective
  };
  %squad.onCreate();
  %squad.setOrders("Regroup");
  MissionCleanup.add(%squad);
  echo("Squad1::Create--" @ %squad);
    return %squad;
}

function Squad1::OnCreate(%this)
{
  // Tell the members they are part of the squad
  %this.member[0].squad = %this;
  %this.member[0].memberIndex = 0;
  %this.member[1].squad = %this;
  %this.member[1].memberIndex = 1;
  %this.member[2].squad = %this;
  %this.member[2].memberIndex = 2;
  %this.member[3].squad = %this;
  %this.member[3].memberIndex = 3;
  
  %this.type = %this.member[0].aiType;
  %this.team = %this.member[0].team;
  
  // Array of manually set objective positions
  %this.objectiveArray = new ArrayObject() { };
  MissionCleanup.add(%this.objectveArray);
  
  %this.onOrdersChanged();
}

function Squad1::Respawn(%squad,%marker)
{
  %type = %squad.type;
  %team = %squad.team;
  %s = Squad1::create(spawnai(%team,%marker,%type),spawnai(%team,%marker,%type),spawnai(%team,%marker,%type),spawnai(%team,%marker,%type));
  return %s;
}

//----------------------------------------
// To quickly add a test squad by console
//----------------------------------------

function testsquad(%team,%marker,%type)
{
  %m1 = spawnai(%team,%marker,%type);
  %m2 = spawnai(%team,%marker,%type);
  %m3 = spawnai(%team,%marker,%type);
  //%m4 = spawnai(1,%marker,6);
  
  %squad = Squad1::create(%m1,%m2,%m3);
  $a = %squad;
  echo("Squad1 added, id is " @ %squad);
  return %squad;
}

//----------------------------------------
// Ease-of-use functions
//----------------------------------------

function Squad1::setOrders(%this,%order)
{
  %this.currentOrder = %order;
  // This is a special case
  // We delay each member's recieving the move order
  // If they all move at once, they usually get stuck
  if (%order $= "Move")
  {
    for (%i = 0; %i < %this.numMembers; %i++)
    {
      %member = %this.member[%i];
      %delay = 500; // how many milliseconds between each member recieving the order
      if (isObject(%member) && %member.getState() !$= "Dead" && %member.brain)
      {
        %schedDelay = %delay * %i;
        error(%schedDelay SPC %i);
        //%member.brain.setCurrentOrder(%order);
        //schedule(%schedDelay,%member.brain,setCurrentOrder,%order);
        %member.schedule(%schedDelay,setCurrentOrder,%order);
      }
    }
  }
  else
  {
    for (%i = 0; %i < %this.numMembers; %i++)
    {
      %member = %this.member[%i];
      if (isObject(%member) && %member.getState() !$= "Dead" && %member.brain)
      {
        %member.brain.setCurrentOrder(%order);
      }
    }
  }
}

// Set the squad to move to an objective
function Squad1::ObjectiveMove(%this,%pos)
{
  %this.objectiveType = "Move";
  %this.objectiveData = %pos;
  warn(%this @ ": squad assigned move order.");
}

// Sets the squad to move to positions in an array
function Squad1::ObjectiveMission(%this,%array)
{
  %this.objectiveNum = 0;
  if (%array.count() > 0)
  {
    %this.objectiveMove(%array.getKey(0));
    %this.objectiveArray = %array;
    warn(%this @ ": squad assigned mission with array " @ %array);
    %array.echo();
  }
}

// Set the squad to patrol the area around %pos within radius %radius
function Squad1::ObjectivePatrol(%this,%pos,%radius)
{
  %this.objectiveType = "Move";
  %this.objectiveData = %pos;
  %this.patrolOrigin = %pos;
  %this.patrolRadius = %radius;
}

// See if all squad members are regrouped
function Squad1::isSquadRegrouped(%this,%fromStateObjective)
{
  %isRegrouped = 1; // The variable that decides whether or not the squad is regrouped
  for (%i = 0; %i < %this.numMembers; %i++)
  {
    %member = %this.member[%i];
    if (isObject(%member))
    {
      if (%member.getState() !$= "Dead")
      {
				%distToLeader = VectorDist(%member.getPosition(),%this.member[0].getPosition());
				if (%fromStateObjective == true)
				{
					if (%distToLeader > $Squad1::ObjectiveRegroupedDistance)
					{
						%isRegrouped = 0;
						break;
					}
				}
				else
				{
					if (%distToLeader > $Squad1::RegroupedDistance)
					{
						%isRegrouped = 0;
						break; // This member is not in position, so we are not regrouped
					}
				}
      }
    }
  }
  return %isRegrouped;
}

// See if the squad has finished the move order given by an AI squad leader
function Squad1::isMovedUp(%this)
{
  if (!%this.orderedMovePosition)
    return true;
  
  %moveUpComplete = true;
  for (%i = 0; %i < %this.numMembers; %i++)
  {
    %member = %this.member[%i];
    if (isObject(%member) && %member.memberIndex != 0 && %member.getState() !$= "Dead")
    {
      %distToOrderedMovePos = VectorDist(%this.orderedMovePosition,%member.getPosition());
      %ray = ContainerRayCast(%member.getWorldBoxCenter(),%this.orderedMovePosition,$AIPlayer::SolidMasks);
      if (!%member.brain)
      {
        if (%distToOrderedMovePos > $Squad1::IsMoveCompleteDist || %ray)
          %moveUpComplete = false;
      }
      else
      {
        if (!%member.isStopped()) // || %member.brain.state !$= "Move")
          %moveUpComplete = false;
      }
    }
  }
  return %moveUpComplete;
}

// See how many members are alive
function Squad1::getLivingMemberCount(%this)
{
  %count = 0;
  for (%i = 0; %i < %this.numMembers; %i++)
  {
    %member = %this.member[%i];
    if (isObject(%member))
    {
      if (%member.getState() !$= "Dead")
      {
        %count++;
      }
    }
  }
  return %count;
}

function Squad1::onOrdersChanged(%this)
{
  %order = %this.currentOrder;
  %leader = %this.member[0];
  
  echo(%this @ "--onOrdersChanged()");
  for (%i = 0; %i < %this.numMembers; %i++)
  {
    %member = %this.member[%i];
    if (isObject(%member) && %member.getState() !$= "Dead")
    {
    	%member.clearAim();
      if (%member.client)
      {
        if (%member != %this.member[0]) // Not the team leader
        {
          switch$ (%order)
          {
            case "Objective":
            CenterPrint(%member.client, "Team Leader: Follow me to the objective.", 1.5, 1);
            //alxPlay("Radio_objective1");
            
            case "Regroup":
            CenterPrint(%member.client, "Team Leader: Regroup.", 1.5, 1);
            //alxPlay("Radio_regroup1");
      
            case "Move":
            CenterPrint(%member.client, "Team Leader: Move Up.", 1.5, 1);
            //alxPlay("Radio_moveup1");
          }
        }
      }
    }
  }
}

// Set member %member to be part of the squad, with index %index
function Squad1::setMember(%this,%member,%index)
{
  %this.member[%index] = %member;
  echo(%member @ "'s idx is now " @ %member.memberIndex @ ", squad member[" @ %index @ "] is " @ %this.member[%index] );
}

// Tells the squad to quit reacting and perform the given order
function setQuitReact(%squad,%bool)
{
	%squad.quitReact = %bool;
}

// Figure out where a squad member should stand to create a wedge formation (if possible)
function Squad1::getSquadFormationPosition(%this,%squadIndex)
{
  %leader = %this.member[0];
  %leaderPos = %leader.getPosition();

  %plr = %this.member[%squadIndex];
  %offset = %this.offset[%squadIndex];

  // Do a matrix calculation based on the leader's transform and see where we would be standing in formation
  %startPos = %leaderPos;
  %vec = MatrixMulVector(%leader.getTransform(), %offset);
  %endPos = VectorAdd(%startPos, %vec);

  // Run a raycast from the leader's position to the formation position to see if we can actually stand there
  %ray = ContainerRayCast(%leader.getWorldBoxCenter(),%endPos,$AIPlayer::SolidMasks);
  if (%ray) // If the raycast hit an object, we need to determine where we can actually stand to stay close to the squad leader without completely crowding him
  {
    %intersect = getWords(%ray,1,3); // Where the raycast hit an obstruction
    %vectorToEnd = VectorSub(%intersect,%leader.getWorldBoxCenter());
    %len = VectorLen(%vectorToEnd);
    %newLength = %len - 0.7; // We need to shorten the distance from the leader to the intersect point so we don't screw up the pathfinding system.
                       // Sometimes the intersect point will be inside the obstruction a little, which could make the NPC go to a location he shouldn't go to
    %vecNormalized = VectorNormalize(%vectorToEnd);
    %newVector = VectorScale(%vecNormalized,%newLength);
    %endPos = VectorAdd(%leader.getWorldBoxCenter(),%newVector);
    %formationPosition = %endPos; // The new position we can go to
  }
  else
  {
    // If the raycast hit nothing, we can go ahead and use that position
    %formationPosition = %endPos;
  }
  return %formationPosition;
}

// If our squad leader dies, we need to put the next member of the squad in command
function Squad1::ChangeLeader(%this)
{
  // Add all living members to an array
  %count = %this.numMembers;
  if (%count < 1)
    return;

  %livingMemberCount = -1;
  for(%i=0;%i<%count;%i++)
  {
    %member = %this.member[%i];
    if (isObject(%member) && %member.getState() !$= "Dead")
    {
      %livingMemberCount++;
      %livingMember[%livingMemberCount] = %member;
    }
  }
  if (%livingMemberCount < 0)
    return; // All squad members are dead, so we can't get a new squad leader

  // Once we have our list of living members, we need to rewrite the squad member list
  for(%i=0;%i<%count;%i++)
  {
    %this.member[%i] = %livingMember[%i];
    %this.member[%i].memberIndex = %i;
  }
  warn("m0: " @ %this.member[0] @ ", idx: " @ %this.member[0].memberIndex);
  warn("m1: " @ %this.member[1] @ ", idx: " @ %this.member[1].memberIndex);
  warn("m2: " @ %this.member[2] @ ", idx: " @ %this.member[2].memberIndex);
  warn("m3: " @ %this.member[3] @ ", idx: " @ %this.member[3].memberIndex);

  %this.onOrdersChanged();
}

// Set positions for each squad member to move to
// Picks nodes %radius distance from %basePosition, and sets those members to move there
function Squad1::setMovePositions(%this,%basePosition,%radius)
{
  %positionArray = new ArrayObject();
  MissionCleanup.add(%positionArray);
  
  %this.orderedMovePosition = %basePosition;

  // Iterate through each living squad member
  %numLivingMembers = 0;
  for (%mIdx = 0; %mIdx < %this.numMembers; %mIdx++)
  {
    %member = %this.member[%mIdx];
    if (!isObject(%member) || !%member.brain || %member.getState() $= "Dead")
      continue; // Look up the next member

    %numLivingMembers++;

    // Set up the array that holds the list of positions that we plan to have the squad members move to
    %addPosition = %this.getMovePosition(%basePosition,%radius,%positionArray);
    if (%addPosition == -1)
    {
      %positionArray.add(%basePosition,0); // If we couldn't find a node to move to, then use the base position
    }
  }
  %positionArray.echo();

  // Iterate through the positionArray and assign its values to the squad members
  %positionsAssigned = 0;
  for (%arrayIdx = 0; %arrayIdx < %this.numMembers; %arrayIdx++)
  {
    %assignMember = %this.member[%arrayIdx]; // was arrayidx+1

    echo("checking " @ %assignMember @ ", state is " @ %assignMember.getState());
    if (isObject(%assignMember) && %assignMember.brain && %assignMember.getState() !$= "Dead")
    {
      // Erase each member's current cover status (unless they are engaging an enemy) to allow them to move to other cover
      if (%assignMember.brain.coverMarker && %assignMember.brain.state !$= "FINDCOVER")
      {
        %assignMember.brain.coverMarker.isTaken = 0;
        %assignMember.brain.coverMarker = 0;
        %assignMember.brain.coverPos = 0;
      }
      echo(%assignMember @ " has been assigned to move to " @ %positionArray.getKey(%positionsAssigned));
      %cover = %positionArray.getValue(%positionsAssigned); // If the point chosen was a covermarker, it was saved in ::getMovePosition
      if (isObject(%cover))
      {
        %cover.isTaken = 1;
        %assignMember.brain.coverMarker = %cover;
        %assignMember.brain.shootOverCover = %cover.fireOverThis;
        %assignMember.brain.coverPos = %cover.getPosition();
        if (%cover.fireOverThis == false)
        {
          if (%cover.rightFirePos !$= "")
          {
            %assignMember.brain.shootRight = true;
            %assignMember.brain.returnFirePos = %cover.rightFirePos;
          }
          else
          {
            %assignMember.brain.shootRight = false;
            %assignMember.brain.returnFirePos = %cover.leftFirePos;
          }
        }
      }
      %assignMember.brain.movePosition = %positionArray.getKey(%positionsAssigned);
      %positionsAssigned++;
    }
    else
      echo("invalid member: " @ %assignMember @ " at index: " @ %arrayIdx);
  }
}
function Squad1::getMovePosition(%this,%basePosition,%radius,%array)
{
  // Iterate through the AIPaths list to find nodes within %radius of %basePosition
  InitContainerRadiusSearch(%basePosition,9,$TypeMasks::StaticShapeObjectType); //HACK
  while((%obj = ContainerSearchNext()) != 0)
  {
    if (%obj.getGroup().getName() $= "CoverPoints")
    {
      if (%array.getIndexFromKey(%obj.getPosition()) == -1 && %obj.isTaken == false)
      {
        %cRay = ContainerRayCast(%obj.getPosition(),%basePosition,$AIPlayer::SolidMasks);
        if (%cRay)
          continue; // The cover point should be able to see the ordered move position. If not, check other markers

        %markerDir = VectorNormalize(%obj.getForwardVector());
        %advanceDir = VectorSub(%basePosition,%this.member[0].getPosition());
        %advanceDir = VectorNormalize(%advanceDir);
        %angle = AIPlayer::getAz(%markerDir,%advanceDir);
        %abs = mAbs(%angle);
        if (%abs > 60) // HACK
          continue; // the cover point should point within a certain angle of the direction the team leader wants us to advance

        if (AIPlayer::getPathDistance(0,%basePosition,%obj.getPosition()) > 20) // HACK
          continue; // Must be close enough

        %array.add(%obj.getPosition(),%obj);
        error("FOUND COVERPOINT " @ %obj @ " AT " @ %obj.position);
        return;
      }
    }
  }

  // No cover points that can be moved to, so look for other waypoints
  for (%nIdx = 0; %nIdx < AIPaths.getCount() - 1; %nIdx++)
  {
    %node = AIPaths.getObject(%nIdx);
    %dist = VectorDist(%basePosition,%node.getPosition());

    // If the node is close enough to %basePosition and can see it, find out if it is available
    if (%dist < %radius && !ContainerRayCast(%basePosition,%node.getPosition(),$AIPlayer::SolidMasks))
    {
      if (%array.getIndexFromKey(%node.getPosition()) == -1) // If this position is not already on the list, add it to the list and use it
      {
        %array.add(%node.getPosition(),0);
        return;
      }
      else
      {
        // It already belongs to the array, so try a different node
        continue;
      }
    }
  }
  // If we are this far in the function, we never found a good node. Return -1 so ::setMovePositions() knows it wasn't successful
  return -1;
}