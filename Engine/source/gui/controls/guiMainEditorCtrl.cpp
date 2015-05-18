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
#include "gui/controls/guiMainEditorCtrl.h"

#include "console/console.h"
#include "console/consoleTypes.h"
#include "gfx/gfxDevice.h"
#include "gfx/gfxDrawUtil.h"
#include "gui/core/guiCanvas.h"
#include "gui/core/guiDefaultControlRender.h"

#include "gfx/gfxDrawUtil.h"
#include "console/engineAPI.h"

#include "math/mathUtils.h"

#include "gui/worldEditor/editTSCtrl.h"

#include "T3D/gameTSCtrl.h"

IMPLEMENT_CONOBJECT(guiMainEditorCtrl);

ConsoleDocClass( guiMainEditorCtrl,
   "@brief The most widely used button class.\n\n"
   
   "guiMainEditorCtrl renders seperately of, but utilizes all of the functionality of GuiBaseButtonCtrl.\n" 
   "This grants guiMainEditorCtrl the versatility to be either of the 3 button types.\n\n"

   "@tsexample\n"
   "// Create a PushButton guiMainEditorCtrl that calls randomFunction when clicked\n"
   "%button = new guiMainEditorCtrl()\n"
   "{\n"
   "   profile    = \"GuiButtonProfile\";\n"
   "   buttonType = \"PushButton\";\n"
   "   command    = \"randomFunction();\";\n"
   "};\n"
   "@endtsexample\n\n"
   
   "@ingroup GuiButtons"
);


//-----------------------------------------------------------------------------

guiMainEditorCtrl::guiMainEditorCtrl()
{
   String fontCacheDir = Con::getVariable("$GUI::fontCacheDirectory");
   mFont = GFont::create("Arial", 12, fontCacheDir);

   borderWidth = 5;
   dockHoverWindowIdx = -1;
   mMovingWindow = false;

   draggingDivider = -1;
   hoveredDivider = -1;

   defaultTabSize = Point2I(80,20);
}

//-----------------------------------------------------------------------------

bool guiMainEditorCtrl::onWake()
{
   if( !Parent::onWake() )
      return false;

   setBounds(RectI(0,0,800,600));

   addWindow("3d View");

   mWindows[0].bounds = getBounds();
   mWindows[0].rootWindow = true;

   GameTSCtrl* editViewCtrl = new GameTSCtrl();

   editViewCtrl->registerObject();

   editViewCtrl->setPosition(getWindowBodyBounds(0).point);
   editViewCtrl->setExtent(getWindowBodyBounds(0).extent);

   mWindows[0].mChildControls.push_back(editViewCtrl);

   return true;
}

//-----------------------------------------------------------------------------

void guiMainEditorCtrl::onRender(Point2I      offset,
   const RectI& updateRect)
{
   GFXDrawUtil *drawer = GFX->getDrawUtil();

   //first our background
   drawer->drawRectFill(updateRect.point, updateRect.point + updateRect.extent, ColorI(42, 42, 42));

   for (U32 i = 0; i < mWindows.size(); i++)
   {
      Point2I nodePos = mWindows[i].pos;// +offset;

      //first our background
      Point2F minPos, maxPos;
      RectI windowBounds = getWindowBounds(i);

      ColorI bgColor = ColorI(42, 42, 42);
      if (selectedWindow == i)
         bgColor = ColorI(42, 42, 42, 80);

      drawer->drawRectFill(windowBounds.point, windowBounds.point + windowBounds.extent, bgColor);

      if (!mWindows[i].rootWindow)
      {

         //draw the headerbar first
         windowBounds = getWindowTabBounds(i);

         Point2F minPos = Point2F(windowBounds.point.x, windowBounds.point.y);
         Point2F maxPos = Point2F(windowBounds.point.x + mWindows[i].tabBounds.extent.x, windowBounds.point.y + mWindows[i].tabBounds.extent.y);

         ColorI tabColor = ColorI(169, 169, 169);
         if (selectedWindow == i)
            tabColor = ColorI(169, 169, 169, 80);

         drawer->drawRectFill(minPos, maxPos, tabColor);

         //header text
         U32 strWidth = mFont->getStrWidth(mWindows[i].windowName.c_str());

         Point2F headerPos = Point2F(minPos.x + windowBounds.extent.x / 2 - strWidth / 2, minPos.y + windowBounds.extent.y / 2 - 6); //6 is half the text height

         drawer->setBitmapModulation(ColorI(255, 255, 255));
         drawer->drawText(mFont, headerPos, mWindows[i].windowName.c_str());
         drawer->clearBitmapModulation();
      }

      windowBounds = getWindowBodyBounds(i);

      //body
      minPos = Point2F(windowBounds.point.x, windowBounds.point.y);
      maxPos = Point2F(windowBounds.point.x + windowBounds.extent.x, windowBounds.point.y + windowBounds.extent.y);

      ColorI bodyColor = ColorI(106, 106, 106);
      if (selectedWindow == i)
         bodyColor = ColorI(106, 106, 106, 80);

      drawer->drawRectFill(minPos, maxPos, bodyColor);

      if (dockHoverWindowIdx == i)
      {
         RectI scaledBox = RectI(windowBounds.point + windowBounds.extent / 4, windowBounds.extent / 2);

         drawer->drawLine(scaledBox.point, windowBounds.point, ColorI(255, 255, 255));

         drawer->drawLine(scaledBox.point, Point2I(scaledBox.point.x + scaledBox.extent.x, scaledBox.point.y), ColorI(255, 255, 255));

         drawer->drawLine(Point2I(scaledBox.point.x + scaledBox.extent.x, scaledBox.point.y),
            Point2I(windowBounds.point.x + windowBounds.extent.x, windowBounds.point.y), ColorI(255, 255, 255));

         drawer->drawLine(Point2I(scaledBox.point.x + scaledBox.extent.x, scaledBox.point.y),
            Point2I(scaledBox.point.x + scaledBox.extent.x, scaledBox.point.y + scaledBox.extent.y), ColorI(255, 255, 255));

         drawer->drawLine(Point2I(scaledBox.point.x + scaledBox.extent.x, scaledBox.point.y + scaledBox.extent.y),
            Point2I(windowBounds.point.x + windowBounds.extent.x, windowBounds.point.y + windowBounds.extent.y), ColorI(255, 255, 255));

         drawer->drawLine(Point2I(scaledBox.point.x + scaledBox.extent.x, scaledBox.point.y + scaledBox.extent.y),
            Point2I(scaledBox.point.x, scaledBox.point.y + scaledBox.extent.y), ColorI(255, 255, 255));

         drawer->drawLine(Point2I(scaledBox.point.x, scaledBox.point.y + scaledBox.extent.y),
            Point2I(windowBounds.point.x, windowBounds.point.y + windowBounds.extent.y), ColorI(255, 255, 255));

         drawer->drawLine(Point2I(scaledBox.point.x, scaledBox.point.y + scaledBox.extent.y), scaledBox.point, ColorI(255, 255, 255));

         //next, if we're over one of our dockable spaces, draw a box to indicate where it'd dock to
         S32 dockSpace = getWindowDockSpace(i, mMousePosition);
         if (dockSpace != -1)
         {
            if (dockSpace == 0)
            {
               drawer->drawRectFill(RectI(windowBounds.point, Point2I(windowBounds.extent.x, windowBounds.extent.y / 4)), ColorI(0, 0, 255, 80));
            }
            else if (dockSpace == 1)
            {
               drawer->drawRectFill(RectI(Point2I(scaledBox.point.x + scaledBox.extent.x, windowBounds.point.y),
                  Point2I(windowBounds.extent.x / 4, windowBounds.extent.y)), ColorI(0, 0, 255, 80));
            }
            else if (dockSpace == 2)
            {
               drawer->drawRectFill(RectI(Point2I(windowBounds.point.x, scaledBox.point.y + scaledBox.extent.y),
                  Point2I(windowBounds.extent.x, windowBounds.extent.y / 4)), ColorI(0, 0, 255, 80));
            }
            else if (dockSpace == 3)
            {
               drawer->drawRectFill(RectI(windowBounds.point, Point2I(windowBounds.extent.x / 4, windowBounds.extent.y)), ColorI(0, 0, 255, 80));
            }
         }
      }


      for (U32 c = 0; c < mWindows[i].mChildControls.size(); c++)
      {
         mWindows[i].mChildControls[c]->onRender(offset, mWindows[i].bounds);
      }
   }

   for(U32 i=0; i < mDividers.size(); i++)
   {
      if(hoveredDivider == i)
      {
         drawer->drawRectFill(mDividers[i].dividerBounds, ColorI(255,255,255));
      }
   }
}

//
void guiMainEditorCtrl::onMouseDown(const GuiEvent &event)
{
   if (!mActive)
      return;

   mMovingWindow = false;
   mMouseDownPosition = event.mousePoint;
   mLastDragPosition = mDeltaMousePosition = mMousePosition;

   for (U32 i = 0; i < mWindows.size(); i++)
   {
      if (mWindows[i].rootWindow)
         continue;

      RectI tabBounds = getWindowTabBounds(i);

      if(tabBounds.pointInRect(event.mousePoint))
      {
         mMovingWindow = true;
         windowMoveOffset = mWindows[i].bounds.point - event.mousePoint;
         mLastDragPosition = event.mousePoint;
         selectedWindow = i;
         break;
      }
   }

   if(hoveredDivider != -1)
   {
      draggingDivider = hoveredDivider;
   }

   /*for (U32 i = 0; i < mNodes.size(); i++)
   {
      //RectI nodeBounds = RectI(mNodes[i].pos.x - mNodes[i].bounds.x / 2, mNodes[i].pos.y - mNodes[i].bounds.y / 2, mNodes[i].bounds.x, mNodes[i].bounds.y);
      RectI nodeBounds = getNodeBounds(i);
      nodeBounds.point.y -= mNodes[i].headerHeight;
      nodeBounds.extent.y += mNodes[i].headerHeight;

      if (nodeBounds.pointInRect(event.mousePoint))
      {
         Point2I nodePos = mNodes[i].pos;

         for (U32 s = 0; s < mNodes[i].mInSockets.size(); s++)
         {
            RectI sockBounds = getSocketBounds(i, IN_ARG, s);

            if (sockBounds.pointInRect(event.mousePoint))
            {
               //test if we have a connection to this socket. if we don't just bail, if we do, remove it and prepare a new connection
               for (U32 c = 0; c < mConnections.size(); c++)
               {
                  if (mConnections[c].endNode == i && mConnections[c].endSocket == s)
                  {
                     mDraggingConnection = true;
                     newConnectionStartNode = mConnections[c].startNode;
                     newConnectionStartSocket = mConnections[c].startSocket;
                     mConnections.erase(c);
                     break;
                  }
               }
            }
         }

         for (U32 s = 0; s < mNodes[i].mOutSockets.size(); s++)
         {
            RectI sockBounds = getSocketBounds(i, OUT_ARG, s);
            //Point2F minPos = Point2F(nodePos.x + mNodes[i].bodyBounds.x / 2 - 10, nodePos.y - mNodes[i].bodyBounds.y / 2 + 5);
            //Point2F maxPos = Point2F(nodePos.x + mNodes[i].bodyBounds.x / 2, nodePos.y - mNodes[i].bodyBounds.y / 2 + 15);
            
            //RectI sockBounds = RectI(minPos.x, minPos.y, mNodes[i].mOutSockets[s].buttonBounds.x, mNodes[i].mOutSockets[s].buttonBounds.y);
            if (sockBounds.pointInRect(event.mousePoint))
            {
               mDraggingConnection = true;

               newConnectionStartNode = i;
               newConnectionStartSocket = s;
            }
         }

         if(!mDraggingConnection)
         {
            //test the out path real fast
            RectI sockBounds = getSocketBounds(i, OUT_PATH, 0);

            if (sockBounds.pointInRect(event.mousePoint))
            {
               mDraggingConnection = true;

               newConnectionStartNode = i;
               newConnectionStartSocket = -2;
            }
         }

         if (!mDraggingConnection)
         {
            mMovingNode = true;
            //selectedNode = &mNodes[i];
            nodeMoveOffset = mNodes[i].pos - event.mousePoint;
            mLastDragPosition = event.mousePoint;

            mSelectedNodes.push_back_unique(i);
            break; //first come, first serve
         }
         else
         {
            mSelectedNodes.clear();
         }
      }
   }

   if (!mMovingNode && !mDraggingConnection)
   {
      //if we're not moving a node or connection, we're selecting
      mDraggingSelect = true;
      selectBoxStart = event.mousePoint;
      mSelectedNodes.clear();
   }*/
}

void guiMainEditorCtrl::onMouseDragged(const GuiEvent &event)
{
   if (!mActive)
      return;

   mMousePosition = event.mousePoint;
   Point2I deltaMousePosition = mMousePosition - mLastDragPosition;

   if (mMovingWindow)
   {
      //find if we've docked this window anywhere
      bool wasDocked = false;
      S32 dockedDivider = -1;
      for(U32 i=0; i <  mDividers.size(); i++)
      {
         if(mDividers[i].windowAIdx == selectedWindow ||
            mDividers[i].windowBIdx == selectedWindow)
         {
            wasDocked = true;
            dockedDivider = i;
            break;
         }
      }
      
      //first thing's first, we need to "pop" the window out and remove the divider if we were docked
      if(wasDocked)
      {
         if(mDividers[dockedDivider].windowAIdx != selectedWindow)
         {
            mWindows[mDividers[dockedDivider].windowAIdx].bounds = mDividers[dockedDivider].windowSpaceBounds;
         }
         else
         {
            mWindows[mDividers[dockedDivider].windowBIdx].bounds = mDividers[dockedDivider].windowSpaceBounds;
         }
         mDividers.erase(dockedDivider);
      }

      mWindows[selectedWindow].bounds.point += deltaMousePosition;

      //next, see if we're over another window
      for (U32 i = 0; i < mWindows.size(); i++)
      {
         RectI bodyBounds = getWindowBodyBounds(i);
         if (bodyBounds.pointInRect(mMousePosition))
         {
            dockHoverWindowIdx = i;
            break;
         }
      }
   }

   //we're dragging a window divider, do some scaling action!
   if(draggingDivider != -1)
   {
      //adjust it's position based on the axis it can move
      if(mDividers[draggingDivider].mOrientation == divider::VERT)
      {
         mDividers[draggingDivider].dividerBounds.point.y += deltaMousePosition.y;
         //and adjust the 2 windows' extents that connect to this divider
         mWindows[mDividers[draggingDivider].windowAIdx].bounds.extent.y += deltaMousePosition.y;
         mWindows[mDividers[draggingDivider].windowBIdx].bounds.point.y += deltaMousePosition.y;
         mWindows[mDividers[draggingDivider].windowBIdx].bounds.extent.y -= deltaMousePosition.y;
      }
      else if(mDividers[draggingDivider].mOrientation == divider::HORZ)
      {
         mDividers[draggingDivider].dividerBounds.point.x += deltaMousePosition.x;
         //and adjust the 2 windows' extents that connect to this divider
         mWindows[mDividers[draggingDivider].windowAIdx].bounds.extent.x += deltaMousePosition.x;
         mWindows[mDividers[draggingDivider].windowBIdx].bounds.point.x += deltaMousePosition.x;
         mWindows[mDividers[draggingDivider].windowBIdx].bounds.extent.x -= deltaMousePosition.x;
      }
   }

   /*if (mMovingNode || !mSelectedNodes.empty() && !mDraggingSelect)
   {
      for (U32 i = 0; i < mSelectedNodes.size(); i++)
      {
         mNodes[mSelectedNodes[i]].pos += deltaMousePosition;
      }
      //selectedNode->pos += deltaMousePosition;// +adjustedMousePos;
   }

   if (!mMovingNode && !mDraggingConnection && !mDraggingSelect)
   {
      mDraggingSelect = true;
   }

   if (mDraggingSelect)
   {
      for (U32 i = 0; i < mNodes.size(); i++)
      {
         RectI nodeBounds = getNodeBounds(i);

         RectI selectRect;
         if (selectBoxStart.x < mMousePosition.x && selectBoxStart.y < mMousePosition.y)
         {
            selectRect = RectI(selectBoxStart, Point2I(mAbs(selectBoxStart.x - mMousePosition.x), mAbs(selectBoxStart.y - mMousePosition.y)));
         }
         else
         {
            selectRect = RectI(mMousePosition, Point2I(mAbs(selectBoxStart.x - mMousePosition.x), mAbs(selectBoxStart.y - mMousePosition.y)));
         }

         S32 headerHeight = mNodes[i].headerHeight;
         Point2I ulCorner = Point2I(nodeBounds.point.x, nodeBounds.point.y - headerHeight);
         Point2I urCorner = Point2I(nodeBounds.point.x + nodeBounds.extent.x, nodeBounds.point.y - headerHeight);
         Point2I llCorner = Point2I(nodeBounds.point.x, nodeBounds.point.y + nodeBounds.extent.y);
         Point2I lrCorner = Point2I(nodeBounds.point.x + nodeBounds.extent.x, nodeBounds.point.y + nodeBounds.extent.y);

         if (selectRect.pointInRect(ulCorner) || selectRect.pointInRect(urCorner) || selectRect.pointInRect(llCorner) ||
            selectRect.pointInRect(lrCorner))
         {
            mSelectedNodes.push_back_unique(i);
         }
         else
         {
            //remove it in REAL TIME!!! WOOOAAAH
            if (!mSelectedNodes.empty())
            {
               S32 foundIdx = mSelectedNodes.find_next(i, 0);
               if (foundIdx != -1)
                  mSelectedNodes.erase(foundIdx);
            }
         }
      }
   }*/

   mLastDragPosition = mMousePosition;
}

void guiMainEditorCtrl::onMouseUp(const GuiEvent &event)
{
   if (!mActive)
      return;

   if (mMovingWindow && selectedWindow != -1 && dockHoverWindowIdx != -1)
   {
      mWindows[selectedWindow].bounds = getWindowBounds(selectedWindow);
      mWindows[selectedWindow].tabBounds = getWindowTabBounds(selectedWindow);

      RectI newDockeeWindowBounds = RectI(0,0,0,0); 

      //now, push the window we docked with over respective the dock position
      S32 dockSpace = getWindowDockSpace(dockHoverWindowIdx, mMousePosition);
      if (dockSpace == DOCK_TOP)
      {
         newDockeeWindowBounds.point.x = mWindows[dockHoverWindowIdx].bounds.point.x;
         newDockeeWindowBounds.point.y = mWindows[selectedWindow].bounds.point.y 
            + mWindows[selectedWindow].bounds.extent.y;
         newDockeeWindowBounds.extent.x = mWindows[dockHoverWindowIdx].bounds.extent.x;
         newDockeeWindowBounds.extent.y = mWindows[dockHoverWindowIdx].bounds.extent.y
            - mWindows[selectedWindow].bounds.extent.y;

         RectI fullBounds = mWindows[dockHoverWindowIdx].bounds;
         mWindows[dockHoverWindowIdx].bounds = newDockeeWindowBounds;

         divider div;
         div.windowAIdx = selectedWindow;
         div.windowBIdx = dockHoverWindowIdx;
         div.mOrientation = divider::VERT;
         div.dividerBounds = RectI(newDockeeWindowBounds.point.x, newDockeeWindowBounds.point.y - borderWidth + (S32)(borderWidth/2),
            newDockeeWindowBounds.extent.x, borderWidth);

         div.windowSpaceBounds = fullBounds;

         mDividers.push_back(div);
      }
      else if(dockSpace == DOCK_RIGHT)
      {
         newDockeeWindowBounds.point.x = mWindows[dockHoverWindowIdx].bounds.point.x;
         newDockeeWindowBounds.point.y = mWindows[selectedWindow].bounds.point.y;;
         newDockeeWindowBounds.extent.x = mWindows[dockHoverWindowIdx].bounds.extent.x / 2;
         newDockeeWindowBounds.extent.y = mWindows[dockHoverWindowIdx].bounds.extent.y;

         RectI fullBounds = mWindows[dockHoverWindowIdx].bounds;
         mWindows[dockHoverWindowIdx].bounds = newDockeeWindowBounds;

         divider div;
         div.windowAIdx = dockHoverWindowIdx;
         div.windowBIdx = selectedWindow;
         div.mOrientation = divider::HORZ;
         div.dividerBounds = RectI(newDockeeWindowBounds.point.x + newDockeeWindowBounds.extent.x - borderWidth + (S32)(borderWidth/2), 
            newDockeeWindowBounds.point.y, borderWidth, newDockeeWindowBounds.extent.y);

         div.windowSpaceBounds = fullBounds;

         mDividers.push_back(div);
      }
   }

   //test to see if we were making a connection and, if so, did we finish it
   /*if(mDraggingConnection)
   {
      for(U32 i=0; i < mNodes.size(); i++)
      {
         if(newConnectionStartSocket == -2)
         {
            RectI socketBounds = getSocketBounds(i, IN_PATH, 0);
            if (socketBounds.pointInRect(event.mousePoint))
            {
               //only one connection to an in-socket
               bool found = false;
               for (U32 c = 0; c < mConnections.size(); c++)
               {
                  if (mConnections[c].endNode == i && mConnections[c].endSocket == -2)
                  {
                     found = true;
                     break;
                  }
               }

               if (!found)
               {
                  connection newConnection;

                  newConnection.startSocket = newConnectionStartSocket;
                  newConnection.endSocket = -2;
                  newConnection.startNode = newConnectionStartNode;
                  newConnection.endNode = i;
                  newConnection.error = false;

                  mConnections.push_back(newConnection);
               }
            }
         }
         else
         {
            for(U32 s=0; s < mNodes[i].mInSockets.size(); s++)
            {
               RectI socketBounds = getSocketBounds(i, IN_ARG, s);
               if (socketBounds.pointInRect(event.mousePoint))
               {
                  //only one connection to an in-socket
                  bool found = false;
                  for (U32 c = 0; c < mConnections.size(); c++)
                  {
                     if (mConnections[c].endNode == i && mConnections[c].endSocket == s)
                     {
                        found = true;
                        break;
                     }
                  }

                  if (!found)
                  {
                     connection newConnection;

                     newConnection.startSocket = newConnectionStartSocket;
                     newConnection.endSocket = s;
                     newConnection.startNode = newConnectionStartNode;
                     newConnection.endNode = i;
                     newConnection.error = false;

                     mConnections.push_back(newConnection);
                  }
               }
            }
         }
      }
   }*/

   mMovingWindow = false;
   selectedWindow = -1;
   dockHoverWindowIdx = -1;
   draggingDivider = -1;
}

void guiMainEditorCtrl::onMouseMove(const GuiEvent &event)
{
   mMousePosition = globalToLocalCoord(event.mousePoint);

   hoveredDivider = -1;

   //check if we're hovering over a divider!
   for(U32 i=0; i < mDividers.size(); i++)
   {
      if(mDividers[i].dividerBounds.pointInRect(mMousePosition))
      {
         //hovered!
         hoveredDivider = i;
         break;
      }
   }
}
//

void guiMainEditorCtrl::addWindow(String windowName)
{
   Window newWindow;

   newWindow.pos = Point2I(100, 100);
   newWindow.tabBounds.extent.x = defaultTabSize.x;
   newWindow.tabBounds.extent.y = defaultTabSize.y;
   //newWindow.bounds = Point2I(120, 100);
   newWindow.bounds = RectI(newWindow.pos, Point2I(120, 100 + newWindow.tabBounds.extent.y));
   newWindow.windowName = windowName == String("") ? "Test" : windowName;
   newWindow.rootWindow = false;

   mWindows.push_back(newWindow);
}

RectI guiMainEditorCtrl::getWindowBounds(S32 windowIdx)
{
   //get our largest horizontal height
   if (dockHoverWindowIdx != -1 && selectedWindow == windowIdx)
   {
      //previs based on where we may end up!
      S32 dockSpace = getWindowDockSpace(dockHoverWindowIdx, mMousePosition);
      if (dockSpace == DOCK_TOP)
      {
         //top space, so we do half-down
         RectI dockedWindowBounds = getWindowBounds(dockHoverWindowIdx);

         RectI bounds = RectI(dockedWindowBounds.point.x, dockedWindowBounds.point.y,
            dockedWindowBounds.extent.x, dockedWindowBounds.point.y + dockedWindowBounds.extent.y / 2);

         return bounds;
      }
      else if (dockSpace == DOCK_RIGHT)
      {
         //top space, so we do half-left
         RectI dockedWindowBounds = getWindowBounds(dockHoverWindowIdx);

         RectI bounds = RectI(dockedWindowBounds.point.x + dockedWindowBounds.extent.x/2, dockedWindowBounds.point.y,
            dockedWindowBounds.extent.x / 2, dockedWindowBounds.point.y + dockedWindowBounds.extent.y);

         return bounds;
      }
      else if (dockSpace == DOCK_BOTTOM)
      {
         //top space, so we do half-up
         RectI dockedWindowBounds = getWindowBounds(dockHoverWindowIdx);

         RectI bounds = RectI(dockedWindowBounds.point.x, dockedWindowBounds.point.y,
            dockedWindowBounds.extent.x, dockedWindowBounds.point.y + dockedWindowBounds.extent.y / 2);

         return bounds;
      }
      else if (dockSpace == DOCK_LEFT)
      {
         //top space, so we do half-right
         RectI dockedWindowBounds = getWindowBounds(dockHoverWindowIdx);

         RectI bounds = RectI(dockedWindowBounds.point.x, dockedWindowBounds.point.y,
            dockedWindowBounds.extent.x, dockedWindowBounds.point.y + dockedWindowBounds.extent.y / 2);

         return bounds;
      }
   }

   return mWindows[windowIdx].bounds;
}

RectI guiMainEditorCtrl::getWindowTabBounds(S32 windowIdx)
{
   //get our largest horizontal height
   if (dockHoverWindowIdx != -1 && selectedWindow == windowIdx)
   {
      //previs based on where we may end up!
      S32 dockSpace = getWindowDockSpace(dockHoverWindowIdx, mMousePosition);
      if (dockSpace == 0)
      {
         //top space, so we do half-down
         RectI dockedWindowBounds = getWindowBodyBounds(dockHoverWindowIdx);

         return RectI(dockedWindowBounds.point.x + (borderWidth * 2), dockedWindowBounds.point.y + borderWidth,
            mWindows[windowIdx].tabBounds.extent.x, mWindows[windowIdx].tabBounds.extent.y + borderWidth);
      }
   }

   RectI tabBounds = RectI(mWindows[windowIdx].bounds.point.x + (borderWidth*2), mWindows[windowIdx].bounds.point.y + borderWidth, 
      defaultTabSize.x, defaultTabSize.y + borderWidth);

   return tabBounds;
}

RectI guiMainEditorCtrl::getWindowBodyBounds(S32 windowIdx)
{
   //get our largest horizontal height
   RectI bodyBounds;
   if (mWindows[windowIdx].rootWindow)
   {
      bodyBounds = RectI(mWindows[windowIdx].bounds.point.x + borderWidth, mWindows[windowIdx].bounds.point.y + borderWidth,
         mWindows[windowIdx].bounds.extent.x - (borderWidth * 2), mWindows[windowIdx].bounds.extent.y - (borderWidth * 2));
   }
   else
   {
      if (dockHoverWindowIdx != -1 && selectedWindow == windowIdx)
      {
         //previs based on where we may end up!
         S32 dockSpace = getWindowDockSpace(dockHoverWindowIdx, mMousePosition);
         if (dockSpace == 0)
         {
            //top space, so we do half-down
            RectI dockedWindowBounds = getWindowBodyBounds(dockHoverWindowIdx);

            bodyBounds = RectI(dockedWindowBounds.point.x + borderWidth, dockedWindowBounds.point.y + defaultTabSize.y + borderWidth,
               dockedWindowBounds.extent.x - (borderWidth * 2), defaultTabSize.y / 2 - mWindows[windowIdx].tabBounds.extent.y - (borderWidth * 2));
         }
         else
         {
            bodyBounds = RectI(mWindows[windowIdx].bounds.point.x + borderWidth, mWindows[windowIdx].bounds.point.y + defaultTabSize.y + borderWidth,
               mWindows[windowIdx].bounds.extent.x - (borderWidth * 2), mWindows[windowIdx].bounds.extent.y - defaultTabSize.y - (borderWidth * 2));
         }
      }
      else
      {
         bodyBounds = RectI(mWindows[windowIdx].bounds.point.x + borderWidth, mWindows[windowIdx].bounds.point.y + defaultTabSize.y + borderWidth,
            mWindows[windowIdx].bounds.extent.x - (borderWidth * 2), mWindows[windowIdx].bounds.extent.y - defaultTabSize.y - (borderWidth * 2));
      }
   }

   return bodyBounds;
}

S32 guiMainEditorCtrl::getWindowDockSpace(S32 windowIdx, Point2I mousePoint)
{
   RectI windowBounds = getWindowBodyBounds(windowIdx);

   RectI scaledBox = RectI(windowBounds.point + windowBounds.extent / 4, windowBounds.extent / 2);

   Point2F upperDockPoints[4] = { Point2F(scaledBox.point.x, scaledBox.point.y), Point2F(windowBounds.point.x, windowBounds.point.y),
      Point2F(windowBounds.point.x + windowBounds.extent.x, windowBounds.point.y), Point2F(scaledBox.point.x + scaledBox.extent.x, scaledBox.point.y) };

   Point2F rightDockPoints[4] = { Point2F(windowBounds.point.x + windowBounds.extent.x, windowBounds.point.y), 
      Point2F(scaledBox.point.x + scaledBox.extent.x, scaledBox.point.y), Point2F(scaledBox.point.x + scaledBox.extent.x, scaledBox.point.y + scaledBox.extent.y),
      Point2F(windowBounds.point.x + windowBounds.extent.x, windowBounds.point.y + windowBounds.extent.y) };

   Point2F bottomDockPoints[4] = { Point2F(windowBounds.point.x + windowBounds.extent.x, windowBounds.point.y + windowBounds.extent.y),
      Point2F(scaledBox.point.x + scaledBox.extent.x, scaledBox.point.y + scaledBox.extent.y),
      Point2F(scaledBox.point.x, scaledBox.point.y + scaledBox.extent.y),
      Point2F(windowBounds.point.x, windowBounds.point.y + windowBounds.extent.y)  };

   Point2F leftDockPoints[4] = { Point2F(windowBounds.point.x, windowBounds.point.y + windowBounds.extent.y),
      Point2F(scaledBox.point.x, scaledBox.point.y + scaledBox.extent.y), Point2F(scaledBox.point.x, scaledBox.point.y),
      Point2F(windowBounds.point.x, windowBounds.point.y) };

   if (MathUtils::pointInPolygon(upperDockPoints, 4, Point2F(mousePoint.x, mousePoint.y)))
   {
      return DOCK_TOP;
   }
   else if (MathUtils::pointInPolygon(rightDockPoints, 4, Point2F(mousePoint.x, mousePoint.y)))
   {
      return DOCK_RIGHT;
   }
   else if (MathUtils::pointInPolygon(bottomDockPoints, 4, Point2F(mousePoint.x, mousePoint.y)))
   {
      return DOCK_BOTTOM;
   }
   else if (MathUtils::pointInPolygon(leftDockPoints, 4, Point2F(mousePoint.x, mousePoint.y)))
   {
      return DOCK_LEFT;
   }

   return -1;
}

/*S32 guiMainEditorCtrl::isPointOnWindowboundary(S32 windowIdx, Point2I mousePoint)
{
   RectI windowBounds = getWindowBounds(windowIdx);

   RectI leftRectBnd = RectI(windowBounds.point, Point2I(borderWidth, windowsBounds.extent.y));
   RectI upRectBnd = RectI(windowBounds.point, Point2I(windowsBounds.extent.x, borderWidth));

   RectI leftRectBnd = RectI(windowBounds.point, Point2I(borderWidth, windowsBounds.extent.y));
   RectI upRectBnd = RectI(windowBounds.point, Point2I(windowsBounds.extent.x, borderWidth));
}*/

DefineEngineMethod( guiMainEditorCtrl, addWindow, void, (String windowName), (""),
   "Set the pattern by which to filter items in the tree.  Only items in the tree whose text "
   "matches this pattern are displayed.\n\n"
   "@param pattern New pattern based on which visible items in the tree should be filtered.  If empty, all items become visible.\n\n"
   "@see getFilterText\n"
   "@see clearFilterText" )
{
   object->addWindow(windowName);
}