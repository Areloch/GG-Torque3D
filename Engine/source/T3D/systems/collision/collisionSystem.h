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
#ifndef _CONVEX_H_
#include "collision/convex.h"
#endif
#ifndef _T3D_PHYSICSCOMMON_H_
#include "T3D/physics/physicsCommon.h"
#endif
#ifndef _T3D_PHYSICS_PHYSICSWORLD_H_
#include "T3D/physics/physicsWorld.h"
#endif

class CollisionSystemInterface;

struct ContactInfo
{
   bool contacted, move;
   SceneObject *contactObject;
   VectorF  idealContactNormal;
   VectorF  contactNormal;
   Point3F  contactPoint;
   F32	   contactTime;
   S32	   contactTimer;
   BaseMatInstance *contactMaterial;

   void clear()
   {
      contacted = move = false;
      contactObject = NULL;
      contactNormal.set(0, 0, 0);
      contactTime = 0.f;
      contactTimer = 0;
      idealContactNormal.set(0, 0, 1);
      contactMaterial = NULL;
   }

   ContactInfo() { clear(); }
};

/// CollisionTimeout
/// This struct lets us track our collisions and estimate when they've have timed out and we'll need to act on it.
struct CollisionTimeout
{
   CollisionTimeout* next;
   SceneObject* object;
   U32 objectNumber;
   SimTime expireTime;
   VectorF vector;
};

class CollisionSystem
{
protected:

public:
   enum PublicConstants {
      CollisionTimeoutValue = 250
   };

   //void renderConvex(ObjectRenderInst *ri, SceneRenderState *state, BaseMatInstance *overrideMat);

   //void prepCollision();
   void _updatePhysics();

   //
   /// checkCollisions
   // This is our main function for checking if a collision is happening based on the start point, velocity and time
   // We do the bulk of the collision checking in here
   static bool checkCollisions(CollisionSystemInterface* colInterface, const F32 travelTime, Point3F *velocity, Point3F start);

   //bool buildPolyList(PolyListContext context, AbstractPolyList* polyList, const Box3F &, const SphereF &);

   bool castRay(const Point3F &start, const Point3F &end, RayInfo* info);

   static bool updateCollisions(CollisionSystemInterface* colInterface, F32 time, VectorF velocity);

   static void updateWorkingCollisionSet(CollisionSystemInterface* colInterface, const VectorF velocity, const U32 mask);

   //
   /// handleCollisionList
   /// This basically takes in a CollisionList and calls handleCollision for each.
   static void handleCollisionList(CollisionSystemInterface* colInterface, VectorF velocity);

   /// handleCollision
   /// This will take a collision and queue the collision info for the object so that in knows about the collision.
   static void handleCollision(CollisionSystemInterface* colInterface, Collision &col, VectorF velocity);

   static void handleCollisionNotifyList(CollisionSystemInterface* colInterface);

   static void queueCollision(CollisionSystemInterface* colInterface, SceneObject *obj, const VectorF &vec);

   /// checkEarlyOut
   /// This function lets you trying and early out of any expensive collision checks by using simple extruded poly boxes representing our objects
   /// If it returns true, we know we won't hit with the given parameters and can successfully early out. If it returns false, our test case collided
   /// and we should do the full collision sim.
   static bool checkEarlyOut(Point3F start, VectorF velocity, F32 time, Box3F objectBox, Point3F objectScale,
      Box3F collisionBox, U32 collisionMask, CollisionWorkingList &colWorkingList);

   static CollisionList *getCollisionList(CollisionSystemInterface* colInterface);

   static void clearCollisionList(CollisionSystemInterface* colInterface);

   static void clearCollisionNotifyList(CollisionSystemInterface* colInterface);

   static Collision* getCollision(CollisionSystemInterface* colInterface, S32 col);

   static ContactInfo* getContactInfo(CollisionSystemInterface* colInterface);

   static Convex *getConvexList(CollisionSystemInterface* colInterface);

   static void buildConvex(const Box3F& box, Convex* convex);

   static bool buildPolyList(PolyListContext context, AbstractPolyList* polyList, const Box3F &box, const SphereF &sphere);

   static bool doesBlockColliding(CollisionSystemInterface* colInterface);
};

class CollisionSystemInterface : public SystemInterface<CollisionSystemInterface>
{
public:
	Convex *mConvexList;
	PhysicsBody* mPhysicsRep;

	//Used for visible mesh or collisionmesh stuff
	TSShapeInstance* mShapeInstance;

	bool mUsePhysicsAPI;

	VectorF mVelocity;
	F32 mMass;

	bool mAwake;

	CollisionTimeout* mTimeoutList;
	static CollisionTimeout* sFreeTimeoutList;

	CollisionList mCollisionList;
	Vector<CollisionSystemInterface*> mCollisionNotifyList;

	ContactInfo mContactInfo;

	Box3F mWorkingQueryBox;

	U32 CollisionMoveMask;

	bool mBlockColliding;

	Signal< void(SceneObject*) > CollisionSystemInterface::onCollisionSignal;
	Signal< void(SceneObject*) > CollisionSystemInterface::onContactSignal;

   CollisionSystemInterface(Component* owner) : SystemInterface(owner), 
	   mConvexList(nullptr),
	   mPhysicsRep(nullptr), 
	   mShapeInstance(nullptr),
	   mAwake(false), 
	   mUsePhysicsAPI(false)
   {
   }

   ~CollisionSystemInterface()
   {
      SAFE_DELETE(mPhysicsRep);
      SAFE_DELETE(mConvexList);
	  SAFE_DELETE(mShapeInstance);
   }

   inline bool checkCollisions(const F32 travelTime, Point3F *velocity, Point3F start)
   {
      return CollisionSystem::checkCollisions(this, travelTime, velocity, start);
   }

   inline bool updateCollisions(F32 time, VectorF velocity)
   {
      return CollisionSystem::updateCollisions(this, time, velocity);
   }

   inline CollisionList *getCollisionList()
   {
      return CollisionSystem::getCollisionList(this);
   }

   inline void handleCollisionList(VectorF velocity)
   {
      CollisionSystem::handleCollisionList(this, velocity);
   }

   inline Collision* getCollision(S32 col)
   {
      return CollisionSystem::getCollision(this, col);
   }

   inline void handleCollisionNotifyList()
   {
      CollisionSystem::handleCollisionNotifyList(this);
   }
};