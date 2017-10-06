// Added this stage to creating a player so game types can override it easily.
// This is a good place to initiate team selection.
function DeathMatchGame::preparePlayer(%this, %client)
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
   %this.spawnPlayer(%client, %playerSpawnPoint);

   // Starting equipment
   %this.loadOut(%client.player);
   
   //Make sure the team is set
   %client.player.team = 1;
}