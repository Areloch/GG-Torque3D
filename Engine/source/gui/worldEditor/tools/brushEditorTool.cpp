#include "brushEditorTool.h"
#include "gfx/gfxDrawUtil.h"

IMPLEMENT_CONOBJECT(BrushEditorTool);

BrushEditorTool::BrushEditorTool()
{
   mWorldEditor = NULL;

   mUseMouseDown = true;
   mUseMouseUp = true;
   mUseMouseMove = true;

   mUseRightMouseDown = false;
   mUseRightMouseUp = false;
   mUseRightMouseMove = false;

   mUseMiddleMouseDown = true;
   mUseMiddleMouseUp = true;
   mUseMiddleMouseMove = true;

   mUseKeyInput = true;

   mMouseDown = false;
   mCreateMode = false;
   mCreateStage = -1;
   mBrushObjHL = NULL;
   mBrushHL = -1;
   mFaceHL = -1;
}

bool BrushEditorTool::onAdd()
{
   return Parent::onAdd();
}

void BrushEditorTool::onRemove()
{
   Parent::onRemove();
}

//Called when the tool is activated on the World Editor
void BrushEditorTool::onActivated(WorldEditor* editor)
{
   mWorldEditor = editor;
   Con::executef(this, "onActivated");
}

//Called when the tool is deactivated on the World Editor
void BrushEditorTool::onDeactivated()
{
   mWorldEditor = NULL;
   Con::executef(this, "onDeactivated");
}

//
bool BrushEditorTool::_cursorCastCallback(RayInfo* ri)
{
   // Reject anything that's not a ConvexShape.
   return dynamic_cast< BrushObject* >(ri->object);
}

bool BrushEditorTool::_cursorCast(const Gui3DMouseEvent &event, BrushObject **hitShape, S32 *hitBrush, S32 *hitFace)
{
   RayInfo ri;

   if (gServerContainer.castRay(event.pos, event.pos + event.vec * 10000.0f, StaticShapeObjectType, &ri, &BrushEditorTool::_cursorCastCallback) &&
      dynamic_cast< BrushObject* >(ri.object))
   {
      *hitShape = static_cast< BrushObject* >(ri.object);
      *hitBrush = (S32)ri.userData;
      *hitFace = ri.face;
      mLastRayInfo = ri;

      return true;
   }

   return false;
}
//
bool BrushEditorTool::onMouseMove(const Gui3DMouseEvent &e)
{
   if (!mUseMouseMove)
      return false;

   /*if (mActiveTool)
   {
      // If we have an active tool pass this event to it.
      // If it handled it, consume the event.
      if (mActiveTool->on3DMouseMove(event))
         return;
   }*/

   BrushObject *hitShape = NULL;
   S32 hitFace = -1;
   S32 hitBrush = -1;

   _cursorCast(e, &hitShape, &hitBrush, &hitFace);

   //visualiiiiiize
   if (hitShape != NULL)
   {
      mBrushObjHL = hitShape;
      mBrushHL = hitBrush;
      mFaceHL = hitFace;
   }
   else
   {
      mBrushObjHL = NULL;
      mBrushHL = -1;
      mFaceHL = -1;
   }

   /*if (!mConvexSEL)
   {
      mConvexHL = hitShape;
      mFaceHL = -1;
   }
   else
   {
      if (mConvexSEL == hitShape)
      {
         mConvexHL = hitShape;
         mFaceHL = hitFace;
      }
      else
      {
         // Mousing over a shape that is not the one currently selected.

         if (mFaceSEL != -1)
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

   if (mConvexSEL)
      mGizmo->on3DMouseMove(event);*/
   return true;
}
bool BrushEditorTool::onMouseDown(const Gui3DMouseEvent &e)
{
   if (!mUseMouseDown)
      return false;

   /*mWorldEditor->mouseLock();

   mMouseDown = true;

   if (e.modifier & SI_ALT)
   {
      mCreateMode = true;

      if (mCreateStage == -1)
      {
         //mEditor->setFirstResponder();
         //mEditor->mouseLock();

         Point3F start(e.pos);
         Point3F end(e.pos + e.vec * 10000.0f);
         RayInfo ri;

         bool hit = gServerContainer.castRay(e.pos, end, STATIC_COLLISION_TYPEMASK, &ri);

         MatrixF objMat(true);

         // Calculate the orientation matrix of the new ConvexShape
         // based on what has been clicked.

         if (!hit)
         {
            objMat.setPosition(e.pos + e.vec * 100.0f);
         }
         else
         {
            if (dynamic_cast< ConvexShape* >(ri.object))
            {
               ConvexShape *hitShape = static_cast< ConvexShape* >(ri.object);
               objMat = hitShape->getSurfaceWorldMat(ri.face);
               objMat.setPosition(ri.point);
            }
            else
            {
               Point3F rvec;
               Point3F fvec(mWorldEditor->getCameraMat().getForwardVector());
               Point3F uvec(ri.normal);

               rvec = mCross(fvec, uvec);

               if (rvec.isZero())
               {
                  fvec = mEditor->getCameraMat().getRightVector();
                  rvec = mCross(fvec, uvec);
               }

               rvec.normalizeSafe();
               fvec = mCross(uvec, rvec);
               fvec.normalizeSafe();
               uvec = mCross(rvec, fvec);
               uvec.normalizeSafe();

               objMat.setColumn(0, rvec);
               objMat.setColumn(1, fvec);
               objMat.setColumn(2, uvec);

               objMat.setPosition(ri.point);
            }
         }

         mNewConvex = new Brush();

         mNewConvex->setTransform(objMat);

         mNewConvex->setField("material", Parent::mEditor->mMaterialName);

         mNewConvex->registerObject();
         mPlaneSizes.set(0.1f, 0.1f, 0.1f);
         mNewConvex->resizePlanes(mPlaneSizes);
         mEditor->updateShape(mNewConvex);

         mTransform = objMat;

         mCreatePlane.set(objMat.getPosition(), objMat.getUpVector());
      }
      else if (mStage == 0)
      {
         // Handle this on mouseUp
      }

      return;
   }*/

   //if (mConvexSEL && isShapeValid(mConvexSEL))
   //if (!mSelectedBrushes.empty())
   //   mLastValidShape = mConvexSEL->mSurfaces;

   /*if (mConvexSEL &&
      mFaceSEL != -1 &&
      mWorldEditor->getGizmo()->getMode() == RotateMode &&
      mWorldEditor->getGizmo()->getSelection() == Gizmo::Centroid)
   {
      mSettingPivot = true;
      mSavedPivotPos = mWorldEditor->getGizmo()->getPosition();
      setPivotPos(mConvexSEL, mFaceSEL, e);
      updateGizmoPos();
      return;
   }*/

   mWorldEditor->getGizmo()->on3DMouseDown(e);
   return true;
}
bool BrushEditorTool::onMouseDragged(const Gui3DMouseEvent &e)
{
   /*if (mCreateMode)
   {
      if (!mNewConvex || mStage != -1)
         return Handled;

      Point3F start(e.pos);
      Point3F end(e.pos + e.vec * 10000.0f);

      F32 t = mCreatePlane.intersect(start, end);

      if (t < 0.0f || t > 1.0f)
         return Handled;

      Point3F hitPos;
      hitPos.interpolate(start, end, t);

      MatrixF xfm(mTransform);
      xfm.inverse();
      xfm.mulP(hitPos);

      Point3F scale;
      scale.x = getMax(mFabs(hitPos.x), 0.1f);
      scale.y = getMax(mFabs(hitPos.y), 0.1f);
      scale.z = 0.1f;

      mNewConvex->resizePlanes(scale);
      mPlaneSizes = scale;
      mEditor->updateShape(mNewConvex);

      Point3F pos(mTransform.getPosition());
      pos += mTransform.getRightVector() * hitPos.x * 0.5f;
      pos += mTransform.getForwardVector() * hitPos.y * 0.5f;

      mNewConvex->setPosition(pos);
      return;
   }

   //mGizmoProfile->rotateScalar = 0.55f;
   //mGizmoProfile->scaleScalar = 0.55f;

   if (!mConvexSEL)
      return;

   if (mGizmo->getMode() == RotateMode &&
      mGizmo->getSelection() == Gizmo::Centroid)
   {
      setPivotPos(mConvexSEL, mFaceSEL, event);
      mDragging = true;
      return;
   }

   mGizmo->on3DMouseDragged(event);

   if (event.modifier & SI_SHIFT &&
      (mGizmo->getMode() == MoveMode || mGizmo->getMode() == RotateMode) &&
      !mHasCopied)
   {
      if (mFaceSEL != -1)
      {
         ConvexShape *newShape = mCreateTool->extrudeShapeFromFace(mConvexSEL, mFaceSEL);
         //newShape->_updateGeometry();

         submitUndo(CreateShape, newShape);
         setSelection(newShape, 0);
         updateGizmoPos();

         mGizmo->on3DMouseDown(event);

         mHasCopied = true;
         mSavedUndo = true;
      }
      else
      {
         ConvexShape *newShape = new ConvexShape();
         newShape->setTransform(mConvexSEL->getTransform());
         newShape->setScale(mConvexSEL->getScale());
         newShape->mSurfaces.clear();
         newShape->mSurfaces.merge(mConvexSEL->mSurfaces);

         setupShape(newShape);

         submitUndo(CreateShape, newShape);

         setSelection(newShape, -1);

         updateGizmoPos();

         mHasCopied = true;
         mSavedUndo = true;
      }

      return;
   }

   if (mGizmo->getMode() == RotateMode &&
      event.modifier & SI_CTRL &&
      !mHasCopied &&
      mFaceSEL != -1)
   {
      // Can must verify that splitting the face at the current angle 
      // ( of the gizmo ) will generate a valid shape.  If not enough rotation
      // has occurred we will have two faces that are coplanar and must wait
      // until later in the drag to perform the split.

      //AssertFatal( isShapeValid( mConvexSEL ), "Shape was already invalid at beginning of split operation." );

      if (!isShapeValid(mConvexSEL))
         return;

      mLastValidShape = mConvexSEL->mSurfaces;

      Point3F rot = mGizmo->getDeltaTotalRot();
      rot.normalize();
      rot *= mDegToRad(10.0f);

      MatrixF rotMat((EulerF)rot);

      MatrixF worldToObj(mConvexSEL->getTransform());
      worldToObj.scale(mConvexSEL->getScale());
      worldToObj.inverse();

      mConvexSEL->mSurfaces.increment();
      MatrixF &newSurf = mConvexSEL->mSurfaces.last();
      newSurf = mConvexSEL->mSurfaces[mFaceSEL] * rotMat;

      //worldToObj.mul( mGizmo->getTransform() );
      //Point3F pos( mPivotPos );
      //worldToObj.mulP( pos );
      //newSurf.setPosition( pos );

      updateShape(mConvexSEL);

      if (!isShapeValid(mConvexSEL))
      {
         mConvexSEL->mSurfaces = mLastValidShape;
         updateShape(mConvexSEL);
      }
      else
      {
         mHasCopied = true;
         mSavedUndo = true;

         mLastValidShape = mConvexSEL->mSurfaces;

         submitUndo(ModifyShape, mConvexSEL);

         setSelection(mConvexSEL, mConvexSEL->mSurfaces.size() - 1);

         updateGizmoPos();
      }

      return;
   }

   // If we are dragging, but no gizmo selection...
   // Then treat this like a regular mouse move, update the highlighted
   // convex/face under the cursor and handle onMouseUp as we normally would
   // to change the selection.
   if (mGizmo->getSelection() == Gizmo::None)
   {
      ConvexShape *hitShape = NULL;
      S32 hitFace = -1;

      _cursorCast(event, &hitShape, &hitFace);
      mFaceHL = hitFace;
      mConvexHL = hitShape;

      return;
   }

   mDragging = true;

   // Manipulating a face.

   if (mFaceSEL != -1)
   {
      if (!mSavedUndo)
      {
         mSavedUndo = true;
         submitUndo(ModifyShape, mConvexSEL);
      }

      if (mGizmo->getMode() == ScaleMode)
      {
         scaleFace(mConvexSEL, mFaceSEL, mGizmo->getScale());
      }
      else
      {
         // Why does this have to be so ugly.
         if (mGizmo->getMode() == RotateMode ||
            (mGizmo->getMode() == MoveMode &&
            (event.modifier & SI_CTRL ||
            (mGizmo->getSelection() == Gizmo::Axis_Z && mHasCopied)
            )
            )
            )
         {
            const MatrixF &gMat = mGizmo->getTransform();
            MatrixF surfMat;
            surfMat.mul(mConvexSEL->mWorldToObj, gMat);

            MatrixF worldToObj(mConvexSEL->getTransform());
            worldToObj.scale(mConvexSEL->getScale());
            worldToObj.inverse();

            Point3F newPos;
            newPos = gMat.getPosition();

            worldToObj.mulP(newPos);
            surfMat.setPosition(newPos);

            // Clear out floating point errors.
            cleanMatrix(surfMat);

            mConvexSEL->mSurfaces[mFaceSEL] = surfMat;

            updateShape(mConvexSEL, mFaceSEL);
         }
         else
         {
            // Translating a face in x/y/z

            translateFace(mConvexSEL, mFaceSEL, mGizmo->getTotalOffset());
         }
      }

      if (isShapeValid(mConvexSEL))
      {
         AssertFatal(mConvexSEL->mSurfaces.size() > mFaceSEL, "mFaceSEL out of range.");
         mLastValidShape = mConvexSEL->mSurfaces;
      }
      else
      {
         AssertFatal(mLastValidShape.size() > mFaceSEL, "mFaceSEL out of range.");
         mConvexSEL->mSurfaces = mLastValidShape;
         updateShape(mConvexSEL);
      }

      return;
   }

   // Manipulating a whole Convex.

   if (!mSavedUndo)
   {
      mSavedUndo = true;
      submitUndo(ModifyShape, mConvexSEL);
   }

   if (mGizmo->getMode() == MoveMode)
   {
      mConvexSEL->setPosition(mGizmo->getPosition());
   }
   else if (mGizmo->getMode() == RotateMode)
   {
      mConvexSEL->setTransform(mGizmo->getTransform());
   }
   else
   {
      mConvexSEL->setScale(mGizmo->getScale());
   }

   if (mConvexSEL->getClientObject())
   {
      ConvexShape *clientObj = static_cast< ConvexShape* >(mConvexSEL->getClientObject());
      clientObj->setTransform(mConvexSEL->getTransform());
      clientObj->setScale(mConvexSEL->getScale());
   }*/
   return true;
}

bool BrushEditorTool::onMouseUp(const Gui3DMouseEvent &e)
{
   if (!mUseMouseDown)
      return false;

   mWorldEditor->mouseUnlock();

   mMouseDown = false;
   mCreateMode = false;

   //mHasCopied = false;
   //mHasGeometry = false;

   /*if (mActiveTool)
   {
      ConvexEditorTool::EventResult result = mActiveTool->on3DMouseUp(event);

      if (result == ConvexEditorTool::Done)
         setActiveTool(NULL);

      return;
   }*/

   /*if (!mSettingPivot && !mDragging && (mGizmo->getSelection() == Gizmo::None || !mConvexSEL))
   {
      if (mConvexSEL != mConvexHL)
      {
         setSelection(mConvexHL, -1);
      }
      else
      {
         if (mFaceSEL != mFaceHL)
            setSelection(mConvexSEL, mFaceHL);
         else
            setSelection(mConvexSEL, -1);
      }

      mUsingPivot = false;
   }

   mSettingPivot = false;
   mSavedPivotPos = mGizmo->getPosition();
   mSavedUndo = false;

   mGizmo->on3DMouseUp(event);

   if (mDragging)
   {
      mDragging = false;

      if (mConvexSEL)
      {
         Vector< U32 > removedPlanes;
         mConvexSEL->cullEmptyPlanes(&removedPlanes);

         // If a face has been removed we need to validate / remap
         // our selected and highlighted faces.
         if (!removedPlanes.empty())
         {
            S32 prevFaceHL = mFaceHL;
            S32 prevFaceSEL = mFaceSEL;

            if (removedPlanes.contains(mFaceHL))
               prevFaceHL = mFaceHL = -1;
            if (removedPlanes.contains(mFaceSEL))
               prevFaceSEL = mFaceSEL = -1;

            for (S32 i = 0; i < removedPlanes.size(); i++)
            {
               if ((S32)removedPlanes[i] < prevFaceSEL)
                  mFaceSEL--;
               if ((S32)removedPlanes[i] < prevFaceHL)
                  mFaceHL--;
            }

            setSelection(mConvexSEL, mFaceSEL);

            // We need to reindex faces.
            updateShape(mConvexSEL);
         }
      }
   }*/

   //mWorldEditor->updateGizmoPos();
   return true;
}

//
bool BrushEditorTool::onRightMouseDown(const Gui3DMouseEvent &e)
{
   if (!mUseRightMouseDown)
      return false;

   Con::executef(this, "onRightMouseDown", e.mousePoint);
   return true;
}
bool BrushEditorTool::onRightMouseDragged(const Gui3DMouseEvent &e)
{
   Con::executef(this, "onRightMouseDragged", e.mousePoint);
   return true;
}
bool BrushEditorTool::onRightMouseUp(const Gui3DMouseEvent &e)
{
   if (!mUseRightMouseDown)
      return false;

   Con::executef(this, "onRightMouseUp", e.mousePoint);
   return true;
}

//
bool BrushEditorTool::onMiddleMouseDown(const Gui3DMouseEvent &e)
{
   if (!mUseMiddleMouseDown)
      return false;

   Con::executef(this, "onMiddleMouseDown", e.mousePoint);
   return true;
}
bool BrushEditorTool::onMiddleMouseDragged(const Gui3DMouseEvent &e)
{
   Con::executef(this, "onMiddleMouseDragged", e.mousePoint);
   return true;
}
bool BrushEditorTool::onMiddleMouseUp(const Gui3DMouseEvent &e)
{
   if (!mUseMiddleMouseDown)
      return false;

   Con::executef(this, "onMiddleMouseUp", e.mousePoint);
   return true;
}

//
bool BrushEditorTool::onInputEvent(const InputEventInfo &e)
{
   if (!mUseKeyInput)
      return false;

   Con::executef(this, "onKeyPress", e.ascii, e.modifier);
   return true;
}

//
void BrushEditorTool::render()
{
   if (mBrushObjHL)
   {
      GFXDrawUtil *drawer = GFX->getDrawUtil();

      //build the bounds
      Box3F bnds = Box3F::Zero;
      for (U32 i = 0; i < mBrushObjHL->mBrushes[mBrushHL].mGeometry.points.size(); i++)
      {
         bnds.extend(mBrushObjHL->mBrushes[mBrushHL].mGeometry.points[i]);
      }

      GFXStateBlockDesc desc;
      desc.setCullMode(GFXCullNone);
      desc.setFillModeWireframe();
      drawer->drawCube(desc, bnds, ColorI::WHITE);
   }
}

//Editor actions
bool BrushEditorTool::carveAction()
{
   if (mSelectedBrushes.empty())
      return false;

   //first, we iterate through all of our selected brushes
   /*for (U32 i = 0; i < mSelectedBrushes.size(); i++)
   {
      //next, we iterate through all brushes and find any brushes with a bounds overlap. We can't do any
      //CSG operations on brushes we're not touching
      for (U32 b = 0; b < mBrushes.size(); b++)
      {
         //make sure we don't try and subtract against any of the selected brushes!
         if (mSelectedBrushes.contains(mBrushes[b]))
            continue;

         if (mSelectedBrushes[i]->setGlobalBounds().overlap(mBrushes[b]))
         {
            //yup, we overlap a non-selected brush, do a carve operation on it!
            return true;
         }
      }
   }*/

   return false;
}

bool BrushEditorTool::addBoxBrush(Box3F newBrushBounds)
{
   //only box brush atm. types will be added later

   // Face Order:
   // Top, Bottom, Front, Back, Left, Right
   Brush* newBrush = new Brush();

   // X Axis
   static const Point3F cubeTangents[6] =
   {
      Point3F(1, 0, 0),
      Point3F(-1, 0, 0),
      Point3F(1, 0, 0),
      Point3F(-1, 0, 0),
      Point3F(0, 1, 0),
      Point3F(0, -1, 0)
   };

   // Y Axis
   static const Point3F cubeBinormals[6] =
   {
      Point3F(0, 1, 0),
      Point3F(0, 1, 0),
      Point3F(0, 0, -1),
      Point3F(0, 0, -1),
      Point3F(0, 0, -1),
      Point3F(0, 0, -1)
   };

   // Z Axis
   static const Point3F cubeNormals[6] =
   {
      Point3F(0, 0, 1),
      Point3F(0, 0, -1),
      Point3F(0, 1, 0),
      Point3F(0, -1, 0),
      Point3F(-1, 0, 0),
      Point3F(1, 0, 0),
   };

   Vector<MatrixF> surfaces;

   for (S32 i = 0; i < 6; i++)
   {
      surfaces.increment();
      MatrixF &surf = surfaces.last();

      surf.identity();

      surf.setColumn(0, cubeTangents[i]);
      surf.setColumn(1, cubeBinormals[i]);
      surf.setColumn(2, cubeNormals[i]);
      surf.setPosition(cubeNormals[i] * 0.5f);
   }

   //newBrush->addBrushFromSurfaces(surfaces);
   return true;
}

DefineConsoleFunction(toggleBrushEditor, void, (),,
   "Create a ConvexShape from the given polyhedral object.")
{
   BrushEditorTool* brushTool;
   if (!Sim::findObject("BrushEditor", brushTool))
   {
      brushTool = new BrushEditorTool();
      brushTool->registerObject("BrushEditor");
   }

   WorldEditor* worldEditor;
   if (!Sim::findObject("EWorldEditor", worldEditor))
   {
      return;
   }

   EditorTool* curTool = worldEditor->getActiveEditorTool();

   if(curTool == nullptr || brushTool != curTool)
      worldEditor->setEditorTool(brushTool);
   else
      worldEditor->setEditorTool(nullptr);
}

