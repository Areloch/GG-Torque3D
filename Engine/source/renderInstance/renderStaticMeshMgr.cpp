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
   sgData.init( state );

   for (U32 i = 0; i < mBuffers.size(); i++)
   {
      for (U32 b = 0; b < mBuffers[i].buffers.size(); b++)
      {
         if (mBuffers[i].buffers[b].vertData.empty())
            continue;

         MeshRenderInst *ri = renderPass->allocInst<MeshRenderInst>();

         // Set our RenderInst as a standard mesh render
         ri->type = RenderPassManager::RIT_Mesh;

         // Calculate our sorting point
         if (state)
         {
            // Calculate our sort point manually.
            const Box3F& rBox = getRenderWorldBox();
            ri->sortDistSq = rBox.getSqDistanceToPoint(state->getCameraPosition());
         }
         else
            ri->sortDistSq = 0.0f;

         // Set up our transforms
         //MatrixF objectToWorld = getRenderTransform();
         //objectToWorld.scale(getScale());

         ri->objectToWorld = renderPass->allocUniqueXform(objectToWorld);
         //ri->objectToWorld = renderPass->allocUniqueXform(MatrixF::Identity);
         ri->worldToCamera = renderPass->allocSharedXform(RenderPassManager::View);
         ri->projection = renderPass->allocSharedXform(RenderPassManager::Projection);

         // Make sure we have an up-to-date backbuffer in case
         // our Material would like to make use of it
         // NOTICE: SFXBB is removed and refraction is disabled!
         //ri->backBuffTex = GFX->getSfxBackBuffer();

         // Set our Material
         ri->matInst = state->getOverrideMaterial(mSurfaceMaterials[mBuffers[i].surfaceMaterialId].mMaterialInst ?
            mSurfaceMaterials[mBuffers[i].surfaceMaterialId].mMaterialInst : MATMGR->getWarningMatInstance());
         if (ri->matInst == NULL)
            continue; //if we still have no valid mat, skip out

         // If we need lights then set them up.
         if (ri->matInst->isForwardLit())
         {
            LightQuery query;
            query.init(getWorldSphere());
            query.getLights(ri->lights, 8);
         }

         if (ri->matInst->getMaterial()->isTranslucent())
         {
            ri->translucentSort = true;
            ri->type = RenderPassManager::RIT_Translucent;
         }

         // Set up our vertex buffer and primitive buffer
         ri->vertBuff = &mBuffers[i].buffers[b].vertexBuffer;
         ri->primBuff = &mBuffers[i].buffers[b].primitiveBuffer;

         ri->prim = renderPass->allocPrim();
         ri->prim->type = GFXTriangleList;
         ri->prim->minIndex = 0;
         ri->prim->startIndex = 0;
         ri->prim->numPrimitives = mBuffers[i].buffers[b].primData.size() / 3;
         ri->prim->startVertex = 0;
         ri->prim->numVertices = mBuffers[i].buffers[b].vertData.size();

         // We sort by the material then vertex buffer.
         ri->defaultKey = ri->matInst->getStateHint();
         ri->defaultKey2 = (uintptr_t)ri->vertBuff; // Not 64bit safe!

         // Submit our RenderInst to the RenderPassManager
         state->getRenderPass()->addInst(ri);
      }
   }

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

      if ( ( MATMGR->getPrePassEnabled() && mBasicOnly && !mat->isCustomMaterial() ) )
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
               lastAccuTex = lastAccuTex;
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

