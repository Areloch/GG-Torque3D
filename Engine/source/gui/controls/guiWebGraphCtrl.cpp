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
#include "gui/controls/guiWebGraphCtrl.h"

#include "console/console.h"
#include "console/consoleTypes.h"
#include "gfx/gfxDevice.h"
#include "gfx/gfxDrawUtil.h"
#include "gui/core/guiCanvas.h"
#include "gui/core/guiDefaultControlRender.h"

#include "gfx/gfxDrawUtil.h"
#include "console/engineAPI.h"

#include "math/mathUtils.h"

IMPLEMENT_CONOBJECT(guiWebGraphCtrl);

ConsoleDocClass(guiWebGraphCtrl,
   "@brief The most widely used button class.\n\n"

   "guiWebGraphCtrl renders seperately of, but utilizes all of the functionality of GuiBaseButtonCtrl.\n"
   "This grants guiWebGraphCtrl the versatility to be either of the 3 button types.\n\n"

   "@tsexample\n"
   "// Create a PushButton guiWebGraphCtrl that calls randomFunction when clicked\n"
   "%button = new guiWebGraphCtrl()\n"
   "{\n"
   "   profile    = \"GuiButtonProfile\";\n"
   "   buttonType = \"PushButton\";\n"
   "   command    = \"randomFunction();\";\n"
   "};\n"
   "@endtsexample\n\n"

   "@ingroup GuiButtons"
   );


//-----------------------------------------------------------------------------

guiWebGraphCtrl::guiWebGraphCtrl()
{
   String fontCacheDir = Con::getVariable("$GUI::fontCacheDirectory");
   mFont = GFont::create("Arial", 15, fontCacheDir);

   mDraggingConnection = false;
   mDraggingSelect = false;

   viewScalar = 1;

   center = Point2I::Zero;

   baseNodeColor = ColorI(34, 36, 34, 255);
   baseNodeOutlineColor = ColorI::BLACK;
   baseConnectionColor = ColorI::WHITE;
   errorColor = ColorI::RED;
   selectColor = ColorI(228, 164, 65);
}

//-----------------------------------------------------------------------------

bool guiWebGraphCtrl::onWake()
{
   if (!Parent::onWake())
      return false;

   return true;
}

//-----------------------------------------------------------------------------

void guiWebGraphCtrl::onRender(Point2I offset,
   const RectI& updateRect)
{
   GFXDrawUtil *drawer = GFX->getDrawUtil();

   //first our background
   drawer->drawRectFill(updateRect.point, updateRect.point + updateRect.extent, ColorI(42, 42, 42));

   GFXStateBlockDesc desc;
   desc.setZReadWrite(true, true);
   desc.setBlend(true);
   desc.setCullMode(GFXCullNone);
   desc.fillMode = GFXFillSolid;

   //drawer->drawPlaneGrid(desc, Point3F(0, 0, 0), Point2F(100, 100), Point2F(100, 100), ColorI(65, 65, 65));

   //draw the origin axis
   realCenter = center + (updateRect.point + updateRect.extent / 2);

   drawer->drawLine(Point2I(realCenter.x - 1000, realCenter.y), Point2I(realCenter.x + 1000, realCenter.y), ColorI::BLACK);
   drawer->drawLine(Point2I(realCenter.x, realCenter.y - 1000), Point2I(realCenter.x, realCenter.y + 1000), ColorI::BLACK);

   if (mDraggingSelect)
   {
      drawer->drawRectFill(selectBoxStart, mMousePosition, ColorI(200, 200, 200));
   }

   //folders
   for (U32 i = 0; i < mFolders.size(); i++)
   {
      Point2I nodePos = mFolders[i].pos;// +offset;

      //draw the headerbar first
      RectI folderBounds = getFolderBounds(i);

      Point2F minPos = Point2F(folderBounds.point.x, folderBounds.point.y - mFolders[i].headerHeight);
      Point2F maxPos = Point2F(folderBounds.point.x + folderBounds.extent.x, folderBounds.point.y);

      drawer->drawRectFill(minPos, maxPos, ColorI(169, 169, 169));

      //header text
      U32 strWidth = mFont->getStrWidth(mFolders[i].nodeTitle.c_str());

      Point2F headerPos = Point2F(minPos.x + folderBounds.extent.x / 2 - strWidth / 2, minPos.y);

      drawer->setBitmapModulation(ColorI(255, 255, 255));
      drawer->drawText(mFont, headerPos, mFolders[i].nodeTitle.c_str());
      drawer->clearBitmapModulation();

      //body
      minPos = Point2F(folderBounds.point.x, folderBounds.point.y);
      maxPos = Point2F(folderBounds.point.x + folderBounds.extent.x, folderBounds.point.y + folderBounds.extent.y);

      drawer->drawRectFill(minPos, maxPos, ColorI(106, 106, 106));
   }

   for (U32 i = 0; i < mConnections.size(); i++)
   {
      Point2I startPoint = mNodes[mConnections[i].startNode].pos + realCenter;
      Point2I endPoint = mNodes[mConnections[i].endNode].pos + realCenter;

      VectorF connectionVec = VectorF(endPoint.x - startPoint.x, endPoint.y - startPoint.y, 0);
      VectorF rightVec = -mCross(connectionVec, VectorF(0, 0, 1));
      rightVec.normalize();

      startPoint += Point2I(rightVec.x * 10, rightVec.y * 10);
      endPoint += Point2I(rightVec.x * 10, rightVec.y * 10);

      //mid-way point, draw the triangle to indicate direction
      F32 len = connectionVec.len();
      connectionVec.normalize();

      F32 triWidth = 8;

      ColorI lineColor = mConnections[i].error ? errorColor : baseConnectionColor;

      if (selectedConnection == i)
      {
         lineColor = selectColor;
      }

      drawer->drawLine(startPoint, endPoint, lineColor);

      Point3F mid = connectionVec * (len / 2) + Point3F(startPoint.x, startPoint.y, 0);
      Point3F tribase = mid - (connectionVec * triWidth);

      drawer->drawLine(Point3F(tribase.x - (rightVec.x * triWidth), tribase.y - (rightVec.y * triWidth), 0),
         Point3F(tribase.x + (rightVec.x * triWidth), tribase.y + (rightVec.y * triWidth), 0), lineColor);

      drawer->drawLine(Point3F(tribase.x - (rightVec.x * triWidth), tribase.y - (rightVec.y * triWidth), 0), mid, lineColor);
      drawer->drawLine(mid, Point3F(tribase.x + (rightVec.x * triWidth), tribase.y + (rightVec.y * triWidth), 0), lineColor);
   }

   if (mDraggingConnection)
   {
      Point2I startPoint = mNodes[newConnectionStartNode].pos + realCenter;
      Point2I endPoint = mMousePosition;

      VectorF connectionVec = VectorF(endPoint.x - startPoint.x, endPoint.y - startPoint.y, 0);
      VectorF rightVec = -mCross(connectionVec, VectorF(0, 0, 1));
      rightVec.normalize();

      startPoint += Point2I(rightVec.x * 10, rightVec.y * 10);

      drawer->drawLine(startPoint, endPoint, baseConnectionColor);

   }

   for (U32 i = 0; i < mNodes.size(); i++)
   {
      Point2I nodePos = mNodes[i].pos;// +offset;

      RectI nodeBounds = getNodeBounds(i);

      //Draw the outline
      ColorI borderColor = mNodes[i].error ? errorColor : baseNodeOutlineColor;
      if (!mSelectedNodes.empty() && mSelectedNodes.find_next(i, 0) != -1)
         borderColor = selectColor;

      drawer->drawRect(Point2F(nodeBounds.point.x - 1, nodeBounds.point.y - 1),
         Point2F(nodeBounds.point.x + nodeBounds.extent.x + 1, nodeBounds.point.y + nodeBounds.extent.y + 1), borderColor);

      Point2F minPos = Point2F(nodeBounds.point.x, nodeBounds.point.y);
      Point2F maxPos = Point2F(nodeBounds.point.x + nodeBounds.extent.x,
         nodeBounds.point.y);

      //body
      minPos = Point2F(nodeBounds.point.x, nodeBounds.point.y);
      maxPos = Point2F(nodeBounds.point.x + nodeBounds.extent.x,
         nodeBounds.point.y + nodeBounds.extent.y);

      ColorI bodyColor = mNodes[i].useOverrideColor ? mNodes[i].overrideColor : baseNodeColor;

      drawer->drawRectFill(minPos, maxPos, bodyColor);

      //header text
      U32 strWidth = mFont->getStrWidth(mNodes[i].nodeTitle.c_str());

      Point2F nodeNamePos = Point2F(minPos.x + nodeBounds.extent.x / 2 - strWidth / 2, minPos.y + nodeBounds.extent.y / 2 - 7);

      drawer->setBitmapModulation(ColorI(255, 255, 255));
      drawer->drawText(mFont, nodeNamePos, mNodes[i].nodeTitle.c_str());
      drawer->clearBitmapModulation();
   }
}

//
void guiWebGraphCtrl::onMouseDown(const GuiEvent &event)
{
   if (!mActive)
      return;

   setFirstResponder();

   mMovingNode = false;
   //mDraggingConnection = false;
   mMouseDownPosition = event.mousePoint;
   mDeltaMousePosition = mMousePosition;

   mDraggingSelect = false;

   for (U32 i = 0; i < mNodes.size(); i++)
   {
      //RectI nodeBounds = RectI(mNodes[i].pos.x - mNodes[i].bounds.x / 2, mNodes[i].pos.y - mNodes[i].bounds.y / 2, mNodes[i].bounds.x, mNodes[i].bounds.y);
      RectI nodeBounds = getNodeBounds(i);

      if (nodeBounds.pointInRect(event.mousePoint))
      {
         Point2I nodePos = mNodes[i].pos;

         if (!mDraggingConnection)
         {
            mMovingNode = true;
            //selectedNode = &mNodes[i];
            nodeMoveOffset = mNodes[i].pos - event.mousePoint;
            mLastDragPosition = event.mousePoint;

            selectedConnection = -1;

            if (event.modifier & SI_SHIFT)
            {
               mSelectedNodes.push_back_unique(i);
            }
            else
            {
               if (!mSelectedNodes.contains(i))
               {
                  mSelectedNodes.clear();
                  mSelectedNodes.push_back_unique(i);
                  Con::executef(this, "onNodeSelected", i);
               }
            }

            break; //first come, first serve
         }
         else
         {
            if (newConnectionStartNode != i)
            {
               connection newConnection;

               newConnection.startNode = newConnectionStartNode;
               newConnection.endNode = i;
               newConnection.error = false;

               mConnections.push_back(newConnection);

               Con::executef(this, "onNewConnection", mConnections.size() - 1);
            }
            mSelectedNodes.clear();
            Con::executef(this, "onNodeUnselected");
         }
      }
   }

   if (!mMovingNode && !mDraggingConnection)
   {
      //test if we clicked a connection
      bool hitConnection = false;
      for (U32 i = 0; i < mConnections.size(); i++)
      {
         Point2I startPoint = mNodes[mConnections[i].startNode].pos + realCenter;
         Point2I endPoint = mNodes[mConnections[i].endNode].pos + realCenter;

         VectorF connectionVec = VectorF(endPoint.x - startPoint.x, endPoint.y - startPoint.y, 0);
         VectorF rightVec = -mCross(connectionVec, VectorF(0, 0, 1));
         rightVec.normalize();

         startPoint += Point2I(rightVec.x * 10, rightVec.y * 10);
         endPoint += Point2I(rightVec.x * 10, rightVec.y * 10);

         //mid-way point, draw the triangle to indicate direction
         F32 len = connectionVec.len();
         connectionVec.normalize();

         F32 connectionSelWidth = 3;

         Point2F points[4] = { Point2F(startPoint.x - (rightVec.x * connectionSelWidth), startPoint.y - (rightVec.y * connectionSelWidth)),
            Point2F(startPoint.x + (rightVec.x * connectionSelWidth), startPoint.y + (rightVec.y * connectionSelWidth)),
            Point2F(endPoint.x + (rightVec.x * connectionSelWidth), endPoint.y + (rightVec.y * connectionSelWidth)),
            Point2F(endPoint.x - (rightVec.x * connectionSelWidth), endPoint.y - (rightVec.y * connectionSelWidth)) };

         Point2F clickPoint = Point2F(event.mousePoint.x, event.mousePoint.y);
         if (MathUtils::pointInPolygon(points, 4, clickPoint))
         {
            selectedConnection = i;
            Con::executef(this, "onConnectionSelected", i);
            hitConnection = true;
            break;
         }
      }

      if (!hitConnection)
      {
         //if we're not moving a node or connection, we're selecting
         mDraggingSelect = true;
         selectBoxStart = event.mousePoint;
         mMousePosition = event.mousePoint;
         mSelectedNodes.clear();
         selectedConnection = -1;
         Con::executef(this, "onNodeUnselected");
         Con::executef(this, "onConnectionUnselected");
      }
   }
}

void guiWebGraphCtrl::onMouseDragged(const GuiEvent &event)
{
   if (!mActive)
      return;

   mMousePosition = event.mousePoint;
   Point2I deltaMousePosition = mMousePosition - mLastDragPosition;

   if (mMovingNode || !mSelectedNodes.empty() && !mDraggingSelect)
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

         Point2I ulCorner = Point2I(nodeBounds.point.x, nodeBounds.point.y);
         Point2I urCorner = Point2I(nodeBounds.point.x + nodeBounds.extent.x, nodeBounds.point.y);
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
   }

   mLastDragPosition = mMousePosition;
}

void guiWebGraphCtrl::onMouseUp(const GuiEvent &event)
{
   if (!mActive)
      return;

   //test to see if we were making a connection and, if so, did we finish it
   if (mDraggingConnection)
   {
      for (U32 i = 0; i < mNodes.size(); i++)
      {
         int g = 0;
      }
   }

   mDraggingSelect = false;
   mDraggingConnection = false;
   mMovingNode = false;
   newConnectionStartPoint = Point2I(0, 0);
}

void guiWebGraphCtrl::onMouseMove(const GuiEvent &event)
{
   //mMousePosition = globalToLocalCoord(event.mousePoint);
   mMousePosition = event.mousePoint;
}

void guiWebGraphCtrl::onRightMouseDown(const GuiEvent &event)
{
   S32 nodeIdx = -1;

   for (U32 i = 0; i < mNodes.size(); i++)
   {
      //RectI nodeBounds = RectI(mNodes[i].pos.x - mNodes[i].bounds.x / 2, mNodes[i].pos.y - mNodes[i].bounds.y / 2, mNodes[i].bounds.x, mNodes[i].bounds.y);
      RectI nodeBounds = getNodeBounds(i);

      if (nodeBounds.pointInRect(event.mousePoint))
      {
         nodeIdx = i;
         break;
      }
   }

   Con::executef(this, "onRightMouseDown", event.mousePoint, nodeIdx);
}

bool guiWebGraphCtrl::onMouseWheelUp(const GuiEvent &event)
{
   /*viewScalar += scalarStep;
   curZoomStep -= 1;
   curZoomStep = mClamp(curZoomStep, 1, zoomSteps);
   viewScalar = mClampF(viewScalar, scalarStep, 1);*/

   return true;
}

bool guiWebGraphCtrl::onMouseWheelDown(const GuiEvent &event)
{
   /*viewScalar -= scalarStep;
   curZoomStep += 1;
   curZoomStep = mClamp(curZoomStep, 1, zoomSteps);
   viewScalar = mClampF(viewScalar, scalarStep, 1);*/

   return true;
}
void guiWebGraphCtrl::onMiddleMouseDown(const GuiEvent &event)
{
   mMouseDownPosition = event.mousePoint;
   Point2I localPoint = globalToLocalCoord(event.mousePoint);

   mLastDragPosition = mMousePosition;
}

void guiWebGraphCtrl::onMiddleMouseDragged(const GuiEvent &event)
{
   mMousePosition = event.mousePoint;
   Point2I deltaMousePosition = mMousePosition - mLastDragPosition;

   center += deltaMousePosition;

   mLastDragPosition = mMousePosition;
}

void guiWebGraphCtrl::onMiddleMouseUp(const GuiEvent &event)
{
   /*for(U32 i=0; i < mNodes.size(); i++)
   {
   Point2I nodePos = mNodes[i]->getPosition();
   mNodes[i]->setPosition(nodePos - origin);
   }
   origin = Point2I(0,0);*/
   //sendMouseEvent("onMiddleMouseUp", event);
}

bool guiWebGraphCtrl::onKeyDown(const GuiEvent& event)
{
   // Otherwise, let the parent have the event...
   return Parent::onKeyDown(event);
}

bool guiWebGraphCtrl::onKeyUp(const GuiEvent& event)
{
   //only cut/copy work with this control...
   /*if (event.modifier & SI_COPYPASTE)
   {
      switch (event.keyCode)
      {
         //copy
      case KEY_C:
      {
         //make sure we actually have something selected
         if (mSelectionActive)
         {
            copyToClipboard(mSelectionStart, mSelectionEnd);
            setUpdate();
         }
         return true;
      }

      default:
         break;
      }
   }*/

   switch (event.keyCode)
   {
      case KEY_DELETE:
      {
         //Find out if we've selected anything, and if so, delete that crap
         for (U32 i = 0; i < mSelectedNodes.size(); i++)
         {
            deleteNode(mSelectedNodes[i]);
         }

         mSelectedNodes.clear();

         if (selectedConnection != -1)
         {
            mConnections.erase(selectedConnection);

            selectedConnection = -1;
         }
      }

      default:
         break;
   }

   // Otherwise, let the parent have the event...
   return Parent::onKeyUp(event);
}
//

S32 guiWebGraphCtrl::addNode(String nodeName)
{
   Node newNode;

   newNode.pos = Point2I(200, 200);
   newNode.bodyBounds = Point2I(120, 30);
   newNode.bounds = Point2I(newNode.bodyBounds.x, newNode.bodyBounds.y);
   newNode.nodeTitle = nodeName == String("") ? "Test" : nodeName;
   newNode.error = false;

   mNodes.push_back(newNode);

   return mNodes.size() - 1;
}

void guiWebGraphCtrl::addFolder(String folderName)
{
   Folder newFolder;

   newFolder.pos = Point2I(200, 200);
   newFolder.headerHeight = 20;
   newFolder.bodyBounds = Point2I(120, 100);
   newFolder.bounds = Point2I(newFolder.bodyBounds.x, newFolder.bodyBounds.y + newFolder.headerHeight);
   newFolder.nodeTitle = folderName == String("") ? "Test" : folderName;

   mFolders.push_back(newFolder);
}

S32 guiWebGraphCtrl::addConnection(S32 startNodeIdx, S32 endNodeIdx)
{
   connection newConnection;

   newConnection.startNode = startNodeIdx;
   newConnection.endNode = endNodeIdx;
   newConnection.error = false;

   mConnections.push_back(newConnection);

   Con::executef(this, "onNewConnection", mConnections.size() - 1);

   return mConnections.size() - 1;
}

void guiWebGraphCtrl::deleteNode(S32 nodeIdx)
{
   if (nodeIdx >= 0 && mNodes.size() > nodeIdx)
   {
      for (U32 i = 0; i < mConnections.size(); i++)
      {
         if (mConnections[i].endNode == nodeIdx || mConnections[i].startNode == nodeIdx)
         {
            mConnections.erase(i);
            i--;
         }
      }

      mNodes.erase(nodeIdx);
   }
}

RectI guiWebGraphCtrl::getNodeBounds(S32 nodeIdx)
{
   Point2I nodePos = mNodes[nodeIdx].pos + realCenter;
   return RectI(nodePos.x - mNodes[nodeIdx].bodyBounds.x / 2, nodePos.y - mNodes[nodeIdx].bodyBounds.y / 2,
      mNodes[nodeIdx].bodyBounds.x, mNodes[nodeIdx].bodyBounds.y);
}

RectI guiWebGraphCtrl::getFolderBounds(S32 folderIdx)
{
   Point2I folderPos = mFolders[folderIdx].pos + realCenter;

   return RectI(folderPos.x - mFolders[folderIdx].bodyBounds.x / 2, folderPos.y - mFolders[folderIdx].bodyBounds.y / 2,
      mFolders[folderIdx].bodyBounds.x, mFolders[folderIdx].bodyBounds.y);
}

void guiWebGraphCtrl::setFolderExtent(S32 folderIdx, Point2I extent)
{
   mFolders[folderIdx].bodyBounds = extent;
}

void guiWebGraphCtrl::setNodePosition(S32 nodeIdx, Point2I position)
{
   mNodes[nodeIdx].pos = position;
}

Point2I guiWebGraphCtrl::getNodePosition(S32 nodeIdx)
{
   return mNodes[nodeIdx].pos;
}

void guiWebGraphCtrl::setNodeColor(S32 nodeIdx, ColorI color)
{
   mNodes[nodeIdx].overrideColor = color;
}

void guiWebGraphCtrl::useNodeColor(S32 nodeIdx, bool useOverride)
{
   mNodes[nodeIdx].useOverrideColor = useOverride;
}

String guiWebGraphCtrl::getNodeName(S32 nodeIdx)
{
   //mNodes[nodeIdx]. = position;
   return mNodes[nodeIdx].nodeTitle;
}

void guiWebGraphCtrl::setNodeName(S32 nodeIdx, String newName)
{
   //mNodes[nodeIdx]. = position;
   mNodes[nodeIdx].nodeTitle = newName;
}

String guiWebGraphCtrl::getNodeField(S32 nodeIdx, String fieldName)
{
   if (mNodes.size() > nodeIdx && nodeIdx >= 0)
   {
      for (U32 i = 0; i < mNodes[nodeIdx].fields.size(); i++)
      {
         if(mNodes[nodeIdx].fields[i].fieldName == fieldName)
            return mNodes[nodeIdx].fields[i].fieldValue;
      }
   }

   return "";
}

S32 guiWebGraphCtrl::getNodeFieldCount(S32 nodeIdx)
{
   if (mNodes.size() > nodeIdx && nodeIdx >= 0)
      return mNodes[nodeIdx].fields.size();
   else
      return 0;
}

String guiWebGraphCtrl::getNodeFieldName(S32 nodeIdx, S32 fieldIdx)
{
   if (mNodes.size() > nodeIdx && nodeIdx >= 0 && mNodes[nodeIdx].fields.size() > fieldIdx && fieldIdx >= 0)
   {
      return mNodes[nodeIdx].fields[fieldIdx].fieldName;
   }

   return "";
}

void guiWebGraphCtrl::setNodeField(S32 nodeIdx, String fieldName, String value)
{
   if (mNodes.size() > nodeIdx && nodeIdx >= 0)
   {
      for (U32 i = 0; i < mNodes[nodeIdx].fields.size(); i++)
      {
         if (mNodes[nodeIdx].fields[i].fieldName == fieldName)
         {
            mNodes[nodeIdx].fields[i].fieldValue = value;
            return;
         }
      }

      Node::field newField;

      newField.fieldName = fieldName;
      newField.fieldValue = value;

      mNodes[nodeIdx].fields.push_back(newField);
   }
}

S32 guiWebGraphCtrl::getConnectionOwnerNode(S32 connIdx)
{
   return mConnections[connIdx].startNode;
}

S32 guiWebGraphCtrl::getConnectionEndNode(S32 connIdx)
{
   return mConnections[connIdx].endNode;
}

DefineEngineMethod(guiWebGraphCtrl, addNode, S32, (String nodeName), (""),
   "Set the pattern by which to filter items in the tree.  Only items in the tree whose text "
   "matches this pattern are displayed.\n\n"
   "@param pattern New pattern based on which visible items in the tree should be filtered.  If empty, all items become visible.\n\n"
   "@see getFilterText\n"
   "@see clearFilterText")
{
   return object->addNode(nodeName);
}

DefineEngineMethod(guiWebGraphCtrl, addConnection, S32, (S32 startNodeIdx, S32 endNodeIdx), ,
   "Set the pattern by which to filter items in the tree.  Only items in the tree whose text "
   "matches this pattern are displayed.\n\n"
   "@param pattern New pattern based on which visible items in the tree should be filtered.  If empty, all items become visible.\n\n"
   "@see getFilterText\n"
   "@see clearFilterText")
{
   return object->addConnection(startNodeIdx, endNodeIdx);
}

DefineEngineMethod(guiWebGraphCtrl, createConnection, void, (int startNodeIdx), ,
   "Set the pattern by which to filter items in the tree.  Only items in the tree whose text "
   "matches this pattern are displayed.\n\n"
   "@param pattern New pattern based on which visible items in the tree should be filtered.  If empty, all items become visible.\n\n"
   "@see getFilterText\n"
   "@see clearFilterText")
{
   //object->addSocketToNode(nodeIdx, SocketType, socketText);
   object->startConnection(startNodeIdx);
   return;
}

DefineEngineMethod(guiWebGraphCtrl, addFolder, void, (String folderName), (""),
   "Set the pattern by which to filter items in the tree.  Only items in the tree whose text "
   "matches this pattern are displayed.\n\n"
   "@param pattern New pattern based on which visible items in the tree should be filtered.  If empty, all items become visible.\n\n"
   "@see getFilterText\n"
   "@see clearFilterText")
{
   object->addFolder(folderName);
}

DefineEngineMethod(guiWebGraphCtrl, setFolderExtent, void, (int folderIdx, Point2I extent), ,
   "Set the pattern by which to filter items in the tree.  Only items in the tree whose text "
   "matches this pattern are displayed.\n\n"
   "@param pattern New pattern based on which visible items in the tree should be filtered.  If empty, all items become visible.\n\n"
   "@see getFilterText\n"
   "@see clearFilterText")
{
   object->setFolderExtent(folderIdx, extent);
}

DefineEngineMethod(guiWebGraphCtrl, setNodeError, void, (int nodeIdx, bool error), (true),
   "Set the pattern by which to filter items in the tree.  Only items in the tree whose text "
   "matches this pattern are displayed.\n\n"
   "@param pattern New pattern based on which visible items in the tree should be filtered.  If empty, all items become visible.\n\n"
   "@see getFilterText\n"
   "@see clearFilterText")
{
   object->setNodeError(nodeIdx, error);
}

DefineEngineMethod(guiWebGraphCtrl, setNodePosition, void, (int nodeIdx, Point2I position),,
   "Set the pattern by which to filter items in the tree.  Only items in the tree whose text "
   "matches this pattern are displayed.\n\n"
   "@param pattern New pattern based on which visible items in the tree should be filtered.  If empty, all items become visible.\n\n"
   "@see getFilterText\n"
   "@see clearFilterText")
{
   object->setNodePosition(nodeIdx, position);
}

DefineEngineMethod(guiWebGraphCtrl, getNodePosition, Point2I, (int nodeIdx), ,
   "Set the pattern by which to filter items in the tree.  Only items in the tree whose text "
   "matches this pattern are displayed.\n\n"
   "@param pattern New pattern based on which visible items in the tree should be filtered.  If empty, all items become visible.\n\n"
   "@see getFilterText\n"
   "@see clearFilterText")
{
   return object->getNodePosition(nodeIdx);
}

DefineEngineMethod(guiWebGraphCtrl, getNodeCount, S32, (), ,
   "Set the pattern by which to filter items in the tree.  Only items in the tree whose text "
   "matches this pattern are displayed.\n\n"
   "@param pattern New pattern based on which visible items in the tree should be filtered.  If empty, all items become visible.\n\n"
   "@see getFilterText\n"
   "@see clearFilterText")
{
   return object->getNodeCount();
}

DefineEngineMethod(guiWebGraphCtrl, getConnectionCount, S32, (), ,
   "Set the pattern by which to filter items in the tree.  Only items in the tree whose text "
   "matches this pattern are displayed.\n\n"
   "@param pattern New pattern based on which visible items in the tree should be filtered.  If empty, all items become visible.\n\n"
   "@see getFilterText\n"
   "@see clearFilterText")
{
   return object->getConnectionCount();
}

DefineEngineMethod(guiWebGraphCtrl, getConnectionOwnerNode, S32, (S32 connectionIdx), ,
   "Set the pattern by which to filter items in the tree.  Only items in the tree whose text "
   "matches this pattern are displayed.\n\n"
   "@param pattern New pattern based on which visible items in the tree should be filtered.  If empty, all items become visible.\n\n"
   "@see getFilterText\n"
   "@see clearFilterText")
{
   return object->getConnectionOwnerNode(connectionIdx);
}

DefineEngineMethod(guiWebGraphCtrl, getConnectionEndNode, S32, (S32 connectionIdx), ,
   "Set the pattern by which to filter items in the tree.  Only items in the tree whose text "
   "matches this pattern are displayed.\n\n"
   "@param pattern New pattern based on which visible items in the tree should be filtered.  If empty, all items become visible.\n\n"
   "@see getFilterText\n"
   "@see clearFilterText")
{
   return object->getConnectionEndNode(connectionIdx);
}

DefineEngineMethod(guiWebGraphCtrl, setNodeColor, void, (S32 nodeIdx, ColorI color), ,
   "Set the pattern by which to filter items in the tree.  Only items in the tree whose text "
   "matches this pattern are displayed.\n\n"
   "@param pattern New pattern based on which visible items in the tree should be filtered.  If empty, all items become visible.\n\n"
   "@see getFilterText\n"
   "@see clearFilterText")
{
   object->setNodeColor(nodeIdx, color);
}

DefineEngineMethod(guiWebGraphCtrl, useNodeColor, void, (S32 nodeIdx, bool useOverride), ,
   "Set the pattern by which to filter items in the tree.  Only items in the tree whose text "
   "matches this pattern are displayed.\n\n"
   "@param pattern New pattern based on which visible items in the tree should be filtered.  If empty, all items become visible.\n\n"
   "@see getFilterText\n"
   "@see clearFilterText")
{
   object->useNodeColor(nodeIdx, useOverride);
}

DefineEngineMethod(guiWebGraphCtrl, getNodeName, String, (S32 nodeIdx), ,
   "Set the pattern by which to filter items in the tree.  Only items in the tree whose text "
   "matches this pattern are displayed.\n\n"
   "@param pattern New pattern based on which visible items in the tree should be filtered.  If empty, all items become visible.\n\n"
   "@see getFilterText\n"
   "@see clearFilterText")
{
   return object->getNodeName(nodeIdx);
}

DefineEngineMethod(guiWebGraphCtrl, setNodeName, void, (S32 nodeIdx, String newName), ,
   "Set the pattern by which to filter items in the tree.  Only items in the tree whose text "
   "matches this pattern are displayed.\n\n"
   "@param pattern New pattern based on which visible items in the tree should be filtered.  If empty, all items become visible.\n\n"
   "@see getFilterText\n"
   "@see clearFilterText")
{
   object->setNodeName(nodeIdx, newName);
}

DefineEngineMethod(guiWebGraphCtrl, setNodeField, void, (S32 nodeIdx, String fieldName, String value), ,
   "Set the pattern by which to filter items in the tree.  Only items in the tree whose text "
   "matches this pattern are displayed.\n\n"
   "@param pattern New pattern based on which visible items in the tree should be filtered.  If empty, all items become visible.\n\n"
   "@see getFilterText\n"
   "@see clearFilterText")
{
   object->setNodeField(nodeIdx, fieldName, value);
}

DefineEngineMethod(guiWebGraphCtrl, getNodeField, String, (S32 nodeIdx, String fieldName), ,
   "Set the pattern by which to filter items in the tree.  Only items in the tree whose text "
   "matches this pattern are displayed.\n\n"
   "@param pattern New pattern based on which visible items in the tree should be filtered.  If empty, all items become visible.\n\n"
   "@see getFilterText\n"
   "@see clearFilterText")
{
   return object->getNodeField(nodeIdx, fieldName);
}

DefineEngineMethod(guiWebGraphCtrl, getNodeFieldName, String, (S32 nodeIdx, S32 fieldIdx), ,
   "Set the pattern by which to filter items in the tree.  Only items in the tree whose text "
   "matches this pattern are displayed.\n\n"
   "@param pattern New pattern based on which visible items in the tree should be filtered.  If empty, all items become visible.\n\n"
   "@see getFilterText\n"
   "@see clearFilterText")
{
   return object->getNodeFieldName(nodeIdx, fieldIdx);
}

DefineEngineMethod(guiWebGraphCtrl, getNodeFieldCount, S32, (S32 nodeIdx), ,
   "Set the pattern by which to filter items in the tree.  Only items in the tree whose text "
   "matches this pattern are displayed.\n\n"
   "@param pattern New pattern based on which visible items in the tree should be filtered.  If empty, all items become visible.\n\n"
   "@see getFilterText\n"
   "@see clearFilterText")
{
   return object->getNodeFieldCount(nodeIdx);
}


DefineEngineMethod(guiWebGraphCtrl, setConnectionError, void, (int connectionIdx, bool error), (true),
   "Set the pattern by which to filter items in the tree.  Only items in the tree whose text "
   "matches this pattern are displayed.\n\n"
   "@param pattern New pattern based on which visible items in the tree should be filtered.  If empty, all items become visible.\n\n"
   "@see getFilterText\n"
   "@see clearFilterText")
{
   object->setConnectionError(connectionIdx, error);
}

DefineEngineMethod(guiWebGraphCtrl, recenterGraph, void, (),,
   "Set the pattern by which to filter items in the tree.  Only items in the tree whose text "
   "matches this pattern are displayed.\n\n"
   "@param pattern New pattern based on which visible items in the tree should be filtered.  If empty, all items become visible.\n\n"
   "@see getFilterText\n"
   "@see clearFilterText")
{
   object->recenterGraph();
}

DefineEngineMethod(guiWebGraphCtrl, clear, void, (), ,
   "Set the pattern by which to filter items in the tree.  Only items in the tree whose text "
   "matches this pattern are displayed.\n\n"
   "@param pattern New pattern based on which visible items in the tree should be filtered.  If empty, all items become visible.\n\n"
   "@see getFilterText\n"
   "@see clearFilterText")
{
   object->clearGraph();
}

DefineEngineMethod(guiWebGraphCtrl, getGraphCenter, Point2I, (), ,
   "Set the pattern by which to filter items in the tree.  Only items in the tree whose text "
   "matches this pattern are displayed.\n\n"
   "@param pattern New pattern based on which visible items in the tree should be filtered.  If empty, all items become visible.\n\n"
   "@see getFilterText\n"
   "@see clearFilterText")
{
   return object->getGraphCenter();
}