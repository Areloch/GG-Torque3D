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
#include "gui/controls/guiNodeGraphCtrl.h"

#include "console/console.h"
#include "console/consoleTypes.h"
#include "gfx/gfxDevice.h"
#include "gfx/gfxDrawUtil.h"
#include "gui/core/guiCanvas.h"
#include "gui/core/guiDefaultControlRender.h"

#include "gfx/gfxDrawUtil.h"
#include "console/engineAPI.h"

#include "gfx/primBuilder.h"

IMPLEMENT_CONOBJECT(guiNodeGraphCtrl);

ConsoleDocClass( guiNodeGraphCtrl,
   "@brief The most widely used button class.\n\n"
   
   "guiNodeGraphCtrl renders seperately of, but utilizes all of the functionality of GuiBaseButtonCtrl.\n" 
   "This grants guiNodeGraphCtrl the versatility to be either of the 3 button types.\n\n"

   "@tsexample\n"
   "// Create a PushButton guiNodeGraphCtrl that calls randomFunction when clicked\n"
   "%button = new guiNodeGraphCtrl()\n"
   "{\n"
   "   profile    = \"GuiButtonProfile\";\n"
   "   buttonType = \"PushButton\";\n"
   "   command    = \"randomFunction();\";\n"
   "};\n"
   "@endtsexample\n\n"
   
   "@ingroup GuiButtons"
);


//-----------------------------------------------------------------------------

guiNodeGraphCtrl::guiNodeGraphCtrl()
{
   String fontCacheDir = Con::getVariable("$GUI::fontCacheDirectory");
   mFont = GFont::create("Arial", 12, fontCacheDir);

   socketPaddingDistance = 5;
   mDraggingConnection = false;
   mDraggingSelect = false;

   viewScalar = 1;
   zoomSteps = 12;
   curZoomStep = 1;
   scalarStep = 1.0f / zoomSteps;
}

//-----------------------------------------------------------------------------

bool guiNodeGraphCtrl::onWake()
{
   if( !Parent::onWake() )
      return false;

   return true;
}

//-----------------------------------------------------------------------------

void guiNodeGraphCtrl::onRender(Point2I      offset,
                             const RectI& updateRect)
{
   GFXDrawUtil *drawer = GFX->getDrawUtil();

   //first our background
   drawer->drawRectFill(updateRect.point, updateRect.point + updateRect.extent, ColorI(42, 42, 42));

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

      Point2F headerPos = Point2F(minPos.x + folderBounds.extent.x / 2 - strWidth / 2, minPos.y + folderBounds.extent.y / 2 + 6); //6 is half the text height

      drawer->setBitmapModulation(ColorI(255, 255, 255));
      drawer->drawText(mFont, headerPos, mFolders[i].nodeTitle.c_str());
      drawer->clearBitmapModulation();

      //body
      minPos = Point2F(folderBounds.point.x, folderBounds.point.y);
      maxPos = Point2F(folderBounds.point.x + folderBounds.extent.x, folderBounds.point.y + folderBounds.extent.y);

      drawer->drawRectFill(minPos, maxPos, ColorI(106, 106, 106));
   }

   for(U32 i=0; i < mConnections.size(); i++)
   {
      //we're connecting path sockets
      if(mConnections[i].startSocket == -2)
      {
         Point2I startPoint = getSocketCenter(mConnections[i].startNode, OUT_PATH, 0);
         Point2I endPoint = getSocketCenter(mConnections[i].endNode, IN_PATH, 0);

         drawer->drawLine(startPoint, endPoint, ColorI(255, 255, 255));
      }
      else
      {
         Point2I startPoint = getSocketCenter(mConnections[i].startNode, OUT_ARG, mConnections[i].startSocket);
         Point2I endPoint = getSocketCenter(mConnections[i].endNode, IN_ARG, mConnections[i].endSocket);

         if (!mConnections[i].error)
            drawer->drawLine(startPoint, endPoint, ColorI(255, 255, 0));
         else
            drawer->drawLine(startPoint, endPoint, ColorI(255, 0, 0));
      }
   }

   if(mDraggingConnection)
   {
      //we're connecting a path socket
      if(newConnectionStartSocket == -2)
      {
         Point2I start = getSocketCenter(newConnectionStartNode, OUT_PATH, 0);
         drawer->drawLine(start, mMousePosition, ColorI(255,255,255));
      }
      else
      {
         Point2I start = getSocketCenter(newConnectionStartNode, OUT_ARG, newConnectionStartSocket);
         drawer->drawLine(start, mMousePosition, ColorI(255,255,0));
      }
   }

   for(U32 i=0; i < mNodes.size(); i++)
   {
      Point2I nodePos = mNodes[i].pos;// +offset;

      RectI nodeBounds = getNodeBounds(i);

      S32 headerHeight = mNodes[i].headerHeight;

      headerHeight *= viewScalar;

      //Draw the outline
      if (!mSelectedNodes.empty() && mSelectedNodes.find_next(i, 0) != -1)
      {
         drawer->drawRect(Point2F(nodeBounds.point.x - 1, nodeBounds.point.y - headerHeight - 1),
            Point2F(nodeBounds.point.x + nodeBounds.extent.x + 1, nodeBounds.point.y + nodeBounds.extent.y + 1), ColorI(228, 164, 65));
      }
      else
      {
         drawer->drawRect(Point2F(nodeBounds.point.x - 1, nodeBounds.point.y - headerHeight - 1),
            Point2F(nodeBounds.point.x + nodeBounds.extent.x + 1, nodeBounds.point.y + nodeBounds.extent.y + 1), ColorI(0, 0, 0));
      }

      Point2F minPos = Point2F(nodeBounds.point.x, nodeBounds.point.y - headerHeight);
      Point2F maxPos = Point2F(nodeBounds.point.x + nodeBounds.extent.x, 
         nodeBounds.point.y );

      //next, the headbar
      if (!mNodes[i].error)
         drawer->drawRectFill(minPos, maxPos, ColorI(96,128,92));
      else
         drawer->drawRectFill(minPos, maxPos, ColorI(255, 0, 0));

      //header text
      if(curZoomStep < zoomSteps / 2)
      {
         U32 strWidth = mFont->getStrWidth(mNodes[i].nodeTitle.c_str());

         Point2F headerPos = Point2F(minPos.x + nodeBounds.extent.x / 2 - strWidth / 2, minPos.y + nodeBounds.extent.y / 2 - 6); //6 is half the text height

         drawer->setBitmapModulation(ColorI(255,255,255)); 
         drawer->drawText(mFont, headerPos, mNodes[i].nodeTitle.c_str());
         drawer->clearBitmapModulation();
      }

      //body
      minPos = Point2F(nodeBounds.point.x, nodeBounds.point.y);
      maxPos = Point2F(nodeBounds.point.x + nodeBounds.extent.x, 
         nodeBounds.point.y + nodeBounds.extent.y);

      drawer->drawRectFill(minPos, maxPos, ColorI(34,36,34));

      if(mNodes[i].mHasInPath)
      {
         RectI socketBnds = getSocketBounds(i, IN_PATH, 0);
         //Point2I socketTextPos = getSocketTextStart(i, IN_ARG, s);

         Point2F minPos = Point2F(socketBnds.point.x, socketBnds.point.y);
         Point2F maxPos = minPos + Point2F(socketBnds.extent.x, socketBnds.extent.y);

         //find out if we are connected
         bool found = false;
         for (U32 c = 0; c < mConnections.size(); c++)
         {
            if (mConnections[c].endNode == i && mConnections[c].endSocket == -2)
            {
               found = true;
               break;
            }
         }

         if (found)
            drawer->drawRectFill(minPos, maxPos, ColorI(255, 255, 255));
         else
            drawer->drawRect(minPos, maxPos, ColorI(255, 255, 255));

         /*drawer->setBitmapModulation(ColorI(255, 255, 255));
         drawer->drawText(mFont, socketTextPos, mNodes[i].mInSockets[s].text.c_str());
         drawer->clearBitmapModulation();*/
      }

      if(mNodes[i].mHasOutPath)
      {
         RectI socketBnds = getSocketBounds(i, OUT_PATH, 0);

         Point2F minPos = Point2F(socketBnds.point.x, socketBnds.point.y);
         Point2F maxPos = minPos + Point2F(socketBnds.extent.x, socketBnds.extent.y);

         //drawer->drawRectFill(minPos, maxPos, ColorI(255, 255, 255));

         //can early out if we're still dragging one
         bool found = false;
         if (mDraggingConnection && newConnectionStartNode == i && newConnectionStartSocket == -2)
         {
            found = true;
         }
         else
         {
            //find out if we are connected
            for (U32 c = 0; c < mConnections.size(); c++)
            {
               if (mConnections[c].startNode == i && mConnections[c].startSocket == -2)
               {
                  found = true;
                  break;
               }
            }
         }

         if (found)
            drawer->drawRectFill(minPos, maxPos, ColorI(255, 255, 255));
         else
            drawer->drawRect(minPos, maxPos, ColorI(255, 255, 255));
      }

      //draw sockets
      for(U32 s=0; s < mNodes[i].mInSockets.size(); s++)
      {
         RectI socketBnds = getSocketBounds(i, IN_ARG, s);
         Point2I socketTextPos = getSocketTextStart(i, IN_ARG, s);

         Point2F minPos = Point2F(socketBnds.point.x, socketBnds.point.y);
         Point2F maxPos = minPos + Point2F(socketBnds.extent.x, socketBnds.extent.y);

         bool found = false;
         for (U32 c = 0; c < mConnections.size(); c++)
         {
            if (mConnections[c].endNode == i && mConnections[c].endSocket == s)
            {
               found = true;
               break;
            }
         }

         if (found)
         {
            if (!mNodes[i].mInSockets[s].error)
               drawer->drawRectFill(minPos, maxPos, ColorI(0, 255, 0));
            else
               drawer->drawRectFill(minPos, maxPos, ColorI(255, 0, 0));
         }
         else
         {
            if (!mNodes[i].mInSockets[s].error)
               drawer->drawRect(minPos, maxPos, ColorI(0, 255, 0));
            else
               drawer->drawRect(minPos, maxPos, ColorI(255, 0, 0));
         }

         //header text
         if(curZoomStep < zoomSteps / 2)
         {
            drawer->setBitmapModulation(ColorI(255, 255, 255));
            drawer->drawText(mFont, socketTextPos, mNodes[i].mInSockets[s].text.c_str());
            drawer->clearBitmapModulation();
         }
      }

      for (U32 s = 0; s < mNodes[i].mOutSockets.size(); s++)
      {
         RectI socketBnds = getSocketBounds(i, OUT_ARG, s);
         Point2I socketTextPos = getSocketTextStart(i, OUT_ARG, s);

         Point2F minPos = Point2F(socketBnds.point.x, socketBnds.point.y);
         Point2F maxPos = minPos + Point2F(socketBnds.extent.x, socketBnds.extent.y);

         bool found = false;
         if (mDraggingConnection && newConnectionStartNode == i && newConnectionStartSocket == s)
         {
            found = true;
         }
         else
         {
            //find out if we are connected
            for (U32 c = 0; c < mConnections.size(); c++)
            {
               if (mConnections[c].startNode == i && mConnections[c].startSocket == s)
               {
                  found = true;
                  break;
               }
            }
         }

         if (found)
         {
            if (!mNodes[i].mOutSockets[s].error)
               drawer->drawRectFill(minPos, maxPos, ColorI(255, 255, 0));
            else
               drawer->drawRectFill(minPos, maxPos, ColorI(255, 0, 0));
         }
         else
         {
            if (!mNodes[i].mOutSockets[s].error)
               drawer->drawRect(minPos, maxPos, ColorI(255, 255, 0));
            else
               drawer->drawRect(minPos, maxPos, ColorI(255, 0, 0));
         }

         if(curZoomStep < zoomSteps / 2)
         {   
            drawer->setBitmapModulation(ColorI(255, 255, 255));
            drawer->drawText(mFont, socketTextPos, mNodes[i].mOutSockets[s].text.c_str());
            drawer->clearBitmapModulation();
         }
      }
   }

   GFXStateBlockDesc desc;
   desc.setBlend(true, GFXBlendSrcAlpha, GFXBlendInvSrcAlpha);
   desc.setZReadWrite(false);
   desc.zWriteEnable = false;
   desc.setCullMode(GFXCullNone);
   //mStateBlock = GFX->createStateBlock( desc );

   GFX->setStateBlock(GFX->createStateBlock( desc ));

   //primbuild test
   PrimBuild::begin( GFXTriangleFan, 4 );
	PrimBuild::color( ColorI(255, 255, 255) );
	PrimBuild::vertex2i( 200, 200 );

	PrimBuild::color( ColorI(255, 255, 255) );
	PrimBuild::vertex2i( 400, 200 );

	PrimBuild::color( ColorI(255, 255, 255) );
	PrimBuild::vertex2i( 400, 400 );

	PrimBuild::color( ColorI(255, 255, 255) );
	PrimBuild::vertex2i( 200, 400 );
	PrimBuild::end();
}

//
void guiNodeGraphCtrl::onMouseDown(const GuiEvent &event)
{
   if (!mActive)
      return;

   mMovingNode = false;
   mDraggingConnection = false;
   mMouseDownPosition = event.mousePoint;
   mDeltaMousePosition = mMousePosition;

   mDraggingSelect = false;

   for (U32 i = 0; i < mNodes.size(); i++)
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
   }
}

void guiNodeGraphCtrl::onMouseDragged(const GuiEvent &event)
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
   }

   mLastDragPosition = mMousePosition;
}

void guiNodeGraphCtrl::onMouseUp(const GuiEvent &event)
{
   if (!mActive)
      return;

   //test to see if we were making a connection and, if so, did we finish it
   if(mDraggingConnection)
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
   }

   mDraggingSelect = false;
   mDraggingConnection = false;
   mMovingNode = false;
   newConnectionStartPoint = Point2I(0,0);
}

void guiNodeGraphCtrl::onMouseMove(const GuiEvent &event)
{
   mMousePosition = globalToLocalCoord(event.mousePoint);
}

bool guiNodeGraphCtrl::onMouseWheelUp(const GuiEvent &event)
{
   viewScalar += scalarStep;
   curZoomStep -= 1;
   curZoomStep = mClamp(curZoomStep, 1, zoomSteps);
   viewScalar = mClampF(viewScalar, scalarStep, 1);

   return true;
}

bool guiNodeGraphCtrl::onMouseWheelDown(const GuiEvent &event)
{
   viewScalar -= scalarStep;
   curZoomStep += 1;
   curZoomStep = mClamp(curZoomStep, 1, zoomSteps);
   viewScalar = mClampF(viewScalar, scalarStep, 1);

   return true;
}
void guiNodeGraphCtrl::onMiddleMouseDown(const GuiEvent &event)
{
	mMouseDownPosition = event.mousePoint;
    Point2I localPoint = globalToLocalCoord(event.mousePoint);

    mLastDragPosition = mMousePosition;

	//sendMouseEvent("onMiddleMouseDown", event);
}

void guiNodeGraphCtrl::onMiddleMouseDragged(const GuiEvent &event)
{
	mMousePosition = event.mousePoint;
   Point2I deltaMousePosition = mMousePosition - mLastDragPosition;

   for (U32 i = 0; i < mNodes.size(); i++)
   {
      mNodes[i].pos += deltaMousePosition;
   }

   mLastDragPosition = mMousePosition;
}

void guiNodeGraphCtrl::onMiddleMouseUp(const GuiEvent &event)
{
	/*for(U32 i=0; i < mNodes.size(); i++)
	{
		Point2I nodePos = mNodes[i]->getPosition();
		mNodes[i]->setPosition(nodePos - origin);
	}
	origin = Point2I(0,0);*/
	//sendMouseEvent("onMiddleMouseUp", event);
}

//

void guiNodeGraphCtrl::addNode(String nodeName)
{
   Node newNode;

   newNode.pos = Point2I(200,200);
   newNode.headerHeight = 20;
   newNode.bodyBounds = Point2I(120,100);
   newNode.bounds = Point2I(newNode.bodyBounds.x, newNode.bodyBounds.y + newNode.headerHeight);
   newNode.nodeTitle = nodeName == String("") ? "Test" : nodeName;
   newNode.mHasInPath = false;
   newNode.mHasOutPath = false;
   newNode.error = false;

   mNodes.push_back(newNode);

   if(nodeName == String(""))
   {
      S32 nodeIdx = mNodes.size() - 1;
      addSocketToNode(nodeIdx, IN_ARG, "In");
      addSocketToNode(nodeIdx, OUT_ARG, "Out");
   }
}

void guiNodeGraphCtrl::addFolder(String folderName)
{
   Folder newFolder;

   newFolder.pos = Point2I(200, 200);
   newFolder.headerHeight = 20;
   newFolder.bodyBounds = Point2I(120, 100);
   newFolder.bounds = Point2I(newFolder.bodyBounds.x, newFolder.bodyBounds.y + newFolder.headerHeight);
   newFolder.nodeTitle = folderName == String("") ? "Test" : folderName;

   mFolders.push_back(newFolder);
}

void guiNodeGraphCtrl::addSocketToNode(int nodeIdx, int SocketType, String socketText)
{
   if (nodeIdx > mNodes.size())
      return;

   Node *node = &mNodes[nodeIdx];

   socket newSocket;

   newSocket.type = (socketTypes)(SocketType);
   newSocket.text = socketText;
   newSocket.buttonBounds = Point2I(10, 10);
   newSocket.bounds = Point2I(10 + mFont->getStrWidth(socketText), 10);
   newSocket.error = false;

   if(SocketType == IN_ARG)
      node->mInSockets.push_back(newSocket);
   else if(SocketType == OUT_ARG)
      node->mOutSockets.push_back(newSocket);
   else if(SocketType == IN_PATH)
   {
      node->mInPathSocket = newSocket;
      node->mHasInPath = true;
   }
   else if(SocketType == OUT_PATH)
   {
      node->mOutPathSocket = newSocket;
      node->mHasOutPath = true;
   }
}

RectI guiNodeGraphCtrl::getNodeBounds(S32 nodeIdx)
{
   //get our largest horizontal height
   S32 inWidth = 0;
   S32 inHeight = socketPaddingDistance;
   if(mNodes[nodeIdx].mHasInPath)
      inHeight += mNodes[nodeIdx].mInPathSocket.bounds.y + socketPaddingDistance;

   for(U32 i=0; i < mNodes[nodeIdx].mInSockets.size(); i++)
   {
      inHeight += mNodes[nodeIdx].mInSockets[i].bounds.y + socketPaddingDistance;
      inWidth = inWidth > mNodes[nodeIdx].mInSockets[i].bounds.x ? inWidth : mNodes[nodeIdx].mInSockets[i].bounds.x;
   }

   S32 outWidth = 0;
   S32 outHeight = socketPaddingDistance;
   if(mNodes[nodeIdx].mHasOutPath)
      outHeight += mNodes[nodeIdx].mOutPathSocket.bounds.y + socketPaddingDistance;

   for(U32 i=0; i < mNodes[nodeIdx].mOutSockets.size(); i++)
   {
      outHeight += mNodes[nodeIdx].mOutSockets[i].bounds.y + socketPaddingDistance;
      outWidth = inWidth > mNodes[nodeIdx].mOutSockets[i].bounds.x ? inWidth : mNodes[nodeIdx].mOutSockets[i].bounds.x;
   }

   S32 maxHeight = outHeight > inHeight ? outHeight : inHeight;

   maxHeight = maxHeight > 30 ? maxHeight : 30;

   S32 maxWidth = inWidth + outWidth + 20 > 60 ? inWidth + outWidth + 20 : 60; //20 for middle-space padding

   maxHeight *= viewScalar;
   maxWidth *= viewScalar;

   return RectI(mNodes[nodeIdx].pos.x - maxWidth /2, mNodes[nodeIdx].pos.y - maxHeight /2, maxWidth, maxHeight);
}

RectI guiNodeGraphCtrl::getFolderBounds(S32 folderIdx)
{
   //get our largest horizontal height


   return RectI(mFolders[folderIdx].pos.x - mFolders[folderIdx].bodyBounds.x / 2, mFolders[folderIdx].pos.y - mFolders[folderIdx].bodyBounds.y / 2,
      mFolders[folderIdx].bodyBounds.x, mFolders[folderIdx].bodyBounds.y);
}

RectI guiNodeGraphCtrl::getSocketBounds(S32 nodeIdx, S32 socketType, S32 socketIdx)
{
   RectI nodeBounds = getNodeBounds(nodeIdx);
   Point2I nodePos = nodeBounds.point;

   RectI socketBounds = RectI();

   if(socketType == IN_PATH)
   {
      //socket *inSocket = &mNodes[nodeIdx].mInSockets[socketIdx];

      Point2I minPos = Point2I(nodePos.x/* - nodeBounds.x / 2*/, 
         nodePos.y/* - nodeBounds.y / 2*/ + socketPaddingDistance);

      socketBounds = RectI(minPos, Point2I(10, 10));
   }
   else if(socketType == OUT_PATH)
   {
      Point2I minPos = Point2I(nodePos.x + nodeBounds.extent.x - mNodes[nodeIdx].mOutPathSocket.bounds.x/* + nodeBounds.x / 2 - 10*/, 
         nodePos.y/* - nodeBounds.y / 2*/ + socketPaddingDistance);

      return RectI(minPos, Point2I(10, 10));
   }
   else if(socketType == OUT_ARG)
   {
      socket *outSocket = &mNodes[nodeIdx].mOutSockets[socketIdx];

      Point2I minPos;
      if(mNodes[nodeIdx].mHasOutPath)
      {
         minPos = Point2I(nodePos.x + nodeBounds.extent.x - outSocket->buttonBounds.x/* + nodeBounds.x / 2 - 10*/, 
            nodePos.y/* - nodeBounds.y / 2*/ + (socketIdx * outSocket->buttonBounds.y + socketPaddingDistance) + 
            (socketIdx * socketPaddingDistance) + mNodes[nodeIdx].mOutPathSocket.bounds.y + socketPaddingDistance);
      }
      else
      {
         minPos = Point2I(nodePos.x + nodeBounds.extent.x - outSocket->buttonBounds.x/* + nodeBounds.x / 2 - 10*/, 
            nodePos.y/* - nodeBounds.y / 2*/ + (socketIdx * outSocket->buttonBounds.y + socketPaddingDistance) + 
            (socketIdx * socketPaddingDistance));
      }

      socketBounds = RectI(minPos, outSocket->buttonBounds);
   }
   else if(socketType == IN_ARG)
   {
      socket *inSocket = &mNodes[nodeIdx].mInSockets[socketIdx];

      Point2I minPos;
      if(mNodes[nodeIdx].mHasInPath)
      {
         minPos = Point2I(nodePos.x/* - nodeBounds.x / 2 - 10*/, 
            nodePos.y/* - nodeBounds.y / 2*/ + (socketIdx * inSocket->buttonBounds.y + socketPaddingDistance) + 
            (socketIdx * socketPaddingDistance) + mNodes[nodeIdx].mInPathSocket.bounds.y + socketPaddingDistance);
      }
      else
      {
         minPos = Point2I(nodePos.x/* - nodeBounds.x / 2*/, 
         nodePos.y/* - nodeBounds.y / 2*/ + (socketIdx * inSocket->buttonBounds.y + socketPaddingDistance) + 
            (socketIdx * socketPaddingDistance));
      }

      socketBounds = RectI(minPos, inSocket->buttonBounds);
   }

   socketBounds.extent.x *= viewScalar;
   socketBounds.extent.y *= viewScalar;

   return socketBounds;
}

Point2I guiNodeGraphCtrl::getSocketCenter(S32 nodeIdx, S32 socketType, S32 socketIdx)
{
   RectI socketBounds = getSocketBounds(nodeIdx, socketType, socketIdx);

   return Point2I(socketBounds.point.x + socketBounds.extent.x / 2,
                  socketBounds.point.y + socketBounds.extent.y / 2);
}

Point2I guiNodeGraphCtrl::getSocketTextStart(S32 nodeIdx, S32 socketType, S32 socketIdx)
{
   RectI socketBounds = getSocketBounds(nodeIdx, socketType, socketIdx);

   if(socketType == IN_ARG)
   {
      U32 strWidth = mFont->getStrWidth(mNodes[nodeIdx].mInSockets[socketIdx].text.c_str());
      return Point2I(socketBounds.point.x + socketBounds.extent.x + 2, socketBounds.point.y);
   }
   else if(socketType == OUT_ARG)
   {
      U32 strWidth = mFont->getStrWidth(mNodes[nodeIdx].mOutSockets[socketIdx].text.c_str());
      return Point2I(socketBounds.point.x - strWidth - 2, socketBounds.point.y);
   }

   return Point2I(0,0);
}

void guiNodeGraphCtrl::setFolderExtent(S32 folderIdx, Point2I extent)
{
   mFolders[folderIdx].bodyBounds = extent;
}

DefineEngineMethod( guiNodeGraphCtrl, addNode, void, (String nodeName), (""),
   "Set the pattern by which to filter items in the tree.  Only items in the tree whose text "
   "matches this pattern are displayed.\n\n"
   "@param pattern New pattern based on which visible items in the tree should be filtered.  If empty, all items become visible.\n\n"
   "@see getFilterText\n"
   "@see clearFilterText" )
{
   object->addNode(nodeName);
}

DefineEngineMethod( guiNodeGraphCtrl, addSocketToNode, void, (int nodeIdx, int SocketType, String socketText),,
   "Set the pattern by which to filter items in the tree.  Only items in the tree whose text "
   "matches this pattern are displayed.\n\n"
   "@param pattern New pattern based on which visible items in the tree should be filtered.  If empty, all items become visible.\n\n"
   "@see getFilterText\n"
   "@see clearFilterText" )
{
   object->addSocketToNode(nodeIdx, SocketType, socketText);
}

DefineEngineMethod( guiNodeGraphCtrl, addInPathToNode, void, (int nodeIdx),,
   "Set the pattern by which to filter items in the tree.  Only items in the tree whose text "
   "matches this pattern are displayed.\n\n"
   "@param pattern New pattern based on which visible items in the tree should be filtered.  If empty, all items become visible.\n\n"
   "@see getFilterText\n"
   "@see clearFilterText" )
{
   object->addSocketToNode(nodeIdx, 3, "");
}

DefineEngineMethod( guiNodeGraphCtrl, addOutPathToNode, void, (int nodeIdx),,
   "Set the pattern by which to filter items in the tree.  Only items in the tree whose text "
   "matches this pattern are displayed.\n\n"
   "@param pattern New pattern based on which visible items in the tree should be filtered.  If empty, all items become visible.\n\n"
   "@see getFilterText\n"
   "@see clearFilterText" )
{
   object->addSocketToNode(nodeIdx, 4, "");
}

DefineEngineMethod(guiNodeGraphCtrl, addFolder, void, (String folderName), (""),
   "Set the pattern by which to filter items in the tree.  Only items in the tree whose text "
   "matches this pattern are displayed.\n\n"
   "@param pattern New pattern based on which visible items in the tree should be filtered.  If empty, all items become visible.\n\n"
   "@see getFilterText\n"
   "@see clearFilterText")
{
   object->addFolder(folderName);
}

DefineEngineMethod(guiNodeGraphCtrl, setFolderExtent, void, (int folderIdx, Point2I extent), ,
   "Set the pattern by which to filter items in the tree.  Only items in the tree whose text "
   "matches this pattern are displayed.\n\n"
   "@param pattern New pattern based on which visible items in the tree should be filtered.  If empty, all items become visible.\n\n"
   "@see getFilterText\n"
   "@see clearFilterText")
{
   object->setFolderExtent(folderIdx, extent);
}

DefineEngineMethod(guiNodeGraphCtrl, setNodeError, void, (int nodeIdx, bool error), (true),
   "Set the pattern by which to filter items in the tree.  Only items in the tree whose text "
   "matches this pattern are displayed.\n\n"
   "@param pattern New pattern based on which visible items in the tree should be filtered.  If empty, all items become visible.\n\n"
   "@see getFilterText\n"
   "@see clearFilterText")
{
   object->setNodeError(nodeIdx, error);
}

DefineEngineMethod(guiNodeGraphCtrl, setConnectionError, void, (int connectionIdx, bool error), (true),
   "Set the pattern by which to filter items in the tree.  Only items in the tree whose text "
   "matches this pattern are displayed.\n\n"
   "@param pattern New pattern based on which visible items in the tree should be filtered.  If empty, all items become visible.\n\n"
   "@see getFilterText\n"
   "@see clearFilterText")
{
   object->setConnectionError(connectionIdx, error);
}

DefineEngineMethod(guiNodeGraphCtrl, setSocketError, void, (int nodeIdx, int socketType, int socketIdx, bool error), (true),
   "Set the pattern by which to filter items in the tree.  Only items in the tree whose text "
   "matches this pattern are displayed.\n\n"
   "@param pattern New pattern based on which visible items in the tree should be filtered.  If empty, all items become visible.\n\n"
   "@see getFilterText\n"
   "@see clearFilterText")
{
   object->setSocketError(nodeIdx, socketType, socketIdx, error);
}