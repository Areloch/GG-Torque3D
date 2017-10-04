//-----------------------------------------------------------------------------
// Torque
// Copyright GarageGames, LLC 2011
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Misc server commands avialable to clients
//-----------------------------------------------------------------------------

function serverCmdSuicide(%client)
{
   if (isObject(%client.player))
      %client.player.kill("Suicide");
}

function serverCmdPlayCel(%client,%anim)
{
   if (isObject(%client.player))
      %client.player.playCelAnimation(%anim);
}

function serverCmdTestAnimation(%client, %anim)
{
   if (isObject(%client.player))
      %client.player.playTestAnimation(%anim);
}

function serverCmdPlayDeath(%client)
{
   if (isObject(%client.player))
      %client.player.playDeathAnimation();
}

// ----------------------------------------------------------------------------
// Throw/Toss
// ----------------------------------------------------------------------------

function serverCmdThrow(%client, %data)
{
   %player = %client.player;
   if(!isObject(%player) || %player.getState() $= "Dead" || !$Game::Running)
      return;
   switch$ (%data)
   {
      case "Weapon":
         %item = (%player.getMountedImage($WeaponSlot) == 0) ? "" : %player.getMountedImage($WeaponSlot).item;
         if (%item !$="")
            %player.throw(%item);
      case "Ammo":
         %weapon = (%player.getMountedImage($WeaponSlot) == 0) ? "" : %player.getMountedImage($WeaponSlot);
         if (%weapon !$= "")
         {
            if(%weapon.ammo !$= "")
               %player.throw(%weapon.ammo);
         }
      default:
         if(%player.hasInventory(%data.getName()))
            %player.throw(%data);
   }
}

// ----------------------------------------------------------------------------
// Force game end and cycle
// Probably don't want this in a final game without some checks.  Anyone could
// restart a game.
// ----------------------------------------------------------------------------

function serverCmdFinishGame()
{
   cycleGame();
}

// ----------------------------------------------------------------------------
// Cycle weapons
// ----------------------------------------------------------------------------

function serverCmdCycleWeapon(%client, %direction)
{
   %client.getControlObject().cycleWeapon(%direction);
}

// ----------------------------------------------------------------------------
// Unmount current weapon
// ----------------------------------------------------------------------------

function serverCmdUnmountWeapon(%client)
{
   %client.getControlObject().unmountImage($WeaponSlot);
}

// ----------------------------------------------------------------------------
// Weapon reloading
// ----------------------------------------------------------------------------

function serverCmdReloadWeapon(%client)
{
   %player = %client.getControlObject();
   %image = %player.getMountedImage($WeaponSlot);
   
   // Don't reload if the weapon's full.
   if (%player.getInventory(%image.ammo) == %image.ammo.maxInventory)
      return;
      
   if (%image > 0)
      %image.clearAmmoClip(%player, $WeaponSlot);
}

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