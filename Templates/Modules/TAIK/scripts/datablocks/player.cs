// ----------------------------------------------------------------------------
// This is our default player datablock that all others will derive from.
// ----------------------------------------------------------------------------

datablock PlayerData(DefaultPlayerData)
{
   renderFirstPerson = false;

   computeCRC = false;

   // Third person shape
   // shapeFile = "art/shapes/actors/Soldier/soldier_rigged.dae";
   shapeFile = "data/TAIK/art/shapes/TAIKExamplePlayers/Specops/Specops.dts"; // TAIK, optional
   cameraMaxDist = 3;
   allowImageStateAnimation = true;

   // First person arms
   imageAnimPrefixFP = "soldier";
   shapeNameFP[0] = "data/FPSGameplay/art/shapes/actors/Soldier/FP/FP_SoldierArms.DAE";

   canObserve = 1;
   cmdCategory = "Clients";

   cameraDefaultFov = 55.0;
   cameraMinFov = 5.0;
   cameraMaxFov = 65.0;

   debrisShapeName = "data/FPSGameplay/art/shapes/actors/common/debris_player.dts";
   debris = playerDebris;
   
   throwForce = 30;

   aiAvoidThis = 1;

   minLookAngle = "-1.4";
   maxLookAngle = "0.9";
   maxFreelookAngle = 3.0;

   mass = 120;
   drag = 1.3;
   maxdrag = 0.4;
   density = 1.1;
   maxDamage = 100;
   maxEnergy =  60;
   repairRate = 0.33;
   energyPerDamagePoint = 75;

   rechargeRate = 0.256;

   runForce = 4320;
   runEnergyDrain = 0;
   minRunEnergy = 0;
   maxForwardSpeed = 8;
   maxBackwardSpeed = 6;
   maxSideSpeed = 6;

   sprintForce = 4320;
   sprintEnergyDrain = 0;
   minSprintEnergy = 0;
   maxSprintForwardSpeed = 14;
   maxSprintBackwardSpeed = 8;
   maxSprintSideSpeed = 6;
   sprintStrafeScale = 0.25;
   sprintYawScale = 0.05;
   sprintPitchScale = 0.05;
   sprintCanJump = true;

   crouchForce = 405;
   maxCrouchForwardSpeed = 4.0;
   maxCrouchBackwardSpeed = 2.0;
   maxCrouchSideSpeed = 2.0;

   maxUnderwaterForwardSpeed = 8.4;
   maxUnderwaterBackwardSpeed = 7.8;
   maxUnderwaterSideSpeed = 4.0;

   jumpForce = "747";
   jumpEnergyDrain = 0;
   minJumpEnergy = 0;
   jumpDelay = "15";
   airControl = 0.3;

   fallingSpeedThreshold = -6.0;

   landSequenceTime = 0.33;
   transitionToLand = false;
   recoverDelay = 0;
   recoverRunForceScale = 0;

   minImpactSpeed = 10;
   minLateralImpactSpeed = 20;
   speedDamageScale = 0.4;

   boundingBox = "0.7 0.7 2.275"; // TAIK, optional
   crouchBoundingBox = "0.7 0.7 1.14"; // TAIK, optional
   //boundingBox = "0.65 0.75 1.85";
   //crouchBoundingBox = "0.65 0.75 1.3";
   swimBoundingBox = "1 2 2";
   pickupRadius = 1;

   // Damage location details
   boxHeadPercentage       = 0.83;
   boxTorsoPercentage      = 0.49;
   boxHeadLeftPercentage         = 0.30;
   boxHeadRightPercentage        = 0.60;
   boxHeadBackPercentage         = 0.30;
   boxHeadFrontPercentage        = 0.60;

   // Foot Prints
   decalOffset = 0.25;

   footPuffEmitter = "LightPuffEmitter";
   footPuffNumParts = 10;
   footPuffRadius = "0.25";

   dustEmitter = "LightPuffEmitter";

   splash = PlayerSplash;
   splashVelocity = 4.0;
   splashAngle = 67.0;
   splashFreqMod = 300.0;
   splashVelEpsilon = 0.60;
   bubbleEmitTime = 0.4;
   splashEmitter[0] = PlayerWakeEmitter;
   splashEmitter[1] = PlayerFoamEmitter;
   splashEmitter[2] = PlayerBubbleEmitter;
   mediumSplashSoundVelocity = 10.0;
   hardSplashSoundVelocity = 20.0;
   exitSplashSoundVelocity = 5.0;

   // Controls over slope of runnable/jumpable surfaces
   runSurfaceAngle  = 38;
   jumpSurfaceAngle = 80;
   maxStepHeight = 0.35;  //two meters
   minJumpSpeed = 20;
   maxJumpSpeed = 30;

   horizMaxSpeed = 68;
   horizResistSpeed = 33;
   horizResistFactor = 0.35;

   upMaxSpeed = 80;
   upResistSpeed = 25;
   upResistFactor = 0.3;

   footstepSplashHeight = 0.35;

   //NOTE:  some sounds commented out until wav's are available

   // Footstep Sounds
   FootSoftSound        = FootLightSoftSound;
   FootHardSound        = FootLightHardSound;
   FootMetalSound       = FootLightMetalSound;
   FootSnowSound        = FootLightSnowSound;
   FootShallowSound     = FootLightShallowSplashSound;
   FootWadingSound      = FootLightWadingSound;
   FootUnderwaterSound  = FootLightUnderwaterSound;

   //FootBubblesSound     = FootLightBubblesSound;
   //movingBubblesSound   = ArmorMoveBubblesSound;
   //waterBreathSound     = WaterBreathMaleSound;

   //impactSoftSound      = ImpactLightSoftSound;
   //impactHardSound      = ImpactLightHardSound;
   //impactMetalSound     = ImpactLightMetalSound;
   //impactSnowSound      = ImpactLightSnowSound;

   //impactWaterEasy      = ImpactLightWaterEasySound;
   //impactWaterMedium    = ImpactLightWaterMediumSound;
   //impactWaterHard      = ImpactLightWaterHardSound;

   groundImpactMinSpeed    = "45";
   groundImpactShakeFreq   = "4.0 4.0 4.0";
   groundImpactShakeAmp    = "1.0 1.0 1.0";
   groundImpactShakeDuration = 0.8;
   groundImpactShakeFalloff = 10.0;

   //exitingWater         = ExitingWaterLightSound;

   observeParameters = "0.5 4.5 4.5";
   
   camoColor = "149 134 98"; 

   cameraMinDist = "0";
   DecalData = "PlayerFootprint";

   // Allowable Inventory Items
   mainWeapon = Lurker;

   maxInv[Lurker] = 1;
   maxInv[LurkerClip] = 20;

   maxInv[LurkerGrenadeLauncher] = 1;
   maxInv[LurkerGrenadeAmmo] = 20;
   
   maxInv[TaikGun] = "0";
   maxInv[TaikGunAmmo] = "1000";

   maxInv[Ryder] = 1;
   maxInv[RyderClip] = 10;

   maxInv[ProxMine] = 5;

   maxInv[DeployableTurret] = 5;

   // available skins (see materials.cs in model folder)
   availableSkins =  "base	DarkBlue	DarkGreen	LightGreen	Orange	Red	Teal	Violet	Yellow";
};
