
// The general flow of a gane - server's creation, loading and hosting clients, and then destruction is as follows:

// First, a client will always create a server in the event that they want to host a single player
// game. Torque3D treats even single player connections as a soft multiplayer game, with some stuff
// in the networking short-circuited to sidestep around lag and packet transmission times.

// initServer() is called, loading the default server scripts.
// After that, if this is a dedicated server session, initDedicated() is called, otherwise initClient is called
// to prep a playable client session.

// When a local game is started - a listen server - via calling StartGame() a server is created and then the client is
// connected to it via createAndConnectToLocalServer().

function UI::onCreate( %this )
{
   $PanelSize = 150;
   $PanelSpacing = 40;
}

function UI::onDestroy( %this )
{
}

function UI::initServer(%this){}

function UI::onCreateGameServer(%this){}

function UI::onDestroyGameServer(%this){}

function UI::initClient(%this)
{
   //Load UI stuff
   //we need to load this because some of the menu profiles use the sounds here
   exec("./datablocks/guiSounds.cs");
   
   //Profiles
   exec("./scripts/profiles.cs");
   exec("./scripts/profiles2.cs");
   
   //Now gui files
   exec("./guis/mainMenu.gui");
   exec("./guis/mainMenu.cs");
   
   exec("./guis/startupGui.gui");
   exec("./guis/startupGui.cs");
   
   //Load scripts
   exec("./scripts/messageBoxes.cs");
   exec("./scripts/cursors.cs");
   
   exec("./scripts/assetLibrary.cs");
   exec("./scripts/projectLibrary.cs");
   
   exec("./scripts/fieldTypes.cs");
   
   new Settings(PMSettings) { file = "projectManagerSettings.xml"; };
   PMSettings.read();
   
   //loadStartup();
   Canvas.setContent(MainMenuGui);
}

function UI::onCreateClientConnection(%this){}

function UI::onDestroyClientConnection(%this){}