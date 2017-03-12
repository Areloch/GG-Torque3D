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
#include "renderInstance/renderMeshMgr.h"

#include "console/consoleTypes.h"
#include "gfx/gfxTransformSaver.h"
#include "gfx/gfxPrimitiveBuffer.h"
#include "materials/sceneData.h"
#include "materials/processedMaterial.h"
#include "materials/materialManager.h"
#include "scene/sceneRenderState.h"
#include "gfx/gfxDebugEvent.h"
#include "math/util/matrixSet.h"


IMPLEMENT_CONOBJECT(RenderMeshMgr);

ConsoleDocClass( RenderMeshMgr, 
   "@brief A render bin for mesh rendering.\n\n"
   "This is the primary render bin in Torque which does most of the "
   "work of rendering DTS shapes and arbitrary mesh geometry.  It knows "
   "how to render mesh instances using materials and supports hardware mesh "
   "instancing.\n\n"
   "@ingroup RenderBin\n" );


RenderMeshMgr::RenderMeshMgr()
: RenderBinManager(RenderPassManager::RIT_Mesh, 1.0f, 1.0f)
{
   VECTOR_SET_ASSOCIATION(mDynamicBatchElements);
   mDynamicBatchElements.reserve(2048);
}

RenderMeshMgr::RenderMeshMgr(RenderInstType riType, F32 renderOrder, F32 processAddOrder)
   : RenderBinManager(riType, renderOrder, processAddOrder)
{
}

void RenderMeshMgr::init()
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

void RenderMeshMgr::initPersistFields()
{
   Parent::initPersistFields();
}

void RenderMeshMgr::clear()
{
   Parent::clear();

   mDynamicBatchElements.clear();
   mDynamicBuffers.clear();
}

//-----------------------------------------------------------------------------
// add element
//-----------------------------------------------------------------------------
void RenderMeshMgr::addElement( RenderInst *inst )
{
   // If this instance is translucent handle it in RenderTranslucentMgr
   if (inst->translucentSort)
      return;

   AssertFatal( inst->defaultKey != 0, "RenderMeshMgr::addElement() - Got null sort key... did you forget to set it?" );

   internalAddElement(inst);
}

void RenderMeshMgr::addStaticBatchElement(SimObject* owner, OptimizedPolyList* geometry, String batchName)
{
   bool found = false;
   for (U32 i = 0; i < mStaticBatchElements.size(); ++i)
   {
      if (mStaticBatchElements[i].owner != NULL && mStaticBatchElements[i].owner->getId() == owner->getId())
      {
         found = true;
         mStaticBatchElements[i].geometry = *geometry;
         break;
      }
   }

   if (!found)
   {
      MeshBatchElement newElement;

      newElement.owner = owner;
      newElement.geometry = *geometry;

      mStaticBatchElements.push_back(newElement);
   }

   rebuildStaticBuffers();
}

void RenderMeshMgr::addDynamicBatchElement(SimObject* owner, OptimizedPolyList* geometry, String batchName)
{
   MeshBatchElement newElement;

   newElement.owner = nullptr;
   newElement.geometry = *geometry;

   mDynamicBatchElements.push_back(newElement);
}

//-----------------------------------------------------------------------------
// render
//-----------------------------------------------------------------------------
void RenderMeshMgr::render(SceneRenderState * state)
{
   PROFILE_SCOPE(RenderMeshMgr_render);

   buildDynamicBuffers();
   prepareBufferRenderInsts(state);

   // Early out if nothing to draw.
   if(!mElementList.size())
      return;

   GFXDEBUGEVENT_SCOPE( RenderMeshMgr_Render, ColorI::GREEN );

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
   sgData.init( state );

   U32 binSize = mElementList.size();

   for( U32 j=0; j<binSize; )
   {
      MeshRenderInst *ri = static_cast<MeshRenderInst*>(mElementList[j].inst);

      setupSGData( ri, sgData );
      BaseMatInstance *mat = ri->matInst;

      // If we have an override delegate then give it a 
      // chance to swap the material with another.
      if ( mMatOverrideDelegate )
      {
         mat = mMatOverrideDelegate( mat );
         if ( !mat )
         {
            j++;
            continue;
         }
      }

      if( !mat )
         mat = MATMGR->getWarningMatInstance();

      // Check if bin is disabled in advanced lighting.
      // Allow forward rendering pass on custom materials.

      if ( ( MATMGR->getDeferredEnabled() && mBasicOnly && !mat->isCustomMaterial() ) )
      {
         j++;
         continue;
      }

      U32 matListEnd = j;
      lastMiscTex = sgData.miscTex;
      U32 a;

      while( mat && mat->setupPass(state, sgData ) )
      {
         for( a=j; a<binSize; a++ )
         {
            MeshRenderInst *passRI = static_cast<MeshRenderInst*>(mElementList[a].inst);

            // Check to see if we need to break this batch.
            if (  newPassNeeded( ri, passRI ) ||
                  lastMiscTex != passRI->miscTex )
            {
               lastLM = NULL;
               break;
            }

            matrixSet.setWorld(*passRI->objectToWorld);
            matrixSet.setView(*passRI->worldToCamera);
            matrixSet.setProjection(*passRI->projection);
            mat->setTransforms(matrixSet, state);

            // Setup HW skinning transforms if applicable
            if (mat->usesHardwareSkinning())
            {
               mat->setNodeTransforms(passRI->mNodeTransforms, passRI->mNodeTransformCount);
            }

            setupSGData( passRI, sgData );
            mat->setSceneInfo( state, sgData );

            // If we're instanced then don't render yet.
            if ( mat->isInstanced() )
            {
               // Let the material increment the instance buffer, but
               // break the batch if it runs out of room for more.
               if ( !mat->stepInstance() )
               {
                  a++;
                  break;
               }

               continue;
            }

            // TODO: This could proably be done in a cleaner way.
            //
            // This section of code is dangerous, it overwrites the
            // lightmap values in sgData.  This could be a problem when multiple
            // render instances use the same multi-pass material.  When
            // the first pass is done, setupPass() is called again on
            // the material, but the lightmap data has been changed in
            // sgData to the lightmaps in the last renderInstance rendered.

            // This section sets the lightmap data for the current batch.
            // For the first iteration, it sets the same lightmap data,
            // however the redundancy will be caught by GFXDevice and not
            // actually sent to the card.  This is done for simplicity given
            // the possible condition mentioned above.  Better to set always
            // than to get bogged down into special case detection.
            //-------------------------------------
            bool dirty = false;

            // set the lightmaps if different
            if( passRI->lightmap && passRI->lightmap != lastLM )
            {
               sgData.lightmap = passRI->lightmap;
               lastLM = passRI->lightmap;
               dirty = true;
            }

            // set the cubemap if different.
            if ( passRI->cubemap != lastCubemap )
            {
               sgData.cubemap = passRI->cubemap;
               lastCubemap = passRI->cubemap;
               dirty = true;
            }

            if ( passRI->reflectTex != lastReflectTex )
            {
               sgData.reflectTex = passRI->reflectTex;
               lastReflectTex = passRI->reflectTex;
               dirty = true;
            }

            // Update accumulation texture if it changed.
            // Note: accumulation texture can be NULL, and must be updated.
            if ( passRI->accuTex != lastAccuTex )
            {
               sgData.accuTex = passRI->accuTex;
               lastAccuTex = passRI->accuTex;
               dirty = true;
            }

            if ( dirty )
               mat->setTextureStages( state, sgData );

            // Setup the vertex and index buffers.
            mat->setBuffers( passRI->vertBuff, passRI->primBuff );

            // Render this sucker.
            if ( passRI->prim )
               GFX->drawPrimitive( *passRI->prim );
            else
               GFX->drawPrimitive( passRI->primBuffIndex );
         }

         // Draw the instanced batch.
         if ( mat->isInstanced() )
         {
            // Sets the buffers including the instancing stream.
            mat->setBuffers( ri->vertBuff, ri->primBuff );

            // Render the instanced stream.
            if ( ri->prim )
               GFX->drawPrimitive( *ri->prim );
            else
               GFX->drawPrimitive( ri->primBuffIndex );
         }

         matListEnd = a;
      }

      // force increment if none happened, otherwise go to end of batch
      j = ( j == matListEnd ) ? j+1 : matListEnd;
   }
}

void RenderMeshMgr::rebuildStaticBuffers()
{
   U32 BUFFER_SIZE = 65000;
   Vector<U32> tempIndices;
   tempIndices.reserve(4);

   Box3F newBounds = Box3F::Zero;

   mStaticBuffers.clear();

   for (U32 i = 0; i < mStaticBatchElements.size(); ++i)
   {
      for (U32 j = 0; j < mStaticBatchElements[i].geometry.mPolyList.size(); j++)
      {
         const OptimizedPolyList::Poly& poly = mStaticBatchElements[i].geometry.mPolyList[j];

         if (poly.vertexCount < 3)
            continue;

         tempIndices.setSize(poly.vertexCount);
         dMemset(tempIndices.address(), 0, poly.vertexCount);

         if (poly.type == OptimizedPolyList::TriangleStrip ||
            poly.type == OptimizedPolyList::TriangleFan)
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

         if (bufferId == -1)
         {
            //add a new buffer set if we didn't get a match!
            BufferSet newSet;
            newSet.surfaceMaterialId = poly.material;

            mStaticBuffers.push_back(newSet);

            bufferId = mStaticBuffers.size() - 1;
         }

         //see if this would push us over our buffer size limit, if it is, make a new buffer for this set
         if (mStaticBuffers[bufferId].buffers.last().vertData.size() + 3 > BUFFER_SIZE
            || mStaticBuffers[bufferId].buffers.last().primData.size() + 1 > BUFFER_SIZE)
         {
            //yep, we'll overstep with this, so spool up a new buffer in this set
            BufferSet::Buffers newBuffer = BufferSet::Buffers();
            mStaticBuffers[bufferId].buffers.push_back(newBuffer);
         }

         const U32& firstIdx = mStaticBatchElements[i].geometry.mIndexList[poly.vertexStart];
         const OptimizedPolyList::VertIndex& firstVertIdx = mStaticBatchElements[i].geometry.mVertexList[firstIdx];

         Vector<Point3F> geomPoints = mStaticBatchElements[i].geometry.mPoints;
         Vector<Point3F> geomNormals = mStaticBatchElements[i].geometry.mNormals;
         Vector<Point2F> geoUVs = mStaticBatchElements[i].geometry.mUV0s;

         for (U32 k = 1; k < poly.vertexCount - 1; k++)
         {
            const U32& secondIdx = mStaticBatchElements[i].geometry.mIndexList[poly.vertexStart + tempIndices[k]];
            const U32& thirdIdx = mStaticBatchElements[i].geometry.mIndexList[poly.vertexStart + tempIndices[k + 1]];

            const OptimizedPolyList::VertIndex& secondVertIdx = mStaticBatchElements[i].geometry.mVertexList[secondIdx];
            const OptimizedPolyList::VertIndex& thirdVertIdx = mStaticBatchElements[i].geometry.mVertexList[thirdIdx];

            Point3F points[3];
            points[0] = mStaticBatchElements[i].geometry.mPoints[firstVertIdx.vertIdx];
            points[1] = mStaticBatchElements[i].geometry.mPoints[secondVertIdx.vertIdx];
            points[2] = mStaticBatchElements[i].geometry.mPoints[thirdVertIdx.vertIdx];

            Point3F normals[3];
            normals[0] = mStaticBatchElements[i].geometry.mNormals[firstVertIdx.normalIdx];
            normals[1] = mStaticBatchElements[i].geometry.mNormals[secondVertIdx.normalIdx];
            normals[2] = mStaticBatchElements[i].geometry.mNormals[thirdVertIdx.normalIdx];

            Point3F tangents[3];
            tangents[0] = mCross(points[1] - points[0], normals[0]);
            tangents[1] = mCross(points[2] - points[1], normals[1]);
            tangents[2] = mCross(points[0] - points[2], normals[2]);

            Point2F uvs[3];
            uvs[0] = mStaticBatchElements[i].geometry.mUV0s[firstVertIdx.uv0Idx];
            uvs[1] = mStaticBatchElements[i].geometry.mUV0s[secondVertIdx.uv0Idx];
            uvs[2] = mStaticBatchElements[i].geometry.mUV0s[thirdVertIdx.uv0Idx];

            mStaticBuffers[bufferId].vertCount += 3;
            mStaticBuffers[bufferId].primCount += 1;

            for (U32 v = 0; v < 3; ++v)
            {
               //Build the vert and store it to the buffers!
               GFXVertexPNTT bufVert;
               bufVert.point = points[v];
               bufVert.normal = normals[v];
               bufVert.tangent = tangents[v];
               bufVert.texCoord = uvs[v];

               newBounds.extend(points[v]);

               mStaticBuffers[bufferId].buffers.last().vertData.push_back(bufVert);

               U32 vertPrimId = mStaticBuffers[bufferId].buffers.last().vertData.size() - 1;
               mStaticBuffers[bufferId].buffers.last().primData.push_back(vertPrimId);

               mStaticBuffers[bufferId].center += points[v];
            }
         }
      }
   }

   //Now, iterate through the organized data and turn them into renderable buffers
   for (U32 i = 0; i < mStaticBuffers.size(); i++)
   {
      for (U32 b = 0; b < mStaticBuffers[i].buffers.size(); b++)
      {
         BufferSet::Buffers& buffers = mStaticBuffers[i].buffers[b];

         //if there's no data to be had in this buffer, just skip it
         if (buffers.vertData.empty())
            continue;

         buffers.vertexBuffer.set(GFX, buffers.vertData.size(), GFXBufferTypeStatic);
         GFXVertexPNTT *pVert = buffers.vertexBuffer.lock();

         for (U32 v = 0; v < buffers.vertData.size(); v++)
         {
            pVert->normal = buffers.vertData[v].normal;
            pVert->tangent = buffers.vertData[v].tangent;
            //pVert->color = buffers.vertData[v].color;
            pVert->point = buffers.vertData[v].point;
            pVert->texCoord = buffers.vertData[v].texCoord;

            pVert++;
         }

         buffers.vertexBuffer.unlock();

         // Allocate PB
         buffers.primitiveBuffer.set(GFX, buffers.primData.size(), buffers.primData.size() / 3, GFXBufferTypeStatic);

         U16 *pIndex;
         buffers.primitiveBuffer.lock(&pIndex);

         for (U16 i = 0; i < buffers.primData.size(); i++)
         {
            *pIndex = i;
            pIndex++;
         }

         buffers.primitiveBuffer.unlock();
      }

      mStaticBuffers[i].center /= mStaticBuffers[i].vertCount;
   }

   //mObjBox = newBounds;
   //resetWorldBox();
}

void RenderMeshMgr::buildDynamicBuffers()
{
   U32 BUFFER_SIZE = 65000;
   Vector<U32> tempIndices;
   tempIndices.reserve(4);

   Box3F newBounds = Box3F::Zero;

   mDynamicBuffers.clear();

   for (U32 i = 0; i < mDynamicBatchElements.size(); ++i)
   {
      for (U32 j = 0; j < mDynamicBatchElements[i].geometry.mPolyList.size(); j++)
      {
         const OptimizedPolyList::Poly& poly = mDynamicBatchElements[i].geometry.mPolyList[j];

         if (poly.vertexCount < 3)
            continue;

         tempIndices.setSize(poly.vertexCount);
         dMemset(tempIndices.address(), 0, poly.vertexCount);

         if (poly.type == OptimizedPolyList::TriangleStrip ||
            poly.type == OptimizedPolyList::TriangleFan)
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

         if (bufferId == -1)
         {
            //add a new buffer set if we didn't get a match!
            BufferSet newSet;
            newSet.surfaceMaterialId = poly.material;

            mDynamicBuffers.push_back(newSet);

            bufferId = mDynamicBuffers.size() - 1;
         }

         //see if this would push us over our buffer size limit, if it is, make a new buffer for this set
         if (mDynamicBuffers[bufferId].buffers.last().vertData.size() + 3 > BUFFER_SIZE
            || mDynamicBuffers[bufferId].buffers.last().primData.size() + 1 > BUFFER_SIZE)
         {
            //yep, we'll overstep with this, so spool up a new buffer in this set
            BufferSet::Buffers newBuffer = BufferSet::Buffers();
            mDynamicBuffers[bufferId].buffers.push_back(newBuffer);
         }

         const U32& firstIdx = mDynamicBatchElements[i].geometry.mIndexList[poly.vertexStart];
         const OptimizedPolyList::VertIndex& firstVertIdx = mDynamicBatchElements[i].geometry.mVertexList[firstIdx];

         Vector<Point3F> geomPoints = mDynamicBatchElements[i].geometry.mPoints;
         Vector<Point3F> geomNormals = mDynamicBatchElements[i].geometry.mNormals;
         Vector<Point2F> geoUVs = mDynamicBatchElements[i].geometry.mUV0s;

         for (U32 k = 1; k < poly.vertexCount - 1; k++)
         {
            const U32& secondIdx = mDynamicBatchElements[i].geometry.mIndexList[poly.vertexStart + tempIndices[k]];
            const U32& thirdIdx = mDynamicBatchElements[i].geometry.mIndexList[poly.vertexStart + tempIndices[k + 1]];

            const OptimizedPolyList::VertIndex& secondVertIdx = mDynamicBatchElements[i].geometry.mVertexList[secondIdx];
            const OptimizedPolyList::VertIndex& thirdVertIdx = mDynamicBatchElements[i].geometry.mVertexList[thirdIdx];

            Point3F points[3];
            points[0] = mDynamicBatchElements[i].geometry.mPoints[firstVertIdx.vertIdx];
            points[1] = mDynamicBatchElements[i].geometry.mPoints[secondVertIdx.vertIdx];
            points[2] = mDynamicBatchElements[i].geometry.mPoints[thirdVertIdx.vertIdx];

            Point3F normals[3];
            normals[0] = mDynamicBatchElements[i].geometry.mNormals[firstVertIdx.normalIdx];
            normals[1] = mDynamicBatchElements[i].geometry.mNormals[secondVertIdx.normalIdx];
            normals[2] = mDynamicBatchElements[i].geometry.mNormals[thirdVertIdx.normalIdx];

            Point3F tangents[3];
            tangents[0] = mCross(points[1] - points[0], normals[0]);
            tangents[1] = mCross(points[2] - points[1], normals[1]);
            tangents[2] = mCross(points[0] - points[2], normals[2]);

            Point2F uvs[3];
            uvs[0] = mDynamicBatchElements[i].geometry.mUV0s[firstVertIdx.uv0Idx];
            uvs[1] = mDynamicBatchElements[i].geometry.mUV0s[secondVertIdx.uv0Idx];
            uvs[2] = mDynamicBatchElements[i].geometry.mUV0s[thirdVertIdx.uv0Idx];

            mDynamicBuffers[bufferId].vertCount += 3;
            mDynamicBuffers[bufferId].primCount += 1;

            for (U32 v = 0; v < 3; ++v)
            {
               //Build the vert and store it to the buffers!
               GFXVertexPNTT bufVert;
               bufVert.point = points[v];
               bufVert.normal = normals[v];
               bufVert.tangent = tangents[v];
               bufVert.texCoord = uvs[v];

               newBounds.extend(points[v]);

               mDynamicBuffers[bufferId].buffers.last().vertData.push_back(bufVert);

               U32 vertPrimId = mDynamicBuffers[bufferId].buffers.last().vertData.size() - 1;
               mDynamicBuffers[bufferId].buffers.last().primData.push_back(vertPrimId);

               mDynamicBuffers[bufferId].center += points[v];
            }
         }
      }
   }

   //Now, iterate through the organized data and turn them into renderable buffers
   for (U32 i = 0; i < mDynamicBuffers.size(); i++)
   {
      for (U32 b = 0; b < mDynamicBuffers[i].buffers.size(); b++)
      {
         BufferSet::Buffers& buffers = mDynamicBuffers[i].buffers[b];

         //if there's no data to be had in this buffer, just skip it
         if (buffers.vertData.empty())
            continue;

         buffers.vertexBuffer.set(GFX, buffers.vertData.size(), GFXBufferTypeStatic);
         GFXVertexPNTT *pVert = buffers.vertexBuffer.lock();

         for (U32 v = 0; v < buffers.vertData.size(); v++)
         {
            pVert->normal = buffers.vertData[v].normal;
            pVert->tangent = buffers.vertData[v].tangent;
            //pVert->color = buffers.vertData[v].color;
            pVert->point = buffers.vertData[v].point;
            pVert->texCoord = buffers.vertData[v].texCoord;

            pVert++;
         }

         buffers.vertexBuffer.unlock();

         // Allocate PB
         buffers.primitiveBuffer.set(GFX, buffers.primData.size(), buffers.primData.size() / 3, GFXBufferTypeStatic);

         U16 *pIndex;
         buffers.primitiveBuffer.lock(&pIndex);

         for (U16 i = 0; i < buffers.primData.size(); i++)
         {
            *pIndex = i;
            pIndex++;
         }

         buffers.primitiveBuffer.unlock();
      }

      mDynamicBuffers[i].center /= mDynamicBuffers[i].vertCount;
   }
}

void RenderMeshMgr::prepareBufferRenderInsts(SceneRenderState *state)
{
   BaseMatInstance *matInst = MATMGR->getWarningMatInstance();

   //First, do our static buffers' render instances
   // Get a handy pointer to our RenderPassmanager
   RenderPassManager *renderPass = state->getRenderPass();

   for (U32 i = 0; i < mStaticBuffers.size(); i++)
   {
      for (U32 b = 0; b < mStaticBuffers[i].buffers.size(); b++)
      {
         if (mStaticBuffers[i].buffers[b].vertData.empty())
            continue;

         MeshRenderInst *ri = renderPass->allocInst<MeshRenderInst>();

         // Set our RenderInst as a standard mesh render
         ri->type = RenderPassManager::RIT_Mesh;

         //If our material has transparency set on this will redirect it to proper render bin
         if (matInst->getMaterial()->isTranslucent())
         {
            ri->type = RenderPassManager::RIT_Translucent;
            ri->translucentSort = true;
         }

         // Calculate our sorting point
         if (state)
         {
            // Calculate our sort point manually.
            const Box3F& rBox = Box3F(500);// getRenderWorldBox();
            ri->sortDistSq = rBox.getSqDistanceToPoint(state->getCameraPosition());
         }
         else
            ri->sortDistSq = 0.0f;

         // Set up our transforms
         MatrixF objectToWorld = MatrixF::Identity;
         //objectToWorld.scale(getScale());

         ri->objectToWorld = renderPass->allocUniqueXform(objectToWorld);
         ri->worldToCamera = renderPass->allocSharedXform(RenderPassManager::View);
         ri->projection = renderPass->allocSharedXform(RenderPassManager::Projection);

         // If our material needs lights then fill the RIs 
         // light vector with the best lights.
         if (matInst->isForwardLit())
         {
            LightQuery query;
            query.init(Box3F(500)/*getWorldSphere()*/);
            query.getLights(ri->lights, 8);
         }

         // Make sure we have an up-to-date backbuffer in case
         // our Material would like to make use of it
         // NOTICE: SFXBB is removed and refraction is disabled!
         //ri->backBuffTex = GFX->getSfxBackBuffer();

         // Set our Material
         ri->matInst = matInst;

         // Set up our vertex buffer and primitive buffer
         ri->vertBuff = &mStaticBuffers[i].buffers[b].vertexBuffer;
         ri->primBuff = &mStaticBuffers[i].buffers[b].primitiveBuffer;

         ri->prim = renderPass->allocPrim();
         ri->prim->type = GFXTriangleList;
         ri->prim->minIndex = 0;
         ri->prim->startIndex = 0;
         ri->prim->numPrimitives = mStaticBuffers[i].buffers[b].primData.size();
         ri->prim->startVertex = 0;
         ri->prim->numVertices = mStaticBuffers[i].buffers[b].vertData.size();

         // We sort by the material then vertex buffer
         ri->defaultKey = matInst->getStateHint();
         ri->defaultKey2 = (uintptr_t)ri->vertBuff; // Not 64bit safe!

         this->addElement(ri);
      }
   }
}