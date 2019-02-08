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

#ifndef TILESET_OBJECT_EDITOR_TOOL
#define TILESET_OBJECT_EDITOR_TOOL

#ifndef _EDITOR_TOOL_
#include "gui/worldEditor/tools/editorTool.h"
#endif

#ifndef TILESETOBJECT_H
#include "T3D/tilesetObject.h"
#endif


class TilePlaceholderMesh : public SceneObject
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
      UpdateMask = Parent::NextFreeMask << 1,
      NextFreeMask = Parent::NextFreeMask << 2
   };

public:
   //--------------------------------------------------------------------------
   // Rendering variables
   //--------------------------------------------------------------------------
   // The name of the Material we will use for rendering
   String            mMaterialName;

private:
   // The actual Material instance
   BaseMatInstance*  mMaterialInst;

   // Define our vertex format here so we don't have to
   // change it in multiple spots later
   typedef GFXVertexPNT VertexType;

   // The GFX vertex and primitive buffers
   GFXVertexBufferHandle< VertexType > mVertexBuffer;
   GFXPrimitiveBufferHandle            mPrimitiveBuffer;

public:
   TilePlaceholderMesh();
   virtual ~TilePlaceholderMesh();

   // Declare this object as a ConsoleObject so that we can
   // instantiate it into the world and network it
   DECLARE_CONOBJECT(TilePlaceholderMesh);

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
   void setTransform(const MatrixF &mat);

   // This function handles sending the relevant data from the server
   // object to the client object
   U32 packUpdate(NetConnection *conn, U32 mask, BitStream *stream);
   // This function handles receiving relevant data from the server
   // object and applying it to the client object
   void unpackUpdate(NetConnection *conn, BitStream *stream);

   //--------------------------------------------------------------------------
   // Object Rendering
   // Torque utilizes a "batch" rendering system. This means that it builds a
   // list of objects that need to render (via RenderInst's) and then renders
   // them all in one batch. This allows it to optimized on things like
   // minimizing texture, state, and shader switching by grouping objects that
   // use the same Materials.
   //--------------------------------------------------------------------------
   // Create the geometry for rendering
   void createGeometry();

   // Get the Material instance
   void updateMaterial();

   // This is the function that allows this object to submit itself for rendering
   void prepRenderImage(SceneRenderState *state);
};

class TilesetObjectEditorTool : public EditorTool
{
   typedef EditorTool Parent;

public:
   enum PaintElementMode
   {
      Floor = 0,
      Wall,
      Ceiling,
      FullTile
   };

   enum ActionMode
   {
      Select = 0,
      Paint,
      Erase,
      PaintBucket
   };

private:
   struct BrushSelection
   {
      U32 brush;
   };
   struct FaceSelection
   {
      U32 brush;
      U32 face;
   };
   struct VertSelection
   {
      U32 brush;
      U32 vert;
   };
   Vector<BrushSelection> mSelectedBrushes;
   Vector<FaceSelection> mSelectedFaces;
   Vector<VertSelection> mVertBrushes;

   RayInfo mLastRayInfo;

   bool mMouseDown;
   bool mMouseDragged;

   bool mCreateMode;
   S32 mCreateStage;

   S32 mBrushHL;
   S32 mFaceHL;

   //Grid render
   F32 mGridPlaneSize;

   Point2F mHLGridPosition;

   Point2F mMouseDownPosition;

   //Modifier tracking
   bool shiftMod;
   bool altMod;

   U32 mCurrentLevel;

   TilesetObject* mSelectedTileObject;

   Vector<TileElement> mSelectedElements;

   ActionMode mActionMode;
   PaintElementMode mElementMode;

   Point3F mouseVector;
   Point3F mouseStartPos;

   bool mRestrictWallsToFlooredTiles;

public:
   TilesetObjectEditorTool();
   ~TilesetObjectEditorTool(){}

   DECLARE_CONOBJECT(TilesetObjectEditorTool);

   bool onAdd();
   void onRemove();

   //Called when the tool is activated on the World Editor
   virtual void onActivated(WorldEditor*);

   //Called when the tool is deactivated on the World Editor
   virtual void onDeactivated();

   //
   static bool _cursorCastCallback(RayInfo* ri);

   //
   virtual bool onMouseMove(const Gui3DMouseEvent &);
   virtual bool onMouseDown(const Gui3DMouseEvent &);
   virtual bool onMouseDragged(const Gui3DMouseEvent &);
   virtual bool onMouseUp(const Gui3DMouseEvent &);

   //
   virtual bool onRightMouseDown(const Gui3DMouseEvent &);
   virtual bool onRightMouseDragged(const Gui3DMouseEvent &);
   virtual bool onRightMouseUp(const Gui3DMouseEvent &);

   //
   virtual bool onMiddleMouseDown(const Gui3DMouseEvent &);
   virtual bool onMiddleMouseDragged(const Gui3DMouseEvent &);
   virtual bool onMiddleMouseUp(const Gui3DMouseEvent &);

   //
   virtual bool onInputEvent(const InputEventInfo &);

   //
   virtual void render();

   void renderGrid();

   bool carveAction();

   bool addBoxBrush(Box3F);

   U32 getCurrentLevel() 
   {
      return mCurrentLevel;
   }

   Level* getLevel(U32 levelIndex)
   {
      if(mSelectedTileObject != nullptr && mSelectedTileObject->mLevels.size() >= levelIndex)
         return &mSelectedTileObject->mLevels[levelIndex];

      return nullptr;
   }

   inline float snap(float value, float size)
   {
      return mFloor(value / size) * size;
   }

   /*inline F32 snap(F32 value, F32 size)
   {
      // Added std::abs to give correct behaviour for negative values
      return value - mAbs(mFmod(value, size));
   }*/


   void setElementMode(U32 mode)
   {
      mElementMode = static_cast<PaintElementMode>(mode);
   }

   void setActionMode(U32 mode)
   {
      mActionMode = static_cast<ActionMode>(mode);
   }

   Tile* getTileAtCoord(Point2F coord);
   Vector<Tile*> getTilesInBox(Box3F box);

   Box3F getTileBoundsFaceBox(Tile *tile, Point3F rayStart, Point3F rayEnd);

   void clearSelectedElements()
   {
      if (mSelectedTileObject == nullptr)
         return;

      for (U32 i = 0; i < mSelectedTileObject->mLevels[mCurrentLevel].tiles.size(); ++i)
      {
         Tile* tile = &mSelectedTileObject->mLevels[mCurrentLevel].tiles[i];

         for (U32 e = 0; e < tile->elements.size(); ++e)
         {
            tile->elements[e].selected = false;
         }
      }
   }

   void riseFloor();
   void lowerFloor();

   void nextFloor();
   void prevFloor();

   void setActiveTileObject(TilesetObject* tilesetObject);
   TilesetObject* getActiveTileObject() { return mSelectedTileObject;  }

};

#endif