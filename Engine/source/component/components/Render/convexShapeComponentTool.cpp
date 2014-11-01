#include "component/components/render/convexShapeComponentTool.h"
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

#include "platform/platformInput.h"

IMPLEMENT_CONOBJECT(ConvexShapeCreateTool);
IMPLEMENT_CONOBJECT(ConvexShapeComponentTool);

ConvexShapeComponentTool::ConvexShapeComponentTool()
{
	mCreateTool = new ConvexShapeCreateTool();
	mTextureTool = NULL;
	//mTextureTool = new ConvexShapeTextureTool();

	mMaterialName = StringTable->insert("Grid512_OrangeLines_Mat");
	mDragging = false;
	mFaceSEL = -1;
}

void ConvexShapeComponentTool::initPersistFields()
{   
	addField( "isDirty", TypeBool, Offset( mIsDirty, ConvexShapeComponentTool ) );
	addField( "materialName", TypeString, Offset(mMaterialName, ConvexShapeComponentTool) );

	addField( "defaultTemplate", TypeSimObjectPtr, Offset(mConvexBehaviorTemplate, ConvexShapeComponentTool) );

   Parent::initPersistFields();
}

ConvexShapeComponentTool::EventResult ConvexShapeComponentTool::onKeyDown( const GuiEvent &event ) 
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

ConvexShapeComponentTool::EventResult ConvexShapeComponentTool::on3DMouseDown( const Gui3DMouseEvent &event ) 
{ 
	mEditor->mouseLock();   

    mMouseDown = true;

   if ( event.modifier & SI_ALT )
   {
		//update the default convex behavior as needed
		dynamic_cast<ConvexShapeCreateTool*>(mCreateTool)->mConvexBehaviorTemplate = mConvexBehaviorTemplate;
      dynamic_cast<ConvexShapeCreateTool*>(mCreateTool)->mMaterialName = mMaterialName;

		mEditor->setActiveTool(mCreateTool);
		return mCreateTool->on3DMouseDown( event );
   }

	if( !mConvexSEL && mConvexHL)
	{
		mConvexSEL = mConvexHL;
	}
	else if(mConvexSEL && mConvexHL && mConvexSEL != mConvexHL)
	{
		mConvexSEL = mConvexHL;
	}

   if ( mConvexSEL && isShapeValid( mConvexSEL ) )      
      mLastValidShape = mConvexSEL->mSurfaces;  

	if ( mConvexSEL &&
        mFaceSEL != -1 &&
        mEditor->getGizmo()->getMode() == RotateMode &&
        mEditor->getGizmo()->getSelection() == Gizmo::Centroid )
   {      
      mSettingPivot = true;      
      mSavedPivotPos = mEditor->getGizmo()->getPosition();
      setPivotPos( mConvexSEL, mFaceSEL, event );
      updateGizmoPos();
      return Done;
   }

   mEditor->getGizmo()->on3DMouseDown( event ); 

   return Done;
}

ConvexShapeComponentTool::EventResult ConvexShapeComponentTool::on3DMouseUp( const Gui3DMouseEvent &event ) 
{ 
	mEditor->mouseUnlock();

   mMouseDown = false;

   mHasCopied = false;
   mHasGeometry = false;   

   /*if ( createMode )
   {
      ConvexShapeComponentTool::EventResult result = mActiveTool->on3DMouseUp( event );

      if ( result == ConvexShapeComponentTool::Done )      
         createMode = false;         
      
      return;
   }*/

   if ( !mSettingPivot && !mDragging && ( mEditor->getGizmo()->getSelection() == Gizmo::None || !mConvexSEL ) )
   {
      if ( mConvexSEL != mConvexHL )
      {         
         setSelection( mConvexHL, -1 );
      }
      else
      {
         if ( mFaceSEL != mFaceHL )         
            setSelection( mConvexSEL, mFaceHL );         
         else
            setSelection( mConvexSEL, -1 );
      }

      mUsingPivot = false;
   }

   mSettingPivot = false;
   mSavedPivotPos = mEditor->getGizmo()->getPosition();
   mSavedUndo = false;   

   mEditor->getGizmo()->on3DMouseUp( event );

   if ( mDragging )
   {
      mDragging = false;

      if ( mConvexSEL )
      {         
         Vector< U32 > removedPlanes;
         mConvexSEL->cullEmptyPlanes( &removedPlanes );

         // If a face has been removed we need to validate / remap
         // our selected and highlighted faces.
         if ( !removedPlanes.empty() )
         {
            S32 prevFaceHL = mFaceHL;
            S32 prevFaceSEL = mFaceSEL;

            if ( removedPlanes.contains( mFaceHL ) )
               prevFaceHL = mFaceHL = -1;
            if ( removedPlanes.contains( mFaceSEL ) )
               prevFaceSEL = mFaceSEL = -1;
            
            for ( S32 i = 0; i < removedPlanes.size(); i++ )
            {
               if ( (S32)removedPlanes[i] < prevFaceSEL )
                  mFaceSEL--;               
               if ( (S32)removedPlanes[i] < prevFaceHL )
                  mFaceHL--;     
            }        

            setSelection( mConvexSEL, mFaceSEL );

            // We need to reindex faces.
            updateShape( mConvexSEL );
         }
      }
   }

   updateGizmoPos(); 

   return Done; 
}
ConvexShapeComponentTool::EventResult ConvexShapeComponentTool::on3DMouseMove( const Gui3DMouseEvent &event ) 
{ 
	/*if ( createMode )
   {
      // If we have an active tool pass this event to it.
      // If it handled it, consume the event.
      if ( mActiveTool->on3DMouseMove( event ) )
         return;
   }*/

   ConvexShapeComponentInstance *hitShape = NULL;
   S32 hitFace = -1;
   
   _cursorCast( event, &hitShape, &hitFace );

   if ( !mConvexSEL )
   {
      mConvexHL = hitShape;
      mFaceHL = -1;
   }
   else
   {
      if ( mConvexSEL == hitShape )
      {
         mConvexHL = hitShape;        
         mFaceHL = hitFace;
      }
      else
      {
         // Mousing over a shape that is not the one currently selected.

         if ( mFaceSEL != -1 )
         {
            mFaceHL = -1;
         }
         else
         {
            mConvexHL = hitShape;
            mFaceHL = -1;
         } 
      }
   }

   if ( mConvexSEL )
      mEditor->getGizmo()->on3DMouseMove( event );

   return Done;
}

ConvexShapeComponentTool::EventResult ConvexShapeComponentTool::on3DMouseDragged( const Gui3DMouseEvent &event ) 
{
	/*if ( createMode )
   {
      // If we have an active tool pass this event to it.
      // If it handled it, consume the event.
      if ( mActiveTool->on3DMouseDragged( event ) )
         return;
   }*/

   //mEditor->getGizmo()->getProfile()->rotateScalar = 0.55f;
   //mEditor->getGizmo()->getProfile()->scaleScalar = 0.55f;

   if ( !mConvexSEL )
      return Done;

   if ( mEditor->getGizmo()->getMode() == RotateMode &&
        mEditor->getGizmo()->getSelection() == Gizmo::Centroid )
   {            
      setPivotPos( mConvexSEL, mFaceSEL, event );      
      mDragging = true;
      return Done;
   }

   mEditor->getGizmo()->on3DMouseDragged( event );
      
   if ( event.modifier & SI_SHIFT && 
       ( mEditor->getGizmo()->getMode() == MoveMode || mEditor->getGizmo()->getMode() == RotateMode ) &&
        !mHasCopied )
   {
      if ( mFaceSEL != -1 )
      {
			//Entity* newOwner = new Entity();
			//newOwner->registerObject();

			ConvexShapeCreateTool* cT = dynamic_cast<ConvexShapeCreateTool*>(mCreateTool);
         ConvexShapeComponentInstance *newShape = cT->extrudeShapeFromFace( mConvexSEL, mFaceSEL );
			//newOwner->addBehavior(newShape);
         //newShape->_updateGeometry();

         submitUndo( CreateShape, newShape );
         setSelection( newShape, 0 );         
         updateGizmoPos();

         mEditor->getGizmo()->on3DMouseDown( event );

         mHasCopied = true;
         mSavedUndo = true;
      }
      else
      {
			Entity* newOwner = new Entity();
			newOwner->registerObject();

         ConvexShapeComponentInstance *newShape = new ConvexShapeComponentInstance(mConvexBehaviorTemplate);
			newShape->setOwner(newOwner);

			newOwner->addComponent(newShape);

         newShape->setTransform( mConvexSEL->getOwner()->getTransform() );
         newShape->getOwner()->setScale( mConvexSEL->getOwner()->getScale() );
         newShape->mSurfaces.clear();
         newShape->mSurfaces.merge( mConvexSEL->mSurfaces );
         
         setupShape( newShape );

         submitUndo( CreateShape, newShape );

         setSelection( newShape, -1 );

         updateGizmoPos();

         mHasCopied = true;
         mSavedUndo = true;
      }

      return Done;
   }

   if ( mEditor->getGizmo()->getMode() == RotateMode &&
        event.modifier & SI_CTRL &&
        !mHasCopied &&
        mFaceSEL != -1 )
   {
      // Can must verify that splitting the face at the current angle 
      // ( of the gizmo ) will generate a valid shape.  If not enough rotation
      // has occurred we will have two faces that are coplanar and must wait
      // until later in the drag to perform the split.

      //AssertFatal( isShapeValid( mConvexSEL ), "Shape was already invalid at beginning of split operation." );

      if ( !isShapeValid( mConvexSEL ) )
         return Done;

      mLastValidShape = mConvexSEL->mSurfaces;

      Point3F rot = mEditor->getGizmo()->getDeltaTotalRot();
      rot.normalize();
      rot *= mDegToRad( 10.0f );

      MatrixF rotMat( (EulerF)rot );

      MatrixF worldToObj( mConvexSEL->getOwner()->getTransform() );
      worldToObj.scale( mConvexSEL->getOwner()->getScale() );
      worldToObj.inverse();      

      mConvexSEL->mSurfaces.increment();
      MatrixF &newSurf = mConvexSEL->mSurfaces.last();
      newSurf = mConvexSEL->mSurfaces[mFaceSEL] * rotMat;
      
      //worldToObj.mul( mEditor->getGizmo()->getOwner()->getTransform() );
      //Point3F pos( mPivotPos );
      //worldToObj.mulP( pos );
      //newSurf.setPosition( pos );

      updateShape( mConvexSEL );

      if ( !isShapeValid( mConvexSEL ) )
      {
         mConvexSEL->mSurfaces = mLastValidShape;
         updateShape( mConvexSEL );
      }
      else
      {
         mHasCopied = true;
         mSavedUndo = true;

         mLastValidShape = mConvexSEL->mSurfaces;

         submitUndo( ModifyShape, mConvexSEL );           

         setSelection( mConvexSEL, mConvexSEL->mSurfaces.size() - 1 );

         updateGizmoPos();
      }      
      
      return Done;
   }

   // If we are dragging, but no gizmo selection...
   // Then treat this like a regular mouse move, update the highlighted
   // convex/face under the cursor and handle onMouseUp as we normally would
   // to change the selection.
   if ( mEditor->getGizmo()->getSelection() == Gizmo::None )
   {
      ConvexShapeComponentInstance *hitShape = NULL;
      S32 hitFace = -1;

      _cursorCast( event, &hitShape, &hitFace );
      mFaceHL = hitFace;
      mConvexHL = hitShape;      

      return Done;
   }

   mDragging = true;

   // Manipulating a face.

   if ( mFaceSEL != -1 )
   {
      if ( !mSavedUndo )
      {
         mSavedUndo = true;
         submitUndo( ModifyShape, mConvexSEL );
      }      

      if ( mEditor->getGizmo()->getMode() == ScaleMode )
      {
         scaleFace( mConvexSEL, mFaceSEL, mEditor->getGizmo()->getScale() );
      }
      else
      {
         // Why does this have to be so ugly.
         if ( mEditor->getGizmo()->getMode() == RotateMode || 
              ( mEditor->getGizmo()->getMode() == MoveMode  && 
                ( event.modifier & SI_CTRL  ||
                  ( mEditor->getGizmo()->getSelection() == Gizmo::Axis_Z && mHasCopied ) 
                )
              )
            )
         {
            const MatrixF &gMat = mEditor->getGizmo()->getTransform();      
            MatrixF surfMat;
            surfMat.mul( mConvexSEL->getOwner()->getWorldToObj(), gMat );

            MatrixF worldToObj ( mConvexSEL->getOwner()->getTransform() );
            worldToObj.scale( mConvexSEL->getOwner()->getScale() );
            worldToObj.inverse();

            Point3F newPos;            
            newPos = gMat.getPosition();      

            worldToObj.mulP( newPos );
            surfMat.setPosition( newPos );
            
            // Clear out floating point errors.
            cleanMatrix( surfMat );

            mConvexSEL->mSurfaces[mFaceSEL] = surfMat;

            updateShape( mConvexSEL, mFaceSEL );         
         }
         else
         {
            // Translating a face in x/y/z

            translateFace( mConvexSEL, mFaceSEL, mEditor->getGizmo()->getTotalOffset() );
         }
      }

      if ( isShapeValid( mConvexSEL ) )          
      {
         AssertFatal( mConvexSEL->mSurfaces.size() > mFaceSEL, "mFaceSEL out of range." );
         mLastValidShape = mConvexSEL->mSurfaces; 
      }
      else
      {
         AssertFatal( mLastValidShape.size() > mFaceSEL, "mFaceSEL out of range." );
         mConvexSEL->mSurfaces = mLastValidShape;
         updateShape( mConvexSEL );
      }

      return Done;
   }

   // Manipulating a whole Convex.

   if ( !mSavedUndo )
   {
      mSavedUndo = true;
      submitUndo( ModifyShape, mConvexSEL );
   }

   if ( mEditor->getGizmo()->getMode() == MoveMode )
   {
      mConvexSEL->getOwner()->setPosition( mEditor->getGizmo()->getPosition() );
   }
   else if ( mEditor->getGizmo()->getMode() == RotateMode )
   {   
      mConvexSEL->setTransform( mEditor->getGizmo()->getTransform() );      
   }
   else
   {
      mConvexSEL->getOwner()->setScale( mEditor->getGizmo()->getScale() );
   }   

	mConvexSEL->setMaskBits(ConvexShapeComponentInstance::UpdateMask);
   /*if ( mConvexSEL->getClientObject() )
   {
      ConvexShapeComponentInstance *clientObj = static_cast< ConvexShapeComponentInstance* >( mConvexSEL->getClientObject() );
      clientObj->setTransform( mConvexSEL->getOwner()->getTransform() );
      clientObj->setScale( mConvexSEL->getOwner()->getScale() );
   } */

   return Done;
}

void ConvexShapeComponentTool::renderScene(const RectI & updateRect)
{      
	// Synch selected ConvexShape with the WorldEditor.

	WorldEditor *wedit;
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
      if ( mConvexSEL && mConvexSEL->getSurfaces().size() > ConvexShapeComponentInstance::smMaxSurfaces )
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
	   mEditor->getGizmo()->renderGizmo( mEditor->getLastCameraQuery().cameraMatrix, mEditor->getLastCameraQuery().fov );
} 

void ConvexShapeComponentTool::updateGizmo()
{
	mEditor->getGizmo()->getProfile()->restoreDefaultState();
   
   const GizmoMode &mode = mEditor->getGizmo()->getProfile()->mode;
   S32 &flags = mEditor->getGizmo()->getProfile()->flags;
   GizmoAlignment &align = mEditor->getGizmo()->getProfile()->alignment;

   U8 keys = Input::getModifierKeys();

   mCtrlDown = keys & ( SI_LCTRL | SI_LSHIFT );

   bool altDown = keys & ( SI_LALT );

   if ( altDown )
   {
      flags = 0;
      return;
   }

   if ( mFaceSEL != -1 )
   {
      align = Object;    
      flags |= GizmoProfile::CanRotateUniform;
      flags &= ~GizmoProfile::CanRotateScreen;
   }
   else
   {
      flags &= ~GizmoProfile::CanRotateUniform;
      flags |= GizmoProfile::CanRotateScreen;
   }

   if ( mFaceSEL != -1 && mode == ScaleMode )
      flags &= ~GizmoProfile::CanScaleZ;
   else
      flags |= GizmoProfile::CanScaleZ;         

   if ( mFaceSEL != -1 && mode == MoveMode )
   {
      if ( mCtrlDown )      
         flags &= ~( GizmoProfile::CanTranslateX | GizmoProfile::CanTranslateY | GizmoProfile::PlanarHandlesOn );      
      else      
         flags |= ( GizmoProfile::CanTranslateX | GizmoProfile::CanTranslateY | GizmoProfile::PlanarHandlesOn );      
   }
}

void ConvexShapeComponentTool::scaleFace( ConvexShapeComponentInstance *shape, S32 faceId, Point3F scale )
{
   if ( !mHasGeometry )
   {
      mHasGeometry = true;
      
      mSavedGeometry = shape->mGeometry;
      mSavedSurfaces = shape->mSurfaces;      
   }
   else
   {
      shape->mGeometry = mSavedGeometry;
      shape->mSurfaces = mSavedSurfaces;
   }

   if ( shape->mGeometry.faces.size() <= faceId )
      return;
   
   ConvexShapeComponentInstance::Face &face = shape->mGeometry.faces[faceId];

   Vector< Point3F > &pointList = shape->mGeometry.points;

   AssertFatal( shape->mSurfaces[ face.id ].isAffine(), "ConvexShapeEditor - surface not affine." );
      
   Point3F projScale;
   scale.z = 1.0f;

   const MatrixF &surfToObj = shape->mSurfaces[ face.id ];
   MatrixF objToSurf( surfToObj );
   objToSurf.inverse();

   for ( S32 i = 0; i < face.points.size(); i++ )
   {                  
      Point3F &pnt = pointList[ face.points[i] ];   

      objToSurf.mulP( pnt );
      pnt *= scale;
      surfToObj.mulP( pnt );
   }

   updateModifiedFace( shape, faceId );
}

void ConvexShapeComponentTool::translateFace( ConvexShapeComponentInstance *shape, S32 faceId, const Point3F &displace )
{
   if ( !mHasGeometry )
   {
      mHasGeometry = true;

      mSavedGeometry = shape->mGeometry;
      mSavedSurfaces = shape->mSurfaces;      
   }
   else
   {
      shape->mGeometry = mSavedGeometry;
      shape->mSurfaces = mSavedSurfaces;
   }

   if ( shape->mGeometry.faces.size() <= faceId )
      return;

   ConvexShapeComponentInstance::Face &face = shape->mGeometry.faces[faceId];

   Vector< Point3F > &pointList = shape->mGeometry.points;

   AssertFatal( shape->mSurfaces[ face.id ].isAffine(), "ConvexShapeEditor - surface not affine." );

   // Transform displacement into object space.    
   MatrixF worldToObj( shape->getOwner()->getTransform() );
   worldToObj.scale( shape->getOwner()->getScale() );
   worldToObj.inverse();

   Point3F displaceOS;
   worldToObj.mulV( displace, &displaceOS );

   for ( S32 i = 0; i < face.points.size(); i++ )
   {                  
      Point3F &pnt = pointList[ face.points[i] ];   
      pnt += displaceOS;      
   }

   updateModifiedFace( shape, faceId );
}

void ConvexShapeComponentTool::updateModifiedFace( ConvexShapeComponentInstance *shape, S32 faceId )
{
   if ( shape->mGeometry.faces.size() <= faceId )
      return;

   ConvexShapeComponentInstance::Face &face = shape->mGeometry.faces[faceId];

   Vector< Point3F > &pointList = shape->mGeometry.points;

   Vector< ConvexShapeComponentInstance::Face > &faceList = shape->mGeometry.faces;

   for ( S32 i = 0; i < faceList.size(); i++ )
   {
      ConvexShapeComponentInstance::Face &curFace = faceList[i];      
      MatrixF &curSurface = shape->mSurfaces[ curFace.id ];

      U32 curPntCount = curFace.points.size();

      if ( curPntCount < 3 )
         continue;

      // Does this face use any of the points which we have modified?
      // Collect them in correct winding order.

      S32 pId0 = -1;

      for ( S32 j = 0; j < curFace.winding.size(); j++ )
      {
         if ( face.points.contains( curFace.points[ curFace.winding[ j ] ] ) )
         {
            pId0 = j;
            break;
         }
      }         

      if ( pId0 == -1 )
         continue;

      S32 pId1 = -1, pId2 = -1;

      pId1 = ( pId0 + 1 ) % curFace.winding.size();
      pId2 = ( pId0 + 2 ) % curFace.winding.size();

      const Point3F &p0 = pointList[ curFace.points[ curFace.winding[ pId0 ] ] ];
      const Point3F &p1 = pointList[ curFace.points[ curFace.winding[ pId1 ] ] ];
      const Point3F &p2 = pointList[ curFace.points[ curFace.winding[ pId2 ] ] ];

      PlaneF newPlane( p0, p1, p2 );
      Point3F uvec = newPlane.getNormal();
      Point3F fvec = curSurface.getForwardVector();
      Point3F rvec = curSurface.getRightVector();

      F32 dt0 = mDot( uvec, fvec );
      F32 dt1 = mDot( uvec, rvec );

      if ( mFabs( dt0 ) < mFabs( dt1 ) )
      {
         rvec = mCross( fvec, uvec );
         rvec.normalizeSafe();
         fvec = mCross( uvec, rvec );
         fvec.normalizeSafe();
      }
      else
      {
         fvec = mCross( uvec, rvec );
         fvec.normalizeSafe();
         rvec = mCross( fvec, uvec );
         rvec.normalizeSafe();
      }

      curSurface.setColumn( 0, rvec );
      curSurface.setColumn( 1, fvec );
      curSurface.setColumn( 2, uvec );   
      curSurface.setPosition( newPlane.getPosition() );
   }

   updateShape( shape );
}

bool ConvexShapeComponentTool::isShapeValid( ConvexShapeComponentInstance *shape )
{
   // Test for no-geometry.
   if ( shape->mGeometry.points.empty() )
      return false;

   const Vector<Point3F> &pointList = shape->mGeometry.points;
   const Vector<ConvexShapeComponentInstance::Face> &faceList = shape->mGeometry.faces;

   // Test that all points are shared by at least 3 faces.

   for ( S32 i = 0; i < pointList.size(); i++ )
   {
      U32 counter = 0;

      for ( S32 j = 0; j < faceList.size(); j++ )
      {
         if ( faceList[j].points.contains( i ) )
            counter++;
      }

      if ( counter < 3 )
         return false;
   }

   // Test for co-planar faces.
   for ( S32 i = 0; i < shape->mPlanes.size(); i++ )
   {
      for ( S32 j = i + 1; j < shape->mPlanes.size(); j++ )
      {
         F32 d = mDot( shape->mPlanes[i], shape->mPlanes[j] );
         if ( d > 0.999f )         
            return false;         
      }
   }

   // Test for faces with zero or negative area.
   for ( S32 i = 0; i < shape->mGeometry.faces.size(); i++ )
   {
      if ( shape->mGeometry.faces[i].area < 0.0f )
         return false;

      if ( shape->mGeometry.faces[i].triangles.empty() )
         return false;
   }

   return true;
}

void ConvexShapeComponentTool::setupShape( ConvexShapeComponentInstance *shape )
{
   shape->setField( "material", mMaterialName );
   shape->registerObject();
   updateShape( shape );

   SimGroup *group;
   if ( Sim::findObject( "missionGroup", group ) )
      group->addObject( shape );
}

void ConvexShapeComponentTool::updateShape( ConvexShapeComponentInstance *shape, S32 offsetFace )
{
   shape->_updateGeometry( true );

   
   //if ( offsetFace != -1 )
   //{
   //   shape->mSurfaces[ offsetFace ].setPosition( mPivotPos );
   //}

	shape->setMaskBits(ConvexShapeComponentInstance::UpdateMask);
   synchClientObject( shape );
}

void ConvexShapeComponentTool::synchClientObject( const ConvexShapeComponentInstance *serverConvex )
{
	return;
   if ( serverConvex->getClientObject() )
   {
      ConvexShapeComponentInstance *clientConvex = static_cast< ConvexShapeComponentInstance* >( serverConvex->getClientObject() );
      //clientConvex->setScale( serverConvex->mBehaviorOwner->getScale() );
      //clientConvex->setTransform( serverConvex->mBehaviorOwner->getTransform() );
      clientConvex->mSurfaces.clear();
      clientConvex->mSurfaces.merge( serverConvex->mSurfaces );
      clientConvex->_updateGeometry(true);
   }
}

void ConvexShapeComponentTool::updateGizmoPos()
{
   if ( mConvexSEL )
   {
      if ( mFaceSEL != -1 )
      {
         MatrixF surfMat = mConvexSEL->getSurfaceWorldMat( mFaceSEL );  

         MatrixF objToWorld( mConvexSEL->getOwner()->getTransform() );
         objToWorld.scale( mConvexSEL->getOwner()->getScale() );

         Point3F gizmoPos(0,0,0);

         if ( mUsingPivot )
         {
            gizmoPos = mPivotPos;
         }
         else
         {
            Point3F faceCenterPnt = mConvexSEL->mSurfaces[ mFaceSEL ].getPosition();
            objToWorld.mulP( faceCenterPnt );

            mGizmoMatOffset = surfMat.getPosition() - faceCenterPnt;

            gizmoPos = faceCenterPnt;
         }

         mEditor->getGizmo()->set( surfMat, gizmoPos, Point3F::One );        
      }
      else
      {
         mGizmoMatOffset = Point3F::Zero;
         mEditor->getGizmo()->set( mConvexSEL->getOwner()->getTransform(), mConvexSEL->getOwner()->getPosition(), mConvexSEL->getOwner()->getScale() ); 
      }
   }   
}

void ConvexShapeComponentTool::setSelection( ConvexShapeComponentInstance *shape, S32 faceId )
{
   mFaceSEL = faceId;
   mConvexSEL = shape;
   updateGizmoPos();

   Con::executef( this, "onSelectionChanged", shape ? shape->getIdString() : "", Con::getIntArg(faceId) );
}

void ConvexShapeComponentTool::submitUndo( UndoType type, ConvexShapeComponentInstance *shape )
{
   Vector< ConvexShapeComponentInstance* > shapes;
   shapes.push_back( shape );
   submitUndo( type, shapes );
}

void ConvexShapeComponentTool::submitUndo( UndoType type, const Vector<ConvexShapeComponentInstance*> &shapes )
{   
   // Grab the mission editor undo manager.
   Sim::findObject( "EUndoManager", mUndoManager );   
   
   if ( !mUndoManager )   
   {
      Con::errorf( "GuiConvexEditorCtrl::submitUndo() - EUndoManager not found!" );
      return;           
   }

   if ( type == ModifyShape )
   {
      // Setup the action.
      ConvexShapeComponentUndo *action = new ConvexShapeComponentUndo( "Modified a ConvexShape" );

      ConvexShapeComponentInstance *shape = shapes.first();

      action->mObjId = shape->getId();   
      action->mEditor = this;   
      action->mSavedObjToWorld = shape->getOwner()->getTransform();
      action->mSavedScale = shape->getOwner()->getScale();
      action->mSavedSurfaces.merge( shape->mSurfaces );             
      action->mUndoManager = mUndoManager;

      mUndoManager->addAction( action );

      mLastUndo = action;
   }
   else if ( type == CreateShape )
   {
      MECreateUndoAction *action = new MECreateUndoAction( "Create ConvexShape" );

      for ( S32 i = 0; i < shapes.size(); i++ )
         action->addObject( shapes[i] );
         
      mUndoManager->addAction( action );
      
      mLastUndo = action;
   }
   else if ( type == DeleteShape )
   {
      MEDeleteUndoAction *action = new MEDeleteUndoAction( "Deleted ConvexShape" );

      for ( S32 i = 0; i < shapes.size(); i++ )
         action->deleteObject( shapes[i] );         

      mUndoManager->addAction( action );

      mLastUndo = action;
   }
   else if ( type == HollowShape )
   {
      CompoundUndoAction *action = new CompoundUndoAction( "Hollow ConvexShape" );

      MECreateUndoAction *createAction = new MECreateUndoAction();
      MEDeleteUndoAction *deleteAction = new MEDeleteUndoAction();

      deleteAction->deleteObject( shapes.first() );
      
      for ( S32 i = 1; i < shapes.size(); i++ )      
         createAction->addObject( shapes[i] );
      
      action->addAction( deleteAction );
      action->addAction( createAction );

      mUndoManager->addAction( action );

      mLastUndo = action;
   }

	mIsDirty = true;
}

void ConvexShapeComponentTool::setPivotPos( ConvexShapeComponentInstance *shape, S32 faceId, const Gui3DMouseEvent &event )
{
   PlaneF plane;
   mTransformPlane( shape->getOwner()->getTransform(), shape->getOwner()->getScale(), shape->mPlanes[ faceId ], &plane );

   Point3F start( event.pos );
   Point3F end( start + event.vec * 10000.0f );

   F32 t = plane.intersect( start, end );

   if ( t >= 0.0f && t <= 1.0f )
   {
      Point3F hitPos;
      hitPos.interpolate( start, end, t );

      mPivotPos = hitPos;
      mUsingPivot = true;

      MatrixF worldToObj( shape->getOwner()->getTransform() );
      worldToObj.scale( shape->getOwner()->getScale() );
      worldToObj.inverse();

      Point3F objPivotPos( mPivotPos );
      worldToObj.mulP( objPivotPos );

      updateGizmoPos();
   }
}

void ConvexShapeComponentTool::cleanMatrix( MatrixF &mat )
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

S32 ConvexShapeComponentTool::getEdgeByPoints( ConvexShapeComponentInstance *shape, S32 faceId, S32 p0, S32 p1 )
{
   const ConvexShapeComponentInstance::Face &face = shape->mGeometry.faces[faceId];

   for ( S32 i = 0; i < face.edges.size(); i++ )
   {
      const ConvexShapeComponentInstance::Edge &edge = face.edges[i];

      if ( edge.p0 != p0 && edge.p0 != p1 )
         continue;
      if ( edge.p1 != p0 && edge.p1 != p1 )
         continue;

      return i;      
   }

   return -1;
}

bool ConvexShapeComponentTool::getEdgesTouchingPoint( ConvexShapeComponentInstance *shape, S32 faceId, S32 pId, Vector< U32 > &edgeIdxList, S32 excludeEdge )
{
   const ConvexShapeComponentInstance::Face &face = shape->mGeometry.faces[faceId];   
   const Vector< ConvexShapeComponentInstance::Edge > &edgeList = face.edges;

   for ( S32 i = 0; i < edgeList.size(); i++ )
   {
      if ( i == excludeEdge )
         continue;

      const ConvexShapeComponentInstance::Edge &curEdge = edgeList[i];

      if ( curEdge.p0 == pId || curEdge.p1 == pId )      
         edgeIdxList.push_back(i);
   }

   return !edgeIdxList.empty();
}

bool ConvexShapeComponentTool::handleEscape()
{
   if ( mCreateTool )
   {
      mCreateTool->onDeactivated( NULL );
      mCreateTool = NULL;

      return true;
   }

   if ( mFaceSEL != -1 )
   {
      setSelection( mConvexSEL, -1 );
      return true;
   }

   if ( mConvexSEL )
   {         
      setSelection( NULL, -1 );
      return true;
   }

   return false;
}

bool ConvexShapeComponentTool::_cursorCastCallback( RayInfo* ri )
{
   // Reject anything that's not a ConvexShape.
   Entity* e = dynamic_cast< Entity* >( ri->object );
   if(e)
   {
	   //if we're in the tool, we'll be looking for the convex shape specifically. Check the user data on the rayInfo
	   if(ri->userData)
	   {
		   return true;
	   }
   }
}

bool ConvexShapeComponentTool::_cursorCast( const Gui3DMouseEvent &event, ConvexShapeComponentInstance **hitShape, S32 *hitFace )
{
   RayInfo ri;
   
   if ( gServerContainer.castRay( event.pos, event.pos + event.vec * 10000.0f, U32(-1), &ri, &ConvexShapeComponentTool::_cursorCastCallback ) &&
        dynamic_cast< Entity* >( ri.object ) )
   {
      // Do not select or edit ConvexShapes that are within a Prefab.
      if ( Prefab::getPrefabByChild( ri.object ) )
         return false;

	  Entity *e = dynamic_cast< Entity* >( ri.object );

      //*hitShape = static_cast< ConvexShapeComponentInstance* >( ri.object );
	   *hitShape = e->getComponent<ConvexShapeComponentInstance>();
      *hitFace = ri.face;
      mLastRayInfo = ri;

      return true;
   }

   return false;
}
//
//
void ConvexShapeComponentUndo::undo()
{
   ConvexShapeComponentInstance *object = NULL;
   if ( !Sim::findObject( mObjId, object ) )
      return;

   // Temporarily save the ConvexShape current data.   
   Vector< MatrixF > tempSurfaces;   
   tempSurfaces.merge( object->mSurfaces );
   MatrixF tempObjToWorld( object->getOwner()->getTransform() );
   Point3F tempScale( object->getOwner()->getScale() );

   // Restore the Object to the UndoAction state.
   object->mSurfaces.clear();
   object->mSurfaces.merge( mSavedSurfaces );   
   object->getOwner()->setScale( mSavedScale );
   object->getOwner()->setTransform( mSavedObjToWorld );

   // Regenerate the ConvexShape and synch the client object.
   object->_updateGeometry();
   //ConvexShapeComponentTool::synchClientObject( object );

   // If applicable set the selected ConvexShape and face
   // on the editor.   
   //mEditor->mEditor->setSelection( object, -1 );
   mEditor->updateGizmoPos();

   // Now save the previous ConvexShape data in this UndoAction
   // since an undo action must become a redo action and vice-versa
   
   mSavedObjToWorld = tempObjToWorld;
   mSavedScale = tempScale;
   mSavedSurfaces.clear();
   mSavedSurfaces.merge( tempSurfaces );   
}
//
//================================================================================
// Create tool
//================================================================================
ConvexShapeCreateTool::ConvexShapeCreateTool(  )
{
	mStage = -1;
	mNewConvex = NULL;
}

void ConvexShapeCreateTool::onActivated( BaseTool *prevTool )
{
   //mEditor->clearSelection();
   mStage = -1;
   mNewConvex = NULL;
	mPrevTool = prevTool;
}

void ConvexShapeCreateTool::onDeactivated( BaseTool *newTool )
{
   if ( mNewConvex )
      mNewConvex->deleteObject();

   mStage = -1;
   mNewConvex = NULL;
	mPrevTool = NULL;
   mEditor->mouseUnlock();
}

BaseTool::EventResult ConvexShapeCreateTool::on3DMouseDown( const Gui3DMouseEvent &event )
{
   if ( mStage == -1 )
   {
		//if we don't have a template, don't even bother
		if(!mConvexBehaviorTemplate)
			return Done;

      mEditor->setFirstResponder();
      mEditor->mouseLock();

      Point3F start( event.pos );
      Point3F end( event.pos + event.vec * 10000.0f );      
      RayInfo ri;
      
		bool hit = gServerContainer.castRay( event.pos, end, U32(-1), &ri);//, &ConvexShapeComponentTool::_cursorCastCallback );
      //gServerContainer.castRay( event.pos, end, STATIC_COLLISION_TYPEMASK, &ri );

      MatrixF objMat( true );

      // Calculate the orientation matrix of the new ConvexShape
      // based on what has been clicked.

      if ( !hit )
      {
         objMat.setPosition( event.pos + event.vec * 100.0f );      
      }
      else
      {
			Entity *e = dynamic_cast< Entity* >( ri.object );

			if(e)
			{
            ConvexShapeComponentInstance *hitShape = e->getComponent<ConvexShapeComponentInstance>();
            objMat = hitShape->getSurfaceWorldMat( ri.face );
            objMat.setPosition( ri.point );
         }
         else
         {
            Point3F rvec;
            Point3F fvec( mEditor->getLastCameraQuery().cameraMatrix.getForwardVector() );
            Point3F uvec( ri.normal );

            rvec = mCross( fvec, uvec );

            if ( rvec.isZero() )
            {
               fvec = mEditor->getLastCameraQuery().cameraMatrix.getRightVector();
               rvec = mCross( fvec, uvec );
            }

            rvec.normalizeSafe();
            fvec = mCross( uvec, rvec );
            fvec.normalizeSafe();
            uvec = mCross( rvec, fvec );
            uvec.normalizeSafe();

            objMat.setColumn( 0, rvec );
            objMat.setColumn( 1, fvec );
            objMat.setColumn( 2, uvec );

            objMat.setPosition( ri.point );
         }
      }

		Entity* newConvexOwner = new Entity();
		newConvexOwner->registerObject();

		mNewConvex = dynamic_cast<ConvexShapeComponentInstance*>(mConvexBehaviorTemplate->createInstance());

		newConvexOwner->addComponent(mNewConvex);		

      newConvexOwner->setTransform( objMat );   
		
		mNewConvex->setField( "material", mMaterialName );
		
      mPlaneSizes.set( 0.1f, 0.1f, 0.1f );
      mNewConvex->resizePlanes( mPlaneSizes );

      updateShape( mNewConvex );
      
      mTransform = objMat;     

      mCreatePlane.set( objMat.getPosition(), objMat.getUpVector() );
   }
   else if ( mStage == 0 )
   {
      // Handle this on mouseUp
   }
   
   return Handled;
}

BaseTool::EventResult ConvexShapeCreateTool::on3DMouseUp( const Gui3DMouseEvent &event )
{
   if ( mNewConvex && mStage == -1 )
   {
      mStage = 0;      

      mCreatePlane = PlaneF( mNewConvex->getOwner()->getPosition(), mNewConvex->getOwner()->getTransform().getForwardVector() );

      mTransform.setPosition( mNewConvex->getOwner()->getPosition() );      

      return Handled;
   }
   else if ( mStage == 0 )
   {
      SimGroup *mg;
      Sim::findObject( "MissionGroup", mg );

		mg->addObject( mNewConvex->getOwner() );

      mStage = -1;

      // Grab the mission editor undo manager.
      UndoManager *undoMan = NULL;
      if ( !Sim::findObject( "EUndoManager", undoMan ) )
      {
         Con::errorf( "ConvexShapeCreateTool::on3DMouseDown() - EUndoManager not found!" );
         mNewConvex = NULL;
         return Failed;           
      }

      // Create the UndoAction.
      MECreateUndoAction *action = new MECreateUndoAction("Create ConvexShape");
      action->addObject( mNewConvex );

      // Submit it.               
      undoMan->addAction( action );

      mEditor->setField( "isDirty", "1" );

      //mEditor->setSelection( mNewConvex, -1 );      

      mNewConvex = NULL;

      mEditor->mouseUnlock();

		mEditor->setActiveTool(mPrevTool);
      return Done;
   }

   return Done;
}

BaseTool::EventResult ConvexShapeCreateTool::on3DMouseMove( const Gui3DMouseEvent &event )
{
   if ( mStage == 0 )
   {
      Point3F start( event.pos );
      Point3F end( start + event.vec * 10000.0f );
      
      F32 t = mCreatePlane.intersect( start, end );

      Point3F hitPos;

      if ( t < 0.0f || t > 1.0f )
         return Handled;

      hitPos.interpolate( start, end, t );      

      MatrixF worldToObj( mTransform );
      worldToObj.inverse();
      worldToObj.mulP( hitPos );

      F32 delta = ( hitPos.z );

      mPlaneSizes.z = getMax( 0.1f, delta );

      mNewConvex->resizePlanes( mPlaneSizes );

      updateShape( mNewConvex );

      Point3F pos( mTransform.getPosition() );
      pos += mPlaneSizes.z * 0.5f * mTransform.getUpVector();
      mNewConvex->getOwner()->setPosition( pos );
   }

   return Handled;
}

BaseTool::EventResult ConvexShapeCreateTool::on3DMouseDragged( const Gui3DMouseEvent &event )
{
   if ( !mNewConvex || mStage != -1 )
      return Handled;

   Point3F start( event.pos );
   Point3F end( event.pos + event.vec * 10000.0f );

   F32 t = mCreatePlane.intersect( start, end );

   if ( t < 0.0f || t > 1.0f )
      return Handled;

   Point3F hitPos;
   hitPos.interpolate( start, end, t );
   
   MatrixF xfm( mTransform );
   xfm.inverse();      
   xfm.mulP( hitPos);      
   
   Point3F scale;
   scale.x = getMax( mFabs( hitPos.x ), 0.1f );
   scale.y = getMax( mFabs( hitPos.y ), 0.1f );
   scale.z = 0.1f;

   mNewConvex->resizePlanes( scale );
   mPlaneSizes = scale;
   updateShape( mNewConvex );   

   Point3F pos( mTransform.getPosition() );
   pos += mTransform.getRightVector() * hitPos.x * 0.5f;
   pos += mTransform.getForwardVector() * hitPos.y * 0.5f;

   mNewConvex->getOwner()->setPosition( pos );

   return Handled;
}

void ConvexShapeCreateTool::renderScene( const RectI &updateRect )
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
      if ( mConvexSEL && mConvexSEL->getSurfaces().size() > ConvexShapeComponentInstance::smMaxSurfaces )
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

ConvexShapeComponentInstance* ConvexShapeCreateTool::extrudeShapeFromFace( ConvexShapeComponentInstance *inShape, S32 inFaceId )
{
	if(!mConvexBehaviorTemplate)
		return NULL;

   ConvexShapeComponentInstance::MeshGeometry inShapeGeometry = *inShape->getConvexGeometry();
   ConvexShapeComponentInstance::Face &inFace = inShapeGeometry.faces[inFaceId];
   Vector< Point3F > &inShapePointList = inShapeGeometry.points;
   Vector< MatrixF > &inShapeSurfaces = inShape->getSurfaces();
   
   S32 shapeFaceCount = inFace.edges.size() + 2;   
   
   MatrixF inShapeToWorld( inShape->getOwner()->getTransform() );
   inShapeToWorld.scale( inShape->getOwner()->getScale() );
   //MatrixF inWorldToShape( inShapeToWorld );
   //inWorldToShape.inverse();

   MatrixF shapeToWorld;
   shapeToWorld.mul( inShape->getOwner()->getTransform(), inShapeSurfaces[inFaceId] );
   Point3F tmp( inShapeSurfaces[inFaceId].getPosition() );
   inShapeToWorld.mulP( tmp );
   shapeToWorld.setPosition( tmp );
   MatrixF worldToShape( shapeToWorld );
   worldToShape.inverse();

   MatrixF inShapeToNewShape;
   inShapeToNewShape.mul( inShapeToWorld, worldToShape );   

	Entity* newConvexEntity = new Entity();
	newConvexEntity->registerObject();

	ConvexShapeComponentInstance *newShape = new ConvexShapeComponentInstance(inShape->getTemplate());   

	newShape->registerObject();

	newConvexEntity->addComponent(newShape);

	newConvexEntity->setTransform( shapeToWorld ); 

   Vector< MatrixF > &shapeSurfaces = newShape->getSurfaces();
   shapeSurfaces.setSize( shapeFaceCount );
   //shapeSurfaces.setSize( 2 );

   const Point3F &shapePos = shapeToWorld.getPosition();
   
   shapeSurfaces[0].identity();
   shapeSurfaces[1].identity();   
   shapeSurfaces[1].setColumn( 0, -shapeSurfaces[1].getColumn3F(0) );
   shapeSurfaces[1].setColumn( 2, -shapeSurfaces[1].getColumn3F(2) );

   for ( S32 i = 0; i < inFace.winding.size(); i++ )
   {      
      Point3F p0 = inShapePointList[ inFace.points[ inFace.winding[ i ] ] ];
      Point3F p1;
      
      if ( i+1 < inFace.winding.size() )
         p1 = inShapePointList[ inFace.points[ inFace.winding[ i+1 ] ] ];
      else
         p1 = inShapePointList[ inFace.points[ inFace.winding[ 0 ] ] ];

      inShapeToWorld.mulP( p0 );
      inShapeToWorld.mulP( p1 );

      Point3F newPos = MathUtils::mClosestPointOnSegment( p0, p1, shapePos );      

      Point3F rvec = p0 - p1;
      rvec.normalizeSafe();

      Point3F fvec = shapeToWorld.getUpVector();

      Point3F uvec = mCross( rvec, fvec );      

      if ( i + 2 >= shapeSurfaces.size() )
         continue;
      
      //F32 dt = mDot( shapeToWorld.getUpVector(), rvec );
      //AssertFatal( mIsZero( dt ), "bad" );
      
      MatrixF &surf = shapeSurfaces[i+2];
      surf.identity();
      surf.setColumn( 0, rvec );
      surf.setColumn( 1, fvec );
      surf.setColumn( 2, uvec );
      surf.setPosition( newPos );

      surf.mulL( worldToShape );      
   }

	newShape->setField( "material", mMaterialName );

   updateShape( newShape );

   SimGroup *group;
   if ( Sim::findObject( "missionGroup", group ) )
      group->addObject( newConvexEntity );

   return newShape;
}

void ConvexShapeCreateTool::updateShape(ConvexShapeComponentInstance* shape, S32 offsetFace)
{
	shape->_updateGeometry( true );

	shape->setMaskBits(ConvexShapeComponentInstance::UpdateMask);
	shape->getOwner()->updateComponents();
}