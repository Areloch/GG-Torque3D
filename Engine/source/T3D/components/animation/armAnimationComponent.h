#pragma once

#include "animationComponent.h"

class ArmAnimationComponent : public AnimationComponent
{
   typedef AnimationComponent Parent;

   struct ArmAnimation {
      U32 action;
      TSThread* thread;
   } mArmAnimation;

   TSThread* mArmThread;

   TSThread* mHeadVThread;
   TSThread* mHeadHThread;
   TSThread* mRecoilThread;

   bool         overrideLookAnimation;
   F32          armLookOverridePos;
   F32          headVLookOverridePos;
   F32          headHLookOverridePos;

   Vector<StringTableEntry> mActiveTags;

   F32 mMinLookAngle;
   F32 mMaxLookAngle;
   F32 mMaxFreelookAngle;

   S32 mNewAnimationTickTime;
   F32 mAnimationTransitionTime;
   bool mUseAnimationTransitions;

   Point3F head;
   VectorF headVec;

public:
   void         setLookAnimationOverride(bool flag);

public:
   ArmAnimationComponent();
   ~ArmAnimationComponent();

   static void initPersistFields();

   virtual bool onAdd();
   virtual void onRemove();

   virtual void onComponentAdd();
   virtual void componentAddedToOwner(Component *comp);
   virtual void componentRemovedFromOwner(Component *comp);

   virtual void targetShapeChanged(RenderComponentInterface* instanceInterface);

   virtual void processTick();
   virtual void interpolateTick(F32 dt);
   virtual void advanceTime(F32 dt);

   ///Update head animation
   void updateLookAnimation(F32 dT = 0.f);

   ///Update other animations
   void updateAnimation(F32 dt);
   void updateAnimationTree(bool firstPerson);

   virtual U32 getArmAction() const { return mArmAnimation.action; }
   virtual bool setArmThread(U32 action);

   /// Are we in the process of dying?
   bool inDeathAnim();
   F32  deathDelta(Point3F &delta);
   void updateDeathOffsets();

   const String& getArmThread() const;
   bool setArmThread(const char* sequence);
};
