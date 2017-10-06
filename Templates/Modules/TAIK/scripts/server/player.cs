//-----------------------------------------------------------------------------
// Torque
// Copyright GarageGames, LLC 2011
//-----------------------------------------------------------------------------

// Timeouts for corpse deletion.
$CorpseTimeoutValue = 45 * 1000;

// Death Animations
$PlayerDeathAnim::TorsoFrontFallForward = 1;
$PlayerDeathAnim::TorsoFrontFallBack = 2;
$PlayerDeathAnim::TorsoBackFallForward = 3;
$PlayerDeathAnim::TorsoLeftSpinDeath = 4;
$PlayerDeathAnim::TorsoRightSpinDeath = 5;
$PlayerDeathAnim::LegsLeftGimp = 6;
$PlayerDeathAnim::LegsRightGimp = 7;
$PlayerDeathAnim::TorsoBackFallForward = 8;
$PlayerDeathAnim::HeadFrontDirect = 9;
$PlayerDeathAnim::HeadBackFallForward = 10;
$PlayerDeathAnim::ExplosionBlowBack = 11;

//----------------------------------------------------------------------------
// Armor Datablock methods
//----------------------------------------------------------------------------

function PlayerData::onAdd(%this, %obj)
{
   // Vehicle timeout
   %obj.mountVehicle = true;

   // Default dynamic armor stats
   %obj.setRechargeRate(%this.rechargeRate);
   %obj.setRepairRate(0);
   
   PlayerArray.add(%obj,"");
   PlayerArray.sortKA();

   // Set the numerical Health HUD
   //%obj.updateHealth();

   // Calling updateHealth() must be delayed now... for some reason
   %obj.schedule(50, "updateHealth");
}

function PlayerData::onRemove(%this, %obj)
{
   %objIndex = PlayerArray.getIndexFromKey(%obj);
   PlayerArray.erase(%objIndex);
   PlayerArray.sortKA();
   
   if (%obj.client.player == %obj)
      %obj.client.player = 0;
}

//----------------------------------------------------------------------------





//----------------------------------------------------------------------------

function Player::playDeathAnimation(%this)
{
   //---------------------------------------------------------
   // This change here is to support the TAIK character models
    // and their terrible, awful animations.
   %this.setActionThread("Death1");
   return;
   // --------------------------------------------------------
   
   %numDeathAnimations = %this.getNumDeathAnimations();
   if ( %numDeathAnimations > 0 )
   {
      if (isObject(%this.client))
      {
         if (%this.client.deathIdx++ > %numDeathAnimations)
            %this.client.deathIdx = 1;
         %this.setActionThread("Death" @ %this.client.deathIdx);
      }
      else
      {
         %rand = getRandom(1, %numDeathAnimations);
         %this.setActionThread("Death" @ %rand);
      }
   }
}
