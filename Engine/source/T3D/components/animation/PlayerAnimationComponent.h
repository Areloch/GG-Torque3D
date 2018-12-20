#pragma once

#include "animationComponent.h"

class PlayerAnimationComponent : public AnimationComponent
{
   typedef AnimationComponent Parent;

   struct ActionAnimationDef 
   {
      const char* name;       ///< Sequence name
      S32         sequence;      ///< Sequence index
      VectorF     dir;            ///< Dir of animation ground transform
      F32         speed;         ///< Speed in m/s
      bool        velocityScale; ///< Scale animation by velocity
      bool        death;         ///< Are we dying?
      Vector<StringTableEntry> tags;
   };

   Vector<ActionAnimationDef> mActionAnimationList;

   struct ActionAnimation {
      S32 action;
      TSThread* thread;
      S32 delayTicks;               // before picking another.
      bool forward;
      bool firstPerson;
      bool waitForEnd;
      bool holdAtEnd;
      bool animateOnServer;
      bool atEnd;
   } mActionAnimation;

   bool         overrideLookAnimation;
   F32          armLookOverridePos;
   F32          headVLookOverridePos;
   F32          headHLookOverridePos;

   Vector<StringTableEntry> mActiveTags;

   S32 mNewAnimationTickTime;
   F32 mAnimationTransitionTime;
   bool mUseAnimationTransitions;

   VectorF mVelocity;

   Point3F head;
   VectorF headVec;

public:
   void         setLookAnimationOverride(bool flag);

public:
   PlayerAnimationComponent();
   ~PlayerAnimationComponent();
   DECLARE_CONOBJECT(PlayerAnimationComponent);

   static void initPersistFields();

   virtual bool onAdd();
   virtual void onRemove();

   virtual void onComponentAdd();
   virtual void componentAddedToOwner(Component *comp);
   virtual void componentRemovedFromOwner(Component *comp);

   virtual void targetShapeChanged(RenderComponentInterface* instanceInterface);

   void addAction(String animName, VectorF direction, String tags);

   virtual void processTick();
   virtual void interpolateTick(F32 dt);
   virtual void advanceTime(F32 dt);
   //
   //
   void getGroundInfo(TSShapeInstance*, TSThread*, ActionAnimationDef*);

   ///Update other animations
   void updateAnimation(F32 dt);
   void updateAnimationTree(bool firstPerson);

   virtual void setActionThread(U32 action, bool forward, bool hold = false, bool wait = false, bool fsp = false, bool forceSet = false);
   virtual void updateActionThread();
   //virtual void pickBestMoveAction(U32 startAnim, U32 endAnim, U32 * action, bool * forward) const;
   virtual void pickActionAnimation();

   /// Are we in the process of dying?
   bool inDeathAnim();
   F32  deathDelta(Point3F &delta);
   void updateDeathOffsets();

   bool setActionThread(const char* sequence, bool hold, bool wait, bool fsp = false);

   void setActiveTags(String tags);
};
