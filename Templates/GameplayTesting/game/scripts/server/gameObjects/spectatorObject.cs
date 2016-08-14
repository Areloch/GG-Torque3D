function spectatorObject::onAdd(%this)
{

}


function spectatorObject::onRemove(%this)
{

}

function spectatorObject::onMoveTrigger(%this, %triggerID)
{
   //check if our jump trigger was pressed!
   if(%triggerID == 2)
   {
      %this.owner.applyImpulse("0 0 0", "0 0 " @ %this.jumpStrength);
   }
}

function spectatorObject::moveVectorEvent(%this)
{
   %moveVector = %this.getMoveVector();
   
   %forwardVec = %this.getForwardVector();

   %rightVec = %this.getRightVector();

   %moveVec = VectorAdd(VectorScale(%RightVec, %moveVector.x), VectorScale(%ForwardVec, %moveVector.y));
   
   if(%moveVec.x)
   {
      %rv = VectorNormalize(%rightVec);
      %rightMove = VectorScale(%rv, (%moveVector.x));
      %this.position = VectorAdd(%this.position, %rightMove);
      //%this.position.x += %rightMove.x + 10;
   }
   
   if(%moveVector.y)
   {
      %fv = VectorNormalize(%forwardVec);
      %forwardMove = VectorScale(%fv, (%moveVector.y));
      %this.position = VectorAdd(%this.position, %forwardMove);
      //%this.position.y += %moveVec.y + 10;
   }
}

function spectatorObject::moveYawEvent(%this)
{
   %moveRotation = %this.getMoveRotation();
   
   //if(%moveRotation.z != 0)
   //   %this.rotation.z += mRadToDeg(%moveRotation.z);
   
   %newRot = Rotation::add(%this.rotation, %moveRotation);
   
   %this.rotation = %newRot;
}

function spectatorObject::movePitchEvent(%this)
{
   %moveRotation = %this.getMoveRotation();
   
   //if(%moveRotation.x != 0)
   //   %this.rotation.x += mRadToDeg(%moveRotation.x);
   
   %newRot = Rotation::add(%this.rotation, %moveRotation);
   
   %this.rotation = %newRot;
}

function spectatorObject::Update(%this)
{
}

