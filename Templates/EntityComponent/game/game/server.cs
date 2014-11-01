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
/// Attempt to find an open port to initialize the server with
function portInit(%port)
{
   %failCount = 0;
   while(%failCount < 10 && !setNetPort(%port))
   {
      echo("Port init failed on port " @ %port @ " trying next port.");
      %port++; %failCount++;
   }
}

/// Create a server of the given type, load the given level, and then
/// create a local client connection to the server.
//
/// @return true if successful.
function createAndConnectToLocalServer( %serverType, %level )
{
   if( !createServer( %serverType, %level ) )
      return false;
   
   %conn = new GameConnection( ServerConnection );
   RootGroup.add( ServerConnection );
   
   %conn.setConnectArgs( $pref::Player::Name );
   %conn.setJoinPassword( $Client::Password );
   
   %result = %conn.connectLocal();
   if( %result !$= "" )
   {
      %conn.delete();
      destroyServer();
      
      return false;
   }
   
   //TODO: move this to a proper place?
   loadMaterials();
   
   return true;
}

/// Create a server with either a "SinglePlayer" or "MultiPlayer" type
/// Specify the level to load on the server
function createServer(%serverType, %level)
{
   // Increase the server session number.  This is used to make sure we're
   // working with the server session we think we are.
   $Server::Session++;
   
   if (%level $= "")
   {
      error("createServer(): level name unspecified");
      return false;
   }
   
   // Make sure our level name is relative so that it can send
   // across the network correctly
   %level = makeRelativePath(%level, getWorkingDirectory());

   destroyServer();

   $missionSequence = 0;
   $Server::PlayerCount = 0;
   $Server::ServerType = %serverType;
   $Server::LoadFailMsg = "";
   $Physics::isSinglePlayer = true;
   
   // Setup for multi-player, the network must have been
   // initialized before now.
   if (%serverType $= "MultiPlayer")
   {
      $Physics::isSinglePlayer = false;
            
      echo("Starting multiplayer mode");

      // Make sure the network port is set to the correct pref.
      portInit($Pref::Server::Port);
      allowConnections(true);

      if ($pref::Net::DisplayOnMaster !$= "Never" )
         schedule(0,0,startHeartbeat);
   }

   // Let the game initialize some things now that the
   // the server has been created
   onServerCreated();

   loadMission(%level, true);
   
   $Game::running = true;
   
   return true;
}

/// Shut down the server
function destroyServer()
{
   $Server::ServerType = "";
   $Server::Running = false;
   
   allowConnections(false);
   stopHeartbeat();
   $missionRunning = false;
   
   // End any running levels
   endMission();
   //onServerDestroyed();

   // Delete all the server objects
   if (isObject(ServerGroup))
      ServerGroup.delete();

   // Delete all the connections:
   while (ClientGroup.getCount())
   {
      %client = ClientGroup.getObject(0);
      %client.delete();
   }

   $Server::GuidList = "";

   // Delete all the data blocks...
   deleteDataBlocks();
   
   // Save any server settings
   //echo( "Exporting server prefs..." );
   //export( "$Pref::Server::*", "~/prefs.cs", false );

   // Increase the server session number.  This is used to make sure we're
   // working with the server session we think we are.
   $Server::Session++;
}

function onServerCreated()
{
   // Server::GameType is sent to the master server.
   // This variable should uniquely identify your game and/or mod.
   $Server::GameType = $appName;

   // Server::MissionType sent to the master server.  Clients can
   // filter servers based on mission type.
  // $Server::MissionType = "Deathmatch";

   // GameStartTime is the sim time the game started. Used to calculated
   // game elapsed time.
   $Game::StartTime = 0;

   // Create the server physics world.
   physicsInitWorld( "server" );

   // Run the other gameplay scripts in this folder
   //exec("./scriptExec.cs");

   %datablockFiles = new ArrayObject();
   %datablockFiles.add( "game/art/forest/managedItemData.cs" );
   loadDatablockFiles( %datablockFiles, true );

   // Keep track of when the game started
   $Game::StartTime = $Sim::Time;
}

/// Guid list maintenance functions
function addToServerGuidList( %guid )
{
   %count = getFieldCount( $Server::GuidList );
   for ( %i = 0; %i < %count; %i++ )
   {
      if ( getField( $Server::GuidList, %i ) == %guid )
         return;
   }

   $Server::GuidList = $Server::GuidList $= "" ? %guid : $Server::GuidList TAB %guid;
}

function removeFromServerGuidList( %guid )
{
   %count = getFieldCount( $Server::GuidList );
   for ( %i = 0; %i < %count; %i++ )
   {
      if ( getField( $Server::GuidList, %i ) == %guid )
      {
         $Server::GuidList = removeField( $Server::GuidList, %i );
         return;
      }
   }
}

/// When the server is queried for information, the value of this function is
/// returned as the status field of the query packet.  This information is
/// accessible as the ServerInfo::State variable.
function onServerInfoQuery()
{
   return "Doing Ok";
}

function endMission()
{
   if (!isObject( MissionGroup ))
      return;

   echo("*** ENDING MISSION");
   
   // Inform the game code we're done.
   onMissionEnded();

   // Inform the clients
   for( %clientIndex = 0; %clientIndex < ClientGroup.getCount(); %clientIndex++ ) {
      // clear ghosts and paths from all clients
      %cl = ClientGroup.getObject( %clientIndex );
      %cl.endMission();
      %cl.resetGhosting();
      %cl.clearPaths();
   }
   
   // Delete everything
   MissionGroup.delete();
   MissionCleanup.delete();
   
   clearServerPaths();
}


//-----------------------------------------------------------------------------
function onMissionLoaded()
{
   // Called by loadMission() once the mission is finished loading
   //startGame();
}

function onMissionEnded()
{
   // Called by endMission(), right before the mission is destroyed
   //endGame();
}

//-----------------------------------------------------------------------------
// Server mission loading
//-----------------------------------------------------------------------------
// On every mission load except the first, there is a pause after
// the initial mission info is downloaded to the client.
$MissionLoadPause = 5000;

//-----------------------------------------------------------------------------

function loadMission( %missionName, %isFirstMission ) 
{
   endMission();
   echo("*** LOADING MISSION: " @ %missionName);
   echo("*** Stage 1 load");

   // increment the mission sequence (used for ghost sequencing)
   $missionSequence++;
   $missionRunning = false;
   $Server::MissionFile = %missionName;
   $Server::LoadFailMsg = "";

   // Extract mission info from the mission file,
   // including the display name and stuff to send
   // to the client.
   buildLoadInfo( %missionName );

   // Download mission info to the clients
   %count = ClientGroup.getCount();
   for( %cl = 0; %cl < %count; %cl++ ) {
      %client = ClientGroup.getObject( %cl );
      if (!%client.isAIControlled())
         sendLoadInfoToClient(%client);
   }

   // Now that we've sent the LevelInfo to the clients
   // clear it so that it won't conflict with the actual
   // LevelInfo loaded in the level
   clearLoadInfo();

   // if this isn't the first mission, allow some time for the server
   // to transmit information to the clients:
   if( %isFirstMission || $Server::ServerType $= "SinglePlayer" )
      loadMissionStage2();
   else
      schedule( $MissionLoadPause, ServerGroup, loadMissionStage2 );
}

//-----------------------------------------------------------------------------

function loadMissionStage2() 
{
   echo("*** Stage 2 load");

   // Create the mission group off the ServerGroup
   $instantGroup = ServerGroup;

   // Make sure the mission exists
   %file = $Server::MissionFile;
   
   if( !isFile( %file ) )
   {
      $Server::LoadFailMsg = "Could not find mission \"" @ %file @ "\"";
   }
   else
   {
      // Calculate the mission CRC.  The CRC is used by the clients
      // to caching mission lighting.
      $missionCRC = getFileCRC( %file );

      // Exec the mission.  The MissionGroup (loaded components) is added to the ServerGroup
      exec(%file);

      if( !isObject(MissionGroup) )
      {
         $Server::LoadFailMsg = "No 'MissionGroup' found in mission \"" @ %file @ "\".";
      }
   }

   if( $Server::LoadFailMsg !$= "" )
   {
      // Inform clients that are already connected
      for (%clientIndex = 0; %clientIndex < ClientGroup.getCount(); %clientIndex++)
         messageClient(ClientGroup.getObject(%clientIndex), 'MsgLoadFailed', $Server::LoadFailMsg);    
      return;
   }

   // Set mission name.
   if( isObject( theLevelInfo ) )
      $Server::MissionName = theLevelInfo.levelName;

   // Mission cleanup group.  This is where run time components will reside.  The MissionCleanup
   // group will be added to the ServerGroup.
   new SimGroup( MissionCleanup );

   // Make the MissionCleanup group the place where all new objects will automatically be added.
   $instantGroup = MissionCleanup;
   
   // Construct MOD paths
   pathOnMissionLoadDone();

   // Mission loading done...
   echo("*** Mission loaded");

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
   //Now, initialize our behaviors since that should all be done on the server
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
         
		 if(%behav.isMethod("onAdd"))
            %behav.onAdd();
      }
   }
   
   // Start all the clients in the mission
   $missionRunning = true;
   for( %clientIndex = 0; %clientIndex < ClientGroup.getCount(); %clientIndex++ )
      ClientGroup.getObject(%clientIndex).loadMission();

   // Go ahead and launch the game
   onMissionLoaded();
}

function loadDatablockFiles( %datablockFiles, %recurse )
{
   if ( %recurse )
   {
      recursiveLoadDatablockFiles( %datablockFiles, 9999 );
      return;
   }
   
   %count = %datablockFiles.count();
   for ( %i=0; %i < %count; %i++ )
   {
      %file = %datablockFiles.getKey( %i );
      if ( !isFile(%file @ ".dso") && !isFile(%file) )
         continue;
                  
      exec( %file );
   }
      
   // Destroy the incoming list.
   %datablockFiles.delete();
}

function recursiveLoadDatablockFiles( %datablockFiles, %previousErrors )
{
   %reloadDatablockFiles = new ArrayObject();

   // Keep track of the number of datablocks that 
   // failed during this pass.
   %failedDatablocks = 0;
   
   // Try re-executing the list of datablock files.
   %count = %datablockFiles.count();
   for ( %i=0; %i < %count; %i++ )
   {      
      %file = %datablockFiles.getKey( %i );
      if ( !isFile(%file @ ".dso") && !isFile(%file) )
         continue;
         
      // Start counting copy constructor creation errors.
      $Con::objectCopyFailures = 0;
                                       
      exec( %file );
                                    
      // If errors occured then store this file for re-exec later.
      if ( $Con::objectCopyFailures > 0 )
      {
         %reloadDatablockFiles.add( %file );
         %failedDatablocks = %failedDatablocks + $Con::objectCopyFailures;
      }
   }
            
   // Clear the object copy failure counter so that
   // we get console error messages again.
   $Con::objectCopyFailures = -1;
                  
   // Delete the old incoming list... we're done with it.
   %datablockFiles.delete();
               
   // If we still have datablocks to retry.
   %newCount = %reloadDatablockFiles.count();
   if ( %newCount > 0 )
   {
      // If the datablock failures have not been reduced
      // from the last pass then we must have a real syntax
      // error and not just a bad dependancy.         
      if ( %lastFailures > %failedDatablocks )
         recursiveLoadDatablockFiles( %reloadDatablockFiles, %failedDatablocks );
                  
      else
      {      
         // Since we must have real syntax errors do one 
         // last normal exec to output error messages.
         loadDatablockFiles( %reloadDatablockFiles, false );
      }
      
      return;
   }
                  
   // Cleanup the empty reload list.
   %reloadDatablockFiles.delete();         
}