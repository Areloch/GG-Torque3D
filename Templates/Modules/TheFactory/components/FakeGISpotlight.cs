function FakeGISpotlight::onAdd(%this)
{
   %this.addComponentField(numberOfRays, "Number of rays to shoot out for simulating fake GI in a bounce", "int", "3");
   %this.addComponentField(numberOfBounces, "Number of bounces to shoot out for simulating fake GI in a bounce", "int", "2");
   %this.addComponentField(drawBouncePaths, "Debug draw the bounce paths", "bool", "false");
   %this.addComponentField(surfaceSpacing, "How far from the surface should the bounce lights be spaced", "float", "0");
   %this.addComponentField(bounceBrightnessDegrade, "How much brightness is lost per-bounce", "float", "0.2");
   %this.addComponentField(bounceConeAngle, "How wide is the bounce light's cone", "float", "30");
   %this.addComponentField(bounceDistance, "How wide is the bounce light's cone", "float", "10");

   %this.spotLight = new SpotLight()
   {
      range = 15;
      brightness = 1;  
   };
   
   %this.owner.add(%this.spotLight);
   
   %this.spotLight.mountPos = "0 0 0";
}

function FakeGISpotlight::onRemove(%this)
{

}

function FakeGISpotlight::onClientConnect(%this, %client)
{

}

function FakeGISpotlight::onClientDisonnect(%this, %client)
{

}

function FakeGISpotlight::Update(%this)
{
   setRandomSeed(1);
   for(%i = 0; %i < %this.numberOfRays; %i++)
   {
      %this.castRay(%i);
   }
}

function FakeGISpotlight::castRay(%this, %rayIdx)
{
   %forVec = %this.owner.getForwardVector();
   
   %angle = %this.spotLight.outerAngle;
   
   %randEul = mDegToRad(getRandom(-%angle, %angle)) SPC mDegToRad(getRandom(-%angle, %angle)) SPC mDegToRad(getRandom(-%angle, %angle));
   
   %mat = MatrixCreateFromEuler(%randEul);

   // Which we'll use to alter the projectile's initial vector with
   %rayVec = MatrixMulVector(%mat, %forVec);

   %rayVec = VectorNormalize(%rayVec);
      
   %rayVec = VectorScale(%rayVec, %this.spotLight.range);
   
   %endPos = VectorAdd(%rayVec,%this.owner.position);
   
   %hit = containerRayCast(%this.owner.position, %endPos, -1);
   
   if(%this.drawBouncePaths)
   {
      %hitPos = getWords( %hit, 1, 3 );
      DebugDrawing::drawLine(%this.owner.position, %hitPos, "1 1 1", 32);
   }
   
   %this.curBounceCount = 0;
   %this.bounceRay(%rayIdx, %this.owner.position, %hit);
}

function FakeGISpotlight::bounceRay(%this, %rayIdx, %rayStart, %hitInfo)
{
   if(%this.curBounceCount >= %this.numberOfBounces)
      return;
      
   if(%hitInfo $= "")
      return;
      
   %hitPos = getWords( %hitInfo, 1, 3 );
   %hitNormal = getWords( %hitInfo, 4, 6 );
   
   %this.curBounceCount++;

   if ( %hitPos $= "" )
   {
      %contactLight.enabled = false;
      %contactLight.position = %endPos;
   }
   else
   {
      %contactLight = %this.getBounceLight(%rayIdx, %this.curBounceCount);
      
      %contactLight.enabled = true;  
      %contactLight.position = VectorAdd(%hitPos, VectorScale(%hitNormal, %this.surfaceSpacing));
      
      %rayVec = VectorSub(%hitPos, %rayStart);
         
      %dist = VectorLen(%rayVec);
      
      %distDif = %dist/%this.spotLight.range;
      
      %intensity = 1 - (%this.bounceBrightnessDegrade * %this.curBounceCount);
      %intensity = %intensity > 0 ? %intensity : 0;
      %contactLight.brightness = %intensity;
      
      %contactLight.radius = %this.spotLight.range * (1 - (%this.bounceBrightnessDegrade * %this.curBounceCount));
      
      //%contactLight.outerAngle = %this.bounceConeAngle;
      
      //do bounce
      %bounceVec = VectorReflect(%rayVec, %hitNormal);
      
      //%contactLight.setForwardVector(%hitNormal, %bounceVec);
      
      if(%this.drawBouncePaths)
      {
         DebugDrawing::drawLine(%hitPos, VectorAdd(%hitPos, VectorScale(%hitNormal, 0.5)), "1 0 0", 32);
      }
      
      %bounceRayVec = VectorScale(%bounceVec,%this.spotLight.range);
      
      %endPos = VectorAdd(%bounceRayVec,%hitPos);
      
      %bounceHit = containerRayCast(%hitPos, %endPos, -1);
      
      if(%this.drawBouncePaths)
      {
         %bounceHitPos = getWords( %bounceHit, 1, 3 );
         DebugDrawing::drawLine(%hitPos, %bounceHitPos, "0 1 0", 32);
      }
      
      %this.bounceRay(%rayIdx, %hitPos, %bounceHit);
   }
}

function FakeGISpotlight::getBounceLight(%this, %rayIdx, %bounceCount)
{
   if(!isObject(%this.bounceLight[%rayIdx @ "_" @ %bounceCount]))
   {
      %this.bounceLight[%rayIdx @ "_" @ %bounceCount] = new PointLight()
      {
         radius = %this.spotlight.range;
         outerAngle = 20;
         innerAngle = 1;
         brightness = 1;  
      };
   }
   
   return %this.bounceLight[%rayIdx @ "_" @ %bounceCount];
}

