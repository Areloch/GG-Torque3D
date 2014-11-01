//This is basically a helper file that has general-usage behavior interfaces for rendering
#ifndef _COLLISION_INTERFACES_H_
#define _COLLISION_INTERFACES_H_

#ifndef _CONVEX_H_
#include "collision/convex.h"
#endif
#ifndef _COLLISION_H_
#include "collision/collision.h"
#endif
#ifndef _EARLYOUTPOLYLIST_H_
#include "collision/earlyOutPolyList.h"
#endif
#ifndef _SIM_H_
#include "console/sim.h"
#endif

#ifndef _COMPONENTINSTANCE_H_
#include "component/components/componentInstance.h"
#endif

struct ContactInfo 
{
   bool contacted, move;
   SceneObject *contactObject;
   VectorF  contactNormal;
   Point3F  contactPoint;
   F32	   contactTime;
   S32	   contactTimer;

   void clear()
   {
      contacted=move=false; 
      contactObject = NULL; 
      contactNormal.set(0,0,0);
      contactTime = 0.f;
      contactTimer = 0;
   }

   ContactInfo() { clear(); }

};

class CollisionInterface
{
public:
	// CollisionTimeout
	// This struct lets us track our collisions and estimate when they've have timed out and we'll need to act on it.
	struct CollisionTimeout 
   {
      CollisionTimeout* next;
      SceneObject* object;
      U32 objectNumber;
      SimTime expireTime;
      VectorF vector;
   };

   Signal< void( SceneObject* ) > CollisionInterface::onCollisionSignal;
   Signal< void( SceneObject* ) > CollisionInterface::onContactSignal;

protected:
   CollisionTimeout* mTimeoutList;
   static CollisionTimeout* sFreeTimeoutList;

   CollisionList mCollisionList;

   Box3F mWorkingQueryBox;

   U32 CollisionMoveMask;

   Convex *mConvexList;

   bool mBlockColliding;

   /// handleCollisionList
   /// This basically takes in a CollisionList and calls handleCollision for each.
   void handleCollisionList( CollisionList &collisionList, VectorF velocity );

   /// handleCollision
   /// This will take a collision and queue the collision info for the object so that in knows about the collision.
   void handleCollision( Collision &col, VectorF velocity );

   void queueCollision( SceneObject *obj, const VectorF &vec);

	/// checkEarlyOut
	/// This function lets you trying and early out of any expensive collision checks by using simple extruded poly boxes representing our objects
	/// If it returns true, we know we won't hit with the given parameters and can successfully early out. If it returns false, our test case collided
	/// and we should do the full collision sim.
	bool checkEarlyOut(Point3F start, VectorF velocity, F32 time, Box3F objectBox, Point3F objectScale, 
														Box3F collisionBox, U32 collisionMask, CollisionWorkingList &colWorkingList);

public:
   /// checkCollisions
   // This is our main function for checking if a collision is happening based on the start point, velocity and time
   // We do the bulk of the collision checking in here
   virtual bool checkCollisions( const F32 travelTime, Point3F *velocity, Point3F start )=0;

   CollisionList *getCollisionList() { return &mCollisionList; }

   Collision *getCollision(S32 col);

	Convex *getConvexList() { return mConvexList; }

   virtual bool buildPolyList(PolyListContext context, AbstractPolyList* polyList, const Box3F &box, const SphereF &sphere)=0;

	enum PublicConstants { 
      CollisionTimeoutValue = 250
   };

   bool doesBlockColliding() { return mBlockColliding; }
};

class BuildConvexInterface
{
public:
   virtual bool buildConvex(const Box3F& box, Convex* convex)=0;
};

class BuildPolyListInterface
{
public:
   virtual bool buildPolyList(PolyListContext context, AbstractPolyList* polyList, const Box3F &box, const SphereF &sphere)=0;
};

#endif