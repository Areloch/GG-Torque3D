#ifndef _LEVEL_TILE_BEHAVIOR_TOOL_H_
#define _LEVEL_TILE_BEHAVIOR_TOOL_H_

#ifndef _BASE_TOOL_H_
	#include "component/components/baseTool.h"
#endif
#ifndef _ENTITY_H_
	#include "T3D/entity.h"
#endif
#ifndef _UNDO_H_
#include "util/undo.h"
#endif
#ifndef _GIZMO_H_
#include "gui/worldEditor/gizmo.h"
#endif
#ifndef _LEVEL_TILE_COMPONENT_H_
	#include "component/components/game/LevelTileComponent.h"
#endif
#ifndef _GUI_TABBOOKCTRL_H_
   #include "gui/containers/guiTabBookCtrl.h"
#endif
#ifndef _GUITABPAGECTRL_H_
   #include "gui/controls/guiTabPageCtrl.h"
#endif
#ifndef _GUISCROLLCTRL_H_
   #include "gui/containers/guiScrollCtrl.h"
#endif

class LevelTileComponentUndo;
class LevelTileComponentTool : public BaseTool
{
	typedef BaseTool Parent;
	friend class LevelTileComponentUndo;
protected:
	bool mIsDirty;

   U32 mSavedGizmoFlags;

   S32 mFaceSEL;
   S32 mFaceHL;

   Gui3DMouseEvent mMouseDownEvent;

   Point3F mGizmoMatOffset;

   Point3F mPivotPos;
   bool mUsingPivot;
   bool mSettingPivot;

   UndoAction *mLastUndo;
   UndoManager *mUndoManager;

   bool createMode;

   //our guis
   GuiWindowCtrl* mTileSetEditorWindow;
   GuiTabBookCtrl *TileEditTabBook;
   GuiTabPageCtrl *TileEditTabPage;

   LevelTileComponentInstance *mSelectedTileObject;

   bool hasSelectedTile;
   Point2I selectedTile;

   bool builtTiles;

   static const S32 MAX_TILES = 100;

   struct Tile
   {
      F32 width;
      Point2F min;
      Point2F max;
      Point2F center;
      S32 type;
   };

   Tile mTiles[MAX_TILES][MAX_TILES];

	enum UndoType
	{
		ModifyShape = 0,
		CreateShape,
		DeleteShape,
		HollowShape
	};

	BaseTool* mCreateTool;
public:

   LevelTileComponentTool();
   ~LevelTileComponentTool() {}

   static void initPersistFields();

   virtual void onActivated( BaseTool *prevTool );
   virtual void onDeactivated( BaseTool *newTool );

   virtual EventResult onKeyDown( const GuiEvent &event );
   virtual EventResult on3DMouseDown( const Gui3DMouseEvent &event );
   virtual EventResult on3DMouseUp( const Gui3DMouseEvent &event );
   virtual EventResult on3DMouseMove( const Gui3DMouseEvent &event ) ;
   virtual EventResult on3DMouseDragged( const Gui3DMouseEvent &event );
   virtual EventResult on3DRightMouseDown( const Gui3DMouseEvent &event ) { return Handled; }
   virtual EventResult on3DRightMouseUp( const Gui3DMouseEvent &event ) { return Handled; }

   virtual void updateGizmo();
   
   virtual void renderScene(const RectI & updateRect);
   virtual void render2D() {} 

   void buildGrid();

   //
	void scaleFace( LevelTileComponentInstance *shape, S32 faceId, Point3F scale );
	void translateFace( LevelTileComponentInstance *shape, S32 faceId, const Point3F &displace );
	void updateModifiedFace( LevelTileComponentInstance *shape, S32 faceId );
	bool isShapeValid( LevelTileComponentInstance *shape );
	void setupShape( LevelTileComponentInstance *shape );
	void updateShape( LevelTileComponentInstance *shape, S32 offsetFace = -1 );
	void synchClientObject( const LevelTileComponentInstance *serverConvex );
	void updateGizmoPos();
	void setSelection( LevelTileComponentInstance *shape, S32 faceId );
	void submitUndo( UndoType type, LevelTileComponentInstance *shape );
	void submitUndo( UndoType type, const Vector<LevelTileComponentInstance*> &shapes );
	void setPivotPos( LevelTileComponentInstance *shape, S32 faceId, const Gui3DMouseEvent &event );
	void cleanMatrix( MatrixF &mat );
	S32 getEdgeByPoints( LevelTileComponentInstance *shape, S32 faceId, S32 p0, S32 p1 );
	bool getEdgesTouchingPoint( LevelTileComponentInstance *shape, S32 faceId, S32 pId, Vector< U32 > &edgeIdxList, S32 excludeEdge );

	bool handleEscape();

	static bool _cursorCastCallback( RayInfo* ri );
	bool _cursorCast( const Gui3DMouseEvent &event, LevelTileComponentInstance **hitShape, S32 *hitFace );

	DECLARE_CONOBJECT(LevelTileComponentTool);
};

class LevelTileComponentUndo : public UndoAction
{
   friend class LevelTileComponentTool;
public:

   LevelTileComponentUndo( const UTF8* actionName ) : UndoAction( actionName )
   {
   }

   LevelTileComponentTool *mEditor;         
   
   SimObjectId mObjId;

   Vector< MatrixF > mSavedSurfaces;
   MatrixF mSavedObjToWorld;
   Point3F mSavedScale;   

   virtual void undo();
   virtual void redo() { undo(); }
};

//================================================================================
// Create Tool
//================================================================================
class LevelTileCreateTool : public BaseTool
{
protected:
	
	BaseTool *mPrevTool;
public:

   LevelTileCreateTool();
	~LevelTileCreateTool(){}

   virtual void onActivated( BaseTool *prevTool ){}
   virtual void onDeactivated( BaseTool *newTool ){}

   virtual EventResult onKeyDown( const GuiEvent &event ) { return NotHandled; }
   virtual EventResult on3DMouseDown( const Gui3DMouseEvent &event );
   virtual EventResult on3DMouseUp( const Gui3DMouseEvent &event );
   virtual EventResult on3DMouseMove( const Gui3DMouseEvent &event );
   virtual EventResult on3DMouseDragged( const Gui3DMouseEvent &event );
   virtual EventResult on3DMouseEnter( const Gui3DMouseEvent &event ) { return NotHandled; }
   virtual EventResult on3DMouseLeave( const Gui3DMouseEvent &event ) { return NotHandled; }
   virtual EventResult on3DRightMouseDown( const Gui3DMouseEvent &event ) { return NotHandled; }
   virtual EventResult on3DRightMouseUp( const Gui3DMouseEvent &event ) { return NotHandled; }

	virtual void updateGizmo(){}
   
   virtual void renderScene(const RectI & updateRect);
   virtual void render2D() {} 

   DECLARE_CONOBJECT(LevelTileCreateTool);
};

#endif