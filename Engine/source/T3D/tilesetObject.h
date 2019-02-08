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

#ifndef TILESETOBJECT_H
#define TILESETOBJECT_H

#ifndef _SCENEOBJECT_H_
#include "scene/sceneObject.h"
#endif
#ifndef _GFXSTATEBLOCK_H_
#include "gfx/gfxStateBlock.h"
#endif
#ifndef _GFXVERTEXBUFFER_H_
#include "gfx/gfxVertexBuffer.h"
#endif
#ifndef _GFXPRIMITIVEBUFFER_H_
#include "gfx/gfxPrimitiveBuffer.h"
#endif

class BaseMatInstance;
class TilesetObjectEditorTool;

//-----------------------------------------------------------------------------
// This class implements a basic SceneObject that can exist in the world at a
// 3D position and render itself. Note that TilesetObject handles its own
// rendering by submitting itself as an ObjectRenderInst (see
// renderInstance\renderPassmanager.h) along with a delegate for its render()
// function. However, the preffered rendering method in the engine is to submit
// a MeshRenderInst along with a Material, vertex buffer, primitive buffer, and
// transform and allow the RenderMeshMgr handle the actual rendering. You can
// see this implemented in RenderMeshExample.
//-----------------------------------------------------------------------------
struct TileElement
{
   enum ElementType
   {
      Floor = 0,
      Wall,
      Door,
      Window,
      Ceiling,
      FullTile
   };

   SceneObject* elementObject;

   ElementType type;

   bool selected;

   TileElement() : 
      elementObject(nullptr), 
      type(ElementType::Floor), 
      selected(false)
   {
   }
};

struct Tile
{
   Point2F levelCoords;

   Point2F positionNudge;

   Vector<TileElement> elements;

   Tile() : levelCoords(Point2F::Zero), positionNudge(Point2F::Zero) {}
};

struct Level
{
   Vector<Tile> tiles;

   F32 floorHeight;

   F32 tileHeight;

   F32 tileWidth;

   Level() : floorHeight(0), tileHeight(2), tileWidth(1) {}
};

class TilesetObject : public SceneObject
{
   typedef SceneObject Parent;
   friend class TilesetObjectEditorTool;

   // Networking masks
   // We need to implement at least one of these to allow
   // the client version of the object to receive updates
   // from the server version (like if it has been moved
   // or edited)
   enum MaskBits 
   {
      TransformMask = Parent::NextFreeMask << 0,
      NextFreeMask  = Parent::NextFreeMask << 1
   };

   Vector<Level> mLevels;

public:
   TilesetObject();
   virtual ~TilesetObject();

   // Declare this object as a ConsoleObject so that we can
   // instantiate it into the world and network it
   DECLARE_CONOBJECT(TilesetObject);

   //--------------------------------------------------------------------------
   // Object Editing
   // Since there is always a server and a client object in Torque and we
   // actually edit the server object we need to implement some basic
   // networking functions
   //--------------------------------------------------------------------------
   // Set up any fields that we want to be editable (like position)
   static void initPersistFields();

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

   U32 getLevelsCount()
   {
      return mLevels.size();
   }

   Level* getLevel(U32 index)
   {
      if (index < mLevels.size())
         return &mLevels[index];

      return nullptr;
   }
};

#endif // _TilesetObject_H_