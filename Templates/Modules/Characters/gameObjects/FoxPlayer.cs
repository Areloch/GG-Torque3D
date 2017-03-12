function FoxPlayer::onAdd(%this)
{
   %this.turnRate = 0.3;

   %this.phys = %this.getComponent("PlayerControllerComponent");
   %this.collision = %this.getComponent("CollisionComponent");
   %this.cam = %this.getComponent("CameraComponent");
   %this.camArm = %this.getComponent("CameraOrbiterComponent");
   %this.animation = %this.getComponent("AnimationComponent");
   %this.stateMachine = %this.getComponent("StateMachineComponent");
   %this.mesh = %this.getComponent("MeshComponent");
   %this.interact = %this.getComponent("InteractComponent");

   %this.stateMachine.forwardVector = 0;
   %this.stateMachine.isCrouched = false;

   %this.crouch = false;
   %this.prone = false;
   
   %this.aiming = false;
   %this.firstPerson = false;
   %this.otherShoulder = false;
   
   %this.falling = false;
   
   %this.runSpeed = 3.5; 
   
   %this.crouchSpeedMod = 0.5;
   
   %this.aimOrbitDist = 0.5;
   %this.regularOrbitDist = 1;
   %this.regularShoulderOffset = 0.5;
   %this.aimShoulderOffset = 0.5;
   
   %this.regularOrbitMaxPitch = 70;
   %this.regularOrbitMinPitch = -10;
   
   %this.aimedMaxPitch = 90;
   %this.aimedMinPitch = -90;
   
   %this.speedScalar = 1;
   %this.speedScalarAdjust = 0.1;
   
   //Set our initial pose
   %this.animation.playThread(0, "ReadyIdle");
   
   %this.moveAnimThread = 0;
   %this.lookAnimThread = 1;
}

function FoxPlayer::onRemove(%this)
{

}

function FoxPlayer::moveVectorEvent(%this)
{
    %moveVector = %this.getMoveVector();

    // forward of the camera on the x-z plane
    %cameraForward = %this.cam.getForwardVector();

    %cameraRight = %this.cam.getRightVector();

    %moveVec = VectorAdd(VectorScale(%cameraRight, %moveVector.x), VectorScale(%cameraForward, %moveVector.y));

   if(%this.aiming || %this.prone)
   {
      %forMove = "0 0 0";
      
      if(%moveVector.x != 0)
      {
         %this.phys.inputVelocity.x = %moveVector.x * %this.runSpeed;
      }
      else
      {
         %this.phys.inputVelocity.x = 0;
      }

      if(%moveVector.y != 0)
      {
         %this.phys.inputVelocity.y = %moveVector.y * %this.runSpeed;
      }
      else
      {
         %this.phys.inputVelocity.y = 0;
      }
   }
   else
   {
      if(%moveVec.x == 0 && %moveVec.y == 0)
      {
         %this.phys.inputVelocity = "0 0 0";
         %this.stateMachine.forwardVector = 0;
      }
      else
      {
         %moveVec.z = 0;

         %curForVec = %this.getForwardVector();

         %newForVec = VectorLerp(%curForVec, %moveVec, %this.turnRate);

         %this.setForwardVector(%newForVec);
         
         %this.phys.inputVelocity.y = %this.runSpeed;

         %this.stateMachine.forwardVector = 1;
      }
   }
   
   if(%this.crouch)
      %this.phys.inputVelocity = VectorScale(%this.phys.inputVelocity, %this.crouchSpeedMod);
   else if(%this.prone)
      %this.phys.inputVelocity = VectorScale(%this.phys.inputVelocity, %this.crouchSpeedMod);
      
   %this.phys.inputVelocity = VectorScale(%this.phys.inputVelocity, %this.speedScalar);
}

function FoxPlayer::moveYawEvent(%this)
{
   %moveRotation = %this.getMoveRotation();

    %camOrb = %this.getComponent("CameraOrbiterComponent");
    
    if(%this.aiming || (%this.prone && %this.getMoveVector() !$= "0 0 0"))
    {
      %this.rotation.z += %moveRotation.z * 10;
      
      if(%this.otherShoulder)
      {
         %shift = VectorScale(VectorNormalize(%this.getRightVector()), -%this.aimShoulderOffset);
      }
      else
      {
         %shift = VectorScale(VectorNormalize(%this.getRightVector()), %this.aimShoulderOffset);
      }
      
      %this.camArm.positionOffset = %shift;
    }
    else
    {
      if(%this.otherShoulder)
      {
         %shift = VectorScale(VectorNormalize(%this.getRightVector()), -%this.regularShoulderOffset);
      }
      else
      {
         %shift = VectorScale(VectorNormalize(%this.getRightVector()), %this.regularShoulderOffset);
      }

      %this.camArm.positionOffset = %shift;  
    }

    %camOrb.rotation.z += %moveRotation.z * 10;
}

function FoxPlayer::movePitchEvent(%this)
{
   %moveRotation = %this.getMoveRotation();

    %camOrb = %this.getComponent("CameraOrbiterComponent");

    %camOrb.rotation.x += %moveRotation.x * 10;
}

function FoxPlayer::moveRollEvent(%this){}

function FoxPlayer::moveTriggerEvent(%this, %triggerNum, %triggerValue)
{
   if(%triggerNum == 0 && %triggerValue)
	{
	   %arrow = spawnGameObject(ArrowProjectile, false);
	   %arrow.position = %this.cam.getWorldPosition();
	   %arrow.setForwardVector(%this.cam.getForwardVector());
	   
	   %arrow.getComponent(ProjectileComponent).velocity = 100;
	}
   else if(%triggerNum == 3)
   {
      if(%triggerValue)
      {
        %this.firstPerson = !%this.firstPerson;
        
        if(%this.firstPerson)
        {
            %this.rotation.z = %this.cam.rotationOffset.z;
            %this.camArm.orbitDistance = 0;
            %this.camArm.maxPitchAngle = %this.aimedMaxPitch;
            %this.camArm.minPitchAngle = %this.aimedMinPitch;
            
            %this.cam.positionOffset = "0 0 0";
            %this.cam.rotationOffset = "0 0 0";
        }
        else if(%this.aiming)
        {
            %this.camArm.orbitDistance = %this.aimOrbitDist;
            
            %this.camArm.maxPitchAngle = %this.aimedMaxPitch;
            %this.camArm.minPitchAngle = %this.aimedMinPitch;
        }
        else
        {
            %this.camArm.orbitDistance = %this.regularOrbitDist;
            
            %this.camArm.maxPitchAngle = %this.regularOrbitMaxPitch;
            %this.camArm.minPitchAngle = %this.regularOrbitMinPitch;
        }
        
        //commandToClient(localclientConnection, 'SetClientRenderShapeVisibility', 
        //    localclientConnection.getGhostID(%this.getComponent("MeshComponent")), !%this.firstPerson);
      }
   }
	else if(%triggerNum == 2 && %triggerValue == true)
	{
	   //get our best collision assuming up is 0 0 1
	   %collisionAngle = %this.collision.getBestCollisionAngle("0 0 1");
	   
	   if(%collisionAngle >= 80)
	   {
	      %surfaceNormal = %this.collision.getCollisionNormal(0);
	      %jumpVector = VectorScale(%surfaceNormal, 200);
	      echo("Jump surface Angle is at: " @ %surfaceNormal);
	      
	      %this.phys.applyImpulse(%this.position, %jumpVector);
	      %this.setForwardVector(%jumpVector);
	   }
      else
         %this.phys.applyImpulse(%this.position, "0 0 300");
	}
	else if(%triggerNum == 4)
	{
      %this.crouch = %triggerValue;
      %this.stateMachine.isCrouched = %triggerValue;
	}
	else if(%triggerNum == 1)
	{
	   %this.aiming = %triggerValue;  
	   
	   if(%this.aiming)
      {
         %this.rotation.z = %this.cam.rotationOffset.z;
         %this.camArm.orbitDistance = %this.aimOrbitDist;
         %this.camArm.maxPitchAngle = %this.aimedMaxPitch;
         %this.camArm.minPitchAngle = %this.aimedMinPitch;
         
         if(%this.otherShoulder)
         {
            %shift = VectorScale(VectorNormalize(%this.getRightVector()), -%this.aimShoulderOffset);
         }
         else
         {
            %shift = VectorScale(VectorNormalize(%this.getRightVector()), %this.aimShoulderOffset);
         }
         
         %this.camArm.positionOffset = %shift;
      }
      else
      {
         %this.camArm.orbitDistance = %this.regularOrbitDist;
         %this.camArm.maxPitchAngle = %this.regularOrbitMaxPitch;
         %this.camArm.minPitchAngle = %this.regularOrbitMinPitch;
         
         if(%this.otherShoulder)
         {
            %shift = VectorScale(VectorNormalize(%this.getRightVector()), -%this.regularShoulderOffset);
         }
         else
         {
            %shift = VectorScale(VectorNormalize(%this.getRightVector()), %this.regularShoulderOffset);
         }
         
         %this.camArm.positionOffset = %shift;
      }
	}
	else if(%triggerNum == 6)
	{
      //interact
      %this.interact.onInteract();
	}
}

function FoxPlayer::onCollisionEvent(%this, %colObject, %colNormal, %colPoint, %colMatID, %velocity)
{
   if(!%this.phys.isContacted())
    echo(%this @ " collided with " @ %colObject);
}

function FoxPlayer::processTick(%this)
{
   %moveVec = %this.getMoveVector();
   
   //make sure we're actually falling and not merely having jumped by seeing if we're
   //moving downward   
   if(!%this.collision.hasContact() && !%this.falling && %this.phys.velocity.z < 0)
   {
      //we lost contact, but we're not yet falling, do a raycast down and see how far down the nearest thing is
      %searchResult = containerRayCast(%this.position, VectorAdd(%this.position, "0 0 -10"), 
         $TypeMasks::EntityObjectType | 
         $TypeMasks::StaticObjectType |
         $TypeMasks::StaticShapeObjectType |
         $TypeMasks::DynamicShapeObjectType |
         $TypeMasks::InteriorObjectType |
         $TypeMasks::TerrainObjectType |
         $TypeMasks::VehicleObjectType, %this);
   
      if(%searchResult !$= "" && %searchResult != 0)
      {  
         %hitPos = getWords(%searchResult, 1, 3);
         //get the distance
         %distance = getWord(%searchResult, 7);
         
         if(%distance < 0.1)
         {
            //weird hiccup in the ground, but we're not "falling" really
         }
         else if(%distance > 2)
         {
            //long fall  
            %this.falling = true;
         }
         else
         {
            //mid-range fall
            %this.falling = true; 
         }
      }
      else
      {
         //hookay, there's nothing even 10 meters down, so yes, we're absolutely falling  
         %this.falling = true;
      }
   }
   else if(%this.collision.hasContact() && %this.falling)
   {
      //landed
      %this.falling = false;
   }
   
   %actionAnim = %this.findBestActionAnimation();
   
   if(%this.animation.getThreadAnimation(0) !$= %actionAnim)
   {
      //echo("FoxPlayer is changing it's action anim to: " @ %actionAnim);
      %this.animation.playThread(0, %actionAnim);
   }
   
   %this.updateLookAnimation();
}

function FoxPlayer::findBestActionAnimation(%this)
{
   if(%this.falling)
   {
      %bestFit = "Ambient";
   }
   else
   {
      if(%this.crouch)
      {
         %bestFit = "CrouchRifleLow";
      }
      else if(%this.aiming)
      {
         %bestFit = "StandRifleShouldered";
      }
      else if(%this.prone)
      {
         %bestFit = "ProneRifleLow";
      }
      else
      {
         %bestFit = "StandRifleLow";
      }  
   }
   
   return %bestFit;
}

function FoxPlayer::updateLookAnimation(%this)
{
   %bestFit = "";
   if(%this.aiming)
   {
      //vertical look anim
      %bestFit = "StandRifleShouldered_Look";
   }
   else
   {
      //horizontal look anim
      if(!%this.crouch)
      {
         %bestFit = "StandRifleLow_HLook";
      }
      else
      {
         %bestFit = "StandRifleLow_HLook";  
      }
   }
   
   if(%this.animation.getThreadAnimation(%this.lookAnimThread) !$= %bestFit)
      %this.animation.setThreadAnimation(%this.lookAnimThread, %bestFit);
   
   if(%this.aiming)
   {
      %curForVec = %this.getForwardVector();
      
      %curCamVec = %this.cam.getForwardVector();
      
      %angle = mRadToDeg(mGetSignedAngleBetweenVectors(%curCamVec, %curForVec, %this.getRightVector()));
      %angle = %angle + 90;
      //echo("Vertical Look differential for other angle is: " @ %angle);
      
      //
      %pos = %angle / 180;
      %this.animation.setThreadPosition(%this.lookAnimThread, mClamp(%pos, 0, 1));
   }
   else
   {      
      %curForVec = %this.getForwardVector();
      
      %curCamVec = %this.cam.getForwardVector();
      
      %angle = mRadToDeg(mGetSignedAngleBetweenVectors(%curCamVec, %curForVec, %this.getUpVector()));
      %angle = %angle + 180;
      //echo("Horizontal Look differential for other angle is: " @ %angle);
      
      //
      %pos = %angle / 360;
      %this.animation.setThreadPosition(%this.lookAnimThread, mClamp(%pos, 0, 1));
   }
   
   //now, set the thread position based on the look angle
   
}

//State Machine bits
function FoxPlayer::onStandingRoot(%this)
{
   echo("Switched to onStandingRoot state!");
   //%this.animation.playThread(0, "ReadyIdle");
}

function FoxPlayer::onForwardRun(%this)
{
   echo("Switched to onForwardRun state!");
   //%this.animation.playThread(0, "JogF");
}

function FoxPlayer::onCrouchRoot(%this)
{
   //%this.animation.playThread(0, "CrouchIdle");
}

function FoxPlayer::onCrouchForward(%this)
{
   //%this.animation.playThread(0, "JogF");
}

function serverCmdToggleFirstPerson(%client)
{
   %obj = %client.getControlObject();
   %obj.firstPerson = !%obj.firstPerson;
   
   if(%obj.firstPerson)
   {
      %obj.rotation.z = %obj.cam.rotationOffset.z;
      %obj.camArm.orbitDistance = 0;
      %obj.camArm.maxPitchAngle = %obj.aimedMaxPitch;
      %obj.camArm.minPitchAngle = %obj.aimedMinPitch;

      %obj.cam.positionOffset = "0 0 0";
      %obj.cam.rotationOffset = "0 0 0";
   }
   else if(%obj.aiming)
   {
      %obj.camArm.orbitDistance = %obj.aimOrbitDist;

      %obj.camArm.maxPitchAngle = %obj.aimedMaxPitch;
      %obj.camArm.minPitchAngle = %obj.aimedMinPitch;
   }
   else
   {
      %obj.camArm.orbitDistance = %obj.regularOrbitDist;

      %obj.camArm.maxPitchAngle = %obj.regularOrbitMaxPitch;
      %obj.camArm.minPitchAngle = %obj.regularOrbitMinPitch;
   }
}

function serverCmdspeedScalarUp(%client)
{
   %obj = %client.getControlObject();
   %obj.speedScalar += %obj.speedScalarAdjust;
   if(%obj.speedScalar > 1)
      %obj.speedScalar = 1;
}

function serverCmdspeedScalarDown(%client)
{
   %obj = %client.getControlObject();
   %obj.speedScalar -= %obj.speedScalarAdjust;
   if(%obj.speedScalar < 0.1)
      %obj.speedScalar = 0.1;
}

function serverCmdToggleProne(%client)
{
   %obj = %client.getControlObject();
   
   if(%obj.prone == true)
      %obj.prone = false;
   else
      %obj.prone = true;
}

function serverCmdAlternateShoulderView(%client)
{
   %obj = %client.getControlObject();
   
   if(%obj.otherShoulder == true)
      %obj.otherShoulder = false;
   else
      %obj.otherShoulder = true;
}

function FoxPlayer::testMount(%this)
{
   %obj = spawnGameObject("BaseWeaponObject", true);  
   
   %this.mesh.mountObject(%obj, "Mount0");
}