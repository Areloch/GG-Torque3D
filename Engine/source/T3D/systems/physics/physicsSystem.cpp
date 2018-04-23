#include "T3D/systems/physics/physicsSystem.h"
#include "gfx/gfxTransformSaver.h"
#include "lighting/lightQuery.h"

#include "renderInstance/renderPassManager.h"
#include "materials/materialManager.h"
#include "materials/baseMatInstance.h"

#include "T3D/physics/physicsPlugin.h"

static F32 sVerticalStepDot = 0.173f;   // 80
static F32 sMinFaceDistance = 0.01f;
static F32 sTractionDistance = 0.04f;
static F32 sNormalElasticity = 0.01f;
static U32 sMoveRetryCount = 5;
static F32 sMaxImpulseVelocity = 200.0f;
// Physics and collision constants
static F32 sRestTol = 0.5;             // % of gravity energy to be at rest
static S32 sRestCount = 10;            // Consecutive ticks before comming to rest

void PhysicsSystem::processTick()
{
   U32 count = PhysicsSystemInterface::all.size();
   for (U32 i = 0; i < count; i++)
   {
      PhysicsSystemInterface* inter = PhysicsSystemInterface::all[i];

      if (!inter->mEnabled || !inter->mOwner || !inter->mOwner->getOwner())
         continue;

      Entity* ownerEntity = inter->mOwner->getOwner();

      //We operate under the presumption that because we are designed around a client/server behavior, networking misalignment will happen at some point
      //So we just deal with the delta warp/interpolation behavior innately to make things behave consistently.

      // Warp to catch up to server
      if (inter->mDelta.warpCount < inter->mDelta.warpTicks)
      {
         inter->mDelta.warpCount++;

         // Set new pos.
         inter->mDelta.pos = ownerEntity->getPosition();
         inter->mDelta.pos += inter->mDelta.warpOffset;
         inter->mDelta.rot[0] = inter->mDelta.rot[1];
         inter->mDelta.rot[1].interpolate(inter->mDelta.warpRot[0], inter->mDelta.warpRot[1], F32(inter->mDelta.warpCount) / inter->mDelta.warpTicks);

         MatrixF trans;
         inter->mDelta.rot[1].setMatrix(&trans);
         trans.setPosition(inter->mDelta.pos);

         ownerEntity->setTransform(trans);

         // Pos backstepping
         inter->mDelta.posVec.x = -inter->mDelta.warpOffset.x;
         inter->mDelta.posVec.y = -inter->mDelta.warpOffset.y;
         inter->mDelta.posVec.z = -inter->mDelta.warpOffset.z;
      }
      else
      {
         // Save current rigid state interpolation
         inter->mDelta.posVec = ownerEntity->getPosition();
         inter->mDelta.rot[0] = ownerEntity->getTransform();

         //Update and movement-driven behavior
         updateMove(inter, inter->moveEvent);

         //Update the simulations' forces, as well as forces fed from zones and containers
         updateForces(inter, TickSec);

         //now, before we enact our movement, we may need to update our working collision set to know what all we might collide with when we try to move
         //so kick that off where applicable
         if (inter->mCollisionInterface)
            CollisionSystem::updateWorkingCollisionSet(inter->mCollisionInterface, inter->mVelocity, -1);
         
         //Finally, try and move to our new position with the given forces acting on us.
         updatePosition(inter, TickSec);

         // Wrap up interpolation info
         inter->mDelta.pos = ownerEntity->getPosition();
         inter->mDelta.posVec -= ownerEntity->getPosition();
         inter->mDelta.rot[1] = ownerEntity->getTransform();

         // Update container database
         inter->mOwner->setTransform(ownerEntity->getTransform());
      }
   }
}

void PhysicsSystem::updateForces(PhysicsSystemInterface* inter, F32 deltaTime)
{
   //Direct us to the appropriate management of forces based on our interface's behavioral type
   //not the prettiest system, but it'll work for now
   if (inter->mBehaviorType == PhysicsSystemInterface::Simple)
      updateSimpleForces(inter, deltaTime);
   else if (inter->mBehaviorType == PhysicsSystemInterface::Player)
      updatePlayerForces(inter, deltaTime);
   else if (inter->mBehaviorType == PhysicsSystemInterface::RigidBody)
      updateRigidForces(inter, deltaTime);
}

void PhysicsSystem::updateSimpleForces(PhysicsSystemInterface* interface, F32 deltaTime)
{

}

void PhysicsSystem::updatePlayerForces(PhysicsSystemInterface* interface, F32 deltaTime)
{
   VectorF acc = interface->mMass * interface->mGravity * interface->mGravityMod * deltaTime;

   CollisionSystemInterface* cI = interface->mCollisionInterface;
   if (cI)
   {
      CollisionList* colList = CollisionSystem::getCollisionList(cI);
      U32 contactCount = colList->getCount();
      Point3F vel = interface->mVelocity + acc;

      if (contactCount > 0 && (interface->mVelocity + acc != Point3F(0, 0, 0)))
      {
         Point3F contactNormal = Point3F(0, 0, 0);
         bool moveable = false;
         F32 runSurfaceAngle = 40.f;
         F32 moveSurfaceCos = mCos(mDegToRad(runSurfaceAngle));
         F32 bestVd = -1.0f;

         //get our best normal, and if it's a move-able surface
         for (U32 i = 0; i < colList->getCount(); i++)
         {
            Collision c = *CollisionSystem::getCollision(cI, i);

            //find the flattest surface
            F32 vd = mDot(Point3F(0, 0, 1), c.normal);//poly->plane.z;       // i.e.  mDot(Point3F(0,0,1), poly->plane);
            if (vd > bestVd)
            {
               bestVd = vd;
               contactNormal = c.normal;
               moveable = vd > moveSurfaceCos;
            }
         }

         if (!moveable && !contactNormal.isZero())
            acc = (acc - 2 * contactNormal * mDot(acc, contactNormal));

         // Acceleration on run surface
         if (moveable)
         {
            // Remove acc into contact surface (should only be gravity)
            // Clear out floating point acc errors, this will allow
            // the player to "rest" on the ground.
            // However, no need to do that if we're using a physics library.
            // It will take care of itself.
            if (!interface->mPhysicsRep)
            {
               F32 vd = -mDot(acc, contactNormal);
               if (vd > 0.0f)
               {
                  VectorF dv = contactNormal * (vd + 0.002f);
                  acc += dv;
                  if (acc.len() < 0.0001f)
                     acc.set(0.0f, 0.0f, 0.0f);
               }
            }

            // Force a 0 move if there is no energy, and only drain
            // move energy if we're moving.
            //VectorF pv = acc;
            VectorF pv = interface->mMoveSpeed;

            // Adjust the player's requested dir. to be parallel
            // to the contact surface.
            F32 pvl = pv.len();
            if (!interface->mPhysicsRep)
            {
               // We only do this if we're not using a physics library.  The
               // library will take care of itself.
               if (pvl)
               {
                  VectorF nn;
                  mCross(pv, VectorF(0.0f, 0.0f, 1.0f), &nn);
                  nn *= 1.0f / pvl;
                  VectorF cv = contactNormal;
                  cv -= nn * mDot(nn, cv);
                  pv -= cv * mDot(pv, cv);
                  pvl = pv.len();
               }
            }

            // Convert to acceleration
            if (pvl)
               pv *= interface->mMoveSpeed / pvl;

            VectorF moveAcc = pv - (interface->mVelocity + acc);
            acc += moveAcc;
         }
      }
   }

   // Adjust velocity with all the move & gravity acceleration
   // TG: I forgot why doesn't the TickSec multiply happen here...
   interface->mVelocity += acc;

   interface->mVelocity -= interface->mVelocity * interface->mDrag * TickSec;

   if (interface->mVelocity.isZero())
      interface->mVelocity = Point3F::Zero;
}

void PhysicsSystem::updateRigidForces(PhysicsSystemInterface* interface, F32 deltaTime)
{
   if (interface->mPhysicsRep)
   {
      //update the rigid behaviors via the physics rep

      PROFILE_SCOPE(PhysicsShape_updateContainerForces);

      // If we're not simulating don't update forces.
      if (!interface->mPhysicsWorld->isEnabled())
         return;

      ContainerQueryInfo info;

      interface->mTransform.mul(interface->mBounds);
      info.box = interface->mBounds;
      info.mass = interface->mMass;

      // Find and retreive physics info from intersecting WaterObject(s)
      interface->mOwner->getOwner()->getContainer()->findObjects(interface->mBounds, WaterObjectType | PhysicalZoneObjectType, findRouter, &info);

      // Calculate buoyancy and drag
      F32 angDrag = interface->mAngularDamping;
      F32 linDrag = interface->mLinearDamping;
      F32 buoyancy = 0.0f;
      Point3F cmass = interface->mPhysicsRep->getCMassPosition();

      F32 density = interface->mBouyancyDensity;
      if (density > 0.0f)
      {
         if (info.waterCoverage > 0.0f)
         {
            F32 waterDragScale = info.waterViscosity * interface->mWaterDampingScale;
            F32 powCoverage = mPow(info.waterCoverage, 0.25f);

            angDrag = mLerp(angDrag, angDrag * waterDragScale, powCoverage);
            linDrag = mLerp(linDrag, linDrag * waterDragScale, powCoverage);
         }

         buoyancy = (info.waterDensity / density) * mPow(info.waterCoverage, 2.0f);

         // A little hackery to prevent oscillation
         // Based on this blog post:
         // (http://reinot.blogspot.com/2005/11/oh-yes-they-float-georgie-they-all.html)
         // JCF: disabled!
         Point3F buoyancyForce = buoyancy * -interface->mPhysicsWorld->getGravity() * TickSec * interface->mMass;
         interface->mPhysicsRep->applyImpulse(cmass, buoyancyForce);
      }

      // Update the dampening as the container might have changed.
      interface->mPhysicsRep->setDamping(linDrag, angDrag);

      // Apply physical zone forces.
      if (!info.appliedForce.isZero())
         interface->mPhysicsRep->applyImpulse(cmass, info.appliedForce);
   }
   else if (interface->mRigid)
   {
      //update the rigid behaviors via the Rigid class
      Point3F gravForce(interface->mGravity * interface->mRigid->mass * interface->mGravityMod);

      MatrixF currTransform;
      interface->mRigid->getTransform(&currTransform);

      Point3F torque(0, 0, 0);
      Point3F force(0, 0, 0);

      Point3F vel = interface->mRigid->linVelocity;

      // Gravity
      force += gravForce;

      // Apply drag
      Point3F vDrag = interface->mRigid->linVelocity;
      vDrag.convolve(Point3F(1, 1, interface->mVertFactor));
      force -= vDrag * interface->mDrag;

      // Add in physical zone force
      ContainerQueryInfo info;

      interface->mTransform.mul(interface->mBounds);
      info.box = interface->mBounds;
      info.mass = interface->mMass;

      // Find and retreive physics info from intersecting WaterObject(s)
      interface->mOwner->getOwner()->getContainer()->findObjects(interface->mBounds, WaterObjectType | PhysicalZoneObjectType, findRouter, &info);

      force += info.appliedForce;

      // Container buoyancy & drag
      force += Point3F(-interface->mBuoyancy * interface->mGravity * interface->mRigid->mass * interface->mGravityMod);
      force -= interface->mRigid->linVelocity * interface->mDrag;
      torque -= interface->mRigid->angMomentum * interface->mDrag;

      interface->mRigid->force = force;
      interface->mRigid->torque = torque;
   }
   else
   {
      //uh....hm. Not sure what to do here
   }

}

void PhysicsSystem::updatePosition(PhysicsSystemInterface* inter, F32 deltaTime)
{
   //Direct us to the appropriate management of forces based on our interface's behavioral type
   //not the prettiest system, but it'll work for now
   if (inter->mBehaviorType == PhysicsSystemInterface::Simple)
      updateSimplePosition(inter, deltaTime);
   else if (inter->mBehaviorType == PhysicsSystemInterface::Player)
      updatePlayerPosition(inter, deltaTime);
   else if (inter->mBehaviorType == PhysicsSystemInterface::RigidBody)
      updateRigidPosition(inter, deltaTime);
}

void PhysicsSystem::updateSimplePosition(PhysicsSystemInterface* interface, F32 deltaTime)
{

}

void PhysicsSystem::updatePlayerPosition(PhysicsSystemInterface* interface, F32 deltaTime)
{
   if (interface->mPlayerPhysicsRep)
   {
      if (!PHYSICSMGR)
         return;

      PROFILE_SCOPE(PlayerControllerComponent_UpdatePos);

      Point3F newPos;

      Collision col;
      dMemset(&col, 0, sizeof(col));

      static CollisionList collisionList;
      collisionList.clear();

      newPos = interface->mPlayerPhysicsRep->move(interface->mVelocity * deltaTime, collisionList);

      bool haveCollisions = false;
      bool wasFalling = interface->mFalling;
      if (collisionList.getCount() > 0)
      {
         interface->mFalling = false;
         haveCollisions = true;

         //TODO: clean this up so the phys component doesn't have to tell the col interface to do this
         if (interface->mCollisionInterface)
         {
            interface->mCollisionInterface->mCollisionList = collisionList;
            interface->mCollisionInterface->handleCollisionList(interface->mVelocity);
         }
      }

      if (haveCollisions)
      {
         // Pick the collision that most closely matches our direction
         VectorF velNormal = interface->mVelocity;
         velNormal.normalizeSafe();
         const Collision *collision = &collisionList[0];
         F32 collisionDot = mDot(velNormal, collision->normal);
         const Collision *cp = collision + 1;
         const Collision *ep = collision + collisionList.getCount();
         for (; cp != ep; cp++)
         {
            F32 dp = mDot(velNormal, cp->normal);
            if (dp < collisionDot)
            {
               collisionDot = dp;
               collision = cp;
            }
         }

         // Modify our velocity based on collisions
         for (U32 i = 0; i<collisionList.getCount(); ++i)
         {
            F32 bd = -mDot(interface->mVelocity, collisionList[i].normal);
            VectorF dv = collisionList[i].normal * (bd + sNormalElasticity);
            interface->mVelocity += dv;
         }

         // Store the last collision for use later on.  The handle collision
         // code only expects a single collision object.
         if (collisionList.getCount() > 0)
            col = collisionList[collisionList.getCount() - 1];

         // We'll handle any player-to-player collision, and the last collision
         // with other obejct types.
         for (U32 i = 0; i<collisionList.getCount(); ++i)
         {
            Collision& colCheck = collisionList[i];
            if (colCheck.object)
            {
               col = colCheck;
            }
         }
      }

      MatrixF newMat;
      newMat.setPosition(newPos);
      interface->mPhysicsRep->setTransform(newMat);

      interface->mOwner->getOwner()->setPosition(newPos);
   }
   else
   {
      // Try and move to new pos
      F32 totalMotion = 0.0f;

      Point3F start;
      Point3F end;
      Point3F initialPosition;
      interface->mOwner->getOwner()->getTransform().getColumn(3, &start);
      initialPosition = start;

      VectorF firstNormal(0.0f, 0.0f, 0.0f);
      //F32 maxStep = maxStepHeight;
      F32 time = deltaTime;
      U32 count = 0;
      S32 sMoveRetryCount = 5;

      if (!interface->mCollisionInterface)
         end = start + interface->mVelocity * time;
      else
      {
         for (; count < sMoveRetryCount; count++)
         {
            F32 speed = interface->mVelocity.len();
            if (!speed)
               break;

            Point3F end = start + interface->mVelocity * time;
            Point3F distance = end - start;

            bool collided = interface->mCollisionInterface->checkCollisions(time, &interface->mVelocity, start);

            if (collided && interface->mCollisionInterface->getCollisionList()->getCount() != 0 && interface->mCollisionInterface->getCollisionList()->getTime() < 1.0f)
            {
               // Set to collision point
               F32 velLen = interface->mVelocity.len();

               F32 dt = time * getMin(interface->mCollisionInterface->getCollisionList()->getTime(), 1.0f);
               start += interface->mVelocity * dt;
               time -= dt;

               totalMotion += velLen * dt;

               // Back off...
               if (velLen > 0.f)
               {
                  F32 newT = getMin(0.01f / velLen, dt);
                  start -= interface->mVelocity * newT;
                  totalMotion -= velLen * newT;
               }

               //implement stepping

               // Pick the surface most parallel to the face that was hit.
               U32 colCount = interface->mCollisionInterface->getCollisionList()->getCount();

               const Collision *collision = NULL;
               //const Collision *cp = collision + 1;
               //const Collision *ep = collision + colInterface->getCollisionList()->getCount();
               /*for (; cp != ep; cp++)
               {
               U32 colCountLoop = colInterface->getCollisionList()->getCount();

               //TODO: Move this somewhere else
               if (Entity* colEnt = dynamic_cast<Entity*>(collision->object))
               {
               if (CollisionInterface *colInt = colEnt->getComponent<CollisionInterface>())
               {
               if (!colInt->doesBlockColliding())
               {
               continue;
               }
               }
               }

               if (cp->faceDot > collision->faceDot)
               collision = cp;
               }*/

               for (int c = 0; c < colCount; c++)
               {
                  const Collision *colTemp = interface->mCollisionInterface->getCollision(c);

                  //TODO: Move this somewhere else
                  /*if (Entity* colEnt = dynamic_cast<Entity*>(colTemp->object))
                  {
                  if (CollisionInterface *colInt = colEnt->getComponent<CollisionInterface>())
                  {
                  if (!colInt->doesBlockColliding())
                  {
                  continue;
                  }
                  }
                  }*/

                  //we didn't reject it, so set it
                  if (collision == NULL)
                  {
                     collision = colTemp;
                  }
                  else
                  {
                     if (colTemp->faceDot > collision->faceDot)
                        collision = colTemp;
                  }
               }

               //found no valid collisions so just exit
               if (collision == NULL)
               {
                  end = start + interface->mVelocity * time;
                  break;
               }

               //check the last/first one just incase
               /*if (Entity* colEnt = dynamic_cast<Entity*>(collision->object))
               {
               if (CollisionInterface *colInt = colEnt->getComponent<CollisionInterface>())
               {
               if (!colInt->doesBlockColliding())
               {
               //if our ideal surface doesn't stop us, just move along
               return start + mVelocity * time;
               }
               }
               }*/

               //F32 bd = _doCollisionImpact( collision, wasFalling );
               F32 bd = -mDot(interface->mVelocity, collision->normal);

               // Subtract out velocity
               F32 sNormalElasticity = 0.01f;
               VectorF dv = collision->normal * (bd + sNormalElasticity);
               interface->mVelocity += dv;
               if (count == 0)
               {
                  firstNormal = collision->normal;
               }
               else
               {
                  if (count == 1)
                  {
                     // Re-orient velocity along the crease.
                     if (mDot(dv, firstNormal) < 0.0f &&
                        mDot(collision->normal, firstNormal) < 0.0f)
                     {
                        VectorF nv;
                        mCross(collision->normal, firstNormal, &nv);
                        F32 nvl = nv.len();
                        if (nvl)
                        {
                           if (mDot(nv, interface->mVelocity) < 0.0f)
                              nvl = -nvl;
                           nv *= interface->mVelocity.len() / nvl;
                           interface->mVelocity = nv;
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
            end = initialPosition;
            interface->mVelocity.set(0.0f, 0.0f, 0.0f);
         }
      }

      interface->mOwner->getOwner()->setPosition(end);
   }
}

void PhysicsSystem::updateRigidPosition(PhysicsSystemInterface* interface, F32 deltaTime)
{
   /*if (interface->mPhysicsRep)
   {
      AssertFatal(interface->mPhysicsRep && !mDestroyed, "PhysicsSystem::updateRigidPosition - Shouldn't be processing a destroyed shape!");

      // Note that unlike TSStatic, the serverside PhysicsShape does not
      // need to play the ambient animation because even if the animation were
      // to move collision shapes it would not affect the physx representation.

      if (!interface->mPhysicsRep->isDynamic())
         return;

      // Store the last render state.
      mRenderState[0] = mRenderState[1];

      // If the last render state doesn't match the last simulation 
      // state then we got a correction and need to 
      Point3F errorDelta = mRenderState[1].position - mState.position;
      const bool doSmoothing = !errorDelta.isZero() && !smNoSmoothing;

      const bool wasSleeping = mState.sleeping;

      // Get the new physics state.
      if (interface->mPhysicsRep)
      {
         interface->mPhysicsRep->getState(&mState);
         _updateContainerForces();
      }
      else
      {
         // This is where we could extrapolate.
      }

      // Smooth the correction back into the render state.
      mRenderState[1] = mState;
      if (doSmoothing)
      {
         F32 correction = mClampF(errorDelta.len() / 20.0f, 0.1f, 0.9f);
         mRenderState[1].position.interpolate(mState.position, mRenderState[0].position, correction);
         mRenderState[1].orientation.interpolate(mState.orientation, mRenderState[0].orientation, correction);
      }

      // If we haven't been sleeping then update our transform
      // and set ourselves as dirty for the next client update.
      if (!wasSleeping || !mState.sleeping)
      {
         // Set the transform on the parent so that
         // the physics object isn't moved.
         interface->mOwner->getOwner()->setTransform(mState.getTransform());

         // If we're doing server simulation then we need
         // to send the client a state update.
         /*if (interface->mIsServer && interface->mPhysicsRep && !smNoCorrections &&
            !PHYSICSMGR->isSinglePlayer() // SINGLE PLAYER HACK!!!!
            )
            interface->setMaskBits(StateMask);*/
      //}
   //}
   //else
   {
      Point3F origVelocity = interface->mRigid->linVelocity;

      // Update internal forces acting on the body.
      interface->mRigid->clearForces();
      updateForces(interface, deltaTime);

      CollisionSystemInterface* colInterface = interface->mCollisionInterface;

      // Update collision information based on our current pos.
      bool collided = false;
      if (!interface->mRigid->atRest && interface->mEnabled)
      {
         //collided = colInterface->updateCollision(deltaTime);
         collided = colInterface->updateCollisions(deltaTime, interface->mVelocity);

         // Now that all the forces have been processed, lets       
         // see if we're at rest.  Basically, if the kinetic energy of
         // the shape is less than some percentage of the energy added
         // by gravity for a short period, we're considered at rest.
         // This should really be part of the rigid class...
         if (colInterface->mCollisionList.getCount())
         {
            F32 k = interface->mRigid->getKineticEnergy();
            F32 G = interface->mGravity.len() * deltaTime;
            F32 Kg = 0.5 * interface->mRigid->mass * G * G;
            if (k < sRestTol * Kg && ++interface->restCount > sRestCount)
               interface->mRigid->setAtRest();
         }
         else
            interface->restCount = 0;
      }

      // Integrate forward
      if (!interface->mRigid->atRest && interface->mEnabled)
         interface->mRigid->integrate(deltaTime);

      // Check triggers and other objects that we normally don't
      // collide with.  This function must be called before notifyCollision
      // as it will queue collision.
      //checkTriggers();

      // Invoke the onCollision notify callback for all the objects
      // we've just hit.
      colInterface->handleCollisionNotifyList();

      // Server side impact script callback
      /*if (collided)
      {
         VectorF collVec = interface->mRigid->linVelocity - origVelocity;
         F32 collSpeed = collVec.len();
         if (collSpeed > mDataBlock->minImpactSpeed)
            onImpact(collVec);
      }*/

      // Water script callbacks      
      /*if (!inLiquid && mWaterCoverage != 0.0f)
      {
         onEnterLiquid_callback(getIdString(), mWaterCoverage, mLiquidType.c_str());
         inLiquid = true;
      }
      else if (inLiquid && mWaterCoverage == 0.0f)
      {
         onLeaveLiquid_callback(getIdString(), mLiquidType.c_str());
         inLiquid = false;
      }*/
   }
}

//
void PhysicsSystem::setVelocity(PhysicsSystemInterface* interface, const VectorF& vel)
{
   // Clamp against the maximum velocity.
   if (interface->mMaxVelocity > 0)
   {
      F32 len = interface->mVelocity.magnitudeSafe();
      if (len > interface->mMaxVelocity)
      {
         Point3F excess = interface->mVelocity * (1.0f - (interface->mMaxVelocity / len));
         interface->mVelocity -= excess;
      }
   }
}