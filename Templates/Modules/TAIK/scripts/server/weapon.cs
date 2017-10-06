//-----------------------------------------------------------------------------
// Torque
// Copyright GarageGames, LLC 2011
//-----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
// A "generic" weaponimage onFire handler for most weapons.  Can be overridden
// with an appropriate namespace method for any weapon that requires a custom
// firing solution.

// projectileSpread is a dynamic property declared in the weaponImage datablock
// for those weapons in which bullet skew is desired.  Must be greater than 0,
// otherwise the projectile goes straight ahead as normal.  lower values give
// greater accuracy, higher values increase the spread pattern.
// ----------------------------------------------------------------------------
function WeaponImage::onFire(%this, %obj, %slot)
{
   //echo("\c4WeaponImage::onFire( "@%this.getName()@", "@%obj.client.nameBase@", "@%slot@" )");

   // Make sure we have valid data
   if (!isObject(%this.projectile))
   {
      error("WeaponImage::onFire() - Invalid projectile datablock");
      return;
   }
   
   // Decrement inventory ammo. The image's ammo state is updated
   // automatically by the ammo inventory hooks.
   if ( !%this.infiniteAmmo )
      %obj.decInventory(%this.ammo, 1);
      
   // Alert AI Players of the shot
   InitContainerRadiusSearch(%obj.getWorldBoxCenter(), %this.fireSoundRadius, $TypeMasks::PlayerObjectType | $TypeMasks::AIPlayerObjectType);
   while((%ai = ContainerSearchNext()) != 0)
   {
      if (%ai.getState() !$= "Dead" && %ai.team != %obj.team)
      {
         %distance = VectorDist(%ai.getPosition(),%obj.getPosition());
         if (%distance <= $AIPlayer::AudFireRange)
         {
            %ai.brain.audFireLocation = %obj.getMuzzlePoint(0);
         }
      }
   }
   AlertAIPlayers(%obj.getWorldBoxCenter(),%this.fireSoundRadius,0,"ShotsFired",2,0,%obj);

   // Get the player's velocity, we'll then add it to that of the projectile
   %objectVelocity = %obj.getVelocity();
   
   %numProjectiles = %this.projectileNum;
   if (%numProjectiles == 0)
      %numProjectiles = 1;
      
   for (%i = 0; %i < %numProjectiles; %i++)
   {
      if (%this.projectileSpread)
      {
         // We'll need to "skew" this projectile a little bit.  We start by
         // getting the straight ahead aiming point of the gun
         %vec = %obj.getMuzzleVector(%slot);

         // Then we'll create a spread matrix by randomly generating x, y, and z
         // points in a circle
         %matrix = "";
         for(%j = 0; %j < 3; %j++)
            %matrix = %matrix @ (getRandom() - 0.5) * 2 * 3.1415926 * %this.projectileSpread @ " ";
         %mat = MatrixCreateFromEuler(%matrix);

         // Which we'll use to alter the projectile's initial vector with
         %muzzleVector = MatrixMulVector(%mat, %vec);
      }
      else
      {
         // Weapon projectile doesn't have a spread factor so we fire it using
         // the straight ahead aiming point of the gun
         %muzzleVector = %obj.getMuzzleVector(%slot);
      }

      // Add player's velocity
      %muzzleVelocity = VectorAdd(
         VectorScale(%muzzleVector, %this.projectile.muzzleVelocity),
         VectorScale(%objectVelocity, %this.projectile.velInheritFactor));

      // Create the projectile object
      %p = new (%this.projectileType)()
      {
         dataBlock = %this.projectile;
         initialVelocity = %muzzleVelocity;
         initialPosition = %obj.getMuzzlePoint(%slot);
         sourceObject = %obj;
         sourceSlot = %slot;
         client = %obj.client;
         sourceClass = %obj.getClassName();
      };
      MissionCleanup.add(%p);
   }
}
