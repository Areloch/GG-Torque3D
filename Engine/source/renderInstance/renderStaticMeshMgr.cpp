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

#include "platform/platform.h"
#include "renderInstance/RenderStaticMeshMgr.h"

#include "console/consoleTypes.h"
#include "gfx/gfxTransformSaver.h"
#include "gfx/gfxPrimitiveBuffer.h"
#include "materials/sceneData.h"
#include "materials/processedMaterial.h"
#include "materials/materialManager.h"
#include "scene/sceneRenderState.h"
#include "gfx/gfxDebugEvent.h"
#include "math/util/matrixSet.h"

#include "collision/optimizedPolyList.h"


IMPLEMENT_CONOBJECT(RenderStaticMeshMgr);

ConsoleDocClass( RenderStaticMeshMgr, 
   "@brief A render bin for mesh rendering.\n\n"
   "This is the primary render bin in Torque which does most of the "
   "work of rendering DTS shapes and arbitrary mesh geometry.  It knows "
   "how to render mesh instances using materials and supports hardware mesh "
   "instancing.\n\n"
   "@ingroup RenderBin\n" );


RenderStaticMeshMgr::RenderStaticMeshMgr()
: RenderBinManager(RenderPassManager::RIT_StaticMesh, 1.0f, 1.0f)
{
}

RenderStaticMeshMgr::RenderStaticMeshMgr(RenderInstType riType, F32 renderOrder, F32 processAddOrder)
   : RenderBinManager(riType, renderOrder, processAddOrder)
{
}

void RenderStaticMeshMgr::init()
{
   GFXStateBlockDesc d;

   d.cullDefined = true;
   d.cullMode = GFXCullCCW;
   d.samplersDefined = true;
   d.samplers[0] = GFXSamplerStateDesc::getWrapLinear();

   mNormalSB = GFX->createStateBlock(d);

   d.cullMode = GFXCullCW;
   mReflectSB = GFX->createStateBlock(d);
}

void RenderStaticMeshMgr::initPersistFields()
{
   Parent::initPersistFields();
}

//-----------------------------------------------------------------------------
// add element
//-----------------------------------------------------------------------------
void RenderStaticMeshMgr::addElement( RenderInst *inst )
{
   // If this instance is translucent handle it in RenderTranslucentMgr
   if (inst->translucentSort)
      return;

   AssertFatal( inst->defaultKey != 0, "RenderStaticMeshMgr::addElement() - Got null sort key... did you forget to set it?" );

   internalAddElement(inst);
}

void RenderStaticMeshMgr::addElement(SceneObject* elementOwner, OptimizedPolyList* geometry)
{
   Vector<U32> tempIndices;
   tempIndices.reserve(4);

   bool found = false;
   for (U32 i = 0; i < mElements.size(); ++i)
   {
      if (mElements[i].elementOwner != NULL && mElements[i].elementOwner->getId() == elementOwner->getId())
      {
         found = true;
         mElements[i].geometry = *geometry;
         break;
      }
   }

   if (!found)
   {
      StaticMeshElement newElement;

      newElement.elementOwner = elementOwner;
      newElement.geometry = *geometry;

      mElements.push_back(newElement);
   }

   rebuildBuffers();
}

void RenderStaticMeshMgr::rebuildBuffers()
{
   U32 BUFFER_SIZE = 65000;
   Vector<U32> tempIndices;
   tempIndices.reserve(4);

   for (U32 i = 0; i < mElements.size(); ++i)
   {
      for (U32 j = 0; j < mElements[i].geometry.mPolyList.size(); j++)
      {
         const OptimizedPolyList::Poly& poly = mElements[i].geometry.mPolyList[j];

         if (poly.vertexCount < 3)
            continue;

         tempIndices.setSize(poly.vertexCount);
         dMemset(tempIndices.address(), 0, poly.vertexCount);

         if (poly.type == OptimizedPolyList::TriangleStrip)
         {
            tempIndices[0] = 0;
            U32 idx = 1;

            for (U32 k = 1; k < poly.vertexCount; k += 2)
               tempIndices[idx++] = k;

            for (U32 k = ((poly.vertexCount - 1) & (~0x1)); k > 0; k -= 2)
               tempIndices[idx++] = k;
         }
         else if (poly.type == OptimizedPolyList::TriangleList)
         {
            for (U32 k = 0; k < poly.vertexCount; k++)
               tempIndices[k] = k;
         }
         else
            continue;

         //got our data, now insert it into the correct buffer!
         S32 bufferId = findBufferSetByMaterial(poly.material);

         //see if this would push us over our buffer size limit, if it is, make a new buffer for this set
         if (mBuffers[bufferId].buffers.last().vertCount + 3 > BUFFER_SIZE
            || mBuffers[bufferId].buffers.last().primCount + 1 > BUFFER_SIZE)
         {
            //yep, we'll overstep with this, so spool up a new buffer in this set
            BufferSet::Buffers newBuffer = BufferSet::Buffers();
            mBuffers[bufferId].buffers.push_back(newBuffer);
         }

         const U32& firstIdx = mElements[i].geometry.mIndexList[poly.vertexStart];
         const OptimizedPolyList::VertIndex& firstVertIdx = mElements[i].geometry.mVertexList[firstIdx];

         for (U32 k = 1; k < poly.vertexCount - 1; k++)
         {
            const U32& secondIdx = mElements[i].geometry.mIndexList[poly.vertexStart + tempIndices[k]];
            const U32& thirdIdx = mElements[i].geometry.mIndexList[poly.vertexStart + tempIndices[k + 1]];

            const OptimizedPolyList::VertIndex& secondVertIdx = mElements[i].geometry.mVertexList[secondIdx];
            const OptimizedPolyList::VertIndex& thirdVertIdx = mElements[i].geometry.mVertexList[thirdIdx];

            Point3F points[3];
            points[0] = mElements[i].geometry.mPoints[firstVertIdx.vertIdx];
            points[1] = mElements[i].geometry.mPoints[secondVertIdx.vertIdx];
            points[2] = mElements[i].geometry.mPoints[thirdVertIdx.vertIdx];

            Point3F normals[3];
            normals[0] = mElements[i].geometry.mNormals[firstVertIdx.vertIdx];
            normals[1] = mElements[i].geometry.mNormals[secondVertIdx.vertIdx];
            normals[2] = mElements[i].geometry.mNormals[thirdVertIdx.vertIdx];

            Point3F tangents[3];
            tangents[0] = mCross(points[1] - points[0], normals[0]);
            tangents[1] = mCross(points[2] - points[1], normals[1]);
            tangents[2] = mCross(points[0] - points[2], normals[2]);

            Point2F uvs[3];
            uvs[0] = mElements[i].geometry.mUV0s[firstVertIdx.uv0Idx];
            uvs[1] = mElements[i].geometry.mUV0s[secondVertIdx.uv0Idx];
            uvs[2] = mElements[i].geometry.mUV0s[thirdVertIdx.uv0Idx];

            mBuffers[bufferId].vertCount += 3;
            mBuffers[bufferId].primCount += 1;

            for (U32 i = 0; i < 3; ++i)
            {
               //Build the vert and store it to the buffers!
               GFXVertexPNTT bufVert;
               bufVert.point = points[i];
               bufVert.normal = normals[i];
               bufVert.tangent = tangents[i];
               bufVert.texCoord = uvs[i];

               mBuffers[bufferId].buffers.last().vertData.push_back(bufVert);

               U32 vertPrimId = mBuffers[bufferId].buffers.last().vertData.size() - 1;
               mBuffers[bufferId].buffers.last().primData.push_back(vertPrimId);
            }
         }
      }
   }
}

//-----------------------------------------------------------------------------
// render
//-----------------------------------------------------------------------------
void RenderStaticMeshMgr::render(SceneRenderState * state)
{
   PROFILE_SCOPE(RenderStaticMeshMgr_render);

   // Early out if nothing to draw.
   if(!mElementList.size())
      return;

   GFXDEBUGEVENT_SCOPE( RenderStaticMeshMgr_Render, ColorI::GREEN );

   // Automagically save & restore our viewport and transforms.
   GFXTransformSaver saver;

   // Restore transforms
   MatrixSet &matrixSet = getRenderPass()->getMatrixSet();
   matrixSet.restoreSceneViewProjection();

   // init loop data
   GFXTextureObject *lastLM = NULL;
   GFXCubemap *lastCubemap = NULL;
   GFXTextureObject *lastReflectTex = NULL;
   GFXTextureObject *lastMiscTex = NULL;
   GFXTextureObject *lastAccuTex = NULL;

   SceneData sgData;
   sgData.init(state);

   for (U32 i = 0; i < mBuffers.size(); i++)
   {
      for (U32 b = 0; b < mBuffers[i].buffers.size(); b++)
      {
         if (mBuffers[i].buffers[b].vertData.empty())
            continue;

         BaseMatInstance *mat = mBufferMaterials[mBuffers[i].surfaceMaterialId].mMaterialInst;

         if (!mat)
            mat = MATMGR->getWarningMatInstance();

         RenderPassManager* renderPass = getRenderPass();

         matrixSet.setWorld(*passRI->objectToWorld);
         matrixSet.setView(*renderPass->allocSharedXform(RenderPassManager::View));
         matrixSet.setProjection(*renderPass->allocSharedXform(RenderPassManager::Projection));
         mat->setTransforms(matrixSet, state);

         setupSGData(passRI, sgData);
         mat->setSceneInfo(state, sgData);

         /*sgData.lightmap = passRI->lightmap;
         sgData.cubemap = passRI->cubemap;
         sgData.reflectTex = passRI->reflectTex;
         sgData.accuTex = passRI->accuTex;*/

         mat->setTextureStages(state, sgData);

         // Setup the vertex and index buffers.
         mat->setBuffers(&mBuffers[i].buffers[b].vertexBuffer, &mBuffers[i].buffers[b].primitiveBuffer);

         GFXPrimitive* prim = renderPass->allocPrim();
         prim->type = GFXTriangleList;
         prim->minIndex = 0;
         prim->startIndex = 0;
         prim->numPrimitives = mBuffers[i].buffers[b].primData.size() / 3;
         prim->startVertex = 0;
         prim->numVertices = mBuffers[i].buffers[b].vertData.size();

         GFX->drawPrimitive(*prim);
      }
   }
}

