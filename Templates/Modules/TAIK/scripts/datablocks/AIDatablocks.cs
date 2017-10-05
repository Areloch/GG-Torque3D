//-----------------------------------------------------------------------------
// Tactical AI Kit
// Copyright (C) Bryce Carrington
//-----------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------------
// AIDatablocks.cs
// Defines datablocks that are used by AI Players
//---------------------------------------------------------------------------------------------------------

function GrenadeLauncherProjectile::onExplode(%this,%obj,%pos,%fade)
{
    radiusDamage(%obj.sourceObject, %pos, %this.damageRadius, %this.radiusDamage, "Explosive", %this.areaImpulse, true);
    AlertAIPlayers(%pos,400,25,"Explosion",3);
}

//---------------------------------------------------------------------------------------------------------
datablock PlayerData(BaseAIPlayer : DefaultPlayerData)
{                                           
  // This is based off of the regular player datablock that comes with starter.fps
  // You can add fields here that deviate from the standard PlayerBody
  // For example:
  // You could add maxdamage = 10 in here
  //   It will still use all of the regular fields from playerbody, but will use the maxdamage of 10
  // we specified instead

  shapeFile = "data/TAIK/art/shapes/TAIKExamplePlayers/SpecOps/SpecOps.dts";
  maxDamage = 100;
  
  standVisWeight = 1.0;
  crouchVisWeight = 0.6;
  proneVisWeight = 0.4;
  camoColor = "60 60 60";
};

//---------------------------------------------------------------------------------------------------------
// The AIPlayer datablocks
//---------------------------------------------------------------------------------------------------------

datablock PlayerData(SpecOpsRifleman : BaseAIPlayer)
{
  maxDamage = 100;
  
  shapeFile = "data/TAIK/art/shapes/TAIKExamplePlayers/SpecOps/SpecOps.dts";
  maxForwardSpeed = 7.5;
  maxBackwardSpeed = 3;
  maxSideSpeed = 4.5;
  
  camoColor = "31 31 30";
   coverLeanOutDist = "0.2";
   crouchBoundingBox = "0.7 0.7 1.7";
};

datablock PlayerData(OpforRifleman : BaseAIPlayer)
{
  maxDamage = 100;
  
  shapeFile = "data/TAIK/art/shapes/TAIKExamplePlayers/OpFor/OpFor.dts";
  maxForwardSpeed = 7.5;
  maxBackwardSpeed = 3;
  maxSideSpeed = 4.5;

  camoColor = "70 81 70";
};

//---------------------------------------------------------------------------------------------------------
// Datablock Methods that apply to all AI datablocks based off of it
//---------------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------------
function SpecOpsRifleman::onReachDestination(%this,%obj)
{
  %obj.onReachDestination();
}
function SpecOpsRifleman::onEndofPath(%this,%obj)
{
  %obj.onEndOfPath(%path);
}
function SpecOpsRifleman::onCollision(%this,%obj,%col)
{
  %obj.onCollision(%col);
}
//---------------------------------------------------------------------------------------------------------
function OpForRifleman::onReachDestination(%this,%obj)
{
  %obj.onReachDestination();
}
function OpForRifleman::onEndofPath(%this,%obj)
{
  %obj.onEndOfPath(%path);
}
function OpForRifleman::onCollision(%this,%obj,%col)
{
  %obj.onCollision(%col);
}
//---------------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------------
// AIPlayer onReachDestination method that is called by the datablock callbacks above
//---------------------------------------------------------------------------------------------------------

function AIPlayer::onReachDestination(%this)
{
   //G.Notman Begin
   if (isObject(AiPaths) && %this.pathArray.count() > 0 && %this.curNode != -1)
   {
      if ((%this.CurNode + 1) == %this.EndNode)
      {
         %this.onEndOfPath(%this.path);
         %ray = ContainerRayCast(VectorAdd(%this.getPosition(),"0 0 0.3"),VectorAdd(%this.targetPathDestination,"0 0 0.3"),$AIPlayer::SolidMasks);
         if (!%ray && VectorDist(%this.getPosition(),%this.targetPathDestination) < 12 && mAbs(getWord(%this.getPosition(),2) - getWord(%this.targetPathDestination,2)) < 2)  //HACK
         {
           %this.setMoveDestination(%this.targetPathDestination);
           %this.curnode = %this.endnode;
           %this.onEndOfPath();
         }
         return;
      }
      else
      {
         //%this.moveToNextNode();
         %ray = ContainerRayCast(%this.getPosition(),%this.targetPathDestination,$AIPlayer::SolidMasks);
         if (!%ray && VectorDist(%this.getPosition(),%this.targetPathDestination) < 10 && mAbs(getWord(%this.getPosition(),2) - getWord(%this.targetPathDestination,2)) < 2) // HACK
         {
           %this.setMoveDestination(%this.targetPathDestination);
           %this.onEndOfPath();
         }
         else
         {
           %this.moveToNextNode();
         }
      }  
      return;
   }
   else
   {
     %this.stop();
   }
}

function AIPlayer::onEndOfPath(%this,%path)
{
   if (isObject(AiPaths))
   {
     // %this.schedule(1000,"SetOnPath");
      //%this.stop();
      //%this.targetPathDestination = ""; // Un-commented on January 8 2009
      %this.path = "";
      %this.endnode = -1;
      %this.curnode = -1;
      return;
   }
}

function AIPlayer::onCollision(%this,%col)
{
  if (%this.getState() $= "Dead")
     return;
  if (%col.getClassName() $= "AIPlayer" || %col.getClassName() $= "Player")
  {
    if (%col.getState() $= "Dead")
      return;

    %mySpeed = VectorLen(%this.getVelocity());
    %colSpeed = VectorLen(%col.getVelocity());
    if (%mySpeed >= %colSpeed) // && (%mySpeed > 0.2 && %colSpeed > 0.2)
    {
      %colpos = %col.getWorldBoxCenter();
      %objpos = %this.getWorldBoxCenter();
      %imp = vectorSub(%colPos,%objPos);
      %imp = vectorScale(%imp,400);
      %col.applyImpulse(%this.getWorldBoxCenter(),%imp);
    }
    else
    {
      %colpos = %col.getWorldBoxCenter();
      %objpos = %this.getWorldBoxCenter();
      %imp = vectorSub(%objPos,%colPos);
      %imp = vectorScale(%imp,400);
      %this.applyImpulse(%col.getWorldBoxCenter(),%imp);
    }
  }
if (%col.item !$= "") {
   // Apply an impulse to the object we collided with
   %eye = %obj.getEyeVector();
         %vec = vectorScale(%eye, 10);
   
        // Add a vertical component to give the item a better arc
  %dot = vectorDot("0 0 1",%eye);
  if (%dot < 0)
    %dot = -%dot;
  %vec = vectorAdd(%vec,vectorScale("0 0 2",1 - %dot));

  // Set the object's position and initial velocity
  %trans = %col.getTransform();
   
   // Heres the position and rotation.
   %pos = getWords(%trans, 0, 2);
  %col.applyImpulse(%pos,VectorScale(%vec,100));
}
}
