//-----------------------------------------------------------------------------
// Copyright (c) 2013 GarageGames, LLC
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

#ifndef _ANIMATION_ASSET_H_
#include "AnimationAsset.h"
#endif

#ifndef _ASSET_MANAGER_H_
#include "assets/assetManager.h"
#endif

#ifndef _CONSOLETYPES_H_
#include "console/consoleTypes.h"
#endif

#ifndef _TAML_
#include "persistence/taml/taml.h"
#endif

#ifndef _ASSET_PTR_H_
#include "assets/assetPtr.h"
#endif

#include "core/resourceManager.h"

#include "materials/materialDefinition.h"
#include "materials/materialManager.h"

#include "materials/customMaterialDefinition.h"

#include "materials/shaderData.h"

#include "platform/types.h"

// Debug Profiling.
#include "platform/profiler.h"

#include <assimp/cimport.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/types.h>

static U32 execDepth = 0;
static U32 journalDepth = 1;
const F64 minFrameRate = 15.0f;
const F64 maxFrameRate = 60.0f;

const F64 SAMPLE_FRAMERATE = 60.0f;

//-----------------------------------------------------------------------------

IMPLEMENT_CONOBJECT(AnimationAsset);

ConsoleType(TestAssetPtr, TypeAnimationAssetPtr, AnimationAsset, ASSET_ID_FIELD_PREFIX)

//-----------------------------------------------------------------------------

ConsoleGetType(TypeAnimationAssetPtr)
{
   // Fetch asset Id.
   return (*((AssetPtr<AnimationAsset>*)dptr)).getAssetId();
}

//-----------------------------------------------------------------------------

ConsoleSetType(TypeAnimationAssetPtr)
{
   // Was a single argument specified?
   if (argc == 1)
   {
      // Yes, so fetch field value.
      const char* pFieldValue = argv[0];

      // Fetch asset pointer.
      AssetPtr<AnimationAsset>* pAssetPtr = dynamic_cast<AssetPtr<AnimationAsset>*>((AssetPtrBase*)(dptr));

      // Is the asset pointer the correct type?
      if (pAssetPtr == NULL)
      {
         // No, so fail.
         //Con::warnf("(TypeTextureAssetPtr) - Failed to set asset Id '%d'.", pFieldValue);
         return;
      }

      // Set asset.
      pAssetPtr->setAssetId(pFieldValue);

      return;
   }

   // Warn.
   Con::warnf("(TypeTextureAssetPtr) - Cannot set multiple args to a single asset.");
}

//-----------------------------------------------------------------------------

AnimationAsset::AnimationAsset() :
mAcquireReferenceCount(0),
mpOwningAssetManager(NULL),
mAssetInitialized(false)
{
   // Generate an asset definition.
   mpAssetDefinition = new AssetDefinition();

   mModelScene = NULL;
}

//-----------------------------------------------------------------------------

AnimationAsset::~AnimationAsset()
{
   // If the asset manager does not own the asset then we own the
   // asset definition so delete it.
   if (!getOwned())
      delete mpAssetDefinition;
}

//-----------------------------------------------------------------------------

void AnimationAsset::initPersistFields()
{
   // Call parent.
   Parent::initPersistFields();

   addField("fileName", TypeFilename, Offset(mFileName, AnimationAsset), "Path to the script file we want to execute");
}

void AnimationAsset::initializeAsset()
{
   // Call parent.
   Parent::initializeAsset();

   if (dStrcmp(mFileName, "") == 0)
      return;

   loadShape();
}

bool AnimationAsset::loadShape()
{
   //mShape = ResourceManager::get().load(mFileName);

   mModelScene = mImporter.ReadFile(mFileName, 
      (aiProcessPreset_TargetRealtime_MaxQuality | aiProcess_FlipWindingOrder | aiProcess_FlipUVs | aiProcess_CalcTangentSpace) & ~aiProcess_RemoveRedundantMaterials);

   if (!mModelScene)
   {
      Con::errorf("AnimationAsset::loadShape : failed to load shape file!");
      return false; //if it failed to load, bail out
   }

   bool hasAnimations = mModelScene->HasAnimations();

   if (hasAnimations)
   {
      U32 numAnimation = mModelScene->mNumAnimations;

      for (U32 i = 0; i < numAnimation; i++)
      {
         aiAnimation* aiAnim = mModelScene->mAnimations[i];

         sequence newSeq;

         newSeq.name = aiAnim->mName.C_Str();

         //ticks per second
         newSeq.ticksPerSecond = aiAnim->mTicksPerSecond;

         //duration in ticks
         newSeq.duration = aiAnim->mDuration;

         F32 durInSec = newSeq.duration / newSeq.ticksPerSecond;

         U32 numberOfKeyframes = 0;

         U32 numAnimatedBones = aiAnim->mNumChannels;

         //first, run through real fast and find our max keyframe count so we can math the frame rate and duration
         U32 maxFrameCount = 0;

         for (U32 c = 0; c < numAnimatedBones; ++c)
         {
            aiNodeAnim* nodeAnim = aiAnim->mChannels[c];

            if (maxFrameCount < nodeAnim->mNumPositionKeys)
               maxFrameCount = nodeAnim->mNumPositionKeys;
            if (maxFrameCount < nodeAnim->mNumRotationKeys)
               maxFrameCount = nodeAnim->mNumRotationKeys;
            if (maxFrameCount < nodeAnim->mNumScalingKeys)
               maxFrameCount = nodeAnim->mNumScalingKeys;
         }

         for (U32 c = 0; c < numAnimatedBones; ++c)
         {
            aiNodeAnim* nodeAnim = aiAnim->mChannels[c];

            sequence::node seqNode;

            U32 posKeys = nodeAnim->mNumPositionKeys;
            for (U32 p = 0; p < posKeys; ++p)
            {
               aiVectorKey* aiPosKey = &nodeAnim->mPositionKeys[p];

               sequence::node::posKey posKey;
               posKey.time = aiPosKey->mTime;
               posKey.position = Point3F(aiPosKey->mValue.x, aiPosKey->mValue.y, aiPosKey->mValue.z);

               F32 delta = nodeAnim->mPositionKeys[p].mTime - nodeAnim->mPositionKeys[p-1].mTime;

               seqNode.positionKeys.push_back(posKey);
            }

            U32 rotKeys = nodeAnim->mNumRotationKeys;
            for (U32 p = 0; p < posKeys; ++p)
            {
               aiQuatKey* aiRotKey = &nodeAnim->mRotationKeys[p];
               
               sequence::node::quatKey rotKey;
               rotKey.time = aiRotKey->mTime;

               rotKey.rotation = QuatF(aiRotKey->mValue.x, aiRotKey->mValue.y, aiRotKey->mValue.z, aiRotKey->mValue.w);

               seqNode.rotationKeys.push_back(rotKey);
            }

            U32 scaleKeys = nodeAnim->mNumScalingKeys;
            for (U32 p = 0; p < posKeys; ++p)
            {
               aiVectorKey* aiScaleKey = &nodeAnim->mScalingKeys[p];

               sequence::node::scaleKey scaleKey;
               scaleKey.time = aiScaleKey->mTime;
               scaleKey.position = Point3F(aiScaleKey->mValue.x, aiScaleKey->mValue.y, aiScaleKey->mValue.z);

               seqNode.scaleKeys.push_back(scaleKey);
            }

            newSeq.nodes.push_back(seqNode);
         }

         mSequences.push_back(newSeq);
      }

      return true;
   }

   Con::errorf("AnimationAsset::loadShape(): Attempted to load a shape file with no meshes!");
   return false;
}

Vector<MatrixF> AnimationAsset::getNodeTransforms(U32 animation, F32 time)
{
   Vector<MatrixF> nodeTransforms;

   //interpolate the time value here

   return nodeTransforms;
}

String AnimationAsset::getSequenceName(U32 animation)
{
   return "";
}
/// Returns the duration in seconds
F32 AnimationAsset::getSequenceDuration(U32 animation)
{
   if (animation < 0 || animation >= mSequences.size())
      return -1;

   return mSequences[animation].duration;
}
//------------------------------------------------------------------------------
void AnimationAsset::copyTo(SimObject* object)
{
   // Call to parent.
   Parent::copyTo(object);
}