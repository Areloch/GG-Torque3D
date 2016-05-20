//-----------------------------------------------------------------------------
// Copyright (c) 2012 GarageGames, LLC
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.
//-----------------------------------------------------------------------------

#include "platform/platform.h"
#include "T3D/physics/torque/T3DPhysicsPlayer.h"

#include "T3D/physics/physicsPlugin.h"
#include "T3D/physics/torque/T3DPhysicsWorld.h"
#include "collision/collision.h"

T3DPhysPlayer::T3DPhysPlayer()
   : PhysicsPlayer(),
   mWorld(NULL),
   mObject(NULL),
   mGhostObject(NULL),
   mColShape(NULL),
   mOriginOffset(0.0f)
{
}

T3DPhysPlayer::~T3DPhysPlayer()
{
   _releaseController();
}

void T3DPhysPlayer::_releaseController()
{
   if (!mGhostObject)
      return;

   mWorld->getDynamicsWorld()->removeCollisionObject(mGhostObject);

   SAFE_DELETE(mGhostObject);
   SAFE_DELETE(mColShape);
}

void T3DPhysPlayer::init(const char *type,
   const Point3F &size,
   F32 runSurfaceCos,
   F32 stepHeight,
   SceneObject *obj,
   PhysicsWorld *world)
{
   AssertFatal(obj, "T3DPhysPlayer::init - Got a null scene object!");
   AssertFatal(world, "T3DPhysPlayer::init - Got a null world!");
   AssertFatal(dynamic_cast<T3DPhysWorld*>(world), "T3DPhysPlayer::init - The world is the wrong type!");

   // Cleanup any previous controller.
   _releaseController();

   mObject = obj;
   mWorld = (T3DPhysWorld*)world;

   mStepHeight = stepHeight;

   //if ( dStricmp( type, "Capsule" ) == 0 )
   {
      F32 radius = getMax(size.x, size.y) * 0.5f;
      F32 height = size.z - (radius * 2.0f);
      mColShape = new btCapsuleShapeZ(radius, height);
      mColShape->setMargin(0.05f);
      mOriginOffset = (height * 0.5) + radius;
   }
   //else
   {
      //mColShape = new btBoxShape( btVector3( 0.5f, 0.5f, 1.0f ) );
      //mOriginOffset = 1.0f;
   }

   mGhostObject = new btPairCachingGhostObject();
   mGhostObject->setCollisionShape(mColShape);
   mGhostObject->setCollisionFlags(btCollisionObject::CF_CHARACTER_OBJECT);
   mWorld->getDynamicsWorld()->addCollisionObject(mGhostObject,
      btBroadphaseProxy::CharacterFilter,
      btBroadphaseProxy::StaticFilter | btBroadphaseProxy::DefaultFilter);

   mUserData.setObject(obj);
   mGhostObject->setUserPointer(&mUserData);
}

Point3F T3DPhysPlayer::move(const VectorF &disp, CollisionList &outCol)
{
   AssertFatal(mGhostObject, "T3DPhysPlayer::move - The controller is null!");

   // Try and move to new pos
   F32 totalMotion = 0.0f;

   Point3F start;
   Point3F initialPosition;
   mOwner->getTransform().getColumn(3, &start);
   initialPosition = start;

   VectorF firstNormal(0.0f, 0.0f, 0.0f);
   //F32 maxStep = maxStepHeight;
   F32 time = travelTime;
   U32 count = 0;
   S32 sMoveRetryCount = 5;

   for (; count < sMoveRetryCount; count++)
   {
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
         if (velLen > 0.f)
         {
            F32 newT = getMin(0.01f / velLen, dt);
            start -= mVelocity * newT;
            totalMotion -= velLen * newT;
         }

         // Pick the surface most parallel to the face that was hit.
         U32 colCount = colInterface->getCollisionList()->getCount();

         const Collision *collision = colInterface->getCollision(0);
         const Collision *cp = collision + 1;
         const Collision *ep = collision + colInterface->getCollisionList()->getCount();
         for (; cp != ep; cp++)
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
         }

         //check the last/first one just incase
         if (Entity* colEnt = dynamic_cast<Entity*>(collision->object))
         {
            if (CollisionInterface *colInt = colEnt->getComponent<CollisionInterface>())
            {
               if (!colInt->doesBlockColliding())
               {
                  //if our ideal surface doesn't stop us, just move along
                  return start + mVelocity * time;
               }
            }
         }

         //F32 bd = _doCollisionImpact( collision, wasFalling );
         F32 bd = -mDot(mVelocity, collision->normal);

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
               if (mDot(dv, firstNormal) < 0.0f &&
                  mDot(collision->normal, firstNormal) < 0.0f)
               {
                  VectorF nv;
                  mCross(collision->normal, firstNormal, &nv);
                  F32 nvl = nv.len();
                  if (nvl)
                  {
                     if (mDot(nv, mVelocity) < 0.0f)
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

   U32 colCountThree = colInterface->getCollisionList()->getCount();

   if (colCountThree != 0)
      bool derp = true;

   if (count == sMoveRetryCount)
   {
      // Failed to move
      start = initialPosition;
      mVelocity.set(0.0f, 0.0f, 0.0f);
   }

   return start;
}

bool T3DPhysPlayer::_recoverFromPenetration()
{
   bool penetration = false;

   btDynamicsWorld *collWorld = mWorld->getDynamicsWorld();

   collWorld->getDispatcher()->dispatchAllCollisionPairs(mGhostObject->getOverlappingPairCache(),
      collWorld->getDispatchInfo(),
      collWorld->getDispatcher());

   btVector3 currPos = mGhostObject->getWorldTransform().getOrigin();
   btScalar maxPen = 0.0f;
   btManifoldArray manifoldArray;

   for (U32 i = 0; i < mGhostObject->getOverlappingPairCache()->getNumOverlappingPairs(); i++)
   {
      btBroadphasePair *collisionPair = &mGhostObject->getOverlappingPairCache()->getOverlappingPairArray()[i];

      if (((btCollisionObject*)collisionPair->m_pProxy0->m_clientObject)->getCollisionFlags() & btCollisionObject::CF_NO_CONTACT_RESPONSE ||
         ((btCollisionObject*)collisionPair->m_pProxy1->m_clientObject)->getCollisionFlags() & btCollisionObject::CF_NO_CONTACT_RESPONSE)
         continue;

      manifoldArray.resize(0);
      if (collisionPair->m_algorithm)
         collisionPair->m_algorithm->getAllContactManifolds(manifoldArray);

      for (U32 j = 0; j < manifoldArray.size(); j++)
      {
         btPersistentManifold* manifold = manifoldArray[j];
         btScalar directionSign = manifold->getBody0() == mGhostObject ? -1.0f : 1.0f;

         for (U32 p = 0; p < manifold->getNumContacts(); p++)
         {
            const btManifoldPoint&pt = manifold->getContactPoint(p);

            if (pt.getDistance() < -mColShape->getMargin())
            {
               if (pt.getDistance() < maxPen)
               {
                  maxPen = pt.getDistance();
                  //m_touchingNormal = pt.m_normalWorldOnB * directionSign;//??
               }

               currPos += pt.m_normalWorldOnB * directionSign * pt.getDistance(); // * 0.25f;
               penetration = true;
            }
            else
            {
               //printf("touching %f\n", pt.getDistance());
            }
         }

         //manifold->clearManifold();
      }
   }

   // Update the ghost transform.
   btTransform newTrans = mGhostObject->getWorldTransform();
   newTrans.setOrigin(currPos);
   mGhostObject->setWorldTransform(newTrans);

   return penetration;
}


class T3DPhysPlayerSweepCallback : public btCollisionWorld::ClosestConvexResultCallback
{
   typedef btCollisionWorld::ClosestConvexResultCallback Parent;

public:

   T3DPhysPlayerSweepCallback(btCollisionObject *me, const btVector3 &moveVec)
      : Parent(btVector3(0.0, 0.0, 0.0), btVector3(0.0, 0.0, 0.0)),
      mMe(me),
      mMoveVec(moveVec)
   {
   }

   virtual bool needsCollision(btBroadphaseProxy* proxy0) const
   {
      if (proxy0->m_clientObject == mMe)
         return false;

      return Parent::needsCollision(proxy0);
   }

   virtual btScalar addSingleResult(btCollisionWorld::LocalConvexResult &convexResult,
      bool normalInWorldSpace)
   {
      // NOTE: I shouldn't have to do any of this, but Bullet 
      // has some weird bugs.
      //
      // For one the plane type will return hits on a Z up surface
      // for sweeps that have no Z sweep component.
      //
      // Second the normal returned here is sometimes backwards
      // to the sweep direction... no clue why.
      //
      F32 dotN = mMoveVec.dot(convexResult.m_hitNormalLocal);
      if (mFabs(dotN) < 0.1f)
         return 1.0f;

      if (convexResult.m_hitCollisionObject->getCollisionFlags() & btCollisionObject::CF_NO_CONTACT_RESPONSE)
         return 1.0f;

      return Parent::addSingleResult(convexResult, normalInWorldSpace);
   }

protected:
   btVector3 mMoveVec;
   btCollisionObject *mMe;
};

bool T3DPhysPlayer::_sweep(btVector3 *inOutCurrPos, const btVector3 &disp, CollisionList *outCol)
{
   btTransform start(btTransform::getIdentity());
   start.setOrigin(*inOutCurrPos);

   btTransform end(btTransform::getIdentity());
   end.setOrigin(*inOutCurrPos + disp);

   T3DPhysPlayerSweepCallback callback(mGhostObject, disp.normalized());
   callback.m_collisionFilterGroup = mGhostObject->getBroadphaseHandle()->m_collisionFilterGroup;
   callback.m_collisionFilterMask = mGhostObject->getBroadphaseHandle()->m_collisionFilterMask;

   if (disp.length()>0.0001)
      mGhostObject->convexSweepTest(mColShape, start, end, callback, 0.0f);

   inOutCurrPos->setInterpolate3(start.getOrigin(), end.getOrigin(), callback.m_closestHitFraction);
   if (callback.hasHit())
   {
      if (outCol)
      {
         Collision& col = outCol->increment();
         dMemset(&col, 0, sizeof(col));

         col.normal = btCast<Point3F>(callback.m_hitNormalWorld);
         col.object = PhysicsUserData::getObject(callback.m_hitCollisionObject->getUserPointer());

         if (disp.z() < 0.0f)
         {
            // We're sweeping down as part of the stepping routine.    In this
            // case we want to have the collision normal only point in the opposite direction.
            // i.e. up  If we include the sideways part of the normal then the Player class
            // velocity calculations using this normal will affect the player's forwards
            // momentum.  This is especially noticable on stairs as the rounded bottom of
            // the capsule slides up the corner of a stair.
            col.normal.set(0.0f, 0.0f, 1.0f);
         }
      }

      return true;
   }

   return false;
}

void T3DPhysPlayer::_stepForward(btVector3 *inOutCurrPos, const btVector3 &displacement, CollisionList *outCol)
{
   btTransform start(btTransform::getIdentity());
   btTransform end(btTransform::getIdentity());
   F32 fraction = 1.0f;
   S32 maxIter = 10;
   btVector3 disp = displacement;

   while (fraction > 0.01f && maxIter-- > 0)
   {
      // Setup the sweep start and end transforms.
      start.setOrigin(*inOutCurrPos);
      end.setOrigin(*inOutCurrPos + disp);

      T3DPhysPlayerSweepCallback callback(mGhostObject, disp.length2() > 0.0f ? disp.normalized() : disp);
      callback.m_collisionFilterGroup = mGhostObject->getBroadphaseHandle()->m_collisionFilterGroup;
      callback.m_collisionFilterMask = mGhostObject->getBroadphaseHandle()->m_collisionFilterMask;

      if (disp.length()>0.0001)
         mGhostObject->convexSweepTest(mColShape, start, end, callback, 0.0f);

      // Subtract from the travel fraction.
      fraction -= callback.m_closestHitFraction;

      // Did we get a hit?
      if (callback.hasHit())
      {
         /*
         // Get the real hit normal... Bullet returns the 'seperating normal' and not
         // the normal of the hit object.
         btTransform rayStart( btTransform::getIdentity() );
         rayStart.setOrigin( callback.m_hitPointWorld + callback.m_hitNormalWorld );
         btTransform rayEnd( btTransform::getIdentity() );
         rayEnd.setOrigin( callback.m_hitPointWorld - callback.m_hitNormalWorld );

         btCollisionWorld::ClosestRayResultCallback rayHit( rayStart.getOrigin(), rayEnd.getOrigin() );
         mWorld->getDynamicsWorld()->rayTestSingle(   rayStart,
         rayEnd,
         callback.m_hitCollisionObject,
         callback.m_hitCollisionObject->getCollisionShape(),
         callback.m_hitCollisionObject->getWorldTransform(),
         rayHit );

         if ( !rayHit.hasHit() )
         break;
         */

         Collision& col = outCol->increment();
         dMemset(&col, 0, sizeof(col));

         col.normal = btCast<Point3F>(callback.m_hitNormalWorld);
         col.object = PhysicsUserData::getObject(callback.m_hitCollisionObject->getUserPointer());

         // If the collision direction is sideways then modify the collision normal
         // to remove any z component.  This takes care of any sideways collisions
         // with the round bottom of the capsule when it comes to the Player class
         // velocity calculations.  We want all sideways collisions to be treated
         // as if they hit the side of a cylinder.
         if (col.normal.z > 0.0f)
         {
            // This will only remove the z component of the collision normal
            // for the bottom of the character controller, which would hit during
            // a step.  We'll leave the top hemisphere of the character's capsule
            // alone as bumping one's head is an entirely different story.  This
            // helps with low doorways.
            col.normal.z = 0.0f;
            col.normal.normalizeSafe();
         }

         // Interpolate to the new position.
         inOutCurrPos->setInterpolate3(start.getOrigin(), end.getOrigin(), callback.m_closestHitFraction);

         // Subtract out the displacement along the collision normal.
         F32 bd = -disp.dot(callback.m_hitNormalWorld);
         btVector3 dv = callback.m_hitNormalWorld * bd;
         disp += dv;
      }
      else
      {
         // we moved whole way
         *inOutCurrPos = end.getOrigin();
         break;
      }
   }
}

void T3DPhysPlayer::findContact(SceneObject **contactObject,
   VectorF *contactNormal,
   Vector<SceneObject*> *outOverlapObjects) const
{
   AssertFatal(mGhostObject, "T3DPhysPlayer::findContact - The controller is null!");

   VectorF normal;
   F32 maxDot = -1.0f;

   // Go thru the contact points... get the first contact.
   btHashedOverlappingPairCache *pairCache = mGhostObject->getOverlappingPairCache();
   btBroadphasePairArray& pairArray = pairCache->getOverlappingPairArray();
   U32 numPairs = pairArray.size();
   btManifoldArray manifoldArray;

   for (U32 i = 0; i < numPairs; i++)
   {
      const btBroadphasePair &pair = pairArray[i];

      btBroadphasePair *collisionPair = pairCache->findPair(pair.m_pProxy0, pair.m_pProxy1);
      if (!collisionPair || !collisionPair->m_algorithm)
         continue;

      btCollisionObject *other = (btCollisionObject*)pair.m_pProxy0->m_clientObject;
      if (other == mGhostObject)
         other = (btCollisionObject*)pair.m_pProxy1->m_clientObject;

      if (!outOverlapObjects->contains(PhysicsUserData::getObject(other->getUserPointer())))
         outOverlapObjects->push_back(PhysicsUserData::getObject(other->getUserPointer()));

      if (other->getCollisionFlags() & btCollisionObject::CF_NO_CONTACT_RESPONSE)
         continue;

      manifoldArray.clear();
      collisionPair->m_algorithm->getAllContactManifolds(manifoldArray);

      for (U32 j = 0; j < manifoldArray.size(); j++)
      {
         btPersistentManifold *manifold = manifoldArray[j];
         btScalar directionSign = manifold->getBody0() == mGhostObject ? 1.0f : -1.0f;

         for (U32 p = 0; p < manifold->getNumContacts(); p++)
         {
            const btManifoldPoint &pt = manifold->getContactPoint(p);

            // Test the normal... is it the most vertical one we got?
            normal = btCast<Point3F>(pt.m_normalWorldOnB * directionSign);
            F32 dot = mDot(normal, VectorF(0, 0, 1));
            if (dot > maxDot)
            {
               maxDot = dot;

               btCollisionObject *colObject = (btCollisionObject*)collisionPair->m_pProxy0->m_clientObject;
               *contactObject = PhysicsUserData::getObject(colObject->getUserPointer());
               *contactNormal = normal;
            }
         }
      }
   }
}

void T3DPhysPlayer::enableCollision()
{
   AssertFatal(mGhostObject, "T3DPhysPlayer::enableCollision - The controller is null!");

   //mController->setCollision( true );   
}

void T3DPhysPlayer::disableCollision()
{
   AssertFatal(mGhostObject, "T3DPhysPlayer::disableCollision - The controller is null!");

   //mController->setCollision( false );   
}

PhysicsWorld* T3DPhysPlayer::getWorld()
{
   return mWorld;
}

void T3DPhysPlayer::setTransform(const MatrixF &transform)
{
   AssertFatal(mGhostObject, "T3DPhysPlayer::setTransform - The ghost object is null!");

   btTransform xfm = btCast<btTransform>(transform);
   xfm.getOrigin()[2] += mOriginOffset;

   mGhostObject->setWorldTransform(xfm);
}

MatrixF& T3DPhysPlayer::getTransform(MatrixF *outMatrix)
{
   AssertFatal(mGhostObject, "T3DPhysPlayer::getTransform - The ghost object is null!");

   *outMatrix = btCast<MatrixF>(mGhostObject->getWorldTransform());
   *outMatrix[11] -= mOriginOffset;

   return *outMatrix;
}

void T3DPhysPlayer::setScale(const Point3F &scale)
{
}
