//-----------------------------------------------------------------------------
// Tactical AI Kit
// Copyright (C) Bryce Carrington
//-----------------------------------------------------------------------------

// aimPatterns.cs
// Scripts for preset aim patterns
//   An aim pattern defines how an NPC can aim for preset actions, e.g.
// Searching left, right, up, down while entering a room, turning 360 degrees to check
// all directions, etc.
//
//   In the functions (AIPlayer::AimPattern[n], the %index argument should be incremented
// at the end. This way, a new index will be used every think tick for a new aim direction.

//------------------------------------------------
// Global Variables
//------------------------------------------------

$AIPatterns::CanAutoStop = true; // Automatically cancel aim patterns when we get an attackObj

//------------------------------------------------
// Starting/Stopping functions
//------------------------------------------------

// Start an AI Player on the aimPattern specified by %type
function AIPlayer::startAimPattern(%this,%type,%index)
{
  %this.aimPatternIndex = 0;
  %this.aimPatternType = %type;
  
  if (%index $= "")
    %index = 0;
  
  %string = "%this.AimPattern" @ %type @ "(" @ %index @ ");";
  eval(%string);
}

// Stops the current aimPattern
function AIPlayer::stopAimPattern(%this,%dontClearAim)
{
  %this.aimPatternIndex = 0;
  %this.aimPatternType = -1;
  
  if (!%dontClearAim)
    %this.clearAim();
}

//------------------------------------------------
// Aim Pattern definition functions
//------------------------------------------------

// Look up, then left and right. Then level aim, then look left and right.
function AIPlayer::AimPattern1(%this,%index)
{
  if ($AIPatterns::CanAutoStop == true && isObject(%this.brain.attackObj))
  {
    %this.stopAimPattern(true);
    return;
  }
  %index = %this.aimPatternIndex;
  switch (%index)
  {
    case 1:
      %this.clearAim();
      %this.aimPatternIndex++;
    case 3:
      %this.doPitch(-20);
      %this.aimPatternIndex++;
    case 6:
      %this.doYaw(320);
      %this.aimPatternIndex++;
		case 9:
			%this.doYaw(80);
			%this.aimPatternIndex++;
		case 15:
			%this.doYaw(320);
			%this.aimPatternIndex++;
		case 18:
			%this.doYaw(80);
			%this.aimPatternIndex++;
		case 21:
			%this.clearAim();
			%this.aimPatternIndex = 0;
		default:
			%this.aimPatternIndex++;
			return;
  }
}
