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
#include "ts/tsShape.h"
#endif
#ifndef __RESOURCE_H__
#include "core/resource.h"
#endif

//-----------------------------------------------------------------------------
class AnimationAsset : public AssetBase
{
   typedef AssetBase Parent;

   AssetManager*           mpOwningAssetManager;
   bool                    mAssetInitialized;
   AssetDefinition*        mpAssetDefinition;
   U32                     mAcquireReferenceCount;

protected:
   StringTableEntry   mFileName;
   Resource<TSShape>	 mShape;

   //
   const char* source;
   const char* name;
   S32 start;
   S32 end;
   bool padRot;
   bool padTrans;

public:
   AnimationAsset();
   virtual ~AnimationAsset();

   /// Engine.
   static void initPersistFields();
   virtual void copyTo(SimObject* object);

   /// Declare Console Object.
   DECLARE_CONOBJECT(AnimationAsset);

protected:
   virtual void            initializeAsset(void) {}
   virtual void            onAssetRefresh(void) {}

   bool loadShape() {
      return false;
   }

   TSShape* getShape() { return mShape; }

   Resource<TSShape> getShapeResource() { return mShape; }
};

DefineConsoleType(TypeAnimationAssetPtr, AnimationAsset)

//-----------------------------------------------------------------------------
// TypeAssetId GuiInspectorField Class
//-----------------------------------------------------------------------------
/*class GuiInspectorTypeAnimationAssetPtr : public GuiInspectorTypeFileName
{
   typedef GuiInspectorTypeFileName Parent;
public:

   GuiBitmapButtonCtrl  *mShapeEdButton;

   DECLARE_CONOBJECT(GuiInspectorTypeAnimationAssetPtr);
   static void consoleInit();

   virtual GuiControl* constructEditControl();
   virtual bool updateRects();
};*/

#endif // _ASSET_BASE_H_

