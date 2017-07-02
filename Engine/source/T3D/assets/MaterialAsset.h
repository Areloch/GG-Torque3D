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
#ifndef MATERIAL_ASSET_H
#define MATERIAL_ASSET_H


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

#ifndef _GFXDEVICE_H_
#include "gfx/gfxDevice.h"
#endif

#include "materials/matTextureTarget.h"
#include "materials/materialDefinition.h"
#include "materials/customMaterialDefinition.h"

//-----------------------------------------------------------------------------
class MaterialAsset : public AssetBase
{
   typedef AssetBase Parent;

   AssetManager*           mpOwningAssetManager;
   bool                    mAssetInitialized;
   AssetDefinition*        mpAssetDefinition;
   U32                     mAcquireReferenceCount;

   String                  mShaderGraphFile;
   String                  mScriptFile;
   String                  mMatDefinitionName;

   //
   //BaseMatInstance*        mMatInstance;
   //CustomMaterial*         mMaterial;
   /*GFXShaderRef            mShader;
   GFXShaderConstBufferRef mShaderConstBuffer;

   struct constHandle
   {
      GFXShaderConstHandle* handle;
      String handleName;

      enum type
      {
         Bool=0,
         Float,
         Float2,
         Float3,
         Float4,
         Texture2D,
         Texture3D
      }mType;

      bool boolVal;
      F32 floatVal;
      Point2F float2Val;
      Point3F float3Val;
      Point4F float4Val;
      GFXTextureObject* texture2DVal;
      GFXTexHandle texture3DVal;
   };

   Vector<constHandle> mConstHandlesList;*/

   //StringTableEntry mShaderFile;
   //StringTableEntry mShaderGraphFile;

   //String mDiffuseTexture;

   //String mDiffuseText;

public:
   MaterialAsset();
   virtual ~MaterialAsset();

   /// Engine.
   static void initPersistFields();
   virtual void copyTo(SimObject* object);

   virtual void initializeAsset();

   void compileShader();

   //void parseShaderData();
   //void parseShaderGraph();

   String getMaterialDefinitionName() { return mMatDefinitionName; }

   /// Declare Console Object.
   DECLARE_CONOBJECT(MaterialAsset);

protected:
   virtual void            onAssetRefresh(void) {}
};

DefineConsoleType(TypeMaterialAssetPtr, MaterialAsset)

#endif // _ASSET_BASE_H_

