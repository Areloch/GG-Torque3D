// Display a splash window immediately to improve app responsiveness before
// engine is initialized and main window created
displaySplashWindow("splash.bmp");

// Console does something.
setLogMode(2);
// Disable script trace.
trace(false);

//-----------------------------------------------------------------------------
// Load up scripts to initialise subsystems.
exec("sys/main.cs");

// If we have editors, initialize them here as well
if(isFile("tools/main.cs"))
	exec("tools/main.cs");

// The canvas needs to be initialized before any gui scripts are run since
// some of the controls assume that the canvas exists at load time.
createCanvas("The Factory");
//Canvas.hideWindow();

// Start rendering and stuff.
initRenderManager();
initLightingSystems("Advanced Lighting"); 

// Start PostFX. If you use "Advanced Lighting" above, uncomment this.
initPostEffects();

// Start audio.
sfxStartup();

// Provide stubs so we don't get console errors. If you actually want to use
// any of these functions, be sure to remove the empty definition here.
function onDatablockObjectReceived() {}
function onGhostAlwaysObjectReceived() {}
function onGhostAlwaysStarted() {}
function updateTSShapeLoadProgress() {}

//-----------------------------------------------------------------------------
// Load console.
exec("sys/console/main.cs");

// Load up game code.
exec("game/gui/mainMenu.gui");
exec("game/main.cs");

exec("tutorials/using_convex_shapes/convex.cs");

exec("art/datablocks/managedDatablocks.cs");

$Game::MissionGroup = "MissionGroup";

// Called when all datablocks from above have been transmitted.
/*function GameConnection::onDataBlocksDone(%client) 
{
   // Start sending ghosts to the client.
   %client.activateGhosting();
   %client.onEnterGame();
}*/

//-----------------------------------------------------------------------------
// Called when the engine is shutting down.
function onExit() 
{
   // Stop file change events.
   stopFileChangeNotifications();
   
   sfxShutdown();

   if($Server::ServerType !$= "")
   {
      // Clean up game objects and so on.
      onEnd();

      // Delete the connection if it's still there.
      //ServerConnection.delete();
      //ServerGroup.delete();

      // Delete all the datablocks.
      //deleteDataBlocks();
   }
   else if(isObject(ServerConnection))
   {
      //We've got a server connection without there having been a server type. This probably means it's the
      //temp mission the editor builds
      MissionGroup.delete();
      ServerConnection.delete();
      ServerGroup.delete();
   }

   echo("Exporting client prefs");
   export("$pref::*", "./prefs.cs", False);

   Parent::onExit();
}

// Called when we connect to the local game.
//StartLevel("game/levels/testLevel.mis", "SinglePlayer");
Canvas.pushDialog(MainMenuGui);

closeSplashWindow();
Canvas.showWindow();