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
#ifndef _SHAPE_ASSET_H_
#define _SHAPE_ASSET_H_

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

GFXDeclareVertexFormat(MeshVert)
{
   Point3F point;
   GFXVertexColor color;
   Point3F normal;
   Point3F tangent;
   Point2F texCoord;
};

typedef GFXVertexPNTTB VertexType;

//-----------------------------------------------------------------------------
class ShapeAsset : public AssetBase
{
   typedef AssetBase Parent;

   AssetManager*           mpOwningAssetManager;
   bool                    mAssetInitialized;
   AssetDefinition*        mpAssetDefinition;
   U32                     mAcquireReferenceCount;

public:
   enum
   {
      UniformScale = BIT(0),
      AlignedScale = BIT(1),
      ArbitraryScale = BIT(2),
      Blend = BIT(3),
      Cyclic = BIT(4),
      MakePath = BIT(5),
      HasTranslucency = BIT(6),
      AnyScale = UniformScale | AlignedScale | ArbitraryScale
   };

   /// Nodes hold the transforms in the shape's tree.  They are the bones of the skeleton.
   struct Node
   {
      S32 nameIndex;
      S32 parentIndex;

      // computed at runtime
      S32 firstObject;
      S32 firstChild;
      S32 nextSibling;
   };

   /// Objects hold renderable items (in particular meshes).
   ///
   /// Each object has a number of meshes associated with it.
   /// Each mesh corresponds to a different detail level.
   ///
   /// meshIndicesIndex points to numMeshes consecutive indices
   /// into the meshList and meshType vectors.  It indexes the
   /// meshIndexList vector (meshIndexList is merely a clearinghouse
   /// for the object's mesh lists).  Some indices may correspond to
   /// no mesh -- which means no mesh will be drawn for the part for
   /// the given detail level.  See comments on the meshIndexList
   /// for how null meshes are coded.
   ///
   /// @note Things are stored this way so that there are no pointers.
   ///       This makes serialization to disk dramatically simpler.
   struct Object
   {
      S32 nameIndex;
      S32 numMeshes;
      S32 startMeshIndex; ///< Index into meshes array.
      S32 nodeIndex;

      // computed at load
      S32 nextSibling;
   };

   /// Details are used for render detail selection.
   ///
   /// As the projected size of the shape changes,
   /// a different node structure can be used (subShape) and a different objectDetail can be selected
   /// for each object drawn.   Either of these two parameters can also stay constant, but presumably
   /// not both.  If size is negative then the detail level will never be selected by the standard
   /// detail selection process.  It will have to be selected by name.  Such details are "utility
   /// details" because they exist to hold data (node positions or collision information) but not
   /// normally to be drawn.  By default there will always be a "Ground" utility detail.
   ///
   /// Note that this struct should always be 32bit aligned
   /// as its required by assembleShape/disassembleShape.
   struct Detail
   {
      S32 nameIndex;
      S32 subShapeNum;
      S32 objectDetailNum;
      F32 size;
      F32 averageError;
      F32 maxError;
      S32 polyCount;

      /// These values are new autobillboard settings stored
      /// as part of the Detail struct in version 26 and above.
      /// @{

      S32 bbDimension;     ///< The size of the autobillboard image.
      S32 bbDetailLevel;   ///< The detail to render as the autobillboard.
      U32 bbEquatorSteps;  ///< The number of autobillboard images to capture around the equator.
      U32 bbPolarSteps;    ///< The number of autobillboard images to capture along the pole.
      F32 bbPolarAngle;    ///< The angle in radians at which the top/bottom autobillboard images should be displayed.
      U32 bbIncludePoles;  ///< If non-zero then top and bottom images are generated for the autobillboard.

      /// @}
   };

   /// @name Collision Accelerators
   ///
   /// For speeding up buildpolylist and support calls.
   /// @{
   struct ConvexHullAccelerator {
      S32      numVerts;
      Point3F* vertexList;
      Point3F* normalList;
      U8**     emitStrings;
   };
   ConvexHullAccelerator* getAccelerator(S32 dl);
   /// @}

   /// @name Shape Vector Data
   /// @{
   Vector<Node> mNodes;
   Vector<Object> mObjects;
   Vector<S32> mSubShapeFirstNode;
   Vector<S32> mSubShapeFirstObject;
   Vector<S32> mDetailFirstSkin;
   Vector<S32> mSubShapeNumNodes;
   Vector<S32> mSubShapeNumObjects;
   Vector<Detail> mDetails;
   Vector<Quat16> mDefaultRotations;
   Vector<Point3F> mDefaultTranslations;

   /// @}

   /// These are set up at load time, but memory is allocated along with loaded data
   /// @{
   Vector<S32> subShapeFirstTranslucentObject;

   /// @}

   /// @name Alpha Vectors
   /// these vectors describe how to transition between detail
   /// levels using alpha. "alpha-in" next detail as intraDL goes
   /// from alphaIn+alphaOut to alphaOut. "alpha-out" current
   /// detail level as intraDL goes from alphaOut to 0.
   /// @note
   ///   - intraDL is at 1 when if shape were any closer to us we'd be at dl-1
   ///   - intraDL is at 0 when if shape were any farther away we'd be at dl+1
   /// @{
   Vector<F32> alphaIn;
   Vector<F32> alphaOut;
   /// @}

   /// @name Resizeable vectors
   /// @{
   Vector<TSLastDetail*>            billboardDetails;
   Vector<ConvexHullAccelerator*>   detailCollisionAccelerators;
   Vector<String>                   names;

   /// @name Bounding
   /// @{
   F32 radius;
   F32 tubeRadius;
   Point3F center;
   Box3F bounds;

   /// @}

   // various...
   U32 mExporterVersion;
   F32 mSmallestVisibleSize;  ///< Computed at load time from details vector.
   S32 mSmallestVisibleDL;    ///< @see mSmallestVisibleSize
   S32 mReadVersion;          ///< File version that this shape was read from.
   U32 mFlags;                ///< hasTranslucancy
   U32 data;                  ///< User-defined data storage.

   /// If enabled detail selection will use the
   /// legacy screen error method for lod.
   /// @see setDetailFromScreenError
   bool mUseDetailFromScreenError;

   // TODO: This would be nice as Tuple<>
   struct LodPair
   {
      S8 level; // -1 to 128
      U8 intra; // encoded 0 to 1

      inline void set(S32 dl, F32 intraDL)
      {
         level = (S8)dl;
         intra = (S8)(intraDL * 255.0f);
      }

      inline void get(S32 &dl, F32 &intraDL)
      {
         dl = level;
         intraDL = (F32)intra / 255.0f;
      }
   };

   /// The lod lookup table where we mark down the detail
   /// level and intra-detail level for each pixel size.
   Vector<LodPair> mDetailLevelLookup;

   /// Is true if this shape contains skin meshes.
   bool mHasSkinMesh;

   S8* mShapeData;
   U32 mShapeDataSize;

   //
   struct Bone
   {
      String name;
      MatrixF baseTransform;

      struct vertWeight
      {
         U32 vertIndex;
         F32 weight;
      };

      Vector<vertWeight> weights;
   };

   struct SubMesh
   {
      enum
      {
         /// types...
         StandardMeshType = 0,
         SkinMeshType = 1,
         DecalMeshType = 2,
         SortedMeshType = 3,
         NullMeshType = 4,
         TypeMask = StandardMeshType | SkinMeshType | DecalMeshType | SortedMeshType | NullMeshType,

         /// flags (stored with meshType)...
         Billboard = BIT(31), HasDetailTexture = BIT(30),
         BillboardZAxis = BIT(29), UseEncodedNormals = BIT(28),
         FlagMask = Billboard | BillboardZAxis | HasDetailTexture | UseEncodedNormals
      };
      U32 meshType;

      Box3F mBounds;
      Point3F mCenter;
      F32 mRadius;
      F32 mVisibility;

      //In the event of a non-animated mesh, we can just use the static buffers that are pre-defined here.
      //Once Hardware skinning is in, the rendering will use this exclusively rather than temp buffers in the utilizing class
      GFXVertexBufferHandle< VertexType > vertexBuffer;
      GFXPrimitiveBufferHandle            primitiveBuffer;

      struct Vert
      {
         Point3F position;
         ColorI  color;
         Point3F normal;
         Point2F texCoord;
         Point2F texCoord2;
         Point3F tangent;
         Point3F bitangent;

         //4 bones per vert is pretty standard
         U32 boneIndicies[4];
         F32 boneWeights[4];
      };

      struct Face
      {
         Vector<U32> indicies;
      };

      Vector<Vert> verts;
      Vector<Face> faces;

      U32 materialIndex;
   };

   struct DetailLevel
   {
      //What size of pixels the minimum bound for showing this LOD is.
      //If the object is 2 pixels tall and the pixelSize is 2, it'll display this LODMesh
      //Assuming that there isn't a larger pixelSize LODMesh.
      U32 pixelSize;

      Vector<SubMesh> mSubMeshes;
   };

protected:
   Vector<Bone> mBones;
   Vector<SubMesh> mSubMeshes;
   Vector<String> mMaterialNames;
   Vector<BaseMaterialDefinition*> mMaterials;

   Vector<DetailLevel> mDetailLevels;

protected:
   StringTableEntry   mFileName;
   Assimp::Importer   mImporter;
   const aiScene*     mModelScene;

public:
   ShapeAsset();
   virtual ~ShapeAsset();

   /// Engine.
   static void initPersistFields();
   virtual void copyTo(SimObject* object);

   virtual void initializeAsset();

   /// Declare Console Object.
   DECLARE_CONOBJECT(ShapeAsset);

   bool loadShape();

   U32 getDetailLevelCount() { return mDetailLevels.size(); }

   U32 getSubmeshCount() { return mSubMeshes.size(); }

   SubMesh* getSubmesh(U32 index) { return &mSubMeshes[index]; }

   U32 getMaterialCount() { return mMaterials.size(); }

   BaseMaterialDefinition* getMaterial(U32 index) 
   {
      if (index < 0 || index > mMaterials.size())
         return NULL; 
      
      return mMaterials[index];
   }

   DetailLevel* getDetailLevel(S32 pixelLevel);

protected:
   virtual void            onAssetRefresh(void) {}
};

DefineConsoleType(TypeShapeAssetPtr, ShapeAsset)

#endif // _ASSET_BASE_H_

