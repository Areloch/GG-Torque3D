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

#ifndef _ShapeAssetExample_H_
#define _ShapeAssetExample_H_

#ifndef _SCENEOBJECT_H_
#include "scene/sceneObject.h"
#endif

#include "assets/assetPtr.h"
#include "T3D/assets/ShapeAsset.h"
#include "T3D/assets/MaterialAsset.h"
#include "T3D/assets/AnimationAsset.h"

//-----------------------------------------------------------------------------
// This class implements a basic SceneObject that can exist in the world at a
// 3D position and render itself. There are several valid ways to render an
// object in Torque. This class makes use of the "TS" (three space) shape
// system. TS manages loading the various mesh formats supported by Torque as
// well was rendering those meshes (including LOD and animation...though this
// example doesn't include any animation over time).
//-----------------------------------------------------------------------------

class ShapeAssetExample : public SceneObject
{
   typedef SceneObject Parent;

   // Networking masks
   // We need to implement a mask specifically to handle
   // updating our transform from the server object to its
   // client-side "ghost". We also need to implement a
   // maks for handling editor updates to our properties
   // (like material).
   enum MaskBits 
   {
      TransformMask = Parent::NextFreeMask << 0,
      UpdateMask    = Parent::NextFreeMask << 1,
      ShapeMask = Parent::NextFreeMask << 2,
      NextFreeMask  = Parent::NextFreeMask << 3
   };

   //--------------------------------------------------------------------------
   // Rendering variables
   //--------------------------------------------------------------------------
   // The name of the shape file we will use for rendering
   //String            mShapeFile;
   // The actual shape instance
   //TSShapeInstance*  mShapeInstance;
   // Store the resource so we can access the filename later
   //Resource<TSShape> mShape;

   StringTableEntry       mMeshAssetId;
   AssetPtr<ShapeAsset>   mMeshAsset;

   StringTableEntry        mMaterialAssetId;
   AssetPtr<MaterialAsset> mMaterialAsset;

   Vector<AssetPtr<MaterialAsset>> mMaterialAssets;
   Vector<BaseMatInstance*>         mMaterialInstances;

   StringTableEntry           mAnimAssetId;
   AssetPtr<AnimationAsset>   mAnimAsset;

   struct AnimThread
   {
      String sequenceName;
      F32 playTime;
      F32 playSpeed;

      AnimThread()
      {
         sequenceName = "";
         playTime = 0;
         playSpeed = 1;
      }
   };

   Vector<AnimThread> mAnimationThreads;

   Vector<MatrixF> mNodeTransforms;

   //
   //In the event of a non-animated mesh, we can just use the static buffers that are pre-defined here.
   //Once Hardware skinning is in, the rendering will use this exclusively rather than temp buffers in the utilizing class
   struct BufferSet
   {
      U32 matId;

      /*struct Buffers
      {
         U32 vertStart;
         U32 primStart;
         U32 vertCount;
         U32 primCount;

         GFXVertexBufferHandle< VertexType > vertexBuffer;
         GFXPrimitiveBufferHandle            primitiveBuffer;

         Buffers()
         {
            vertStart = 0;
            primStart = 0;
            vertCount = 0;
            primCount = 0;

            vertexBuffer = NULL;
            primitiveBuffer = NULL;
         }
      };

      Vector<Buffers> buffers;*/

      U32 vertCount;
      U32 primCount;

      GFXVertexBufferHandle< VertexType > vertexBuffer;
      GFXPrimitiveBufferHandle            primitiveBuffer;

      BufferSet()
      {
         //Buffers newBuffer;
         //buffers.push_back(newBuffer);

         vertCount = 0;
         primCount = 0;

         vertexBuffer = NULL;
         primitiveBuffer = NULL;

         matId = 0;
      }
   };
   
   Vector < BufferSet > mBufferList;

protected:
   U32 findBufferSetByMaterial(U32 matId)
   {
      for (U32 i = 0; i < mBufferList.size(); i++)
      {
         if (mBufferList[i].matId == matId)
            return i;
      }

      return -1;
   }

public:
   ShapeAssetExample();
   virtual ~ShapeAssetExample();

   // Declare this object as a ConsoleObject so that we can
   // instantiate it into the world and network it
   DECLARE_CONOBJECT(ShapeAssetExample);

   //--------------------------------------------------------------------------
   // Object Editing
   // Since there is always a server and a client object in Torque and we
   // actually edit the server object we need to implement some basic
   // networking functions
   //--------------------------------------------------------------------------
   // Set up any fields that we want to be editable (like position)
   static void initPersistFields();

   // Allows the object to update its editable settings
   // from the server object to the client
   virtual void inspectPostApply();

   // Handle when we are added to the scene and removed from the scene
   bool onAdd();
   void onRemove();

   // Override this so that we can dirty the network flag when it is called
   void setTransform( const MatrixF &mat );

   // This function handles sending the relevant data from the server
   // object to the client object
   U32 packUpdate( NetConnection *conn, U32 mask, BitStream *stream );
   // This function handles receiving relevant data from the server
   // object and applying it to the client object
   void unpackUpdate( NetConnection *conn, BitStream *stream );

   //--------------------------------------------------------------------------
   // Object Rendering
   // Torque utilizes a "batch" rendering system. This means that it builds a
   // list of objects that need to render (via RenderInst's) and then renders
   // them all in one batch. This allows it to optimized on things like
   // minimizing texture, state, and shader switching by grouping objects that
   // use the same Materials.
   //--------------------------------------------------------------------------
   // Create the geometry for rendering
   void createShape();

   // This is the function that allows this object to submit itself for rendering
   void prepRenderImage( SceneRenderState *state );

   virtual void processTick(const Move* move);
   virtual void advanceTime(F32 delta);

   static bool _setMesh(void *object, const char *index, const char *data);
   bool setMeshAsset(const char* assetName);

   static bool _setAnimation(void *object, const char *index, const char *data);
   bool setAnimationAsset(const char* assetName);

   //
   void rebuildMeshBuffer();
   S32 setDetailFromDistance(const SceneRenderState *state, F32 scaledDistance);
   S32 setDetailFromScreenError(F32 errorTolerance);
   S32 setDetailFromPosAndScale(const SceneRenderState *state, const Point3F &pos, const Point3F &scale);
   //
};

#endif // _ShapeAssetExample_H_