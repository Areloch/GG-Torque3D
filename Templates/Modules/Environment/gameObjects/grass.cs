function grass::onAdd(%this)
{
}

function grass::onRemove(%this)
{
   
}

function grass::onCollisionEvent(%this, %colObject, %colNormal, %colPoint, %colMatID, %velocity)
{
   %this.getComponent(AnimationComponent).playThread(0, "crouchIdle");
}

