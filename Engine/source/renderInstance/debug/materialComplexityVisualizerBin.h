#pragma once

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
#ifndef _RENDERMESHMGR_H_
#include "renderInstance/renderMeshMgr.h"
#endif

class Material;

/// Basically the same as RenderMeshMgr, but will override the material of the instance.
class MaterailComplexityVisualizerBin : public RenderMeshMgr
{
   typedef RenderMeshMgr Parent;
public:
   MaterailComplexityVisualizerBin();
   MaterailComplexityVisualizerBin(RenderInstType riType, F32 renderOrder, F32 processAddOrder, BaseMatInstance* overrideMaterial);
   virtual ~MaterailComplexityVisualizerBin();

   void setOverrideMaterial(BaseMatInstance* overrideMaterial);

   // RenderBinManager interface
   virtual void render(SceneRenderState* state);

   DECLARE_CONOBJECT(MaterailComplexityVisualizerBin);
   static void initPersistFields();
private:
   BaseMatInstance* mOverrideInstance;
   SimObjectPtr<Material> mOverrideMaterial;

   F32 mMaxComplexity;

   static const char* _getOverrideMat(void* object, const char* data);
   static bool _setOverrideMat(void* object, const char* index, const char* data);
};
