function InteractComponent::onAdd(%this)
{
   %this.cam = %this.owner.getComponent("CameraComponent");
}

function InteractComponent::onRemove(%this)
{

}

function InteractComponent::onClientConnect(%this, %client)
{

}

function InteractComponent::onClientDisonnect(%this, %client)
{

}

function InteractComponent::Update(%this)
{
   if(!isObject(%this.cam))
      return;
      
   //raycast to hit interactive entities
   %eyePos = %this.cam.getWorldPosition();
   %eyeForward = %this.cam.getForwardVector();
   
   %eyeVecNorm = VectorNormalize(%eyeForward);
   %eyeEnd = VectorScale(%eyeVecNorm, 5);
   %eyeEnd = VectorAdd(%eyeEnd, %eyePos);
   
   //%searchResult = containerRayCast(%eyePos, %eyeEnd, $TypeMasks::EntityObjectType, %this.owner);
   
   if(%searchResult !$= "" && %searchResult != 0)
   {
      %object = getWord(%searchResult,0);
      %interactComp = %object.getComponent(InteractableComponent);
      if(isObject(%interactComp))
      {
         %this.highlighted = %interactComp;
         
         //INTERACTION
         %interactComp.setHighlighted(true);
      }
   }
   else if(isObject(%this.highlighted))
   {
      %this.highlighted.setHighlighted(false);
      %this.highlighted = 0;
   }
}

function InteractComponent::onInteract(%this)
{
   if(!isObject(%this.highlighted))
      return;
      
   %this.highlighted.use(%this);
}