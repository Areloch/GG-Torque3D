
// The general flow of a gane - server's creation, loading and hosting clients, and then destruction is as follows:

// First, a client will always create a server in the event that they want to host a single player
// game. Torque3D treats even single player connections as a soft multiplayer game, with some stuff
// in the networking short-circuited to sidestep around lag and packet transmission times.

// initServer() is called, loading the default server scripts.
// After that, if this is a dedicated server session, initDedicated() is called, otherwise initClient is called
// to prep a playable client session.

// When a local game is started - a listen server - via calling StartGame() a server is created and then the client is
// connected to it via createAndConnectToLocalServer().

function TAIK::create( %this )
{
   //add DBs
   if(isObject(DatablockFilesList))
   {
      DatablockFilesList.add( "data/TAIK/scripts/datablocks/markers.cs" );
      DatablockFilesList.add( "data/TAIK/scripts/datablocks/Nodes.cs" );
      DatablockFilesList.add( "data/TAIK/scripts/datablocks/Spawn.cs" );
      DatablockFilesList.add( "data/TAIK/scripts/datablocks/player.cs" );
      DatablockFilesList.add( "data/TAIK/scripts/datablocks/AIDatablocks.cs" );
      DatablockFilesList.add( "data/TAIK/scripts/datablocks/weapons/taikGun.cs" );
   }
   
   if(isObject(LevelFilesList))
   {
      for( %file = findFirstFile( "data/TAIK/levels/*.mis" );
      %file !$= "";
      %file = findNextFile( "data/TAIK/levels/*.mis" ))
      {
         LevelFilesList.add(%file);
      }
   }
   
   //server scripts
   exec("./scripts/server/commands.cs");
   exec("./scripts/server/gameCore.cs");
   exec("./scripts/server/health.cs");
   exec("./scripts/server/player.cs");
   exec("./scripts/server/projectile.cs");
   exec("./scripts/server/weapon.cs");
   
   //tools
   //exec(".tools/worldEditor/scripts/menuHandlers.cs");
   //exec(".tools/worldEditor/scripts/menus.ed.cs");
   
   //creator additions
   EWCreatorWindow.beginGroup( "System" );
   EWCreatorWindow.registerMissionObject( "AIPathGroup" );
   EWCreatorWindow.endGroup( "System" );

   exec(".tools/worldEditor/scripts/editors/worldEditor.ed.cs");
   
   // AI scripts
   exec("./scripts/server/ai/aiplayer/aiplayer.cs");
   execAI(); // This function, contained in ai/aiPlayer/aiPlayer.cs, exec()'s the rest of the AI files
   
   if (!$Server::Dedicated)
   {
      exec("./scripts/client/default.bind.cs");
      
      exec("./art/gui/taik/taikGui.gui");
      exec("./scripts/client/weapon.cs");
   }
}

function TAIK::destroy( %this )
{
   
}