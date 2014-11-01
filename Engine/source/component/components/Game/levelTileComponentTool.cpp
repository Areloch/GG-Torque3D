#include "component/components/game/LevelTileComponentTool.h"
#include "platform/platform.h"
#include "gfx/sim/debugDraw.h"
#include "gfx/gfxDrawUtil.h"
#include "gui/worldEditor/undoActions.h"

#include "math/util/frustum.h"
#include "math/mathUtils.h"
#include "gfx/gfxPrimitiveBuffer.h"
#include "gfx/gfxTextureHandle.h"
#include "gfx/gfxTransformSaver.h"
#include "gfx/primBuilder.h"
#include "T3D/prefab.h"
#include "T3D/Entity.h"

#include "gui/worldEditor/worldEditor.h"

#include "gui/containers/guiWindowCollapseCtrl.h"

#include "platform/platformInput.h"

#include "core/strings/stringUnit.h"

IMPLEMENT_CONOBJECT(LevelTileCreateTool);
IMPLEMENT_CONOBJECT(LevelTileComponentTool);

LevelTileComponentTool::LevelTileComponentTool()
{
	mCreateTool = new LevelTileCreateTool();
	//mTextureTool = new ConvexShapeTextureTool();

   for(U32 i=0; i < MAX_TILES; i++)
   {
      for(U32 j=0; j < MAX_TILES; j++)
      {
         mTiles[i][j] = Tile();
         mTiles[i][j].type = -1;
      }
   }

   hasSelectedTile = false;

   builtTiles = false;

	mFaceSEL = -1;
}

void LevelTileComponentTool::initPersistFields()
{   
	addField( "isDirty", TypeBool, Offset( mIsDirty, LevelTileComponentTool ) );

   addField( "selectedTile", TypeSimObjectPtr, Offset(mSelectedTileObject, LevelTileComponentTool) );

   Parent::initPersistFields();
}

void LevelTileComponentTool::onActivated( BaseTool *prevTool )
{
   Con::executef(this, "onActivated");
}

void LevelTileComponentTool::onDeactivated( BaseTool *newTool )
{
	//mPrevTool = NULL;
   mEditor->mouseUnlock();

   Con::executef(this, "onDeactivated");
}

LevelTileComponentTool::EventResult LevelTileComponentTool::onKeyDown( const GuiEvent &event ) 
{ 
	bool handled = false;

   switch ( event.keyCode )
   {
   case KEY_ESCAPE:
      handled = handleEscape();      
      break;   
   case KEY_A:
      if ( event.modifier & SI_ALT )
      {
		   GizmoAlignment align = mEditor->getGizmo()->getProfile()->alignment;
         if ( align == World )
             mEditor->getGizmo()->getProfile()->alignment = Object;
         else
             mEditor->getGizmo()->getProfile()->alignment = World;
         handled = true;
      }
      break;
   case KEY_LCONTROL:
      //mCtrlDown = true;
      break;  
	case KEY_Q:
		mEditor->setActiveTool(NULL);
      break;
   default:
      break;
   }
   
   if(handled)
	   return Handled;
   else
	   return Done;
}

void LevelTileComponentTool::buildGrid() 
{
   if(mSelectedTileObject == NULL)
      return;

   Point2F gridCenter = Point2F(mSelectedTileObject->getOwner()->getPosition().x, mSelectedTileObject->getOwner()->getPosition().y);

   //start point of grid
   Point2F gridStart = gridCenter;
   gridStart.x -= 50; //hardcoded atm 
   gridStart.y -= 50; //hardcoded atm 

   for(U32 i=0; i < MAX_TILES; i++)
   {
      for(U32 j=0; j < MAX_TILES; j++)
      {
         Point2F tileStart = gridStart + Point2F(i,j);

         mTiles[i][j] = Tile();
         mTiles[i][j].min = tileStart;
         mTiles[i][j].max = tileStart + Point2F(1,1);
         mTiles[i][j].center = tileStart + Point2F(0.5, 0.5);
         mTiles[i][j].width = 1;
      }
   }

   builtTiles = true;
}

LevelTileComponentTool::EventResult LevelTileComponentTool::on3DMouseDown( const Gui3DMouseEvent &event ) 
{ 
	mEditor->mouseLock();   

   buildGrid();

   //find out which grid position we've clicked
   PlaneF plane = PlaneF(mSelectedTileObject->getOwner()->getPosition(), Point3F(0,0,1));
   
   Point3F start( event.pos );
   Point3F end( start + event.vec * 10000.0f );

   F32 t = plane.intersect( start, end );

   if ( t >= 0.0f && t <= 1.0f )
   {
      Point3F hitPos;
      hitPos.interpolate( start, end, t );

      for(U32 i=0; i < MAX_TILES; i++)
      {
         for(U32 j=0; j < MAX_TILES; j++)
         {

            if(hitPos.x > mTiles[i][j].min.x && hitPos.y > mTiles[i][j].min.y
               && hitPos.x < mTiles[i][j].max.x && hitPos.y < mTiles[i][j].max.y)
            {
               hasSelectedTile = true;
               selectedTile = Point2I(i,j);
            }
         }
      }
   }

   //mEditor->getGizmo()->on3DMouseDown( event ); 

   return Done;
}

LevelTileComponentTool::EventResult LevelTileComponentTool::on3DMouseUp( const Gui3DMouseEvent &event ) 
{ 
	mEditor->mouseUnlock();

   updateGizmoPos(); 

   return Done; 
}
LevelTileComponentTool::EventResult LevelTileComponentTool::on3DMouseMove( const Gui3DMouseEvent &event ) 
{ 
   mEditor->getGizmo()->on3DMouseMove( event );

   return Done;
}

LevelTileComponentTool::EventResult LevelTileComponentTool::on3DMouseDragged( const Gui3DMouseEvent &event ) 
{
   return Done;
}

void LevelTileComponentTool::renderScene(const RectI & updateRect)
{      
   GFXDrawUtil *drawer = GFX->getDrawUtil();

   GFXStateBlockDesc desc;
   desc.setZReadWrite( true, false );
   desc.setBlend( true );
   desc.setCullMode( GFXCullNone );
   desc.fillMode = GFXFillWireframe;

   drawer->drawSphere(desc, 1, mSelectedTileObject->getOwner()->getPosition(), ColorI(255, 100, 200, 255));

   Point3F gridStart = mSelectedTileObject->getOwner()->getPosition() + Point3F(-50, -50, 0);

   //draw the grid
   for(U32 i=0; i < MAX_TILES/2; i++)
   {
      //X aix
      Point3F xLineStart = Point3F(gridStart.x, gridStart.y + (i*2), gridStart.z);
      drawer->drawLine(xLineStart, xLineStart + Point3F(100, 0, 0), ColorI(200, 200, 200, 255));

      Point3F yLineStart = Point3F(gridStart.x + (i*2), gridStart.y, gridStart.z);
      drawer->drawLine(yLineStart, yLineStart + Point3F(0, 100, 0), ColorI(200, 200, 200, 255));
   }
   //drawer->drawPlaneGrid( desc, mSelectedTileObject->getOwner()->getPosition(), Point2F(100,100), Point2F(1,1), ColorI(200, 200, 200, 255) );

   if(hasSelectedTile)
   {
      S32 i = selectedTile.x;
      S32 j = selectedTile.y;

      Point2F pos = mTiles[i][j].center;

      Box3F box = Box3F( Point3F(mTiles[i][j].min.x, mTiles[i][j].min.y, mSelectedTileObject->getOwner()->getPosition().z),
               Point3F(mTiles[i][j].max.x, mTiles[i][j].max.y, mSelectedTileObject->getOwner()->getPosition().z + 1));
      drawer->drawCube(desc, box, ColorI(0, 0, 255, 255));
      /*for(U32 i=0; i < MAX_TILES; i++)
      {
         for(U32 j=0; j < MAX_TILES; j++)
         {
            //Box3F n = Box3F( Point3F(mTiles[i][j].center.x, mTiles[i][j].center.y, mSelectedTileObject->getOwner()->getPosition().z-0.1) + Point3F(-0.1, -0.1, 0),
            //   Point3F(mTiles[i][j].center.x, mTiles[i][j].center.y, mSelectedTileObject->getOwner()->getPosition().z + 0.1) + Point3F(0.1, 0.1, 0));

            Box3F n = Box3F( Point3F(mTiles[i][j].min.x, mTiles[i][j].min.y, mSelectedTileObject->getOwner()->getPosition().z),
               Point3F(mTiles[i][j].max.x, mTiles[i][j].max.y, mSelectedTileObject->getOwner()->getPosition().z + 1));

            drawer->drawCube(desc, n, ColorI(0, 0, 255, 255));
         }
      }*/
   }

   SimObject *statusbar;
   if ( Sim::findObject( "EditorGuiStatusBar", statusbar ) )
   {
      String text( "Tile Tool." );      
      GizmoMode mode = mEditor->getGizmo()->getMode();

      {     
          text = "Tile Tool.  ALT + Click-Drag to create a new ConvexShape.";
      }

      Con::executef( statusbar, "setInfo", text.c_str() );

	  //Con::executef( statusbar, "setSelectionObjectsByCount", 0 );
   }

   mEditor->getGizmo()->renderGizmo( mEditor->getLastCameraQuery().cameraMatrix, mEditor->getLastCameraQuery().fov );
} 

void LevelTileComponentTool::updateGizmo()
{
	mEditor->getGizmo()->getProfile()->restoreDefaultState();
}

void LevelTileComponentTool::scaleFace( LevelTileComponentInstance *shape, S32 faceId, Point3F scale )
{
   
}

void LevelTileComponentTool::translateFace( LevelTileComponentInstance *shape, S32 faceId, const Point3F &displace )
{
   
}

void LevelTileComponentTool::updateModifiedFace( LevelTileComponentInstance *shape, S32 faceId )
{
 
}

bool LevelTileComponentTool::isShapeValid( LevelTileComponentInstance *shape )
{
   

   return true;
}

void LevelTileComponentTool::setupShape( LevelTileComponentInstance *shape )
{

}

void LevelTileComponentTool::updateShape( LevelTileComponentInstance *shape, S32 offsetFace )
{
   
}

void LevelTileComponentTool::synchClientObject( const LevelTileComponentInstance *serverConvex )
{
	return;
   
}

void LevelTileComponentTool::updateGizmoPos()
{
   Parent::updateGizmo(); 
}

void LevelTileComponentTool::setSelection( LevelTileComponentInstance *shape, S32 faceId )
{
   mFaceSEL = faceId;
   updateGizmoPos();
}

void LevelTileComponentTool::submitUndo( UndoType type, LevelTileComponentInstance *shape )
{
   Vector< LevelTileComponentInstance* > shapes;
   shapes.push_back( shape );
   submitUndo( type, shapes );
}

void LevelTileComponentTool::submitUndo( UndoType type, const Vector<LevelTileComponentInstance*> &shapes )
{   
   // Grab the mission editor undo manager.
   Sim::findObject( "EUndoManager", mUndoManager );   
   
   if ( !mUndoManager )   
   {
      Con::errorf( "GuiConvexEditorCtrl::submitUndo() - EUndoManager not found!" );
      return;           
   }

	mIsDirty = true;
}

void LevelTileComponentTool::setPivotPos( LevelTileComponentInstance *shape, S32 faceId, const Gui3DMouseEvent &event )
{
   
}

void LevelTileComponentTool::cleanMatrix( MatrixF &mat )
{
   if ( mat.isAffine() )
      return;

   VectorF col0 = mat.getColumn3F(0);
   VectorF col1 = mat.getColumn3F(1);
   VectorF col2 = mat.getColumn3F(2);

   col0.normalize();
   col1.normalize();
   col2.normalize();

   col2 = mCross( col0, col1 );
   col2.normalize();
   col1 = mCross( col2, col0 );
   col1.normalize();
   col0 = mCross( col1, col2 );
   col0.normalize();

   mat.setColumn(0,col0);
   mat.setColumn(1,col1);
   mat.setColumn(2,col2);

   AssertFatal( mat.isAffine(), "GuiConvexEditorCtrl::cleanMatrix, non-affine matrix" );
}

S32 LevelTileComponentTool::getEdgeByPoints( LevelTileComponentInstance *shape, S32 faceId, S32 p0, S32 p1 )
{

   return -1;
}

bool LevelTileComponentTool::getEdgesTouchingPoint( LevelTileComponentInstance *shape, S32 faceId, S32 pId, Vector< U32 > &edgeIdxList, S32 excludeEdge )
{
   return false;
}

bool LevelTileComponentTool::handleEscape()
{
   return false;
}

bool LevelTileComponentTool::_cursorCastCallback( RayInfo* ri )
{
   // Reject anything that's not a ConvexShape.
   /*Entity* e = dynamic_cast< Entity* >( ri->object );
   if(e)
   {
	   //if we're in the tool, we'll be looking for the convex shape specifically. Check the user data on the rayInfo
	   if(ri->userData)
	   {
		   return true;
	   }
   }*/
   return true;
}

bool LevelTileComponentTool::_cursorCast( const Gui3DMouseEvent &event, LevelTileComponentInstance **hitShape, S32 *hitFace )
{
   return false;
}
//
//
void LevelTileComponentUndo::undo()
{
   LevelTileComponentInstance *object = NULL;
   if ( !Sim::findObject( mObjId, object ) )
      return;

   
}
//
//================================================================================
// Create tool
//================================================================================
LevelTileCreateTool::LevelTileCreateTool(  )
{

}

BaseTool::EventResult LevelTileCreateTool::on3DMouseDown( const Gui3DMouseEvent &event )
{
  
   return Handled;
}

BaseTool::EventResult LevelTileCreateTool::on3DMouseUp( const Gui3DMouseEvent &event )
{
   
   return Done;
}

BaseTool::EventResult LevelTileCreateTool::on3DMouseMove( const Gui3DMouseEvent &event )
{
   

   return Handled;
}

BaseTool::EventResult LevelTileCreateTool::on3DMouseDragged( const Gui3DMouseEvent &event )
{
   
   return Handled;
}

void LevelTileCreateTool::renderScene( const RectI &updateRect )
{
	// Synch selected ConvexShape with the WorldEditor.

	/*WorldEditor *wedit;
	if ( Sim::findObject( "EWorldEditor", wedit) )
	{
		S32 count = wedit->getSelectionSize();

		if ( !mConvexSEL && count != 0 )
			wedit->clearSelection();
		else if ( mConvexSEL && count != 1 )
		{
			wedit->clearSelection();
			wedit->selectObject( mConvexSEL->getIdString() );
		}
		else if ( mConvexSEL && count == 1 )
		{
			if ( wedit->getSelectObject(0) != mConvexSEL->getId() )
			{
				wedit->clearSelection();
				wedit->selectObject( mConvexSEL->getIdString() );
			}
		}
	}   

   // Update status bar text.

   SimObject *statusbar;
   if ( Sim::findObject( "EditorGuiStatusBar", statusbar ) )
   {
      String text( "Sketch Tool." );      
      GizmoMode mode = mEditor->getGizmo()->getMode();

      if ( mMouseDown && mEditor->getGizmo()->getSelection() != Gizmo::None && mConvexSEL )
      {
         Point3F delta;
         String qualifier;

         if ( mode == RotateMode )   
         {
            if ( mSettingPivot )            
               delta = mEditor->getGizmo()->getPosition() - mSavedPivotPos;
            else
               delta = mEditor->getGizmo()->getDeltaTotalRot();         
         }
         else if ( mode == MoveMode )         
            delta = mEditor->getGizmo()->getTotalOffset();         
         else if ( mode == ScaleMode )
            delta = mEditor->getGizmo()->getDeltaTotalScale();            
         
         if ( mEditor->getGizmo()->getAlignment() == Object && 
              mode != ScaleMode )
         {            
            mConvexSEL->getOwner()->getWorldToObj().mulV( delta );            
            if ( mFaceSEL != -1 && mode != RotateMode )
            {
               MatrixF objToSurf( mConvexSEL->mSurfaces[ mFaceSEL ] );
               objToSurf.scale( mConvexSEL->getOwner()->getScale() );
               objToSurf.inverse();
               objToSurf.mulV( delta );
            }
         }

         if ( mIsZero( delta.x, 0.0001f ) )
            delta.x = 0.0f;
         if ( mIsZero( delta.y, 0.0001f ) )
            delta.y = 0.0f;
         if ( mIsZero( delta.z, 0.0001f ) )
            delta.z = 0.0f;
         
         if ( mode == RotateMode )         
         {
            if ( mSettingPivot )            
               text = String::ToString( "Delta position ( x: %4.2f, y: %4.2f, z: %4.2f ).", delta.x, delta.y, delta.z ); 
            else
            {
               delta.x = mRadToDeg( delta.x );
               delta.y = mRadToDeg( delta.y );
               delta.z = mRadToDeg( delta.z );
               text = String::ToString( "Delta angle ( x: %4.2f, y: %4.2f, z: %4.2f ).", delta.x, delta.y, delta.z ); 
            }
         }
         else if ( mode == MoveMode )     
            text = String::ToString( "Delta position ( x: %4.2f, y: %4.2f, z: %4.2f ).", delta.x, delta.y, delta.z ); 
         else if ( mode == ScaleMode )
            text = String::ToString( "Delta scale ( x: %4.2f, y: %4.2f, z: %4.2f ).", delta.x, delta.y, delta.z ); 
      }
      else 
      {     
         if ( !mConvexSEL )
            text = "Sketch Tool.  ALT + Click-Drag to create a new ConvexShape.";
         else if ( mFaceSEL == -1 )
         {
            if ( mode == MoveMode )            
               text = "Move selection.  SHIFT while dragging duplicates objects.";
            else if ( mode == RotateMode )            
               text = "Rotate selection.";
            else if ( mode == ScaleMode )            
               text = "Scale selection.";        
         }
         else 
         {
            if ( mode == MoveMode )            
               text = "Move face.  SHIFT while beginning a drag EXTRUDES a new convex. Press CTRL for alternate translation mode.";
            else if ( mode == RotateMode )            
               text = "Rotate face.  Gizmo/Pivot is draggable. CTRL while dragging splits/folds a new face. SHIFT while dragging extrudes a new convex.";
            else if ( mode == ScaleMode )            
            text = "Scale face.";
         }
      }
   
      // Issue a warning in the status bar
      // if this convex has an excessive number of surfaces...
      if ( mConvexSEL && mConvexSEL->getSurfaces().size() > LevelTileComponentInstance::smMaxSurfaces )
      {
          text = "WARNING: Reduce the number of surfaces on the selected ConvexShape, only the first 100 will be saved!";
      }

      Con::executef( statusbar, "setInfo", text.c_str() );

	Con::executef( statusbar, "setSelectionObjectsByCount", Con::getIntArg( mConvexSEL == NULL ? 0 : 1 ) );
   }   

   //if ( createMode )
   //   mActiveTool->renderScene( updateRect );

   ColorI colorHL( 255, 50, 255, 255 );
   ColorI colorSEL( 255, 50, 255, 255 );
   ColorI colorNA( 255, 255, 255, 100 );

   GFXDrawUtil *drawer = GFX->getDrawUtil();

   if ( mConvexSEL && !mDragging )
   {
      if ( mFaceSEL == -1 )
      {
         GFXStateBlockDesc desc;
         desc.setBlend( true );
         desc.setZReadWrite( true, true );

         Box3F objBox = mConvexSEL->getOwner()->getObjBox();
         objBox.scale( mConvexSEL->getOwner()->getScale() );

         const MatrixF &objMat = mConvexSEL->getOwner()->getTransform();

         Point3F boxPos = objBox.getCenter();
         objMat.mulP( boxPos );
         
         drawer->drawObjectBox( desc, objBox.getExtents(), boxPos, objMat, ColorI::WHITE );
      }
      else
      {
         mConvexSEL->renderFaceEdges( -1, colorNA );     

		 //Not currently imlemented?
         //drawFacePlane( mConvexSEL, mFaceSEL );         
      }

      if ( mConvexHL == mConvexSEL &&
           mFaceHL != -1 && 
           mFaceHL != mFaceSEL && 
           mEditor->getGizmo()->getSelection() == Gizmo::None )
      {
         mConvexSEL->renderFaceEdges( mFaceHL, colorHL );
      }
   }

   if ( mConvexHL && mConvexHL != mConvexSEL )
   {
      mConvexHL->renderFaceEdges( -1 );      
   }

   if ( mEditor->getGizmo()->getMode() != RotateMode && mUsingPivot )
   {
      mUsingPivot = false;
      updateGizmoPos();
   }

   F32 gizmoAlpha = 1.0f;
	if ( !mConvexSEL )
		gizmoAlpha = 0.0f;

   if ( mMouseDown && mEditor->getGizmo()->getSelection() != Gizmo::None && mConvexSEL )
   {
      if ( mSettingPivot )
         gizmoAlpha = 1.0f;
      else
         gizmoAlpha = 0.0f;
   }

   DebugDrawer::get()->render();

   {
      GFXTransformSaver saver;
      // Now draw all the 2d stuff!
      GFX->setClipRect(updateRect); 

      if ( mConvexSEL && mFaceSEL != -1 )
      {      
         Vector< Point3F > lineList;
         mConvexSEL->getSurfaceLineList( mFaceSEL, lineList );

         MatrixF objToWorld( mConvexSEL->getOwner()->getTransform() );
         objToWorld.scale( mConvexSEL->getOwner()->getScale() );      

         for ( S32 i = 0; i < lineList.size(); i++ )     
            objToWorld.mulP( lineList[i] );			

         for ( S32 i = 0; i < lineList.size() - 1; i++ )
         {
			   Point3F p0( lineList[i] );
			   Point3F p1( lineList[i+1] );

			   mEditor->drawLine( p0, p1, colorSEL, 3.0f );
         }
	   }

      if ( gizmoAlpha == 1.0f )
      {
         if ( mEditor->getGizmo()->getProfile()->mode != NoneMode )
            mEditor->getGizmo()->renderText( mEditor->getLastViewportRect(), mEditor->getLastWorldMatrix(), mEditor->getLastProjectionMatrix() );   	
      }

      //if ( createMode )
      //   mActiveTool->render2D();
   }

   if ( gizmoAlpha == 1.0f )   
	   mEditor->getGizmo()->renderGizmo( mEditor->getLastCameraQuery().cameraMatrix, mEditor->getLastCameraQuery().fov );*/
}