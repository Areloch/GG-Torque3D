//-----------------------------------------------------------------------------
// Copyright (c) 2012 GarageGames, LLC
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// What kind of "player" is spawned is either controlled directly by the
// SpawnSphere or it defaults back to the values set here. This also controls
// which SimGroups to attempt to select the spawn sphere's from by walking down
// the list of SpawnGroups till it finds a valid spawn object.
//-----------------------------------------------------------------------------

// Leave $Game::defaultPlayerClass and $Game::defaultPlayerDataBlock as empty strings ("")
// to spawn a the $Game::defaultCameraClass as the control object.
$Game::DefaultPlayerClass        = "Player";
$Game::DefaultPlayerDataBlock    = "DefaultPlayerData";
$Game::DefaultPlayerSpawnGroups  = "PlayerSpawnPoints";

//-----------------------------------------------------------------------------
// What kind of "camera" is spawned is either controlled directly by the
// SpawnSphere or it defaults back to the values set here. This also controls
// which SimGroups to attempt to select the spawn sphere's from by walking down
// the list of SpawnGroups till it finds a valid spawn object.
//-----------------------------------------------------------------------------
$Game::DefaultCameraClass        = "Camera";
$Game::DefaultCameraDataBlock    = "Observer";
$Game::DefaultCameraSpawnGroups  = "CameraSpawnPoints PlayerSpawnPoints";

//-----------------------------------------------------------------------------
// pickCameraSpawnPoint() is responsible for finding a valid spawn point for a
// camera.
//-----------------------------------------------------------------------------
function pickCameraSpawnPoint(%spawnGroups)
{
   // Walk through the groups until we find a valid object
   for (%i = 0; %i < getWordCount(%spawnGroups); %i++)
   {
      %group = getWord(%spawnGroups, %i);
      
      %count = getWordCount(%group);

      if (isObject(%group))
         %spawnPoint = %group.getRandom();

      if (isObject(%spawnPoint))
         return %spawnPoint;
   }

   // Didn't find a spawn point by looking for the groups
   // so let's return the "default" SpawnSphere
   // First create it if it doesn't already exist
   if (!isObject(DefaultCameraSpawnSphere))
   {
      %spawn = new SpawnSphere(DefaultCameraSpawnSphere)
      {
         dataBlock      = "SpawnSphereMarker";
         spawnClass     = $Game::DefaultCameraClass;
         spawnDatablock = $Game::DefaultCameraDataBlock;
      };

      // Add it to the MissionCleanup group so that it
      // doesn't get saved to the Mission (and gets cleaned
      // up of course)
      MissionCleanup.add(%spawn);
   }

   return DefaultCameraSpawnSphere;
}

function GameConnection::spawnCamera(%this, %spawnPoint)
{
   // Set the control object to the default camera
   if (!isObject(%this.camera))
   {
      if (isDefined("$Game::DefaultCameraClass"))
         %this.camera = spawnObject($Game::DefaultCameraClass, $Game::DefaultCameraDataBlock);
   }

   // If we have a camera then set up some properties
   if (isObject(%this.camera))
   {
      MissionCleanup.add( %this.camera );
      %this.camera.scopeToClient(%this);

      %this.setControlObject(%this.camera);

      if (isDefined("%spawnPoint"))
      {
         // Attempt to treat %spawnPoint as an object
         if (getWordCount(%spawnPoint) == 1 && isObject(%spawnPoint))
         {
            %this.camera.setTransform(%spawnPoint.getTransform());
         }
         else
         {
            // Treat %spawnPoint as an AxisAngle transform
            %this.camera.setTransform(%spawnPoint);
         }
      }
   }
}