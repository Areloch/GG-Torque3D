function ProjectileComponent::onAdd(%this)
{
   %this.addComponentField(velocity, "How fast the projectile moves in meters per second", "float", "0", "");
   %this.addComponentField(lifeTime, "How long in seconds the projectile will exist before being deleted.", "float", "60", "");
   
   %this.addComponentField(collisionFilter, "What typemasks the projectile will collide with", "typeMask", "0", "");
   
   %this.collisionMask = $TypeMasks::EntityObjectType | 
                           $TypeMasks::StaticObjectType |
                           $TypeMasks::StaticShapeObjectType |
                           $TypeMasks::DynamicShapeObjectType |
                           $TypeMasks::InteriorObjectType |
                           $TypeMasks::TerrainObjectType |
                           $TypeMasks::VehicleObjectType;
}

function ProjectileComponent::onRemove(%this)
{

}

function ProjectileComponent::Update(%this)
{
   if(%this.velocity == 0)
      return;
      
   %forVec = %this.owner.getForwardVector();
   
   %tickRate = 0.032;
   
   %startPos = %this.owner.position;
   %newPos = VectorScale(%forVec, %this.velocity * %tickRate);
   
   %newPos = VectorAdd(%newPos, %startPos);
   
   %hit = containerRayCast(%startPos, %newPos, %this.collisionMask, %this.owner);
   
   if(%hit == 0)
   {
      %this.owner.position = %newPos;
   }
   else
   {
      //we have a hit!
      %this.velocity = 0;
      %this.owner.position = getWords(%hit, 1, 3);
      %this.enabled = false;
   }
}
