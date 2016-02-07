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

#include "T3D/examples/ShapeAssetExample.h"

#include "math/mathIO.h"
#include "sim/netConnection.h"
#include "scene/sceneRenderState.h"
#include "console/consoleTypes.h"
#include "core/resourceManager.h"
#include "core/stream/bitStream.h"
#include "gfx/gfxTransformSaver.h"
#include "renderInstance/renderPassManager.h"
#include "lighting/lightQuery.h"

#include "materials/materialManager.h"
#include "materials/baseMatInstance.h"

IMPLEMENT_CO_NETOBJECT_V1(ShapeAssetExample);

ConsoleDocClass( ShapeAssetExample, 
   "@brief An example scene object which renders a DTS.\n\n"
   "This class implements a basic SceneObject that can exist in the world at a "
   "3D position and render itself. There are several valid ways to render an "
   "object in Torque. This class makes use of the 'TS' (three space) shape "
   "system. TS manages loading the various mesh formats supported by Torque as "
   "well was rendering those meshes (including LOD and animation...though this "
   "example doesn't include any animation over time).\n\n"
   "See the C++ code for implementation details.\n\n"
   "@ingroup Examples\n" );

//-----------------------------------------------------------------------------
// Object setup and teardown
//-----------------------------------------------------------------------------
ShapeAssetExample::ShapeAssetExample()
{
   // Flag this object so that it will always
   // be sent across the network to clients
   mNetFlags.set( Ghostable | ScopeAlways );

   // Set it as a "static" object.
   mTypeMask |= StaticObjectType | StaticShapeObjectType;

   // Make sure to initialize our TSShapeInstance to NULL
   //mShapeInstance = NULL;

   mMeshAssetId = StringTable->EmptyString();
   mAnimAssetId = StringTable->EmptyString();

   mAnimationThreads.clear();
}

ShapeAssetExample::~ShapeAssetExample()
{
}

//-----------------------------------------------------------------------------
// Object Editing
//-----------------------------------------------------------------------------
void ShapeAssetExample::initPersistFields()
{
   addGroup( "Rendering" );
   addProtectedField("MeshAsset", TypeAssetId, Offset(mMeshAssetId, ShapeAssetExample), &_setMesh, &defaultProtectedGetFn,
      "The asset Id used for the mesh.");
   //addProtectedField("materialAsset", TypeAssetId, Offset(mMaterialAssetId, ShapeAssetExample), &_setMaterial, &defaultProtectedGetFn,
   //   "The asset Id used for the mesh.");
   addProtectedField("animationAsset", TypeAssetId, Offset(mAnimAssetId, ShapeAssetExample), &_setAnimation, &defaultProtectedGetFn,
      "The asset Id used for the mesh.");
   endGroup( "Rendering" );

   // SceneObject already handles exposing the transform
   Parent::initPersistFields();
}

bool ShapeAssetExample::_setMesh(void *object, const char *index, const char *data)
{
   ShapeAssetExample *shapeExample = static_cast<ShapeAssetExample*>(object);

   // Sanity!
   AssertFatal(data != NULL, "Cannot use a NULL asset Id.");

   return shapeExample->setMeshAsset(data);
}

bool ShapeAssetExample::setMeshAsset(const char* assetName)
{
   // Fetch the asset Id.
   mMeshAssetId = StringTable->insert(assetName);
   mMeshAsset.setAssetId(mMeshAssetId);

   if (mMeshAsset.isNull())
   {
      Con::errorf("[MeshComponent] Failed to load mesh asset.");
      return false;
   }

   mMeshAsset = mMeshAssetId;
   createShape(); //make sure we force the update to resize the owner bounds
   if (isServerObject())
      setMaskBits(ShapeMask);

   return true;
}

//
bool ShapeAssetExample::_setAnimation(void *object, const char *index, const char *data)
{
   ShapeAssetExample *shapeExample = static_cast<ShapeAssetExample*>(object);

   // Sanity!
   AssertFatal(data != NULL, "Cannot use a NULL asset Id.");

   return shapeExample->setAnimationAsset(data);
}

bool ShapeAssetExample::setAnimationAsset(const char* assetName)
{
   // Fetch the asset Id.
   mAnimAssetId = StringTable->insert(assetName);
   mAnimAsset.setAssetId(mAnimAssetId);

   if (mAnimAsset.isNull())
   {
      Con::errorf("[MeshComponent] Failed to load mesh asset.");
      return false;
   }

   mAnimAsset = mAnimAssetId;
   createShape(); //make sure we force the update to resize the owner bounds
   if (isServerObject())
      setMaskBits(ShapeMask);

   return true;
}
//

void ShapeAssetExample::inspectPostApply()
{
   Parent::inspectPostApply();

   // Flag the network mask to send the updates
   // to the client object
   setMaskBits( UpdateMask );
}

bool ShapeAssetExample::onAdd()
{
   if ( !Parent::onAdd() )
      return false;

   setProcessTick(true);

   // Set up a 1x1x1 bounding box
   mObjBox.set( Point3F( -0.5f, -0.5f, -0.5f ),
                Point3F(  0.5f,  0.5f,  0.5f ) );

   resetWorldBox();

   // Add this object to the scene
   addToScene();

   // Setup the shape.
   createShape();

   if (isServerObject())
      setMaskBits(ShapeMask);

   return true;
}

void ShapeAssetExample::onRemove()
{
   // Remove this object from the scene
   removeFromScene();

   // Remove our TSShapeInstance
   //if ( mShapeInstance )
   //   SAFE_DELETE( mShapeInstance );

   Parent::onRemove();
}

void ShapeAssetExample::setTransform(const MatrixF & mat)
{
   // Let SceneObject handle all of the matrix manipulation
   Parent::setTransform( mat );

   // Dirty our network mask so that the new transform gets
   // transmitted to the client object
   setMaskBits( TransformMask );
}

U32 ShapeAssetExample::packUpdate( NetConnection *conn, U32 mask, BitStream *stream )
{
   // Allow the Parent to get a crack at writing its info
   U32 retMask = Parent::packUpdate( conn, mask, stream );

   // Write our transform information
   if ( stream->writeFlag( mask & TransformMask ) )
   {
      mathWrite(*stream, getTransform());
      mathWrite(*stream, getScale());
   }

   // Write out any of the updated editable properties
   if (stream->writeFlag(mask & ShapeMask))
   {
      String meshId = mMeshAssetId;
      stream->write(meshId);

      String animId = mAnimAssetId;
      stream->write(animId);

      // Allow the server object a chance to handle a new shape
      //createShape();
   }

   return retMask;
}

void ShapeAssetExample::unpackUpdate(NetConnection *conn, BitStream *stream)
{
   // Let the Parent read any info it sent
   Parent::unpackUpdate(conn, stream);

   if ( stream->readFlag() )  // TransformMask
   {
      mathRead(*stream, &mObjToWorld);
      mathRead(*stream, &mObjScale);

      setTransform( mObjToWorld );
   }

   if ( stream->readFlag() )  // ShapeMask
   {
      String mesh;
      stream->read(&mesh);

      mMeshAssetId = StringTable->insert(mesh);

      //if (isProperlyAdded())
         setMeshAsset(mMeshAssetId);

      String anim;
      stream->read(&anim);

      mAnimAssetId = StringTable->insert(anim);

      //if (isProperlyAdded())
         setAnimationAsset(mAnimAssetId);

      //if ( isProperlyAdded() )
         createShape();
   }
}

void ShapeAssetExample::processTick(const Move* move)
{
   Parent::processTick(move);
}

void ShapeAssetExample::advanceTime(F32 delta)
{
   Parent::advanceTime(delta);

   if (mAnimAsset && mMeshAsset)
   {
      for (U32 i = 0; i < mAnimationThreads.size(); i++)
      {
         mAnimationThreads[i].playTime += delta * mAnimationThreads[i].playSpeed;

         if (mAnimationThreads[i].playTime > mAnimAsset->getSequenceDuration(0))
            mAnimationThreads[i].playTime = mAnimationThreads[i].playTime - mAnimAsset->getSequenceDuration(0);

         Vector<MatrixF> nodeTransforms = mAnimAsset->getNodeTransforms(0, mAnimationThreads[i].playTime);

         mNodeTransforms = nodeTransforms;
      }
   }
   //animation updaaaaaates
}

//
//In the event our mesh is marked as dirty, we rebuild the mesh's buffer
//This happens when animations update the mesh shape, or a change in LOD or a change in which meshes are visible
void ShapeAssetExample::rebuildMeshBuffer()
{
   if (!mMeshAsset || mMeshAsset->getDetailLevelCount() == 0)
      return;

   //rebuild!
   /*ShapeAsset::DetailLevel* detail = mMeshAsset->getDetailLevel(1000);

   //clear out our old buffer selection
   mBufferList.clear();

   U32 vertCount = 0;
   U32 primCount = 0;

   //Iterate through the submeshes so we can accrue their geometry data
   U32 currentBuffer;
   U32 BUFFER_SIZE = 65000;

   //Temporarily store the vert and faces so we can collect them into one-per-material buffers
   Vector<ShapeAsset::SubMesh::Vert> tempVerts;
   Vector<ShapeAsset::SubMesh::Face> tempFaces;

   for (U32 i = 0; i < detail->mSubMeshes.size(); ++i)
   {
      ShapeAsset::SubMesh& subMesh = detail->mSubMeshes[i];

      //double-check that the submesh isn't hidden by us
      
      vertCount += subMesh.verts.size();
      primCount += subMesh.faces.size();

      currentBuffer = findBufferSetByMaterial(subMesh.materialIndex);
      if (currentBuffer == -1)
      {
         BufferSet newSet;
         newSet.matId = subMesh.materialIndex;

         mBufferList.push_back(newSet);

         currentBuffer = mBufferList.size() - 1;
      }

      //Now that we know for sure what buffer to write into
   }
   
   //now build the buffers
   mVertexBuffer.set(GFX, vertCount, GFXBufferTypeStatic);
   VertexType *pVert = mVertexBuffer.lock();

   for (U32 s = 0; s < detail->mSubMeshes.size(); ++s)
   {
      ShapeAsset::SubMesh& subMesh = detail->mSubMeshes[s];

      for (U32 v = 0; v < subMesh.verts.size(); ++v)
      {
         pVert->normal = subMesh.verts[v].normal;
         pVert->B = subMesh.verts[v].bitangent;
         pVert->T = subMesh.verts[v].tangent;
         pVert->point = subMesh.verts[v].position;
         pVert->texCoord = subMesh.verts[v].texCoord;
         pVert->texCoord2 = subMesh.verts[v].texCoord2;
      }
   }

   mVertexBuffer.unlock();

   //primitive buffers
   // Allocate PB
   mPrimitiveBuffer.set(GFX, primCount * 3, primCount, GFXBufferTypeStatic);

   U16 *pIndex;
   mPrimitiveBuffer.lock(&pIndex);

   for (U32 s = 0; s < detail->mSubMeshes.size(); ++s)
   {
      ShapeAsset::SubMesh& subMesh = detail->mSubMeshes[s];

      for (U16 f = 0; f < subMesh.faces.size(); ++f)
      {
         for (U16 i = 0; i < subMesh.faces[f].indicies.size(); i++)
         {
            *pIndex = subMesh.faces[f].indicies[i];
            pIndex++;
         }
      }
   }

   mPrimitiveBuffer.unlock();*/
}
//

//Get the relevent detail level
S32 ShapeAssetExample::setDetailFromPosAndScale(const SceneRenderState *state,
   const Point3F &pos,
   const Point3F &scale)
{
   VectorF camVector = pos - state->getDiffuseCameraPosition();
   F32 dist = getMax(camVector.len(), 0.01f);
   F32 invScale = (1.0f / getMax(getMax(scale.x, scale.y), scale.z));

   return setDetailFromDistance(state, dist * invScale);
}

S32 ShapeAssetExample::setDetailFromDistance(const SceneRenderState *state, F32 scaledDistance)
{
   /*PROFILE_SCOPE(MeshComponent_setDetailFromDistance);

   // For debugging/metrics.
   smLastScaledDistance = scaledDistance;

   // Shortcut if the distance is really close or negative.
   if (scaledDistance <= 0.0f)
   {
      mShape->mDetailLevelLookup[0].get(mCurrentDetailLevel, mCurrentIntraDetailLevel);
      return mCurrentDetailLevel;
   }

   // The pixel scale is used the linearly scale the lod
   // selection based on the viewport size.
   //
   // The original calculation from TGEA was...
   //
   // pixelScale = viewport.extent.x * 1.6f / 640.0f;
   //
   // Since we now work on the viewport height, assuming
   // 4:3 aspect ratio, we've changed the reference value
   // to 300 to be more compatible with legacy shapes.
   //
   const F32 pixelScale = state->getViewport().extent.y / 300.0f;

   // This is legacy DTS support for older "multires" based
   // meshes.  The original crossbow weapon uses this.
   //
   // If we have more than one detail level and the maxError
   // is non-negative then we do some sort of screen error 
   // metric for detail selection.
   //
   if (mShape->mUseDetailFromScreenError)
   {
      // The pixel size of 1 meter at the input distance.
      F32 pixelRadius = state->projectRadius(scaledDistance, 1.0f) * pixelScale;
      static const F32 smScreenError = 5.0f;
      return setDetailFromScreenError(smScreenError / pixelRadius);
   }

   // We're inlining SceneRenderState::projectRadius here to 
   // skip the unnessasary divide by zero protection.
   F32 pixelRadius = (mShape->radius / scaledDistance) * state->getWorldToScreenScale().y * pixelScale;
   F32 pixelSize = pixelRadius * smDetailAdjust;

   if (pixelSize < smSmallestVisiblePixelSize) {
      mCurrentDetailLevel = -1;
      return mCurrentDetailLevel;
   }

   if (pixelSize > smSmallestVisiblePixelSize &&
      pixelSize <= mShape->mSmallestVisibleSize)
      pixelSize = mShape->mSmallestVisibleSize + 0.01f;

   // For debugging/metrics.
   smLastPixelSize = pixelSize;

   // Clamp it to an acceptable range for the lookup table.
   U32 index = (U32)mClampF(pixelSize, 0, mShape->mDetailLevelLookup.size() - 1);

   // Check the lookup table for the detail and intra detail levels.
   mShape->mDetailLevelLookup[index].get(mCurrentDetailLevel, mCurrentIntraDetailLevel);

   // Restrict the chosen detail level by cutoff value.
   if (smNumSkipRenderDetails > 0 && mCurrentDetailLevel >= 0)
   {
      S32 cutoff = getMin(smNumSkipRenderDetails, mShape->mSmallestVisibleDL);
      if (mCurrentDetailLevel < cutoff)
      {
         mCurrentDetailLevel = cutoff;
         mCurrentIntraDetailLevel = 1.0f;
      }
   }

   return mCurrentDetailLevel;*/
   return 0;
}

S32 ShapeAssetExample::setDetailFromScreenError(F32 errorTolerance)
{
   /*PROFILE_SCOPE(MeshComponent_setDetailFromScreenError);

   // For debugging/metrics.
   smLastScreenErrorTolerance = errorTolerance;

   // note:  we use 10 time the average error as the metric...this is
   // more robust than the maxError...the factor of 10 is to put average error
   // on about the same scale as maxError.  The errorTOL is how much
   // error we are able to tolerate before going to a more detailed version of the
   // shape.  We look for a pair of details with errors bounding our errorTOL,
   // and then we select an interpolation parameter to tween betwen them.  Ok, so
   // this isn't exactly an error tolerance.  A tween value of 0 is the lower poly
   // model (higher detail number) and a value of 1 is the higher poly model (lower
   // detail number).

   // deal with degenerate case first...
   // if smallest detail corresponds to less than half tolerable error, then don't even draw
   F32 prevErr;
   if (mShape->mSmallestVisibleDL < 0)
      prevErr = 0.0f;
   else
      prevErr = 10.0f * mShape->details[mShape->mSmallestVisibleDL].averageError * 20.0f;
   if (mShape->mSmallestVisibleDL < 0 || prevErr < errorTolerance)
   {
      // draw last detail
      mCurrentDetailLevel = mShape->mSmallestVisibleDL;
      mCurrentIntraDetailLevel = 0.0f;
      return mCurrentDetailLevel;
   }

   // this function is a little odd
   // the reason is that the detail numbers correspond to
   // when we stop using a given detail level...
   // we search the details from most error to least error
   // until we fit under the tolerance (errorTOL) and then
   // we use the next highest detail (higher error)
   for (S32 i = mShape->mSmallestVisibleDL; i >= 0; i--)
   {
      F32 err0 = 10.0f * mShape->details[i].averageError;
      if (err0 < errorTolerance)
      {
         // ok, stop here

         // intraDL = 1 corresponds to fully this detail
         // intraDL = 0 corresponds to the next lower (higher number) detail
         mCurrentDetailLevel = i;
         mCurrentIntraDetailLevel = 1.0f - (errorTolerance - err0) / (prevErr - err0);
         return mCurrentDetailLevel;
      }
      prevErr = err0;
   }

   // get here if we are drawing at DL==0
   mCurrentDetailLevel = 0;
   mCurrentIntraDetailLevel = 1.0f;
   return mCurrentDetailLevel;*/
   return 0;
}

//-----------------------------------------------------------------------------
// Object Rendering
//-----------------------------------------------------------------------------
void ShapeAssetExample::createShape()
{
   if (mMeshAsset == NULL)
      return;

   if (!isServerObject())
   {
      //alright, fetch materials
      mMaterialInstances.clear();

      for (U32 m = 0; m < mMeshAsset->getMaterialCount(); ++m)
      {
         BaseMatInstance *matInst = mMeshAsset->getMaterial(m)->createMatInstance();

         matInst->init(MATMGR->getDefaultFeatures(), getGFXVertexFormat<VertexType>());

         mMaterialInstances.push_back(matInst);

         //
         /*mMeshAsset->getMaterial(m)->getName();
         AssetPtr<MaterialAsset> mat;
         mat.setAssetId(mMeshAsset->getAssetName);

         if (!mat.isNull())
            mMeshAsset = mMeshAssetId;*/
      }

      rebuildMeshBuffer();
   }

   //set up the animations
   mAnimationThreads.clear();

   if (mAnimAsset == NULL)
      return;

   String seqName = mAnimAsset->getSequenceName(0);

   AnimThread newThread;

   newThread.sequenceName = seqName;

   mAnimationThreads.push_back(newThread);

   /*if ( mShapeFile.isEmpty() )
      return;

   // If this is the same shape then no reason to update it
   if ( mShapeInstance && mShapeFile.equal( mShape.getPath().getFullPath(), String::NoCase ) )
      return;

   // Clean up our previous shape
   if ( mShapeInstance )
      SAFE_DELETE( mShapeInstance );
   mShape = NULL;

   // Attempt to get the resource from the ResourceManager
   mShape = ResourceManager::get().load( mShapeFile );

   if ( !mShape )
   {
      Con::errorf( "ShapeAssetExample::createShape() - Unable to load shape: %s", mShapeFile.c_str() );
      return;
   }

   // Attempt to preload the Materials for this shape
   if ( isClientObject() && 
        !mShape->preloadMaterialList( mShape.getPath() ) && 
        NetConnection::filesWereDownloaded() )
   {
      mShape = NULL;
      return;
   }

   // Update the bounding box
   mObjBox = mShape->bounds;
   resetWorldBox();
   setRenderTransform(mObjToWorld);

   // Create the TSShapeInstance
   mShapeInstance = new TSShapeInstance( mShape, isClientObject() );*/
}

void ShapeAssetExample::prepRenderImage( SceneRenderState *state )
{
   // Do a little prep work if needed
   if (!mMeshAsset || mMeshAsset->getDetailLevelCount() == 0 || !state)
      return;

   // Get a handy pointer to our RenderPassmanager
   RenderPassManager *renderPass = state->getRenderPass();

   // Set up our transforms
   MatrixF objectToWorld = getRenderTransform();
   objectToWorld.scale(getScale());

   ShapeAsset::DetailLevel* detail = mMeshAsset->getDetailLevel(1000);
   if (detail == NULL)
      return;

   U32 subMesheCount = detail->mSubMeshes.size();;
   for (U32 i = 0; i < subMesheCount; ++i)
   {
      ShapeAsset::SubMesh* mesh = &detail->mSubMeshes[i];

      MeshRenderInst *ri = renderPass->allocInst<MeshRenderInst>();

      //pluck our material
      BaseMatInstance *matInst = mMaterialInstances[mesh->materialIndex];

      if (matInst == NULL)
         matInst = MATMGR->getWarningMatInstance();

      //BaseMatInstance *matInst = state->getOverrideMaterial(mMaterialInstances[mesh.materialIndex] 
      //   ? mMaterialInstances[mesh.materialIndex] : MATMGR->getWarningMatInstance());

      //BaseMatInstance *matInst = MATMGR->getWarningMatInstance();

      if (!matInst)
         return;

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

      ri->objectToWorld = renderPass->allocUniqueXform(objectToWorld);
      ri->worldToCamera = renderPass->allocSharedXform(RenderPassManager::View);
      ri->projection = renderPass->allocSharedXform(RenderPassManager::Projection);

      // If we need lights then set them up.
      if (matInst->isForwardLit())
      {
         LightQuery query;
         query.init(getWorldSphere());
         query.getLights(ri->lights, 8);
      }

      // Make sure we have an up-to-date backbuffer in case
      // our Material would like to make use of it
      // NOTICE: SFXBB is removed and refraction is disabled!
      //ri->backBuffTex = GFX->getSfxBackBuffer();

      // Set our Material
      ri->matInst = matInst;

      if (matInst->getMaterial()->isTranslucent())
      {
         ri->translucentSort = true;
         ri->type = RenderPassManager::RIT_Translucent;
      }

      // Set up our vertex buffer and primitive buffer
      ri->vertBuff = &mesh->vertexBuffer;
      ri->primBuff = &mesh->primitiveBuffer;

      ri->prim = renderPass->allocPrim();
      ri->prim->type = GFXTriangleList;
      ri->prim->minIndex = 0;
      ri->prim->startIndex = 0;
      ri->prim->numPrimitives = mesh->faces.size();
      ri->prim->startVertex = 0;
      ri->prim->numVertices = mesh->verts.size();

      // We sort by the material then vertex buffer.
      ri->defaultKey = matInst->getStateHint();
      ri->defaultKey2 = (uintptr_t)ri->vertBuff; // Not 64bit safe!

      // Submit our RenderInst to the RenderPassManager
      state->getRenderPass()->addInst(ri);
   }
}