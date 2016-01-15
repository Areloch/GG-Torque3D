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
#include "ShapeAsset.h"
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

// Debug Profiling.
#include "platform/profiler.h"

#include <assimp/cimport.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/types.h>

static U32 execDepth = 0;
static U32 journalDepth = 1;

//-----------------------------------------------------------------------------

IMPLEMENT_CONOBJECT(ShapeAsset);

ConsoleType(TestAssetPtr, TypeShapeAssetPtr, ShapeAsset, ASSET_ID_FIELD_PREFIX)

//-----------------------------------------------------------------------------

ConsoleGetType(TypeShapeAssetPtr)
{
   // Fetch asset Id.
   return (*((AssetPtr<ShapeAsset>*)dptr)).getAssetId();
}

//-----------------------------------------------------------------------------

ConsoleSetType(TypeShapeAssetPtr)
{
   // Was a single argument specified?
   if (argc == 1)
   {
      // Yes, so fetch field value.
      const char* pFieldValue = argv[0];

      // Fetch asset pointer.
      AssetPtr<ShapeAsset>* pAssetPtr = dynamic_cast<AssetPtr<ShapeAsset>*>((AssetPtrBase*)(dptr));

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

ShapeAsset::ShapeAsset() :
mAcquireReferenceCount(0),
mpOwningAssetManager(NULL),
mAssetInitialized(false)
{
   // Generate an asset definition.
   mpAssetDefinition = new AssetDefinition();

   mModelScene = NULL;
}

//-----------------------------------------------------------------------------

ShapeAsset::~ShapeAsset()
{
   // If the asset manager does not own the asset then we own the
   // asset definition so delete it.
   if (!getOwned())
      delete mpAssetDefinition;
}

//-----------------------------------------------------------------------------

void ShapeAsset::initPersistFields()
{
   // Call parent.
   Parent::initPersistFields();

   addField("fileName", TypeFilename, Offset(mFileName, ShapeAsset), "Path to the script file we want to execute");
}

void ShapeAsset::initializeAsset()
{
   // Call parent.
   Parent::initializeAsset();

   if (dStrcmp(mFileName, "") == 0)
      return;

   loadShape();
}

bool ShapeAsset::loadShape()
{
   //mShape = ResourceManager::get().load(mFileName);

   mModelScene = mImporter.ReadFile(mFileName, 
      aiProcessPreset_TargetRealtime_MaxQuality | aiProcess_FlipWindingOrder | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);

   if (!mModelScene)
   {
      Con::errorf("ShapeAsset::loadShape : failed to load shape file!");
      return false; //if it failed to load, bail out
   }

   bool hasMeshes = mModelScene->HasMeshes();

   if (hasMeshes)
   {
      U32 numMeshes = mModelScene->mNumMeshes;

      for (U32 i = 0; i < numMeshes; i++)
      {
         String meshName = mModelScene->mMeshes[i]->mName.C_Str();

         S32 LODNum;
         String::GetTrailingNumber(meshName, LODNum);

         aiMesh* aiSubMesh = mModelScene->mMeshes[i];

         subMesh mesh;

         U32 matIndex = aiSubMesh->mMaterialIndex;

         U32 primitiveType = aiSubMesh->mPrimitiveTypes;
         
         //vertex info
         U32 vertCount = aiSubMesh->mNumVertices;
         for (U32 v = 0; v < vertCount; v++)
         {
            subMesh::vert newVert;

            aiVector3D* asVert = &aiSubMesh->mVertices[v];
            newVert.position = Point3F(asVert->x, asVert->y, asVert->z);

            aiVector3D* asNorm = &aiSubMesh->mNormals[v];
            newVert.normal = Point3F(asNorm->x, asNorm->y, asNorm->z);

            aiVector3D* asTangent = &aiSubMesh->mTangents[v];
            newVert.tangent = Point3F(asTangent->x, asTangent->y, asTangent->z);

            aiVector3D* asBitangent = &aiSubMesh->mBitangents[v];
            newVert.bitangent = Point3F(asBitangent->x, asBitangent->y, asBitangent->z);

            aiVector3D* texCoord = &aiSubMesh->mTextureCoords[0][v];
            newVert.texCoord = Point2F(asBitangent->x, asBitangent->y);

            mesh.verts.push_back(newVert);
         }

         //get faces
         U32 faceCount = aiSubMesh->mNumFaces;
         for (U32 f = 0; f < faceCount; f++)
         {
            aiFace* asFace = &aiSubMesh->mFaces[f];

            subMesh::face newFace;

            U32 indexCount = asFace->mNumIndices;
            for (U32 ind = 0; ind < indexCount; ind++)
            {
               U32 index = asFace->mIndices[ind];
               newFace.indicies.push_back(index);
            }

            mesh.faces.push_back(newFace);
         }

         U32 boneCount = aiSubMesh->mNumBones;
         /*for (U32 b = 0; b < boneCount; b++)
         {
            aiSubMesh->mBones
         }*/

         mSubMeshes.push_back(mesh);
      }

      U32 materialCount = mModelScene->mNumMaterials;
      for (U32 m = 0; m < materialCount; m++)
      {
         aiMaterial* aiMat = mModelScene->mMaterials[m];

         //aiMat->
      }
   }

   return true;
}

//------------------------------------------------------------------------------

void ShapeAsset::copyTo(SimObject* object)
{
   // Call to parent.
   Parent::copyTo(object);
}