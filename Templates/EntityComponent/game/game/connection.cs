// Whether the local client is currently running a mission.
$Client::missionRunning = false;

// Sequence number for currently running mission.
$Client::missionSeq = -1;

addMessageCallback( 'MsgConnectionError', handleConnectionErrorMessage );

function handleConnectionErrorMessage(%msgType, %msgString, %msgError)
{
   // On connect the server transmits a message to display if there
   // are any problems with the connection.  Most connection errors
   // are game version differences, so hopefully the server message
   // will tell us where to get the latest version of the game.
   $ServerConnectionErrorMessage = %msgError;
}

//----------------------------------------------------------------------------
// GameConnection client callbacks
//----------------------------------------------------------------------------
function GameConnection::onConnect(%client) 
{
   //%client.transmitDataBlocks(0);
   //loadMaterials();
 
   // Send down the connection error info, the client is responsible for
	// displaying this message if a connection error occurs.
	messageClient(%client, 'MsgConnectionError',"",$Pref::Server::ConnectionError);
	
	// Send mission information to the client
	sendLoadInfoToClient(%client);
	
	// Simulated client lag for testing...
	// %client.setSimulatedNetParams(0.1, 30);
	
	// Get the client's unique id:
	// %authInfo = %client.getAuthInfo();
	// %client.guid = getField(%authInfo, 3);
	%client.guid = 0;
	addToServerGuidList(%client.guid);
	
	// Set admin status
	if (%client.getAddress() $= "local")
	{
		%client.isAdmin = true;
		%client.isSuperAdmin = true;
	}
	else
	{
		%client.isAdmin = false;
		%client.isSuperAdmin = false;
	}
	
	echo("CADD: "@ %client @" "@ %client.getAddress());

	// If the mission is running, go ahead download it to the client
	if ($missionRunning)
	{
		%client.loadMission();
	}
	else if ($Server::LoadFailMsg !$= "")
	{
		messageClient(%client, 'MsgLoadFailed', $Server::LoadFailMsg);
	}
	$Server::PlayerCount++;
}

// Called when all datablocks from above have been transmitted.
function GameConnection::onDataBlocksDone(%client) 
{
   // Start sending ghosts to the client.
   %client.activateGhosting();
   %client.onEnterGame();
}

function GameConnection::loadMission(%this)
{
   // Send over the information that will display the server info
   // when we learn it got there, we'll send the data blocks
   %this.currentPhase = 0;
   if (%this.isAIControlled())
   {
      // Cut to the chase...
      %this.onEnterGame();
   }
   else
   {
      commandToClient(%this, 'MissionStartPhase1', $missionSequence,
         $Server::MissionFile, MissionGroup.musicTrack);
         
      echo("*** Sending mission load to client: " @ $Server::MissionFile);
   }
}

function GameConnection::initialControlSet(%this)
{
   echo ("*** Initial Control Object");

   // The first control object has been set by the server
   // and we are now ready to go.
   
   // first check if the editor is active
   //if (!isToolBuild() || !Editor::checkActiveLoadDone())
   //{
      if (Canvas.getContent() != PlayGui.getId())
      {
         Canvas.setContent(PlayGui);
      }
   //}

   //-JR
   %canvasExt = Canvas.getExtent();
   commandToServer('setClientAspectRatio', %canvasExt.x, %canvasExt.y);
   //-JR
}

function GameConnection::onControlObjectChange(%this)
{
   echo ("*** Control Object Changed");
}

function GameConnection::setLagIcon(%this, %state)
{
   if (%this.getAddress() $= "local")
      return;
   LagIcon.setVisible(%state $= "true");
}

// Called on the new connection object after connect() succeeds.
function GameConnection::onConnectionAccepted(%this)
{
   // Startup the physics world on the client before any
   // datablocks and objects are ghosted over.
   physicsInitWorld( "client" );   
}

function GameConnection::onConnectionTimedOut(%this)
{
   // Called when an established connection times out
   disconnectedCleanup();
   MessageBoxOK( "TIMED OUT", "The server connection has timed out.");
}

function GameConnection::onConnectionDropped(%this, %msg)
{
   // Established connection was dropped by the server
   disconnectedCleanup();
   MessageBoxOK( "DISCONNECT", "The server has dropped the connection: " @ %msg);
}

function GameConnection::onConnectionError(%this, %msg)
{
   // General connection error, usually raised by ghosted objects
   // initialization problems, such as missing files.  We'll display
   // the server's connection error message.
   disconnectedCleanup();
   MessageBoxOK( "DISCONNECT", $ServerConnectionErrorMessage @ " (" @ %msg @ ")" );
}

//----------------------------------------------------------------------------
// Connection Failed Events
//----------------------------------------------------------------------------
function GameConnection::onConnectRequestRejected( %this, %msg )
{
   switch$(%msg)
   {
      case "CR_INVALID_PROTOCOL_VERSION":
         %error = "Incompatible protocol version: Your game version is not compatible with this server.";
      case "CR_INVALID_CONNECT_PACKET":
         %error = "Internal Error: badly formed network packet";
      case "CR_YOUAREBANNED":
         %error = "You are not allowed to play on this server.";
      case "CR_SERVERFULL":
         %error = "This server is full.";
      case "CHR_PASSWORD":
         // XXX Should put up a password-entry dialog.
         if ($Client::Password $= "")
            MessageBoxOK( "REJECTED", "That server requires a password.");
         else {
            $Client::Password = "";
            MessageBoxOK( "REJECTED", "That password is incorrect.");
         }
         return;
      case "CHR_PROTOCOL":
         %error = "Incompatible protocol version: Your game version is not compatible with this server.";
      case "CHR_CLASSCRC":
         %error = "Incompatible game classes: Your game version is not compatible with this server.";
      case "CHR_INVALID_CHALLENGE_PACKET":
         %error = "Internal Error: Invalid server response packet";
      default:
         %error = "Connection error.  Please try another server.  Error code: (" @ %msg @ ")";
   }
   disconnectedCleanup();
   MessageBoxOK( "REJECTED", %error);
}

function GameConnection::onConnectRequestTimedOut(%this)
{
   disconnectedCleanup();
   MessageBoxOK( "TIMED OUT", "Your connection to the server timed out." );
}

function GameConnection::startMission(%this)
{
   // Inform the client the mission starting
   commandToClient(%this, 'MissionStart', $missionSequence);
}

function GameConnection::endMission(%this)
{
   // Inform the client the mission is done.  Note that if this is
   // called as part of the server destruction routine, the client will
   // actually never see this comment since the client connection will
   // be destroyed before another round of command processing occurs.
   // In this case, the client will only see the disconnect from the server
   // and should manually trigger a mission cleanup.
   commandToClient(%this, 'MissionEnd', $missionSequence);
}

function clientCmdMissionStart(%seq)
{
   clientStartLevel();
   $Client::missionSeq = %seq;
}

function clientCmdMissionEnd( %seq )
{
   if( $Client::missionRunning && $Client::missionSeq == %seq )
   {
      clientEndLevel();
      $Client::missionSeq = -1;
   }
}
//-----------------------------------------------------------------------------
// Disconnect
//-----------------------------------------------------------------------------
function disconnect()
{
   // We need to stop the client side simulation
   // else physics resources will not cleanup properly.
   physicsStopSimulation( "client" );

   // Delete the connection if it's still there.
   if (isObject(ServerConnection)){
      ServerConnection.delete();
   }
      
   disconnectedCleanup();

   // Call destroyServer in case we're hosting
   destroyServer();
}

function disconnectedCleanup()
{
   // End mission, if it's running.
   if( $Client::missionRunning )
      clientEndLevel();
      
   // Disable mission lighting if it's going, this is here
   // in case we're disconnected while the mission is loading.
   
   $lightingMission = false;
   $sceneLighting::terminateLighting = true;
   
   //
   //PlayerListGui.clear();
   
   // Back to the launch screen
   if (isObject( MainMenuGui ))
      Canvas.setContent( MainMenuGui );

   // Before we destroy the client physics world
   // make sure all ServerConnection objects are deleted.
   if(isObject(ServerConnection))
   {
      ServerConnection.deleteAllObjects();
   }
   
   // We can now delete the client physics simulation.
   physicsDestroyWorld( "client" );                 
}

//Client connect/disconnect calls
function clientStartLevel()
{
   // The client recieves a mission start right before
   // being dropped into the game.
   physicsStartSimulation( "client" );
   
   // Start game audio effects channels.
   
   AudioChannelEffects.play();
   
   // Create client mission cleanup group.
      
   new SimGroup( ClientMissionCleanup );

   // Done.
      
   $Client::missionRunning = true;
}

// Called when mission is ended (either through disconnect or
// mission end client command).
function clientEndLevel()
{
   // Stop physics simulation on client.
   physicsStopSimulation( "client" );

   // Stop game audio effects channels.
   
   AudioChannelEffects.stop();
   
   // Delete all the decals.
   decalManagerClear();
  
   // Delete client mission cleanup group. 
   if( isObject( ClientMissionCleanup ) )
      ClientMissionCleanup.delete();
      
   clearClientPaths();
      
   // Done.
   $Client::missionRunning = false;
}