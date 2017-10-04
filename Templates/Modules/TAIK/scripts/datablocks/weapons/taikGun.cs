// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.

// TAIKGun weapon.
// Pretty super.
// But not really. Use it as an example.

// ----------------------------------------------------------------------------
// Sound profiles
// ----------------------------------------------------------------------------

datablock SFXProfile(TAIKGunFireSound)
{
   filename = "data/TAIK/art/sound/weapons/taikgun.ogg";
   description = AudioClose3d;
   preload = true;
};
datablock SFXProfile(TAIKBulletSound)
{
   filename = "data/TAIK/art/sound/weapons/taikbullet.ogg";
   description = AudioClosest3d;
   preload = true;
};

datablock SFXProfile(BulletFireSound)
{
   filename = "data/TAIK/art/sound/weapons/taikBullet";
   description = BulletImpactDesc;
   preload = true;
};

// ----------------------------------------------------------------------------
// Particles
// ----------------------------------------------------------------------------

datablock ParticleData(TaikGunImpactSmallSmoke) {
   canSaveDynamicFields = "1";
   dragCoefficient = "0";
   windCoefficient = "0";
   gravityCoefficient = "0";
   inheritedVelFactor = "0";
   constantAcceleration = "-0.4";
   lifetimeMS = "4000";
   lifetimeVarianceMS = "2200";
   spinSpeed = "0";
   spinRandomMin = "-100";
   spinRandomMax = "180";
   useInvAlpha = "1";
   animateTexture = "0";
   framesPerSec = "1";
   textureName = "data/FPSGameplay/art/particles/smoke";
   animTexName[0] = "data/FPSGameplay/art/particles/smoke";
   colors[0] = "0.826772 0.826772 0.826772 0.1";
   colors[1] = "0.826772 0.826772 0.826772 0.05";
   colors[2] = "0.826772 0.826772 0.826772 0";
   colors[3] = "0 0 0 0";
   sizes[0] = "0.18";
   sizes[1] = "0.29";
   sizes[2] = "0.4";
   sizes[3] = "0";
   times[0] = "0";
   times[1] = "0.5";
   times[2] = "1";
   times[3] = "1";
      dragCoeffiecient = "100";
};
datablock ParticleEmitterData(TaikGunImpactSmallSmokeEmitter) {
   canSaveDynamicFields = "1";
   className = "ParticleEmitterData";
   ejectionPeriodMS = "4";
   periodVarianceMS = "0";
   ejectionVelocity = "0.2";
   velocityVariance = "0.19";
   ejectionOffset = "0.2";
   thetaMin = "0";
   thetaMax = "61.2";
   phiReferenceVel = "0";
   phiVariance = "360";
   overrideAdvance = "0";
   orientParticles = "0";
   orientOnVelocity = "1";
   particles = "TaikGunImpactSmallSmoke";
   lifetimeMS = "32";
   lifetimeVarianceMS = "0";
   useEmitterSizes = "0";
   useEmitterColors = "0";
};

datablock ParticleData(TaikGunImpactSmoke) {
   canSaveDynamicFields = "1";
   dragCoefficient = "0";
   windCoefficient = "0";
   gravityCoefficient = "0";
   inheritedVelFactor = "0";
   constantAcceleration = "-0.2";
   lifetimeMS = "530";
   lifetimeVarianceMS = "80";
   spinSpeed = "1";
   spinRandomMin = "10";
   spinRandomMax = "390";
   useInvAlpha = "1";
   animateTexture = "0";
   framesPerSec = "1";
   textureName = "data/FPSGameplay/art/particles/smoke";
   animTexName[0] = "data/FPSGameplay/art/particles/smoke";
   colors[0] = "0.826772 0.826772 0.826772 0.1";
   colors[1] = "0.826772 0.826772 0.826772 0.05";
   colors[2] = "0.826772 0.826772 0.826772 0";
   colors[3] = "0 0 0 0";
   sizes[0] = "0.2";
   sizes[1] = "0.47";
   sizes[2] = "0.74";
   sizes[3] = "0";
   times[0] = "0";
   times[1] = "0.5";
   times[2] = "1";
   times[3] = "1";
      dragCoeffiecient = "100";
};
datablock ParticleEmitterData(TaikGunImpactSmokeEmitter) {
   canSaveDynamicFields = "1";
   className = "ParticleEmitterData";
   ejectionPeriodMS = "8";
   periodVarianceMS = "0";
   ejectionVelocity = "1.6";
   velocityVariance = "0";
   ejectionOffset = "0.1";
   thetaMin = "0";
   thetaMax = "5";
   phiReferenceVel = "0";
   phiVariance = "147.6";
   overrideAdvance = "0";
   orientParticles = "0";
   orientOnVelocity = "1";
   particles = "TaikGunImpactSmoke";
   lifetimeMS = "32";
   lifetimeVarianceMS = "0";
   useEmitterSizes = "0";
   useEmitterColors = "0";
};

datablock ExplosionData(TaikGunExplosion)
{
	 soundProfile = TAIKBulletSound;
   lifetime = 2000;
   offset = 0;
   emitter[0] = TaikGunImpactSmokeEmitter;
   emitter[1] = TaikGunImpactSmallSmokeEmitter;
   
   // Camera Shaking
   shakeCamera = true;
   camShakeFreq = "20.0 21.0 20.0"; // 10 11 10
   camShakeAmp = "3.0 3.0 3.0"; // 1 1 1
   camShakeDuration = 0.5;
   camShakeRadius = 4.0;
};

// ----------------------------------------------------------------------------
// Projectile
// ----------------------------------------------------------------------------

datablock ProjectileData(TAIKGunProjectile)
{
   projectileShapeName = "data/TAIK/art/shapes/weapons/taikGun/rocket.dts";
   directDamage = 40;
   radiusDamage = 0;
   damageRadius = 0;
   areaImpulse = 0;

   explosion = TAIKGunExplosion;

   //particleEmitter = RocketProjSmokeTrailEmitter;
   //particleWaterEmitter = RocketTrailWaterEmitter;

   muzzleVelocity = 600;
   velInheritFactor = 0.3;

   armingDelay = 0;
   lifetime = 1500;
   fadeDelay = 200;

   bounceElasticity = 0;
   bounceFriction = 0;
   isBallistic = false;
   gravityMod = 0.80;

   damageType = "TAIKGunDamage";
};

// ----------------------------------------------------------------------------
// Ammo item
// ----------------------------------------------------------------------------

datablock ItemData(TAIKGunAmmo)
{
   // Mission editor category
   category = "Ammo";

   // Add the Ammo namespace as a parent. The ammo namespace provides
   // common ammo related functions and hooks into the inventory system.
   className = "Ammo";

   // Basic Item properties
   shapeFile = "data/FPSGameplay/art/shapes/weapons/Lurker/TP_Lurker.DAE";
   mass = 2;
   elasticity = 0.2;
   friction = 0.6;

   // Dynamic properties defined by the scripts
   pickUpName = "Rockets";
   maxInventory = 20;
};

// ----------------------------------------------------------------------------
// Weapon Item. This is the item that exists in the world,
// i.e. when it's been dropped, thrown or is acting as re-spawnable item.
// When the weapon is mounted onto a shape, the Image is used.
// ----------------------------------------------------------------------------

datablock ItemData(TAIKGun)
{
   // Mission editor category
   category = "Weapon";

   // Hook into Item Weapon class hierarchy. The weapon namespace
   // provides common weapon handling functions in addition to hooks
   // into the inventory system.
   className = "Weapon";

   // Basic Item properties
   shapefile = "data/TAIK/art/shapes/weapons/taikGun/taikGun.dts";
   mass = 5;
   elasticity = 0.2;
   friction = 0.6;
   emap = true;

   // Dynamic properties defined by the scripts
   pickUpName = "SwarmGun";
   description = "TAIKGun";
   image = TAIKGunImage;

   // weaponHUD
   previewImage = 'swarmer.png';
   reticle = 'reticle_TAIKGun';
};

// ----------------------------------------------------------------------------
// Image which does all the work. Images do not normally exist in
// the world, they can only be mounted on ShapeBase objects.
// ----------------------------------------------------------------------------

datablock ShapeBaseImageData(TAIKGunImage)
{
   // Basic Item properties
   shapefile = "data/TAIK/art/shapes/weapons/taikGun/taikGun.dts";
   emap = true;

   // Specify mount point & offset for 3rd person, and eye offset
   // for first person rendering.
   mountPoint = 0;
   eyeOffset = "0.1 0.32 -0.16";

   // When firing from a point offset from the eye, muzzle correction
   // will adjust the muzzle vector to point to the eye LOS point.
   // Since this weapon doesn't actually fire from the muzzle point,
   // we need to turn this off.
   correctMuzzleVector = false;

   // Add the WeaponImage namespace as a parent, WeaponImage namespace
   // provides some hooks into the inventory system.
   className = "WeaponImage";

   // Projectile && Ammo.
   item = TAIKGun;
   ammo = TAIKGunAmmo;
   projectile = TAIKGunProjectile;
   wetProjectile = RocketWetProjectile;
   projectileType = Projectile;
   
   clipsize = 30;
   fireSoundRadius = 400;
   reloadTime = 3000;
   usePreciseAim = false;
   useRadiusDamage = 0;

   // Images have a state system which controls how the animations
   // are run, which sounds are played, script callbacks, etc. This
   // state system is downloaded to the client so that clients can
   // predict state changes and animate accordingly. The following
   // system supports basic ready->fire->reload transitions as
   // well as a no-ammo->dryfire idle state.

   // Initial start up state
   stateName[0] = "Preactivate";
   stateTransitionOnLoaded[0] = "Activate";
   stateTransitionOnNoAmmo[0] = "NoAmmo";

   // Activating the gun.
   // Called when the weapon is first mounted and there is ammo.
   stateName[1] = "Activate";
   stateTransitionOnTimeout[1] = "Ready";
   stateTimeoutValue[1] = 0.3;

   // Ready to fire, just waiting for the trigger
   stateName[2] = "Ready";
   stateTransitionOnNoAmmo[2] = "NoAmmo";
   stateTransitionOnTriggerDown[2] = "Fire";

   // Fire the weapon. Calls the fire script which does the actual work.
   stateName[3] = "Fire";
   stateTransitionOnTimeout[3] = "Reload";
   stateTimeoutValue[3] = 0.025;
   stateFire[3] = true;
   stateRecoil[3] = LightRecoil;
   stateAllowImageChange[3] = false;
   stateScript[3] = "onFire";
   stateSound[3] = TAIKGunFireSound;

   // Play the reload animation, and transition into
   stateName[5] = "Reload";
   stateTransitionOnTimeout[5] = "Ready";
   stateTimeoutValue[5] = 0.05;
   stateAllowImageChange[5] = false;

   // No ammo in the weapon, just idle until something shows up.
   // Play the dry fire sound if the trigger iS pulled.
   stateName[6] = "NoAmmo";
   stateTransitionOnAmmo[6] = "Reload";
   stateTransitionOnTriggerDown[6] = "DryFire";

   // No ammo dry fire
   stateName[7] = "DryFire";
   stateTimeoutValue[7] = 1.0;
   stateTransitionOnTimeout[7] = "NoAmmo";
   stateSound[7] = TAIKGunFireEmptySound;
};
