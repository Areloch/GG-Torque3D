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

#include <assimp/cimport.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/types.h>

// Debug Profiling.
#include "platform/profiler.h"

//-----------------------------------------------------------------------------

IMPLEMENT_CONOBJECT(ShapeAsset);

ConsoleType(assetIdString, TypeShapeAssetPtr, String, ASSET_ID_FIELD_PREFIX)

//-----------------------------------------------------------------------------

ConsoleGetType(TypeShapeAssetPtr)
{
   // Fetch asset Id.
   return *((StringTableEntry*)dptr);
}

//-----------------------------------------------------------------------------

ConsoleSetType(TypeShapeAssetPtr)
{
   // Was a single argument specified?
   if (argc == 1)
   {
      // Yes, so fetch field value.
      const char* pFieldValue = argv[0];

      // Fetch asset Id.
      StringTableEntry* assetId = (StringTableEntry*)(dptr);

      // Update asset value.
      *assetId = StringTable->insert(pFieldValue);

      return;
   }

   // Warn.
   Con::warnf("(TypeAssetId) - Cannot set multiple args to a single asset.");
}

//-----------------------------------------------------------------------------

ShapeAsset::ShapeAsset() :
mpOwningAssetManager(NULL),
mAssetInitialized(false),
mAcquireReferenceCount(0)
{
   mModelScene = NULL;

   mNewShape = false;

   mDetailLevels.clear();
}

//-----------------------------------------------------------------------------

ShapeAsset::~ShapeAsset()
{
   // If the asset manager does not own the asset then we own the
   // asset definition so delete it.
   if (!getOwned() && mpAssetDefinition)
      delete mpAssetDefinition;
}

//-----------------------------------------------------------------------------

void ShapeAsset::initPersistFields()
{
   // Call parent.
   Parent::initPersistFields();

   addField("fileName", TypeFilename, Offset(mFileName, ShapeAsset), "Path to the script file we want to execute");
   addField("isNewShape", TypeBool, Offset(mNewShape, ShapeAsset), "Is this a new Shape, utilizing the new Shape mesh system?");
}

void ShapeAsset::initializeAsset()
{
   // Call parent.
   Parent::initializeAsset();

   if (dStrcmp(mFileName, "") == 0)
      return;

   loadShape();

   loadAnimations();

   //If we don't have an associated preview image for this model, make one now
   //based on the imposter creation code
   /*
   GBitmap *imposter = NULL;
   GBitmap *normalmap = NULL;
   GBitmap destBmp( texSize.x, texSize.y, true, format );
   GBitmap destNormal( texSize.x, texSize.y, true, format );

   U32 mipLevels = destBmp.getNumMipLevels();

   ImposterCapture *imposterCap = new ImposterCapture();

   F32 equatorStepSize = M_2PI_F / (F32)mNumEquatorSteps;

   static const MatrixF topXfm( EulerF( -M_PI_F / 2.0f, 0, 0 ) );
   static const MatrixF bottomXfm( EulerF( M_PI_F / 2.0f, 0, 0 ) );

   MatrixF angMat;

   F32 polarStepSize = 0.0f;
   if ( mNumPolarSteps > 0 )
      polarStepSize = -( 0.5f * M_PI_F - mDegToRad( mPolarAngle ) ) / (F32)mNumPolarSteps;

   PROFILE_START(TSLastDetail_snapshots);

   S32 currDim = mDim;
   for ( S32 mip = 0; mip < mipLevels; mip++ )
   {
      if ( currDim < 1 )
         currDim = 1;
      
      dMemset( destBmp.getWritableBits(mip), 0, destBmp.getWidth(mip) * destBmp.getHeight(mip) * GFXFormat_getByteSize( format ) );
      dMemset( destNormal.getWritableBits(mip), 0, destNormal.getWidth(mip) * destNormal.getHeight(mip) * GFXFormat_getByteSize( format ) );

      bitmaps.clear();
      normalmaps.clear();

      F32 rotX = 0.0f;
      if ( mNumPolarSteps > 0 )
         rotX = -( mDegToRad( mPolarAngle ) - 0.5f * M_PI_F );

      // We capture the images in a particular order which must
      // match the order expected by the imposter renderer.

      imposterCap->begin( shape, mDl, currDim, mRadius, mCenter );

      for ( U32 j=0; j < (2 * mNumPolarSteps + 1); j++ )
      {
         F32 rotZ = -M_PI_F / 2.0f;

         for ( U32 k=0; k < mNumEquatorSteps; k++ )
         {            
            angMat.mul( MatrixF( EulerF( rotX, 0, 0 ) ),
                        MatrixF( EulerF( 0, 0, rotZ ) ) );

            imposterCap->capture( angMat, &imposter, &normalmap );

            bitmaps.push_back( imposter );
            normalmaps.push_back( normalmap );

            rotZ += equatorStepSize;
         }

         rotX += polarStepSize;

         if ( mIncludePoles )
         {
            imposterCap->capture( topXfm, &imposter, &normalmap );

            bitmaps.push_back(imposter);
            normalmaps.push_back( normalmap );

            imposterCap->capture( bottomXfm, &imposter, &normalmap );

            bitmaps.push_back( imposter );
            normalmaps.push_back( normalmap );
         }         
      }

      imposterCap->end();

      Point2I texSize( destBmp.getWidth(mip), destBmp.getHeight(mip) );

      // Ok... pack in bitmaps till we run out.
      for ( S32 y=0; y+currDim <= texSize.y; )
      {
         for ( S32 x=0; x+currDim <= texSize.x; )
         {
            // Copy the next bitmap to the dest texture.
            GBitmap* bmp = bitmaps.first();
            bitmaps.pop_front();
            destBmp.copyRect( bmp, RectI( 0, 0, currDim, currDim ), Point2I( x, y ), 0, mip );
            delete bmp;

            // Copy the next normal to the dest texture.
            GBitmap* normalmap = normalmaps.first();
            normalmaps.pop_front();
            destNormal.copyRect( normalmap, RectI( 0, 0, currDim, currDim ), Point2I( x, y ), 0, mip );
            delete normalmap;

            // Did we finish?
            if ( bitmaps.empty() )
               break;

            x += currDim;
         }

         // Did we finish?
         if ( bitmaps.empty() )
            break;

         y += currDim;
      }

      // Next mip...
      currDim /= 2;
   }

   PROFILE_END(); // TSLastDetail_snapshots

   delete imposterCap;
   delete shape;   
   
   
   // Should we dump the images?
   if ( Con::getBoolVariable( "$TSLastDetail::dumpImposters", false ) )
   {
      String imposterPath = mCachePath + ".imposter.png";
      String normalsPath = mCachePath + ".imposter_normals.png";

      FileStream stream;
      if ( stream.open( imposterPath, Torque::FS::File::Write  ) )
         destBmp.writeBitmap( "png", stream );
      stream.close();

      if ( stream.open( normalsPath, Torque::FS::File::Write ) )
         destNormal.writeBitmap( "png", stream );
      stream.close();
   }

   // DEBUG: Some code to force usage of a test image.
   //GBitmap* tempMap = GBitmap::load( "./forest/data/test1234.png" );
   //tempMap->extrudeMipLevels();
   //mTexture.set( tempMap, &GFXDefaultStaticDiffuseProfile, false );
   //delete tempMap;

   DDSFile *ddsDest = DDSFile::createDDSFileFromGBitmap( &destBmp );
   ImageUtil::ddsCompress( ddsDest, GFXFormatBC2 );

   DDSFile *ddsNormals = DDSFile::createDDSFileFromGBitmap( &destNormal );
   ImageUtil::ddsCompress( ddsNormals, GFXFormatBC3 );

   // Finally save the imposters to disk.
   FileStream fs;
   if ( fs.open( _getDiffuseMapPath(), Torque::FS::File::Write ) )
   {
      ddsDest->write( fs );
      fs.close();
   }
   if ( fs.open( _getNormalMapPath(), Torque::FS::File::Write ) )
   {
      ddsNormals->write( fs );
      fs.close();
   }

   delete ddsDest;
   delete ddsNormals;

   // If we did a begin then end it now.
   if ( !sceneBegun )
      GFX->endScene();
   */
}

bool ShapeAsset::loadShape()
{
   if (!mNewShape)
   {
      mShape = ResourceManager::get().load(mFileName);

      if (!mShape)
      {
         Con::errorf("StaticMesh::updateShape : failed to load shape file!");
         return false; //if it failed to load, bail out
      }
   }
   else
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

      bounds = Box3F::Zero;

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

               bounds.extend(newVert.position);

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
            BaseMaterialDefinition* mat;
            if (Sim::findObject(mMaterialNames[m], mat))
            {
               mMaterials.push_back(mat);
            }

            /*ShaderData* shdr = new ShaderData();
            shdr->registerObject();
            shdr->mDXPixelShaderName = "shaders/common/MeshAssetP.hlsl";
            shdr->mDXVertexShaderName = "shaders/common/MeshAssetV.hlsl";
            shdr->mPixVersion = 2.0;

            CustomMaterial* mat = new CustomMaterial();
            mat->registerObject();
            mat->mShaderData = shdr;
            mat->mSamplerNames[0] = "diffuseMap";
            mat->mTexFilename[0] = "core/art/grids/512_orange";

            mMaterials.push_back(mat);*/
         }

         return true;
      }

      Con::errorf("ShapeAsset::loadShape(): Attempted to load a shape file with no meshes!");
      return false;
   }

   /*OptimizedPolyList polyList;
   Vector<BaseMatInstance*> matList;
   //build the poly list if we need a static pull of the geometry later
   U32 i = 0;
   for (U32 i = 0; i < mShape->meshes.size(); i++)
   {
      String meshName = mShape->getMeshName(i);

      OptimizedPolyList geom;
      U32 surfaceKey;
      TSMaterialList* mats;
      mShape->findMesh(meshName)->buildPolyList(0, &geom, surfaceKey, mats);

      //geom.mPoints
   }*/
   
   return true;
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

void ShapeAsset::onAssetRefresh(void)
{
}

//-----------------------------------------------------------------------------
// GuiInspectorTypeAssetId
//-----------------------------------------------------------------------------

IMPLEMENT_CONOBJECT(GuiInspectorTypeShapeAssetPtr);

ConsoleDocClass(GuiInspectorTypeShapeAssetPtr,
   "@brief Inspector field type for Shapes\n\n"
   "Editor use only.\n\n"
   "@internal"
   );

void GuiInspectorTypeShapeAssetPtr::consoleInit()
{
   Parent::consoleInit();

   ConsoleBaseType::getType(TypeShapeAssetPtr)->setInspectorFieldType("GuiInspectorTypeShapeAssetPtr");
}

GuiControl* GuiInspectorTypeShapeAssetPtr::constructEditControl()
{
   // Create base filename edit controls
   GuiControl *retCtrl = Parent::constructEditControl();
   if (retCtrl == NULL)
      return retCtrl;

   // Change filespec
   char szBuffer[512];
   dSprintf(szBuffer, sizeof(szBuffer), "AssetBrowser.showDialog(\"ShapeAsset\", \"AssetBrowser.changeAsset\", %d, %s);", 
      mInspector->getComponentGroupTargetId(), mCaption);
   mBrowseButton->setField("Command", szBuffer);

   setDataField(StringTable->insert("ComponentOwner"), NULL, String::ToString(mInspector->getComponentGroupTargetId()).c_str());

   // Create "Open in ShapeEditor" button
   mShapeEdButton = new GuiBitmapButtonCtrl();

   dSprintf(szBuffer, sizeof(szBuffer), "ShapeEditorPlugin.openShapeAsset(%d.getText());", retCtrl->getId());
   mShapeEdButton->setField("Command", szBuffer);

   char bitmapName[512] = "tools/worldEditor/images/toolbar/shape-editor";
   mShapeEdButton->setBitmap(bitmapName);

   mShapeEdButton->setDataField(StringTable->insert("Profile"), NULL, "GuiButtonProfile");
   mShapeEdButton->setDataField(StringTable->insert("tooltipprofile"), NULL, "GuiToolTipProfile");
   mShapeEdButton->setDataField(StringTable->insert("hovertime"), NULL, "1000");
   mShapeEdButton->setDataField(StringTable->insert("tooltip"), NULL, "Open this file in the Shape Editor");

   mShapeEdButton->registerObject();
   addObject(mShapeEdButton);

   return retCtrl;
}

bool GuiInspectorTypeShapeAssetPtr::updateRects()
{
   S32 dividerPos, dividerMargin;
   mInspector->getDivider(dividerPos, dividerMargin);
   Point2I fieldExtent = getExtent();
   Point2I fieldPos = getPosition();

   mCaptionRect.set(0, 0, fieldExtent.x - dividerPos - dividerMargin, fieldExtent.y);
   mEditCtrlRect.set(fieldExtent.x - dividerPos + dividerMargin, 1, dividerPos - dividerMargin - 34, fieldExtent.y);

   bool resized = mEdit->resize(mEditCtrlRect.point, mEditCtrlRect.extent);
   if (mBrowseButton != NULL)
   {
      mBrowseRect.set(fieldExtent.x - 32, 2, 14, fieldExtent.y - 4);
      resized |= mBrowseButton->resize(mBrowseRect.point, mBrowseRect.extent);
   }

   if (mShapeEdButton != NULL)
   {
      RectI shapeEdRect(fieldExtent.x - 16, 2, 14, fieldExtent.y - 4);
      resized |= mShapeEdButton->resize(shapeEdRect.point, shapeEdRect.extent);
   }

   return resized;
}
