#include "armAnimationComponent.h"

ArmAnimationComponent::ArmAnimationComponent() :
   mNewAnimationTickTime(4),
   mAnimationTransitionTime(0.25f),
   mUseAnimationTransitions(true),
   mMinLookAngle(-90),
   mMaxLookAngle(90)
{
}

ArmAnimationComponent::~ArmAnimationComponent()
{
}

void ArmAnimationComponent::initPersistFields()
{
   addGroup("AnimationController");
   addField("newAnimationTickTime", TypeF32, Offset(mNewAnimationTickTime, ArmAnimationComponent), "");
   addField("animationTransitionTime", TypeF32, Offset(mAnimationTransitionTime, ArmAnimationComponent), "");
   addField("useAnimationTransitions", TypeF32, Offset(mUseAnimationTransitions, ArmAnimationComponent), "");
   addField("minLookAngle", TypeF32, Offset(mMinLookAngle, ArmAnimationComponent), "");
   addField("maxLookAngle", TypeF32, Offset(mMaxLookAngle, ArmAnimationComponent), "");
   endGroup("AnimationController");

   Parent::initPersistFields();
}

bool ArmAnimationComponent::onAdd()
{
   if (!Parent::onAdd())
      return false;

   //Configure our references here for ease of use
   mArmThread = mAnimationThreads[1].thread;
   mHeadVThread = mAnimationThreads[2].thread;
   mHeadHThread = mAnimationThreads[3].thread;
   mRecoilThread = mAnimationThreads[4].thread;

   return true;
}

void ArmAnimationComponent::onRemove()
{
   Parent::onRemove();
}

void ArmAnimationComponent::onComponentAdd()
{
   Parent::onComponentAdd();
}

void ArmAnimationComponent::componentAddedToOwner(Component *comp)
{
   Parent::componentAddedToOwner(comp);
}

void ArmAnimationComponent::componentRemovedFromOwner(Component *comp)
{
   Parent::componentRemovedFromOwner(comp);
}

void ArmAnimationComponent::targetShapeChanged(RenderComponentInterface* instanceInterface)
{
   Parent::targetShapeChanged(instanceInterface);

   mArmAnimation.thread = mAnimationThreads[1].thread;
   mHeadVThread = mAnimationThreads[2].thread;
   mHeadHThread = mAnimationThreads[3].thread;
}

void ArmAnimationComponent::processTick()
{
   Parent::processTick();

   if (!isActive() || mOwnerShapeInstance == nullptr)
      return;

   Entity::StateDelta delta = mOwner->getNetworkDelta();

   if (delta.warpTicks > 0)
   {
      //updateDeathOffsets();
      updateLookAnimation();
   }
   else
   {
      if (isServerObject())
      {
         updateAnimation(TickSec);

         updateLookAnimation();
         //updateDeathOffsets();

         // Animations are advanced based on frame rate on the
         // client and must be ticked on the server.
         updateAnimationTree(true);
      }
   }
}

void ArmAnimationComponent::interpolateTick(F32 dt)
{
   Parent::interpolateTick(dt);

   if (!isActive() || mOwnerShapeInstance == nullptr)
      return;

   updateLookAnimation(dt);
}

void ArmAnimationComponent::advanceTime(F32 dt)
{
   Parent::advanceTime(dt);

   if (!isActive() || mOwnerShapeInstance == nullptr)
      return;

   updateAnimation(dt);
}

void ArmAnimationComponent::updateLookAnimation(F32 dt)
{
   // If the preference setting overrideLookAnimation is true, the player's
   // arm and head no longer animate according to the view direction. They
   // are instead given fixed positions.
   if (overrideLookAnimation)
   {
      if (mArmAnimation.thread)
         mOwnerShapeInstance->setPos(mArmAnimation.thread, armLookOverridePos);
      if (mHeadVThread)
         mOwnerShapeInstance->setPos(mHeadVThread, headVLookOverridePos);
      if (mHeadHThread)
         mOwnerShapeInstance->setPos(mHeadHThread, headHLookOverridePos);
      return;
   }
   // Calculate our interpolated head position.
   Point3F renderHead = head + headVec * dt;

   // Adjust look pos.  This assumes that the animations match
   // the min and max look angles provided in the datablock.
   if (mArmAnimation.thread)
   {
      /*if (mControlObject)
      {
         mOwnerShapeInstance->setPos(mArmAnimation.thread, 0.5f);
      }
      else
      {*/
         F32 d = mMaxLookAngle - mMinLookAngle;
         F32 tp = (renderHead.x - mMinLookAngle) / d;
         mOwnerShapeInstance->setPos(mArmAnimation.thread, mClampF(tp, 0, 1));
      //}
   }

   if (mHeadVThread)
   {
      F32 d = mMaxLookAngle - mMinLookAngle;
      F32 tp = (renderHead.x - mMinLookAngle) / d;
      mOwnerShapeInstance->setPos(mHeadVThread, mClampF(tp, 0, 1));
   }

   if (mHeadHThread)
   {
      F32 d = 2 * mMaxFreelookAngle;
      F32 tp = (renderHead.z + mMaxFreelookAngle) / d;
      mOwnerShapeInstance->setPos(mHeadHThread, mClampF(tp, 0, 1));
   }
}


//----------------------------------------------------------------------------
// Methods to get delta (as amount to affect velocity by)

/*bool ArmAnimationComponent::inDeathAnim()
{
   if ((anim_clip_flags & ANIM_OVERRIDDEN) != 0 && (anim_clip_flags & IS_DEATH_ANIM) == 0)
      return false;
   if (mActionAnimation.thread && mActionAnimation.action >= 0)
      if (mActionAnimation.action < mActionList.size())
         return mActionAnimationList[mActionAnimation.action].death;

   return false;
}

// Get change from mLastDeathPos - return current pos.  Assumes we're in death anim.
F32 ArmAnimationComponent::deathDelta(Point3F & delta)
{
   // Get ground delta from the last time we offset this.
   MatrixF  mat;
   F32 pos = mOwnerShapeInstance->getPos(mActionAnimation.thread);
   mOwnerShapeInstance->deltaGround1(mActionAnimation.thread, mDeath.lastPos, pos, mat);
   mat.getColumn(3, &delta);
   return pos;
}

// Called before updatePos() to prepare it's needed change to velocity, which
// must roll over.  Should be updated on tick, this is where we remember last
// position of animation that was used to roll into velocity.
void ArmAnimationComponent::updateDeathOffsets()
{
   if (inDeathAnim())
      // Get ground delta from the last time we offset this.
      mDeath.lastPos = deathDelta(mDeath.posAdd);
   else
      mDeath.clear();
}*/

void ArmAnimationComponent::updateAnimation(F32 dt)
{
   // update any active blend clips
   //if (isClientObject())
   //   for (S32 i = 0; i < blend_clips.size(); i++)
   //      mOwnerShapeInstance->advanceTime(dt, blend_clips[i].thread);

   // If we are the client's player on this machine, then we need
   // to make sure the transforms are up to date as they are used
   // to setup the camera.
   if (isClientObject())
   {
      updateAnimationTree(false);
   }
}

void ArmAnimationComponent::updateAnimationTree(bool firstPerson)
{
   S32 mode = 0;
   if (firstPerson)
   {
      if (mActionAnimation.firstPerson)
         mode = 0;
      //            TSShapeInstance::MaskNodeRotation;
      //            TSShapeInstance::MaskNodePosX |
      //            TSShapeInstance::MaskNodePosY;
      else
         mode = TSShapeInstance::MaskNodeAllButBlend;
   }

   /*for (U32 i = 0; i < PlayerData::NumSpineNodes; i++)
      if (mDataBlock->spineNode[i] != -1)
         mOwnerShapeInstance->setNodeAnimationState(mDataBlock->spineNode[i], mode);*/
}

const String& ArmAnimationComponent::getArmThread() const
{
   if (mArmAnimation.thread && mArmAnimation.thread->hasSequence())
   {
      return mArmAnimation.thread->getSequenceName();
   }

   return String::EmptyString;
}

bool ArmAnimationComponent::setArmThread(const char* sequence)
{
   // The arm sequence must be in the action list.
   for (U32 i = 1; i < mActionAnimationList.size(); i++)
      if (!dStricmp(mActionAnimationList[i].name, sequence))
         return setArmThread(i);
   return false;
}

bool ArmAnimationComponent::setArmThread(U32 action)
{
   ActionAnimationDef &anim = mActionAnimationList[action];
   if (anim.sequence != -1 &&
      anim.sequence != mOwnerShapeInstance->getSequence(mArmAnimation.thread))
   {
      mOwnerShapeInstance->setSequence(mArmAnimation.thread, anim.sequence, 0);
      mArmAnimation.action = action;
      setMaskBits(ThreadMaskN << 1);
      return true;
   }
   return false;
}