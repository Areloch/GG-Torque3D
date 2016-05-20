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
#include "T3D/physics/torque/T3DPhysicsBody.h"
#include "T3D/physics/torque/T3DPhysicsWorld.h"
#include "T3D/physics/torque/T3DPhysicsCollision.h"
#include "math/mBox.h"
#include "console/console.h"


T3DPhysBody::T3DPhysBody() :
mActor(NULL),
mWorld(NULL),
mMass(0.0f),
mCompound(NULL),
mCenterOfMass(NULL),
mInvCenterOfMass(NULL),
mIsDynamic(false),
mIsEnabled(false)
{
}

T3DPhysBody::~T3DPhysBody()
{
   _releaseActor();
}

void T3DPhysBody::_releaseActor()
{
   if (mActor)
   {
      mWorld->getDynamicsWorld()->removeRigidBody(mActor);
      mActor->setUserPointer(NULL);
      SAFE_DELETE(mActor);
   }

   SAFE_DELETE(mCompound);
   SAFE_DELETE(mCenterOfMass);
   SAFE_DELETE(mInvCenterOfMass);

   mColShape = NULL;
}

bool T3DPhysBody::init(PhysicsCollision *shape,
   F32 mass,
   U32 bodyFlags,
   SceneObject *obj,
   PhysicsWorld *world)
{
   AssertFatal(obj, "T3DPhysBody::init - Got a null scene object!");
   AssertFatal(world, "T3DPhysBody::init - Got a null world!");
   AssertFatal(dynamic_cast<BtWorld*>(world), "T3DPhysBody::init - The world is the wrong type!");
   AssertFatal(shape, "T3DPhysBody::init - Got a null collision shape!");
   AssertFatal(dynamic_cast<BtCollision*>(shape), "T3DPhysBody::init - The collision shape is the wrong type!");
   AssertFatal(((BtCollision*)shape)->getShape(), "T3DPhysBody::init - Got empty collision shape!");

   // Cleanup any previous actor.
   _releaseActor();

   mWorld = (BtWorld*)world;

   mColShape = (BtCollision*)shape;
   btCollisionShape *btColShape = mColShape->getShape();
   MatrixF localXfm = mColShape->getLocalTransform();
   btVector3 localInertia(0, 0, 0);

   // If we have a mass then we're dynamic.
   mIsDynamic = mass > 0.0f;
   if (mIsDynamic)
   {
      if (btColShape->isCompound())
      {
         btCompoundShape *btCompound = (btCompoundShape*)btColShape;

         btScalar *masses = new btScalar[btCompound->getNumChildShapes()];
         for (U32 j = 0; j < btCompound->getNumChildShapes(); j++)
            masses[j] = mass / btCompound->getNumChildShapes();

         btVector3 principalInertia;
         btTransform principal;
         btCompound->calculatePrincipalAxisTransform(masses, principal, principalInertia);
         delete[] masses;

         // Create a new compound with the shifted children.
         btColShape = mCompound = new btCompoundShape();
         for (U32 i = 0; i < btCompound->getNumChildShapes(); i++)
         {
            btTransform newChildTransform = principal.inverse() * btCompound->getChildTransform(i);
            mCompound->addChildShape(newChildTransform, btCompound->getChildShape(i));
         }

         localXfm = btCast<MatrixF>(principal);
      }

      // Note... this looks like we're changing the shape, but 
      // we're not.  All this does is ask the shape to calculate the
      // local inertia vector from the mass... the shape doesn't change.
      btColShape->calculateLocalInertia(mass, localInertia);
   }

   // If we have a local transform then we need to
   // store it and the inverse to offset the center
   // of mass from the graphics origin.
   if (!localXfm.isIdentity())
   {
      mCenterOfMass = new MatrixF(localXfm);
      mInvCenterOfMass = new MatrixF(*mCenterOfMass);
      mInvCenterOfMass->inverse();
   }

   mMass = mass;
   mActor = new btRigidBody(mass, NULL, btColShape, localInertia);

   int btFlags = mActor->getCollisionFlags();

   if (bodyFlags & BF_TRIGGER)
      btFlags |= btCollisionObject::CF_NO_CONTACT_RESPONSE;
   if (bodyFlags & BF_KINEMATIC)
   {
      btFlags &= ~btCollisionObject::CF_STATIC_OBJECT;
      btFlags |= btCollisionObject::CF_KINEMATIC_OBJECT;
   }

   mActor->setCollisionFlags(btFlags);

   mWorld->getDynamicsWorld()->addRigidBody(mActor);
   mIsEnabled = true;

   mUserData.setObject(obj);
   mUserData.setBody(this);
   mActor->setUserPointer(&mUserData);

   return true;
}

void T3DPhysBody::setMaterial(F32 restitution,
   F32 friction,
   F32 staticFriction)
{
   AssertFatal(mActor, "T3DPhysBody::setMaterial - The actor is null!");

   mActor->setRestitution(restitution);

   // TODO: Weird.. Bullet doesn't have seperate dynamic 
   // and static friction.
   //
   // Either add it and submit it as an official patch
   // or hack it via contact reporting or something
   // like that.

   mActor->setFriction(friction);

   // Wake it up... it may need to move.
   mActor->activate();
}

void T3DPhysBody::setSleepThreshold(F32 linear, F32 angular)
{
   AssertFatal(mActor, "T3DPhysBody::setSleepThreshold - The actor is null!");
   mActor->setSleepingThresholds(linear, angular);
}

void T3DPhysBody::setDamping(F32 linear, F32 angular)
{
   AssertFatal(mActor, "T3DPhysBody::setDamping - The actor is null!");
   mActor->setDamping(linear, angular);
}

void T3DPhysBody::getState(PhysicsState *outState)
{
   AssertFatal(isDynamic(), "T3DPhysBody::getState - This call is only for dynamics!");

   // TODO: Fix this to do what we intended... to return
   // false so that the caller can early out of the state
   // hasn't changed since the last tick.

   MatrixF trans;
   if (mInvCenterOfMass)
      trans.mul(btCast<MatrixF>(mActor->getCenterOfMassTransform()), *mInvCenterOfMass);
   else
      trans = btCast<MatrixF>(mActor->getCenterOfMassTransform());

   outState->position = trans.getPosition();
   outState->orientation.set(trans);
   outState->linVelocity = btCast<Point3F>(mActor->getLinearVelocity());
   outState->angVelocity = btCast<Point3F>(mActor->getAngularVelocity());
   outState->sleeping = !mActor->isActive();

   // Bullet doesn't keep the momentum... recalc it.
   outState->momentum = (1.0f / mActor->getInvMass()) * outState->linVelocity;
}

Point3F T3DPhysBody::getCMassPosition() const
{
   AssertFatal(mActor, "T3DPhysBody::getCMassPosition - The actor is null!");
   return btCast<Point3F>(mActor->getCenterOfMassTransform().getOrigin());
}

void T3DPhysBody::setLinVelocity(const Point3F &vel)
{
   AssertFatal(mActor, "T3DPhysBody::setLinVelocity - The actor is null!");
   AssertFatal(isDynamic(), "T3DPhysBody::setLinVelocity - This call is only for dynamics!");

   mActor->setLinearVelocity(btCast<btVector3>(vel));
}

void T3DPhysBody::setAngVelocity(const Point3F &vel)
{
   AssertFatal(mActor, "T3DPhysBody::setAngVelocity - The actor is null!");
   AssertFatal(isDynamic(), "T3DPhysBody::setAngVelocity - This call is only for dynamics!");

   mActor->setAngularVelocity(btCast<btVector3>(vel));
}

Point3F T3DPhysBody::getLinVelocity() const
{
   AssertFatal(mActor, "T3DPhysBody::getLinVelocity - The actor is null!");
   AssertFatal(isDynamic(), "T3DPhysBody::getLinVelocity - This call is only for dynamics!");

   return btCast<Point3F>(mActor->getLinearVelocity());
}

Point3F T3DPhysBody::getAngVelocity() const
{
   AssertFatal(mActor, "T3DPhysBody::getAngVelocity - The actor is null!");
   AssertFatal(isDynamic(), "T3DPhysBody::getAngVelocity - This call is only for dynamics!");

   return btCast<Point3F>(mActor->getAngularVelocity());
}

void T3DPhysBody::setSleeping(bool sleeping)
{
   AssertFatal(mActor, "T3DPhysBody::setSleeping - The actor is null!");
   AssertFatal(isDynamic(), "T3DPhysBody::setSleeping - This call is only for dynamics!");

   if (sleeping)
   {
      //mActor->setCollisionFlags( mActor->getCollisionFlags() | btCollisionObject::CF_KINEMATIC_OBJECT );
      mActor->setActivationState(WANTS_DEACTIVATION);
      mActor->setDeactivationTime(0.0f);
   }
   else
   {
      //mActor->setCollisionFlags( mActor->getCollisionFlags() & ~btCollisionObject::CF_KINEMATIC_OBJECT );
      mActor->activate();
   }
}

PhysicsWorld* T3DPhysBody::getWorld()
{
   return mWorld;
}

PhysicsCollision* T3DPhysBody::getColShape()
{
   return mColShape;
}

MatrixF& T3DPhysBody::getTransform(MatrixF *outMatrix)
{
   AssertFatal(mActor, "T3DPhysBody::getTransform - The actor is null!");

   if (mInvCenterOfMass)
      outMatrix->mul(*mInvCenterOfMass, btCast<MatrixF>(mActor->getCenterOfMassTransform()));
   else
      *outMatrix = btCast<MatrixF>(mActor->getCenterOfMassTransform());

   return *outMatrix;
}

void T3DPhysBody::setTransform(const MatrixF &transform)
{
   AssertFatal(mActor, "T3DPhysBody::setTransform - The actor is null!");

   if (mCenterOfMass)
   {
      MatrixF xfm;
      xfm.mul(transform, *mCenterOfMass);
      mActor->setCenterOfMassTransform(btCast<btTransform>(xfm));
   }
   else
      mActor->setCenterOfMassTransform(btCast<btTransform>(transform));

   // If its dynamic we have more to do.
   if (isDynamic())
   {
      // Clear any velocity and forces... this is a warp.
      mActor->clearForces();
      mActor->setLinearVelocity(btVector3(0, 0, 0));
      mActor->setAngularVelocity(btVector3(0, 0, 0));
      mActor->activate();
   }
}

void T3DPhysBody::applyCorrection(const MatrixF &transform)
{
   AssertFatal(mActor, "T3DPhysBody::applyCorrection - The actor is null!");
   AssertFatal(isDynamic(), "T3DPhysBody::applyCorrection - This call is only for dynamics!");

   if (mCenterOfMass)
   {
      MatrixF xfm;
      xfm.mul(transform, *mCenterOfMass);
      mActor->setCenterOfMassTransform(btCast<btTransform>(xfm));
   }
   else
      mActor->setCenterOfMassTransform(btCast<btTransform>(transform));
}

void T3DPhysBody::applyImpulse(const Point3F &origin, const Point3F &force)
{
   AssertFatal(mActor, "T3DPhysBody::applyImpulse - The actor is null!");
   AssertFatal(isDynamic(), "T3DPhysBody::applyImpulse - This call is only for dynamics!");

   // Convert the world position to local
   MatrixF trans = btCast<MatrixF>(mActor->getCenterOfMassTransform());
   trans.inverse();
   Point3F localOrigin(origin);
   trans.mulP(localOrigin);

   if (mCenterOfMass)
   {
      Point3F relOrigin(localOrigin);
      mCenterOfMass->mulP(relOrigin);
      Point3F relForce(force);
      mCenterOfMass->mulV(relForce);
      mActor->applyImpulse(btCast<btVector3>(relForce), btCast<btVector3>(relOrigin));
   }
   else
      mActor->applyImpulse(btCast<btVector3>(force), btCast<btVector3>(localOrigin));

   if (!mActor->isActive())
      mActor->activate();
}

Box3F T3DPhysBody::getWorldBounds()
{
   btVector3 min, max;
   mActor->getAabb(min, max);

   Box3F bounds(btCast<Point3F>(min), btCast<Point3F>(max));

   return bounds;
}

void T3DPhysBody::setSimulationEnabled(bool enabled)
{
   if (mIsEnabled == enabled)
      return;

   if (!enabled)
      mWorld->getDynamicsWorld()->removeRigidBody(mActor);
   else
      mWorld->getDynamicsWorld()->addRigidBody(mActor);

   mIsEnabled = enabled;
}
