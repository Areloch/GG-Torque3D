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

#ifndef _GUI_NODE_GRAPH_CTRL_H_
#define _GUI_NODE_GRAPH_CTRL_H_

#ifndef _GUIBUTTONBASECTRL_H_
#include "gui/buttons/guiButtonBaseCtrl.h"
#endif

class guiNodeGraphCtrl : public GuiButtonBaseCtrl
{
   typedef GuiButtonBaseCtrl Parent;

   Resource<GFont> mFont;

   enum socketTypes
   {
      IN_ARG = 0,
      OUT_ARG,
      PARAM,
      IN_PATH,
      OUT_PATH
   };

   struct socket
   {
      socketTypes type;
      String text;

      Point2I bounds;
      Point2I buttonBounds;

      bool error;
   };

   struct Folder
   {
      Point2I pos;
      String nodeTitle;

      U32 headerHeight;
      Point2I bodyBounds;

      Point2I bounds;
   };

   struct Node
   {
      Point2I pos;
      String nodeTitle;

      U32 headerHeight;
      Point2I bodyBounds;

      Point2I bounds;

      //
      bool mHasInPath;
      bool mHasOutPath;

      socket mInPathSocket;
      socket mOutPathSocket;

      Vector<socket>  mInSockets;
      Vector<socket>  mOutSockets;

      bool error;
   };

   struct connection
   {
      S32 startSocket;
      S32 endSocket;

      S32 startNode;
      S32 endNode;

      bool error;
   };

   Point2I mCenterOffset;

   Vector<Folder> mFolders;
   Vector<Node> mNodes;
   Vector<connection> mConnections;

   Vector<S32> mSelectedNodes;
   bool mDraggingSelect;
   Point2I selectBoxStart;

   bool mMovingNode;
   bool mDraggingConnection;

   S32 newConnectionStartNode;
   S32 newConnectionStartSocket;
   Point2I newConnectionStartPoint;

   Point2I mMouseDownPosition;
   Point2I mLastDragPosition;
   Point2I mMousePosition;
   Point2I mDeltaMousePosition;

   Point2I nodeMoveOffset;

   S32 socketPaddingDistance;

   F32 viewScalar;

   S32 zoomSteps;
   S32 curZoomStep;
   F32 scalarStep;
   //

public:
   DECLARE_CONOBJECT(guiNodeGraphCtrl);
   guiNodeGraphCtrl();
   bool onWake();
   void onRender(Point2I offset, const RectI &updateRect);

   //
   virtual void onMouseDown(const GuiEvent &event);
   virtual void onMouseDragged(const GuiEvent &event);
   virtual void onMouseUp(const GuiEvent &event);
   virtual void onMouseMove(const GuiEvent &event);

   virtual bool onMouseWheelUp(const GuiEvent &event);
   virtual bool onMouseWheelDown(const GuiEvent &event);

   virtual void onMiddleMouseDown(const GuiEvent &event);
   virtual void onMiddleMouseDragged(const GuiEvent &event);
   virtual void onMiddleMouseUp(const GuiEvent &event);
   //

   void addFolder(String nodeName);
   void addNode(String nodeName);
   void addSocketToNode(S32 nodeIdx, S32 SocketType, String socketText);

   RectI getNodeBounds(S32 nodeIdx);
   RectI getFolderBounds(S32 folderIdx);

   RectI getSocketBounds(S32 nodeIdx, S32 socketType, S32 SocketIdx);
   Point2I getSocketCenter(S32 nodeIdx, S32 socketType, S32 socketIdx);
   Point2I getSocketTextStart(S32 nodeIdx, S32 socketType, S32 socketIdx);

   void setFolderExtent(S32 folderIdx, Point2I extent);

   void setNodeError(S32 nodeIdx, bool error)
   {
      if (nodeIdx < mNodes.size())
      {
         mNodes[nodeIdx].error = error;
      }
   }
   void setConnectionError(S32 connectionIdx, bool error)
   {
      if (connectionIdx < mConnections.size())
      {
         mConnections[connectionIdx].error = error;
      }
   }
   void setSocketError(S32 nodeIdx, S32 socketType, S32 socketIdx, bool error)
   {
      if (nodeIdx < mNodes.size())
      {
         if (socketType == IN_ARG && socketIdx < mNodes[nodeIdx].mInSockets.size())
            mNodes[nodeIdx].mInSockets[socketIdx].error = error;
         else if (socketType == OUT_ARG && socketIdx < mNodes[nodeIdx].mOutSockets.size())
            mNodes[nodeIdx].mOutSockets[socketIdx].error = error;
      }
   }
};

#endif //_GUI_BUTTON_CTRL_H
