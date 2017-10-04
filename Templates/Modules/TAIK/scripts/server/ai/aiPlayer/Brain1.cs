//-----------------------------------------------------------------------------
// Tactical AI Kit
// Copyright (C) Bryce Carrington
//-----------------------------------------------------------------------------

// Brain1.cs
// Scripts for an AI Player's brain
// Contains things like cover searching, the state machine, etc

//----------------------------------------
// Global Variables
//----------------------------------------

//Thinking
$Brain1::ThinkDelay = 400;

// Weapon loadout
$Brain1::PrimaryWeapons = "TAIKGun"; // The primary weapon can be any one of these by random
$Brain1::SecondaryWeapons = "TAIKGun"; // The secondary weapon can be any one of these by random

// Visibility
$Brain1::MinVisRange = 30; // Detect all non-moving objects within this range
$Brain1::MaxVisRange = 90; // Detect all moving objects within this
$Brain1::MinAlertVisRange = 110; // Detect all non-moving objects within this range if we are alert
$Brain1::MaxAlertVisRange = 250; // Detect all moving objects within this range if we are alert
$Brain1::MaxProcessedObjects = 6; // Only check this many objects for visiblity in ::getVisObjects()
$Brain1::MaxProcessedTeammates = 2; // Only process up to this many teammates in ::getVisObjects()
$Brain1::TreeHeight = 5; // Since we cannot tell the height of an object in a shape replicator, assume that all trees are taller than this
$Brain1::CloseObjDist = 8; // Objects closer than this should be noticed even when only a part of them is visible
$Brain1::VeryCloseVisObjDist = 2; // If our attackObj is closer to us than this, see him no matter what
$Brain1::FiringVisWeight = 1.5; // Firing players are more visible
$Brain1::LastKnownEnemyVisDist = 1.5; // If an enemy player is within this distance of our last known enemy position, he will be seen
$Brain1::LastKnownWeight = 5; // If within the above distance, multiply the visibility weight by this
$Brain1::UnsureVeryVisDist = 5; // If we're unsure of a hidden object, it will become more visible when we are within this distance of that position
$Brain1::UnsureVeryVisWeight = 20; // If within the above distance, multiply the visibility weight by this to make it easier to see
$Brain1::BodyVisWeight = 1.1; // Dead bodies are a little easier to spot
$Brain1::UnsureVisWeight = 0.2; // If someone we see has a visiblity weight less than this, we should investigate
$Brain1::Fov = 100; // Our field of view (in degrees)
$Brain1::OpenVisiblity = 0.9; // An object which we can see at least this percent of is considered to be standing in the open

// Hearing
$Brain1::MaxAudRange = 400; // Maximum distance we should check for hearing
$Brain1::MaxAudFireRange = 500; // Maximum distance we can hear enemy gunfire
$Brain1::MaxAudPlayerRange = 18; // Maximum distance we can hear player movement
$Brain1::AudPlayerVel = 3.75; // Hear players moving faster than this
$Brain1::ChanceOfHearingPlayer = 2; // 1 in 2 chance of us hearing moving players
$Brain1::MaxAudShoutHelpRange = 40; // Hear reacting teammates within this range
$Brain1::MaxAudShoutSuspiciousRange = 15; // Hear teammates who noticed something suspicious within this range
$Brain1::HearPlayerTolerance = 5; // Hear players again only if their position changes at least this many meters

// Cover search, cover usage, and stress level management
$Brain1::CoverEnemyCloseDist = 6; // If our enemy is this close or closer to me, ditch cover and kill him however we can
$Brain1::AttackFindCoverDot = 0.6; // Find cover from an enemy automatically if he is facing this way
$Brain1::LowAmmoPercentage = 0.5; // If the clip in our gun is less than this percent full, reload if we don't have a target
$Brain1::NumCoverSearches = 40; // How many times to cycle through cover markers unsuccessfully before giving up
$Brain1::NumMarkersToSearch = 10; // Only look at this many cover markers at a time when cover searching
$Brain1::CoverMoveTolerance = 4; // Calculate a new path to cover if its position changes more than this
$Brain1::CoverMoveCrouchDist = 6; // When moving to cover, crouch if we are closer than this
$Brain1::CoverRange = 50; // Only run for cover if it is within this distance from our position
$Brain1::CoverRunSuppressDist = 15; // Only lay down suppressing fire if we're closer than this to cover we're running to
$Brain1::CoverRunSnapForwardDist = 5; // If our cover marker is closer than this to our position while running for cover, aim the direction our cover marker is rotated
$Brain1::CoverPushSpeed = 4.5; // How much to bump an NPC out of horizontal (shoot-around) cover when he leaves it
$Brain1::StepOutofCoverDist = 1.75; // How many meters we move to return fire from non-muzzleclear cover
$Brain1::ProjectileStressInc = 7; // Stress increases this much when we see a projectile
$Brain1::ClosestPreferredAttackRange = 5; // Don't get cover closer than this to our enemy if we can help it
$Brain1::ExplosionStressInc = 10; // Stress increases this much when we a bullet hits near us
$Brain1::PinnedStressLevel = 20; // Pinned down if our stress reaches this or above
$Brain1::AssistObjPinnedStress = 50; // We need to lay down some covering fire for our assistObj if his stress is higher than this
$Brain1::DropStressRateMin = 4; // The minimum amount of stress to lose per think
$Brain1::DropStressRateMax = 12; // The maximum amount of stress to lose per think
$Brain1::MaxAttackActiveCountToSuppress = 10; // If we have gone this many think cycles without an enemy sighting, stop suppressing
$Brain1::InvestigateAttackActiveCount = 50; // If our attackActiveCount reaches this, assume the enemy has hidden, so go find him
$Brain1::InvestigateDistance = 1; // Only investigate if the last enemy location is closer than this hack: Was 140, disabling investigating for now
$Brain1::ChanceOfInvestigate = 30; // After InvestigateAttackActiveCount is reached, the chance of investigating on a think cycle. Helps to randomize things so multiple NPCs don't investigate simultaneously

// Movement and stuck avoidance
$Think::IsCrowdingDist = 1.2; // If we are closer than this to one of our friends, we are crowding them, so back off
$Brain1::PushVelocity = 3.5; // Players (including ourselves) can be pushed away at this velocity to avoid crowding
$Brain1::StuckDist = 0.4; // If we are farther than this from our destination and we are not moving, we are stuck  //WAS 1.5
$Brain1::StuckVelocity = 0.2; // If we are slower than this speed while trying to move, we may be stuck
$Brain1::MaxStuckCount = 3; // If we think we are stuck this many times or more, try to get un-stuck
$Brain1::StuckPushImpulse = 700; // The impulse amount we should push ourselves in a random direction to get un-stuck

// Melee Attack
$Brain1::MeleeRange = 3; // Enemies closer than this are close enough for a melee attack
$Brain1::MeleeDamageMin = 90; // Minimum damage that our melee attack does
$Brain1::MeleeDamageMax = 1500; // Maximum damage that our melee attack does
$Brain1::MeleeTime = 200; // Delay before applying damage (sync the damage with the animation)

// Civilians
$Brain1::KillCivilians = false; // Should we deliberately target and shoot civilians?

// Chance/random
// I advise leaving these alone
$Brain1::VeryLargeChanceRandom = 5;
$Brain1::LargeChanceRandom = 10;
$Brain1::MediumChanceRandom = 30;
$Brain1::SmallChanceRandom = 50;

// Close Attack
$Brain1::ChaseDist = 20; // If our threat is farther than this from us, chase after him
$Brain1::PursueLostContactDist = 70; // Don't move to last known enemy positions farther away than this
$Brain1::CloseAttackInvestigateDist = 50; // Investigate only if we are closer than this to the last known enemy position
$Brain1::CloseAttackSuppressActiveCount = 20; // Don't suppress if we don't see an enemy for at least this many think cycles

// Assist
$Brain1::AssistObjFollowDist = 5; // Follow our assistObj by this distance
$Brain1::AssistActiveCount = 30; // Run the assist state this many times with no enemy activity before giving up
$Brain1::AssistObjMoveDist = 10; // If our assistObj moves this far from his last position, reset our assistActiveCount so we keep following him

// Aim accuracy
$Brain1::AimInaccuracyMin = 0.5/1000; // Minimum aim innacuracy
$Brain1::AimInaccuracyMax = 15/1000; // Maximum aim innacuracy (when stress is high and/or the player is moving)

// Reaction to threats
$Brain1::ThreatPosTolerance = 12; // Only react if we notice a new threat at least this far away from the last one
$Brain1::ReactFindAttackerDist = 140; // If the path distance from our position to the presumed enemy position is lower than this, move toward our attacker to find him
$Brain1::MinReactMoveDist = 4; // Minimum distance to run after reacting
$Brain1::MaxReactMoveDist = 10; // Minimum distance to run after reacting
$Brain1::ReactMoveCrouchDist = 5; // Crouch when cover is closer than this distance to our position
$Brain1::CheckTime = 1000; // How many milliseconds to randomly peek out of cover when looking for an attacker
$Brain1::ShotsFiredDangerDist = 140; // If we hear gunfire within this range, go to the location and figure out what's going on
$Brain1::FireReactionDangerDist = 65; // If we are at least this distance from a location someone has come under fire, search for the attacker rather than hide
$Brain1::ExplosionDangerDist = 25; // Try to stay this far away from an area where something just blew up
$Brain1::MinSniperGuessDist = 80; // When casting rays for a sniper position guess, don't use a point if it's closer to us than this
$Brain1::MaxSniperGuessDist = 400; // When casting rays for a sniper position guess, don't use a point if it's farther from us than this
$Brain1::SquadReactTime = 40; // How many think ticks squad AI will spend reacting before moving on their objective again

// Flanking
$Brain1::FlankTolerance = 1; // Calculate a new flank path if the flank position has changed more than this

// Squad
$Brain1::ObjectiveCoverCheckDistance = 8; // Every time we travel this far in stateObjective, we run a cover search to prepare for any possible enemy engagement
$Brain1::SquadHoldDist = 25; // After moving this far, the squad leader should have the squad regroup to keep everyone together
$Brain1::MoveUpMinDist = 5; // An AI squad leader should only send the squad to non-visible nodes at least this far away
$Brain1::MoveUpAgainDist = 15; // Make sure that we are at least this far from the last place an AI leader ordered the squad to move up before allowing the order again
$Brain1::ObjectiveThreatDistGuess = 25; // Guess that an enemy would be this far ahead while moving. Used for cover-to-cover movement in stateObjective.
$Brain1::ObjectiveCoverSearchDistance = 50; // Different from ::ObjectiveCoverCheckDistance. This is how far out we search for cover
$Brain1::RegroupTolerance = 5; // Calculate a new path when regrouping if the squad leader's position has changed more than this
$Brain1::RegroupCloseTolerance = 2; // If the squad leader's position changes this much when we can see him, move again
$Brain1::ObjectiveTolerance = 10; // Calculate a new path when moving towards the objective if the objective's position has changed more than this
$Brain1::ObjectiveMoveTolerance = 1; // Calculate a new path to the ordered move position if the position has changed more than this
$Brain1::SquadFollowDist = 4; // How close to get to the squad leader
$Brain1::DangerousNodeVisScore = 27; // Move carefully through highly visible areas

//----------------------------------------

// Typemasks for easy access in raycasts
$Brain1::SolidMasks = $TypeMasks::InteriorObjectType |
                      $TypeMasks::TerrainObjectType |
                      $TypeMasks::ItemObjectType |
          //$TypeMasks::PlayerObjectType |
                      $TypeMasks::StaticTSObjectType |
                      $TypeMasks::StaticShapeObjectType |
                      $TypeMasks::VehicleObjectType |
                      $TypeMasks::WaterObjectType;

$Brain1::CoverMasks = $TypeMasks::InteriorObjectType |
                      $TypeMasks::ItemObjectType |
                      $TypeMasks::StaticTSObjectType |
                      $TypeMasks::StaticShapeObjectType |
                      $TypeMasks::VehicleObjectType;

$Brain1::OpaqueMasks = $TypeMasks::InteriorObjectType |
                       $TypeMasks::TerrainObjectType |
                       $TypeMasks::StaticTSObjectType |
                       $TypeMasks::StaticShapeObjectType |
                       $TypeMasks::ItemObjectType |
                       $TypeMasks::VehicleObjectType |
                       $TypeMasks::WaterObjectType;


//----------------------------------------
// Setting up the brain
//----------------------------------------

// This function creates the brain
// The id of this set is returned and saved with the AIPlayer so it can use this brain
function Brain1::create(%player)
{
  %brain = new ScriptObject() {
    class = "Brain1";

    player = %player;
    name = "NPC";
    state = "NONE";
    attackObj = 0;
    prevAttackObj = 0;
    assistObj = 0;
    reloading = 0;

    furtherSearching = 0; // So we know when to search hiding spots when hunting
    attackActiveCount = 0; // When we're suppressing from cover, this number goes up so we know when to search for the enemy
    huntNodeIndex = -1;

    lastState = "NONE";
    lastDamageLevel = %player.getDamageLevel();
    lastKnownEnemyPos = 0; // Last known location of an enemy when we are using cover
    assumedEnemyPosition = 0;
    audFireLocation = 0;
    lastAlertness = -1;

    movingToCover = 0;
    coverPos = 0;
    returnFirePos = 0;
    coverMarker = 0;

    threatPosition = 0;
    threatType = "";
    lastThreatPosition = 0;
    lastThreatType = "";

    followsPaths = true;
    pathPause = false;
    alertness = 0;

    invPosition = 0;
    invStartPosition = 0;

    priorityMoveLocation = 0;

    squadStopPosition = 0;
    movePosition = 0;
    lastMovePosition = 0;

    canWander = true;
  };

  // add self to cleanup and return
  MissionCleanup.add(%brain);
  return %brain;
}

// Gives the NPC a primary and secondary weapon
function Brain1::initLoadout(%this,%weapon1,%weapon2)
{
  %this.player.weapon1 = %weapon1 @ "Image";
  %this.player.weapon1ammo = %this.player.weapon1.ammo;
  %this.player.weapon2 = %weapon2 @ "Image";
  %this.player.weapon2ammo = %this.player.weapon2.ammo;
  %this.player.currentWeapon = 0; // 0 = primary weapon, 1 = secondary weapon
  %this.weapon1 = %weapon1 @ "Image";
  %this.weapon1ammo = %this.weapon1.ammo;
  %this.weapon2 = %weapon2 @ "Image";
  %this.weapon2ammo = %this.weapon2.ammo;
  %this.currentWeapon = 0; // 0 = primary weapon, 1 = secondary weapon

  %this.player.weapon = %weapon1 @ "Image";
  %this.player.ammo = %this.player.weapon.ammo;
  %this.weapon = %weapon1 @ "Image";
  %this.ammo = %this.weapon.ammo;

  echo("brain.weapon = " @ %this.weapon @ ", brain.ammo = " @ %this.ammo);

  %this.player.mountimage(%weapon1 @ "Image",0);
  %this.player.setInventory(%this.weapon1.ammo,%this.weapon1.clipSize);
  %this.player.setInventory(%this.weapon2.ammo,%this.weapon2.clipSize);
}

// This function wakes up the brain
// Called before the first call to think
function Brain1::wake(%this)
{
  %this.name = %this.player.getShapeName();
  %this.posHome = %this.player.getPosition();
  %this.posPatrol = %this.posHome;

  // Choose a random primary weapon
  %pWeaponCount = getWordCount($Brain1::PrimaryWeapons) - 1;
  %rand = getRandom(0,%pWeaponCount);
  %myPrimaryWeapon = getWord($Brain1::PrimaryWeapons,%rand);

  // Choose a random secondary weapon
  %sWeaponCount = getWordCount($Brain1::SecondaryWeapons) - 1;
  %rand = getRandom(0,%sWeaponCount);
  %mySecondaryWeapon = getWord($Brain1::SecondaryWeapons,%rand);

  %this.initLoadout(%myPrimaryWeapon,%mySecondaryWeapon);
  %this.reloadWeapon(true); // Get ready to fight
  %this.spreadDefault = $Brain1::AimInaccuracyMin;
  //%this.player.createCList();
  //%this.player.createHList();

  %this.player.fov = $Brain1::Fov;
  %this.player.minViewDist = $Brain1::MinVisRange;
  %this.player.maxViewDist = $Brain1::MaxVisRange;
  %this.player.visMod = 1;
  %this.player.thinkDelay = $Brain1::thinkDelay;
}

//----------------------------------------
// Thinking
//----------------------------------------

// The main thinking loop, called by the AIPlayer thinking loop
function Brain1::think(%this)
{
  %plr = %this.player;

  //----------------------------------------------------------------------
  // See if we've been damaged
  //----------------------------------------------------------------------
  %curDamage = %plr.getDamageLevel();
  %lastDamage = %this.lastDamageLevel;
  if (%curDamage > %lastDamage)
  {
    %this.isTakingDamage = true;
    error(%plr @ ": taking damage!");
    %this.setStress(100); // Getting shot should distract you a bit.
  }
  else
  {
    %this.isTakingDamage = false;
  }
  %this.lastDamageLevel = %curDamage;

  //----------------------------------------------------------------------
  // If our enemy is very close, stop everything and attempt to perform a
  //   melee attack.
  //----------------------------------------------------------------------
  if (%this.veryLargeChanceRandom() && isObject(%this.attackObj) && %this.attackObj.getState() !$= "Dead" && Vectordist(%this.attackObj.getPosition(),%plr.getPosition()) <= $Brain1::MeleeRange)
  {
    if (!%this.isMelee)
    {
      %this.doMelee(%this.attackObj);
    }
    return;
  }

  //----------------------------------------------------------------------
  // If we are at our threat position, don't stare at it
  //----------------------------------------------------------------------
  if (VectorDist(%plr.getPosition(),%this.threatPosition) < 8 && !isObject(%this.attackObj))
  {
    %plr.clearAim();
  }

  //----------------------------------------------------------------------
  // If we have a coverObj, decide if we can see him so we don't have to
  //   test for visibility too often
  //----------------------------------------------------------------------
  %this.coverObjIsVisible = false;
  if (isObject(%this.coverObj))
  {
    %isVis = %this.isObjVisible(%this.coverObj);
    if (%isVis)
    {
      %this.coverObjIsVisible = true;
    }
  }

  //----------------------------------------------------------------------
  // Alert other AI Players if we are in trouble
  //----------------------------------------------------------------------
  if (%this.threatType !$= "" && !%plr.isAsleep && !$noAlertOthers)
  {
    %chanceOfShoutOut = %this.largeChanceRandom();
    if (%chanceOfShoutOut == 1)
    {
      AlertAIPlayers(%plr.getPosition(),$Think4::MaxAudShoutHelpRange,0,%this.threatType,%this.alertness,0,%plr,%this.threatPosition,-1,%plr.team,(isObject(%plr.squad)));
    }
  }

  //----------------------------------------------------------------------
  // Save our last alert level
  //----------------------------------------------------------------------
  %this.lastAlertness = %this.alertness;

  //----------------------------------------------------------------------
  // Check for grenades that might kill us
  //----------------------------------------------------------------------
  %this.checkForGrenades();

  //----------------------------------------------------------------------
  // Make sure we are not crowding other teammates
  //----------------------------------------------------------------------
  %this.avoidCrowding();

  //----------------------------------------------------------------------
  // Squad functions
  //----------------------------------------------------------------------
  if (%plr.squad)
    %this.keepInformed(); // Alerts us of enemy players that other squad members are attacking

  //----------------------------------------------------------------------
  // See if we are stuck
  //----------------------------------------------------------------------
  if (!$noStuckAvoid)
  {
    if (%this.stuckCount < $Brain1::MaxStuckCount)
    {
      if (!%plr.isStopped() && %plr.getMoveDestination() != -1 && %plr.getMoveDestination() !$= "" && %plr.getMoveDestination() !$= "0 0 0" && mAbs(VectorLen(%plr.getVelocity()) < $Brain1::StuckVelocity) && VectorDist(%plr.getPosition(),%plr.targetPathDestination) > $Brain1::StuckDist)
      {
        if (VectorDist(%plr.getPosition(),%this.returnFirePos) > 4 && VectorDist(%plr.getPosition(),%this.coverPos) > 4)
        {
          // If we are trying to move but we're travelling less than $Brain1::StuckVelocity, we increment the stuck count
          echo(%plr @ ": incrementing stuck count, velocity is " @ VectorLen(%plr.getVelocity));
          %this.stuckCount++;
        }
      }
      else
      {
        %this.stuckCount = 0; // We were either not stuck in the first place or we got un-stuck
      }
    }
    else // Stuck count is high enough, let's try to free ourselves
    {
      echo(%plr @ ": attempting to get unstuck, stuck count is " @ %this.stuckCount);
      %speed = $Brain1::StuckPushImpulse;
      %vector = getRandom(-1,1) SPC getRandom(-1,1) SPC "0";
      %pushVector = VectorScale(%vector,%speed); // The vector we should push ourselves in order to get free
      %plr.applyImpulse(%plr.getPosition(),%pushVector);
      %pathDest = %plr.targetPathDestination;
      %plr.targetPathDestination = "";
      %plr.curNode = -1;
      %plr.endNode = -1;
      %plr.setOnPath(%pathDest);
      %this.stuckCount = 0; // So we have time to see a result before trying to get unstuck again
      if (%plr.getMoveSpeed() == 0)
        %plr.setMoveSpeed(1.0);
    }
  }

  //----------------------------------------------------------------------
  // Lower stress
  //----------------------------------------------------------------------
  %n = getRandom($Brain1::DropStressRateMin,$Brain1::DropStressRateMax);
  %this.lowerStress(%n);

  //----------------------------------------------------------------------
  // Figure out visibility capabilities and gather information about the nearest important object
  //----------------------------------------------------------------------
  if (!%plr.isAsleep)
    %this.getVisObjects();

  if (%this.alertness > 1) // Have higher visibility if we are alert
  {
    %plr.minViewDist = %plr.visMod * $Brain1::MinAlertVisRange;
    %plr.maxViewDist = %plr.visMod * $Brain1::MaxAlertVisRange;
  }
  else
  {
    %plr.minViewDist = %plr.visMod * $Brain1::MinVisRange;
    %plr.maxViewDist = %plr.visMod * $Brain1::MaxVisRange;
  }

  //----------------------------------------------------------------------
  // Set variables that only last one think tick
  //----------------------------------------------------------------------
  if (%this.attackObj != %this.prevAttackObj && %plr.enemyArray.countKey(%this.attackObj) == 0)
    %this.newContact = true;
  else
    %this.newContact = false;

  //----------------------------------------------------------------------
  // Perform a specified aim pattern
  //----------------------------------------------------------------------
  if (%plr.aimPatternType != -1)
  {
    %string = "%plr.aimPattern" @ %plr.aimPatternType @ "();";
    eval(%string);
  }

  //----------------------------------------------------------------------
  // Set the priority move location if our spawn marker says that's what we should do
  //----------------------------------------------------------------------
  if (VectorDist(%plr.spawnNode.moveDestination,%plr.getPosition()) > 3 && %plr.spawnNode.highPriorityMove)
  {
    if (%this.reachedPriorityMoveLoc != true)
      %this.priorityMoveLocation = %plr.spawnNode.moveDestination;
    else
      %this.priorityMoveLocation = 0;
  }

  //----------------------------------------------------------------------
  // Calculate weapon projectile spread based on stress and velocity
  //----------------------------------------------------------------------
  if (VectorLen(%plr.getVelocity()) >= 3)
  {
    %moveSpread = $Brain1::AimInaccuracyMax;
  }
  %spreadMin = $Brain1::AimInaccuracyMin;
  %spreadMax = $Brain1::AimInaccuracyMax;
  %stressSpread = (%spreadMax-%SpreadMin)/(100) * %this.stress + %spreadMin;
  %spread = %moveSpread + %stressSpread;
  %this.spread = %spread;

  //----------------------------------------------------------------------
  // See if our threat type has changed
  //----------------------------------------------------------------------
  if (%this.lastThreatType !$= %this.threatType)
    %this.threatTypeChanged = true;
  else
    %this.threatTypeChanged = false;

  %this.lastThreatType = %this.threatType;

  //----------------------------------------------------------------------
  // Work with the state system to perform the AI behavior
  //----------------------------------------------------------------------
  // See if our state has changed
  if (%this.lastState !$= %this.state)
  {
    %this.stateChanged = true;
    %this.attackActiveCount = 0;
    %plr.stopAimPattern();
  }
  else
  {
    %this.stateChanged = false;
  }
  // Do the state behavior
  if (!$noStateChange)
  {
    if (!%plr.isAsleep)
    {
      %this.lastState = %this.state;
      switch$( %this.state)
      {
        case "NONE":
          %this.stateIdle();
        case "FINDCOVER":
          %this.stateFindCover();
        case "REACT":
          %this.stateReact();
        case "SQUADREACT":
          %this.stateSquadReact();
        case "HUNT":
          %this.stateHunt();
        case "GRENADE":
          %this.stateGrenade();
        case "CLOSEATTACK":
          %this.stateCloseAttack();
        case "ASSIST":
          %this.stateAssist();
        case "PRIORITYMOVE":
          %this.statePriorityMove();
        case "INVESTIGATE":
          %this.stateInvestigate();
        case "ASSIST":
          %this.stateAssist();
        case "REGROUP":
          %this.stateRegroup();
        case "OBJECTIVE":
          %this.stateObjective();
        case "MOVE":
          %this.stateMove();
        case "HOLD":
          %this.stateHold();
        default: %this.state = "NONE";
      }
    }
  }

  //----------------------------------------------------------------------
  // Hear
  //----------------------------------------------------------------------
  %this.thinkAud();

  //----------------------------------------------------------------------
  // Look at the node ahead of the node we're moving to so that we
  //   strafe around corners
  //----------------------------------------------------------------------
  if (VectorLen(%plr.getVelocity()) > 1 && !isObject(%this.coverMarker) && !isObject(%this.attackObj) && !isObject(%this.evadeObj) && %this.alertness > 1 && !%plr.aimPatternType)
  {
    %curNodeIdx = %plr.curNode; // Node we are moving to
    %nextNodeIdx = %plr.curNode + 1; // The node after the node we are moving to
    if (%nextNodeIdx < %plr.pathNodeArray.count() - 1)
    {
      %curNode = %plr.pathNodeArray.getKey(%curNodeIdx);
      %aheadNode = %this.getHiddenNodeAhead();
      if (isObject(%curNode) && isObject(%aheadNode))
      {
        %dist = VectorDist(%plr.getPosition(),%aheadNode.getPosition());
        %plr.clearAim();
        %plr.setCoverPosition(5);
        %plr.setAimLocation(%aheadNode.getPosition());
      }
      else
      {
        %plr.clearAim();
      }
    }
    else
    {
      %plr.clearAim();
    }
  }

  %this.hasOrders = (%this.orderSuppressPos || %this.orderMovePos);
}

// Another think function that looks for audible objects
function Brain1::thinkAud(%this)
{
  %plr = %this.player;
  if (%plr.getState() $= "Dead")
    return;

  InitContainerRadiusSearch(%plr.getTrueEyePoint(),$Brain1::MaxAudPlayerRange,$TypeMasks::PlayerObjectType);
  while((%obj = ContainerSearchNext()) != 0)
  {
    if (%plr == %obj) // Ignore self and look at the next found object
      continue;

    // Don't process sleeping NPCs
    if (%obj.isAsleep == true)
      continue;

    // Only listen for enemy players
    if (%obj.team != %plr.team && %obj.team != -1)
    {
      %shouldHearPlayer = getRandom(0,$Brain1::ChanceOfHearingPlayer);
      if (%this.alertness > 1)
        %shouldHearPlayer = 1;
      if (%obj.getState() !$= "Dead" && %shouldHearPlayer == 1 && VectorDist(%plr.getPosition(),%obj.getPosition()) <= $Brain1::MaxAudPlayerRange && VectorLen(%obj.getVelocity()) >= $Brain1::AudPlayerVel) // Player is close enough and moving
      {
        if (!%plr.squad && VectorDist(%obj.getPosition(),%this.threatPosition) >= $Brain1::HearPlayerTolerance)
        {
          echo("line 557--NPC " @ %plr @ " hears enemy player " @ %obj);
          if (%this.alertness < 2)
          {
            %this.threatType = "Player";
            %this.threatPosition = %obj.getPosition();
            %this.myPosWhenHeard = %plr.getPosition(); // Save the position we were at when we heard the threat for more accurate searches
            %this.setAlertness(1);
            %plr.schedule(%plr.thinkDelay,thinkAud);

            %this.state = "HUNT";
          }
          else
          {
            if (%obj == %this.attackObj || !isObject(%this.attackObj))
            {
              %this.assumedEnemyPosition = %obj.getWorldBoxCenter();
              %this.threatPosition = %obj.getWorldBoxCenter();
              %this.threatOrigin = %obj.getWorldBoxCenter();
              %plr.setAimLocation(%obj.getWorldBoxCenter());
            }
          }
          return; // only hear the closest enemy player
        }
      }
    }
  }
}

// See if our muzzle vector is blocked by a teammate, and prohibit firing
// if there is someone in the way
function Brain1::CheckLineOfFire(%this)
{
  %plr = %this.player;

  if (%plr.getState() $= "Dead")
    return;

  //if (%plr.isAsleep == false)
    //%this.lineOfFireSched = %plr.schedule(getrandom(300,500),checkLineOfFire);
  //else
    //%this.lineOfFireSched = %plr.schedule(10000,checkLineOfFire);

  if (%this.attackObj) // && %plr.isImageFiring(0) == true)
  {
    %plr.isFriendlyInLOS = false;

    // First, do a basic check in front of us
    %muzzlePoint = %plr.getTrueMuzzlePoint(0);
    %muzzleVector = %plr.getTrueMuzzleVector(0);
    %endVec = VectorScale(VectorNormalize(%muzzleVector),500);
    %endPos = VectorAdd(%endVec,%muzzlePoint);
    //%obviousDanger = ContainerRayCast(%muzzlePoint,%endPos,$TypeMasks::PlayerObjectType,%plr);
    %possibleTeammate = firstWord(%obviousDanger);

    // Check all of our teammates to see if they are inside our cone of fire
    for (%t = 0;%t < PlayerArray.count();%t++)
    {
      %obj = PlayerArray.getKey(%t);
      // Make sure that he is actually a teammate, that we are not checking ourself,
      //   that he is not a civilian (unless we are supposed to kill them), that
      //   we are not already attacking them, and make sure that he's alive.
      if (%obj.team != %plr.team && !%obj.isCivilian)
        continue;
      if (%obj.isCivilian && $Brain1::KillCivilians == true)
        continue;
      if (%obj == %this.attackObj)
        continue;
      if (%obj == %plr || %obj.isAsleep == true || %obj.getState() $= "Dead")
        continue; // if not, check the next object

      // Vector math....horray.
      // Check the vector to the teammate against our muzzle vector to see if we
      //   are aiming in the direction of this teammate
      %vectorToTeammate = VectorSub(%obj.getWorldBoxCenter(),%plr.getWorldBoxCenter());

      // Scale up the muzzle vector to make it more accurate
      %muzzleVector = VectorScale(%muzzleVector,500);
      // Get the dot product of the two vectors
      %dot = VectorDot(VectorNormalize(%vectorToTeammate),VectorNormalize(%muzzleVector));
      %dist = VectorDist(%plr.getPosition(),%obj.getPosition());

      // If the dot product is bigger than 0.93, we are aiming in this teammate's direction
      if (%dot > 0.93 && %dist <= 60) // hack
      {
        // Cast a ray to the teammate if he is within our cone of fire
        %ray = ContainerRayCast(%muzzlePoint,%obj.getWorldBoxCenter(),$AIPlayer::SolidMasks);
        if (!%ray) // If the ray is clear, hold fire so we don't accidentally shoot our friend
        {
          %plr.isFriendlyInLOS = true;
          %plr.setImageTrigger(0,0); // This will stop us from shooting
        }
      }
    }
  }
  else
  {
    %plr.isFriendlyInLOS = false;
  }

  if (%plr.getState() !$= "Dead")
  {
    if (%plr.isAsleep == false)
      %this.lineOfFireSched = %plr.schedule(500,checkLineOfFire);
    else
      %this.lineOfFireSched = %plr.schedule(10000,checkLineOfFire);
  }
}

// Checks for nearby grenades that we should get away from
function Brain1::CheckForGrenades(%this)
{
  %plr = %this.player;

  InitContainerRadiusSearch(%plr.getPosition(),100,$TypeMasks::ProjectileObjectType);
  while((%obj = ContainerSearchNext()) != 0)
  {
    if (%obj.getDatablock().isGrenade && !%obj.noAvoid)
    {
      if (VectorLen(%obj.getVelocity()) < 3)
      {
        %dist = VectorDist(%plr.getPosition(),%obj.position);
        if (%dist <= %obj.getDatablock().damageRadius)
        {
          if (%this.isClearTo(%obj.position))
          {
            error(%plr @ ": found grenade " @ %obj);
            %this.evadeObj = %obj;
          }
        }
      }
    }
  }
}

//-----------------------------------------------------------------
// AI State functions
//-----------------------------------------------------------------

// Nothing is really happening, so wait for something interesting
function Brain1::stateIdle(%this)
{
  %plr = %this.player;
  %this.state = "NONE";
  %plr.visMod = 1;

  if (%this.threatPosition) // React to threats
  {
    %this.pathPause = 0;
    %plr.setMoveSpeed(1.0);
    %plr.clearAim();
    if (%this.threatType $= "Fire" && !%plr.squad) // Under fire
    {
      %this.state = "REACT"; // Take cover and locate our attacker
        return;
    }
    if (%this.threatType $= "Body" && !%plr.squad) // Dead teammate
    {
      %this.state = "REACT";
        return;
    }
    if (%this.threatType $= "Explosion" && !%plr.squad) // Big explosion
    {
      %this.state = "REACT";
        return;
    }
    if (%this.threatType $= "ShotsFired" && !%plr.squad) // Dead teammate
    {
      %this.state = "REACT";
        return;
    }
    if (%this.threatType $= "ScoutHint" && !%plr.squad) // Scout hint (from helicopter, security cam, etc)
    {
      %this.state = "REACT";
        return;
    }
    if (%this.threatType $= "Annoyance" && !%plr.squad) // Scout hint (from helicopter, security cam, etc)
    {
      %this.state = "REACT";
        return;
    }
  }

  if (%plr.squad !$= "")
  {
    %this.state = "REGROUP";
      return;
  }

  // Sneak
  if (%this.stealthMovePos)
  {
    %this.state = "STEALTH";
    return;
  }

  // If we have a priority move location, go there.
  if (%this.priorityMoveLocation)
  {
    if (!%plr.squad)
    {
      %this.state = "PRIORITYMOVE";
      return;
    }
    else
    {
      if (%plr.memberIndex == 0)
      {
        %plr.squad.objectiveMove(%this.priorityMoveLocation);
      }
    }
  }

  // Evade grenades
  if (isObject(%this.evadeObj))
  {
    %this.lastEvadeState = %this.state;
    if (VectorDist(%plr.targetPathDestination, %plr.getPosition()) > 2)
      %this.lastDestination = %plr.targetPathDestination;
    else
      %this.lastDestination = %plr.getPosition();

    %this.state = "GRENADE";
    return;
  }

  // Assist friends
  if (isObject(%this.assistObj))
  {
    %this.pathPause = 0;
    %plr.setMoveSpeed(1.0);
    %plr.clearAim();
    %this.state = "ASSIST";
      return;
  }

  // check for enemies
  if (isObject(%this.attackObj) && %this.attackObj.getState() !$= "Dead")
  {
    %plr.setMoveSpeed(1.0);
    %this.setAlertness(3);
    %this.pathPause = 0;
    if (%this.shouldTakeCover(%this.attackObj))
    {
      %this.coverObj = %this.attackObj;
      if (%this.coverObjIsVisible)
      {
        %this.suppressPos = %this.coverObj.getWorldBoxCenter();
        %this.lastKnownEnemyPos = %this.coverObj.getPosition();
      }
      %this.state = "FINDCOVER";
        return;
    }
    else
    {
      if (%this.attackObjIsVisible)
      {
        %this.state = "CLOSEATTACK";
        %plr.setAimObject(%this.attackObj);
        %plr.suppressingFire(%this.attackObj.getWorldBoxCenter(),1,%this.attackObj);
        %this.lastKnownEnemyPos = %this.attackObj.getTrueEyePoint();
      }
    }
  }

  // See if we killed an enemy
  if (isObject(%this.attackObj) && %this.attackObj.getState() $= "Dead")
  {
    %this.pathPause = 0;
    %plr.setMoveSpeed(1.0);
    %plr.clearAim();
    %this.attackObj = 0;
      return;
  }

  if (%this.shouldReload())
  {
    %this.reloadWeapon();
  }

  %spawnMarker = %plr.spawnNode;
  // Either follow a path or move to our spawn marker's moveDestination
  if (%spawnMarker.path !$= "")
  {
    if (%this.followsPaths == true && %this.pathPause == false)
    {
      // Move at the specified speed
      %mvToNode = %spawnMarker.path.getId().getObject(%this.curPathNodeIndex);
      if (isObject(%mvToNode))
      {
        %plr.setMoveSpeed(%mvToNode.moveSpeed);
      }

      // If we are moving, look where we are going
      if (VectorLen(%plr.getVelocity()) > 0.2 && %plr.aimPatternType == -1)
        %plr.clearAim();

      if (%this.curPathNodeIndex $= "")
      {
        // We have not been initialized on a path yet, so move to the closest path node to us
        %closestMarker = %this.getClosestPathNode(%spawnMarker.path.getId());
        %this.moveToMarker(%spawnMarker.path.getId(),%closestMarker);
      }
      %path = %spawnMarker.path.getId();
      %curNode = %path.getObject(%this.curPathNodeIndex);
      %distToNode = VectorDist(%plr.getPosition(),%curNode.getPosition());
      if (%distToNode < 1)
      {
        %plr.stop();
        // Arrived at our current node
        //  Now, we see if there is any more information about the node we are on that we should think about
        if(%curNode.stopTime !$= "")
        {
          // If stopTime is -1, then stop following paths.
          if (%curNode.stopTime == -1)
          {
            %this.followsPaths = false;
            %plr.stop();
          }
          else
          {
            //%this.schedule(%curNode.stopTime,moveToNextMarker,%spawnMarker.path.getId());
            %plr.schedule(%curNode.stopTime,moveToNextMarker,%spawnMarker.path.getId());
            %this.pathPause = true;
            // Look the direction that the marker is facing
            %forwardVec = %curNode.getForwardVector();
            %forwardVec = VectorScale(VectorNormalize(%forwardVec),100);
            %forwardPos = VectorAdd(%forwardVec,%plr.getTrueEyePoint());
            %plr.setAimLocation(%forwardPos);

            // Do what is specified under the marker's Event field (if there IS something specified)
            if (%curNode.event !$= "")
            {
              if (%curNode.isNPCCommand == true)
                %eventString = "%plr." @ %curNode.event;
              else
                %eventString = %curNode.event;

              eval(%eventString);
            }
          }
        }
        else
        {
          %this.moveToNextMarker(%spawnMarker.path.getId());
          %plr.clearAim();
        }
      }
      else
      {
        if (VectorLen(%plr.getVelocity()) < 0.2)
        {
          echo(%plr @ ": am I stuck?");
          %plr.targetPathDestination = 0;
          %this.moveToMarker(%spawnMarker.path.getId(),%this.curPathNodeIndex,true);
          %plr.clearAim();
        }
      }
    }
  }
  else if (%spawnMarker.moveDestination && %this.alertness < 2) // we have a move destination, go there
  {
    if (%spawnMarker.highPriorityMove) // If it's high priority, move there quickly and kill everything on the way
    {
      %this.priorityMoveLocation = %spawnMarker.moveDestination;
      %this.state = "PRIORITYMOVE";
      return;
    }
    else // No pressure
    {
      %plr.setMoveSpeed(0.2);
      if (%spawnMarker.wanderRange)
      {
        // We're set to wander, so let's do that
        if (%plr.isStopped() && %this.largeChanceRandom()) // The random element is there so that we pause a bit between wandering
          %plr.requestWander(1,%spawnMarker.wanderRange,0,false,%spawnMarker.getPosition());
      }
      else
      {
        %distToDest = VectorDist(%plr.getPosition(),%spawnMarker.moveDestination);
        %ray = ContainerRayCast(VectorAdd(%plr.getPosition(),"0 0 1"),VectorAdd(%spawnMarker.moveDestination,"0 0 1"),$Brain1::SolidMasks);
        if (%distToDest > 3.5 && %ray)
        {
          %plr.SetOnPath(%spawnMarker.moveDestination);
        }
        else if (%distToDest > 3.5 && !%ray)
        {
          %plr.setMoveDestination(%spawnMarker.moveDestination);
        }
        else
        {
          // Nothing, we are already at our moveDestination
          %plr.stop();
        }
      }
    }
  }
}

// React to threats
function Brain1::stateReact(%this)
{
  %plr = %this.player;
  %this.state = "REACT";
  %plr.visMod = 1.1;

  // reload
  if (%this.shouldReload())
  {
    %this.reloadWeapon();
  }

  // check for enemies
  if (isObject(%this.attackObj) && %this.attackObj.getState() !$= "Dead")
  {
    %this.setAlertness(3);
    if (%this.shouldTakeCover(%this.attackObj))
    {
      %plr.setMoveSpeed(1.0);
      %this.coverObj = %this.attackObj;
      if (%this.coverObjIsVisible)
      {
        %this.suppressPos = %this.coverObj.getWorldBoxCenter();
        %this.lastKnownEnemyPos = %this.coverObj.getPosition();
      }
      %this.state = "FINDCOVER";
        return;
    }
    else
    {
      if (%this.attackObjIsVisible)
      {
        %this.state = "CLOSEATTACK";
        %plr.setMoveSpeed(1.0);
        %plr.setAimObject(%this.attackObj);
        %plr.suppressingFire(%this.attackObj.getWorldBoxCenter(),1,%this.attackObj);
        %this.lastKnownEnemyPos = %this.attackObj.getTrueEyePoint();
          return;
      }
    }
  }
  // See if we killed an enemy
  if (isObject(%this.attackObj) && %this.attackObj.getState() $= "Dead")
  {
    %plr.setMoveSpeed(1.0);
    %plr.clearAim();
    %this.attackObj = 0;
      return;
  }

  // Sneak
  if (%this.stealthMovePos)
  {
    %this.state = "STEALTH";
    return;
  }

  // If we have a priority move location, go there.
  if (%this.priorityMoveLocation)
  {
    %this.state = "PRIORITYMOVE";
    return;
  }

  // Evade grenades
  if (isObject(%this.evadeObj))
  {
    %this.lastEvadeState = %this.state;
    if (VectorDist(%plr.targetPathDestination, %plr.getPosition()) > 2)
      %this.lastDestination = %plr.targetPathDestination;
    else
      %this.lastDestination = %plr.getPosition();

    %this.state = "GRENADE";
    return;
  }

  // Assist friends
  if (isObject(%this.assistObj))
  {
    %this.state = "ASSIST";
    return;
  }


  if (%this.threatPosition $= "") // && !%this.threatOrigin))
  {
    %this.threatPosition = 0;
    %this.threatType = "";

    // Momentarily stop thinking to avoid endless loop crashes (they happen sometimes with the state system)
    %plr.setThinking(false);
    %plr.schedule(1000,setThinking,true);

    %plr.setMoveSpeed(1.0);
    if (!%plr.squad)
    {
      %this.state = "NONE";
      %plr.setPlayerPosition(1); //Stand
        return;
    }
    else
    {
      %this.state = %plr.squad.currentOrder;
      //%this.state = "REGROUP";
      %plr.setPlayerPosition(1); //Stand
        return;
    }
  }

  // React to player movement if we are still investigating possible danger
  if (%this.threatType $= "Player" && %this.alertness < 3)
  {
    //%plr.setMoveSpeed(1.0);
    //%this.state = "HUNT";
    //return;
  }

  if (!%this.assumedEnemyPosition)
  {
    if (%this.threatOrigin)
      %this.assumedEnemyPosition = %this.threatOrigin;
    if (%this.audFireLocation)
    {
      %this.assumedEnemyPosition = %this.audFireLocation;
      %this.threatOrigin = %this.audFireLocation;
      if (%this.threatType $= "ShotsFired")
      {
        %this.threatPosition = %this.threatOrigin;
      }
    }

    %this.threatOrigin = %this.assumedEnemyPosition;
    %plr.setAimLocation(%this.assumedEnemyPosition);
  }
  if (%this.audFireLocation)
  {
    %this.assumedEnemyPosition = %this.audFireLocation;
    %this.threatOrigin = %this.audFireLocation;
  }

  //-----------------------------------------------------------------------------------------------
  // -- Hearing gunfire
  // Have a look around for danger, and if we are close enough to where we heard the shots, go and
  //   see what is going on.
  //-----------------------------------------------------------------------------------------------
  if (%this.threatType $= "ShotsFired")
  {
    // If we are asleep, don't react to shots. Otherwise, we run the risk of too many NPCs running into the player's zone, which drags down performance
    if (%plr.isAsleep == true)
      return;

    %threatPos = %this.threatPosition;
    //%plr.clearAim();
    %plr.setAimLocation(%threatPos);
    // If the shot position is close, assume we're in danger and go see what's going on
    //if (%plr.getPathDistance(%plr.getPosition(),%threatPos) <= $Brain1::ShotsFiredDangerDist)
    if (VectorDist(%plr.getPosition(),%this.audFireLocation) <= $Brain1::ShotsFiredDangerDist)
    {
      %this.setAlertness(3);
      if (%this.smallChanceRandom() && %plr.isStopped())
      {
        // Move in on the location of the gunfire
        %plr.setOnPath(%this.audFireLocation);
        //return;
      }
      %plr.setAimLocation(%this.audFireLocation);

      if (!%plr.isStopped())
      {
        // In transit to shots fired location
      }

      if (VectorDist(%plr.getPosition(),%this.audFireLocation) < 10)
      {
        if (%this.largeChanceRandom())
        {
          // Stay mobile, find the shooter.
          %plr.requestWander(10,100); // hack
        }
        else
        {
          %plr.clearAim(); // Lingering around the position of the shot, don't stare at it
        }
      }
      return;
    }
    else
    {
      if (%plr.isStopped())
        %plr.setAimLocation(%this.audFireLocation);
      else
        %plr.clearAim();
    }
  }

  //-----------------------------------------------------------------------------------------------
  // -- Reacting to explosions
  // This one is tricky; you don't want to get close to where the explosion happened for fear of
  //   getting blown up ourselves, but we also want to find threats near the explosion.
  // Behavior here involves fleeing if we are close to the explosion, and searching for threats.
  //-----------------------------------------------------------------------------------------------
  if (%this.threatType $= "Explosion")
  {
    if (%plr.isAsleep == true)
      return;
    %distToExp = VectorDist(%plr.getPosition(),%this.threatPosition);
    if (%distToExp <= $Brain1::ExplosionDangerDist) // if we are close to the explosion
    {
      // Get out of there!
      %plr.clearAim();
      %plr.setMoveSpeed(1.0);
      if (%plr.isStopped())
        %plr.requestWander(30,100); // hack
    }
    else
    {
      // We have moved far enough from the explosion, so search for whoever caused it
      if (%this.assumedEnemyPosition)
      {
        if (%this.mediumChanceRandom())
        {
          %plr.setMoveSpeed(0.5);
          if (VectorDist(%plr.getPosition(),%this.assumedEnemyPosition) < 10)
          {
            if (%plr.isStopped())
              %plr.requestWander(5,100);
          }
          else
          {
            if (%plr.isStopped())
              %plr.setOnPath(%this.assumedEnemyPosition);
          }
        }
      }
      else
      {
        if (%this.mediumChanceRandom())
        {
          //if (%plr.isStopped())
            //%plr.requestWander(4,50); // hack
          if (VectorDist(%plr.getPosition(),%this.threatPosition) > 20) // hack, once we are within this dist of the explosion, search the area
          {
            %plr.setOnPath(%this.threatPosition);
            %plr.setMoveSpeed(0.5);
          }
          else
          {
            %plr.requestWander(5,50);  // hack
          }
        }
        else
        {
          %plr.setAimLocation(%this.threatPosition);
        }
      }
    }
  }

  //-----------------------------------------------------------------------------------------------
  // -- ScoutHint
  //    This is what we can call justified cheating. ScoutHintPos is the recorded location of an
  //  enemy spotted from a helicopter, security camera, etc. This can be used in many ways in your
  //  project, depending on what kind of gameplay you're aiming for. For example, in my project,
  //  an overhead helicopter that spots you (the player) will tell its teammates where it saw you
  //  by setting their ScoutHintPos to that location.
  //-----------------------------------------------------------------------------------------------
  if (%this.threatType $= "ScoutHint")
  {
    // move to the position of scouted enemy
    %hPos = %this.threatPosition;
    %plr.setOnPath(%hPos);
    return;
  }

  //-----------------------------------------------------------------------------------------------
  // -- Annoyance
  //    Our threat may or may not be anything serious, but we'll go and check it out anyway.
  //  I implemented this so that guards can get pissed off by car alarms. Hopefully you'll find some
  //  better use for it ;)
  //-----------------------------------------------------------------------------------------------
  if (%this.threatType $= "Annoyance")
  {
    %aPos = %this.threatPosition;
    %distToAPos = VectorDist(%plr.getPosition(),%aPos);
    if (%distToAPos < 100) // Don't investigate if we're far away
    {
      if (%distToAPos > 5) // If we're not at the position, move to it
      {
        if (%plr.isStopped())
          %plr.setAimLocation(%aPos);
        else
          %plr.clearAim();
        if (%this.largeChanceRandom()) // I use the random function so they will theoretically wait a bit before investigating
        {
          %plr.setMoveSpeed(0.3);
          %plr.setOnPath(%aPos);
        }
      }
      else // Else, we've arrived
      {
        %plr.stop();
        if (%plr.aimPatternType == -1)
          %plr.startAimPattern(1,getRandom(0,20)); // Look around for a bit
        if (%this.largeChanceRandom()) // Again, random to add a bit of delay before returning to our post
        {
          %this.threatType = "";
          %this.threatPosition = 0;
          %this.state = "NONE";
          %plr.clearAim();
          %plr.stopAimPattern();
          return;
        }
      }
    }
  }

  //-----------------------------------------------------------------------------------------------
  // -- Coming under fire or finding a body
  // These use the same reaction code because a dead body should cause the NPC to assume immediate
  //   danger.
  // This involves figuring out where the shooter is, based on either a heard gunshot or an
  //   educated guess, and then taking cover/searching for the shooter.
  //-----------------------------------------------------------------------------------------------
  if (%this.threatType $= "Fire" || %this.threatType $= "Body")
  {
    // If we are asleep, don't react to explosions. Otherwise, we run the risk of too many NPCs running into the player's zone, which drags down performance
    // AlertAIPlayers() will wake us up if we are directly fired at.
    if (%plr.isAsleep == true)
      return;

    %plr.setMoveSpeed(1.0);

    // Do we know (or have an idea of) where the enemy is?
    if (%this.assumedEnemyPosition)
    {
      if (%this.threatOrigin)
        %this.assumedEnemyPosition = %this.threatOrigin;
      if (%this.audFireLocation)
      {
        %this.assumedEnemyPosition = %this.audFireLocation;
        %this.threatOrigin = %this.audFireLocation;
      }

      %this.threatOrigin = %this.assumedEnemyPosition;
      if (%plr.isStopped())
        %plr.setAimLocation(%this.assumedEnemyPosition);
      else
        %plr.clearAim();
    }
    // Figure out where the enemy is
    else
    {
      // If we heard gunfire, use that
      if (%this.audFireLocation)
      {
        %this.assumedEnemyPosition = %this.audFireLocation;
        %this.threatOrigin = %this.audFireLocation;
          return;
      }
      // If we have a threatOrigin already (like if we heard from a friend), use that
      if (%this.threatOrigin)
      {
        %this.assumedEnemyPosition = %this.threatOrigin;
          return;
      }

      // Try to guess where the attacker is if we have no other knowledge of his position
      if (%this.threatType $= "Fire")
      {
        // There are two methods that we use for predicting enemy sniper locations
        // The method we use depends on if the bullet landed in front or behind us, and whether or not we
          // have line-of-sight to that position
        %rayToThreatPos = ContainerRayCast(%plr.getWorldBoxCenter(),%this.threatPosition,$AIPlayer::SolidMasks);
        %dotMuzzleToThreatPos = VectorDot(VectorNormalize(%plr.getTrueMuzzleVector(0)),VectorNormalize(VectorSub(%this.threatPosition,%plr.getPosition())));
        if (%dotMuzzleToThreatPos < 0.75 && %dotMuzzleToThreatPos > 0.25)
          %threatInFront = true;
        else
          %threatInFront = false;
        if (!%rayToThreatPos)
          %losToThreat = true;
        else
          %losToThreat = false;

        // Cast rays in all directions to see where it would make sense for a sniper to be
        %numRays = 15;
        %rotPerRay = 360/%numRays;
        for (%rayIdx = 0;%rayIdx < %numRays;%rayIdx++)
        {
          %checkStart = %plr.getEyePoint();
          %checkEnd = VectorAdd(%checkStart,"0 500 0");
          %checkVector = VectorSub(%checkEnd,%checkStart);
          %checkVector = AIPlayer::vecRotateAz(%checkVector,%rotPerRay * %rayIdx);
          %checkEnd = VectorAdd(%checkVector,%checkStart);
          %randUpVector = "0 0" SPC getRandom(-100,100);
          %checkEnd = VectorAdd(%checkEnd,%randUpVector);

          %checkRay = ContainerRayCast(%checkStart,%checkEnd,$AIPlayer::SolidMasks);
          if (%checkRay)
          {
            %intersect = getWords(%checkRay,1,3);
            %distToIntersect = VectorDist(%plr.getPosition(),%intersect);
            %newDist = %distToIntersect - 1;
            %checkVector = VectorNormalize(%checkVector);
            %checkVector = VectorScale(%checkVector,%newDist);
            %checkEnd = VectorAdd(%checkStart,%checkVector);
            %checkEnd = %intersect;
          }

          %distToCheckEnd = VectorDist(%plr.getPosition(),%checkEnd);
          if (%distToCheckEnd >= $Brain1::MinSniperGuessDist && %distToCheckEnd < $Brain1::MaxSniperGuessDist)
          {
            // See if %checkEnd is within a certain arc of the vector from our position to our threatposition
            //%vecPlrToTPos = VectorSub(%this.threatPosition,%plr.getPosition());
            //%vecPlrToTPos = VectorNormalize(%vecPlrToTPos);
            //%checkVecN = VectorNormalize(%checkVector);

            %vecEndToTPos = VectorSub(%this.threatPosition,%checkEnd); // Vector from the guessed sniper position to the threatPosition
            %vecEndToPlr = VectorSub(%plr.getPosition(),%checkEnd); // Vector from the guessed sniper position to our position

            %plrVec = VectorNormalize(VectorSub(%plr.getPosition(),%checkEnd));
            %tPosVec = VectorNormalize(VectorSub(%this.threatPosition,%checkEnd));

            %aimAlignedDot = VectorDot(%tPosVec,%plrVec);
            if (%aimAlignedDot > 0) // && %aimAlignedDot > 0.20) //hack
            {
              // Now, check to see if a sniper at %checkEnd could actually fire a bullet at our threatPosition
              %shotRay = ContainerRayCast(%checkEnd,%this.threatPosition,$AIPlayer::SolidMasks);
              if (!%shotRay)
              {
                // this position is good, so let's assume the sniper is there
                %aimPoint = %checkEnd;
                %this.assumedEnemyPosition = %aimPoint;
                break;
              }
            }
          }
        }
      }
    }

    %threatPos = %this.threatPosition;

    // Stop if we have a new threatType
    if (%this.threatTypeChanged)
    {
      %plr.stop();
      return;
    }
    // React randomly, or when our threat position changes
    if ((%this.isTakingDamage || VectorDist(%this.threatPosition,%this.lastThreatPosition) >= $Brain1::ThreatPosTolerance) || (%this.mediumChanceRandom() && %this.threatType $= "Body"))// && %this.coverMarker)
    {
      %this.lastThreatPosition = %threatPos;
      %randVec = getRandom($Brain1::MinReactMoveDist,$Brain1::MaxReactMoveDist) SPC getRandom($Brain1::MinReactMoveDist,$Brain1::MaxReactMoveDist) SPC "0";
      %randPos = VectorAdd(%plr.getPosition(),%randVec);

      // If the threat position is close, protect ourselves from potential danger
      if (VectorDist(%plr.getPosition(),%this.threatPosition) < $Brain1::FireReactionDangerDist)
      {
        // Get to some cover
        if (%this.assumedEnemyPosition)
        {
          if (%plr.isStopped())
          {
            %plr.clearCList();
            %cover = %plr.findQuickCover(%this.assumedEnemyPosition,80,true,false);
            if (%cover)
            {
              %plr.SetOnPath(%cover.getPosition());
              %plr.clearAim();
              %plr.setMoveSpeed(1.0);
              %this.coverMarker = %cover;
                return;
            }
            else
            {
              %plr.requestWander(70,150); // hack, running far, far away
              %plr.setMoveSpeed(1.0);
              %this.coverMarker = 0;
            }
          }
        }
        else
        {
          %plr.requestWander(70,150); // hack, running far, far away
          %plr.setMoveSpeed(1.0);
        }
      }
      else
      {
        if (VectorDist(%plr.getPosition(),%this.assumedEnemyPosition) < $Brain1::ShotsFiredDangerDist && %this.assumedEnemyPosition !$= "")
        {
          %plr.setOnPath(%this.assumedEnemyPosition);
          %plr.setMoveSpeed(0.5);
        }
        else
        {
          %plr.requestWander(5,50); //hack
          %plr.setMoveSpeed(1.0);
        }
      }
    }
  }

  // Look around
  if (%plr.aimPatternType == -1)
    %plr.startAimPattern(1,getRandom(0,20));

  // If we have cover, decide how to move to it
  if (VectorDist(%plr.getPosition(),AIPlayer::getGroundPos(%this.coverMarker.position)) > 1) // If we are moving to cover
  {
    if (%this.coverMarker)
    {
      %plr.clearAim(); // Look where we are going
      %plr.setOnPath(%this.coverMarker.getPosition());
      %plr.setMoveSpeed(1.0);
      if (VectorDist(%plr.getPosition(),%this.coverMarker.position) <= $Brain1::ReactMoveCrouchDist)
        %plr.setPlayerPosition(2); // Crouch
      else
        %plr.setPlayerPosition(1); // Stand
    }
  }
  else  // At cover
  {
    %plr.stop();
    if (%this.coverMarker.fireOverThis == true)
    {
      %stand = %this.mediumChanceRandom();
      %searchForAttacker = %this.smallChanceRandom();
      %standRay = ContainerRayCast(%plr.getWorldBoxCenter(),%this.assumedEnemyPosition,$AIPlayer::SolidMasks);
      if (%stand == 1 && !%standRay)
      {
        //%plr.setPlayerPosition(1);
        //%plr.schedule($Brain1::CheckTime,setPlayerPosition,2);
      }
      else
      {
        %plr.setPlayerPosition(2);
      }
    }
    else
    {
      %searchForAttacker = %this.smallChanceRandom();
    }
  }
}

// Reaction behavior for squads
function Brain1::stateSquadReact(%this)
{
  %plr = %this.player;
  %this.state = "SQUADREACT";
  %plr.visMod = 1.2;
  %plr.setCoverPosition(5);
  //-----------------------------------------------------------------
  // Reload
  //-----------------------------------------------------------------
  if (%plr.getInventory(%this.ammo) < 1)
  {
    %this.reloadWeapon();
  }

  //-----------------------------------------------------------------
  // Evade grenades
  //-----------------------------------------------------------------
  if (isObject(%this.evadeObj))
  {
    %this.lastEvadeState = %this.state;
    if (VectorDist(%plr.targetPathDestination, %plr.getPosition()) > 2)
      %this.lastDestination = %plr.targetPathDestination;
    else
      %this.lastDestination = %plr.getPosition();

    %this.state = "GRENADE";
    return;
  }

  //-----------------------------------------------------------------
  // Stop reacting if the squad leader tells us to
  //-----------------------------------------------------------------
  if (%plr.squad.quitReact)
  {
    %this.threatType = "";
    %this.threatPosition = 0;
    %this.attackObj = 0;
    %this.audFireLocation = 0;
    %this.state = %plr.squad.currentOrder;
    %this.reactCoverMarker = 0;
    %plr.clearAim();
    %plr.setPlayerPosition(1);

    %plr.setThinking(false);
    %plr.schedule(1000,setThinking,true);
    return;
  }

  //-----------------------------------------------------------------
  // Use %this.attackActiveCount to track how long we've been in this state
  //-----------------------------------------------------------------
  %this.attackActiveCount++;
  if (%this.attackActiveCount > $Brain1::SquadReactTime)
  {
    %this.threatType = "";
    %this.threatPosition = 0;
    %this.attackObj = 0;
    %this.audFireLocation = 0;
    %this.state = %plr.squad.currentOrder;
    %this.reactCoverMarker = 0;
    %plr.clearAim();

    %plr.setThinking(false);
    %plr.schedule(1000,setThinking,true);
    return;
  }

  //-----------------------------------------------------------------
  // make sure we have a threat
  //-----------------------------------------------------------------
  if (%this.threatType $= "" || !%this.threatPosition)
  {
    %this.threatType = "";
    %this.threatPosition = 0;
    %this.attackObj = 0;
    %this.state = %plr.squad.currentOrder;
    %this.audFireLocation = 0;
    %plr.clearAim();

    %plr.setThinking(false);
    %plr.schedule(1000,setThinking,true);
    return;
  }

  //-----------------------------------------------------------------
  // check for enemies
  //-----------------------------------------------------------------
  if (%this.attackObj != 0 && %this.attackObj.getState() !$= "Dead")
  {
    %this.setAlertness(3);
    if (%this.shouldTakeCover(%this.attackObj))
    {
      %this.coverObj = %this.attackObj;
      if (%this.coverObjIsVisible)
      {
        %this.suppressPos = %this.coverObj.getWorldBoxCenter();
        %this.lastKnownEnemyPos = %this.coverObj.getPosition();
      }
      %this.state = "FINDCOVER";
      %plr.setMoveSpeed(1.0);
        return;
    }
    else
    {
      if (%this.attackObjIsVisible)
      {
        %this.state = "CLOSEATTACK";
        %plr.setAimObject(%this.attackObj,"0 0 1.5");
        %plr.suppressingFire(%this.attackObj.getWorldBoxCenter(),1,%this.attackObj);
        %plr.setMoveSpeed(1.0);
        %plr.setMoveDestination("");
        %plr.stop();
          return;
      }
    }
  }

  //-----------------------------------------------------------------
  // See if we killed an enemy
  //-----------------------------------------------------------------
  if (%this.attackObj && %this.attackObj.getState() $= "Dead")
  {
    %plr.clearAim();
    %plr.setMoveSpeed(1.0);
    %this.attackObj = 0;
    %this.state = %plr.squad.currentOrder;
    %this.threatType = "";
    %this.threatPosition = 0;
    %this.reactCoverMarker = 0;
      return;
  }

  //-----------------------------------------------------------------
  // React!
  //   Run to cover, scan the area
  //-----------------------------------------------------------------

  if (!%this.audFireLocation)
  {
    if (%plr.aimPatternType == -1)
      %plr.startAimPattern(1,getRandom(0,20));
  }
  else
  {
    %plr.setAimLocation(%this.audFireLocation);
    if (%this.isClearTo(%this.audFireLocation) && %this.attackActiveCount < $Brain1::SquadReactTime / 2)
    {
      if (VectorDist(%plr.getPosition(),%this.audFireLocation) > 40) // hack, don't suppress areas too close
        %plr.suppressingFire(%this.audFireLocation,1);
    }
  }

  if (!%this.reactCoverMarker)
  {
    %cover = %plr.findQuickCover(%this.assumedEnemyPosition,30,false,true); // hack, distance should be hooked to a global var
    if (%cover)
    {
      %plr.SetOnPath(%cover.getPosition());
      %plr.clearAim();
      %plr.setMoveSpeed(1.0);
      %this.reactCoverMarker = %cover;
      return;
    }
    else
    {
      %plr.setPlayerPosition(2);
      %plr.stop();
      %this.reactCoverMarker = 0;
      return;
    }
  }

  //-----------------------------------------------------------------
  // What to do when we reach cover
  //-----------------------------------------------------------------
  if (isObject(%this.reactCoverMarker) && Vectordist(%plr.getPosition(),%this.reactCoverMarker.getPosition()) < 2)
  {
    %plr.stop();
    %plr.setPlayerPosition(2);
  }
}

// Search an area
function Brain1::stateInvestigate(%this)
{
  %plr = %this.player;
  %this.state = "INVESTIGATE";
  %plr.visMod = 1.1;

  // reload
  if (%this.shouldReload())
  {
    %this.reloadWeapon();
  }

  // React to threats
  if ((%this.threatType $= "Fire" || %this.threatType $= "Body" || %this.threatType $= "ShotsFired" || %this.threatType $= "Explosion") && !%plr.squad) // Under fire
  {
   // if (!%this.audFireLocation)
     // %this.audFireLocation = %this.threatPosition;

    %this.state = "REACT"; // Take cover and locate our attacker
    %this.huntNodeIndex = 0;
    %this.furtherSearching = 0;
    %plr.setMoveSpeed(1.0);
    %plr.stopAimPattern();
      return;
  }

  // check for enemies
  if (%this.attackObj != 0 && %this.attackObj.getState() !$= "Dead")
  {
    %this.setAlertness(3);
    if (%this.shouldTakeCover(%this.attackObj))
    {
      %this.coverObj = %this.attackObj;
      if (%this.coverObjIsVisible)
      {
        %this.suppressPos = %this.coverObj.getWorldBoxCenter();
        %this.lastKnownEnemyPos = %this.coverObj.getPosition();
      }
      %this.state = "FINDCOVER";
      %this.huntNodeIndex = -1;
      %this.furtherSearching = 0;
      %plr.setMoveSpeed(1.0);
        return;
    }
    else
    {
      if (%this.attackObjIsVisible)
      {
        %this.state = "CLOSEATTACK";
        %plr.setAimObject(%this.attackObj,"0 0 1.5");
        %plr.suppressingFire(%this.attackObj.getWorldBoxCenter(),1,%this.attackObj);
        %this.lastKnownEnemyPos = %this.attackObj.getTrueEyePoint();
        %this.huntNodeIndex = -1;
        %this.furtherSearching = 0;
        %plr.setMoveSpeed(1.0);
        %plr.setMoveDestination("");
        %plr.stop();
        %plr.patharray.empty();
          return;
      }
    }
  }

  // See if we killed an enemy
  if (%this.attackObj && %this.attackObj.getState() $= "Dead")
  {
    %plr.clearAim();
    %plr.setMoveSpeed(1.0);
    %this.attackObj = 0;
    if (!%plr.squad)
      %this.state = "NONE";
    else
      %this.state = %plr.squad.currentOrder;

      return;
  }

  if (!%this.invPosition)
  {
    %this.huntNodeIndex = -1;
    %this.invStartPosition = 0;
    %plr.huntArray.empty();

    if (!%plr.squad)
      %this.state = "NONE";
    else
      %this.state = %plr.squad.currentOrder;
    return;
  }

  // Evade grenades
  if (isObject(%this.evadeObj))
  {
    %this.lastEvadeState = %this.state;
    if (VectorDist(%plr.targetPathDestination, %plr.getPosition()) > 2)
      %this.lastDestination = %plr.targetPathDestination;
    else
      %this.lastDestination = %plr.getPosition();

    %this.state = "GRENADE";
    return;
  }

  %plr.clearAim();

  // Decide if we need to start searching, or if we have already started.
  if (%this.huntNodeIndex == -1)
  {
    // We have not started searching, so set up the huntArray
    warn(%plr @ ": search init");
    if (!isObject(%plr.huntArray))
    {
      %plr.huntArray = new ArrayObject() {};
      MissionCleanup.add(%plr.huntArray);
    }

    %plr.huntArray.empty();

    if (%this.invStartPosition)
      %start = %this.invStartPosition;
    else
      %start = %plr.getPosition();

    echo("dist between start and threatpos: " @ VectorDist(%start,%this.invPosition));
    %this.getHidingNodes(%start,%this.invPosition,%plr.huntArray);

    if (%plr.huntArray.count() > 0)
    {
      // We have nodes to search, so set the huntNodeIndex to 0 to begin searching
      %this.huntNodeIndex = 0;
      return;
    }
    else
    {
      // QUIT THE STATE
      echo(%plr @ ": could not find hiding nodes for " @ %this.invPosition);
      %this.huntNodeIndex = -1;
      %this.invPosition = 0;
      %this.invStartPosition = 0;
      %this.threatType = "";
      %this.threatPosition = 0;

      if (!%plr.squad)
        %this.state = "NONE";
      else
        %this.state = %plr.squad.currentOrder;
      return;
    }
  }
  else
  {
    echo("------" @ %plr @ ": huntNodeIndex is " @ %this.huntNodeIndex);
    %nodeToMoveTo = %plr.huntArray.getKey(%this.huntNodeIndex);

    // Set the move speed
    if (%this.alertness < 2)
      %plr.setMoveSpeed(0.4); // hack. hook this up to a global var called "cautiousmovespeed" or something of the sort
    else
      %plr.setMoveSpeed(1.0);

    if (!isObject(%nodeToMoveTo) || %this.huntNodeIndex == %plr.huntArray.count() - 1)
    {
      // QUIT THE STATE
      echo(%plr @ ": Finished Searching.");
      %this.huntNodeIndex = -1;
      //%this.threatType = "Player";
      //%this.threatPosition = %this.invPosition;
      %this.threatType = "";
      %this.threatPosition = 0;
      %this.invPosition = 0;
      %this.invStartPosition = 0;
      %this.attackObj = 0;
      %this.setAlertness(0);

      if (!%plr.squad)
        %this.state = "NONE";
      else
        %this.state = %plr.squad.currentOrder;
      return;
    }

    // See if we can move to this node
    %distToNode = %plr.getPathDistance(%plr.getPosition(),%nodeToMoveTo.getPosition());
    if (%distToNode < 1000)
    {
      if (VectorDist(%plr.getPosition(),%nodeToMoveTo.getPosition()) > 3)
      {
        echo(%plr @ ": moving to node " @ %nodeToMoveTo);
        %plr.setOnPath(%nodeToMoveTo.getPosition());
        return;
      }
      else
      {
        // We've arrived at our node, so increment the huntNodeIndex to move to the next
        echo(%plr @ ": arrived at node at index " @ %this.huntNodeIndex);

        // Increment the huntNodeIndex so we move to the next
        if (%this.veryLargeChanceRandom()) // random is here so we wait a bit at each hiding node
          %this.huntNodeIndex++;
        return;
      }
    }
    else
    {
      // We can't move there, so increment the huntNodeIndex so we move to the next node
      warn(%plr @ ": cannot move to node " @ %nodeToMoveTo);
      %this.huntNodeIndex++;
      return;
    }
  }
}

// Ramirez! Get to Ramirez and take out Ramirez with your Ramirez!
//  This state moves the AI to a high-priority location. It puts us at high alert, and sends us moving.
// There is nothing in this state that triggers the reaction code, so we'll be doing none of that. This
// is good for relentless onslaughts of enemies.
function Brain1::statePriorityMove(%this)
{
  %plr = %this.player;
  %this.state = "PRIORITYMOVE";
  %plr.visMod = 1.0;

  //-----------------------------------------------------------------
  // See if we still need to be doing this
  //-----------------------------------------------------------------
  if (!%this.priorityMoveLocation)
  {
    %this.state = "NONE";
    return;
  }

  //-----------------------------------------------------------------
  // Reload if we have to
  //-----------------------------------------------------------------
  if (%this.shouldReload())
  {
    %this.reloadWeapon();
  }

  //-----------------------------------------------------------------
  // Attack enemies
  //-----------------------------------------------------------------
  if (%this.attackObj != 0 && %this.attackObj.getState() !$= "Dead")
  {
    %this.setAlertness(2);
    if (%this.shouldTakeCover(%this.attackObj))
    {
      %this.coverObj = %this.attackObj;
      if (%this.coverObjIsVisible)
      {
        %this.suppressPos = %this.coverObj.getWorldBoxCenter();
        %this.lastKnownEnemyPos = %this.coverObj.getPosition();
      }
      %this.state = "FINDCOVER";
      %this.huntNodeIndex = -1;
      %this.furtherSearching = 0;
      %plr.setMoveSpeed(1.0);
      %plr.stopAimPattern();
        return;
    }
    else
    {
      if (%this.attackObjIsVisible)
      {
        %this.state = "CLOSEATTACK";
        %plr.setAimObject(%this.attackObj,"0 0 1.5");
        %this.lastKnownEnemyPos = %this.attackObj.getTrueEyePoint();
        %plr.suppressingFire(%this.attackObj.getWorldBoxCenter(),1,%this.attackObj);
        %this.huntNodeIndex = %plr.getHListCount()+1; // So we stop searching
        %this.furtherSearching = 0;
        %plr.setMoveSpeed(1.0);
        %plr.setMoveDestination("");
        %plr.stop();
        %plr.stopAimPattern();
        %plr.patharray.empty();
          return;
      }
    }
  }
  if (%this.attackObj && %this.attackObj.getState() $= "Dead")
  {
    %plr.clearAim();
    %plr.stopAimPattern();
    %plr.setMoveSpeed(1.0);
    %this.attackObj = 0;
      return;
  }

  //-----------------------------------------------------------------
  // Evade grenades
  //-----------------------------------------------------------------
  if (isObject(%this.evadeObj))
  {
    %plr.stopAimPattern();
    %this.lastEvadeState = %this.state;
    if (VectorDist(%plr.targetPathDestination, %plr.getPosition()) > 2)
      %this.lastDestination = %plr.targetPathDestination;
    else
      %this.lastDestination = %plr.getPosition();

    %this.state = "GRENADE";
    return;
  }

  //-----------------------------------------------------------------
  // Move to the high-priority location
  //-----------------------------------------------------------------
  %dist = VectorDist(%plr.getPosition(),%this.priorityMoveLocation);
  if (%dist > 2)
  {
    %plr.setOnPath(%this.priorityMoveLocation);
    %plr.setMoveSpeed(1.0);
    %this.setAlertness(3);
    // Start an aim pattern so that we search the area while moving
    if (%plr.aimPatternType == -1)
       %plr.startAimPattern(1,getRandom(0,20));
  }
  else
  {
    // We've arrived
    echo(%plr @ ": arrived at priority move location");
    %this.priorityMoveLocation = 0;
    %this.reachedPriorityMoveLoc = true;
    %this.state = "NONE";
    return;
  }
}

// Search an area for a possible threat we heard
function Brain1::stateHunt(%this)
{
  %plr = %this.player;
  %this.state = "HUNT";
  %plr.visMod = 1.7;

  // reload
  if (%this.shouldReload())
  {
    %this.reloadWeapon();
  }

  if ((%this.threatType $= "Fire" || %this.threatType $= "Body" || %this.threatType $= "ShotsFired" || %this.threatType $= "Explosion") && !%plr.squad) // Under fire
  {
   // if (!%this.audFireLocation)
     // %this.audFireLocation = %this.threatPosition;

    %this.state = "REACT"; // Take cover and locate our attacker
    %this.huntNodeIndex = 0;
    %this.furtherSearching = 0;
    %plr.setMoveSpeed(1.0);
    %plr.stopAimPattern();
      return;
  }

    // check for enemies
  if (%this.attackObj != 0 && %this.attackObj.getState() !$= "Dead")
  {
    %this.setAlertness(3);
    if (%this.shouldTakeCover(%this.attackObj))
    {
      %this.coverObj = %this.attackObj;
      if (%this.coverObjIsVisible)
      {
        %this.suppressPos = %this.coverObj.getWorldBoxCenter();
        %this.lastKnownEnemyPos = %this.coverObj.getPosition();
      }
      %this.state = "FINDCOVER";
      %this.huntNodeIndex = -1;
      %this.furtherSearching = 0;
      %plr.setMoveSpeed(1.0);
      %plr.stopAimPattern();
        return;
    }
    else
    {
      if (%this.attackObjIsVisible)
      {
        %this.state = "CLOSEATTACK";
        %plr.setAimObject(%this.attackObj,"0 0 1.5");
        %this.lastKnownEnemyPos = %this.attackObj.getTrueEyePoint();
        %plr.suppressingFire(%this.attackObj.getWorldBoxCenter(),1,%this.attackObj);
        %this.huntNodeIndex = %plr.getHListCount()+1; // So we stop searching
        %this.furtherSearching = 0;
        %plr.setMoveSpeed(1.0);
        %plr.setMoveDestination("");
        %plr.stop();
        %plr.stopAimPattern();
        %plr.patharray.empty();
          return;
      }
    }
  }
  // See if we killed an enemy
  if (%this.attackObj && %this.attackObj.getState() $= "Dead")
  {
    %plr.clearAim();
    %plr.stopAimPattern();
    %plr.setMoveSpeed(1.0);
    %this.attackObj = 0;
      return;
  }

  // Evade grenades
  if (isObject(%this.evadeObj))
  {
    %plr.stopAimPattern();
    %this.lastEvadeState = %this.state;
    if (VectorDist(%plr.targetPathDestination, %plr.getPosition()) > 2)
      %this.lastDestination = %plr.targetPathDestination;
    else
      %this.lastDestination = %plr.getPosition();

    %this.state = "GRENADE";
    return;
  }

  if (!%this.threatPosition || %this.threatType $= "")
  {
    %plr.stopAimPattern();
    %this.threatType = "";
    %this.threatPosition = 0;
    %plr.setMoveSpeed(1.0);
    if (!%plr.squad)
    {
      %this.state = "NONE";
      %this.huntNodeIndex = 0;
      %this.furtherSearching = 0;
      %plr.setPlayerPosition(1); //Stand
        return;
    }
    else
    {
      %this.state = %plr.squad.currentOrder;
      //%this.state = "REGROUP";
      %this.huntNodeIndex = 0;
      %this.furtherSearching = 0;
      %plr.setPlayerPosition(1); //Stand
        return;
    }
  }
  if (%this.threatType $= "Player") // We hear an enemy player
  {
    %plr.setMoveSpeed(0.2);
    %plr.setOnPath(%this.threatPosition);

    // Start an aim pattern so that we search the area while moving
    if (%plr.aimPatternType == -1)
       %plr.startAimPattern(1,getRandom(0,20));

    if (VectorDist(%plr.getPosition(),%this.threatPosition) < 2 && %this.veryLargeChanceRandom()) // Investigate further
    {
      echo(%plr @ ": going to investigate");
      %this.invPosition = %this.threatPosition;
      %this.invStartPosition = %this.myPosWhenHeard;
      %this.threatPosition = 0;
      %this.threatType = "";
      %this.state = "INVESTIGATE";
      return;
    }
  }
}

// Run away from a grenade
function Brain1::stateGrenade(%this)
{
  %this.state = "GRENADE";
  %plr = %this.player;

  %plr.visMod = 1.0;
  %plr.setCoverPosition(5);
  %plr.setPlayerPosition(1);
  %this.setAlertness(3);
  %plr.clearAim();
  %plr.setMoveSpeed(1.0);

  //-----------------------------------------------------------------
  // Quit the state if our evadeObj no longer exists (if it blew up...)
  //-----------------------------------------------------------------
  if (!isObject(%this.evadeObj))
  {
    %this.evadeObj = 0;
    %this.assistObj = 0;
    %this.attackObj = 0;
    %this.state = %this.lastEvadeState;
    %this.lastEvadeState = "NONE";
    %plr.targetPathDestination = 0;
    %plr.setMoveSpeed(1.0);
      return;
  }

  //-----------------------------------------------------------------
  // Run away from the grenade
  //-----------------------------------------------------------------
  %dist = VectorDist(%plr.getPosition(),%this.evadeObj.position);
  if (%plr.isStopped() && %dist < %this.evadeObj.getDatablock().damageRadius)
  {
    warn(%plr @ ": avoiding grenade");
    %this.canWander = true;
    %plr.runAwayFrom(%dist * 2, %dist * 10,%this.evadeObj.position); //hack?
    %plr.setMoveSpeed(1.2);
  }
}

// Attack targets when cover is either not worth it or not available
function Brain1::stateCloseAttack(%this)
{
  %this.state = "CLOSEATTACK";
  %plr = %this.player;

  %plr.visMod = 1.4;
  %plr.setCoverPosition(5);

  //-----------------------------------------------------------------
  // Reload empty weapons
  //-----------------------------------------------------------------
  if (%this.shouldReload())
  {
    %this.reloadWeapon();
  }

  //-----------------------------------------------------------------
  // See if we killed our enemy
  //-----------------------------------------------------------------
  if (!isObject(%this.attackObj) || %this.attackObj.getState() $= "Dead")
  {
    if (!%plr.squad)
      %this.state = "NONE";
    else
      %this.state = %plr.squad.currentorder;

    %plr.targetPathDestination = 0;
    %plr.setPlayerPosition(1);
    %plr.setMoveSpeed(1.0);
    %this.attackActiveCount = 0;

    // Momentarily stop thinking to avoid endless loop crashes (they happen sometimes with the state system)
    %plr.setThinking(false);
    %plr.schedule(1000,setThinking,true);

    return;
  }

  //-----------------------------------------------------------------
  // Keep trying to grab some cover
  //-----------------------------------------------------------------
  if (%this.largeChanceRandom() && %this.attackActiveCount < 20) // hack
  {
    if (isObject(%this.attackObj) && %this.shouldTakeCover(%this.attackObj))
    {
      %this.coverObj = %this.attackObj;
      if (%this.coverObjIsVisible)
      {
        %this.suppressPos = %this.coverObj.getWorldBoxCenter();
        %this.lastKnownEnemyPos = %this.coverObj.getPosition();
      }
      %this.state = "FINDCOVER";
        return;
    }
  }

  //-----------------------------------------------------------------
  // Exit this state after we can't find the target for long enough
  //-----------------------------------------------------------------
  if (%this.attackActiveCount > 60)
  {
    if (!%plr.squad)
      %this.state = "NONE";
    else
      %this.state = %plr.squad.currentorder;

    %plr.targetPathDestination = 0;
    %plr.setCoverSearching(false);
    %plr.schedule(5000,setCoverSearching,true);
    %this.attackObj = 0;
    %this.coverObj = 0;
    %plr.setPlayerPosition(1);
    %plr.setMoveSpeed(1.0);
    %this.attackActiveCount = 0;
    return;
  }

  //-----------------------------------------------------------------
  // Evade grenades
  //-----------------------------------------------------------------
  if (isObject(%this.evadeObj))
  {
    %this.lastEvadeState = %this.state;
    if (VectorDist(%plr.targetPathDestination, %plr.getPosition()) > 2)
      %this.lastDestination = %plr.targetPathDestination;
    else
      %this.lastDestination = %plr.getPosition();

    %this.state = "GRENADE";
    return;
  }

  //-----------------------------------------------------------------
  // Attack Enemies
  //-----------------------------------------------------------------
  if (isObject(%this.attackObj) && %this.attackObj.getState() !$= "Dead")
  {
    %plr.setMoveSpeed(1.0);
    if (%this.stress > 1) // && !%this.crouchWillBreakLOS(%this.attackObj.getTrueEyePoint())) // Crouch if we're under fire
      %plr.setPlayerPosition(2);
    else
      %plr.setPlayerPosition(1); // Otherwise, stand

    if (%plr.squad && %plr.squad.roe == 1)
      %plr.setPlayerPosition(2);

    if(%this.crouchWillBreakLOS(%this.attackObj.getTrueEyePoint()) && %plr.getPlayerPosition() != 1)
      %plr.setImageTrigger(0,0);

    if (%this.attackObjIsVisible == true)
    {
      %this.attackActiveCount = 0;
      %this.lastKnownEnemyPos = %this.attackObj.getWorldBoxCenter();
      %this.losWithEnemy = true;

      if (%this.noCoverSearching != true && %this.getOppositionCount() > 1 && !%this.attackObj.isCivilian)
      {
        if (isObject(%this.attackObj) && %this.attackObjIsVisible)
          %plr.burstFire(%this.attackObj.getTrueEyePoint(),false,%this.attackObj);

        %this.coverObj = %this.attackObj;
        %this.state = "FINDCOVER";
        %this.isFlanked = 0;
        return;
      }
      %plr.stop();

      // See if we should snipe this target
      if (%this.stress < 1)
      {
        %distToTarget = VectorDist(%plr.getTrueMuzzlePoint(0),%this.attackObj.getTrueEyePoint());
        if (%distToTarget > 40) // hack
        {
          %plr.stop();
          %this.spread = 0.01/1000; // hack
          // Increase view distance
          %plr.visMod = 8.0; //hack
          // Take the shot
          %plr.fire(%this.attackObj.getTrueEyePoint(),0,%this.attackObj); // Fire at their head
          return;
        }
      }

      %distToTarget = VectorDist(%this.attackObj.getPosition(),%plr.getPosition());
      if (%distToTarget < $Brain1::ChaseDist)
      {
        %plr.setMoveSpeed(0.5);

        // If we are not at our attack node and we are not moving, then find an attack node
        %distToAttackNode = VectorDist(%plr.getPosition(),%plr.attackNode);
        if (%distToAttackNode > 2 && %plr.canSearchBestAttackNode != false)
        {
          %bestAttackNode = %plr.getBestAttackNode(%this.attackObj.getPosition(),%this.attackObj,60,5,20).getPosition(); //hack
          //%distToAttackNode = %plr.getPathDistance(%plr.getPosition(),%bestAttackNode);
          %plr.canSearchBestAttackNode = false;

          %plr.attackNode = %bestAttackNode;
          return;
        }
      }
      else
      {
        %plr.setMoveSpeed(1.0);
      }
      %plr.setAimLocation(%this.attackObj.getWorldBoxCenter());
      %plr.suppressingFire(%this.attackObj.getWorldBoxCenter(),1,%this.attackObj);
    }
    else
    {
      %this.attackActiveCount++;
      %plr.attackNode = 0;
      %plr.canSearchBestAttackNode = true;

      // If we had line of sight with the enemy during the last think tick, we just lost sight of him, so
      //   remember where we were when we lost him.
      if (%this.losWithEnemy == true)
      {
        %this.LOSBreakPos = %plr.getPosition();
      }
      %this.losWithEnemy = false;

        %shouldSuppress = %plr.shouldSuppress(AIPlayer::getGroundPos(%this.lastKnownEnemyPos));
        if (%shouldSuppress == true && %this.attackActiveCount <= $Brain1::CloseAttackSuppressActiveCount)
        {
          %plr.suppressingFire(AIPlayer::getGroundPos(%this.lastKnownEnemyPos),true);
          return;
        }

      // If we are at the last known enemy position, and we don't see our enemy, go search for him
      if (VectorDist(%plr.getPosition(),%this.lastKnownEnemyPos) < 2)// && %this.attackActiveCount >= $Brain1::InvestigateAttackActiveCount)
      {
        if(%plr.isStopped() == true && !%this.attackObjIsVisible && !%plr.squad)
        {
          %this.attackObj = 0;
          %this.threatPosition = 0;
          %this.threatType = "";
          %this.huntNodeIndex = -1;
          %plr.setPlayerPosition(1);

          if (%this.LOSBreakPos)
            %this.invStartPosition = %this.LOSBreakPos;

          %this.invPosition = AIPlayer::getGroundPos(%this.lastKnownEnemyPos);
          %this.state = "INVESTIGATE";
          //%this.stateInvestigate();
            return;
        }
      }
      else
      {
        //if (!%this.attackObj.isCivilian && VectorDist(%plr.getPosition(),%this.lastKnownEnemyPos) < ($Brain1::CloseAttackInvestigateDist / 1.5) && %plr.getPathDistance(%plr.getPosition(),%this.lastKnownEnemyPos) <= $Brain1::CloseAttackInvestigateDist && !%plr.squad)
        if (VectorDist(%plr.getPosition(),%this.lastKnownEnemyPos) < $Brain1::CloseAttackInvestigateDist && !%plr.squad)
        {
          %plr.setOnPath(%this.lastKnownEnemyPos);
        }
      }
    }
  }
}

// Assist a friend in trouble
function Brain1::stateAssist(%this)
{
  %this.state = "ASSIST";
  %plr = %this.player;

  %plr.visMod = 1.0;
  %plr.setCoverPosition(5);

  //-----------------------------------------------------------------
  // Quit the state if our assistObj doesn't exist, dies, or gives up
  //-----------------------------------------------------------------
  if (!isObject(%this.assistObj) || %this.assistObj.getState() $= "Dead" || (%this.assistObj.brain && !%this.assistObj.brain.attackObj))
  {
    %this.attackActiveCount = 0;
    %plr.setPlayerPosition(1);
    %this.assistObj = 0;
    %this.attackObj = 0;
    %this.state = "NONE";
      return;
  }

  //-----------------------------------------------------------------
  // Reload empty weapons
  //-----------------------------------------------------------------
  if (%this.shouldReload())
  {
    %this.reloadWeapon();
  }

  //-----------------------------------------------------------------
  // Attack enemies
  //-----------------------------------------------------------------
  if (isObject(%this.attackObj) && %this.attackObj.getState() !$= "Dead")
  {
    if (%this.attackObjIsVisible)
    {
      %plr.setPlayerPosition(2);
      %plr.suppressingFire(%this.attackObj.getTrueEyePoint(),true,%this.attackObj);

      if (%this.shouldTakeCover(%this.attackObj))
      {
        %this.coverObj = %this.attackObj;
        if (%this.coverObjIsVisible)
        {
          %this.suppressPos = %this.coverObj.getWorldBoxCenter();
          %this.lastKnownEnemyPos = %this.coverObj.getPosition();
        }
        %this.coverMarker = 0;
        %this.state = "FINDCOVER";
          return;
      }
    }
  }
  else
  {
   // %plr.clearAim();
  }

  //-----------------------------------------------------------------
  // Evade grenades
  //-----------------------------------------------------------------
  if (isObject(%this.evadeObj))
  {
    %this.lastEvadeState = %this.state;
    if (VectorDist(%plr.targetPathDestination, %plr.getPosition()) > 2)
      %this.lastDestination = %plr.targetPathDestination;
    else
      %this.lastDestination = %plr.getPosition();

    %this.state = "GRENADE";
    return;
  }

  //-----------------------------------------------------------------
  // Update the active counter so we do not assist for too long
  //-----------------------------------------------------------------
  if ((isObject(%this.attackObj) && %this.isObjVisible(%this.attackObj)) || %this.assistObj.isImageFiring(0))
  {
    %this.attackActiveCount = 0;
  }
  else
  {
    %this.attackActiveCount++;
    if (%this.attackActiveCount >= $Brain1::AssistActiveCount)
    {
      %this.assistObj = 0;
      %this.attackObj = 0;
      %this.state = "NONE";
      %this.attackActiveCount = 0;
        return;
    }
  }

  //-----------------------------------------------------------------
  // Assist human players
  //-----------------------------------------------------------------
  if (!%this.assistObj.brain) // If no ai brain, we are helping a human player
  {
    warn(%plr @ ": assisting human player " @ %this.assistObj);
    %distToAO = VectorDist(%plr.getPosition(),%this.assistObj.getPosition());
    if (%distToAO >= $Brain1::AssistObjFollowDist)
    {
      // Move in to follow our assistObj
      if (%this.isClearTo(%this.assistObj.getPosition()))
        %plr.setMoveDestination(%this.assistObj.getPosition());
      else
        %plr.setOnPath(%this.assistObj.getPosition());
    }
    else
    {
      %plr.stop();
    }
  }
  else // Otherwise, assist an AI player
  {
    warn(%plr @ ": assisting AIPlayer " @ %this.assistObj);
    %plr.setPlayerPosition(1);
    %distToAO = VectorDist(%plr.getPosition(),%this.assistObj.getPosition());
    %assistObjEnemy = %this.assistObj.brain.attackObj;
    %aimLoc = %assistObjEnemy.position;
    if (isObject(%assistObjEnemy) && %aimLoc && %this.assistObj.brain.attackObjIsVisible) // hack
    {
      %distToAimLoc = VectorDist(%plr.getPosition(),%aimLoc);
      if (%distToAimLoc >= 5 || !%this.isClearTo(%aimLoc)) //hack
      {
        %plr.setOnPath(%aimLoc,true); // Move to intercept/flank the enemy
      }
      else
      {
        %plr.stop();
      }
    }
    else
    {
      if (%distToAO >= $Brain1::AssistObjFollowDist)
      {
        // Move in to follow our assistObj
        if (%this.isClearTo(%this.assistObj.getPosition()))
          %plr.setMoveDestination(%this.assistObj.getPosition());
        else
          %plr.setOnPath(%this.assistObj.getPosition());
      }
      else
      {
        %plr.stop();
      }
    }
    // If we are assisting an AI player, we can both share the same attackObj
    if (!isObject(%this.attackObj) && %this.assistObj.brain.attackObj)
    {
      %this.attackObj = %this.assistObj.brain.attackObj;
      warn(%plr @ ": sharing attackObj " @ %this.assistObj.brain.attackObj @ " with assistObj " @ %this.assistObj);
    }
    if (%this.assistObj.brain.coverObj && %this.assistObj.brain.state $= "FINDCOVER")
    {
      warn(%plr @ ": sharing coverObj " @ %this.coverObj.brain.attackObj @ " with assistObj " @ %this.assistObj);
      %this.coverObj = %this.assistObj.brain.coverObj;
      %this.attackObj = %this.assistObj.brain.attackObj;
      %this.lastKnownEnemyPos = %this.assistObj.brain.lastKnownEnemyPos;
      %this.state = "FINDCOVER";
      %this.suppressPos = %this.assistObj.brain.suppressPos;
      %this.assistObj = 0;
      return;
    }
  }
}

// search for and fire from cover
function Brain1::stateFindCover(%this)
{
  //----------------------------------------------------------------------------------
  // **Things to know**
  //  - AttackObj: We see an enemy who we'd like to attack. AttackObj is him.
  //  - CoverObj: The guy we took cover from
  //  - CoverPos: Where we go to take cover
  //  - ReturnFirePos: If we can't fire over our cover, where we move to return fire.
  //  - %this.stress: How much gunfire we're taking. High stress levels keep us behind
  //                  cover and prevent us from shooting back (also known as being
  //                  "pinned down.")
  //----------------------------------------------------------------------------------

  %plr = %this.player;
  %this.state = "FINDCOVER";
  %plr.visMod = 1.25;
  %this.setAlertNess(3); // Full alert if we are attacking something

  // If we are attacking a civilian, we don't need cover
  if (%this.attackObj.isCivilian == true)
  {
    %this.coverObj = 0;
    %this.coverMarker.isTaken = 0;
    %this.coverMarker = 0;
    %this.coverPos = 0;
    %this.state = "CLOSEATTACK";
    return;
  }

  // Set the suppress pos
  if (%this.attackObjIsVisible)
    %this.suppressPos = %this.attackObj.getWorldBoxCenter();

  if (%this.noCoverSearching == true) // We do this so that we don't endlessly search for cover
  {
    // fire at our attackObj so we're not a sitting duck
    if (isObject(%this.attackObj) && %this.attackObjIsVisible)
    {
      %plr.burstFire(%this.attackObj.getTrueEyePoint,false,%this.attackObj);
    }
    if (isObject(%this.attackObj) && %this.attackObjIsVisible)
    {
      %this.state = "CLOSEATTACK";
      //%this.stateCloseAttack();

      //%plr.requestWander(4,35);
      //%plr.setAimLocation(%this.attackObj.getWorldBoxCenter());
      //%plr.suppressingFire(%this.attackObj.getWorldBoxCenter(),true,%this.attackObj);
      //%this.lastKnownEnemyPos = %this.attackObj.getTrueEyePoint();
    }
    %this.attackActiveCount = 0;
    %this.coverPos = 0;
    %this.movingToCover = true;
    %this.coverMarker.isTaken = false;
    %this.coverMarker = 0;
    %this.shootOverCover = true;
    %this.coverSearchCounter = 0;
    %this.coverMovePosition = 0;
    %this.assistObj = 0;
    %plr.setCoverPosition(5); // Reset cover position
    //if (%plr.squad) // && %plr.squad.member[0] && %plr.squad.member[0].getState() !$= "Dead")
    //{
      //%this.objectiveMovePosition = 0; // so the aiPlayer recalculates the path
      //%this.lastMovePosition = 0;
      //%plr.squad.currentOrder = "Regroup";
      //%this.state = %plr.squad.currentorder;
        //return;
    //}
    //else
    //{
      //%this.state = "NONE";
        //return;
    //}
    return;
  }

  if (%this.shouldReload())
  {
    %plr.setImageTrigger(0,0);

    %this.isReturningFire = false;
    if (!%this.reloading) // If we haven't reloaded yet, let's do it.
    {
      %this.reloadWeapon();
    }

    // See if our attackObj has flanked us
    if (%this.coverMarker)
    {
      %markerVec = VectorNormalize(%this.coverMarker.getForwardVector());
      %vecToAttackObj = VectorNormalize(VectorSub(%this.suppressPos,%plr.getPosition()));
      %dot = VectorDot(%markerVec,%vecToAttackObj);
      if (%dot < 0.6) // hack
      {
        %this.bumpOutOfCover();
        %this.coverPos = 0;
        %this.coverObj = 0;
        %this.coverMarker.isTaken = 0;
        %this.coverMarker = 0;
        %this.state = "CLOSEATTACK";
        %plr.setCoverPosition(5);
          return;
      }
    }

    if (%this.coverPos && !%this.isFlanked)
    {
      // This if/else bit makes the AI player hide behind their cover to reload.
      // There's no sense in exposing ourselves to fire when we can't shoot back.
      if (%this.shootOverCover)
      {
        if (!%this.movingToCover)
        {
          %plr.setPlayerPosition(2); // Crouch
        }
      }
    }
  }

  if (!isObject(%this.coverObj) || %this.coverObj == 0 || %this.coverObj.getState() $= "Dead") // If the guy we're taking cover from has died
  {
    if (%this.shootOverCover)  // Get out of cover to find a new target
    {
      %plr.setPlayerPosition(1);
    }
    else
    {
      if (%this.returnFirePos)
      {
        %plr.setMoveDestination(%this.returnfirepos);
      }
    }
    %plr.stop();
    %this.bumpOutOfCover();
    %plr.targetPathDestination = 0;

    %this.movingToCover = true;
    %this.attackActiveCount = 0;
    %this.coverPos = 0;
    %this.returnFirePos = 0;
    %this.coverMarker.isTaken = false;
    %this.coverMarker = 0;
    %this.coverMovePosition = 0;
    %this.attackObj = 0;
    %this.coverObj = 0;
    %this.coverSearchCounter = 0;
    %this.suppressPos = 0;
    %plr.setCoverPosition(5); // Reset cover position

    // Momentarily stop thinking to avoid endless loop crashes (they happen sometimes with the state system)
    %plr.setThinking(false);
    %plr.schedule(1000,setThinking,true);

    if (%plr.squad)
    {
      %this.state = %plr.squad.currentOrder;
      %this.objectiveMovePosition = 0; // so the aiPlayer recalculates the path
      %this.lastMovePosition = 0;
      %this.orderMovePosition = 0;
      %this.coverObj = 0;
      %this.attackObj = 0; // ADDED DEC 21 2008
      %plr.clearAim();
        return;
    }
    else
    {
      %this.attackObj = 0; // ADDED DEC 21 2008
      %this.state = "NONE";
        return;
    }
  }

  if (isObject(%this.attackObj) && %this.attackObj.getState() $= "Dead")
  {
    %this.attackObj = 0;
    %this.coverPos = 0;
    %this.coverMarker.isTaken = 0;
    %this.coverMarker = 0;
    %this.coverObj = 0;
    %plr.setCoverPosition(5);
    %plr.setPlayerPosition(1);
    if (%plr.squad)
      %this.state = %plr.squad.currentOrder;
    else
      %this.state = "NONE";

      return;
  }

  //-----------------------------------------------------------------
  // Evade grenades
  //-----------------------------------------------------------------
  if (isObject(%this.evadeObj))
  {
    %this.lastEvadeState = %this.state;
    if (VectorDist(%plr.targetPathDestination, %plr.getPosition()) > 2)
      %this.lastDestination = %plr.targetPathDestination;
    else
      %this.lastDestination = %plr.getPosition();

    %this.state = "GRENADE";
    return;
  }

  //if (%this.lastKnownEnemyPos && %this.attackActiveCount >= $Brain1::InvestigateAttackActiveCount)
  if (%this.attackActiveCount >= $Brain1::InvestigateAttackActiveCount)
  {
    if (isObject(%this.attackObj) && !%this.attackObjIsVisible)
    {
      %n = getRandom(0,$Brain1::ChanceOfInvestigate);
      if (%n == 1)
      {
        if (!%plr.squad && %plr.getPathDistance(%plr.getPosition(),%this.lastKnownEnemyPos) < $Brain1::InvestigateDistance)
        {
          %this.bumpOutOfCover();
          %this.threatType = "";
          %this.threatPosition = 0;
          %this.attackActiveCount = 0;
          %this.attackObj = 0;
          %this.coverObj = 0;
          %this.coverPos = 0;
          %this.movingToCover = true;
          %this.lastMovePosition = 0;
          %this.coverMarker.isTaken = false;
          %this.coverMarker = 0;
          %this.shootOverCover = true;
          %this.coverSearchCounter = 0;
          %this.coverMovePosition = 0;
          %this.assistObj = 0;
          %plr.setCoverPosition(5); // Reset cover position
          %plr.setPlayerPosition(1); // Stand
          %this.invPosition = AIPlayer::getGroundPos(%this.lastKnownEnemyPos);
          %this.state = "INVESTIGATE";
            return;
        }
        else
        {
          %this.bumpOutOfCover();
          %this.attackActiveCount = 0;
          %this.attackObj = 0;
          %this.coverObj = 0;
          %this.coverPos = 0;
          %this.movingToCover = true;
          %this.lastMovePosition = 0;
          %this.coverMarker.isTaken = false;
          %this.coverMarker = 0;
          %this.shootOverCover = true;
          %this.coverSearchCounter = 0;
          %this.coverMovePosition = 0;
          %this.assistObj = 0;
          %plr.setCoverPosition(5); // Reset cover position
          %plr.setPlayerPosition(1); // Stand
          if (%plr.squad)
          {
            %this.state = %plr.squad.currentOrder;
            return;
          }
          else
          {
            %this.state = "NONE";
            return;
          }
        }
      }
    }
  }

  if (!%this.coverPos) // If we don't have a coverPos, we have just entered StateFindCover and need to find some cover
  {
    %plr.setCoverPosition(5); // Reset cover position
    %coverString = %this.getBestCoverMarker(%this.coverObj.getPosition());
    if (firstWord(%coverString) !$= "" && %coverString != 0)
    {
      %coverMarker = getWord(%coverString,0);
      %coverPos = %coverMarker.getPosition();
      %fireOverCover = getWord(%coverString,1);
      %returnFirePos = getWords(%coverString,2,4); // Words 2 through 4 is the position where we can return fire
      %fireRight = getWord(%coverString,5); // If we fire around the right side of cover

      %this.coverPos = %coverPos;
      %this.coverMarker = %coverMarker;
      %this.coverMarker.isTaken = true; // Mine!
      if (%fireOverCover == 0)
      {
        %this.returnFirePos = %returnFirePos;
        %this.shootRight = %fireRight;
        %this.shootOverCover = 0;
      }
      else
      {
        %this.shootOverCover = 1;
      }
    }
    else
    {
      if (isObject(%this.attackObj))
      {
        %this.stateCloseAttack();
        %plr.setPlayerPosition(1);
          return;
      }
      else
      {
        %plr.setImageTrigger(0,0);
        // Try to move to intercept the threat
        if (%this.lastKnownEnemyPos)
        {
          %plr.setOnPath(%this.lastKnownEnemyPos,true);
        }
      }
      %plr.setCoverSearching(false); // No cover searching
      %plr.schedule(2000,setCoverSearching,true); // Allow cover searching after a few seconds
    }
    return;
  }
  if (isObject(%this.coverMarker) && VectorDist(%plr.getPosition(), %this.coverPos) > 1.5 && %this.coverObj && %this.coverObj.getState() !$= "Dead") // had || !%this.isClearTo(%this.coverPos)) before && %this.coverObj on feb 8 09
  {
    if (VectorDist(%plr.getPosition(),%this.coverMarker.getPosition()) < $Brain1::CoverRunSuppressDist)
    {
      if (%this.isObjVisible(%this.coverObj))
      {
        //%plr.setAimLocation(%this.coverObj.getTrueEyePoint());
        if (%this.attackObjIsVisible)
          %plr.suppressingFire(%this.coverObj.getTrueEyePoint(),1,%this.coverObj);  // Lay down suppressing fire on the way to our cover
      }
      else
      {
        if (%this.suppressPos)
        {
          %plr.setAimLocation(%this.suppressPos);
        }
        else
        {
          // Aim the direction our cover marker is pointed
          if (isObject(%this.coverMarker))
          {
            %startPos = %this.coverMarker.getPosition();
            %fwdVec = %this.coverMarker.getForwardVector();
            %fwdVec = VectorNormalize(%fwdVec);
            %fwdVec = VectorScale(%fwdVec,100);
            %endPos = VectorAdd(%startPos,%fwdVec);
            %plr.setAimLocation(%endPos);
          }
        }
      }
    }
    else
    {
      %plr.clearAim();
    }

    %plr.setPlayerPosition(1);
    %plr.setCoverPosition(5);

    %movePos = %this.coverMarker.position;
    %this.coverMovePosition = %movePos;
    %this.movingToCover = true;
    if (!ContainerRayCast(VectorAdd(%plr.getPosition(),"0 0 0.4"),VectorAdd(%this.coverMarker.getPosition(),"0 0 0.4"),$Brain1::SolidMasks)) // If we have a clear path to cover hack!!
    {
      if ((getWord(%movePos,2) - getWord(%plr.getPosition(),2)) < 2) // If we are nearly the same height as the cover marker (so we dont get tricked by markers on top of hills)
      {
        %plr.setMoveDestination(%this.coverPos); // Go directly to the cover
      }
      else
      {
        %path = %plr.setOnPath(%this.coverPos); // Otherwise, calculate a path
      }
    }
    else
    {
      %path = %plr.setOnPath(%this.coverPos);
    }

    // If we couldn't get to cover, enter the close attack state
    if (%path !$= "" && %path == -1)
    {
      warn(%plr @ ": cannot path to cover");
      if (%this.coverObj)
      %this.attackObj = %this.coverObj;
      %this.coverPos = 0;
      %this.coverObj = 0;
      %this.coverMarker.isTaken = 0;
      %this.coverMarker = 0;
      //%this.threatPosition = 0;
      //%this.threatType = "None";

      if (!%plr.squad)
        %this.state = "CLOSEATTACK";
        //%this.state = "CLOSEATTACK";
      else
        %this.state = %plr.squad.currentOrder;

      %plr.setCoverSearching(false); // No cover searching
      %plr.schedule(3000,setCoverSearching,true); // Allow cover searching after a few seconds
      %plr.setCoverPosition(5);
      return;
    }
  }

  if (isObject(%this.coverMarker) && VectorDist(%plr.getPosition(),%this.coverPos) <= 2.0 && %this.isClearTo(%this.coverPos)) // || VectorDist(%plr.getPosition(),%this.returnFirePos) <= 1.2)
  {
    %this.movingToCover = false;
    %stress = %this.stress;

    // Snap to the marker's position so we don't get stuck
    if (VectorDist(%plr.getPosition(),%this.coverPos) <= 1.8 && %this.shootOverCover == true)
    {
      %plr.stop();
      %plr.setTransform(AIPlayer::getGroundPos(%this.coverMarker.getPosition()));
    }
    if (isObject(%this.coverMarker) && VectorDist(%plr.getPosition(),%this.coverPos) <= 2.5 && %this.shootOverCover == false) // && %this.getCoverPosition() == 0)// %this.isReturningFire != true)
    {
       // Snap to horizontal cover
      %plr.stop();
      %plr.setTransform(AIPlayer::getGroundPos(%this.coverMarker.getPosition()));
    }

    if (%stress <= $Brain1::MinStressLevel || %this.isFlanked)
    {
      %plr.setPlayerPosition(1); // Stand. If we need to change stances to something other than this, it will happen by the end of this state
      if (%this.shootOverCover == false && %this.returnFirePos) // If we don't shoot over cover
      {
        if (%this.shootRight == 1)
          %plr.setCoverPosition(1);
        else if (%this.shootRight == 0)
          %plr.setCoverPosition(2);
        else
          %plr.setCoverPosition(5);

        %plr.setVelocity("0 0 0");
        //%plr.setMoveDestination(%this.returnfirepos);
        %this.isReturningFire = true;
        %plr.setTransform(AIPlayer::getGroundPos(%this.returnFirePos));
      }
      if (%this.shootOverCover $= true)
      {
        %this.isReturningFire = true;
        %plr.setCoverPosition(4);
        %plr.setVelocity("0 0 0");
      }

      // Hide when reloading
      if (%this.reloading == true)
      {
        if (isObject(%this.attackObj) && %this.attackObjIsVisible)
        {
          %this.attackActiveCount = 0;
          //%plr.setAimLocation(%this.attackObj.getTrueEyePoint());
          %this.suppressPos = %plr.getSuppressPos(%this.attackObj);
          %this.lastKnownEnemyPos = %this.attackObj.getTrueEyePoint();
        }
        if (!%this.isFlanked)
        {
          %plr.stop();
          if (AIPlayer::getXYDistance(%plr.getPosition(),%this.coverPos) > 0.1)
            %plr.setTransform(AIPlayer::getGroundPos(%this.coverPos));
          %plr.setVelocity("0 0 0");
          if (%this.shootOverCover == true)
          {
            %plr.setCoverPosition(3);
          }
          else
          {
            %plr.setCoverPosition(0);
          }
          return;
        }
        else
        {
          %plr.setVelocity("0 0 0");
          %plr.setCoverPosition(5);
          %plr.setPlayerPosition(2);
        }
      }
      else
      {
        %plr.setPlayerPosition(1); // Remove this?
      }

      // Now we do our thing here. If we've got a target, let's deal with him.
      if (isObject(%this.attackObj) && isObject(%this.coverMarker)) // If we have an attackObj
      {
        // See if our attackObj has flanked us
        if (%this.attackObjIsVisible)
          %enemyPosition = %this.attackObj.getPosition();
        else
          %enemyPosition = %this.suppressPos;
        %distToEP = VectorDist(%plr.getPosition(),%this.attackObj.getPosition());
        %markerVec = VectorNormalize(%this.coverMarker.getForwardVector());
        %vecToAttackObj = VectorNormalize(VectorSub(%enemyPosition,%plr.getPosition()));
        %dot = VectorDot(%markerVec,%vecToAttackObj);
        if ((%this.attackObjIsVisible && %this.isExposed(%this.attackObj) && (%dot < 0.6 && %this.suppressPos)) || %distToEP < $Brain1::CoverEnemyCloseDist) // hack, 0.6
        {
          %plr.setCoverPosition(5);
          %this.isFlanked = 1;
          %this.coverPos = 0;
          %this.coverObj = 0;
          %this.coverMarker.isTaken = 0;
          %this.coverMarker = 0;
          %this.state = "CLOSEATTACK";
          %plr.setCoverSearching(false);
          %plr.schedule(5000,setCoverSearching,true);
            return;
        }
        else
        {
          %this.isFlanked = 0;
        }

        if (%this.shootOverCover == true) // If we have an attackObj and we fire over cover
        {
          %plr.setPlayerPosition(1);
          if (%this.attackObjIsVisible) // If we have an attackObj, we fire over cover, and we can see him
          {
            %this.attackActiveCount = 0;
            %plr.burstFire(%this.attackObj.getTrueEyePoint(),0,%this.attackObj);
            %this.suppressPos = %plr.getSuppressPos(%this.attackObj); // We just saw him, so save his last known location so we can lay suppressing fire on it if we lose sight of him
            %this.lastKnownEnemyPos = %this.attackObj.getEyePoint();
          }
          else // If we have an attackObj, we fire over cover, and we CAN'T see him
          {
            %this.attackActiveCount++;
            %moveUp = %this.smallChanceRandom(); // Randomly move to better cover, if it's available
            if (%this.lastKnownEnemyPos && %moveUp == "1")
            {
              %coverString = %this.getBestCoverMarker(%this.lastKnownEnemyPos,true);
              %bestMarker = getWord(%coverString,0);
              %myMarker = %this.coverMarker;
              if (%coverString !$= "" && %coverString !$= "0" && %bestMarker != %myMarker)
              {
                echo(%plr @ ": found better cover! My marker is " @ %myMarker @ ", better marker is " @ %bestMarker);
                %coverMarker = %bestMarker;
                %coverPos = %coverMarker.getPosition();
                %fireOverCover = getWord(%coverString,1);
                %returnFirePos = getWords(%coverString,2,4); // Words 2 through 4 is the postition where we can return fire
                %fireRight = getWord(%coverString,5);

                // Make sure others can use the cover marker we're about to leave
                %this.coverMarker.isTaken = false;

                %this.coverPos = %coverPos;
                %this.coverMarker = %coverMarker;
                %this.coverMarker.isTaken = true; // Mine!
                if (%fireOverCover == false)
                {
                  %this.returnFirePos = %returnFirePos;
                  %this.shootRight = %fireRight;
                  %this.shootOverCover = 0;
                }
                else
                {
                  %this.shootOverCover = 1;
                }
                return;
              }
            }
            if (%this.suppressPos && %this.attackActiveCount <= $Brain1::MaxAttackActiveCountToSuppress)
            {
              // Lay down a bit of suppressing fire
              %n = getRandom(0,2) + 1;
              if (!%plr.getMountedImage(0).usePreciseAim)
              {
                switch$(%n)
                {
                  case 1:
                    %plr.fire(%this.suppressPos,1);
                  case 2:
                    %plr.burstfire(%this.suppressPos,1);
                  case 3:
                    %plr.suppressingFire(%this.suppressPos);
                  default:
                    %plr.fire(%this.suppressPos,1);
                }
              }

              if (%this.largeChanceRandom())
              {
                %plr.flushOut(%this.suppressPos);
              }
            }
          }
        }
        else // If we have an attackObj but we don't fire over cover
        {
          if (%this.attackObjIsVisible) // If we have an attackObj, we don't fire over cover, and we can see him
          {
            %this.attackActiveCount = 0;
            if (%this.returnFirePos)
            {
              //%plr.setMoveDestination(%this.returnfirepos);
              %plr.setTransform(AIPlayer::getGroundPos(%this.returnFirePos));
              %plr.burstFire(%this.attackObj.getTrueEyePoint(),0,%this.attackObj);
              %this.suppressPos = %plr.getSuppressPos(%this.attackObj); // We just saw him, so save his last known location so we can lay suppressing fire on it if we lose sight of him
              %this.lastKnownEnemyPos = %this.attackObj.getEyePoint();
              return;
            }
          }
          else // If we have an attackObj, we don't fire over cover, and we CAN'T see him
          {
            %this.attackActiveCount++;
            %moveUp = %this.smallChanceRandom(); // Randomly move to better cover, if it's available
            if (%this.lastKnownEnemyPos && %moveUp $= "1")
            {
              %coverString = %this.getBestCoverMarker(%this.lastKnownEnemyPos,true);
              %bestMarker = getWord(%coverString,0);
              %myMarker = %this.coverMarker;
              if (%coverString !$= "" && %coverString !$= "0" && %bestMarker != %myMarker)
              {
                echo(%plr @ ": found better cover! My marker is " @ %myMarker @ ", better marker is " @ %bestMarker);
                %coverMarker = %bestMarker;
                %coverPos = %coverMarker.getPosition();
                %fireOverCover = getWord(%coverString,1);
                %returnFirePos = getWords(%coverString,2,4); // Words 2 through 4 is the position where we can return fire
                %fireRight = getWord(%coverString,5);

                // Make sure others can use the cover marker we're about to leave
                %this.coverMarker.isTaken = false;

                %this.coverPos = %coverPos;
                %this.coverMarker = %coverMarker;
                %this.coverMarker.isTaken = true;
                if (%fireOverCover == false)
                {
                  %this.returnFirePos = %returnFirePos;
                  %this.shootRight = %fireRight;
                  %this.shootOverCover = 0;
                }
                else
                {
                  %this.shootOverCover = 1;
                }
                return;
              }
            }
            if (%this.suppressPos && %this.attackActiveCount <= $Brain1::MaxAttackActiveCountToSuppress)
            {
              //%plr.setMoveDestination(%this.returnfirepos);
              %plr.setTransform(AIPlayer::getGroundPos(%this.returnFirePos));
              //%plr.setAimLocation(%this.suppressPos);
              %n = getRandom(0,2) + 1;
              if (!%plr.getMountedImage(0).usePreciseAim)
              {
                switch$(%n)
                {
                  case 1:
                    %plr.fire(%this.suppressPos,1);
                  case 2:
                    %plr.burstfire(%this.suppressPos,1);
                  case 3:
                    %plr.suppressingFire(%this.suppressPos);
                  default:
                    %plr.fire(%this.suppressPos,1);
                }
              }

              if (%this.largeChanceRandom())
              {
                // Flush out the threat
                //%grenadeThrowNode = %plr.getBestGrenadeNode(%this.suppressPos,15);  // hack
                //if (%grenadeThrowNode != -1)
                //{
                //%plr.throwGrenade(%grenadeThrowNode.getPosition());
                %plr.flushOut(%this.suppressPos);
                //}
              }
            }
          }
        }
      }
      if (!%this.attackObjIsVisible) // AttackObj is not visible, determine where to look
      {
        %this.attackActiveCount++;
        if (!%this.suppressPos)
        {
          // Aim the direction our cover marker is pointed
          if (isObject(%this.coverMarker))
          {
            %startPos = %this.coverMarker.getPosition();
            %fwdVec = %this.coverMarker.getForwardVector();
            %fwdVec = VectorNormalize(%fwdVec);
            %fwdVec = VectorScale(%fwdVec,100);
            %endPos = VectorAdd(%startPos,%fwdVec);
            %plr.setAimLocation(%endPos);
          }
        }
        else
        {
          %plr.setAimLocation(%this.suppressPos);
        }
      }
    }
    else // We're pinned down
    {
      %this.isReturningFire = false;

      // See if our attackObj has flanked us
      if (%this.attackObjIsVisible)
        %enemyPosition = %this.attackObj.getPosition();
      else
        %enemyPosition = %this.suppressPos;
      %distToEP = VectorDist(%plr.getPosition(),%this.attackObj.position);
      %markerVec = VectorNormalize(%this.coverMarker.getForwardVector());
      %vecToAttackObj = VectorNormalize(VectorSub(%enemyPosition,%plr.getPosition()));
      %dot = VectorDot(%markerVec,%vecToAttackObj);
      if (isObject(%this.attackObj) && (%this.attackObjIsVisible && %this.isExposed(%this.attackObj) && (%dot < 0.6 && %this.suppressPos)) || %distToEP < $Brain1::CoverEnemyCloseDist) // hack, 0.6
      {
        %plr.setCoverPosition(5);
        %this.isFlanked = 1;
        %this.coverPos = 0;
        %this.coverObj = 0;
        %this.coverMarker.isTaken = 0;
        %this.coverMarker = 0;
        %this.state = "CLOSEATTACK";
        %plr.setCoverSearching(false);
        %plr.schedule(5000,setCoverSearching,true);
          return;
      }
      else
      {
        %this.isFlanked = 0;
      }

      if (%this.shootOverCover $= true) // If we're pinned down and we fire over cover
      {
        %plr.setVelocity("0 0 0");
        %plr.setCoverPosition(3);
        %plr.setPlayerPosition(2); // Crouch
      }
      else // If we're pinned down but we don't fire over cover
      {
        if (%this.coverPos)  // Just a precaution
        {
          %plr.setCoverPosition(0);
          %plr.stop();
          if (AIPlayer::getXYDistance(%plr.getPosition(),%this.coverPos) > 0.1)
            %plr.setTransform(AIPlayer::getGroundPos(%this.coverPos));
          %plr.setVelocity("0 0 0");
        }
      }
    }
  }
  else // Running for cover
  {
    %this.isReturningFire = false;

    if (%this.coverPos && isObject(%this.coverObj) && %this.coverObj.getState() !$= "Dead")
    {
      // Attack threats on the way to cover
      if (isObject(%this.attackObj) && %this.attackObjIsVisible)
      {
        //%plr.setAimLocation(%this.attackObj.getTrueEyePoint());
        if (%this.getOppositionCount() < 2)
        {
          %plr.suppressingFire(%this.attackObj.getTrueEyePoint(),1,%this.attackObj);
        }
        else
        {
          %plr.setImageTrigger(0,0);
          %plr.clearAim();
        }
      }
      else
      {
        // If we are close to cover without a direction to aim, look the direction our cover marker is rotated (or, towards our suppressPos)
        %cDist = VectorDist(%plr.getPosition(),%this.coverMarker.getPosition());
        if (isObject(%this.coverMaker))
        {
          if (%cDist > $Brain1::CoverRunSnapForwardDist)
          {
            %plr.clearAim();
            %plr.setImageTrigger(0,0);
          }
          else
          {
            if (!%this.suppressPos)
            {
              %startPos = %this.coverMarker.getPosition();
              %fwdVec = %this.coverMarker.getForwardVector();
              %fwdVec = VectorNormalize(%fwdVec);
              %fwdVec = VectorScale(%fwdVec,100);
              %endPos = VectorAdd(%startPos,%fwdVec);
              %plr.setAimLocation(%endPos);
            }
            else
            {
              %plr.setAimLocation(%this.suppressPos);
            }
          }
        }
      }
    }
  }
}
//-----------------------------------------------------------------

//----------------------------------------
//  Misc Squad functions
//----------------------------------------
function Brain1::setCurrentOrder(%this,%order)
{
  %this.currentOrder = %order;
}

// If we don't have an attackObj, but another squad member does, we should save it
//  as our attackObj so we can assist
function Brain1::KeepInformed(%this)
{
  %plr = %this.player;
  %squad = %plr.squad;
  if (!%squad)
    return;
  for (%i = 0;%i < %squad.getLivingMemberCount();%i++)
  {
    %member = %squad.member[%i];
    // Only check other AI squad members
    if (%member.brain && isObject(%member))
    {
      if (%member.brain.attackObj && !isObject(%this.attackObj))
      {
        %enemyArray = %plr.enemyArray;
        %isInArray = %enemyArray.countKey(%member.brain.attackObj);
        if (%isInArray == 0)
        {
          %this.attackObj = %member.brain.attackObj;
          %enemyArray.add(%this.attackObj,0);
          echo(%plr @ ": learned about an enemy from a squadmate");
        }
      }
    }
  }
}

//-----------------------------------------------------------------
// Squad states
//-----------------------------------------------------------------

// Regroup and follow the team leader
function Brain1::stateRegroup(%this)
{
  %this.state = "REGROUP";
  %plr = %this.player;
  %plr.visMod = 1;

  %plr.setCoverPosition(5);
  //%plr.clearAim();

  //-----------------------------------------------------------------
  // Mimic the squad leader's stance (if he's human)
  //-----------------------------------------------------------------
  if (%plr.memberIndex != 0 && %plr.squad.member[0].client && %plr.getCoverPosition() == 5 && %plr.squad.roe == 1)
  {
    %plr.setPlayerPosition(%plr.squad.member[0].getPlayerPosition());
    %mimicStance = true;
  }

  //-----------------------------------------------------------------
  // Reload
  //-----------------------------------------------------------------
  if (%this.shouldReload())
  {
    %this.reloadWeapon();
  }

  //-----------------------------------------------------------------
  // Evade grenades
  //-----------------------------------------------------------------
  if (isObject(%this.evadeObj))
  {
    %this.lastEvadeState = %this.state;
    if (VectorDist(%plr.targetPathDestination, %plr.getPosition()) > 2)
      %this.lastDestination = %plr.targetPathDestination;
    else
      %this.lastDestination = %plr.getPosition();

    %this.state = "GRENADE";
    return;
  }
  //-----------------------------------------------------------------
  // Attack Enemies
  //-----------------------------------------------------------------
  if (isObject(%this.attackObj) && %this.attackObj.getState() !$= "Dead")
  {
    %plr.setMoveSpeed(1.0);
    %shouldAttack = %this.shouldReturnFire(%this.attackObj);
    if (%shouldAttack == 1)
    {
      if (%this.shouldTakeCover(%this.attackObj))
      {
        %this.coverObj = %this.attackObj;
        %this.coverMarker.isTaken=0;
        %this.coverMarker = 0;
        %this.coverPos = 0;

        if (%this.coverObjIsVisible)
        {
          %this.suppressPos = %this.coverObj.getWorldBoxCenter();
          %this.lastKnownEnemyPos = %this.coverObj.getPosition();
        }
        %this.objectiveMovePosition = 0;
        %this.objectiveCoverPos = 0;
       // %plr.stop();
        %coverMarker.isTaken = 0;
        %this.state = "FINDCOVER";
          return;
      }
      else
      {
        if (%this.attackObjIsVisible)
        {
          %plr.stop();
          %plr.targetPathDestination = 0;
          warn(%plr @ ": sniping " @ %this.attackObj);

          //%plr.setAimObject(%this.attackObj,"0 0 0.5");
          // Take the shot
          //%plr.pullTrigger();
          %plr.fire(%this.attackObj.getWorldBoxCenter(),0,%this.attackObj);
            return;
        }
      }
    }
    else
    {
      if (%this.attackObjIsVisible)
      {
        %plr.setAimLocation(%this.attackObj.getTrueEyePoint());
        if (%shouldAttack == 2) // Which means that a stealth kill is appropriate
          %plr.fire(%this.attackObj.getTrueEyePoint(),0,%this.attackObj);
        else
          warn(%plr @ ": holding fire...");
      }
      else
        %plr.clearAim();
    }
  }

  // See if we killed an enemy
  if (%this.attackObj && %this.attackObj.getState() $= "Dead")
  {
    %plr.clearAim();
    %this.attackObj = 0;

    // Momentarily stop thinking to avoid endless loop crashes (they happen sometimes with the state system)
    %plr.setThinking(false);
    %plr.schedule(1000,setThinking,true);
      return;
  }

  //-----------------------------------------------------------------
  // React to immediate danger
  //-----------------------------------------------------------------
  if (%this.threatPosition && %this.threatType $= "Fire" && !%plr.squad.quitReact)
  {
    %this.state = "SQUADREACT";
    return;
  }

  //-----------------------------------------------------------------
  // Decide if we should change states
  //-----------------------------------------------------------------
  if (%plr.squad)
  {
    if (%plr.memberIndex == 0) // We are the team leader, decide if we should move for the objective
    {
      if (%plr.squad.isSquadRegrouped() == 1) // If we are regrouped
      {
        // See if we have an objective to fulfill
        if (%plr.squad.objectiveType !$= "" && %plr.squad.objectiveType !$= "NONE" && %plr.squad.objectiveData !$= "") // We have an objective
        {
          %plr.squad.setOrders("Objective");
          %plr.squad.onOrdersChanged();
          %this.state = "OBJECTIVE";
            return;
        }
        // See if we have a location to move to
        if (%plr.squad.moveLocation !$= "" && (%plr.squad.objectiveType $= "" || %plr.squad.objectiveType $= "NONE")) // We have a move location and no objective
        {
          %plr.squad.onOrdersChanged();
          %plr.squad.setOrders("Move");
          %this.state = "MOVE";
            return;
        }
      }
    }
    else // We are not the team leader
    {
      if (%this.currentOrder $= "Move")
      {
        %this.state = "MOVE";
          return;
      }
      if (%this.currentOrder $= "Hold")
      {
        %this.state = "HOLD";
          return;
      }
      if (!%plr.squad.pendingObjective && %this.currentOrder $= "Objective")
      {
        %this.state = "OBJECTIVE";
          return;
      }
    }
  }

  //-----------------------------------------------------------------
  // Figure out how to regroup
  //-----------------------------------------------------------------
  %leader = %plr.squad.member[0];

  if (%plr.memberindex != 0)
  {
    // Set the move speed based on alertness
    %a = %this.alertness;
    if (%a == 0)
      %plr.setMoveSpeed(0.7);
    if (%a == 2)
      %plr.setMoveSpeed(0.8);
    if (%a == 3)
      %plr.setMoveSpeed(1.0);

    if (%plr.squad.roe == 1)
    {
      if (%plr.getPlayerPosition() == 2)
        %plr.setMoveSpeed(0.75);
      if (%plr.getPlayerPosition() == 1)
        %plr.setMoveSpeed(0.75);
    }

    %leaderPos = %plr.squad.member[0].getPosition();
    if (!%this.lastLeaderPosition || (VectorDist(%leaderPos,%this.lastLeaderPosition) >= $Brain1::RegroupTolerance))
    {
      %this.lastLeaderPosition = %leaderPos;
      %formationPos = %plr.squad.getSquadFormationPosition(%plr.memberIndex);
      %this.regroupPosition = %formationPos;

      //%ray = ContainerRayCast(VectorAdd(%this.regroupPosition,"0 0 0.4"),VectorAdd(%plr.getPosition(),"0 0 0.4"),$Brain1::SolidMasks);
      if (%this.isClearTo(%this.regroupPosition))
      {
        %plr.targetPathDestination = %this.regroupPosition; // so we don't confuse the stuck avoidance
        %plr.setMoveDestination(%this.regroupPosition);
      }
      else
      {
        echo(%plr @ ": pathing to " @ %this.regroupPosition);
        %plr.setOnPath(%this.regroupPosition);
      }
    }

    // Make sure we didn't get stuck
    if (%plr.isStopped() && VectorDist(%plr.getPosition(),%this.regroupPosition) > 2)
    {
      echo(%plr @ ": stuck while regrouping");
      if (%this.regroupPosition)
      {
        %plr.setOnPath(%this.regroupPosition);
        %plr.targetPathDestination = 0;
        return;
      }
    }
  }
  else // else, is the squad leader
  {
    %plr.stop();
    if (%this.priorityMoveLocation && !%plr.squad.objectiveData && VectorDist(%plr.getPosition(),%this.priorityMoveLocation) > 5)
    {
      %plr.squad.objectiveMove(%this.priorityMoveLocation);
      return;
    }

    if (%plr.squad.pendingObjective == true) // If we are regrouping while waiting to move to an objective
    {
      if (%plr.squad.isSquadRegrouped() == true)
      {
        // We're regrouped, so get back on the objective
        echo("SQUAD LEADER CHANGING ORDER TO OBJECTIVE");
        %plr.squad.setOrders("Objective");
        %this.state = "OBJECTIVE";
        %plr.squad.onOrdersChanged();
        %plr.squad.pendingObjective = false;
      }
    }
  }

  if (!%mimicStance && !%plr.isStopped())
  {
    if (%plr.squad.roe == 0)
      %plr.setPlayerPosition(1);
    else
      %plr.setPlayerPosition(2);
  }
  //-----------------------------------------------------------------
}

// Fulfill an objective
function Brain1::stateObjective(%this)
{
  %this.state = "OBJECTIVE";
  %plr = %this.player;

  %plr.visMod = 1;
  %plr.setCoverPosition(5);
  %plr.setPlayerPosition(1);

  %this.setAlertness(2);

  //-----------------------------------------------------------------
  // Reload if we need to
  //-----------------------------------------------------------------
  if (%this.shouldReload())
  {
    %this.reloadWeapon();
    return;
  }

  //-----------------------------------------------------------------
  // Evade grenades
  //-----------------------------------------------------------------
  if (isObject(%this.evadeObj))
  {
    %this.lastEvadeState = %this.state;
    if (VectorDist(%plr.targetPathDestination, %plr.getPosition()) > 2)
      %this.lastDestination = %plr.targetPathDestination;
    else
      %this.lastDestination = %plr.getPosition();

    %this.state = "GRENADE";
    return;
  }

  //-----------------------------------------------------------------
  // Attack Enemies
  //-----------------------------------------------------------------
  if (isObject(%this.attackObj) && %this.attackObj.getState() !$= "Dead")
  {
    %plr.setMoveSpeed(1.0);
    %shouldAttack = %this.shouldReturnFire(%this.attackObj);
    if (%shouldAttack == 1)
    {
      if (%this.shouldTakeCover(%this.attackObj))
      {
        %this.coverObj = %this.attackObj;
        %this.coverMarker.isTaken=0;
        %this.coverMarker = 0;
        %this.coverPos = 0;

        if (%this.coverObjIsVisible)
        {
          %this.suppressPos = %this.coverObj.getWorldBoxCenter();
          %this.lastKnownEnemyPos = %this.coverObj.getPosition();
        }
        %this.objectiveMovePosition = 0;
        %this.objectiveCoverPos = 0;
       // %plr.stop();
        %coverMarker.isTaken = 0;
        %this.state = "FINDCOVER";
          return;
      }
      else
      {
        if (%this.attackObjIsVisible)
        {
          %plr.stop();
          %plr.targetPathDestination = 0;
          %plr.suppressingFire(%this.attackObj.getTrueEyePoint(),1,%this.attackObj);
          warn(%plr @ ": opening fire on an easy target");
            return;
        }
      }
    }
    else
    {
      if (%this.attackObjIsVisible)
      {
        %plr.setAimLocation(%this.attackObj.getTrueEyePoint());
        if (%shouldAttack == 2) // Which means that a stealth kill is appropriate
          %plr.fire(%this.attackObj.getTrueEyePoint(),0,%this.attackObj);
        else
          warn(%plr @ ": holding fire...");
      }
      else
        %plr.clearAim();
    }
  }

  //-----------------------------------------------------------------
  // See if we killed an enemy
  //-----------------------------------------------------------------
  if (%this.attackObj && %this.attackObj.getState() $= "Dead")
  {
    %plr.clearAim();
    %this.attackObj = 0;

    // Momentarily stop thinking to avoid endless loop crashes (they happen sometimes with the state system)
    %plr.setThinking(false);
    %plr.schedule(1000,setThinking,true);
      return;
  }

  //-----------------------------------------------------------------
  // React to immediate danger
  //-----------------------------------------------------------------
  if (%this.threatPosition && %this.threatType $= "Fire" && !%plr.squad.quitReact)
  {
    %this.state = "SQUADREACT";
    return;
  }

  //-----------------------------------------------------------------
  // Decide if we have completed our objective
  //-----------------------------------------------------------------
  if (%plr.squad) // If we have a squad,
  {
    if (%plr.memberIndex == 0) // We're the team leader,
    {
      if (%plr.squad.objectiveType $= "Move" && %plr.squad.objectiveData != 0) // We have a move objective,
      {
        if (VectorDist(%plr.getPosition(),%plr.squad.objectiveData) <= 8) // And we've arrived,
        {
          if (%plr.getPathDistance(%plr.getPosition(),%plr.squad.objectiveData) < 20)
          {
            // See if our squad is set to patrol the area
            if (%plr.squad.patrolOrigin)
            {
              warn(%plr @ ": Squad has patrol origin");
              // We have a patrol origin, so pick another random position as our objective
              %radius = %plr.squad.patrolRadius;
              if (%radius > 0)
              {
                echo(%plr @ ": PICKING NEW OBJECTIVE");
                %newObjectivePos = %plr.getRandomNodePosition(%plr.squad.patrolOrigin,0,%radius);
                if (%newObjectivePos != -1)
                {
                  %pathDist = %plr.getPathDistance(%plr.position,%newObjectivePos);
                  echo("PATHDISTANCE: " @ %pathDist);
                  if (%pathDist < 1000) // Not a failed path...   hack
                  {
                    %plr.squad.objectiveData = %newObjectivePos; // Then move there
                  }
                  else
                  {
                    return;
                  }
                }
                else
                {
                  echo("No new objective pos.");
                  return; // Try again on the next think tick
                }
              }
            }
            else // Otherwise, we have arrived at the objective and should regroup
            {
              // Do we have a list of objective points?
              if (%plr.squad.objectiveArray.count() > 0)
              {
                %plr.squad.objectiveNum++;
                if (%plr.squad.objectiveNum < %plr.squad.objectiveArray.count())
                {
                  echo(%plr.squad @ ": fulfilling objective point #" @ %plr.squad.objectiveNum + 1);
                  %plr.squad.objectiveData = %plr.squad.objectiveArray.getKey(%plr.squad.objectiveNum);
                  return;
                }
              }
              else
              {
                echo(%plr.squad @ "- squad arrived at objective position");
                %plr.squad.objectiveType = ""; // Then it's no longer our objective
                %plr.squad.objectiveData = "";
                %plr.squad.setOrders("Regroup");  // Squad should regroup now
                %plr.stop();
                %this.priorityMoveLocation = 0;
                %this.state = "REGROUP";
                return;
              }
            }
          }
        }
      }
    }
  }

  //-----------------------------------------------------------------
  // Move to the position specified by our squad's objectiveData
  //-----------------------------------------------------------------
  %goalPos = %plr.squad.objectiveData;
  if (%plr.squad.objectiveType $= "Move")
  {
    //if (%plr.memberIndex == 0)
      %plr.setMoveSpeed(0.5);
    //else
      //%plr.setMoveSpeed(0.6);

    %pathSuccess = %plr.setOnPath(%goalPos);
    if (%pathSuccess == -1)
    {
      // No path to objective
      echo("No squad path to " @ %goalPos);
      %plr.targetPathDestination = 0;
      return;
    }

    if (%plr.isStopped() == true)
    {
      %plr.targetPathDestination = 0;
    }

    if (%plr.memberIndex == 0)
    {
      // Aim a few nodes ahead in our path
      %plr.stopAimPattern();

      // The squad leader should have the squad stop periodically so that
      //  everyone stays together
      if (!%this.squadStopPosition)
        %this.squadStopPosition = %plr.getPosition();
      %plr.squad.pendingObjective = false;
      if (VectorDist(%plr.getPosition(),%this.squadStopPosition) > $Brain1::SquadHoldDist && !%plr.squad.isMovedUp())
      {
        if (%plr.squadWait != false)
        {
          if (!%plr.squad.isSquadRegrouped(1))
          {
            // Time to regroup
            %plr.stop();
            %this.squadStopPosition = %plr.getPosition();
            echo("SQUAD LEADER CHANGING ORDER TO REGROUP");
            %plr.squad.setOrders("Regroup");
            %this.state = "REGROUP";
            %plr.squad.onOrdersChanged();
            %plr.squad.pendingObjective = true;
            return;
          }
          else
          {
            %this.squadStopPosition = %plr.getPosition();
          }
        }
      }

      // This part here gets tricky.
      //   If we're the squad leader, we analyze the path ahead of us to see where it bends into an area we can't see.
      // These are called bend points. When we find a bend point, the squad leader should set the squad to return fire only, and send
      // them into the area to check for threats.

      // The first thing we do is check the path and get the node after the one we're moving to, and check if we can't see it
      %initNodeIdx = %plr.curNode - 1; // The node we are moving away from
      %curNodeIdx = %plr.curNode; // Node we are moving to
      %sightNodeIdx = %plr.curNode + 1; // The node after it
      %initNode = %plr.pathNodeArray.getKey(%initNodeIdx);
      %curNode = %plr.pathNodeArray.getKey(%curNodeIdx);
      %sightNode = %plr.pathNodeArray.getKey(%sightNodeIdx);
      %lastMoveUpDist = VectorDist(%sightNode.position,%plr.squad.orderedMovePosition);
      %distToSightNode = VectorDist(%plr.getPosition(),%sightNode.position);
      %sightRay = ContainerRayCast(%initNode.position,%sightNode.position,$Brain1::SolidMasks);
      if (isObject(%sightNode) && isObject(%initNode) && %this.alertness > 0 && !%plr.squad.isMovedUp() && %sightRay && isObject(%sightNode) && isObject(%initNode) && %lastMoveUpDist >= $Brain1::MoveUpAgainDist && %distToSightNode >= $Brain1::MoveUpDist)
      {
        // Now we check visibility. If we are moving from a low visible node to a higher visiblity node, %curNode is a bend point
        %initNodeVis = %initNode.crouchVisScore;
        %sightNodeVis = %sightNode.crouchVisScore;
        %percentage = %sightNodeVis / %initNodeVis * 100;

        // We are moving into a very visible area, so %curNode is a bend point
        echo(%plr @ ": found a bend point, node is " @ %curNode @ ", sightNode is " @ %sightNode @ ", ray is " @ %sightRay);
        if (VectorDist(%sightNode.getPosition(),%plr.squad.member[0].getPosition()) < 15)
        {
          // Send the squad to %sightNode
          %plr.stop();
          %plr.squad.setMovePositions(%sightNode.getPosition(),6);
          %this.squadStopPosition = %plr.getPosition();
          echo("SQUAD LEADER CHANGING ORDER TO MOVE");
          %plr.squad.setOrders("MOVE");
          %this.state = "MOVE";
          %plr.squad.onOrdersChanged();
          %plr.squad.pendingObjective = true;
        }
      }
    }
    else // else, not the squad leader
    {
      // If the squad leader is not doing squad-like things, then we should wait
      if (%plr.squad.member[0].brain.state !$= "OBJECTIVE" && %plr.squad.member[0].brain.state !$= "MOVE"&& %plr.squad.member[0].brain.state !$= "REGROUP")
      {
        warn(%plr @ ": waiting for squad leader to be ready");
        %plr.stop();
        return;
      }

      // Start an aim pattern so that we search the area while moving with the squad
      if (%plr.aimPatternType == -1)
        %plr.startAimPattern(1,getRandom(0,20));

      // Make sure that we are behind the squad leader. Otherwise, wait for him to get ahead of us.
      %vecToTL = VectorNormalize(VectorSub(%plr.squad.member[0].getPosition(),%plr.getPosition()));
      %TLForwardVector = VectorNormalize(%plr.squad.member[0].getForwardVector());
      %distToTL = VectorDist(%plr.getPosition(),%plr.squad.member[0].getPosition());
      %dot = VectorDot(%vecToTL,%TLForwardVector);
      if (%dot < 0 || (%distToTL < $Brain1::SquadFollowDist * 0.5 * %plr.memberIndex && %plr.isStopped()))
      {
        %plr.stop();
        %plr.setPlayerPosition(2);
      }
      else
      {
        %plr.setPlayerPosition(1);
      }
    }
  }

  //-----------------------------------------------------------------
  // Decide if we should change states
  //-----------------------------------------------------------------
  if (%plr.squad)
  {
    if (%plr.memberIndex $= "0") // We are the team leader, decide if we should move for the objective
    {
      // Let's see if there's anything we should do
      if (%plr.squad.isSquadRegrouped(1) == 0 && %plr.squad.objectiveType $= "NONE" && %plr.squad.objectiveData $= "" && %plr.squad.moveLocation $= "")
      {
        echo("SQUAD LEADER CHANGING ORDER TO REGROUP");
        %plr.squad.onOrdersChanged();
        %plr.squad.setOrders("Regroup");
        %this.state = "REGROUP";

        %this.coverMarker.isTaken=0;
        %this.coverMarker = 0;
        %this.coverPos = 0;
          return;
      }
      if (%plr.squad.isSquadRegrouped(1) == 1 && %plr.squad.objectiveType $= "NONE" && %plr.squad.objectiveData $= "" && %plr.squad.moveLocation !$= "")
      {
        echo("SQUAD LEADER CHANGING ORDER TO MOVE");
        %plr.squad.onOrdersChanged();
        %plr.squad.setOrders("Move");
        %this.state = "MOVE";

        //%this.coverMarker.isTaken=0;
        //%this.coverMarker = 0;
        //%this.coverPos = 0;
          return;
      }
    }
    if (%plr.memberIndex != 0) // We are not the team leader
    {
      if (%this.currentOrder $= "Regroup")
      {
        echo("RECIEVED ORDER TO CHANGE TO REGROUP");
        %this.state = "REGROUP";

        %this.coverMarker.isTaken=0;
        %this.coverMarker = 0;
        %this.coverPos = 0;
          return;
      }
      else if (%this.currentOrder $= "Move")
      {
        echo("RECIEVED ORDER TO CHANGE TO STATEMOVE");
        %this.state = "MOVE";
          return;
      }
      else
      {

      }
    }
  }
}

// Move to a position ordered by the team leader
function Brain1::stateMove(%this)
{
  %this.state = "MOVE";
  %plr = %this.player;

  %plr.visMod = 1.15;
  //-----------------------------------------------------------------
  // Mimic the squad leader's stance (if he's human)
  //-----------------------------------------------------------------
  if (%plr.memberIndex != 0 && %plr.squad.member[0].client && %plr.getCoverPosition() == 5 && %plr.squad.roe == 1)
  {
    %plr.setPlayerPosition(%plr.squad.member[0].getPlayerPosition());
  }
  else
  {
    if (%plr.squad.roe == 0)
      %plr.setPlayerPosition(1);
  }

  //-----------------------------------------------------------------
  // Reload if necessary
  //-----------------------------------------------------------------
  if( %this.shouldReload())
  {
    %this.reloadWeapon();
  }

  //-----------------------------------------------------------------
  // Evade grenades
  //-----------------------------------------------------------------
  if (isObject(%this.evadeObj))
  {
    %this.lastEvadeState = %this.state;
    if (VectorDist(%plr.targetPathDestination, %plr.getPosition()) > 2)
      %this.lastDestination = %plr.targetPathDestination;
    else
      %this.lastDestination = %plr.getPosition();

    %this.state = "GRENADE";
    return;
  }

  //-----------------------------------------------------------------
  // Attack Enemies
  //-----------------------------------------------------------------
  if (isObject(%this.attackObj) && %this.attackObj.getState() !$= "Dead")
  {
    %plr.setMoveSpeed(1.0);
    %shouldAttack = %this.shouldReturnFire(%this.attackObj);
    if (%shouldAttack == 1)
    {
      %plr.setCoverPosition(5);
      if (%this.shouldTakeCover(%this.attackObj))
      {
        %this.coverObj = %this.attackObj;
        %this.coverMarker.isTaken=0;
        %this.coverMarker = 0;
        %this.coverPos = 0;

        if (%this.coverObjIsVisible)
        {
          %this.suppressPos = %this.coverObj.getWorldBoxCenter();
          %this.lastKnownEnemyPos = %this.coverObj.getPosition();
        }
        %this.objectiveMovePosition = 0;
        %this.objectiveCoverPos = 0;
       // %plr.stop();
        %coverMarker.isTaken = 0;
        %this.state = "FINDCOVER";
          return;
      }
      else
      {
        if (%this.attackObjIsVisible)
        {
          %plr.setCoverPosition(5);
          %plr.stop();
          %plr.targetPathDestination = 0;
          %plr.suppressingFire(%this.attackObj.getTrueEyePoint(),1,%this.attackObj);
          warn(%plr @ ": opening fire on an easy target");
            return;
        }
      }
    }
    else
    {
      if (%this.attackObjIsVisible)
      {
        %plr.setCoverPosition(5);
        %plr.setAimLocation(%this.attackObj.getTrueEyePoint());
        if (%shouldAttack == 2) // Which means that a stealth kill is appropriate
          %plr.fire(%this.attackObj.getTrueEyePoint(),0,%this.attackObj);
        else
          warn(%plr @ ": holding fire...");
      }
      else
        %plr.clearAim();
    }
  }

  // See if we killed an enemy
  if (%this.attackObj && %this.attackObj.getState() $= "Dead")
  {
    %plr.clearAim();
    %plr.setMoveSpeed(1.0);
    %this.attackObj = 0;

    // Momentarily stop thinking to avoid endless loop crashes (they happen sometimes with the state system)
    %plr.setThinking(false);
    %plr.schedule(1000,setThinking,true);
      return;
  }

  //-----------------------------------------------------------------
  // React to immediate danger
  //-----------------------------------------------------------------
  if (%this.threatPosition && %this.threatType $= "Fire" && !%plr.squad.quitReact)
  {
    %this.state = "SQUADREACT";
    return;
  }

  //-----------------------------------------------------------------
  // Decide if we should change what we are doing
  //-----------------------------------------------------------------
  if (%plr.squad)
  {
    if (%plr.memberIndex == 0) // We are the team leader, decide if we should move for the objective
    {
      // If we came from stateObjective, see if the squad is in position. Then move for the objective again
      if (%plr.squad.pendingObjective)
      {
        %plr.stop();
        echo(%plr @ ": waiting for all-clear");

        if (%plr.squad.isMovedUp() == true && %plr.squad.pendingObjective)
        {
          %plr.squad.pendingObjective = 0;
          %plr.targetPathDestination = 0;
          %this.squadStopPosition = %plr.getPosition(); // We move from there
          %plr.squadWait = false;
          %plr.schedule(10000,setSquadWait,true);

          %plr.squad.setOrders("Objective");
          %plr.squad.onOrdersChanged();
          %this.state = "OBJECTIVE";
          return;
        }
      }

      if (!%plr.squad.pendingObjective && %plr.squad.isSquadRegrouped() == 0)
      {
        %plr.squad.setOrders("Regroup");
        if (!isObject(%this.attackObj))
          %plr.clearAim();
        %this.state = "REGROUP";  // march 13 2010

        %this.coverMarker.isTaken=0;
        %this.coverMarker = 0;
        %this.coverPos = 0;
          return;
      }
      if (!%plr.squad.pendingObjective && %plr.squad.objectiveType !$= "" && %plr.squad.objectiveType !$= "NONE" && %plr.squad.objectiveData !$= "") // We have an objective
      {
        %plr.squad.setOrders("Objective");
        if (!isObject(%this.attackObj))
          %plr.clearAim();
        %plr.squad.onOrdersChanged();
        %this.state = "OBJECTIVE";

        %this.coverMarker.isTaken=0;
        %this.coverMarker = 0;
        %this.coverPos = 0;
          return;
      }
      if (!%plr.squad.pendingObjective && %plr.squad.moveLocation $= "" && (%plr.squad.objectiveType $= "" || %plr.squad.objectiveType $= "NONE")) // Nothing to do
      {
        %plr.squad.setOrders("Regroup");
        if (!isObject(%this.attackObj))
          %plr.clearAim();
        %plr.squad.onOrdersChanged();
        %this.state = "Regroup";

        %this.coverMarker.isTaken=0;
        %this.coverMarker = 0;
        %this.coverPos = 0;
          return;
      }
    }
    else // We are not the team leader, so we check for order changes
    {
      if (%this.currentOrder $= "Objective")
      {
        //%plr.clearAim();
        %plr.setPlayerPosition(1);
        %this.coverMarker.isTaken = 0;
        %this.coverMarker = 0;

        %this.orderMovePosition = 0;
        %plr.squad.ordermoveposition = 0;
        %this.state = "OBJECTIVE";
          return;
      }

      if (%this.currentOrder $= "Regroup")
      {
        //%plr.clearAim();
        %plr.setPlayerPosition(1);
        %this.coverMarker.isTaken = 0;
        %this.coverMarker = 0;

        %this.orderMovePosition = 0;
        %plr.squad.ordermoveposition = 0;
        %this.state = "REGROUP";
          return;
      }
      if (!%plr.squad.pendingObjective && %this.currentOrder $= "Hold")
      {
        //%plr.clearAim();
        //%plr.setPlayerPosition(1);
        %this.coverMarker.isTaken = 0;
        %this.coverMarker = 0;
        %this.orderMovePosition = 0;
        %plr.squad.ordermoveposition = 0;
        %this.state = "HOLD";
          return;
      }
    }
  }

  //-----------------------------------------------------------------
  // If we are set to move to cover, use that cover
  //-----------------------------------------------------------------
  if (%this.coverMarker != 0)
  {
    if (VectorDist(%plr.getWorldBoxCenter(),%this.coverPos) < 1.5)
    {
      %aimPos = VectorAdd(VectorScale(%this.coverMarker.getForwardVector(),100),%plr.getTrueMuzzlePoint(0));
      %plr.setAimLocation(%aimPos);

      if (%plr.squad.roe == 1) // If return fire only, stay hidden behind cover
      {
        if (%this.shootOverCover == true)
        {
          %plr.setCoverPosition(3);
          %plr.stop();
          if (AIPlayer::getXYDistance(%plr.getPosition(),%this.coverPos) > 0.1)
            %plr.setTransform(AIPlayer::getGroundPos(%this.coverPos));
          %plr.setVelocity("0 0 0");
        }
        else
        {
          //%plr.setMoveDestination(%this.coverPos);
          %plr.stop();
          if (AIPlayer::getXYDistance(%plr.getPosition(),%this.coverPos) > 0.1)
            %plr.setTransform(AIPlayer::getGroundPos(%this.coverPos));
          %plr.setVelocity("0 0 0");
          %plr.setCoverPosition(0);
        }
        return;
      }

      // We can fire at will, so ocassionaly peek out of cover
      //  However, if we have an attackObj, peek out and kill him
      if (%this.shootOverCover == true)
      {
        if (!isObject(%this.attackObj))
        {
          if (%this.mediumChanceRandom())
          {
            %plr.stop();

            // toggle the cover position
            if (%plr.getCoverPosition() == 4)
            {
              %plr.setCoverPosition(3);
            }
            else if (%plr.getCoverPosition() == 3)
              %plr.setCoverPosition(4);
            else
              %plr.setCoverPosition(3);

            if (AIPlayer::getXYDistance(%plr.getPosition(),%this.coverPos) > 0.1)
              %plr.setTransform(AIPlayer::getGroundPos(%this.coverPos));
            %plr.setVelocity("0 0 0");
          }
          else
          {
            if (%plr.getCoverPosition() == 5)
            {
              %plr.setCoverPosition(3);
            }
          }
        }
        else
        {
          // We have an attackObj, so peek out to shoot him
          %plr.setPlayerPosition(1);
          %plr.setCoverPosition(4);
        }
      }
      else
      {
        if (!isObject(%this.attackObj))
        {
          if (%this.smallChanceRandom())
          {
            if (%plr.getCoverPosition() == 0)
            {
              if (%this.returnFirePos && %this.coverMarker)
              {
                %plr.setMoveDestination(%this.returnFirePos);
              }

              if (%this.shootRight == true)
              {
                %plr.setCoverPosition(1);
              }
              if (%this.shootRight == false)
              {
                %plr.setCoverPosition(2);
              }
            }
            else
            {
              %plr.stop();
              if (AIPlayer::getXYDistance(%plr.getPosition(),%this.coverPos) > 0.1)
                %plr.setTransform(AIPlayer::getGroundPos(%this.coverPos));
              %plr.setVelocity("0 0 0");
              %plr.setCoverPosition(0);
            }
          }
          else
          {
            if (%plr.getCoverPosition() == 5)
            {
              %plr.stop();
              %plr.setCoverPosition(0);
              if (AIPlayer::getXYDistance(%plr.getPosition(),%this.coverPos) > 0.1)
                %plr.setTransform(AIPlayer::getGroundPos(%this.coverPos));
              %plr.setVelocity("0 0 0");
            }
          }
        }
        else
        {
          // We have an attackObj, peek out of cover to shoot him
          if (%this.returnFirePos && %this.coverMarker)
            %plr.setMoveDestination(%this.returnFirePos);

          if (%this.shootRight == true)
            %plr.setCoverPosition(1);
          if (%this.shootRight == false)
            %plr.setCoverPosition(2);
        }
      }
    }
    else
    {
      %plr.setCoverPosition(5);
      if (!isObject(%this.attackObj))
        %plr.clearAim();
    }
  }
  else
  {
    %plr.setCoverPosition(5);
    if (!isObject(%this.attackObj))
      %plr.clearAim();
  }

  //-----------------------------------------------------------------
  // Decide how fast to move
  //-----------------------------------------------------------------
  // Set the move speed based on rules of engagement
  if (%plr.squad.roe == 0) // We fire at will, move quickly
  {
    %plr.setMoveSpeed(0.7);
  }
  else // If we're trying to be stealthy, slow down
  {
    if (%plr.getPlayerPosition() == 1)
      %plr.setMoveSpeed(0.4);
    if (%plr.getPlayerPosition() == 2)
      %plr.setMoveSpeed(0.5);
    if (%plr.getPlayerPosition() == 3)
      %plr.setMoveSpeed(1.0);
  }

  //-----------------------------------------------------------------
  // Decide how to move
  //-----------------------------------------------------------------
  %this.lastLeaderPosition = 0; // So we're ready to regroup again later
  if (!%this.lastMovePosition || VectorDist(%this.movePosition,%this.lastMovePosition) >= $Brain1::ObjectiveMoveTolerance)
  {
    %this.lastMovePosition = %this.movePosition;
    %plr.SetOnPath(%this.movePosition,%plr.squad.roe);
  }
}

// Wait for orders
function Brain1::stateHold(%this)
{
  %this.state = "HOLD";
  %plr = %this.player;

  %plr.visMod = 1.15;
  %plr.setCoverPosition(5);

  //-----------------------------------------------------------------
  // Mimic the squad leader's stance (if he's human)
  //-----------------------------------------------------------------
  if (%plr.memberIndex != 0 && %plr.squad.member[0].client && %plr.getCoverPosition() == 5 && %plr.squad.roe == 1)
  {
    // If the squad leader looks at us, match his stance
    %ldr = %plr.squad.member[0];
    %vecLdrToMe = VectorNormalize(VectorSub(%plr.getPosition(),%ldr.getPosition()));
    %vecLdrLook = VectorNormalize(%ldr.getEyeVector());
    %dot = VectorDot(%vecLdrToMe,%vecLdrLook);
    if (%dot > 0.8)
      %plr.setPlayerPosition(%ldr.getPlayerPosition());
  }

  //-----------------------------------------------------------------
  // Reload empty weapons
  //-----------------------------------------------------------------
  if (%this.shouldReload())
  {
    %this.reloadWeapon();
  }

  //-----------------------------------------------------------------
  // Evade grenades
  //-----------------------------------------------------------------
  if (isObject(%this.evadeObj))
  {
    %this.lastEvadeState = %this.state;
    if (VectorDist(%plr.targetPathDestination, %plr.getPosition()) > 2)
      %this.lastDestination = %plr.targetPathDestination;
    else
      %this.lastDestination = %plr.getPosition();

    %this.state = "GRENADE";
    return;
  }

  //-----------------------------------------------------------------
  // Attack Enemies
  //-----------------------------------------------------------------
  if (isObject(%this.attackObj) && %this.attackObj.getState() !$= "Dead")
  {
    %plr.setMoveSpeed(1.0);
    %shouldAttack = %this.shouldReturnFire(%this.attackObj);
    if (%shouldAttack == 1)
    {
      if (%this.shouldTakeCover(%this.attackObj))
      {
        %this.coverObj = %this.attackObj;
        %this.coverMarker.isTaken=0;
        %this.coverMarker = 0;
        %this.coverPos = 0;

        if (%this.coverObjIsVisible)
        {
          %this.suppressPos = %this.coverObj.getWorldBoxCenter();
          %this.lastKnownEnemyPos = %this.coverObj.getPosition();
        }
        %this.objectiveMovePosition = 0;
        %this.objectiveCoverPos = 0;
       // %plr.stop();
        %coverMarker.isTaken = 0;
        %this.state = "FINDCOVER";
          return;
      }
      else
      {
        if (%this.attackObjIsVisible)
        {
          %plr.stop();
          %plr.targetPathDestination = 0;
          warn(%plr @ ": sniping " @ %this.attackObj);

          //%plr.setAimObject(%this.attackObj,"0 0 0.5");
          // Take the shot
          //%plr.pullTrigger();
          %plr.fire(%this.attackObj.getWorldBoxCenter(),0,%this.attackObj);
            return;
        }
      }
    }
    else
    {
      if (%this.attackObjIsVisible)
      {
        %plr.setAimLocation(%this.attackObj.getTrueEyePoint());
        if (%shouldAttack == 2) // Which means that a stealth kill is appropriate
          %plr.fire(%this.attackObj.getTrueEyePoint(),0,%this.attackObj);
        else
          warn(%plr @ ": holding fire...");
      }
      else
        %plr.clearAim();
    }
  }

  // See if we killed an enemy
  if (%this.attackObj && %this.attackObj.getState() $= "Dead")
  {
    %plr.clearAim();
    %this.attackObj = 0;

    // Momentarily stop thinking to avoid endless loop crashes (they happen sometimes with the state system)
    %plr.setThinking(false);
    %plr.schedule(1000,setThinking,true);
      return;
  }

  //-----------------------------------------------------------------
  // React to immediate danger
  //-----------------------------------------------------------------
  if (%this.threatPosition && %this.threatType $= "Fire" && !%plr.squad.quitReact)
  {
    %this.state = "SQUADREACT";
    return;
  }

  //if (%plr.squad != 0 && %plr.squad.member[0].getState() !$= "Dead")
  //{
    //%squad = %plr.squad;
  //}
  //else
  //{
    //%this.state = "NONE";
      //return;
  //}


  //-----------------------------------------------------------------
  // Hold our position
  //-----------------------------------------------------------------
  %plr.stop();

  //-----------------------------------------------------------------
  // Check to see if we should change states
  //-----------------------------------------------------------------
  if (%plr.squad)
  {
    if (%plr.memberIndex $= "0") // We are the team leader, decide if we should move for the objective
    {
      if (%plr.squad.isSquadRegrouped() == 0)
      {
        %plr.squad.setOrders("Regroup");
        %this.state = "REGROUP"; // march 13 2010
          return;
      }
      // Let's see if there's anything we should do
      if (%plr.squad.isSquadRegrouped() == 1 && %plr.squad.objectiveType !$= "" && %plr.squad.objectiveType !$= "NONE" && %plr.squad.objectiveData !$= "") // We have an objective
      {
        echo("SQUAD LEADER CHANGING ORDER TO OBJECTIVE");
        %plr.squad.setOrders("Objective");
        %plr.squad.onOrdersChanged();
        %this.state = "OBJECTIVE";

        %this.coverMarker.isTaken=0;
        %this.coverMarker = 0;
        %this.coverPos = 0;
          return;
      }
      if (%plr.squad.moveLocation $= "" && (%plr.squad.objectiveType $= "" || %plr.squad.objectiveType $= "NONE")) // Nothing to do
      {
        echo("SQUAD LEADER CHANGING ORDER TO REGROUP");
        %plr.squad.setOrders("Regroup");
        %plr.squad.onOrdersChanged();
        %this.state = "Regroup";

        %this.coverMarker.isTaken=0;
        %this.coverMarker = 0;
        %this.coverPos = 0;
          return;
      }
      if (%plr.squad.currentOrder $= "Move")
      {
        echo("SQUAD LEADER CHANGING ORDER TO STATEMOVE");
        %plr.squad.setOrders("Move");
        %plr.squad.onOrdersChanged();
        %this.state = "MOVE";

        %this.coverMarker.isTaken=0;
        %this.coverMarker = 0;
        %this.coverPos = 0;
          return;
      }
    }

    if (%plr.memberIndex != 0) // We are not the team leader
    {
      if (%this.currentOrder $= "Regroup")  // was else if
      {
        echo("RECIEVED ORDER TO CHANGE TO STATEREGROUP");
        %plr.stop();
        %this.orderMovePosition = 0;
        %plr.squad.ordermoveposition = 0;
        %this.state = "REGROUP";

        %this.coverMarker.isTaken=0;
        %this.coverMarker = 0;
        %this.coverPos = 0;
          return;
      }
      else if (%this.currentOrder $= "Move")
      {
        echo("RECIEVED ORDER TO CHANGE TO STATEMOVE");
        %plr.stop();
        %this.orderMovePosition = 0;
        %plr.squad.ordermoveposition = 0;
        %this.state = "MOVE";
          return;
      }
      else
      {

      }
    }
  }
}

//******************************************************************************
//----------------------------------------
// State code ends here
//----------------------------------------
//******************************************************************************

//----------------------------------------
// Detection functions
//----------------------------------------

// This function gathers all players around the AI player
function Brain1::getVisObjects(%this)
{
  %plr = %this.player;
  %pos = %plr.getTrueEyePoint();

  %this.attackObjIsVisible = false;
  %playersProcessed = 0;
  %teamMatesProcessed = 0; // Keep track of the number of teammates we process
  %this.objectsProcessed = 0;

  // Find any player in range
  //for(%objIdx = 0; %objIdx < PlayerArray.count(); %objIdx++)

  InitContainerRadiusSearch(%plr.getTrueEyePoint(),%plr.maxViewDist,$TypeMasks::PlayerObjectType | $TypeMasks::CorpseObjectType);
  while((%obj = ContainerSearchNext()) != 0)
  {
    //%obj = PlayerArray.getKey(%objIdx);

    // Don't check ourselves
    if (%obj == %plr)
      continue;

    // Don't process sleeping NPCs
    if (%obj.isAsleep == true)
      continue;

    // Make sure we do not process too many things
    if (%playersProcessed > $Brain1::MaxProcessedObjects)
    {
      return;
    }

    if (%obj.isCivilian && %obj.getState() !$= "Dead" && %obj != %this.attackObj)
    {
      if ($Brain1::KillCivilians == false && !%hasInfo)
        continue; // If we're not supposed to target civilians, don't process them
      else
      {
        // Anything?
      }
    }

    if (%obj.team == %plr.team && %obj.getState() !$= "Dead")
    {
      // Decide if we should process this teammate
      %shouldProcessTeammate = false;
      %limitReached = (%teamMatesProcessed > $Brain1::MaxProcessedTeammates);
      %isAIPlr = (%obj.brain != 0); // If the teammate is an ai player
      %isFiring = (%obj.isImageFiring(0)); // If the object is firing, so we know if we should process a human player
      %hasAttackObj = (%obj.brain.attackObj != 0); // If the teammate is attacking someone and we don't have anyone to attack

      // If he has a target, he is an AI player, we don't have a target, and we can see him, we can get enemy information
      //   from this teammate.
      if (%hasAttackObj && %isAIPlr && !isObject(%this.attackObj) && %this.isClearTo(%obj.getWorldBoxCenter()))
      {
        %shouldProcessTeammate = true;
      }
      // If he is a human player, he is firing, and we don't have a target, we can assist him
      if (%isFiring && !isObject(%this.attackObj) && !%isAIPlr)
      {
        %shouldProcessTeammate = true;
      }
      // Make sure that we have not processed too many other teammates.
      if (%limitReached)
      {
        %shouldProcessTeammate = false;
      }

      // Now, we either skip over this teammate or we don't
      if (%shouldProcessTeammate != true)
      {
        continue; // Don't process this teammate
      }
      else
      {
        %teamMatesProcessed++;
      }
    }

    // If he is hostile, let's decide if we should process him
    if ((%obj.team != %plr.team && %obj.team != -1) || (%obj.isCivilian && $Brain1::KillCivilians == true))
    {
      %shouldProcessHostile = false;

      // See if we may want to target this enemy
      if (isObject(%this.attackObj))
      {
        %distToAttackObj = VectorDist(%plr.getPosition(),%this.attackObj.getPosition());
        %distToCurObj = VectorDist(%plr.getPosition(),%obj.getPosition());
        // If the hostile in question is closer to us than our attackObj, or we cannot see our
        //   attackObj, process the hostile in case he comes into view and we need to target him
        if (%distToCurObj < %distToAttackObj || ContainerRayCast(%plr.getTrueEyePoint(),%this.attackObj.getWorldBoxCenter(),$AIPlayer::SolidMasks) != 0)
        {
          %shouldProcessHostile = true;
        }
      }
      else
      {
        %shouldProcessHostile = true; // No attackObj, so we should process this hostile
      }
      // Don't process dead enemies
      if (%obj.getState() $= "Dead")
      {
        %shouldProcessHostile = false;
      }
      // Process civvies
      if ($Brain1::KillCivilians == true && %obj.isCivilian)
      {
        %shouldProcessHostile = true;
      }
      // If we are currently attacking this hostile, yes, we should process him
      if (%obj == %this.attackObj)
      {
        %shouldProcessHostile = true;
      }

      // Decide if we should process this object or not
      if (%shouldProcessHostile != true)
      {
        continue;
      }
    }

    %vis = %this.isObjVisible(%obj);
    %visWeight = getWord(%vis,1);
    if (getWord(%vis,0) == true)
    {
      %playersProcessed++;
      %this.objectsProcessed++;
      // We see it, is it hostile and alive?
      if ((%obj.team != %plr.team && %obj.team != -1) || %obj == %this.attackObj)
      {
        if (%obj.getState() !$= "Dead")
        {
          // Are we really sure this is a threat? In other words, what was the visiblity weight of this
          //   object that we calculate in isObjVisible()? If this weight is low, we're not entirely sure
          //   that this is a threat, and we should investigate before opening fire.
          if (%visWeight <= $Brain1::UnsureVisWeight && %this.alertness < 2 && !%plr.squad)
          {
            echo(%plr @ ": unsure of " @ %obj @ ", going to investigate");
            %this.threatType = "PLAYER";
            %this.threatPosition = %obj.getPosition();
            %this.state = "HUNT";
            return;
          }

          // If we already have an attackObj, check to see that it is not our highest priority
          //  By that I mean that close enemies should be killed before attacking anybody else
          if ((!isObject(%this.attackObj) || %this.attackObj.getState() $= "Dead") || VectorDist(%plr.getPosition(),%this.attackObj.getPosition()) > $Brain1::CloseObjDist)
          {
            %this.attackObj = %obj; // Time to pwn that n00b
            %this.lastKnownEnemyPos = %obj.getPosition();
            %this.assumedEnemyPosition = %obj.getPosition();
          }

          if (!isObject(%plr.enemyArray))
          {
            %plr.enemyArray = new ArrayObject();
            MissionCleanup.add(%plr.enemyArray);
          }

          %enemyArray = %plr.enemyArray;
          %isInArray = %enemyArray.countKey(%obj);
          if (%isInArray == 0)
          {
            %enemyArray.add(%obj,0);
          }
          %this.attackObjIsVisible = true;
          return;
        }
      }
      else // It's friendly
      {
        if (%obj.getState() !$= "Dead" && isObject(%obj.brain.coverObj) && !isObject(%this.coverObj) && %this.isObjVisible(%obj.brain.attackObj)) // A friend is trying to kill someone
        {
          %this.attackObj = %obj.brain.attackObj;
          %this.coverObj = %obj.brain.coverObj; // so, why not give him a hand?
          if (%obj.brain.suppressPos)
          {
            %this.suppressPos = %obj.brain.suppressPos;
          }
          return;
        }
        if (%obj.getState() $= "Dead" && %this.threatType !$= "Fire") // React if we see a dead teammate, but not if we've come under fire
        {
          if (%visWeight <= $Brain1::UnsureVisWeight && %this.alertness < 2 && !%plr.squad)
          {
            echo(%visWeight);
            echo(%plr @ ": unsure of " @ %obj @ ", going to investigate");
            %this.threatType = "PLAYER";
            %this.threatPosition = %obj.getPosition();
            %this.state = "HUNT";
            return;
          }
          %this.setAlertNess(2); // Dead body. Hmmm. That ain't right...
          %this.threatPosition = %obj.getPosition();
          %this.threatType = "Body";
        }
        if (%obj.getState() !$= "Dead" && %this.alertness == 0 && %obj.brain.alertness > 0) // If %obj is alert, we should be, too!
        {
          %this.setAlertness(%obj.brain.alertness);
          echo(%plr @ ": sees a friend who is alert, I should be on the lookout!");
        }
        if (%obj.getState() !$= "Dead" && !%plr.squad && %obj.team == %plr.team && !%obj.brain.assistObj && !%this.assistObj && %obj.isImageFiring(0) == true)
        {
          // hack...assist behavior needs some work.
          %this.assistObj = %obj;
          if (%this.assistObj.brain.attackObj && !%this.attackObj)
          {
            %this.attackObj = %this.assistObj.brain.attackObj;
            return;
          }
          echo(%plr @ ": Setting object [" @ %obj @ "] to be my assistObj.");
        }
        //if (%obj.getState() !$= "Dead" && !%this.assistObj && %obj.brain.assistObj)
        //{
          //%this.assistObj = %obj.brain.assistObj;
        //}
        if (%obj.getState() !$= "Dead" && !%this.invPosition && !%plr.squad && %obj.brain.threatType !$= "" && %obj.brain.threatType !$= "Player" && %obj.brain.threatType !$= "Annoyance" && %obj.brain.threatOrigin) // React if a teammate is reacting
        {
          %this.threatType = %obj.brain.threatType;
          %this.threatOrigin = %obj.brain.threatOrigin;
          %this.threatPosition = %obj.brain.threatPosition;
        }
        // No threat data if we are hunting
        if (%this.invPosition)
        {
          %this.threatType = "";
          %this.threatPosition = 0;
        }
      }
    }
  }
}

// Checks to see if %obj is visible to our player
function Brain1::isObjVisible(%this,%obj)
{
   %plr = %this.player;
   %pos = %plr.getTrueEyePoint();

   if (!isObject(%obj))
     return 0;

   // A quick hack to prevent us from losing a target who is very close to us
   if (%obj == %this.attackObj && VectorDist(%obj.getPosition(),%plr.getPosition()) < $Brain1::VeryCloseVisObjDist)
   {
      return true;
   }

   %dist = VectorDist(%pos,%obj.getTrueEyePoint());
   if
   (
     %dist <= %plr.minViewDist      // If obj is close enough
       ||
     (
       %dist > %plr.minViewDist &&
       %dist < %plr.maxViewDist &&
       VectorLen(%obj.getVelocity()) >= "5"                              // or if obj is moving
         ||
       %dist > %plr.minViewDist &&
       %dist < %plr.maxViewDist &&
       %obj.getMountedImage(0).flashSuppressor != true &&
       %obj.isImageFiring(0)                                             // or if obj is firing a weapon with a visible muzzle flash
        ||
       %obj.getState() $= "Dead"                                          // or if obj is dead. This section needs some serious reworking, what the heck was I thinking?
     )
   )                                                                        // Then obj is eligible for being seen
   {
      %angle = %plr.getAngleTo(%obj.getWorldBoxCenter()); // Gets the angle to the object in question
      if (%angle > %plr.fov)
         return; // If this object is outside our field of view, don't try to process it

     // If this object is near our last known enemy position, and we are attacking him, he will be seen to avoid dumb behavior
     %lastKnownWeight = 1.0;
     if (VectorDist(%this.lastKnownEnemyPos,%obj.getPosition()) <= $Brain1::LastKnownEnemyVisDist && %obj == %this.attackObj && (!%rayFoot || !%rayHead))
        %lastKnownWeight = $Brain1::LastKnownWeight;

     // This is a bit specific, but trust me on this one.
     // If we're unsure of something we just saw, we're going to move toward it.
     // To prevent the AI from being stuck in this state, an object that is still hanging
     //   around that position will be more visible when the AI is close enough to it.
     %unsureVeryVisWeight = 1.0;
     if (%this.threatType $= "Player" && %this.state $= "HUNT" && VectorDist(%plr.getPosition(),%this.threatPosition) < $Brain1::UnsureVeryVisDist)
     {
       if (VectorDist(%obj.getPosition(),%this.threatPosition) < 2) // Object is at the position we're unsure of
       {
         %unsureVeryVisWeight = $Brain1::UnsureVeryVisWeight;
         warn(%plr @ ": hidden object " @ %obj @ " ain't so hidden anymore.");
       }
     }

     // If the enemy is close enough, raise the losProb to 1
     if (VectorDist(%plr.getPosition(),%obj.getPosition()) <= $Brain1::CloseObjDist && %losProb > 0)
       %losProb = 1;

     // Placeholder for now...
     //if (!%rayHead || !%rayChest)
       //%losProb = 1;

     // Let's see if %obj is in our field of view
     // pView needs to be a value from 1 to 0. 1 being in the center of vision, 0 being not visible at all
     %angle = %plr.getAngleTo(%obj.getWorldBoxCenter());
     if (%angle < %plr.fov)
     {
       %pViewMin = 0;
       %pViewMax = 1.0;
       %maxFov = %plr.fov;
       %pView = (%pViewMin-%pViewMax)/(%maxFov) * %angle + %pViewMax;
     }
     else
       %pView = 0;

     // Movement = visiblity
     if (VectorLen(%obj.getVelocity()) >= 5)
       %mvWeight = 3; // hack
     else
       %mvWeight = 1;

     // Players who fire weapons with muzzle flashes should be more visible
     if (%obj.getMountedImage(0) && %obj.isImageFiring(0) == true)
     {
       if (%obj.getMountedImage(0).flashSuppressor == true)
          %firingWeight = 1; // No visible muzzle flash, so no increased chance of seeing %obj
       else
          %firingWeight = $Brain1::FiringVisWeight;
     }
     else
     {
       %firingWeight = 1;
     }

     // Dead players are more visible (somebody flat on the ground should catch your eye)
     if (%obj.getState() $= "Dead")
       %deadWeight = $Brain1::BodyVisWeight;
     else
       %deadWeight = 1.5; // hack

     // Check other visiblity factors and adjust the weight
     %factorWeight = %this.getVisFactorWeight(%obj);

     %rand = getRandom();
     %prob = %pView * %plr.visMod * %firingWeight * %deadWeight * %mvWeight * %factorWeight * %lastKnownWeight * %unsureVeryVisWeight;

     if (%rand < %prob)
     {
       %losProb = 0;
       %rayFoot = ContainerRayCast(%pos,%obj.getPosition(),$Brain1::OpaqueMasks,%plr);
       %rayHead = ContainerRayCast(%pos,%obj.getTrueEyePoint(),$Brain1::OpaqueMasks,%plr);
       if (!%rayFoot) %losProb += 0.5;
       if (!%rayHead && %obj.getState() !$= "Dead") %losProb += 0.5;

       %prob *= %losProb;
       if (%rand < %prob)
       {
         if ($stealthDebug == true)
         {
           echo(%obj @ " has been spotted by " @ %plr @ ", p: " @ %prob);
           return;
         }
         else
         {
           return true SPC %prob; // We can see %obj. Also return the weight, so we know how sure we were that we saw %obj
         }
       }
     }
   }
   return false; // If we detect the object, we would have returned true by now. We don't, so return false.
}

// Gets the visibility weight of %obj based on factors such as stance and fog
function Brain1::getVisFactorWeight(%this,%obj)
{
  %plr = %this.player;
  %weight = 1.0;

  // Get stance weight
  %stance = %obj.getPlayerPosition();
  %db = %obj.getDatablock();
  %sW = %db.standVisWeight;
  %cW = %db.crouchVisWeight;
  %pW = %db.proneVisWeight;
  switch$(%stance)
  {
    // Standing
    case 1:
      if (%sW $= "")
        %sW = 1.0;
      %weight *= %sW;
    // Crouching
    case 2:
      if (%cW $= "")
        %cW = 1.0;
      %weight *= %cW;
    // Prone
    case 3:
      if (%pW $= "")
        %pW = 1.0;
      %weight *= %pW;
  }

  // See if %obj is inside any foliage areas
  if (isObject(FoliageGroup))
  {
    for(%i = 0;%i < FoliageGroup.getCount();%i++)
    {
      %fol = FoliageGroup.getObject(%i);
      // Get the average coverage distance of this foliage
      %area = (%fol.outerRadiusX + %fol.outerRadiusY) / 2;
      // See if %obj is within the area
      %distToFol = VectorDist(%obj.getPosition(),%fol.getPosition());
      if (%distToFol <= %area)
      {
        // %obj is within the distance of this foliage, see if he is actually covered vertically by it
        %terrHeight = getTerrainHeight(%obj.getPosition());
        %folHeight = %fol.maxHeight;
        %objHeight = getWord(VectorAdd(%obj.getTrueEyePoint(),"0 0 0.5"),2);
        if ((%objHeight - %terrHeight) <= %folHeight || %obj.getState() $= "Dead")
        {
          // Adjust the weight based on the camoAmount
          if (%fol.camoAmount $= "")
            %weight *= 0.5; // If camoAmount is not specified on this foliage, default 0.5
          else
            %weight *= %fol.camoAmount;
        }
      }
    }
  }

  // Groups of trees should also be able to obstruct view to %obj
  //   Any object that belongs in TreeGroup that has position, outerRadiusX, outerRadiusY, maxHeight, and camoAmount
  // can obstruct view. For example, I create scriptobjects with these fields to represent the smoke from smoke grenades, and
  // I add them to TreeGroup to simulate the effect.
  if (isObject(TreeGroup))
  {
    for(%i = 0;%i < TreeGroup.getCount();%i++)
    {
      %tG = TreeGroup.getObject(%i);

      %start = %plr.getTrueEyePoint();
      %end = %obj.getTrueEyePoint();

      if (%tG.outerRadiusX < %tG.outerRadiusY)
        %r = %tG.outerRadiusX;
      else
        %r = %tG.outerRadiusY;
      %d = SetWord(VectorSub(%end,%start),2,0);
      %f = SetWord(VectorSub(%start,%tG.position),2,0);
      %a = VectorDot(%d,%d);
      %b = 2 * VectorDot(%f,%d);
      %c = VectorDot(%f,%f) - %r * %r;
      %discriminant = %b * %b - 4 * %a * %c;
      if (%discriminant < 0)
      {
        // No intersection
      }
      else
      {
        // Now, we make a rough estimate to see if it blocks our vision vertically
        %distToTG = VectorDist(%plr.getWorldBoxCenter(),%tG.position);
        %distToObj = VectorDist(%plr.getWorldBoxCenter(),%obj.getWorldBoxCenter());
        %distCo = %distToTG / %distToObj;
        %vecToObj = VectorSub(%obj.getWorldBoxCenter(),%plr.getWorldBoxCenter());
        %scVec = VectorScale(%vecToObj,%distCo);
        %scPos = VectorAdd(%scVec,%plr.getWorldBoxCenter());
        %scZ = getWord(%scPos,2);

        %discriminant = mSqrt(%discriminant);
        %t1 = (-%b + %discriminant) / (2 * %a);
        %t2 = (%b + %discriminant) / (2 * %a);

        if ((%t1 >= 0 || %t2 <= 0) && (%t1 <= 1 || %t2 >= -1))
        {
          if (%tG.maxHeight)
            %tGHeight = %tG.maxHeight;
          else
            %tGHeight = 10;
          %bottomZ = getWord(%tG.position,2);
          %topZ = %bottomZ + %tGHeight;
          if (%scZ > %bottomZ && %scZ < %topZ)
          {
            if (%tG.camoAmount !$= "")
              %weight *= %tG.camoAmount;
            else
              %weight *= 1.0;
          }
        }
      }
    }
  }

  // Adjust the weight based on fog
  if (isObject(Sky))
  {
    %maxFogDist = theLevelInfo.visibleDistance / 1.3;
    %minFogDist = 0;

    %plrDist = VectorDist(%plr.getPosition(),%obj.getPosition());
    if (%plrDist > %minFogDist)
    {
      %fogWeight = 1 - ((1.0 - 0.0) / (%maxFogDist) * %plrDist) * 0.2;
      %weight *= %fogWeight;
    }
  }

  // See how well we are camoflaged based on our average color (if specified) and the average color of the mission (if specified)
  //  Evaluate the color of the mission, go to default 128 128 128 if needed
  if (theLevelInfo.missionColor !$= "")
    %missionColor = theLevelInfo.missionColor;
  else
    %missionColor = "128 128 128";
  // Get the camo color of %obj, defaulting to 128 128 128 if needed
  if (%obj.getDatablock().camoColor !$= "")
    %objColor = %obj.getDatablock().camoColor;
  else
    %objColor = "128 128 128";
  %rDif = mAbs(getWord(%missionColor,0) - getWord(%objColor,0));
  %gDif = mAbs(getWord(%missionColor,1) - getWord(%objColor,1));
  %bDif = mAbs(getWord(%missionColor,2) - getWord(%objColor,2));
  %avgDif = (%rDif + %gDif + %bDif) / 3;
  // pt = 0,0.01 90,2.0
  %cRate = (2.0 - 0.01) / (90 - 0);
  %cWeight = (%cRate * (%avgDif - 0)) + 0.05;
  %weight *= %cWeight;

  // If the object is higher than us, he is less visible
  %vertDist = getWord(%obj.getPosition(),2) - getWord(%plr.getPosition(),2);
  if (%vertDist > 0)
  {
    %heightWeight = 1 - ((1.0 - 0.01) / (20) * %vertDist); // 20 is the height at which the weight is 0.001. hack.
    if (%heightWeight < 0.01)
      %heightWeight = 0.01;
    %weight *= %heightWeight;
  }

  // Finally, multiply this by the overall level visibility
  if (isObject(TheLevelInfo))
  {
    %genVis = TheLevelInfo.generalVisibility;
    if (%genVis $= "")
    {
      %genVis = 1.0;
    }
    %weight *= %genVis;
  }

  return %weight;
}

function Brain1::isPosVisible(%this,%pos)
{
   %plr = %this.player;

   if (VectorDist(%plr.getTrueEyePoint(),%pos) <= $Brain1::MaxVisRange)
   {
     // Let's see if the position is in our field of view
     %angle = %plr.getAngleTo(%pos);
     if (%angle < %plr.fov)
       %pView = 1;
     else
       %pView = 0;

     %rand = getRandom();
     %prob = %pView * %this.visMod;

     if (%rand < %prob)
     {
         return (!ContainerRayCast(%plr.getTrueEyePoint(),%pos,$Brain1::OpaqueMasks,%plr));
     }
   }
   return false; // If we detect the object, we would have returned true by now. We don't, so return false.
}

// Checks if it is clear from our player's position to %pos
function Brain1::isClearTo(%this,%pos)
{
  %plr = %this.player;
    return (!ContainerRayCast(VectorAdd(%plr.getPosition(),"0 0 0.1"),%pos,$Brain1::SolidMasks,%plr));
}

//----------------------------------------
// Misc functions
//----------------------------------------

// Checks if we are fully exposed to an enemy (his muzzle point can see our feet, head, and torso)
function Brain1::isExposed(%this,%enemy)
{
  %plr = %this.player;

  %muzzle = %enemy.getTrueMuzzlePoint(0);
  %myFeet = %plr.getPosition();
  %myTorso = %plr.getWorldBoxCenter();
  %myHead = %plr.getEyePoint();

  %ray1 = ContainerRayCast(%muzzle,%myFeet,$Brain1::SolidMasks);
  %ray2 = ContainerRayCast(%muzzle,%myTorso,$Brain1::SolidMasks);
  %ray3 = ContainerRayCast(%muzzle,%myHead,$Brain1::SolidMasks);

  if (!%ray1 && !%ray2 && !%ray3)
    return true;
  else
    return false;
}

// When an NPC exits cover, we want him to be in a position where he can find another threat
function Brain1::BumpOutOfCover(%this)
{
  %plr = %this.player;
  if (!%this.returnFirePos)
    return;

  %plrVel = %plr.getVelocity();
  %vecToRFP = VectorSub(%this.returnFirePos,%plr.getPosition());
  %pushSpeed = $Brain1::CoverPushSpeed;
  %pushVector = VectorNormalize(%vecToRFP);
  %pushVector = VectorScale(%pushVector,%pushSpeed);
  %pushVector = VectorAdd(%plrVel,%pushVector);
  %pushVector = setWord(%pushVector,2,"0");

  // Now push
  %plr.setVelocity(%pushVector);
}

// Checks to see if we're crowding (or getting crowded by) our teammates and bumps the
//   offending players out of the way
// Getting too close to a teammate can look dumb (and not to mention sexual...),
//   so we like to avoid this, especially with squads
function Brain1::AvoidCrowding(%this)
{
  %plr = %this.player;

  // Do a simple radius search to check for nearby friends
  InitContainerRadiusSearch(%plr.getPosition(),3,$TypeMasks::PlayerObjectType);
  while((%obj = ContainerSearchNext()) != 0)
  {
    //if (%obj.team != %plr.team && %obj.team != -1 && !%obj.isCivilian)
      //continue; // We shouldn't get out of the way of enemy players. No sense in being polite to them.
    if (%obj == %plr)
      continue; // We can't get in the way of ourselves

    %distToObj = VectorDist(%plr.getPosition(),%obj.getPosition());
    if (%distToObj <= $Think::IsCrowdingDist)
    {
      // If it's an enemy, attack him
      if (%obj.team != %plr.team && %obj.team != -1 && !%obj.isCivilian)
      {
        echo(%plr @ ": touched enemy player " @ %obj @ "!");
        %this.attackObj = %obj;
        %this.lastKnownEnemyPos = %obj.position;
        %this.assumedEnemyPosition = %obj.position;
        %this.attackObjIsVisible = true;
        %plr.setAimLocation(%obj.position);
        %plr.suppressingFire(%this.attackObj.getWorldBoxCenter(),1,%this.attackObj);
        %this.suppressPos = %obj.position;
        return;
      }

      // Check our velocities to see who is in the way of who
      %myVel = %plr.getVelocity();
       %myVelN = VectorNormalize(%myVel);
        %myLen = mFloor(VectorLen(%myVel));
      %objVel = %obj.getVelocity();
       %objVelN = VectorNormalize(%objVel);
        %objLen = mFloor(VectorLen(%objVel));
      %vecToObj = VectorNormalize(VectorSub(%obj.getPosition(),%plr.getPosition()));
      %vecToMe = VectorNormalize(VectorSub(%plr.getPosition(),%obj.getPosition()));

      %velDot = VectorDot(%myVelN,%objVelN);
      %positionDot = VectorDot(%vecToObj,%objVelN);

      // If I am moving faster than %obj
      if (%myLen > %objLen)
      {
        // Bump him out of the way if he is not at cover and he hasn't gotten out of our way
        if (%distToObj <= ($Think::IsCrowdingDist / 2) && (!%this.coverMarker || %this.brain.state $= "React") && (!%obj.brain.coverMarker || %obj.brain.state $= "REACT"))
        {
          %pushVelocity = $Brain1::PushVelocity;
          %vector = VectorScale(%vecToObj,%pushVelocity);
          %sidePushVelocity = getRandom(-%pushVelocity,%pushVelocity);
          %sidePushVector = %sidePushVelocity SPC "0" SPC "0";
          %vector = VectorAdd(%vector,%sidePushVector);
          %obj.setVelocity(%vector);
        }
      }
      else if (%objLen > %myLen) // Is %obj moving faster than me?
      {
        // Get out of the way if we're not at cover
        if (!%this.coverMarker || %this.brain.state $= "React")
        {
          %pushVelocity = $Brain1::PushVelocity;
          %vector = VectorScale(%vecToMe,%pushVelocity);
          %sidePushVelocity = getRandom(-%pushVelocity,%pushVelocity);
          %sidePushVector = %sidePushVelocity SPC "0" SPC "0";
          %vector = VectorAdd(%vector,%sidePushVector);
          %plr.setVelocity(%vector);
        }
      }
      else // We must be moving the same speed
      {
        if (%myLen > 0)
        {
          if (%positionDot > 0)
          {
            // Bump him out of the way if he is not at cover and he hasn't gotten out of our way
            if (%distToObj <= ($Think::IsCrowdingDist / 2) && !%this.coverMarker && (!%obj.brain.coverMarker || %obj.brain.state $= "REACT"))
            {
              %pushVelocity = $Brain1::PushVelocity;
              %vector = VectorScale(%vecToObj,%pushVelocity);
              %sidePushVelocity = getRandom(-%pushVelocity,%pushVelocity);
              %sidePushVector = %sidePushVelocity SPC "0" SPC "0";
              %vector = VectorAdd(%vector,%sidePushVector);
              %obj.setVelocity(%vector);
            }
            else
            {
              // Otherwise, try to move around him
              %pushVelocity = $Brain1::PushVelocity;
              %vector = VectorScale(%vecToMe,%pushVelocity);
              %sidePushVelocity = getRandom(-%pushVelocity,%pushVelocity);
              %sidePushVector = %sidePushVelocity SPC "0" SPC "0";
              %vector = VectorAdd(%vector,%sidePushVector);
              %plr.setVelocity(%vector);
            }
          }
          else
          {
            // Get out of the way if we're not at cover
            if ((!%this.coverMarker || %this.brain.state $= "React") && (!%obj.brain.coverMarker || %obj.brain.state $= "REACT"))
            {
              %pushVelocity = $Brain1::PushVelocity;
              %vector = VectorScale(%vecToMe,%pushVelocity);
              %sidePushVelocity = getRandom(-%pushVelocity,%pushVelocity);
              %sidePushVector = %sidePushVelocity SPC "0" SPC "0";
              %vector = VectorAdd(%vector,%sidePushVector);
              %plr.setVelocity(%vector);
            }
          }
        }
        else
        {
          // Get out of the way if we're not at cover
          if (!%this.coverMarker && (!%obj.brain.coverMarker || %obj.brain.state $= "REACT"))
          {
            %pushVelocity = $Brain1::PushVelocity;
            %vector = VectorScale(%vecToMe,%pushVelocity);
            %sidePushVelocity = getRandom(-%pushVelocity,%pushVelocity);
            %sidePushVector = %sidePushVelocity SPC "0" SPC "0";
            %vector = VectorAdd(%vector,%sidePushVector);
            %plr.setVelocity(%vector);
          }
        }
      }
    }
  }
}

//----------------------------------------
// Reloading
//----------------------------------------

// Decide if we should reload based on the amount of ammo in our weapon's clip and whether or not we are attacking
function Brain1::shouldReload(%this)
{
  if (%this.reloading != false)
    return false;
  %plr = %this.player;
  %ammoInClip = %plr.getInventory(%this.ammo);
  %clipSize = %plr.getMountedImage(0).clipSize;
  %clipPercentFull = %ammoInClip / %clipSize;
  if (!isObject(%this.attackObj) && !%plr.isImageFiring(0))
  {
    if (%clipPercentFull <= $Brain1::LowAmmoPercentage)
    {
      return true;
    }
    else
    {
      return false;
    }
  }
  else
  {
    if (%ammoInClip < 1)
    {
      return true;
    }
    else
    {
      return false;
    }
  }
}

// Reload our current weapon
function Brain1::ReloadWeapon(%this,%noSound)
{
  if (!%this.reloading)
  {
   %plr = %this.player;
   %weapon = %plr.getMountedImage(0);
   %ammoType = %weapon.ammo;
   %time = %weapon.reloadtime;

   %plr.setIsReloading(true);
   %plr.setMoveSpeed(0.5); // Move slower while reloading
   %plr.schedule(%time,setInventory,%ammoType,%weapon.clipSize);
   %plr.schedule(%time,setIsReloading,false);
   %plr.schedule(%time,setMoveSpeed,1);
   if(!%noSound) // hack
   {
     %plr.playAudio(0,"M4a1ReloadSound");
   }
   //%plr.setActionThread("reloadrifle");
  }
}

//----------------------------------------
// Chance/random functions
//----------------------------------------

// There is a very large chance that this function will return true
function Brain1::VeryLargeChanceRandom()
{
  if (getRandom(0,$Brain1::VeryLargeChanceRandom) == 1)
    return true;
}

// There is a large chance that this function will return true
function Brain1::LargeChanceRandom()
{
  if (getRandom(0,$Brain1::LargeChanceRandom) == 1)
    return true;
}

// There is a medium chance that this function will return true
function Brain1::MediumChanceRandom()
{
  if (getRandom(0,$Brain1::MediumChanceRandom) == 1)
    return true;
}

// There is a small chance that this function will return true
function Brain1::SmallChanceRandom()
{
  if (getRandom(0,$Brain1::SmallChanceRandom) == 1)
    return true;
}

//----------------------------------------
// Convenience functions
//----------------------------------------

// Gets the Z-height from the ground of %pos1, and gives that to %pos2
function getZFromPos(%pos1,%pos2)
{
   %xy1 = getWords(%pos1,0,1);
   %z1 = getWord(%pos1,2);
   %terrHeight1 = getTerrainHeight(%xy1);

   %zDifference = %z1 - %terrHeight1;

   %xy = getWords(%pos2,0,1);
   %z = getWord(%pos2,2);
   %z = %z + %zDifference;

   %returnPos = %xy SPC %z;
      return %returnPos;
}

// Get the closest position beside %obj at distance %dist
function Brain1::getPosBeside(%this, %obj, %dist)
{
  %plr = %this.player;

  // compute positions left and right
  %vecL = AIPlayer::vecRotateAz(%obj.getEyeVector(), 90);
  %vecR = AIPlayer::vecRotateAz(%obj.getEyeVector(), -90);

  %posL = VectorAdd(%obj.position, VectorScale(%vecL, %dist));
  %posR = VectorAdd(%obj.position, VectorScale(%vecR, %dist));

  // return closest side
  %distL = %plr.getDistTo(%posL);
  %distR = %plr.getDistTo(%posR);

  if( %distL < %distR )
    return %posL;
  return %posR;
}

// Gets the first node ahead of us on our path that we can't see
function Brain1::getHiddenNodeAhead(%this)
{
  %plr = %this.player;

  %curNodeIdx = %plr.curNode; // Node we are moving to
  if (%aheadNodeIdx < %plr.pathNodeArray.count() - 2)
  {
    %curNode = %plr.pathNodeArray.getKey(%curNodeIdx);
    // Cycle through the nodes ahead of us on our path and cast rays to them
    for (%i = %curNodeIdx + 1;%i < %plr.pathNodeArray.count() - 2;%i++)
    {
      %aheadNode = %plr.pathNodeArray.getKey(%i);
      if (isObject(%aheadNode))
      {
        %ray = ContainerRayCast(%plr.getTrueEyePoint(),%aheadNode.getPosition(),$AIPlayer::SolidMasks);
        if (%ray) // If the ray hits something, the node is not visible to us
        {
          return %aheadNode;
        }
      }
    }
  }
}

// Finds the number of visible enemies
function Brain1::getOppositionCount(%this,%createList)
{
  %plr = %this.player;
  %oppositionCount = 0;
  %enemyList = "";
  //for(%i = 0;%i < PlayerArray.count();%i++)
  //{
    //%obj = PlayerArray.getKey(%i);

  InitContainerRadiusSearch(%plr.getPosition(),%plr.maxViewDist,$TypeMasks::PlayerObjectType);
  while((%obj = ContainerSearchNext()) != 0)
  {
    if (%obj.team == %plr.team || %obj.team == -1 || (%obj.isCivilian && $Think::KillCivilians == true))
      continue;
    if (%obj.isCivilian)
      continue;
    if (%obj.isAsleep)
      continue;
    if (%this.isClearTo(%obj.getTrueEyePoint()) == true)
    {
      %oppositionCount++;
      if (%createList == true)
      {
        %enemyList = %enemyList @ %obj @ " ";
      }
    }
  }
  return %oppositionCount SPC %enemyList;
}

// Returns one other enemy that we are not attacking
//  This helps us to attack one enemy while staying hidden from the secondaries
function Brain1::getSecondaryThreat(%this,%primaryTarget)
{
  %plr = %this.player;
  for(%i = 0;%i < PlayerArray.count();%i++)
  {
    %obj = PlayerArray.getKey(%i);
    if (%obj.team == %plr.team)
      continue;
    if (%obj.isCivilian)
      continue;
    if (%obj.isAsleep)
      continue;
    if (%obj != %primaryTarget && %this.isObjVisible(%obj) == true)
    {
      return %obj;
    }
  }
  return 0;
}

// Sees if crouching will break line-of-sight with %pos
function Brain1::CrouchWillBreakLOS(%this,%pos)
{
  %plr = %this.player;

  // Do a raycast to see if it is clear
  %ray = ContainerRayCast(%plr.getTrueEyePoint(),%pos,$Brain1::SolidMasks);
  if (!%ray)
    return false;
  else
    return true;
}

//----------------------------------------
// Stress level management
//----------------------------------------

// Change the stress level to %amount
function Brain1::setStress(%this,%amount)
{
  %curStress = %this.stress;
 //%newStress = %curStress + %amount;
  %newStress = %amount;
  if (%newStress >= 100) // If too big
  {
    %this.stress = 100;
      return;
  }
  else if (%newStress <= 0) // If too small
  {
    %this.stress = 0;
      return;
  }
  else // Just right
  {
    %this.stress = %newStress;
      return;
  }
}
// Add %amount to our stress level
function Brain1::raiseStress(%this,%amount)
{
  %curStress = %this.stress;
  %newStress = %curStress + %amount;
  %this.setStress(%newStress);
}
// Subtract %amount to our stress level
function Brain1::lowerStress(%this,%amount)
{
  %curStress = %this.stress;
  %newStress = %curStress - %amount;
  %this.setStress(%newStress);
}

//----------------------------------------
// Alertness functions
//----------------------------------------

// Sets our alert level to %val
function Brain1::setAlertness(%this,%val)
{
  %cur = %this.alertness; // How alert we are now

  // Special case:
  // Alert level 1 means we aren't sure if a threat is present or not
  // We want to be able to eventually switch down to alert level 0
  // Switching down should not be allowed for the other alert levels
  if (%cur == 1)
  {
    %this.alertness = %val;
      return;
  }
  // Here, we only switch if %val is bigger than our current alert level
  else
  {
    if (%val > %cur)
    {
      %this.alertness = %val;
        return;
    }
  }
}

//----------------------------------------
// Cover Searching
//----------------------------------------

// Gets the best cover marker that isn't taken
function Brain1::getBestCoverMarker(%this,%enemyPos,%movingUp)
{
  %plr = %this.player;

  if ($nocoversearch)
    return 0;

  //------------------------------------------------------------------------------------------------
  // Set up a ScriptObject that keeps track of the markers we find and their data
  %search = new ScriptObject()
  {
    count = 0; // Keeps track of how many markers we have found
  };
  MissionCleanup.add(%search);

  //------------------------------------------------------------------------------------------------
  $count = 0;
  // Start a radius search to look for markers
  InitContainerRadiusSearch(%plr.getPosition(),$Brain1::CoverRange,$TypeMasks::StaticShapeObjectType);
  while((%obj = ContainerSearchNext()) != 0)
  {
    // Don't check too many markers
    if (%search.count > $Brain1::NumMarkersToSearch)
      break;

    %group = %obj.getGroup().getName();
    if (%group $= "CoverPoints" && !%obj.isTaken && !%plr.checkCoverMarkerTaken(%obj)) // If the marker belongs to the correct group and isn't already being used, we can use it
    {
      %search.marker[%search.count] = %obj; // Keep track of this marker
      %search.count++; // Increment the count so we know how many markers we've found
    }
  }
  // Make sure we found at least 1 marker
  if (%search.count < 1)
  {
    // No cover in the area
    warn(%plr @ ": no cover close enough.");
    %plr.setCoverSearching(false);
    %plr.schedule(5000,0,setCoverSearching,true);
    return 0;
  }
  //------------------------------------------------------------------------------------------------

  // Get the weight of each marker based on their distance from the cover-searching player to the marker
  // and whether the marker is cover or concealment.
  %count = %search.count;
  for (%i = 0;%i < %count;%i++)
  {
    %marker = %search.marker[%i];
    %markerPos = %marker.getPosition();

    // Get information about this marker
    %distPlrToMarker = VectorDist(%plr.getPosition(),%markerPos);
    %distEnemyToMarker = VectorDist(%enemyPos,%markerPos);
    %fireOverCover = %marker.fireOverThis;
    %isConcealment = %marker.isConcealment;

    // FORMAT: %distPlrToMarkerWeight = (MaxWeight - MinWeight)/(MaxPlrMarkerDist) * actualplrmarkerdist + minweight;
    %distPlrToMarkerWeight = %distPlrToMarker * 5; // Closer markers are better
    %distEnemyToMarkerWeight = %distEnemyToMarker * 20; // Don't get too close to the enemy
    %isConcealmentWeight = (10 * %isConcealment) + 1; // Prefer cover over concealment
    
    if (%movingUp == true && %distPlrToMarker > 30)
    {
    	warn(%plr @ ": unsafe to move up");
      %search.canProtect[%i] = false;
    }
    
    //------------------------------------------------------------------------------------------------
    // Figure out if we are safe when behind cover, and can hit the enemy when out of cover

    %footPos = PlayerPointZMod(%markerPos, "Foot");
    %waistPos = PlayerPointZMod(%markerPos, "Waist");
    %muzzlePos = PlayerPointZMod(%markerPos, "Muzzle");
    %eyePos = PlayerPointZMod(%markerPos, "Eye");
    %distance = VectorDist(%enemyPos,%markerPos);
    %vecToAttacker = VectorSub(%enemyPos,%markerPos);
    %vecToAttackerNorm = VectorNormalize(%vecToAttacker);
    %muzzleForwardPos = VectorScale(%vecToAttackerNorm,4);
    %muzzleForwardPos = VectorAdd(%muzzleForwardPos,%muzzlePos);
    %footForwardPos = VectorScale(%vecToAttackerNorm,4);
    %footForwardPos = VectorAdd(%footForwardPos,%footPos);
    %muzzleBlocked = ContainerRayCast(%muzzlePos, %muzzleForwardPos, $AIPlayer::SolidMasks);
    %rayFoot = ContainerRayCast(%footPos,%footForwardPos,$AIPlayer::CoverMasks);

    %markerVec = VectorNormalize(%marker.getForwardVector());
    %vecToAttackObj = VectorNormalize(VectorSub(%enemyPos,%marker.getPosition()));
    %dot = VectorDot(%markerVec,%vecToAttackObj);
    if (%dot > 0.6)
    {
      %search.canProtect[%i] = true;
    }
    else
    {
      %search.canProtect[%i] = false;
    }

    // Now let's see if we can return fire
    if (%fireOverCover == true)
    {
      if (%muzzleBlocked == true) // If we cannot see the enemy if we stand up from behind this cover
        %returnFireWeight = 20; // Make this cover less desirable
      else
        %returnFireWeight = 0;
    }
    else // If we don't fire over this cover
    {
      if (%marker.rightFirePos !$= "" || %marker.leftFirePos !$= "") // If this marker has at least one location we can move to return fire
      {
        if (%marker.rightFirePos !$= "") // If it can let us fire from the right
        {
          %rightPos = %marker.rightFirePos;
          %rightPosVec = VectorSub(%marker.rightFirePos,%marker.getPosition());
          %rightPosVec = VectorScale(%rightPosVec,1.2);
          %rightPosForRay = VectorAdd(%marker.getPosition(),%rightPosVec);

          %rightFwd = VectorAdd(VectorScale(VectorNormalize(VectorSub(%enemyPos,%markerPos)),5), %rightPos);
          %rayRight = ContainerRayCast(%rightPosForRay,%enemyPos,$AIPlayer::CoverMasks);
          if (!%rayRight)
          {
            %search.returnFirePos[%i] = %rightPos;
            %search.fireRight[%i] = true;
          }
        }
        if (%marker.leftFirePos !$= "") // If it can let us fire from the left [WAS ELSE IF]
        {
          %leftPos = %marker.leftFirePos;
          %leftPosVec = VectorSub(%marker.leftFirePos,%marker.getPosition());
          %leftPosVec = VectorScale(%leftPosVec,1.2);
          %leftPosForRay = VectorAdd(%marker.getPosition(),%leftPosVec);

          %leftFwd = VectorAdd(VectorScale(VectorNormalize(VectorSub(%enemyPos,%markerPos)),5), %leftPos);
          %rayLeft = ContainerRayCast(%leftPosForRay,%enemyPos,$AIPlayer::CoverMasks);
          if (!%rayLeft)
          {
            %search.returnFirePos[%i] = %leftPos;
            %search.fireRight[%i] = false;
          }
        }
      }
      if (%search.returnFirePos[%i] $= "" && %fireOverCover == false) // If we never saved a returnFirePos (meaning we never entered either control path dealing with left and right positions), we can't return fire from this marker!
      {
        %search.canProtect[%i] = false;
      }
    }

    //------------------------------------------------------------------------------------------------
    // Figure out the total weight for this marker
    %totalWeight = %distPlrToMarkerWeight + %distEnemyToMarkerWeight + %isConcealmentWeight;

    // Save the information we have gathered
    %search.distPlrToMarker[%i] = %distPlrToMarker;
    %search.distEnemyToMarker[%i] = %distEnemyToMarker;
    %search.fireOverCover[%i] = %fireOverCover;
    %search.isConcealment[%i] = %isConcealment;

    %search.distPlrToMarkerWeight[%i] = %distPlrToMarkerWeight;
    %search.distEnemyToMarkerWeight[%i] = %distEnemyToMarkerWeight;
    %search.fireOverCoverWeight[%i] = %fireOverCoverWeight;
    %search.isConcealmentWeight[%i] = %isConcealmentWeight;
    %search.returnFireWeight[%i] = %returnFireWeight;
    %search.totalWeight[%i] = %totalWeight;
  }
  //------------------------------------------------------------------------------------------------
  // Now we look to see which marker has the lowest weight, which is the most desirable cover

  %lowestWeight = 1000000000000000; // Each time the loop cycles, we see if the next marker's weight is lower than the one before it
                                  // The idea is to start out with a weight that is definitely going to be beaten by a lower one
  %bestMarker = -1;
  for (%i = 0;%i < %count;%i++)
  {
    %marker = %search.marker[%i];
    %markerWeight = %search.totalWeight[%i];
    if (%search.canProtect[%i] && %markerWeight < %lowestWeight) // If this marker can protect the NPC and its weight is lower
    {
      %bestMarker = %marker;
      %bestMarkerFireOverThis = %marker.fireOverThis;
      %bestMarkerReturnFirePos = %search.returnFirePos[%i];
      %bestMarkerFireRight = %search.fireRight[%i];
      %lowestWeight = %markerWeight;
    }
  }
  if (%bestMarker == -1)
  {
    return 0;
  }

  // If we are this far, we know what the best marker is
  // Now, we return a string that helps the NPC know what cover we are using and what to do with it
  // The string:   markerid [space] fireovercover [space] returnfirepos [space] fireright
  if (%bestMarker.fireOverThis == 1) // If we fire over this cover, we return a string that reflects that
  {
    return %bestMarker SPC 1 SPC 0;
  }
  else // We dont fire over this marker's cover
  {
    return %bestMarker SPC 0 SPC %bestMarkerReturnFirePos SPC %bestMarkerFireRight;
  }
}

//----------------------------------------
// Path Following
//   Different from path finding; these
//   functions are for following a pre-
//   placed path in the mission.
//----------------------------------------

// Gets the closest node of our assigned path
function Brain1::GetClosestPathNode(%this,%path)
{
  // Add all of the markers in our path to an array for analysis
  %closeArray = new ArrayObject() {};
  MissionCleanup.add(%closeArray);
  for(%a=0;%a<%path.getCount();%a++)
  {
    %marker = %path.getObject(%a);
    %closeArray.add(%a,%a);
    %distance = VectorDist(%this.player.getPosition(),%marker.getPosition());
    %closeArray.setValue(%distance,%a);
  }
  // Make sure that there are nodes in the array
  if (%closeArray.count() <= 0)
    return 0;

  // Find out what the lowest distance is
  %lowestDistance = 10000000;
  %closestMarker = 0;
  for(%d=0;%d<%closeArray.count();%d++)
  {
    %checkMarkerIdx = %closeArray.getKey(%d);
    %val = %closeArray.getValue(%d);
    if (%val < %lowestDistance)
    {
      %lowestDistance = %val;
      %closestMarker = %checkMarkerIdx;
    }
  }
  return %closestMarker;
}

// Move to a marker on our path, but only if we are in our idle state ("NONE")
function Brain1::MoveToMarker(%this,%path,%index,%useNavigation)
{
  if (%this.state !$= "NONE")
    return;

  %this.pathPause = false;

  // Find out what marker is at %index in %path
  %marker = %path.getObject(%index);
  if (%marker != -1)
  {
    // See if there is a specified move speed
    if (%marker.moveSpeed !$= "")
    {
      %mSpeed = %marker.moveSpeed;
    }
    else
    {
      %mSpeed = 1.0;
    }

    %this.player.setMoveSpeed(%mSpeed);
    if (%useNavigation == true)
      %this.player.setOnPath(AIPlayer::getGroundPos(%marker.getPosition()));
    else
      %this.player.setMoveDestination(AIPlayer::getGroundPos(%marker.getPosition()));

    %this.curPathNodeIndex = %index;
    warn(%this.player @ ": On path " @ %path @ ", moving to node at index " @ %index);
  }
}

// Move to the next marker on our path
function Brain1::MoveToNextMarker(%this,%path)
{
  // Figure out the index of the next marker
  // First, we check to see what the index of the last marker in the path is
  %count = %path.getCount();
  %lastIdx = %count - 1;
  %curNode = %this.curPathNodeIndex;
  %nextNodeIdx = %this.curPathNodeIndex + 1;

  // If %nextNodeIdx is larger than %lastIdx, then move to the first marker on our path
  if (%nextNodeIdx > %lastIdx)
  {
    %this.moveToMarker(%path,0,true);
  }
  else // Else, move to the next marker on the path
  {
    %this.moveToMarker(%path,%nextNodeIdx);
  }
}

//----------------------------------------
// Various "Is this a good idea?" functions
//----------------------------------------

// Decide if our squad's rules of engagement allow us to attack an enemy
// If not, see if we are under attack or if the enemy is about to shoot at us
//  This returns 0, 1, or 2:
//    0: Hold Fire
//    1: Kill the threat
//    2: Kill stealthily
function Brain1::shouldReturnFire(%this,%enemy)
{
  %plr = %this.player;

  %squad = %plr.squad;
  %roe = %squad.roe;

  if (!isObject(%enemy))
    return 0;

  // See if the squad's rules of engagement allow us to kill the enemy
  if (%roe == 0) // 0 means fire at will
  {
    return 1;
  }
  else // Otherwise, we're set to return fire only
  {
    // See if we can kill this enemy without alerting anyone else
    if (%this.canKillStealthily(%enemy) && 1 < 0) // 1 < 0 hack, disabling stealth kills for now
    {
      echo(%plr @ ": attempting stealth kill");
      return 2;
    }

    if (%enemy.brain.attackObj == %plr || %enemy.brain.attackObj.squad == %plr.squad)
    {
      return 1; // Cheating here, but if he's engaging us or a squad member, shoot back
    }

    // See if %enemy is looking our way and is close enough
    // If he is, we're in danger, so take him down rather than wait for him to shoot first
    %vecEnemyToPlr = VectorNormalize(VectorSub(%plr.getPosition(),%enemy.getPosition()));
    %vecPlrToEnemy = VectorNormalize(VectorSub(%enemy.getPosition(),%plr.getPosition()));
    %vecEnemyMuzzle = VectorNormalize(%enemy.getTrueMuzzleVector(0));
    %dot = VectorDot(%vecEnemyToPlr,%vecEnemyMuzzle);
    %dist = VectorDist(%plr.getPosition(),%enemy.getPosition());

    // If he is very close, open fire
    if (%dist < 2) // hack
      return 1;

    // If he is looking this way...
    if (%dot > 0.4) // hack
    {
      if (%enemy.getImageTrigger(0) == true)
        return 1; // ...and he is shooting at us, kill him

      %guessEnemyVis = $Brain1::MinVisRange * 0.9;
      if (%dist <= %guessEnemyVis)
      {
        return 1; // ...and he is close enough to notice us, kill him
      }
    }
  }
  return 0; // No reason to engage, so hold fire
}

// Checks to see if it's safe for us to move while hiding from an enemy
function Brain1::SafeToProneAdvance(%this)
{
  %plr = %this.player;

  // get a list of the enemies who can see us
  %opStr = %this.getOppositionCount(1);
  %list = getWords(%opStr,1,8);
  %safe = true;
  for(%i = 0;%i < getWordCount(%list);%i++)
  {
    %obj = getWord(%list,%i);
    if (!isObject(%obj))
      continue;

    %dist = VectorDist(%plr.getPosition(),%obj.getPosition());
    %dir = VectorNormalize(%obj.getTrueMuzzleVector(0));
    %vecTo = VectorNormalize(VectorSub(%plr.getWorldBoxCenter(),%obj.getTrueMuzzlePoint(0)));
    %dot = VectorDot(%dir,%vecTo);

    if (%dist > 20) // hack
    {
      %safe = true;
      continue;
    }
    else
    {
      if (%dot < 0.3)
      {
        %safe = true;
        continue;
      }
      else
      {
        %safe = false;
        return %safe;
      }
    }
  }
  return true;
}

// checks to see if there are enemies within %radius
function Brain1::EnemiesWithinRadius(%this,%radius)
{
  %plr = %this.player;

  InitContainerRadiusSearch(%plr.getPosition(),%plr.maxViewDist,$TypeMasks::PlayerObjectType);
  while((%obj = ContainerSearchNext()) != 0)
  {
    if (%obj.team == %plr.team || %obj.team == -1 || (%obj.isCivilian && $Think::KillCivilians == true))
      continue;
    if (%obj.isCivilian)
      continue;
    if (%obj.isAsleep)
      continue;

    return true;
  }
}

// Checks to see if we can kill an enemy stealthily (without alerting anyone else) with our current weapon
function Brain1::CanKillStealthily(%this,%enemy)
{
  %plr = %this.player;
  %bulletPos = %enemy.getTrueEyePoint(); // Where the bullet goes
  %shotDist = %this.weapon.fireSoundRadius; // How far a shot from our gun can be heard

  // If the enemy is already alert, who cares about stealth. Survive.
  if (%enemy.brain.alertness >= 2)
    return true;

  // First, check to see if nearby enemies would be able to hear the gunshot
  InitContainerRadiusSearch(%plr.getTrueMuzzlePoint(0),%shotDist,$TypeMasks::PlayerObjectType);
  while((%obj = ContainerSearchNext()) != 0)
  {
    if (%obj.team != %plr.team && %obj != %enemy) // Another enemy player is close enough to hear it
    {
      warn(%plr @ ": cannot perform stealth kill, the shot will be heard by " @ %obj @ " with " @ %shotDist @ "m");
      return false; // Can't kill this guy stealthily
    }
  }

  %obj = 0;
  // Now, check to see if the bullet impact will alert anyone
  %bulletAlertDist = %this.weapon.projectile.alertDist;
  if (!%bulletAlertdist)
    %bulletAlertDist = 15;

  InitContainerRadiusSearch(%bulletPos,%bulletAlertDist,$TypeMasks::PlayerObjectType);
  while((%obj = ContainerSearchNext()) != 0)
  {
    if (%obj.team != %plr.team && %obj != %enemy)
    {
      warn(%plr @ ": cannot perform stealth kill, the bullet impact will alert another threat");
      return false;
    }
  }

  %obj = 0;
  // Lastly, check to see if anybody will be able to see the body
  InitContainerRadiusSearch(%bulletPos,$Brain1::MinVisRange * $Brain1::BodyVisWeight,$TypeMasks::PlayerObjectType);
  while((%obj = ContainerSearchNext()) != 0)
  {
    if (%obj.team == %enemy.team && %obj != %enemy)
    {
      %ray = ContainerRayCast(AIPlayer::getGroundPos(%bulletPos),%obj.getTrueEyePoint(),$AIPlayer::SolidMasks);
      if (!%ray)
      {
        warn(%plr @ ": cannot perform stealth kill, the body will be found");
        return false;
      }
    }
  }

  // If we made it this far in the function without returning false, we can kill %enemy without alerting others
  return true;
}

// Figure out whether or not it's worth it to take cover
// Factors in visibility (clear shot?), distance, and enemy aim direction
function Brain1::ShouldTakeCover(%this,%enemy)
{
  %plr = %this.player;

  if (!isObject(%enemy))
    return false;

  if (%this.getOppositionCount() > 2)
    return true;

  // See if the enemy is too close
  %plrPosition = %plr.getPosition();
  %enemyPosition = %enemy.getPosition();

  %dist = VectorDist(%plrPosition,%enemyPosition);
  if (%dist > $Brain1::TooCloseDist)
    %notTooClose = true;
  else
    %notTooClose = false;

  // see if we have sufficient ammo
  %weapon = %this.weapon @ "Image";
  %clipSize = %weapon.clipSize;
  %ammo = %plr.getInventory(%this.ammo);
  %clipPercent = %ammo / %clipSize;

  if (%clipPercent < $Brain1::LowAmmoPercentage)
    %lowAmmo = true;
  else
    %lowAmmo = false;

  // See if the enemy is looking in our direction
  %vecEnemyToPlr = VectorNormalize(VectorSub(%plrPosition,%enemyPosition));
  %muzzleVec = VectorNormalize(%enemy.getTrueMuzzleVector($WeaponSlot));
  %dot = vectorDot(%vecEnemyToPlayer,%muzzleVec);
  if (%dot <= $Brain1::AttackFindCoverDot)
    %isFacing = true;
  else
    %isFacing = false;

  // See how good of a shot we have at our opponent
  %visibility = 0;

  %posFoot = %enemy.getPosition(); // Enemy's feet
  %posChest = %enemy.getWorldBoxCenter(); // Enemy's abdomen
  %posHead = %enemy.getTrueEyePoint(); // Enemy's head

  %rayFoot = ContainerRayCast(%plr.getTrueMuzzlePoint(0),%posFoot,$Brain1::SolidMasks);
  %rayChest = ContainerRayCast(%plr.getTrueMuzzlePoint(0),%posChest,$Brain1::SolidMasks);
  %rayHead = ContainerRayCast(%plr.getTrueMuzzlePoint(0),%posHead,$Brain1::SolidMasks);

  if (%rayFoot)
    %visibility += 0.333;
  if (%rayChest)
    %visibility += 0.333;
  if (%rayHead)
    %visibility += 0.333;

  if (%visibility < $Brain1::OpenVisiblity) // If the object is not completely visible
    %isCovered = true;
  else
    %isCovered = false;

  // Now, based on distance, aim direction, and visiblity, we decide if taking cover is worth it
  %worthIt = false;

  if (!%this.noCoverSearching) // If cover searching is allowed...
  {
    if (%lowAmmo == true)
      %worthIt = true;

    if (%notTooClose == true) // ...the enemy isn't too close...
    {
      if (%isFacing == true) // ...he poses a threat...
      {
        %worthIt = true; // Then taking cover is a good idea
      }
    }
  }
  return %worthIt;
}

//----------------------------------------
// Melee attack
//----------------------------------------

// Perform a melee attack on %obj
function Brain1::DoMelee(%this,%obj)
{
  %plr = %this.player;

  %plr.setAimLocation(%obj.getWorldBoxCenter());

  if (%this.isMelee)
    return; // Already performing a melee attack

  %vecToObj = VectorSub(%obj.getPosition(),%plr.getPosition());
  %vecToObjN = VectorNormalize(%vecToObj);
  warn(%plr @ ": melee attack on " @ %obj);
  if (VectorLen(%plr.getVelocity()) < 8)
    %plr.applyImpulse(%plr.getWorldBoxCenter(),VectorScale(%vecToObjN,1000));

  %plr.schedule($Brain1::MeleeTime,meleeHit,%obj);

  %plr.setCoverPosition(5);
  %plr.setPlayerPosition(1);
  %plr.setIsMelee(true);
  %plr.schedule($Brain1::MeleeTime * 2,setIsMelee,false);

  return;
}

//----------------------------------------
// Hunting behavior
//----------------------------------------

// Find nodes that an enemy player, detected at %loc, may hide at, and adds them to %array for analysis
function Brain1::getHidingNodes(%this,%start,%loc,%array)
{
  %plr = %this.player;

  // Figure out the best distance to search for hiding places
  %maxSearchRadius = VectorDist(%start,%loc) * 2;   // The farthest a potential hiding spot could be from %loc, using linear (direct-to) distance

  // Iterate through the AIPaths group (if it exists) and check to see if they could be considered to be hiding spots
  if (!isObject(AIPaths))
    return;
  %count = AIPaths.getCount();
  %nodeList = "";
  for (%i = 0; %i < %count; %i++)
  {
    %node = AIPaths.getObject(%i);
    %distToNode = VectorDist(%loc,%node.getPosition());

    // If the node is close enough
    if (%distToNode <= %maxSearchRadius)
    {
      %rayPlrToNode = ContainerRayCast(%start,%node.getPosition(),$AIPlayer::SolidMasks);
      if (%rayPlrToNode)
      {
        %rayLocToNode = ContainerRayCast(%loc,%node.getPosition(),$Brain1::SolidMasks);
        if (%rayLocToNode)
        {
          // Make sure it can't see a node already on the list
          %listCount = getWordCount(%nodeList);
          if (%listCount < 1)
          {
            // no list, so add this node
            %nodeList = %node;
              continue;
          }
          for (%listIdx = 0;%listIdx < %listCount;%listIdx++)
          {
            %listNode = getWord(%nodeList,%listIdx);
            if (!%listNode)
              break;

            %nodeGood = 1;
            %listNodeRay = ContainerRayCast(%node.getPosition(),%listNode.getPosition(),$Brain1::SolidMasks);
            if (!%listNodeRay) // if another node on the list can see this one, it isn't good
            {
              %nodeGood = 0;
              break;
            }
          }
          if (%nodeGood != 1)
          {
            // Not good, try a different node
            continue;
          }
          else
          {
            // Node is good, so add it to the list and array
            %array.add(%node,getWordCount(%nodeList)-1);
            if (%nodeList $= "")
            {
              echo("Hiding node " @ %node);
              %nodeList = %node;
            }
            else
            {
              %nodeList = %nodeList SPC %node;
            }
          }
        }
      }
    }
  }
  echo(%plr @ ": gethidingnodes()--list: " @ %nodelist);

  // Now we sort the array so that the nodes closest to %loc are searched first
  for(%distSort = 0;%distSort < %array.count();%distSort++)
  {
    %node = %array.getKey(%distSort);
    %distToNode = VectorDist(%loc,%node.getPosition());
    %array.setValue(%distToNode,%distSort);
  }

  // Sometimes it doesn't sort everything when I do one sortA call. So, we do many.
  %array.sortA();
  %array.sortA();
  %array.sortA();
  %array.sortA();
  %array.sortA();

  %array.echo();
  return %nodeList;
}
