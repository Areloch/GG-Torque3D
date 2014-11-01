//-----------------------------------------------------------------------------
// Torque Game Builder
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

singleton Component(SpectatorControls)
{   
   friendlyName = "Spectator Controls";
   behaviorType = "Input";
   description  = "First Person Shooter-type controls";
   
   networked = true;
};
   
SpectatorControls.beginGroup("Keys");
   SpectatorControls.addBehaviorField(forwardKey, "Key to bind to vertical thrust", keybind, "keyboard w");
   SpectatorControls.addBehaviorField(backKey, "Key to bind to vertical thrust", keybind, "keyboard s");
   SpectatorControls.addBehaviorField(leftKey, "Key to bind to horizontal thrust", keybind, "keyboard a");
   SpectatorControls.addBehaviorField(rightKey, "Key to bind to horizontal thrust", keybind, "keyboard d");
   
   SpectatorControls.addBehaviorField(jump, "Key to bind to horizontal thrust", keybind, "keyboard space");
SpectatorControls.endGroup();

SpectatorControls.beginGroup("Mouse");
   SpectatorControls.addBehaviorField(pitchAxis, "Key to bind to horizontal thrust", keybind, "mouse yaxis");
   SpectatorControls.addBehaviorField(yawAxis, "Key to bind to horizontal thrust", keybind, "mouse xaxis");
SpectatorControls.endGroup();

SpectatorControls.addBehaviorField(moveSpeed, "Horizontal thrust force", float, 20.0);
SpectatorControls.addBehaviorField(jumpStrength, "Vertical thrust force", float, 3.0);


function SpectatorControls::onAdd(%this)
{
   Parent::onBehaviorAdd(%this);

   %control = %this.owner.getBehavior( ControlObjectBehavior );
   if(!%control)
   	   return echo("SPECTATOR CONTROLS: No Control Object behavior!");
	
	%this.setupControls(%control.getClientID());
}

function SpectatorControls::onRemove(%this)
{
   Parent::onBehaviorRemove(%this);
   
   %control = %this.owner.getBehavior( ControlObjectBehavior );
   if(!%control)
   	   return echo("SPECTATOR CONTROLS: No Control Object behavior!");

   commandToClient(%control.clientOwnerID, 'removeSpecCtrlInput', %this.forwardKey);
   commandToClient(%control.clientOwnerID, 'removeSpecCtrlInput', %this.backKey);
   commandToClient(%control.clientOwnerID, 'removeSpecCtrlInput', %this.leftKey);
   commandToClient(%control.clientOwnerID, 'removeSpecCtrlInput', %this.rightKey);
   
   commandToClient(%control.clientOwnerID, 'removeSpecCtrlInput', %this.pitchAxis);
   commandToClient(%control.clientOwnerID, 'removeSpecCtrlInput', %this.yawAxis);
}

function SpectatorControls::onInspectorUpdate(%this, %field)
{
   %controller = %this.owner.getBehavior( ControlObjectBehavior );
   if(%controller)
      commandToClient(%controller.getClientID(), 'updateSpecCtrlInput', %this.getFieldValue(%field), %field);
}

function SpectatorControls::onClientConnect(%this, %client)
{
   %this.setupControls(%client);
}

function SpectatorControls::setupControls(%this, %client)
{
   %control = %this.owner.getBehavior( ControlObjectBehavior );
   if(!%control.isControlClient(%client))
   {
      echo("SPECTATOR CONTROLS: Client Did Not Match");
      return;
   }
   
   %inputCommand = "SpectatorControls";
   
   SetInput(%client, %this.forwardKey.x,  %this.forwardKey.y,  %inputCommand@"_forwardKey");
   SetInput(%client, %this.backKey.x,     %this.backKey.y,     %inputCommand@"_backKey");
   SetInput(%client, %this.leftKey.x,     %this.leftKey.y,     %inputCommand@"_leftKey");
   SetInput(%client, %this.rightKey.x,    %this.rightKey.y,    %inputCommand@"_rightKey");
   
   SetInput(%client, %this.jump.x,        %this.jump.y,        %inputCommand@"_jump");
      
   SetInput(%client, %this.pitchAxis.x,   %this.pitchAxis.y,   %inputCommand@"_pitchAxis");
   SetInput(%client, %this.yawAxis.x,     %this.yawAxis.y,     %inputCommand@"_yawAxis");
   
   %this.owner.eulerRotation.y = 0;	
}

function SpectatorControls::onMoveTrigger(%this, %triggerID)
{
   //check if our jump trigger was pressed!
   if(%triggerID == 2)
   {
      %this.owner.applyImpulse("0 0 0", "0 0 " @ %this.jumpStrength);
   }
}

function SpectatorControls::Update(%this, %moveVector, %moveRotation)
{
   if(%moveVector.x)
   {
      %fv = VectorNormalize(%this.owner.getRightVector());
      
      %forMove = VectorScale(%fv, (%moveVector.x * (%this.moveSpeed * 0.032)));
      
      %this.owner.position = VectorAdd(%this.owner.position, %forMove);
   }
   
   if(%moveVector.y)
   {
      %fv = VectorNormalize(%this.owner.getForwardVector());
      
      %forMove = VectorScale(%fv, (%moveVector.y * (%this.moveSpeed * 0.032)));
      
      %this.owner.position = VectorAdd(%this.owner.position, %forMove);
   }
   
   /*if(%moveVector.z)
   {
      %fv = VectorNormalize(%this.owner.getUpVector());
      
      %forMove = VectorScale(%fv, (%moveVector.z * (%this.moveSpeed * 0.032)));
      
      %this.Physics.velocity = VectorAdd(%this.Physics.velocity, %forMove);
   }*/
   
   //eulerRotation is managed in degrees for human-readability. 
   if(%moveRotation.x != 0)
      %this.owner.rotation.x += mRadToDeg(%moveRotation.x);
      
   //this setup doesn't use roll, so we ignore the y axis!
   //%this.owner.rotation.y = 0;
   
   if(%moveRotation.z != 0)
      %this.owner.rotation.z += mRadToDeg(%moveRotation.z);
   
}

//
function SpectatorControls_forwardKey(%val){
   $mvForwardAction = %val;
}

function SpectatorControls_backKey(%val){
   $mvBackwardAction = %val;
}

function SpectatorControls_leftKey(%val){
   $mvLeftAction = %val;
}

function SpectatorControls_rightKey(%val){
   $mvRightAction = %val;
}

function SpectatorControls_yawAxis(%val){
   %deg = getMouseAdjustAmount(%val);
   $mvYaw += getMouseAdjustAmount(%val);
}

function SpectatorControls_pitchAxis(%val){
   %deg = getMouseAdjustAmount(%val);
   $mvPitch += getMouseAdjustAmount(%val);
}

function SpectatorControls_jump(%val){
   $mvTriggerCount2++;
}