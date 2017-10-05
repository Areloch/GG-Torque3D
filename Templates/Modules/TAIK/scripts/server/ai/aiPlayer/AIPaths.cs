//-----------------------------------------------------------------------------
// Tactical AI Kit
// Copyright (C) Bryce Carrington
//-----------------------------------------------------------------------------

$AiPaths::AdjDist=8;
$AiPaths::AdjType="HEURISTIC";
$AiPaths::Method=astar;
$AiPaths::Random = 0;

$AiPaths::VisibilityMaxDist = 50; // When doing visiblity scoring, only check for nodes within this distance of the current node

//Key bind for dropping path nodes, navigation nodes, and spawn points
MoveMap.bindCmd(keyboard, "x", "commandToServer(\'DropNode\');", "");

function LoadPaths()
{
   if (isObject(AiPaths))
   {
      AiPaths.loadAdjs("./" @ $Server::MissionFile @ ".dat");
      AiPaths.RenderAll(true);
   }
}

// Sets the move destinations of all markers in a path
function setPathMoveSpeeds(%path,%speed)
{
  %count = %path.getCount();
  for(%i = 0;%i < %count;%i++)
  {
    %obj = %path.getObject(%i);
    %obj.moveSpeed = %speed;
  }
}

// Creates a new path, and sets $PathToAddTo so that we can immediately start dropping nodes
function newAIPath()
{
  if (!isObject(Paths))
  {
    new SimGroup(Paths) {};
    MissionGroup.add(Paths);
  }
  %ct = Paths.getCount()+1;
  %name = "Path" @ %ct;
  
  %path = new Path(%name) { };
  Paths.add(%path);
  $PathToAddTo = %path;
  error("Created path " @ %name);
}

// Creates a new path node and adds it to the path specified by $PathToAddTo, with
//   optional move speed $PathAddMoveSpeed.
function ServerCmdDropPathNode(%client)
{
   if (!$PathToAddTo)
   {
     error("Specify $PathToAddTo first!");
     return;
   }
   
   %position=VectorAdd(%client.player.position, "0 0 0.2");
   %node = new Marker() 
   {
      position = %position;
   };
   
   if ($PathAddMoveSpeed)
     %node.moveSpeed = $PathAddMoveSpeed;
   if ($PathAddStopTime)
     %node.stopTime = $PathAddStopTime;
   
   %node.setTransform(%client.player.getTransform());
   $PathToAddTo.add(%node);
   error("Added path node " @ %node);
}

// Creates a new navigation node
function ServerCmdDropNode(%client)
{
	if ($minNodeRange)
	  %nodeRange = $minNodeRange;
	else
	  %nodeRange = 0;

   %height = 0.2;
   if ($AIPaths::NodeHeight !$= "")
     %height = $AIPaths::NodeHeight;
   
	 %position=VectorAdd(%client.player.position, "0 0 " @ %height);
   if (isObject(AiPaths)) {
      for (%aipc=0;%aipc<AiPaths.getCount();%aipc++) {
         %node = AiPaths.getObject(%aipc);
         if (VectorDist(%position, %node.getPosition()) <= %nodeRange) {
            echo("AI Path node drop denied at " @ %position @ " - there is already another node close by");
            return;
         }
      }
   }
  
   %node = new AIPathNode() 
   {
      dataBlock = PathNode;
      position = %position;
   };
   
   if (!isObject(AiPaths))
   {
      new AiPathGroup(AiPaths) {};
      AiPaths.renderAll(true);
      
      MissionGroup.add(AiPaths);
   }
   
   //AiPaths.addObjectSafe(%node, $AiPaths::AdjType, $AiPaths::AdjDist);
   AIPaths.schedule(250,addObjectSafe,%node,$AIPaths::AdjType,$AIPaths::AdjDist);
   echo("AIPaths: " @ AIPaths.getCount() @ ", id: " @ %node);
   $AIPaths::LastDroppedNode = %node;
}
// Removes the last node we dropped
function removeRecentNode()
{
  if ($AIPaths::LastDroppedNode)
  {
    AIPaths.removeObjectSafe($AIPaths::LastDroppedNode);
    $AIPaths::LastDroppedNode.schedule(1000,delete);
  }
}

function SavePaths()
{
  if (isObject(AiPaths))
      AiPaths.saveAdjs("./" @ $Server::MissionFile @ ".dat");
}

// Waypoint Visiblity
function getWayPointVisValues()
{
  if (!isObject(AIPaths))
  {
    echo("AIPaths does not exists, create an AIPathGroup and name it AIPaths first.");
    return -1;
  }
  %count = AIPaths.getCount();
  if (%count < 1) 
  {
    error("AIPaths group is empty!");
    return 0;
  }
  // Iterate through the AIPaths list
  for (%i = 0; %i < %count; %i++)
  {
    %node = AIPaths.getObject(%i);
    %node.visScore = 0;
    // Iterate thru all other nodes and cast rays to them if they are close
    for (%rayNodeIdx = 0; %rayNodeIdx < %count; %rayNodeIdx++)
    {
      %rayNode = AIPaths.getObject(%rayNodeIdx);
      %dist = VectorDist(%rayNode.getPosition(),%node.getPosition());
      if (%raynode != %node && %dist < $AIPaths::VisibilityMaxDist)
      {
        %standRay = ContainerRayCast(VectorAdd(%node.getPosition(),"0 0 2"),VectorAdd(%rayNode.getPosition(),"0 0 2"),$AIPlayer::SolidMasks);
        %crouchRay = ContainerRayCast(VectorAdd(%node.getPosition(),"0 0 0.7"),VectorAdd(%rayNode.getPosition(),"0 0 0.7"),$AIPlayer::SolidMasks);
        if (!%ray)
        {
          %node.visScore++;
        }
        if (!%crouchRay)
        {
          %node.crouchVisScore++;
        }
      }
    }
    // Now, adjust this based on foliage
    if (isObject(FoliageGroup))
    {
      for(%f = 0;%f < FoliageGroup.getCount();%f++)
      {
        %fol = FoliageGroup.getObject(%f);
        // Get the average coverage distance of this foliage
        %area = (%fol.outerRadiusX + %fol.outerRadiusY) / 2;
        // See if %node is within the area
        %distToFol = VectorDist(%node.getPosition(),%fol.getPosition());
        if (%distToFol <= %area)
        {
          %terrHeight = getTerrainHeight(%node.getPosition());
          %folHeight = %fol.maxHeight;
          %nodeHeight = getWord(VectorAdd(%node.getPosition(),"0 0 0.05"),2);
          if ((%nodeHeight - %terrHeight) <= %folHeight)
          {
            // Adjust the weight based on the camoAmount
            if (%fol.camoAmount $= "")
              %node.crouchVisScore *= 0.2; // If camoAmount is not specified on this foliage, default 0.2
            else
              %node.crouchVisScore *= %fol.camoAmount;
          }
        }
      }
    }
  }
  error("Visibility scoring finished, save the mission to preserve this data.");
}


// Makes a grid of path nodes around lower left corner %start, with distance %len, and side number %num
function PathGrid(%start,%len,%num)
{
   if (!isObject(AiPaths))
   {
      new AiPathGroup(AiPaths) {};
      AiPaths.renderAll(true);
      
      MissionGroup.add(AiPaths);
   }

  %start = setWord(%start,2,"0");
  // Add nodes along the x-axis
  for (%x = 0; %x < %num; %x++)
  {
    // From here, we add nodes along the y-axis
    for (%y = 0; %y < %num; %y++)
    {
      %xVector = %x*%len;
      %yVector = %y*%len;
      // Figure out where we should place the node
      %nodePos = VectorAdd(%start,%xVector SPC "0 0");
      %nodePos = VectorAdd(%nodePos, "0" SPC %yVector SPC "0");
      %nodePos = VectorAdd(%nodePos, "0 0" SPC (getTerrainHeight(%nodePos)+0.2));
      // Drop a node
      %n = new AIPathNode() {
        position = %nodePos;
        datablock = "PathNode";
      };
      AIPaths.addObjectSafe(%n,$AIPaths::AdjType,$AIPaths::AdjDist);
    }
  }
}