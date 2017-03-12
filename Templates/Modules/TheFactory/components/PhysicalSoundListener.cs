function PhysicalSoundListener::onAdd(%this)
{
   %this.addComponentField(numberOfRays, "Number of rays to shoot out for simulating sound in a bounce", "int", "20");
   %this.addComponentField(rayDistance, "Max Distance a listener ray will proejct", "float", "200");
   %this.addComponentField(numberOfBounces, "Number of bounces to shoot out for simulating sound in a bounce", "int", "2");
   %this.addComponentField(drawBouncePaths, "Debug draw the bounce paths", "bool", "false");
   %this.addComponentField(drawDirectSuccessPaths, "Debug draw the direct LOS paths that find an emitter", "bool", "false");
   %this.addComponentField(drawDirectFailPaths, "Debug draw the direct LOS paths that fail to find an emitter", "bool", "false");
   
   %this.emitterListCount = 0;
}

function PhysicalSoundListener::onRemove(%this)
{

}

function PhysicalSoundListener::onClientConnect(%this, %client)
{

}

function PhysicalSoundListener::onClientDisonnect(%this, %client)
{

}

function PhysicalSoundListener::Update(%this)
{
   //Get a list of all current PhysicalSoundEmitters
   %entityIds = parseMissionGroupForIds("Entity", "");
   %entityCount = getWordCount(%entityIds);
   
   %this.emitterListCount = 0;
   
   for(%i=0; %i < %entityCount; %i++)
   {
      %entity = getWord(%entityIds, %i);
      
      for(%e=0; %e < %entity.getCount(); %e++)
      {
         %child = %entity.getObject(%e);
         if(%child.getCLassName() $= "Entity")
            %entityIds = %entityIds SPC %child.getID();  
      }
      
      %soundEmitterComponent = %entity.getComponent(PhysicalSoundEmitter);
      
      if(isObject(%soundEmitterComponent))
      {
         %this.emitterListCount++;
         %this.emitterList[%this.emitterListCount-1] = %soundEmitterComponent;
         
         //check for a direct hit
         %hit = containerRayCast(%this.owner.position, %soundEmitterComponent.owner.position, -1);
         
         if(%hit == 0)
         {
            if(%this.drawBouncePaths)
            {
               DebugDrawing::drawLine(%this.owner.position, %soundEmitterComponent.owner.position, "0.5 0 0.5", 32);
            }
         }
      }
   }
   
   setRandomSeed(%this.getId());
   for(%i = 0; %i < %this.numberOfRays; %i++)
   {
      %this.castRay(%i);
   }
}

function PhysicalSoundListener::castRay(%this, %rayIdx)
{
   %randEul = mDegToRad(getRandom(-180, 180)) SPC mDegToRad(getRandom(-180, 180)) SPC mDegToRad(getRandom(-180, 180));

   %rayVec = VectorNormalize(%randEul);
      
   %rayVec = VectorScale(%rayVec, %this.rayDistance);
   
   %endPos = VectorAdd(%rayVec,%this.owner.position);
   
   %hit = containerRayCast(%this.owner.position, %endPos, -1);
   %hitPos = getWords( %hit, 1, 3 );
   
   if(%this.drawBouncePaths)
   {
      DebugDrawing::drawLine(%this.owner.position, %hitPos, "1 1 1", 32);
   }
   
   for(%i=0; %i < %this.emitterListCount; %i++)
   {
      %emitter = %this.emitterList[%i];
      
      //check for a direct hit
      %directHit = containerRayCast(%hitPos, %emitter.owner.position, -1);
      
      if(%directHit == 0)
      {
         if(%this.drawDirectSuccessPaths)
            DebugDrawing::drawLine(%hitPos, %emitter.owner.position, "0 0 1", 32);
      }
      else
      {
         if(%this.drawDirectFailPaths)
            DebugDrawing::drawLine(%hitPos, getWords( %directHit, 1, 3 ), "1 0 0", 32);
      }
      
      if(%directHit == 0)
      {
         return;
      }
   }
   
   %this.curBounceCount = 0;
   %this.bounceRay(%rayIdx, %this.owner.position, %hit);
}

function PhysicalSoundListener::bounceRay(%this, %rayIdx, %rayStart, %hitInfo)
{
   if(%this.curBounceCount >= %this.numberOfBounces)
      return;
      
   if(%hitInfo == 0)
      return;
      
   %hitPos = getWords( %hitInfo, 1, 3 );
   %hitNormal = getWords( %hitInfo, 4, 6 );
   
   %this.curBounceCount++;

   if ( %hitPos $= "" )
   {
      return;
   }
   else
   {
      %rayVec = VectorSub(%hitPos, %rayStart);
         
      %dist = VectorLen(%rayVec);
      
      //do bounce
      %bounceVec = VectorReflect(%rayVec, %hitNormal);

      %bounceRayVec = VectorScale(%bounceVec,%this.rayDistance);
      
      %endPos = VectorAdd(%bounceRayVec,%hitPos);
      
      %bounceHit = containerRayCast(%hitPos, %endPos, -1);
      
      if(%bounceHit == 0)
         return;
         
      %bounceHitPos = getWords( %bounceHit, 1, 3 );
      
      if(%this.drawBouncePaths)
      {
         DebugDrawing::drawLine(%hitPos, %bounceHitPos, "0 1 0", 32);
      }
      
      for(%i=0; %i < %this.emitterListCount; %i++)
      {
         %emitter = %this.emitterList[%i];
         
         //check for a direct hit
         %directHit = containerRayCast(%bounceHitPos, %emitter.owner.position, -1);
         
         if(%directHit == 0)
         {
            if(%this.drawDirectSuccessPaths)
               DebugDrawing::drawLine(%hitPos, %emitter.owner.position, "0 0 1", 32);
            
            return;
         }
         else
         {
            if(%this.drawDirectFailPaths)
               DebugDrawing::drawLine(%hitPos, getWords( %directHit, 1, 3 ), "1 0 0", 32);
         }
      }
      
      %this.bounceRay(%rayIdx, %hitPos, %bounceHit);
   }
}