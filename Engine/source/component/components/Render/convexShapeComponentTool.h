#ifndef _CONVEX_SHAPE_BEHAVIOR_TOOL_H_
#define _CONVEX_SHAPE_BEHAVIOR_TOOL_H_

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
#ifndef _CONVEX_SHAPE_COMPONENT_H_
	#include "component/components/Render/convexShapeComponent.h"
#endif

class ConvexShapeComponentUndo;
class ConvexShapeComponentTool : public BaseTool
{
	typedef BaseTool Parent;
	friend class ConvexShapeComponentUndo;
protected:
	bool mIsDirty;

   U32 mSavedGizmoFlags;

	/// The selected ConvexShape.
   SimObjectPtr<ConvexShapeComponentInstance> mConvexSEL;      

   /// The highlighted ConvexShape ( mouse over ).
   SimObjectPtr<ConvexShapeComponentInstance> mConvexHL;

   S32 mFaceSEL;
   S32 mFaceHL;

   MatrixF mFaceSavedXfm;

   ConvexShapeComponentInstance::MeshGeometry mSavedGeometry;
   Vector< MatrixF > mSavedSurfaces;
   Vector< MatrixF > mLastValidShape;

   StringTableEntry mMaterialName;

	ConvexShapeComponent *mConvexBehaviorTemplate;

   Point3F mSavedPivotPos;

   bool mCtrlDown;
   bool mSavedUndo;
   bool mHasGeometry;
   bool mDragging;
   bool mMouseDown;
   bool mHasCopied;
   RayInfo mLastRayInfo;

   Gui3DMouseEvent mMouseDownEvent;

   Point3F mGizmoMatOffset;

   Point3F mPivotPos;
   bool mUsingPivot;
   bool mSettingPivot;

   UndoAction *mLastUndo;
   UndoManager *mUndoManager;

   bool createMode;

	enum UndoType
	{
		ModifyShape = 0,
		CreateShape,
		DeleteShape,
		HollowShape
	};

	BaseTool* mCreateTool;
	BaseTool* mTextureTool;
public:

   ConvexShapeComponentTool();
   ~ConvexShapeComponentTool() {}

   static void initPersistFields();

   virtual void onActivated( ConvexShapeComponentTool *prevTool ) {}
   virtual void onDeactivated( ConvexShapeComponentTool *newTool ) {}

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

   //
	void scaleFace( ConvexShapeComponentInstance *shape, S32 faceId, Point3F scale );
	void translateFace( ConvexShapeComponentInstance *shape, S32 faceId, const Point3F &displace );
	void updateModifiedFace( ConvexShapeComponentInstance *shape, S32 faceId );
	bool isShapeValid( ConvexShapeComponentInstance *shape );
	void setupShape( ConvexShapeComponentInstance *shape );
	void updateShape( ConvexShapeComponentInstance *shape, S32 offsetFace = -1 );
	void synchClientObject( const ConvexShapeComponentInstance *serverConvex );
	void updateGizmoPos();
	void setSelection( ConvexShapeComponentInstance *shape, S32 faceId );
	void submitUndo( UndoType type, ConvexShapeComponentInstance *shape );
	void submitUndo( UndoType type, const Vector<ConvexShapeComponentInstance*> &shapes );
	void setPivotPos( ConvexShapeComponentInstance *shape, S32 faceId, const Gui3DMouseEvent &event );
	void cleanMatrix( MatrixF &mat );
	S32 getEdgeByPoints( ConvexShapeComponentInstance *shape, S32 faceId, S32 p0, S32 p1 );
	bool getEdgesTouchingPoint( ConvexShapeComponentInstance *shape, S32 faceId, S32 pId, Vector< U32 > &edgeIdxList, S32 excludeEdge );

	bool handleEscape();

	static bool _cursorCastCallback( RayInfo* ri );
	bool _cursorCast( const Gui3DMouseEvent &event, ConvexShapeComponentInstance **hitShape, S32 *hitFace );

	DECLARE_CONOBJECT(ConvexShapeComponentTool);
};

class ConvexShapeComponentUndo : public UndoAction
{
   friend class ConvexShapeComponentTool;
public:

   ConvexShapeComponentUndo( const UTF8* actionName ) : UndoAction( actionName )
   {
   }

   ConvexShapeComponentTool *mEditor;         
   
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
class ConvexShapeCreateTool : public BaseTool
{
protected:
	ConvexShapeComponentInstance *mNewConvex;
	friend class ConvexShapeComponentTool;

	S32 mStage;   
	PlaneF mCreatePlane;
   
	MatrixF mTransform;
	Point3F mStart;
	Point3F mEnd;
	Point3F mPlaneSizes;

	Entity* editorTarget;

	StringTableEntry mMaterialName;

	ConvexShapeComponent *mConvexBehaviorTemplate;

	BaseTool *mPrevTool;
public:

   ConvexShapeCreateTool();
	~ConvexShapeCreateTool(){}

   virtual void onActivated( BaseTool *prevTool );
   virtual void onDeactivated( BaseTool *newTool );

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

   ConvexShapeComponentInstance* extrudeShapeFromFace( ConvexShapeComponentInstance *shape, S32 face );

	void updateShape( ConvexShapeComponentInstance *shape, S32 offsetFace = -1 );

   DECLARE_CONOBJECT(ConvexShapeCreateTool);
};

//================================================================================
// Texture Tool
//================================================================================
class ConvexShapeTextureTool : public BaseTool
{
protected:
	ConvexShapeComponentInstance *mNewConvex;

	/// The selected ConvexShape.
   SimObjectPtr<ConvexShapeComponentInstance> mConvexSEL;      

   /// The highlighted ConvexShape ( mouse over ).
   SimObjectPtr<ConvexShapeComponentInstance> mConvexHL;

   S32 mFaceSEL;
   S32 mFaceHL;

public:

	ConvexShapeTextureTool(){}
	~ConvexShapeTextureTool(){}

	virtual void onActivated( BaseTool *prevTool ){}
	virtual void onDeactivated( BaseTool *newTool ){}

   virtual EventResult onKeyDown( const GuiEvent &event ) { return NotHandled; }
   virtual EventResult on3DMouseDown( const Gui3DMouseEvent &event ) { return NotHandled; }
   virtual EventResult on3DMouseUp( const Gui3DMouseEvent &event ) { return NotHandled; }
   virtual EventResult on3DMouseMove( const Gui3DMouseEvent &event ) { return NotHandled; }
   virtual EventResult on3DMouseDragged( const Gui3DMouseEvent &event ) { return NotHandled; }
   virtual EventResult on3DMouseEnter( const Gui3DMouseEvent &event ) { return NotHandled; }
   virtual EventResult on3DMouseLeave( const Gui3DMouseEvent &event ) { return NotHandled; }
   virtual EventResult on3DRightMouseDown( const Gui3DMouseEvent &event ) { return NotHandled; }
   virtual EventResult on3DRightMouseUp( const Gui3DMouseEvent &event ) { return NotHandled; }

	virtual void updateGizmo(){}
   
   virtual void renderScene(const RectI & updateRect) {}
   virtual void render2D() {} 

   //ConvexShapeComponentInstance* extrudeShapeFromFace( ConvexShapeComponentInstance *shape, S32 face );

   DECLARE_CONOBJECT(ConvexShapeCreateTool);
};

#endif