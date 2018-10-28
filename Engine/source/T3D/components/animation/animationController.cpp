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

#include "T3D/components/animation/animationController.h"
#include "T3D/components/render/meshComponent.h"

#include "platform/platform.h"
#include "console/consoleTypes.h"
#include "core/util/safeDelete.h"
#include "core/resourceManager.h"
#include "core/stream/fileStream.h"
#include "console/consoleTypes.h"
#include "console/consoleObject.h"
#include "ts/tsShapeInstance.h"
#include "core/stream/bitStream.h"
#include "sim/netConnection.h"
#include "gfx/gfxTransformSaver.h"
#include "console/engineAPI.h"
#include "lighting/lightQuery.h"
#include "gfx/sim/debugDraw.h" 

extern bool gEditingMission;

//////////////////////////////////////////////////////////////////////////
// Callbacks
//////////////////////////////////////////////////////////////////////////
IMPLEMENT_CALLBACK(AnimationController, onAnimationStart, void, (Component* obj, const String& animName), (obj, animName),
   "@brief Called when we collide with another object.\n\n"
   "@param obj The ShapeBase object\n"
   "@param collObj The object we collided with\n"
   "@param vec Collision impact vector\n"
   "@param len Length of the impact vector\n");

IMPLEMENT_CALLBACK(AnimationController, onAnimationEnd, void, (Component* obj, const char* animName), (obj, animName),
   "@brief Called when we collide with another object.\n\n"
   "@param obj The ShapeBase object\n"
   "@param collObj The object we collided with\n"
   "@param vec Collision impact vector\n"
   "@param len Length of the impact vector\n");

IMPLEMENT_CALLBACK(AnimationController, onAnimationTrigger, void, (Component* obj, const String& animName, S32 triggerID), (obj, animName, triggerID),
   "@brief Called when we collide with another object.\n\n"
   "@param obj The ShapeBase object\n"
   "@param collObj The object we collided with\n"
   "@param vec Collision impact vector\n"
   "@param len Length of the impact vector\n");


//////////////////////////////////////////////////////////////////////////
// Constructor/Destructor
//////////////////////////////////////////////////////////////////////////
AnimationController::AnimationController() : Component()
{
   mNetworked = true;

   mFriendlyName = "Animation(Component)";
   mComponentType = "Render";

   mDescription = getDescriptionText("Allows a rendered mesh to be animated");

   mOwnerShapeInstance = NULL;

   for (U32 i = 0; i < MaxScriptThreads; i++)
   {
      mAnimationThreads[i].sequence = -1;
      mAnimationThreads[i].thread = 0;
      mAnimationThreads[i].sound = 0;
      mAnimationThreads[i].state = Thread::Stop;
      mAnimationThreads[i].atEnd = false;
      mAnimationThreads[i].timescale = 1.f;
      mAnimationThreads[i].position = -1.f;
      mAnimationThreads[i].transition = true;
   }

   mTest = false;
   mTestWalk = false;
   mTestRun = false;
   mTestBlend = false;
}

AnimationController::~AnimationController()
{
   for (S32 i = 0; i < mFields.size(); ++i)
   {
      ComponentField &field = mFields[i];
      SAFE_DELETE_ARRAY(field.mFieldDescription);
   }

   SAFE_DELETE_ARRAY(mDescription);
}

IMPLEMENT_CO_NETOBJECT_V1(AnimationController);

bool AnimationController::onAdd()
{
   if (!Parent::onAdd())
      return false;

   return true;
}

void AnimationController::onRemove()
{
   Parent::onRemove();
}

void AnimationController::onComponentAdd()
{
   //test if this is a shape component!
   MeshComponent *meshComp = mOwner->getComponent<MeshComponent>();
   if (meshComp)
   {
      //shapeInstanceInterface->onShapeInstanceChanged.notify(this, &AnimationController::targetShapeChanged);
      //targetShapeChanged(shapeInstanceInterface);
      mOwnerShapeInstance = meshComp->getShapeInstance();
   }
}

void AnimationController::componentAddedToOwner(Component *comp)
{
   if (comp->getId() == getId())
      return;

   //test if this is a shape component!
   MeshComponent *meshComp = dynamic_cast<MeshComponent*>(comp);
   if (meshComp)
   {
      // meshComp->onShapeInstanceChanged.notify(this, &AnimationController::targetShapeChanged);
      mOwnerShapeInstance = meshComp->getShapeInstance();
   }
}

void AnimationController::componentRemovedFromOwner(Component *comp)
{
   if (comp->getId() == getId()) //?????????
      return;

   //test if this is a shape component!
   /*RenderComponentInterface *shapeInstanceInterface = dynamic_cast<RenderComponentInterface*>(comp);
   if (shapeInstanceInterface)
   {
      shapeInstanceInterface->onShapeInstanceChanged.remove(this, &AnimationController::targetShapeChanged);
      mOwnerRenderInst = NULL;
   }*/
}

TSShape* AnimationController::getShape()
{
   if (mOwner == NULL)
      return NULL;

   MeshComponent* meshComp = mOwner->getComponent<MeshComponent>();
   if (meshComp)
   {
      return meshComp->getShape();
   }

   return nullptr;
}

void AnimationController::initPersistFields()
{
   Parent::initPersistFields();
}

U32 AnimationController::packUpdate(NetConnection *con, U32 mask, BitStream *stream)
{
   U32 retMask = Parent::packUpdate(con, mask, stream);

   stream->writeFlag(mTest);
   stream->writeFlag(mTestWalk);
   stream->writeFlag(mTestRun);
   stream->writeFlag(mTestBlend);

   stream->write(blendAmount);

   mTest = false;
   mTestWalk = false;
   mTestRun = false;
   mTestBlend = false;

   return retMask;
}

void AnimationController::unpackUpdate(NetConnection *con, BitStream *stream)
{
   Parent::unpackUpdate(con, stream);

   mTest = stream->readFlag();
   mTestWalk = stream->readFlag();
   mTestRun = stream->readFlag();
   mTestBlend = stream->readFlag();

   stream->read(&blendAmount);

   if(mTest)
      test();
   else if (mTestWalk)
      testWalk();
   else if (mTestRun)
      testRun();
   else if (mTestBlend)
      testBlend(blendAmount);

   mTest = false;
   mTestWalk = false;
   mTestRun = false;
   mTestBlend = false;
}

void AnimationController::processTick()
{
   Parent::processTick();

   if (!isActive())
      return;

   if (isServerObject())
   {
      // Server only...
      //advanceThreads(TickSec);
   }
}

void AnimationController::advanceTime(F32 dt)
{
   Parent::advanceTime(dt);

   // On the client, the shape threads and images are
   // advanced at framerate.
   //advanceThreads(dt);
}

//
//
//
const TSShape::Sequence* AnimationController::getShapeSequence(S32 seqId)
{
   if (!getShape())
      return nullptr;

   return &(getShape()->sequences[seqId]);
}

void AnimationController::setupTest()
{
   if (!getShape())
      return;

   if (mOwnerShapeInstance == nullptr)
   {
      MeshComponent *meshComp = mOwner->getComponent<MeshComponent>();
      if (meshComp)
      {
         mOwnerShapeInstance = meshComp->getShapeInstance();
      }
      else
         return;
   }

   mOwnerShapeInstance->mNodeTransforms.setSize(getShape()->nodes.size());

   // temporary storage for node transforms
   mOwnerShapeInstance->smNodeCurrentRotations.setSize(getShape()->nodes.size());
   mOwnerShapeInstance->smNodeCurrentTranslations.setSize(getShape()->nodes.size());
   mOwnerShapeInstance->smNodeLocalTransforms.setSize(getShape()->nodes.size());
   mOwnerShapeInstance->smRotationThreads.setSize(getShape()->nodes.size());
   mOwnerShapeInstance->smTranslationThreads.setSize(getShape()->nodes.size());
}

void AnimationController::test()
{
   if (!getShape())
      return;

   if (mOwnerShapeInstance == nullptr)
   {
      MeshComponent *meshComp = mOwner->getComponent<MeshComponent>();
      if (meshComp)
      {
         //shapeInstanceInterface->onShapeInstanceChanged.notify(this, &AnimationController::targetShapeChanged);
         //targetShapeChanged(shapeInstanceInterface);
         mOwnerShapeInstance = meshComp->getShapeInstance();
      }
      else
         return;
   }

   S32 ss = getShape()->details[0].subShapeNum;

   // all the nodes marked above need to have the default transform
   S32 a = getShape()->subShapeFirstNode[ss];
   S32 b = a + getShape()->subShapeNumNodes[ss];

   mOwnerShapeInstance->mNodeTransforms.setSize(getShape()->nodes.size());

   // temporary storage for node transforms
   mOwnerShapeInstance->smNodeCurrentRotations.setSize(getShape()->nodes.size());
   mOwnerShapeInstance->smNodeCurrentTranslations.setSize(getShape()->nodes.size());
   mOwnerShapeInstance->smNodeLocalTransforms.setSize(getShape()->nodes.size());
   mOwnerShapeInstance->smRotationThreads.setSize(getShape()->nodes.size());
   mOwnerShapeInstance->smTranslationThreads.setSize(getShape()->nodes.size());

   for (S32 i = a; i < b; i++)
   {
      getShape()->defaultRotations[i].getQuatF(&mOwnerShapeInstance->smNodeCurrentRotations[i]);
      mOwnerShapeInstance->smNodeCurrentTranslations[i] = getShape()->defaultTranslations[i];

      RotationF boneRot = mOwnerShapeInstance->smNodeCurrentRotations[i];
      RotationF randRot = RotationF(mRandF(-0.01, 0.01), mRandF(-0.01, 0.01), mRandF(-0.01, 0.01));

      boneRot += randRot;

      mOwnerShapeInstance->smNodeCurrentRotations[i] = boneRot.asQuatF();

      mOwnerShapeInstance->smNodeCurrentTranslations[i] += Point3F(mRandF(-0.01, 0.01), mRandF(-0.01, 0.01), mRandF(-0.01, 0.01));

      mOwnerShapeInstance->mHandsOffNodes.set(i);
   }

   // handle non-blend sequences
   /*for (i = 0; i<firstBlend; i++)
   {
   TSThread * th = mThreadList[i];

   j = 0;
   start = th->getSequence()->rotationMatters.start();
   end = b;
   for (nodeIndex = start; nodeIndex<end; th->getSequence()->rotationMatters.next(nodeIndex), j++)
   {
   // skip nodes outside of this detail
   if (nodeIndex<a)
   continue;
   if (!rotBeenSet.test(nodeIndex))
   {
   QuatF q1, q2;
   mShape->getRotation(*th->getSequence(), th->keyNum1, j, &q1);
   mShape->getRotation(*th->getSequence(), th->keyNum2, j, &q2);
   TSTransform::interpolate(q1, q2, th->keyPos, &smNodeCurrentRotations[nodeIndex]);
   rotBeenSet.set(nodeIndex);
   smRotationThreads[nodeIndex] = th;
   }
   }

   j = 0;
   start = th->getSequence()->translationMatters.start();
   end = b;
   for (nodeIndex = start; nodeIndex<end; th->getSequence()->translationMatters.next(nodeIndex), j++)
   {
   if (nodeIndex<a)
   continue;
   if (!tranBeenSet.test(nodeIndex))
   {
   if (maskPosNodes.test(nodeIndex))
   handleMaskedPositionNode(th, nodeIndex, j);
   else
   {
   const Point3F & p1 = mShape->getTranslation(*th->getSequence(), th->keyNum1, j);
   const Point3F & p2 = mShape->getTranslation(*th->getSequence(), th->keyNum2, j);
   TSTransform::interpolate(p1, p2, th->keyPos, &smNodeCurrentTranslations[nodeIndex]);
   smTranslationThreads[nodeIndex] = th;
   }
   tranBeenSet.set(nodeIndex);
   }
   }
   }*/

   // compute transforms
   for (S32 i = a; i<b; i++)
   {
      //if (!mHandsOffNodes.test(i))
      TSTransform::setMatrix(mOwnerShapeInstance->smNodeCurrentRotations[i],
         mOwnerShapeInstance->smNodeCurrentTranslations[i], &mOwnerShapeInstance->smNodeLocalTransforms[i]);
      //else
      //	smNodeLocalTransforms[i] = mNodeTransforms[i];     // in case mNodeTransform was changed externally
   }

   for (S32 i = a; i<b; i++)
   {
      S32 parentIdx = getShape()->nodes[i].parentIndex;
      if (parentIdx < 0)
         mOwnerShapeInstance->mNodeTransforms[i] = mOwnerShapeInstance->smNodeLocalTransforms[i];
      else
         mOwnerShapeInstance->mNodeTransforms[i].mul(mOwnerShapeInstance->mNodeTransforms[parentIdx], mOwnerShapeInstance->smNodeLocalTransforms[i]);
   }

   setMaskBits(-1);
}

void AnimationController::testWalk()
{
   if (!getShape())
      return;

   setupTest();

   //get the sequence in question
   S32 walkSeqId = getShape()->findSequence("BaseCharacter_pose_walk");

   if (walkSeqId == -1)
      return;

   mTestWalk = true;

   //Set up our base layer
   Animation* newAnim = processAnim(walkSeqId);

   if (newAnim == nullptr)
      return;
   
   for (S32 i = newAnim->startNode; i<newAnim->endNode; i++)
   {
      //S32 parentIdx = getShape()->nodes[i].parentIndex;
     // if (parentIdx < 0)
         mOwnerShapeInstance->mNodeTransforms[i] = newAnim->mFrames[0].mNodes[i].trans;
      //else
      //   mOwnerShapeInstance->mNodeTransforms[i].mul(mOwnerShapeInstance->mNodeTransforms[parentIdx], mOwnerShapeInstance->smNodeLocalTransforms[i]);
   }

   setMaskBits(-1);
}

void AnimationController::testRun()
{
   if (!getShape())
      return;

   setupTest();

   //get the sequence in question
   S32 walkSeqId = getShape()->findSequence("BaseCharacter_pose_run");

   if (walkSeqId == -1)
   {
      Con::errorf("Could not find run animation!");
      return;
   }

   mTestRun = true;

   //Set up our base layer
   Animation* newAnim = processAnim(walkSeqId);

   if (newAnim == nullptr)
      return;

   for (S32 i = newAnim->startNode; i<newAnim->endNode; i++)
   {
      //S32 parentIdx = getShape()->nodes[i].parentIndex;
      // if (parentIdx < 0)
      mOwnerShapeInstance->mNodeTransforms[i] = newAnim->mFrames[0].mNodes[i].trans;
      //else
      //   mOwnerShapeInstance->mNodeTransforms[i].mul(mOwnerShapeInstance->mNodeTransforms[parentIdx], mOwnerShapeInstance->smNodeLocalTransforms[i]);
   }

   setMaskBits(-1);
}

void AnimationController::testBlend(F32 blend)
{
   if (!getShape())
      return;

   setupTest();

   blendAmount = mClampF(blend, 0, 1);

   //get the sequence in question
   S32 runSeqId = getShape()->findSequence("BaseCharacter_pose_run");

   if (runSeqId == -1)
   {
      Con::errorf("Could not find run animation!");
      return;
   }

   //get the sequence in question
   S32 walkSeqId = getShape()->findSequence("BaseCharacter_pose_walk");

   if (walkSeqId == -1)
   {
      Con::errorf("Could not find walk animation!");
      return;
   }

   mTestBlend = true;

   //Set up our base layer
   Animation* walkAnim = processAnim(walkSeqId);
   Animation* runAnim = processAnim(runSeqId);

   for (S32 i = runAnim->startNode; i<runAnim->endNode; i++)
   {
      //S32 parentIdx = getShape()->nodes[i].parentIndex;
      // if (parentIdx < 0)
      Point3F blendPos;

      Point3F p1 = walkAnim->mFrames[0].mNodes[i].position;
      Point3F p2 = runAnim->mFrames[0].mNodes[i].position;
      //TSTransform::interpolate(p1, p2, blendAmount, &blendPos);

      blendPos.interpolate(p1, p2, blendAmount);

      RotationF blendRot;
      RotationF q1 = walkAnim->mFrames[0].mNodes[i].rotation;
      RotationF q2 = runAnim->mFrames[0].mNodes[i].rotation;
      //TSTransform::interpolate(q1, q2, blendAmount, &blendRot);

      //blendRot.interpolate(q1, q2, blendAmount);

      MatrixF blendTrans = blendRot.asMatrixF();
      blendTrans.setPosition(blendPos);

      mOwnerShapeInstance->smNodeLocalTransforms[i] = blendTrans;
      //else
      //   mOwnerShapeInstance->mNodeTransforms[i].mul(mOwnerShapeInstance->mNodeTransforms[parentIdx], mOwnerShapeInstance->smNodeLocalTransforms[i]);
   }

   // multiply transforms...
   for (S32 i = runAnim->startNode; i<runAnim->endNode; i++)
   {
      S32 parentIdx = getShape()->nodes[i].parentIndex;
      if (parentIdx < 0)
         mOwnerShapeInstance->mNodeTransforms[i] = mOwnerShapeInstance->smNodeLocalTransforms[i];
      else
         mOwnerShapeInstance->mNodeTransforms[i].mul(mOwnerShapeInstance->mNodeTransforms[parentIdx], mOwnerShapeInstance->smNodeLocalTransforms[i]);
   }

   setMaskBits(-1);
}

AnimationController::Animation* AnimationController::processAnim(S32 seqId)
{
   if (seqId == -1)
      return nullptr;

   const TSShape::Sequence* seq = getShapeSequence(seqId);

   if (seq == nullptr)
      return nullptr;

   //Set up our base layer
   mAnimations.increment();

   Animation* newAnim = &mAnimations.last(); //walk anim

   Animation::Frame newFrame; //only one frame for this anim atm
   newFrame.mNodes.setSize(getShape()->nodes.size());

   S32 ss = getShape()->details[0].subShapeNum;

   // all the nodes marked above need to have the default transform
   S32 a = getShape()->subShapeFirstNode[ss];
   S32 b = a + getShape()->subShapeNumNodes[ss];

   newAnim->subShape = ss;
   newAnim->startNode = a;
   newAnim->endNode = b;

   //Clean
   for (S32 i = a; i < b; i++)
   {
      mOwnerShapeInstance->mHandsOffNodes.set(i);

      getShape()->defaultRotations[i].getQuatF(&mOwnerShapeInstance->smNodeCurrentRotations[i]);
      mOwnerShapeInstance->smNodeCurrentTranslations[i] = getShape()->defaultTranslations[i];
   }

   //get the frame

   S32 j = 0;
   S32 start = seq->rotationMatters.start();
   S32 end = b;
   S32 nodeIndex = -1;

   for (nodeIndex = start; nodeIndex<end; seq->rotationMatters.next(nodeIndex), j++)
   {
      // skip nodes outside of this detail
      if (nodeIndex<a)
         continue;

      QuatF q1;
      mOwnerShapeInstance->smNodeCurrentRotations[nodeIndex] = getShape()->getRotation(*seq, 0, j, &q1);
   }

   j = 0;
   start = seq->rotationMatters.start();
   end = b;
   for (nodeIndex = start; nodeIndex<end; seq->translationMatters.next(nodeIndex), j++)
   {
      // skip nodes outside of this detail
      if (nodeIndex<a)
         continue;

      mOwnerShapeInstance->smNodeCurrentTranslations[nodeIndex] = getShape()->getTranslation(*seq, 0, j);
   }

   //

   // compute transforms
   for (S32 i = a; i<b; i++)
   {
      TSTransform::setMatrix(mOwnerShapeInstance->smNodeCurrentRotations[i],
         mOwnerShapeInstance->smNodeCurrentTranslations[i], &mOwnerShapeInstance->smNodeLocalTransforms[i]);

      newFrame.mNodes[i].position = mOwnerShapeInstance->smNodeCurrentTranslations[i];
      newFrame.mNodes[i].rotation = mOwnerShapeInstance->smNodeCurrentRotations[i];
      newFrame.mNodes[i].trans = mOwnerShapeInstance->smNodeLocalTransforms[i];
   }

   newAnim->mFrames.push_back(newFrame);

   return newAnim;
}

void AnimationController::animateNodes(S32 ss)
{

}



DefineEngineMethod(AnimationController, test, void, (), ,
   "Get the name of the indexed sequence.\n"
   "@param index index of the sequence to query (valid range is 0 - getSequenceCount()-1)\n"
   "@return the name of the sequence\n\n"
   "@tsexample\n"
   "// print the name of all sequences in the shape\n"
   "%count = %this.getSequenceCount();\n"
   "for ( %i = 0; %i < %count; %i++ )\n"
   "   echo( %i SPC %this.getSequenceName( %i ) );\n"
   "@endtsexample\n")
{
   return object->test();
}

DefineEngineMethod(AnimationController, testWalk, void, (), ,
   "")
{
   return object->testWalk();
}

DefineEngineMethod(AnimationController, testRun, void, (), ,
   "")
{
   return object->testRun();
}

DefineEngineMethod(AnimationController, testBlend, void, (F32 blend), (0),
   "")
{
   return object->testBlend(blend);
}