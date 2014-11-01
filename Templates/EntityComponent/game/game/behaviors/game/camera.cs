//-----------------------------------------------------------------------------
// Torque Game Builder
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

if (!isObject(CameraBehav))
{
   %template = new CameraComponent(CameraBehav);
   
   %template.friendlyName = "Camera";
   %template.behaviorType = "Game";
   %template.description  = "Allows the behavior owner to operate as a camera.";

   %template.addBehaviorField(clientOwner, "The client that views this camera", "int", "1", "");
   
   //%template.addBehaviorField(FOV, "The shape to use for rendering", "float", "70", "");
   
   %template.category = "Behaviors";
}

function CameraBehav::onAdd(%this) 
{
   Parent::onBehaviorAdd(%this);

   %test = %this.clientOwner;

   %barf = ClientGroup.getCount();

   %clientID = %this.getClientID();
   if(%clientID && !isObject(%clientID.camera))
   {
      %this.scopeToClient(%clientID);
      %this.owner.setBehaviorDirty(%this, true);

      %clientID.setCameraObject(%this.owner);
      %clientID.setControlCameraFov(%this.FOV);
      
      %clientID.camera = %this.owner;
   }

   %res = $pref::Video::mode;
   %derp = 0;
}

function CameraBehav::onRemove(%this)
{
   %clientID = %this.getClientID();
   if(%clientID)
      %clientID.clearCameraObject();
}

function CameraBehav::onInspectorUpdate(%this)
{
   //if(%this.clientOwner)
      //%this.clientOwner.setCameraObject(%this.owner);
}

function CameraBehav::getClientID(%this)
{
	return ClientGroup.getObject(%this.clientOwner-1);
}

function CameraBehav::isClientCamera(%this, %client)
{
	%clientID = ClientGroup.getObject(%this.clientOwner-1);
	
	if(%client.getID() == %clientID)
		return true;
	else
	    return false;
}

function CameraBehav::onClientConnect(%this, %client)
{
   if(%this.isClientCamera(%client) && !isObject(%client.camera))
   {
      %this.scopeToClient(%client);
      %this.owner.setBehaviorDirty(%this, true);
      
      %client.setCameraObject(%this.owner);
      %client.setControlCameraFov(%this.FOV);
      
      %client.camera = %this.owner;
   }
}

function CameraBehav::onClientDisconnect(%this, %client)
{
   Parent::onClientDisconnect(%this, %client);
   
   if(isClientCamera(%client)){
      %this.clearScopeToClient(%client);
      %client.clearCameraObject();
   }
}

//move to the editor later
GlobalActionMap.bind("keyboard", "alt c", "toggleEditorCam");

function switchCamera(%client, %newCamEntity)
{
	if(!isObject(%client) || !isObject(%newCamEntity))
		return error("SwitchCamera: No client or target camera!");
		
	%cam = %newCamEntity.getBehavior("CameraBehav");
		
	if(!isObject(%cam))
		return error("SwitchCamera: Target camera doesn't have a camera behavior!");
		
	//TODO: Cleanup clientOwner for previous camera!
	%this.clientOwner = ClientGroup.getObjectIndex(%client);
		
	%cam.scopeToClient(%client);
	%newCamEntity.setBehaviorDirty(%cam, true);
	
	%client.setCameraObject(%newCamEntity);
	%client.setControlCameraFov(%cam.FOV);
	
	%client.camera = %newCamEntity;
}

function buildEditorCamera()
{
   if(!isObject(EditorCamera))
   {
	   echo("BUILDING THE EDITOR CAMERA!");
	   //make one!
	   new Entity(EditorCamera)
	   {
		   new ComponentInstance()
		   {
			   template = ControlObjectBehavior;
		   };
		
		   new CameraComponentInstance()
		   {
			   template = CameraBehav;
		   };
		
		   new ComponentInstance()
		   { 
			   template = SpectatorControls;
		   };
	   };
   }
}

//TODO: Move this somewhere else!
function toggleEditorCam(%val)
{
   if(!%val)
      return;
      
   %client = ClientGroup.getObject(0);

   if(!isObject(%client.camera))
      return error("ToggleEditorCam: no existing camera!");
   
   buildEditorCamera();

   //if this is our first switch, just go to the editor camera
   if(%client.lastCam == "" || %client.camera.getId() != EditorCamera.getId())
   {
	   if(%client.lastCam == "")
	   {
	      //set up the position
		  EditorCamera.position = %client.camera.position;
		  EditorCamera.rotation = %client.camera.rotation;
	   }
	
	   %client.lastCam = %client.camera;
	   %client.lastController = %client.getControlObject();
	   switchCamera(%client, EditorCamera); 
	   switchControlObject(%client, EditorCamera);
   }
   else  
   {
       switchCamera(%client, %client.lastCam); 
	   switchControlObject(%client, %client.lastController); 
       %client.lastCam = EditorCamera;
	   %client.lastController = EditorCamera;
   }

}

function serverCmdSetClientAspectRatio(%client, %width, %height)
{
   echo("Client: " @ %client SPC "changing screen res to: " @ %width SPC %height);
   %client.screenExtent = %width SPC %height;
   %cam = %client.getCameraObject();
   %cameraComp = %cam.getBehavior(CameraBehav);

   %cameraComp.ScreenAspect = %width SPC %height;
}