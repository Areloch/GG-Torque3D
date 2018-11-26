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

#ifndef _GUI_WEB_GRAPH_CTRL_H_
#define _GUI_WEB_GRAPH_CTRL_H_

#ifndef _GUIBUTTONBASECTRL_H_
#include "gui/buttons/guiButtonBaseCtrl.h"
#endif

class guiWebGraphCtrl : public GuiButtonBaseCtrl
{
   typedef GuiButtonBaseCtrl Parent;

   Resource<GFont> mFont;

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

      Point2I bodyBounds;

      Point2I bounds;

      bool error;

      ColorI overrideColor;
      bool useOverrideColor;

      struct field
      {
         String fieldName;
         String fieldValue;
      };

      Vector<field> fields;

      Node() : overrideColor(ColorI::BLACK), useOverrideColor(false), error(false) {}
   };

   struct connection
   {
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

   S32 selectedConnection;

   S32 newConnectionStartNode;
   Point2I newConnectionStartPoint;

   Point2I mMouseDownPosition;
   Point2I mLastDragPosition;
   Point2I mMousePosition;
   Point2I mDeltaMousePosition;

   Point2I nodeMoveOffset;

   F32 viewScalar;

   Point2I center;
   Point2I realCenter;

   ColorI baseNodeColor;
   ColorI baseNodeOutlineColor;
   ColorI baseConnectionColor;
   ColorI errorColor;
   ColorI selectColor;
   //

public:
   DECLARE_CONOBJECT(guiWebGraphCtrl);
   guiWebGraphCtrl();
   bool onWake();
   void onRender(Point2I offset, const RectI &updateRect);

   //
   virtual void onMouseDown(const GuiEvent &event);
   virtual void onMouseDragged(const GuiEvent &event);
   virtual void onMouseUp(const GuiEvent &event);
   virtual void onMouseMove(const GuiEvent &event);

   virtual void onRightMouseDown(const GuiEvent &event);

   bool onMouseWheelUp(const GuiEvent &event);
   bool onMouseWheelDown(const GuiEvent &event);
   void onMiddleMouseDown(const GuiEvent &event);
   void onMiddleMouseDragged(const GuiEvent &event);
   void onMiddleMouseUp(const GuiEvent &event);

   virtual bool onKeyDown(const GuiEvent& event);
   virtual bool onKeyUp(const GuiEvent& event);
   //

   void addFolder(String nodeName);
   S32 addNode(String nodeName);
   S32 addConnection(S32 startNodeIdx, S32 endNodeIdx);

   void deleteNode(S32 nodeIdx);

   RectI getNodeBounds(S32 nodeIdx);
   RectI getFolderBounds(S32 folderIdx);

   void setFolderExtent(S32 folderIdx, Point2I extent);

   //
   void setNodePosition(S32 nodeIdx, Point2I position);
   Point2I getNodePosition(S32 nodeIdx);
   void setNodeColor(S32 nodeIdx, ColorI color);
   void useNodeColor(S32 nodeIdx, bool useOverride);

   S32 getNodeCount() { return mNodes.size(); }
   S32 getConnectionCount() { return mConnections.size(); }

   S32 getConnectionOwnerNode(S32 connIdx);
   S32 getConnectionEndNode(S32 connIdx);

   String getNodeName(S32 nodeIdx);
   void setNodeName(S32 nodeIdx, String newName);

   String getNodeField(S32 nodeIdx, String fieldName);
   void setNodeField(S32 nodeIdx, String fieldName, String value);

   String getNodeFieldName(S32 nodeIdx, S32 fieldIdx);
   S32 getNodeFieldCount(S32 nodeIdx);
   //

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

   void startConnection(S32 startNodeIdx)
   {
      if (startNodeIdx < mNodes.size())
      {
         newConnectionStartNode = startNodeIdx;
         mDraggingConnection = true;
      }
   }

   void recenterGraph()
   {
      center = Point2I::Zero;
   }

   void clearGraph()
   {
      mNodes.clear();
      mConnections.clear();
      mFolders.clear();
      recenterGraph();
   }

   Point2I getGraphCenter()
   {
      return realCenter;
   }
};

#endif //_GUI_BUTTON_CTRL_H