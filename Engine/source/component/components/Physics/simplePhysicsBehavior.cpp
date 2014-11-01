//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "component/components/Physics/simplePhysicsBehavior.h"

#include "platform/platform.h"
#include "console/consoleTypes.h"
#include "core/util/safeDelete.h"
#include "core/resourceManager.h"
#include "core/stream/fileStream.h"
#include "console/consoleTypes.h"
#include "console/consoleObject.h"
#include "ts/tsShapeInstance.h"
#include "core/stream/bitStream.h"
#include "gfx/gfxTransformSaver.h"
#include "console/engineAPI.h"
#include "lighting/lightQuery.h"
#include "T3D/gameBase/gameConnection.h"
#include "collision/collision.h"

#include "component/components/Collision/collisionInterfaces.h"

//////////////////////////////////////////////////////////////////////////
// Callbacks
IMPLEMENT_CALLBACK( SimplePhysicsBehaviorInstance, updateMove, void, ( SimplePhysicsBehaviorInstance* obj ), ( obj ),
                   "Called when the player updates it's movement, only called if object is set to callback in script(doUpdateMove).\n"
                   "@param obj the Player object\n" );

//////////////////////////////////////////////////////////////////////////
// Constructor/Destructor
//////////////////////////////////////////////////////////////////////////
SimplePhysicsBehavior::SimplePhysicsBehavior()
{
   addComponentField("isStatic", "If enabled, object will not simulate physics", "bool", "0", "");
   addComponentField("gravity", "The direction of gravity affecting this object, as a vector", "vector", "0 0 -9", "");
   addComponentField("drag", "The drag coefficient that constantly affects the object", "float", "0.7", "");
   addComponentField("mass", "The mass of the object", "float", "1", "");

   mNetFlags.set(Ghostable | ScopeAlways);

   mFriendlyName = "Simple Physics";
   mComponentType = "Physics";

   mDescription = getDescriptionText("Simple physics behavior that allows gravity and impulses.");
}

SimplePhysicsBehavior::~SimplePhysicsBehavior()
{
   for(S32 i = 0;i < mFields.size();++i)
   {
      ComponentField &field = mFields[i];
      SAFE_DELETE_ARRAY(field.mFieldDescription);
   }

   SAFE_DELETE_ARRAY(mDescription);
}

IMPLEMENT_CO_NETOBJECT_V1(SimplePhysicsBehavior);

//////////////////////////////////////////////////////////////////////////
ComponentInstance *SimplePhysicsBehavior::createInstance()
{
   //return Parent::createInstance<BoxColliderComponentInstance>();
   SimplePhysicsBehaviorInstance *instance = new SimplePhysicsBehaviorInstance(this);

   setupFields( instance );

   if(instance->registerObject())
      return instance;

   delete instance;
   return NULL;
}

bool SimplePhysicsBehavior::onAdd()
{
   if(! Parent::onAdd())
      return false;

   return true;
}

void SimplePhysicsBehavior::onRemove()
{
   Parent::onRemove();
}
void SimplePhysicsBehavior::initPersistFields()
{
   Parent::initPersistFields();
}

U32 SimplePhysicsBehavior::packUpdate(NetConnection *con, U32 mask, BitStream *stream)
{
   U32 retMask = Parent::packUpdate(con, mask, stream);
   return retMask;
}

void SimplePhysicsBehavior::unpackUpdate(NetConnection *con, BitStream *stream)
{
   Parent::unpackUpdate(con, stream);
}

//==========================================================================================
SimplePhysicsBehaviorInstance::SimplePhysicsBehaviorInstance( Component *btemplate ) 
{
   mTemplate = btemplate;
   mOwner = NULL;

   mBuoyancy = 0.f;
   mFriction = 0.3f;
   mElasticity = 0.4f;
   mMaxVelocity = 3000.f;
   mSticky = false;

   moveSpeed = Point3F(1,1,1);

   mNetFlags.set(Ghostable);
}

SimplePhysicsBehaviorInstance::~SimplePhysicsBehaviorInstance()
{
}
IMPLEMENT_CO_NETOBJECT_V1(SimplePhysicsBehaviorInstance);

bool SimplePhysicsBehaviorInstance::onAdd()
{
   if(! Parent::onAdd())
      return false;

   return true;
}

void SimplePhysicsBehaviorInstance::onRemove()
{
   Parent::onRemove();
}

void SimplePhysicsBehaviorInstance::onComponentRemove()
{
   Parent::onComponentRemove();
}

/*void SimplePhysicsBehaviorInstance::onDeleteNotify( SimObject *obj )
{
if ( obj == mCollisionObject ) 
{
mCollisionObject = NULL;
mCollisionTimeout = 0;
}
}*/

void SimplePhysicsBehaviorInstance::initPersistFields()
{
   Parent::initPersistFields();

   addField( "moveSpeed", TypePoint3F, Offset(moveSpeed, SimplePhysicsBehaviorInstance), "");
}

U32 SimplePhysicsBehaviorInstance::packUpdate(NetConnection *con, U32 mask, BitStream *stream)
{
   U32 retMask = Parent::packUpdate(con, mask, stream);
   return retMask;
}

void SimplePhysicsBehaviorInstance::unpackUpdate(NetConnection *con, BitStream *stream)
{
   Parent::unpackUpdate(con, stream);
}

//
void SimplePhysicsBehaviorInstance::processTick(const Move* move)
{
   Parent::processTick(move);

   //
   //if (mCollisionObject && !--mCollisionTimeout)
   //    mCollisionObject = 0;

   // Warp to catch up to server
   if (mDelta.warpCount < mDelta.warpTicks) 
   {
      mDelta.warpCount++;

      // Set new pos.
      mOwner->getTransform().getColumn(3,&mDelta.pos);
      mDelta.pos += mDelta.warpOffset;
      //mDelta.rot[0] = mDelta.rot[1];
      //mDelta.rot[1].interpolate(mDelta.warpRot[0],mDelta.warpRot[1],F32(mDelta.warpCount)/mDelta.warpTicks);
      MatrixF trans;
      mDelta.rot[1].setMatrix(&trans);
      trans.setPosition(mDelta.pos);
      setTransform(trans);

      // Pos backstepping
      mDelta.posVec.x = -mDelta.warpOffset.x;
      mDelta.posVec.y = -mDelta.warpOffset.y;
      mDelta.posVec.z = -mDelta.warpOffset.z;
   }
   else
   {
      // Save current rigid state interpolation
      mDelta.posVec = mOwner->getPosition();
      //mDelta.rot[0] = mOwner->getTransform();

      updatePos(TickSec);
      updateForces();

      // Wrap up interpolation info
      mDelta.pos     = mOwner->getPosition();
      mDelta.posVec -= mOwner->getPosition();
      //mDelta.rot[1]  = mRigid.angPosition;

      // Update container database
      setTransform(mOwner->getTransform());
      setMaskBits(UpdateMask);
      updateContainer();
   }
}

void SimplePhysicsBehaviorInstance::interpolateTick(F32 dt)
{
   // Client side interpolation
   Point3F pos = mDelta.pos + mDelta.posVec * dt;
   MatrixF mat = mOwner->getRenderTransform();
   mat.setColumn(3,pos);
   mOwner->setRenderTransform(mat);
   mDelta.dt = dt;
}


void SimplePhysicsBehaviorInstance::updatePos(const F32 travelTime)
{
   mOwner->getTransform().getColumn(3,&mDelta.posVec);

   // When mounted to another object, only Z rotation used.
   if (mOwner->isMounted()) {
      mVelocity = mOwner->getObjectMount()->getVelocity();
      setPosition(Point3F(0.0f, 0.0f, 0.0f));
      setMaskBits(UpdateMask);
      return;
   }

   Point3F newPos;

   if ( mVelocity.isZero() )
      newPos = mDelta.posVec;
   else
      newPos = _move( travelTime );
   //}

   // Set new position
   // If on the client, calc delta for backstepping
   if (isClientObject())
   {
      mDelta.pos = newPos;
      mDelta.posVec = mDelta.posVec - mDelta.pos;
      mDelta.dt = 1.0f;
   }

   setPosition( newPos );
   setMaskBits( UpdateMask );
   updateContainer();

   /*if (!isGhost())  
   {
   // Do mission area callbacks on the server as well
   checkMissionArea();
   }*/

   return;
}

Point3F SimplePhysicsBehaviorInstance::_move( const F32 travelTime )
{
   // Try and move to new pos
   F32 totalMotion  = 0.0f;

   Point3F start;
   Point3F initialPosition;
   mOwner->getTransform().getColumn(3,&start);
   initialPosition = start;

   VectorF firstNormal(0.0f, 0.0f, 0.0f);
   //F32 maxStep = mDataBlock->maxStepHeight;
   F32 time = travelTime;
   U32 count = 0;
   S32 sMoveRetryCount = 5;

   CollisionInterface* colInterface = mOwner->getComponent<CollisionInterface>();

   if(!colInterface)
      return start + mVelocity * time;

   for (; count < sMoveRetryCount; count++) {
      F32 speed = mVelocity.len();
      if (!speed)
         break;

      Point3F end = start + mVelocity * time;
      Point3F distance = end - start;

      bool collided = colInterface->checkCollisions(time, &mVelocity, start);

      if (colInterface->getCollisionList()->getCount() != 0 && colInterface->getCollisionList()->getTime() < 1.0f) 
      {
         // Set to collision point
         F32 velLen = mVelocity.len();

         F32 dt = time * getMin(colInterface->getCollisionList()->getTime(), 1.0f);
         start += mVelocity * dt;
         time -= dt;

         totalMotion += velLen * dt;

         // Back off...
         if ( velLen > 0.f ) {
            F32 newT = getMin(0.01f / velLen, dt);
            start -= mVelocity * newT;
            totalMotion -= velLen * newT;
         }

         // Pick the surface most parallel to the face that was hit.
         const Collision *collision = colInterface->getCollision(0);
         const Collision *cp = collision + 1;
         const Collision *ep = collision + colInterface->getCollisionList()->getCount();
         for (; cp != ep; cp++)
         {
            //TODO: Move this somewhere else
            if(Entity* colEnt = dynamic_cast<Entity*>(collision->object))
            {
               if(CollisionInterface *colInt = colEnt->getComponent<CollisionInterface>())
               {
                  if(!colInt->doesBlockColliding())
                  {
                     continue;
                  }
               }
            }

            if (cp->faceDot > collision->faceDot)
               collision = cp;
         }

         //check the last/first one just incase
         if(Entity* colEnt = dynamic_cast<Entity*>(collision->object))
         {
            if(CollisionInterface *colInt = colEnt->getComponent<CollisionInterface>())
            {
               if(!colInt->doesBlockColliding())
               {
                  //if our ideal surface doesn't stop us, just move along
                  return start + mVelocity * time;
               }
            }
         }

         //F32 bd = _doCollisionImpact( collision, wasFalling );
         F32 bd = -mDot( mVelocity, collision->normal);

         // Subtract out velocity
         F32 sNormalElasticity = 0.01f;
         VectorF dv = collision->normal * (bd + sNormalElasticity);
         mVelocity += dv;
         if (count == 0)
         {
            firstNormal = collision->normal;
         }
         else
         {
            if (count == 1)
            {
               // Re-orient velocity along the crease.
               if (mDot(dv,firstNormal) < 0.0f &&
                  mDot(collision->normal,firstNormal) < 0.0f)
               {
                  VectorF nv;
                  mCross(collision->normal,firstNormal,&nv);
                  F32 nvl = nv.len();
                  if (nvl)
                  {
                     if (mDot(nv,mVelocity) < 0.0f)
                        nvl = -nvl;
                     nv *= mVelocity.len() / nvl;
                     mVelocity = nv;
                  }
               }
            }
         }
      }
      else
      {
         totalMotion += (end - start).len();
         start = end;
         break;
      }
   }

   if (count == sMoveRetryCount)
   {
      // Failed to move
      start = initialPosition;
      mVelocity.set(0.0f, 0.0f, 0.0f);
   }

   return start;
}

void SimplePhysicsBehaviorInstance::updateForces()
{
   VectorF acc = mGravity * mGravityMod * TickSec;

   CollisionInterface* cI = mOwner->getComponent<CollisionInterface>();
   if(cI)
   {
      if(cI->getCollisionList()->getCount() > 0 && (mVelocity + acc != Point3F(0,0,0)))
      {
         Point3F contactNormal = Point3F(0,0,0);
         bool moveable = false;
         F32 runSurfaceAngle = 40.f;
         F32 moveSurfaceCos = mCos(mDegToRad(runSurfaceAngle));
         F32 bestVd = -1.0f;

         //get our best normal, and if it's a move-able surface
         for(U32 i = 0; i < cI->getCollisionList()->getCount(); i++)
         {
            Collision c = *cI->getCollision(i);

            //find the flattest surface
            F32 vd = mDot(Point3F(0,0,1), c.normal);//poly->plane.z;       // i.e.  mDot(Point3F(0,0,1), poly->plane);
            if (vd > bestVd)
            {
               bestVd = vd;
               contactNormal = c.normal;
               moveable = vd > moveSurfaceCos;
            }
         }

         if ( !moveable && !contactNormal.isZero() )  
            acc = ( acc - 2 * contactNormal * mDot( acc, contactNormal ) );   

         // Acceleration on run surface
         if (moveable) {
            // Remove acc into contact surface (should only be gravity)
            // Clear out floating point acc errors, this will allow
            // the player to "rest" on the ground.
            // However, no need to do that if we're using a physics library.
            // It will take care of itself.
            if (!getPhysicsRep())
            {
               F32 vd = -mDot(acc,contactNormal);
               if (vd > 0.0f) {
                  VectorF dv = contactNormal * (vd + 0.002f);
                  acc += dv;
                  if (acc.len() < 0.0001f)
                     acc.set(0.0f, 0.0f, 0.0f);
               }
            }

            // Force a 0 move if there is no energy, and only drain
            // move energy if we're moving.
            VectorF pv = acc;

            // Adjust the player's requested dir. to be parallel
            // to the contact surface.
            F32 pvl = pv.len();
            if (!getPhysicsRep())
            {
               // We only do this if we're not using a physics library.  The
               // library will take care of itself.
               if (pvl)
               {
                  VectorF nn;
                  mCross(pv,VectorF(0.0f, 0.0f, 1.0f),&nn);
                  nn *= 1.0f / pvl;
                  VectorF cv = contactNormal;
                  cv -= nn * mDot(nn,cv);
                  pv -= cv * mDot(pv,cv);
                  pvl = pv.len();
               }
            }

            // Convert to acceleration
            if ( pvl )
               pv *= moveSpeed / pvl;
            VectorF moveAcc = pv - (mVelocity + acc);
            acc += moveAcc;
         }
      }
   }

   // Adjust velocity with all the move & gravity acceleration
   // TG: I forgot why doesn't the TickSec multiply happen here...
   mVelocity += acc;

   mVelocity -= mVelocity * mDrag * TickSec;

   if( mVelocity.isZero() )
      mVelocity = Point3F::Zero;
   else
      setMaskBits(VelocityMask);
}

//
void SimplePhysicsBehaviorInstance::setVelocity(const VectorF& vel)
{
   Parent::setVelocity(vel);

   // Clamp against the maximum velocity.
   if ( mMaxVelocity > 0 )
   {
      F32 len = mVelocity.magnitudeSafe();
      if ( len > mMaxVelocity )
      {
         Point3F excess = mVelocity * ( 1.0f - (mMaxVelocity / len ) );
         mVelocity -= excess;
      }
   }
}