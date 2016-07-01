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
   
   %ForwardVec = %this.getForwardVector();

   %RightVec = %this.getRightVector();

   %moveVec = VectorAdd(VectorScale(%RightVec, %moveVector.x), VectorScale(%ForwardVec, %moveVector.y));
   
   if(%moveVec.x)
   {
      %this.position.x += %moveVec.x + 10;
   }
   
   if(%moveVector.y)
   {
      %this.position.y += %moveVec.y + 10;
   }
}

function spectatorObject::moveYawEvent(%this)
{
   %moveRotation = %this.getMoveRotation();
   
   if(%moveRotation.z != 0)
      %this.rotation.z += mRadToDeg(%moveRotation.z);
}

function spectatorObject::movePitchEvent(%this)
{
   %moveRotation = %this.getMoveRotation();
   
   if(%moveRotation.x != 0)
      %this.rotation.x += mRadToDeg(%moveRotation.x);
}

function spectatorObject::Update(%this)
{
}

