#pragma once
#include "scene/sceneRenderState.h"
#include "T3D/systems/componentSystem.h"
#include "ts/tsShape.h"
#include "ts/tsShapeInstance.h"
#include "T3D/assets/ShapeAsset.h"
#include "T3D/assets/MaterialAsset.h"

#ifndef _GFXVERTEXBUFFER_H_
#include "gfx/gfxVertexBuffer.h"
#endif
#ifndef _GFXPRIMITIVEBUFFER_H_
#include "gfx/gfxPrimitiveBuffer.h"
#endif
#ifndef _OPTIMIZEDPOLYLIST_H_
#include "collision/optimizedPolyList.h"
#endif

#include "T3D/systems/collision/collisionSystem.h"

#ifndef _T3D_PHYSICS_PHYSICSPLAYER_H_
#include "T3D/physics/physicsPlayer.h"
#endif
#ifndef _T3D_PHYSICS_PHYSICSBODY_H_
#include "T3D/physics/physicsBody.h"
#endif
#ifndef _RIGID_H_
#include "T3D/rigid.h"
#endif

struct NetworkPhysicsStateDelta
{
   Move move;                    ///< Last move from server
   F32 dt;                       ///< Last interpolation time
                                 // Interpolation data
   Point3F pos;
   Point3F posVec;
   QuatF rot[2];
   // Warp data
   S32 warpTicks;                ///< Number of ticks to warp
   S32 warpCount;                ///< Current pos in warp
   Point3F warpOffset;
   QuatF warpRot[2];
};

class PhysicsSystemInterface : public SystemInterface<PhysicsSystemInterface>
{
public:
   enum PhysicsBehaviorType
   {
      Simple = 0,
      Player,  //We want this object to use controller-based player-style physics
      RigidBody
   };

   bool mEnabled;

   MatrixF mTransform;
   Box3F mBounds;

   /// If we're manipulating this in a kinematic way, then we can feed off this value to control the movement direction
   Move* moveEvent;
   VectorF mMoveSpeed;


   VectorF mImpulse;
   F32 mMaxVelocity;
   VectorF mVelocity;

   //Properties
   F32 mMass;
   F32 mAngularDamping;
   F32 mLinearDamping;
   F32 mBuoyancy;
   F32 mBouyancyDensity;
   F32 mWaterDampingScale;
   F32 mDensity;

   VectorF mGravity;
   F32 mGravityMod;

   F32 mDrag;

   S32 csmAtRestTimer;
   F32 sAtRestVelocity;      // Min speed after collision

   F32 mVertFactor;

   bool mFalling;

   //Our rigidbody/physics body containers
   //Player
   PhysicsPlayer* mPlayerPhysicsRep;
   //Rigid
   Rigid* mRigid;
   PhysicsBody* mPhysicsRep;
   PhysicsWorld* mPhysicsWorld;

   S32 restCount;

   CollisionSystemInterface* mCollisionInterface;

   NetworkPhysicsStateDelta mDelta;

   PhysicsBehaviorType mBehaviorType;

   PhysicsSystemInterface(Component* owner) : SystemInterface(owner), 
      mRigid(nullptr), 
      mPhysicsRep(nullptr)
   {
      mDelta.pos = mDelta.posVec = Point3F::Zero;
      mDelta.warpTicks = mDelta.warpCount = 0;
      mDelta.rot[0].identity();
      mDelta.rot[1].identity();
      mDelta.dt = 1;
   }

   ~PhysicsSystemInterface()
   {
      SAFE_DELETE(mPhysicsRep);
      SAFE_DELETE(mRigid);
   }
};

class PhysicsSystem
{
protected:
   

public:
   static void processTick();

   static void updateMove(PhysicsSystemInterface* interface, const Move* moveEvent);

   static void updateForces(PhysicsSystemInterface* interface, F32 deltaTime);

   static void updateSimpleForces(PhysicsSystemInterface* interface, F32 deltaTime);
   static void updatePlayerForces(PhysicsSystemInterface* interface, F32 deltaTime);
   static void updateRigidForces(PhysicsSystemInterface* interface, F32 deltaTime);

   static void updatePosition(PhysicsSystemInterface* interface, F32 deltaTime);

   static void updateSimplePosition(PhysicsSystemInterface* interface, F32 deltaTime);
   static void updatePlayerPosition(PhysicsSystemInterface* interface, F32 deltaTime);
   static void updateRigidPosition(PhysicsSystemInterface* interface, F32 deltaTime);

   void setVelocity(PhysicsSystemInterface* interface, const VectorF& vel);
};