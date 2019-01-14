#pragma once

#include "T3D/components/component.h"

#ifdef TORQUE_NAVIGATION_ENABLED
#include "navigation/navPath.h"
#include "navigation/navMesh.h"
#include "navigation/coverPoint.h"
#endif // TORQUE_NAVIGATION_ENABLED

class AIControllerComponent : public Component
{
   typedef Component Parent;

public:
   enum MoveState {
      ModeStop,                       // AI has stopped moving.
      ModeMove,                       // AI is currently moving.
      ModeStuck,                      // AI is stuck, but wants to move.
      ModeSlowing,                    // AI is slowing down as it reaches it's destination.
   };

private:
   MoveState mMoveState;
   F32 mMoveSpeed;
   F32 mMoveTolerance;                 // Distance from destination before we stop
   F32 mAttackRadius;                  // Distance to trigger weaponry calcs
   Point3F mMoveDestination;           // Destination for movement
   Point3F mLastLocation;              // For stuck check
   F32 mMoveStuckTolerance;            // Distance tolerance on stuck check
   S32 mMoveStuckTestDelay;            // The number of ticks to wait before checking if the AI is stuck
   S32 mMoveStuckTestCountdown;        // The current countdown until at AI starts to check if it is stuck
   bool mMoveSlowdown;                 // Slowdown as we near the destination

   SimObjectPtr<GameBase> mAimObject; // Object to point at, overrides location
   bool mAimLocationSet;               // Has an aim location been set?
   Point3F mAimLocation;               // Point to look at
   bool mTargetInLOS;                  // Is target object visible?

   Point3F mAimOffset;

   MatrixF mEyeTransform;
   MatrixF mMuzzleTransform;

   // move triggers
   bool mMoveTriggers[MaxTriggerKeys];

   // Utility Methods
   void throwCallback(const char *name);

#ifdef TORQUE_NAVIGATION_ENABLED
   /// Should we jump?
   enum JumpStates {
      None,  ///< No, don't jump.
      Now,   ///< Jump immediately.
      Ledge, ///< Jump when we walk off a ledge.
   } mJump;

   /// Stores information about a path.
   struct PathData {
      /// Pointer to path object.
      SimObjectPtr<NavPath> path;
      /// Do we own our path? If so, we will delete it when finished.
      bool owned;
      /// Path node we're at.
      U32 index;
      /// Default constructor.
      PathData() : path(NULL)
      {
         owned = false;
         index = 0;
      }
   };

   /// Path we are currently following.
   PathData mPathData;


   /// Get the current path we're following.
   NavPath *getPath() { return mPathData.path; }

   /// Stores information about our cover.
   struct CoverData {
      /// Pointer to a cover point.
      SimObjectPtr<CoverPoint> cover;
      /// Default constructor.
      CoverData() : cover(NULL) {}
   };

   /// Current cover we're trying to get to.
   CoverData mCoverData;


   /// Information about a target we're following.
   struct FollowData {
      /// Object to follow.
      SimObjectPtr<SceneObject> object;
      /// Distance at whcih to follow.
      F32 radius;
      Point3F lastPos;
      /// Default constructor.
      FollowData() : object(NULL)
      {
         radius = 5.0f;
         lastPos = Point3F::Zero;
      }
   };

   /// Current object we're following.
   FollowData mFollowData;


   /// NavMesh we pathfind on.
   SimObjectPtr<NavMesh> mNavMesh;

   /// Move to the specified node in the current path.
   void moveToNode(S32 node);
#endif // TORQUE_NAVIGATION_ENABLED

protected:
   virtual void onReachDestination();
   virtual void onStuck();

public:
   AIControllerComponent();
   ~AIControllerComponent();

   DECLARE_CONOBJECT(AIControllerComponent);

   virtual void processTick();

   virtual bool getAIMove(Move *move);
   virtual void updateMove(const Move *move);
   /// Clear out the current path.
   void clearPath();
   /// Stop searching for cover.
   void clearCover();
   /// Stop following me!
   void clearFollow();

   // Targeting and aiming sets/gets
   void setAimObject(GameBase *targetObject);
   void setAimObject(GameBase *targetObject, const Point3F& offset);
   GameBase* getAimObject() const { return mAimObject; }
   void setAimLocation(const Point3F &location);
   Point3F getAimLocation() const { return mAimLocation; }
   void clearAim();
   void getMuzzleVector(VectorF* vec);
   bool checkInLos(GameBase* target = NULL, bool _useMuzzle = false);
   bool checkInFoV(GameBase* target = NULL, F32 camFov = 45.0f);
   F32 getTargetDistance(GameBase* target);
   MatrixF getEyeTransform();
   Point3F getMuzzlePointAI();

   // Movement sets/gets
   void setMoveSpeed(const F32 speed);
   F32 getMoveSpeed() const { return mMoveSpeed; }
   void setMoveTolerance(const F32 tolerance);
   F32 getMoveTolerance() const { return mMoveTolerance; }
   void setMoveDestination(const Point3F &location, bool slowdown);
   Point3F getMoveDestination() const { return mMoveDestination; }
   void stopMove();

   // Trigger sets/gets
   void setMoveTrigger(U32 slot, const bool isSet = true);
   bool getMoveTrigger(U32 slot) const;
   void clearMoveTriggers();

#ifdef TORQUE_NAVIGATION_ENABLED
   /// @name Pathfinding
   /// @{

   enum NavSize {
      Small,
      Regular,
      Large
   } mNavSize;
   void setNavSize(NavSize size) { mNavSize = size; updateNavMesh(); }
   NavSize getNavSize() const { return mNavSize; }

   bool setPathDestination(const Point3F &pos);
   Point3F getPathDestination() const;

   void followNavPath(NavPath *path);
   void followObject(SceneObject *obj, F32 radius);

   void repath();

   bool findCover(const Point3F &from, F32 radius);

   NavMesh *findNavMesh() const;
   void updateNavMesh();
   NavMesh *getNavMesh() const { return mNavMesh; }

   /// Get cover we are moving to.
   CoverPoint *getCover() { return mCoverData.cover; }

   /// Types of link we can use.
   LinkData mLinkTypes;

   /// @}
#endif // TORQUE_NAVIGATION_ENABLED
};