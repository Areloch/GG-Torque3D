// Added this stage to creating a player so game types can override it easily.
// This is a good place to initiate team selection.
function DeathMatchGame::preparePlayer(%game, %client)
{
   //echo (%game @"\c4 -> "@ %game.class @" -> GameCore::preparePlayer");

   // Find a spawn point for the player
   // This function currently relies on some helper functions defined in
   // core/scripts/spawn.cs. For custom spawn behaviors one can either
   // override the properties on the SpawnSphere's or directly override the
   // functions themselves.
   %playerSpawnPoint = pickPlayerSpawnPoint($Game::DefaultPlayerSpawnGroups);
   // Spawn a camera for this client using the found %spawnPoint
   //%client.spawnPlayer(%playerSpawnPoint);
   %game.spawnPlayer(%client, %playerSpawnPoint);

   // Starting equipment
   %game.loadOut(%client.player);
   %client.player.team = 1;
}

function DeathMatchGame::onDeath(%game, %client, %sourceObject, %sourceClient, %damageType, %damLoc)
{
   //echo (%game @"\c4 -> "@ %game.class @" -> GameCore::onDeath");
   
   // clear the weaponHUD
   %client.RefreshWeaponHud(0, "", "");

   // Clear out the name on the corpse
   %client.player.setShapeName("");

   // Update the numerical Health HUD
   %client.player.updateHealth();

   // Switch the client over to the death cam and unhook the player object.
   if (isObject(%client.camera) && isObject(%client.player))
   {
      %client.camera.setMode("Corpse", %client.player);
      %client.setControlObject(%client.camera);
   }
   %client.player = 0;

   // Display damage appropriate kill message
   %sendMsgFunction = "sendMsgClientKilled_" @ %damageType;
   if ( !isFunction( %sendMsgFunction ) )
      %sendMsgFunction = "sendMsgClientKilled_Default";
   call( %sendMsgFunction, 'MsgClientKilled', %client, %sourceClient, %damLoc );

   // Dole out points and check for win
   if ( %damageType $= "Suicide" || %sourceClient == %client && isObject(%sourceClient))
   {
      %game.incDeaths( %client, 1, true );
      %game.incScore( %client, -1, false );
   }
   else
   {
      %game.incDeaths( %client, 1, false );
      %game.incScore( %sourceClient, 1, true );
      %game.incKills( %sourceClient, 1, false );

      // If the game may be ended by a client getting a particular score, check that now.
      if ( $Game::EndGameScore > 0 && %sourceClient.kills >= $Game::EndGameScore )
         %game.cycleGame();
   }
}