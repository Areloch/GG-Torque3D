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
#define _ANIMATION_ASSET_H_

#ifndef _ASSET_BASE_H_
#include "assets/assetBase.h"
#endif

#ifndef _ASSET_DEFINITION_H_
#include "assets/assetDefinition.h"
#endif

#ifndef _STRINGUNIT_H_
#include "string/stringUnit.h"
#endif

#ifndef _ASSET_FIELD_TYPES_H_
#include "assets/assetFieldTypes.h"
#endif

#ifndef _TSSHAPE_H_
#include "ts/TSShape.h"
#endif
#ifndef __RESOURCE_H__
#include "core/resource.h"
#endif


#ifndef __AI_SCENE_H_INC__
#include <assimp/scene.h>
#endif
#ifndef AI_QUATERNION_H_INC
#include <assimp/quaternion.h>
#endif
#ifndef AI_ANIM_H_INC
#include <assimp/anim.h>
#endif
#ifndef INCLUDED_AI_ASSIMP_HPP
#include <assimp/Importer.hpp>
#endif

#include "materials/baseMaterialDefinition.h"

typedef GFXVertexPNTTB VertexType;

//-----------------------------------------------------------------------------
class AnimationAsset : public AssetBase
{
   typedef AssetBase Parent;

   AssetManager*           mpOwningAssetManager;
   bool                    mAssetInitialized;
   AssetDefinition*        mpAssetDefinition;
   U32                     mAcquireReferenceCount;

   enum AnimationConstants
   {
      ANIMATION_FRAMERATE = 60
   };

public:
   struct sequence
   {
      String name;
      F32 ticksPerSecond;
      F32 duration;

      struct node
      {
         struct posKey
         {
            F32 time;
            Point3F position;
         };
         Vector<posKey> positionKeys;

         struct quatKey
         {
            F32 time;
            QuatF rotation;
         };
         Vector<quatKey> rotationKeys;

         struct scaleKey
         {
            F32 time;
            Point3F position;
         };
         Vector<scaleKey> scaleKeys;
      };

      Vector<node> nodes;
   };

   struct bone
   {
      String name;
      MatrixF baseTransform;
   };

protected:
   Vector<bone> mBones;
   Vector<sequence> mSequences;

protected:
   StringTableEntry   mFileName;
   Assimp::Importer   mImporter;
   const aiScene*     mModelScene;

public:
   AnimationAsset();
   virtual ~AnimationAsset();

   /// Engine.
   static void initPersistFields();
   virtual void copyTo(SimObject* object);

   virtual void initializeAsset();

   /// Declare Console Object.
   DECLARE_CONOBJECT(AnimationAsset);

   bool loadShape();

   U32 getSequenceCount() { return mSequences.size(); }

   Vector<MatrixF> getNodeTransforms(U32 animation, F32 time);
   String getSequenceName(U32 animation);
   F32 getSequenceDuration(U32 animation);

protected:
   virtual void            onAssetRefresh(void) {}
};

DefineConsoleType(TypeShapeAssetPtr, AnimationAsset)

#endif // _ANIMATION_BASE_H_

