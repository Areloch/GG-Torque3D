//-----------------------------------------------------------------------------
// Torque Game Builder
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

singleton Component(EditorControls)
{   
   friendlyName = "Editor Controls";
   behaviorType = "Input";
   description  = "First Person Shooter-type controls";
   
   networked = true;
};
   
EditorControls.beginGroup("Keys");
   EditorControls.addBehaviorField(forwardKey, "Key to bind to vertical thrust", keybind, "keyboard w");
   EditorControls.addBehaviorField(backKey, "Key to bind to vertical thrust", keybind, "keyboard s");
   EditorControls.addBehaviorField(leftKey, "Key to bind to horizontal thrust", keybind, "keyboard a");
   EditorControls.addBehaviorField(rightKey, "Key to bind to horizontal thrust", keybind, "keyboard d");
   
   EditorControls.addBehaviorField(jump, "Key to bind to horizontal thrust", keybind, "keyboard space");
EditorControls.endGroup();

EditorControls.beginGroup("Mouse");
   EditorControls.addBehaviorField(pitchAxis, "Key to bind to horizontal thrust", keybind, "mouse yaxis");
   EditorControls.addBehaviorField(yawAxis, "Key to bind to horizontal thrust", keybind, "mouse xaxis");
EditorControls.endGroup();

EditorControls.addBehaviorField(moveSpeed, "Horizontal thrust force", float, 20.0);
EditorControls.addBehaviorField(jumpStrength, "Vertical thrust force", float, 3.0);


function EditorControls::onAdd(%this)
{
   Parent::onBehaviorAdd(%this);
}

function EditorControls::onRemove(%this)
{
   Parent::onBehaviorRemove(%this);
   
   commandToClient(%control.clientOwnerID, 'removeSpecCtrlInput', %this.forwardKey);
   commandToClient(%control.clientOwnerID, 'removeSpecCtrlInput', %this.backKey);
   commandToClient(%control.clientOwnerID, 'removeSpecCtrlInput', %this.leftKey);
   commandToClient(%control.clientOwnerID, 'removeSpecCtrlInput', %this.rightKey);
   
   commandToClient(%control.clientOwnerID, 'removeSpecCtrlInput', %this.pitchAxis);
   commandToClient(%control.clientOwnerID, 'removeSpecCtrlInput', %this.yawAxis);
}

function EditorControls::onInspectorUpdate(%this, %field)
{
   %controller = %this.owner.getBehavior( ControlObjectBehavior );
   commandToClient(%controller.clientOwnerID, 'updateSpecCtrlInput', %this.getFieldValue(%field), %field);
}

function EditorControls::onClientConnect(%this, %client)
{
   %control = %this.owner.getBehavior( ControlObjectBehavior );
   if( %control.clientOwnerID != %client)
   {
      echo("SPECTATOR CONTROLS: Client Did Not Match");
      return;
   }
   
   %inputCommand = "EditorControls";
   
   SetInput(%client, %this.forwardKey.x,  %this.forwardKey.y,  %inputCommand@"_forwardKey");
   SetInput(%client, %this.backKey.x,     %this.backKey.y,     %inputCommand@"_backKey");
   SetInput(%client, %this.leftKey.x,     %this.leftKey.y,     %inputCommand@"_leftKey");
   SetInput(%client, %this.rightKey.x,    %this.rightKey.y,    %inputCommand@"_rightKey");
   
   SetInput(%client, %this.jump.x,        %this.jump.y,        %inputCommand@"_jump");
      
   SetInput(%client, %this.pitchAxis.x,   %this.pitchAxis.y,   %inputCommand@"_pitchAxis");
   SetInput(%client, %this.yawAxis.x,     %this.yawAxis.y,     %inputCommand@"_yawAxis");
   
   %this.owner.eulerRotation.y = 0;
}

function EditorControls::onMoveTrigger(%this, %triggerID)
{
   //check if our jump trigger was pressed!
   if(%triggerID == 2)
   {
      %this.owner.applyImpulse("0 0 0", "0 0 " @ %this.jumpStrength);
   }
}

function EditorControls::Update(%this, %moveVector, %moveRotation)
{
   //if we're spectating in the editor, defer to what camera speed the editor wants
   if(isMethod("", "EditorIsActive") && EditorIsActive())
      %this.moveSpeed = $Camera::movementSpeed;
      
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
   {
	  %rot = %this.owner.rotation;
	  %rot.x += mRadToDeg(%moveRotation.x);
	  %this.owner.rotation = %rot;
   }
      
   //this setup doesn't use roll, so we ignore the y axis!
   
   if(%moveRotation.z != 0)
   {
	  %rot = %this.owner.rotation;
	  %rot.z += mRadToDeg(%moveRotation.z);
	  %this.owner.rotation = %rot;
   }
   
}

//
function EditorControls_forwardKey(%val){
   $mvForwardAction = %val;
}

function EditorControls_backKey(%val){
   $mvBackwardAction = %val;
}

function EditorControls_leftKey(%val){
   $mvLeftAction = %val;
}

function EditorControls_rightKey(%val){
   $mvRightAction = %val;
}

function EditorControls_yawAxis(%val){
   %deg = getMouseAdjustAmount(%val);
   $mvYaw += getMouseAdjustAmount(%val);
}

function EditorControls_pitchAxis(%val){
   %deg = getMouseAdjustAmount(%val);
   $mvPitch += getMouseAdjustAmount(%val);
}

function EditorControls_jump(%val){
   $mvTriggerCount2++;
}