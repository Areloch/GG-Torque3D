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

class guiNodeGraphNode;

class guiNodeGraphCtrl : public GuiButtonBaseCtrl
{
   typedef GuiButtonBaseCtrl Parent;
   friend class guiNodeGraphNode;

public:
   Resource<GFont> mFont;

   bool webGraphMode;

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

   struct connection
   {
      S32 startSocket;
      S32 endSocket;

      S32 startNode;
      S32 endNode;

      bool error;
   };

protected:
   Point2I mCenterOffset;

   Vector<Folder> mFolders;
   Vector<guiNodeGraphNode*> mNodes;
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

   Point2I center;
   Point2I realCenter;
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

   void onRightMouseDown(const GuiEvent &);
   //

   void addFolder(String nodeName);
   S32 addNode(String nodeName);
   void addSocketToNode(S32 nodeIdx, S32 SocketType, String socketText);
   S32 addConnection(S32 startNode, S32 startSocket, S32 endNode, S32 endSocket);

   void clearNodes() { mNodes.clear(); }

   RectI getNodeBounds(S32 nodeIdx);
   RectI getFolderBounds(S32 folderIdx);

   RectI getSocketBounds(S32 nodeIdx, S32 socketType, S32 SocketIdx);
   Point2I getSocketCenter(S32 nodeIdx, S32 socketType, S32 socketIdx);
   Point2I getSocketTextStart(S32 nodeIdx, S32 socketType, S32 socketIdx);

   void setFolderExtent(S32 folderIdx, Point2I extent);

   void setNodePosition(S32 nodeIdx, Point2I pos);
   void setNodeReferenceFile(S32 nodeIdx, String refFilePath);

   void setNodeError(S32 nodeIdx, bool error);
   void setConnectionError(S32 connectionIdx, bool error);
   void setSocketError(S32 nodeIdx, S32 socketType, S32 socketIdx, bool error);

   void recenterGraph()
   {
      center = Point2I::Zero;
   }

   Point2I getGraphCenter()
   {
      return realCenter;
   }

   inline F32 getBezierPoint(F32 n1, F32 n2, float perc)
   {
      F32 diff = n2 - n1;

      return n1 + (diff * perc);
   }

   Vector<Point2F> getQuadradicBezierCurve(Point2F p1, Point2F p2, Point2F p3, F32 stepCount);

   Vector<Point2F> getCubicBezierCurve(Point2F p1, Point2F p2, Point2F p3, Point2F p4, F32 stepCount);
};

class guiNodeGraphNode : public GuiControl
{
   typedef GuiControl Parent;
   friend class guiNodeGraphCtrl;

   guiNodeGraphCtrl* mOwner;

   Point2I pos;
   String nodeTitle;

   U32 headerHeight;
   Point2I bodyBounds;

   U32 nodeIdx;

   //This is for any child controls that are embedded into the node, such as a preview image, etc
   //Any child controls will be encapsulated into this space
   Point2I bodyContentBounds;

   Point2I bounds;

   //
   bool mHasInPath;
   bool mHasOutPath;

   guiNodeGraphCtrl::socket mInPathSocket;
   guiNodeGraphCtrl::socket mOutPathSocket;

   Vector<guiNodeGraphCtrl::socket>  mInSockets;
   Vector<guiNodeGraphCtrl::socket>  mOutSockets;

   String referenceFile;

   bool error;

public:
   DECLARE_CONOBJECT(guiNodeGraphNode);
   guiNodeGraphNode();

   static void initPersistFields();
   bool onWake();
   void onRender(Point2I offset, const RectI &updateRect);

   //
   /*virtual void onMouseDown(const GuiEvent &event);
   virtual void onMouseDragged(const GuiEvent &event);
   virtual void onMouseUp(const GuiEvent &event);
   virtual void onMouseMove(const GuiEvent &event);

   virtual bool onMouseWheelUp(const GuiEvent &event);
   virtual bool onMouseWheelDown(const GuiEvent &event);

   virtual void onMiddleMouseDown(const GuiEvent &event);
   virtual void onMiddleMouseDragged(const GuiEvent &event);
   virtual void onMiddleMouseUp(const GuiEvent &event);

   void onRightMouseDown(const GuiEvent &);*/

   RectI getNodeBounds();
   RectI getSocketBounds(S32 socketType, S32 SocketIdx);
   Point2I getSocketCenter(S32 socketType, S32 socketIdx);
   Point2I getSocketTextStart(S32 socketType, S32 socketIdx);

   void addSocketToNode(int SocketType, String socketText);
   void setNodeReferenceFile(String refFilePath);
   void setNodeError(bool error);
   void setSocketError(S32 socketType, S32 socketIdx, bool error);
};

#endif //_GUI_BUTTON_CTRL_H