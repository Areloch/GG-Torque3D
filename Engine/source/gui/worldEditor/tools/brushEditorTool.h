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

#ifndef _BRUSH_EDITOR_TOOL_
#define _BRUSH_EDITOR_TOOL_

#ifndef _EDITOR_TOOL_
#include "gui/worldEditor/tools/editorTool.h"
#endif

#ifndef _BRUSH_OBJECT_H_
#include "T3D/BrushObject.h"
#endif

class Brush;

class BrushEditorTool : public EditorTool
{
   typedef EditorTool Parent;

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

   bool mCreateMode;
   S32 mCreateStage;

   BrushObject* mBrushObjHL;
   S32 mBrushHL;
   S32 mFaceHL;

public:
   BrushEditorTool();
   ~BrushEditorTool(){}

   DECLARE_CONOBJECT(BrushEditorTool);

   bool onAdd();
   void onRemove();

   //Called when the tool is activated on the World Editor
   virtual void onActivated(WorldEditor*);

   //Called when the tool is deactivated on the World Editor
   virtual void onDeactivated();

   //
   static bool _cursorCastCallback(RayInfo* ri);
   bool _cursorCast(const Gui3DMouseEvent &event, BrushObject **hitShape, S32 *hitBrush, S32 *hitFace);
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

   bool carveAction();

   bool addBoxBrush(Box3F);
};

#endif