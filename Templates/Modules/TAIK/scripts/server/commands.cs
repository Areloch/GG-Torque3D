//-----------------------------------------------------------------------------
// Torque
// Copyright GarageGames, LLC 2011
//-----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// Squad leader commands
//----------------------------------------------------------------------------

// Orders the selected AI to move to a location
function serverCmdOrderMove(%client)
{
  %plr = %client.player;
  %squad = %plr.squad;

  if (%plr.getState() $= "Dead" || !%plr.squad || %plr.memberIndex != 0)
  {
    return;
  }
  
  %eyePosition = %plr.getEyePoint();
  %eyeVector = %plr.getEyeVector();
  %eyeVector = VectorNormalize(%eyeVector);
  %outVec = VectorScale(%eyeVector,1000);
  %outPos = VectorAdd(%eyePosition,%outVec);
  %ray = ContainerRayCast(%eyePosition,%outPos,$TypeMasks::InteriorObjectType |
  																						 $TypeMasks::StaticShapeObjectType |
  																						 $TypeMasks::StaticTSObjectType |
                                               $TypeMasks::TerrainObjectType);
  if (%ray)
  {
    %intersect = getWords(%ray,1,3);
    %intersect = VectorAdd(%intersect,"0 0 1");
    %squad.setMovePositions(%intersect,6);
    %squad.setOrders("Move");
      return;
  }
}

// Orders the squad to stop what they're doing and regroup
function serverCmdOrderRegroup(%client)
{
  %plr = %client.player;
  %squad = %plr.squad;

  if (%plr.getState() $= "Dead" || !%plr.squad || %plr.memberIndex != 0)
  {
    return;
  }
  %squad.setOrders("Regroup");
}

// Orders the squad to hold and wait for orders
function serverCmdOrderHold(%client)
{
  %plr = %client.player;
  %squad = %plr.squad;

  if (%plr.getState() $= "Dead" || !%plr.squad || %plr.memberIndex != 0)
  {
    return;
  }
  %squad.setOrders("Hold");
}

// Toggles the rules of engagement from fire at will to return fire only, and vice-versa
function serverCmdOrderROE(%client)
{
  %plr = %client.player;
  %squad = %plr.squad;

  if (%plr.getState() $= "Dead" || !%plr.squad || %plr.memberIndex != 0)
  {
    return;
  }
  %cur = %squad.roe;
  if (%cur == 0)
  {
    warn(%squad @ ": rules of engagement set to return fire only");
    %squad.roe = 1;
  }
  else
  {
    warn(%squad @ ": rules of engagement set to fire at will");
    %squad.roe = 0;
  }
}

//----------------------------------------------------------------------------
// Player position commands
//----------------------------------------------------------------------------

function serverCmdSetCoverPos(%client,%pos)
{
	if (isObject(%client.player))
		%client.player.setCoverPosition(%pos);
}

function serverCmdGetCoverPos(%client)
{
	if (isObject(%client.player))
		return %client.player.getCoverPosition();
	else
		return 0;
}