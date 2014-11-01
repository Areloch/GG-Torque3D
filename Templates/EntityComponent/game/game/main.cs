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
// Load up our main GUI which lets us see the game.
exec("./gui/scripts/profiles.cs");
exec("./gui/playGui.gui");
exec("./gui/chooseLevelDlg.gui");
exec("./gui/loadingGUI.gui");
exec("./gui/joinServerDlg.gui");
exec("./gui/optionsDlg.gui");
exec("./gui/messageBoxOK.ed.gui");
exec("./gui/messageBoxYesNo.ed.gui");

exec("./behaviors/init.cs");

exec("./levelInfo.cs");
exec("./server.cs");
exec("./connection.cs");
exec("./levelLoad.cs");

if ( isFile( "../prefs.cs" ) )
    exec( "../prefs.cs" );
	
// Initialize the post effect manager.
exec("sys/postFx/postFXManager.gui");
exec("sys/postFx/postFXManager.gui.cs");
exec("sys/postFx/postFXManager.gui.settings.cs");
exec("sys/postFx/postFXManager.persistance.cs");

//-------------------------'aight ----------------------------------------------------
// Specify where the mission files are.
$Server::MissionFileSpec = "game/levels/*.mis";

//-----------------------------------------------------------------------------
// Create a datablock for the observer camera.
//datablock CameraData(Observer) {};

//-----------------------------------------------------------------------------
// And a material to give the ground some colour (even if it's just white).
singleton Material(BlankWhite) {
   diffuseColor[0] = "1 1 1";
};

//-----------------------------------------------------------------------------
// Called when all datablocks have been transmitted.
function GameConnection::onEnterGame(%client) 
{
   echo("Prepping the behaviors on the client");

   //First, parse for any prefabs that contain entities
   %prefabs = parseMissionGroupForIds( "Prefab", "" );
   for(%i=0; %i < getWordCount(%prefabs); %i++)
   {
      %prefab = getWord(%prefabs, %i);
	
	  %prefabCnt = %prefab.getCount();
	  for(%p=0; %p < %prefabCnt; %p++)
	  {
	     %prefabObj = %prefab.getObject(%i);
		 if(%prefabObj.getClassName() $= "Entity")
		 {
		    if(%i==0)
			   %peIDs = %prefabObj.getID();
			else
			   %peIDs = %peIDs SPC %prefabObj.getID();
	     }	
	  }
   }
   
   %entityIDs = parseMissionGroupForIds( "Entity", "" );
   %entityIDs = %entityIDs SPC %peIDs;
   for(%i=0; %i < getWordCount(%entityIDs); %i++)
   {
      %entity = getWord(%entityIDs, %i);
	
	  //check if this entity has any children, and if so add them to our list
	  for(%e = 0; %e < %entity.getCount(); %e++)
	  {
		  %child = %entity.getObject(%e);
		  if(%child.getClassName() == "Entity")
		  	 %entityIDs = %entityIDs SPC %child.getID();
	  }	  

      for(%b = 0; %b < %entity.getBehaviorCount(); %b++)
      {
         %behav = %entity.getBehaviorByIndex(%b);
         
         //%behav.ghostToClient(%client);
         
		   if(%behav.isMethod("onClientConnect"))
            %behav.onClientConnect(%client);
      }
      //finally, we do a update push on our owner to make sure it ghosts over the behaviors it owns to the client
      //This is critical for client-side behaviors like the camera or rendering
      //%entity.schedule(500, "updateBehaviors"); 
   }
   
   //TODO: remove
   /*new Entity(EditorCamera)
   {
      _behavior0 = "CameraBehav";
      _behavior1 = "ControlObjectBehavior";
      _behavior2 = "EditorControls";
   };
   for(%b = 0; %b < EditorCamera.getBehaviorCount(); %b++)
   {
      %behav = EditorCamera.getBehaviorByIndex(%b);
      
      %behav.ghostToClient(%client);
   }
   EditorCamera.schedule(500, "updateBehaviors"); 
   MissionCleanup.add(EditorCamera);*/
   
   /*for(%b = 0; %b < TheCamera.getBehaviorCount(); %b++)
   {
      %behav = TheCamera.getBehaviorByIndex(%b);
      
      %behav.ghostToClient(%client);
      
      if(%behav.isMethod("onClientConnect"))
         %behav.onClientConnect(%client);
   }
   TheCamera.schedule(500, "updateBehaviors"); */
   
   //%client.camera = TheCamera;
   
   // Create a camera for the client.
   /*new Camera(TheCamera) {
      datablock = Observer;
   };*/
   //TheCamera.setTransform("0 0 2 1 0 0 0");
   // Cameras are not ghosted (sent across the network) by default; we need to
   // do it manually for the client that owns the camera or things will go south
   // quickly.
   //TheCamera.scopeToClient(%client);
   // And let the client control the camera.
   //%client.setControlObject(TheCamera);
   // Add the camera to the group of game objects so that it's cleaned up when
   // we close the game.
   //MissionGroup.add(TheCamera);
   // Activate HUD which allows us to see the game. This should technically be
   // a commandToClient, but since the client and server are on the same
   // machine...
   Canvas.setContent(PlayGui);
   activateDirectInput();
   
   GlobalActionMap.bind("keyboard", "escape", "escapeFromGame");
   
   if(!$Game::running)
      $Game::running = true;
}

function escapeFromGame()
{
   /*if ( $Server::ServerType $= "SinglePlayer" )
      MessageBoxYesNo( "Exit", "Exit from this Mission?", "disconnect();", "");
   else
      MessageBoxYesNo( "Disconnect", "Disconnect from the server?", "disconnect();", "");*/
   disconnect();
}

//-----------------------------------------------------------------------------
// Called when the engine has been initialised.
function onGameStart() {
  
   // Mission cleanup group.  This is where run time objects will reside.  The MissionCleanup
   // group will be added to the ServerGroup.
   new SimGroup( MissionCleanup );

   // Make the MissionCleanup group the place where all new objects will automatically be added.
   $instantGroup = MissionCleanup;
   
   // Create objects!
   new SimGroup(MissionGroup) {
      new LevelInfo(TheLevelInfo) {
         canvasClearColor = "0 0 0";
      };
      new GroundPlane(TheGround) {
         position = "0 0 0";
         material = BlankWhite;
      };
      new Sun(TheSun) {
         azimuth = 230;
         elevation = 45;
         color = "1 1 1";
         ambient = "0.1 0.1 0.1";
         castShadows = true;
      };
      
      new Entity(TheCamera)
      {
         _behavior0 = "CameraBehav";
         _behavior1 = "ControlObjectBehavior";
         _behavior2 = "SpectatorControls";
      };
   };

   // Allow us to exit the game...
   GlobalActionMap.bind("keyboard", "escape", "quit");
}

//-----------------------------------------------------------------------------
// Called when the engine is shutting down.
function onEnd() {
   
   // Delete the objects we created.
   //MissionGroup.delete();
   
   // Ensure that we are disconnected and/or the server is destroyed.
   // This prevents crashes due to the SceneGraph being deleted before
   // the objects it contains.
   if ($Server::Dedicated)
      destroyServer();
   else
      disconnect();
   
   // Destroy the physics plugin.
   physicsDestroy();
      
   /*echo("Exporting client prefs");
   export("$pref::*", "./client/prefs.cs", False);

   echo("Exporting server prefs");
   export("$Pref::Server::*", "./server/prefs.cs", False);
   BanList::Export("./server/banlist.cs");*/
   
   deleteDataBlocks();
}

//--------------------------------------------------------------
// Materials
//--------------------------------------------------------------
function loadMaterials()
{
	//load any sys materials
	if( isFile( "sys/images/materials.cs" ) )
         exec( "sys/images/materials.cs" );
		
   // Load any materials files for which we only have DSOs.

   for( %file = findFirstFile( "game/materials.cs.dso" );
        %file !$= "";
        %file = findNextFile( "game/materials.cs.dso" ))
   {
      // Only execute, if we don't have the source file.
      %csFileName = getSubStr( %file, 0, strlen( %file ) - 4 );
      if( !isFile( %csFileName ) )
         exec( %csFileName );
   }

   // Load all source material files.

   for( %file = findFirstFile( "game/materials.cs" );
        %file !$= "";
        %file = findNextFile( "game/materials.cs" ))
   {
      exec( %file );
   }
}

function reloadMaterials()
{
   reloadTextures();
   loadMaterials();
   reInitMaterials();
}