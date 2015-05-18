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

#ifndef _GUI_MAIN_EDITOR_CTRL_H_
#define _GUI_MAIN_EDITOR_CTRL_H_

#ifndef _GUIBUTTONBASECTRL_H_
#include "gui/buttons/guiButtonBaseCtrl.h"
#endif

class guiMainEditorCtrl : public GuiButtonBaseCtrl
{
   typedef GuiButtonBaseCtrl Parent;

   enum
   {
      DOCK_TOP = 0,
      DOCK_RIGHT,
      DOCK_BOTTOM,
      DOCK_LEFT
   };

   Resource<GFont> mFont;

   struct divider
   {
      enum orient
      {
         HORZ = 0,
         VERT
      };

      orient mOrientation;

      S32 windowAIdx;
      S32 windowBIdx;

      RectI dividerBounds;

      RectI windowSpaceBounds;
   };

   struct Window
   {
      Point2I pos;

      RectI bounds;
      RectI tabBounds;

      String windowName;

      bool rootWindow; //cannot remove this window, and it doesn't have a tab at the top. 
                       //Also adjusts docking behavior for other windows assocaited with it

      //child gui controls
      Vector<GuiControl*> mChildControls;
   };

   Vector<divider> mDividers;

   Vector<Window> mWindows;

   bool mMovingWindow;

   S32 dockHoverWindowIdx;

   Point2I defaultTabSize;

   S32 hoveredDivider;
   S32 draggingDivider;

   Point2I mMouseDownPosition;
   Point2I mLastDragPosition;
   Point2I mMousePosition;
   Point2I mDeltaMousePosition;

   Point2I windowMoveOffset;

   S32 selectedWindow;

   S32 borderWidth;
   //

public:
   DECLARE_CONOBJECT(guiMainEditorCtrl);
   guiMainEditorCtrl();
   bool onWake();
   void onRender(Point2I offset, const RectI &updateRect);

   //
   virtual void onMouseDown(const GuiEvent &event);
   virtual void onMouseDragged(const GuiEvent &event);
   virtual void onMouseUp(const GuiEvent &event);
   virtual void onMouseMove(const GuiEvent &event);
   //

   void addWindow(String nodeName);

   RectI getWindowBounds(S32 windowIdx);
   RectI getWindowTabBounds(S32 windowIdx);
   RectI getWindowBodyBounds(S32 windowIdx);

   S32 getWindowDockSpace(S32 windowIdx, Point2I mousePoint);
};

#endif //_GUI_BUTTON_CTRL_H
