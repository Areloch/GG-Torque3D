#include "actionAnimationComponent.h"

IMPLEMENT_CO_NETOBJECT_V1(ActionAnimationComponent);

ActionAnimationComponent::ActionAnimationComponent() :
   mNewAnimationTickTime(4),
   mAnimationTransitionTime(0.25f),
   mUseAnimationTransitions(true)
{
   mFriendlyName = "Player Animation(Component)";
   mComponentType = "Animation";

   mActionAnimation.action = -1;
}

ActionAnimationComponent::~ActionAnimationComponent()
{
}

void ActionAnimationComponent::initPersistFields()
{
   addGroup("AnimationController");
   addField("newAnimationTickTime", TypeF32, Offset(mNewAnimationTickTime, ActionAnimationComponent), "");
   addField("animationTransitionTime", TypeF32, Offset(mAnimationTransitionTime, ActionAnimationComponent), "");
   addField("useAnimationTransitions", TypeF32, Offset(mUseAnimationTransitions, ActionAnimationComponent), "");
   endGroup("AnimationController");

   Parent::initPersistFields();
}

bool ActionAnimationComponent::onAdd()
{
   if (!Parent::onAdd())
      return false;

   //Configure our references here for ease of use
   mActionAnimation.thread = mAnimationThreads[0].thread;

   return true;
}

void ActionAnimationComponent::onRemove()
{
   Parent::onRemove();
}

void ActionAnimationComponent::onComponentAdd()
{
   Parent::onComponentAdd();
}

void ActionAnimationComponent::componentAddedToOwner(Component *comp)
{
   Parent::componentAddedToOwner(comp);
}

void ActionAnimationComponent::componentRemovedFromOwner(Component *comp)
{
   Parent::componentRemovedFromOwner(comp);
}

void ActionAnimationComponent::targetShapeChanged(RenderComponentInterface* instanceInterface)
{
   Parent::targetShapeChanged(instanceInterface);

   mActionAnimation.thread = mAnimationThreads[0].thread;
}

void ActionAnimationComponent::processTick()
{
   Parent::processTick();

   if (!isActive() || mOwnerShapeInstance == nullptr)
      return;

   Entity::StateDelta delta = mOwner->getNetworkDelta();

   if (delta.warpTicks > 0)
   {
   }
   else
   {
      if (isServerObject())
      {
         updateAnimation(TickSec);

         // Animations are advanced based on frame rate on the
         // client and must be ticked on the server.
         updateActionThread();
         updateAnimationTree(true);
      }
   }
}

void ActionAnimationComponent::interpolateTick(F32 dt)
{
   Parent::interpolateTick(dt);

   if (!isActive() || mOwnerShapeInstance == nullptr)
      return;
}

void ActionAnimationComponent::advanceTime(F32 dt)
{
   Parent::advanceTime(dt);

   if (!isActive() || mOwnerShapeInstance == nullptr)
      return;

   updateActionThread();
   updateAnimation(dt);
}

//
//
//
void ActionAnimationComponent::addAction(String animName, VectorF direction, String tags)
{
   ActionAnimationDef newActionDef;

   newActionDef.name = animName;
   newActionDef.dir = direction;
   newActionDef.sequence = -1;
   newActionDef.speed = 1;
   newActionDef.death = 0;
   newActionDef.tags.push_back(StringTable->insert(tags));

   mActionAnimationList.push_back(newActionDef);
}

void ActionAnimationComponent::getGroundInfo(TSShapeInstance* si, TSThread* thread, ActionAnimationDef *dp)
{
  /* dp->death = !dStrnicmp(dp->name, "death", 5);
   if (dp->death)
   {
      // Death animations use roll frame-to-frame changes in ground transform into position
      dp->speed = 0.0f;
      dp->dir.set(0.0f, 0.0f, 0.0f);

      // Death animations MUST define ground transforms, so add dummy ones if required
      if (si->getShape()->sequences[dp->sequence].numGroundFrames == 0)
         si->getShape()->setSequenceGroundSpeed(dp->name, Point3F(0, 0, 0), Point3F(0, 0, 0));
   }
   else
   {*/
      VectorF save = dp->dir;
      si->setSequence(thread, dp->sequence, 0);
      si->animate();
      si->advanceTime(1);
      si->animateGround();
      si->getGroundTransform().getColumn(3, &dp->dir);
      if ((dp->speed = dp->dir.len()) < 0.01f)
      {
         // No ground displacement... In this case we'll use the
         // default table entry, if there is one.
         if (save.len() > 0.01f)
         {
            dp->dir = save;
            dp->speed = 1.0f;
            dp->velocityScale = false;
         }
         else
            dp->speed = 0.0f;
      }
      else
         dp->dir *= 1.0f / dp->speed;
   //}
}

void ActionAnimationComponent::updateAnimation(F32 dt)
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

void ActionAnimationComponent::updateAnimationTree(bool firstPerson)
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

//----------------------------------------------------------------------------

bool ActionAnimationComponent::setActionThread(const char* sequence, bool hold, bool wait, bool fsp)
{
   //if (anim_clip_flags & ANIM_OVERRIDDEN)
    //  return false;

   for (U32 i = 1; i < mActionAnimationList.size(); i++)
   {
      ActionAnimationDef &anim = mActionAnimationList[i];
      if (!dStricmp(anim.name, sequence))
      {
         setActionThread(i, true, hold, wait, fsp);
         setMaskBits(ThreadMaskN << 0);
         return true;
      }
   }
   return false;
}

void ActionAnimationComponent::setActionThread(U32 action, bool forward, bool hold, bool wait, bool fsp, bool forceSet)
{
   if (!mActionAnimationList.size() || (mActionAnimation.action == action && mActionAnimation.forward == forward && !forceSet))
      return;

   if (action >= mActionAnimationList.size())
   {
      Con::errorf("ActionAnimationComponent::setActionThread(%d): Player action out of range", action);
      return;
   }

   /*if (isClientObject())
   {
      mark_idle = (action == PlayerData::RootAnim);
      idle_timer = (mark_idle) ? 0.0f : -1.0f;
   }*/

   ActionAnimationDef &anim = mActionAnimationList[action];
   if (anim.sequence != -1)
   {
      U32 lastAction = mActionAnimation.action;

      mActionAnimation.action = action;
      mActionAnimation.forward = forward;
      mActionAnimation.firstPerson = fsp;
      mActionAnimation.holdAtEnd = hold;
      mActionAnimation.waitForEnd = hold ? true : wait;
      mActionAnimation.animateOnServer = fsp;
      mActionAnimation.atEnd = false;
      mActionAnimation.delayTicks = mNewAnimationTickTime;
      mActionAnimation.atEnd = false;

      if (mUseAnimationTransitions && /*(action != PlayerData::LandAnim || !(mDataBlock->landSequenceTime > 0.0f && !mDataBlock->transitionToLand)) &&*/ (isClientObject()/* || mActionAnimation.animateOnServer*/))
      {
         // The transition code needs the timeScale to be set in the
         // right direction to know which way to go.
         F32   transTime = mAnimationTransitionTime;
         //if (mDataBlock && mDataBlock->isJumpAction(action))
         //   transTime = 0.15f;

         F32 timeScale = mActionAnimation.forward ? 1.0f : -1.0f;
        // if (mDataBlock && mDataBlock->isJumpAction(action))
         //   timeScale *= 1.5f;

         mOwnerShapeInstance->setTimeScale(mActionAnimation.thread, timeScale);

         // If we're transitioning into the same sequence (an action may use the
         // same sequence as a previous action) then we want to start at the same
         // position.
         F32 pos = mActionAnimation.forward ? 0.0f : 1.0f;
         ActionAnimationDef &lastAnim = mActionAnimationList[lastAction];
         if (lastAnim.sequence == anim.sequence)
         {
            pos = mOwnerShapeInstance->getPos(mActionAnimation.thread);
         }

         mOwnerShapeInstance->transitionToSequence(mActionAnimation.thread, anim.sequence,
            pos, transTime, true);
      }
      else
      {
         mOwnerShapeInstance->setSequence(mActionAnimation.thread, anim.sequence,
            mActionAnimation.forward ? 0.0f : 1.0f);
      }
   }
}

void ActionAnimationComponent::updateActionThread()
{
   PROFILE_START(ActionAnimationComponent_UpdateActionThread);

   // Select an action animation sequence, this assumes that
   // this function is called once per tick.
   if (mActionAnimation.action != -1)
   {
      if (mActionAnimation.forward)
         mActionAnimation.atEnd = mOwnerShapeInstance->getPos(mActionAnimation.thread) == 1;
      else
         mActionAnimation.atEnd = mOwnerShapeInstance->getPos(mActionAnimation.thread) == 0;
   }

   // Only need to deal with triggers on the client
   /*if (isClientObject())
   {
      bool triggeredLeft = false;
      bool triggeredRight = false;

      F32 offset = 0.0f;
      if (mOwnerShapeInstance->getTriggerState(1))
      {
         triggeredLeft = true;
         offset = -mDataBlock->decalOffset * getScale().x;
      }
      else if (mOwnerShapeInstance->getTriggerState(2))
      {
         triggeredRight = true;
         offset = mDataBlock->decalOffset * getScale().x;
      }

      process_client_triggers(triggeredLeft, triggeredRight);
      if ((triggeredLeft || triggeredRight) && !noFootfallFX)
      {
         Point3F rot, pos;
         RayInfo rInfo;
         MatrixF mat = getRenderTransform();
         mat.getColumn(1, &rot);
         mat.mulP(Point3F(offset, 0.0f, 0.0f), &pos);

         if (gClientContainer.castRay(Point3F(pos.x, pos.y, pos.z + 0.01f),
            Point3F(pos.x, pos.y, pos.z - 2.0f),
            STATIC_COLLISION_TYPEMASK | VehicleObjectType, &rInfo))
         {
            Material* material = (rInfo.material ? dynamic_cast< Material* >(rInfo.material->getMaterial()) : 0);

            // Put footprints on surface, if appropriate for material.

            if (material && material->mShowFootprints
               && mDataBlock->decalData && !footfallDecalOverride)
            {
               Point3F normal;
               Point3F tangent;
               mObjToWorld.getColumn(0, &tangent);
               mObjToWorld.getColumn(2, &normal);
               gDecalManager->addDecal(rInfo.point, normal, tangent, mDataBlock->decalData, getScale().y);
            }

            // Emit footpuffs.

            if (!footfallDustOverride && rInfo.t <= 0.5f && mWaterCoverage == 0.0f
               && material && material->mShowDust)
            {
               // New emitter every time for visibility reasons
               ParticleEmitter * emitter = new ParticleEmitter;
               emitter->onNewDataBlock(mDataBlock->footPuffEmitter, false);

               LinearColorF colorList[ParticleData::PDC_NUM_KEYS];

               for (U32 x = 0; x < getMin(Material::NUM_EFFECT_COLOR_STAGES, ParticleData::PDC_NUM_KEYS); ++x)
                  colorList[x].set(material->mEffectColor[x].red,
                     material->mEffectColor[x].green,
                     material->mEffectColor[x].blue,
                     material->mEffectColor[x].alpha);
               for (U32 x = Material::NUM_EFFECT_COLOR_STAGES; x < ParticleData::PDC_NUM_KEYS; ++x)
                  colorList[x].set(1.0, 1.0, 1.0, 0.0);

               emitter->setColors(colorList);
               if (!emitter->registerObject())
               {
                  Con::warnf(ConsoleLogEntry::General, "Could not register emitter for particle of class: %s", mDataBlock->getName());
                  delete emitter;
                  emitter = NULL;
               }
               else
               {
                  emitter->emitParticles(pos, Point3F(0.0, 0.0, 1.0), mDataBlock->footPuffRadius,
                     Point3F(0, 0, 0), mDataBlock->footPuffNumParts);
                  emitter->deleteWhenEmpty();
               }
            }

            // Play footstep sound.

            if (footfallSoundOverride <= 0)
               playFootstepSound(triggeredLeft, material, rInfo.object);
         }
      }
   }*/

   // Mount pending variable puts a hold on the delayTicks below so players don't
   // inadvertently stand up because their mount has not come over yet.
   //if (mMountPending)
   //   mMountPending = (mOwner->isMounted() ? 0 : (mMountPending - 1));

   if ((mActionAnimation.action == -1) ||
      ((!mActionAnimation.waitForEnd || mActionAnimation.atEnd) /*&&
      (!mActionAnimation.holdAtEnd && (mActionAnimation.delayTicks -= !mMountPending) <= 0)*/))
   {
      //The scripting language will get a call back when a script animation has finished...
      //  example: When the chat menu animations are done playing...
      if (isServerObject()/* && mActionAnimation.action >= PlayerData::NumTableActionAnims*/)
      {
         Con::executef(this, "onAnimationEnd", mActionAnimation.thread->getSequenceName());
      }
      pickActionAnimation();
   }

   PROFILE_END();
}

/*void ActionAnimationComponent::pickBestMoveAction(U32 * action, bool * forward) const
{
   *action = startAnim;
   *forward = false;

   VectorF vel;
   mOwner->getWorldToObj().mulV(mVelocity, &vel);

   if (vel.lenSquared() > 0.01f)
   {
      // Bias the velocity towards picking the forward/backward anims over
      // the sideways ones to prevent oscillation between anims.
      vel *= VectorF(0.5f, 1.0f, 0.5f);

      // Pick animation that is the best fit for our current (local) velocity.
      // Assumes that the root (stationary) animation is at startAnim.
      U32 animCount = mActionAnimationList.size();

      F32 curMax = -0.1f;
      for (U32 i = 0; i < animCount; i++)
      {
         const ActionAnimationDef &anim = mActionAnimationList[i];

         /*for (U32 a = 0; a < mActiveTags.size(); a++)
         {
            if(mActiveTags[a]
         }*/

         /*if (anim.sequence != -1 && anim.speed)
         {
            F32 d = mDot(vel, anim.dir);
            if (d > curMax)
            {
               curMax = d;
               *action = i;
               *forward = true;
            }
            else
            {
               // Check if reversing this animation would fit (bias against this
               // so that when moving right, the real right anim is still chosen,
               // but if not present, the reversed left anim will be used instead)
               d *= -0.75f;
               if (d > curMax)
               {
                  curMax = d;
                  *action = i;
                  *forward = false;
               }
            }
         }
      }
   }
}*/

void ActionAnimationComponent::pickActionAnimation()
{
   /*if (isMounted() || mMountPending)
   {
      // Go into root position unless something was set explicitly
      // from a script.
      if (mActionAnimation.action != PlayerData::RootAnim &&
         mActionAnimation.action < PlayerData::NumTableActionAnims)
         setActionThread(PlayerData::RootAnim, true, false, false);
      return;
   }

   bool forward = true;
   U32 action = PlayerData::RootAnim;
   bool fsp = false;

   // Jetting overrides the fall animation condition
   if (mJetting)
   {
      // Play the jetting animation
      action = PlayerData::JetAnim;
   }
   else if (mFalling)
   {
      // Not in contact with any surface and falling
      action = PlayerData::FallAnim;
   }
   else if (mSwimming)
   {
      pickBestMoveAction(PlayerData::SwimRootAnim, PlayerData::SwimRightAnim, &action, &forward);
   }
   else if (mPose == StandPose)
   {
      if (mContactTimer >= sContactTickTime)
      {
         // Nothing under our feet
         action = PlayerData::RootAnim;
      }
      else
      {
         // Our feet are on something
         pickBestMoveAction(PlayerData::RootAnim, PlayerData::SideRightAnim, &action, &forward);
      }
   }
   else if (mPose == CrouchPose)
   {
      pickBestMoveAction(PlayerData::CrouchRootAnim, PlayerData::CrouchRightAnim, &action, &forward);
   }
   else if (mPose == PronePose)
   {
      pickBestMoveAction(PlayerData::ProneRootAnim, PlayerData::ProneBackwardAnim, &action, &forward);
   }
   else if (mPose == SprintPose)
   {
      pickBestMoveAction(PlayerData::SprintRootAnim, PlayerData::SprintRightAnim, &action, &forward);
   }*/

   bool forward = true;
   U32 action = 0;
   bool fsp = false;

   //pickBestMoveAction(PlayerData::SprintRootAnim, PlayerData::SprintRightAnim, &action, &forward);

   setActionThread(action, forward, false, false, fsp);
}

void ActionAnimationComponent::setActiveTags(String tags)
{
   mActiveTags.clear();

   U32 count = StringUnit::getUnitCount(tags, ",");

   for (U32 i = 0; i < count; i++)
   {
      StringTableEntry tag = StringTable->insert(StringUnit::getUnit(tags, i, ","));

      mActiveTags.push_back(tag);
   }
}

DefineEngineMethod(ActionAnimationComponent, addAction, void, (String animName, VectorF direction, String tags), ("", VectorF::Zero, ""),
   "")
{
   return object->addAction(animName, direction, tags);
}

DefineEngineMethod(ActionAnimationComponent, setActiveTags, void, (String tags), (""), "")
{
   return object->setActiveTags(tags);
}