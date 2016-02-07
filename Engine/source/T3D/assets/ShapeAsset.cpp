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

#include "materials/materialDefinition.h"
#include "materials/materialManager.h"

#include "materials/customMaterialDefinition.h"

#include "materials/shaderData.h"

// Debug Profiling.
#include "platform/profiler.h"

#include <assimp/cimport.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/types.h>

static U32 execDepth = 0;
static U32 journalDepth = 1;

GFXImplementVertexFormat(MeshVert)
{
   addElement("POSITION", GFXDeclType_Float3);
   addElement("COLOR", GFXDeclType_Color);
   addElement("NORMAL", GFXDeclType_Float3);
   addElement("TANGENT", GFXDeclType_Float3);
   addElement("TEXCOORD", GFXDeclType_Float2, 0);
};

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

   mDetailLevels.clear();
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

   U32 tmp = (aiProcessPreset_TargetRealtime_Quality | aiProcess_FlipWindingOrder | aiProcess_FlipUVs | aiProcess_CalcTangentSpace) & ~aiProcess_RemoveRedundantMaterials & ~aiProcess_OptimizeMeshes;

   U32 tmp2 = (aiProcessPreset_TargetRealtime_Quality | aiProcess_FlipWindingOrder | aiProcess_FlipUVs | aiProcess_CalcTangentSpace) & ~aiProcess_RemoveRedundantMaterials;

   mModelScene = mImporter.ReadFile(mFileName, 
      (aiProcessPreset_TargetRealtime_Quality | aiProcess_FlipWindingOrder | aiProcess_FlipUVs | aiProcess_CalcTangentSpace) & ~aiProcess_RemoveRedundantMaterials);

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

         //Get the relevent detail level
         DetailLevel *detail;

         bool found = false;
         for (U32 i = 0; i < mDetailLevels.size(); ++i)
         {
            if (mDetailLevels[i].pixelSize = LODNum)
            {
               detail = &mDetailLevels[i];
               found = true;
               break;
            }
         }

         if (!found)
         {
            DetailLevel newLevel;
            newLevel.pixelSize = LODNum;

            mDetailLevels.push_back(newLevel);

            detail = &newLevel;
         }

         aiMesh* aiSubMesh = mModelScene->mMeshes[i];

         SubMesh mesh;

         mesh.materialIndex = aiSubMesh->mMaterialIndex;

         U32 primitiveType = aiSubMesh->mPrimitiveTypes;
         
         //vertex info
         U32 vertCount = aiSubMesh->mNumVertices;
         for (U32 v = 0; v < vertCount; v++)
         {
            SubMesh::Vert newVert;

            aiVector3D* asVert = &aiSubMesh->mVertices[v];
            newVert.position = Point3F(asVert->x, asVert->y, asVert->z);

            aiVector3D* asNorm = &aiSubMesh->mNormals[v];
            newVert.normal = Point3F(asNorm->x, asNorm->y, asNorm->z);

            aiVector3D* asTangent = &aiSubMesh->mTangents[v];
            newVert.tangent = Point3F(asTangent->x, asTangent->y, asTangent->z);

            aiVector3D* asBitangent = &aiSubMesh->mBitangents[v];
            newVert.bitangent = Point3F(asBitangent->x, asBitangent->y, asBitangent->z);

            aiVector3D* texCoord = &aiSubMesh->mTextureCoords[0][v];
            newVert.texCoord = Point2F(texCoord->x, texCoord->y);

            //aiVector3D* texCoord2 = &aiSubMesh->mTextureCoords[1][v];
            //newVert.texCoord2 = Point2F(texCoord2->x, texCoord2->y);

            mesh.verts.push_back(newVert);
         }

         //get faces
         U32 faceCount = aiSubMesh->mNumFaces;
         for (U32 f = 0; f < faceCount; f++)
         {
            aiFace* asFace = &aiSubMesh->mFaces[f];

            if (asFace->mNumIndices != 3)
               continue; //non-triangle. need to add support for this?

            SubMesh::Face newFace;

            U32 indexCount = asFace->mNumIndices;
            for (U32 ind = 0; ind < indexCount; ind++)
            {
               U32 index = asFace->mIndices[ind];
               newFace.indicies.push_back(index);
            }

            mesh.faces.push_back(newFace);
         }

         Bone newBone;

         U32 boneCount = aiSubMesh->mNumBones;
         for (U32 b = 0; b < boneCount; b++)
         {
            newBone.name = aiSubMesh->mBones[b]->mName.C_Str();
            /*for (U32 m = 0; m < 16; ++m)
            {
               newBone.baseTransform[m] = *aiSubMesh->mBones[b]->mOffsetMatrix[m];
            }*/

            U32 numWeights = aiSubMesh->mBones[b]->mNumWeights;
            for (U32 w = 0; w < numWeights; ++w)
            {
               aiVertexWeight* aiWeight = aiSubMesh->mBones[b]->mWeights;

               Bone::vertWeight vertWeight;
               //vertWeight. = aiWeight->
            }
            //= mNumWeights
         }

         mBones.push_back(newBone);

         //now build the buffers
         mesh.vertexBuffer.set(GFX, mesh.verts.size(), GFXBufferTypeStatic);
         VertexType *pVert = mesh.vertexBuffer.lock();

         for (U32 v = 0; v < mesh.verts.size(); v++)
         {
            pVert->normal = mesh.verts[v].normal;
            pVert->B = mesh.verts[v].bitangent;
            pVert->T = mesh.verts[v].tangent;
            pVert->point = mesh.verts[v].position;
            pVert->texCoord = mesh.verts[v].texCoord;
            pVert->texCoord2 = mesh.verts[v].texCoord2;

            pVert++;
         }

         mesh.vertexBuffer.unlock();

         //primitive buffers
         // Allocate PB
         mesh.primitiveBuffer.set(GFX, mesh.faces.size() * 3, mesh.faces.size(), GFXBufferTypeStatic);

         U16 *pIndex;
         mesh.primitiveBuffer.lock(&pIndex);

         for (U16 f = 0; f < mesh.faces.size(); f++)
         {
            for (U16 i = 0; i < mesh.faces[f].indicies.size(); i++)
            {
               *pIndex = mesh.faces[f].indicies[i];
               pIndex++;
            }
         }

         mesh.primitiveBuffer.unlock();

         detail->mSubMeshes.push_back(mesh);
      }

      U32 materialCount = mModelScene->mNumMaterials;
      for (U32 m = 0; m < materialCount; m++)
      {
         aiMaterial* aiMat = mModelScene->mMaterials[m];

         aiString matName;
         aiMat->Get(AI_MATKEY_NAME, matName);

         bool found = false;
         for (U32 n = 0; n < mMaterialNames.size(); ++n)
         {
            if (mMaterialNames[n] == String(matName.C_Str()))
            {
               found = true;
               break;
            }
         }

         if (!found)
            mMaterialNames.push_back(matName.C_Str());

         aiColor3D color(0.f, 0.f, 0.f);
         aiMat->Get(AI_MATKEY_COLOR_DIFFUSE, color);
      }

      String astid = getAssetId();
      //String moduleName = AssetDatabase.getAssetModuleDefinition()->getName();
     //mpAssetDefinition->mpModuleDefinition->getName();
      for (U32 m = 0; m < mMaterialNames.size(); ++m)
      {
         /*BaseMaterialDefinition* mat;
         if (Sim::findObject(mMaterialNames[m], mat))
         {
            mMaterials.push_back(mat);
         }*/

         ShaderData* shdr = new ShaderData();
         shdr->registerObject();
         shdr->mDXPixelShaderName = "shaders/common/MeshAssetP.hlsl";
         shdr->mDXVertexShaderName = "shaders/common/MeshAssetV.hlsl";
         shdr->mPixVersion = 2.0;

         CustomMaterial* mat = new CustomMaterial();
         mat->registerObject();
         mat->mShaderData = shdr;
         mat->mSamplerNames[0] = "diffuseMap";
         mat->mTexFilename[0] = "core/art/grids/512_orange";

         mMaterials.push_back(mat);
      }

      return true;
   }

   Con::errorf("ShapeAsset::loadShape(): Attempted to load a shape file with no meshes!");
   return false;
}

ShapeAsset::DetailLevel* ShapeAsset::getDetailLevel(S32 pixelLevel)
{
   DetailLevel *DL;

   S32 bestLevel = -1;
   S32 bestLevelSize = 0;

   for (U32 i = 0; i < mDetailLevels.size(); ++i)
   {
      S32 pixelSize = mDetailLevels[i].pixelSize;
      if (pixelSize < pixelLevel && pixelSize >= bestLevelSize)
      {
         bestLevel = i;
         bestLevelSize = pixelSize;
      }
   }

   if (bestLevel == -1)
      return NULL;

   return &mDetailLevels[bestLevel];
}
//------------------------------------------------------------------------------

void ShapeAsset::copyTo(SimObject* object)
{
   // Call to parent.
   Parent::copyTo(object);
}