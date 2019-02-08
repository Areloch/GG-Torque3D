#include "tilesetObjectEditorTool.h"
#include "gfx/gfxDrawUtil.h"
#include "gui/controls/guiTreeViewCtrl.h"

//placeholder mesh stuff
#include "math/mathIO.h"
#include "scene/sceneRenderState.h"
#include "console/consoleTypes.h"
#include "core/stream/bitStream.h"
#include "materials/materialManager.h"
#include "materials/baseMatInstance.h"
#include "renderInstance/renderPassManager.h"

#include "lighting/lightQuery.h"
IMPLEMENT_CONOBJECT(TilesetObjectEditorTool);

//
IMPLEMENT_CO_NETOBJECT_V1(TilePlaceholderMesh);

ConsoleDocClass(TilePlaceholderMesh,
   "@brief An example scene object which renders a mesh.\n\n"
   "This class implements a basic SceneObject that can exist in the world at a "
   "3D position and render itself. There are several valid ways to render an "
   "object in Torque. This class implements the preferred rendering method which "
   "is to submit a MeshRenderInst along with a Material, vertex buffer, "
   "primitive buffer, and transform and allow the RenderMeshMgr handle the "
   "actual setup and rendering for you.\n\n"
   "See the C++ code for implementation details.\n\n"
   "@ingroup Examples\n");


//-----------------------------------------------------------------------------
// Object setup and teardown
//-----------------------------------------------------------------------------
TilePlaceholderMesh::TilePlaceholderMesh()
{
   // Flag this object so that it will always
   // be sent across the network to clients
   mNetFlags.set(Ghostable | ScopeAlways);

   // Set it as a "static" object that casts shadows
   mTypeMask |= StaticObjectType | StaticShapeObjectType;

   // Make sure we the Material instance to NULL
   // so we don't try to access it incorrectly
   mMaterialInst = NULL;
}

TilePlaceholderMesh::~TilePlaceholderMesh()
{
   if (mMaterialInst)
      SAFE_DELETE(mMaterialInst);
}

//-----------------------------------------------------------------------------
// Object Editing
//-----------------------------------------------------------------------------
void TilePlaceholderMesh::initPersistFields()
{
   addGroup("Rendering");
   addField("material", TypeMaterialName, Offset(mMaterialName, TilePlaceholderMesh),
      "The name of the material used to render the mesh.");
   endGroup("Rendering");

   // SceneObject already handles exposing the transform
   Parent::initPersistFields();
}

void TilePlaceholderMesh::inspectPostApply()
{
   Parent::inspectPostApply();

   // Flag the network mask to send the updates
   // to the client object
   setMaskBits(UpdateMask);
}

bool TilePlaceholderMesh::onAdd()
{
   if (!Parent::onAdd())
      return false;

   // Set up a 1x1x1 bounding box
   mObjBox.set(Point3F(-0.5f, -0.5f, -0.5f),
      Point3F(0.5f, 0.5f, 0.5f));

   resetWorldBox();

   // Add this object to the scene
   addToScene();

   // Refresh this object's material (if any)
   updateMaterial();

   return true;
}

void TilePlaceholderMesh::onRemove()
{
   // Remove this object from the scene
   removeFromScene();

   Parent::onRemove();
}

void TilePlaceholderMesh::setTransform(const MatrixF & mat)
{
   // Let SceneObject handle all of the matrix manipulation
   Parent::setTransform(mat);

   // Dirty our network mask so that the new transform gets
   // transmitted to the client object
   setMaskBits(TransformMask);
}

U32 TilePlaceholderMesh::packUpdate(NetConnection *conn, U32 mask, BitStream *stream)
{
   // Allow the Parent to get a crack at writing its info
   U32 retMask = Parent::packUpdate(conn, mask, stream);

   // Write our transform information
   if (stream->writeFlag(mask & TransformMask))
   {
      mathWrite(*stream, getTransform());
      mathWrite(*stream, getScale());
   }

   // Write out any of the updated editable properties
   if (stream->writeFlag(mask & UpdateMask))
      stream->write(mMaterialName);

   return retMask;
}

void TilePlaceholderMesh::unpackUpdate(NetConnection *conn, BitStream *stream)
{
   // Let the Parent read any info it sent
   Parent::unpackUpdate(conn, stream);

   if (stream->readFlag())  // TransformMask
   {
      mathRead(*stream, &mObjToWorld);
      mathRead(*stream, &mObjScale);

      setTransform(mObjToWorld);
   }

   if (stream->readFlag())  // UpdateMask
   {
      stream->read(&mMaterialName);

      if (isProperlyAdded())
         updateMaterial();
   }
}

//-----------------------------------------------------------------------------
// Object Rendering
//-----------------------------------------------------------------------------
void TilePlaceholderMesh::createGeometry()
{
   static const Point3F cubePoints[8] =
   {
      Point3F(1, -1, -1), Point3F(1, -1,  1), Point3F(1,  1, -1), Point3F(1,  1,  1),
      Point3F(-1, -1, -1), Point3F(-1,  1, -1), Point3F(-1, -1,  1), Point3F(-1,  1,  1)
   };

   static const Point3F cubeNormals[6] =
   {
      Point3F(1,  0,  0), Point3F(-1,  0,  0), Point3F(0,  1,  0),
      Point3F(0, -1,  0), Point3F(0,  0,  1), Point3F(0,  0, -1)
   };

   static const Point2F cubeTexCoords[4] =
   {
      Point2F(0,  0), Point2F(0, -1),
      Point2F(1,  0), Point2F(1, -1)
   };

   static const U32 cubeFaces[36][3] =
   {
      { 3, 0, 3 },{ 0, 0, 0 },{ 1, 0, 1 },
      { 2, 0, 2 },{ 0, 0, 0 },{ 3, 0, 3 },
      { 7, 1, 1 },{ 4, 1, 2 },{ 5, 1, 0 },
      { 6, 1, 3 },{ 4, 1, 2 },{ 7, 1, 1 },
      { 3, 2, 1 },{ 5, 2, 2 },{ 2, 2, 0 },
      { 7, 2, 3 },{ 5, 2, 2 },{ 3, 2, 1 },
      { 1, 3, 3 },{ 4, 3, 0 },{ 6, 3, 1 },
      { 0, 3, 2 },{ 4, 3, 0 },{ 1, 3, 3 },
      { 3, 4, 3 },{ 6, 4, 0 },{ 7, 4, 1 },
      { 1, 4, 2 },{ 6, 4, 0 },{ 3, 4, 3 },
      { 2, 5, 1 },{ 4, 5, 2 },{ 0, 5, 0 },
      { 5, 5, 3 },{ 4, 5, 2 },{ 2, 5, 1 }
   };

   // Fill the vertex buffer
   VertexType *pVert = NULL;

   mVertexBuffer.set(GFX, 36, GFXBufferTypeStatic);
   pVert = mVertexBuffer.lock();

   Point3F halfSize = getObjBox().getExtents() * 0.5f;

   for (U32 i = 0; i < 36; i++)
   {
      const U32& vdx = cubeFaces[i][0];
      const U32& ndx = cubeFaces[i][1];
      const U32& tdx = cubeFaces[i][2];

      pVert[i].point = cubePoints[vdx] * halfSize;
      pVert[i].normal = cubeNormals[ndx];
      pVert[i].texCoord = cubeTexCoords[tdx];
   }

   mVertexBuffer.unlock();

   // Fill the primitive buffer
   U16 *pIdx = NULL;

   mPrimitiveBuffer.set(GFX, 36, 12, GFXBufferTypeStatic);

   mPrimitiveBuffer.lock(&pIdx);

   for (U16 i = 0; i < 36; i++)
      pIdx[i] = i;

   mPrimitiveBuffer.unlock();
}

void TilePlaceholderMesh::updateMaterial()
{
   if (mMaterialName.isEmpty())
      return;

   // If the material name matches then don't bother updating it.
   if (mMaterialInst && mMaterialName.equal(mMaterialInst->getMaterial()->getName(), String::NoCase))
      return;

   SAFE_DELETE(mMaterialInst);

   mMaterialInst = MATMGR->createMatInstance(mMaterialName, getGFXVertexFormat< VertexType >());
   if (!mMaterialInst)
      Con::errorf("TilePlaceholderMesh::updateMaterial - no Material called '%s'", mMaterialName.c_str());
}

void TilePlaceholderMesh::prepRenderImage(SceneRenderState *state)
{
   // Do a little prep work if needed
   if (mVertexBuffer.isNull())
      createGeometry();

   // If we have no material then skip out.
   if (!mMaterialInst || !state)
      return;

   // If we don't have a material instance after the override then 
   // we can skip rendering all together.
   BaseMatInstance *matInst = state->getOverrideMaterial(mMaterialInst);
   if (!matInst)
      return;

   // Get a handy pointer to our RenderPassmanager
   RenderPassManager *renderPass = state->getRenderPass();

   // Allocate an MeshRenderInst so that we can submit it to the RenderPassManager
   MeshRenderInst *ri = renderPass->allocInst<MeshRenderInst>();

   // Set our RenderInst as a standard mesh render
   ri->type = RenderPassManager::RIT_Mesh;

   //If our material has transparency set on this will redirect it to proper render bin
   if (matInst->getMaterial()->isTranslucent())
   {
      ri->type = RenderPassManager::RIT_Translucent;
      ri->translucentSort = true;
   }

   // Calculate our sorting point
   if (state)
   {
      // Calculate our sort point manually.
      const Box3F& rBox = getRenderWorldBox();
      ri->sortDistSq = rBox.getSqDistanceToPoint(state->getCameraPosition());
   }
   else
      ri->sortDistSq = 0.0f;

   // Set up our transforms
   MatrixF objectToWorld = getRenderTransform();
   objectToWorld.scale(getScale());

   ri->objectToWorld = renderPass->allocUniqueXform(objectToWorld);
   ri->worldToCamera = renderPass->allocSharedXform(RenderPassManager::View);
   ri->projection = renderPass->allocSharedXform(RenderPassManager::Projection);

   // If our material needs lights then fill the RIs 
   // light vector with the best lights.
   if (matInst->isForwardLit())
   {
      LightQuery query;
      query.init(getWorldSphere());
      query.getLights(ri->lights, 8);
   }

   // Make sure we have an up-to-date backbuffer in case
   // our Material would like to make use of it
   // NOTICE: SFXBB is removed and refraction is disabled!
   //ri->backBuffTex = GFX->getSfxBackBuffer();

   // Set our Material
   ri->matInst = matInst;

   // Set up our vertex buffer and primitive buffer
   ri->vertBuff = &mVertexBuffer;
   ri->primBuff = &mPrimitiveBuffer;

   ri->prim = renderPass->allocPrim();
   ri->prim->type = GFXTriangleList;
   ri->prim->minIndex = 0;
   ri->prim->startIndex = 0;
   ri->prim->numPrimitives = 12;
   ri->prim->startVertex = 0;
   ri->prim->numVertices = 36;

   // We sort by the material then vertex buffer
   ri->defaultKey = matInst->getStateHint();
   ri->defaultKey2 = (uintptr_t)ri->vertBuff; // Not 64bit safe!

                                              // Submit our RenderInst to the RenderPassManager
   state->getRenderPass()->addInst(ri);
}
//

TilesetObjectEditorTool::TilesetObjectEditorTool() : mMouseDragged(false)
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

   mBrushHL = -1;
   mFaceHL = -1;

   shiftMod = false;
   altMod = false;

   mCurrentLevel = 0;

   mRestrictWallsToFlooredTiles = true;

   mElementMode = PaintElementMode::Floor;

   mSelectedTileObject = nullptr;
}

bool TilesetObjectEditorTool::onAdd()
{
   return Parent::onAdd();
}

void TilesetObjectEditorTool::onRemove()
{
   Parent::onRemove();
}

//Called when the tool is activated on the World Editor
void TilesetObjectEditorTool::onActivated(WorldEditor* editor)
{
   mWorldEditor = editor;
   Con::executef(this, "onActivated");
}

//Called when the tool is deactivated on the World Editor
void TilesetObjectEditorTool::onDeactivated()
{
   mWorldEditor = NULL;
   Con::executef(this, "onDeactivated");
}

//

bool TilesetObjectEditorTool::onMouseMove(const Gui3DMouseEvent &e)
{
   if (!mUseMouseMove)
      return false;

   if (mSelectedTileObject == nullptr)
      return true;

   /*if (mActiveTool)
   {
      // If we have an active tool pass this event to it.
      // If it handled it, consume the event.
      if (mActiveTool->on3DMouseMove(event))
         return;
   }*/

   /*BrushObject *hitShape = NULL;
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
   }*/

   PlaneF floorPlane = PlaneF(Point3F(0, 0, mSelectedTileObject->mLevels[mCurrentLevel].floorHeight), Point3F(0, 0, 1));

   //RayInfo ri;
   //if (gServerContainer.castRay(e.pos, e.pos + e.vec * 10000.0f, StaticShapeObjectType, &ri))
   Point3F mouseFloorPos = Point3F::Zero;
   if(floorPlane.clipSegment(e.pos, e.pos + e.vec * 10000.0f, mouseFloorPos))
   {
      mHLGridPosition = Point2F(mouseFloorPos.x, mouseFloorPos.y);
   }

   mouseVector = e.vec;
   mouseStartPos = e.pos;

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

bool TilesetObjectEditorTool::onMouseDown(const Gui3DMouseEvent &e)
{
   if (!mUseMouseDown || mSelectedTileObject == nullptr)
      return false;

   clearSelectedElements();

   mMouseDown = true;

   /*RayInfo ri;
   if (gServerContainer.castRay(e.pos, e.pos + e.vec * 10000.0f, StaticShapeObjectType, &ri))
   {
      mMouseDownPosition = Point2F(ri.point.x, ri.point.y);
   }*/

   PlaneF floorPlane = PlaneF(Point3F(0, 0, mSelectedTileObject->mLevels[mCurrentLevel].floorHeight), Point3F(0, 0, 1));

   //RayInfo ri;
   //if (gServerContainer.castRay(e.pos, e.pos + e.vec * 10000.0f, StaticShapeObjectType, &ri))
   Point3F mouseFloorPos = Point3F::Zero;
   if (floorPlane.clipSegment(e.pos, e.pos + e.vec * 10000.0f, mouseFloorPos))
   {
      mMouseDownPosition = Point2F(mouseFloorPos.x, mouseFloorPos.y);
   }

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
bool TilesetObjectEditorTool::onMouseDragged(const Gui3DMouseEvent &e)
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

   if (mActionMode == ActionMode::PaintBucket)
   {
      if (mSelectedTileObject == nullptr)
         return false;

      mMouseDragged = true;

      PlaneF floorPlane = PlaneF(Point3F(0, 0, mSelectedTileObject->mLevels[mCurrentLevel].floorHeight), Point3F(0, 0, 1));

      //RayInfo ri;
      //if (gServerContainer.castRay(e.pos, e.pos + e.vec * 10000.0f, StaticShapeObjectType, &ri))
      Point3F mouseFloorPos = Point3F::Zero;
      if (floorPlane.clipSegment(e.pos, e.pos + e.vec * 10000.0f, mouseFloorPos))
      {
         mHLGridPosition = Point2F(mouseFloorPos.x, mouseFloorPos.y);
      }

      /*for (U32 i = 0; i < mSelectedTileObject->mLevels[mCurrentLevel].tiles.size(); ++i)
      {
         mSelectedTileObject->mLevels[mCurrentLevel].
      }*/
   }
   else if (mActionMode == ActionMode::Paint)
   {
      mMouseDragged = true;

      PlaneF floorPlane = PlaneF(Point3F(0, 0, mSelectedTileObject->mLevels[mCurrentLevel].floorHeight), Point3F(0, 0, 1));

      //RayInfo ri;
      //if (gServerContainer.castRay(e.pos, e.pos + e.vec * 10000.0f, StaticShapeObjectType, &ri))
      Point3F mouseFloorPos = Point3F::Zero;
      if (floorPlane.clipSegment(e.pos, e.pos + e.vec * 10000.0f, mouseFloorPos))
      {
         mHLGridPosition = Point2F(mouseFloorPos.x, mouseFloorPos.y);
      }

      //add the element to the list
      Tile* targetTile = getTileAtCoord(mHLGridPosition);

      Con::printf("Dragged tile position: %g %g", mHLGridPosition.x, mHLGridPosition.y);

      if (targetTile != nullptr)
      {
         //make a tile!
         TileElement newElement;

         if (mElementMode == PaintElementMode::Floor)
         {
            //check that we don't already have a floor
            bool hasElement = false;
            for (U32 i = 0; i < targetTile->elements.size(); ++i)
            {
               if (targetTile->elements[i].type == TileElement::ElementType::Floor)
               {
                  hasElement = true;
                  break;
               }
            }

            if (!hasElement)
            {
               SimGroup* misGroup;
               if (Sim::findObject("MissionCleanup", misGroup))
               {
                  newElement.type = TileElement::Floor;
                  newElement.selected = false;

                  TilePlaceholderMesh* elementPlaceholder = new TilePlaceholderMesh();
                  elementPlaceholder->registerObject();
                  elementPlaceholder->setSelectionEnabled(false);

                  elementPlaceholder->mMaterialName = "Grid512_Grey_Mat";

                  Point3F start = Point3F(targetTile->levelCoords.x, targetTile->levelCoords.y, mSelectedTileObject->mLevels[mCurrentLevel].floorHeight);
                  Point3F end = start + Point3F(mSelectedTileObject->mLevels[mCurrentLevel].tileWidth, mSelectedTileObject->mLevels[mCurrentLevel].tileWidth, 0.2);
                  Box3F floorSpace = Box3F(start, end);

                  elementPlaceholder->setPosition(floorSpace.getCenter());
                  elementPlaceholder->setScale(Point3F(mSelectedTileObject->mLevels[mCurrentLevel].tileWidth, mSelectedTileObject->mLevels[mCurrentLevel].tileWidth, 0.2));

                  elementPlaceholder->updateMaterial();
                  elementPlaceholder->createGeometry();

                  newElement.elementObject = elementPlaceholder;
               }
            }
         }
         else if (mElementMode == PaintElementMode::Wall)
         {
            Box3F wallBounds = getTileBoundsFaceBox(targetTile, e.pos, e.pos + e.vec * 10000.0f);

            if (wallBounds != Box3F::Zero)
            {
               /*newElement.type = TileElement::Wall;
               newElement.selected = false;
               newElement.bounds = wallBounds;*/

               SimGroup* misGroup;
               if (Sim::findObject("MissionCleanup", misGroup))
               {
                  newElement.type = TileElement::Wall;
                  newElement.selected = false;

                  TilePlaceholderMesh* elementPlaceholder = new TilePlaceholderMesh();
                  elementPlaceholder->registerObject();
                  elementPlaceholder->setSelectionEnabled(false);

                  elementPlaceholder->mMaterialName = "Grid512_Orange_Mat";

                  elementPlaceholder->setPosition(wallBounds.getCenter());
                  elementPlaceholder->setScale(wallBounds.getExtents());

                  elementPlaceholder->updateMaterial();
                  elementPlaceholder->createGeometry();

                  newElement.elementObject = elementPlaceholder;
               }
            }
         }

         targetTile->elements.push_back(newElement);
      }
   }
   else if (mActionMode == ActionMode::Erase)
   {
      mMouseDragged = true;

      PlaneF floorPlane = PlaneF(Point3F(0, 0, mSelectedTileObject->mLevels[mCurrentLevel].floorHeight), Point3F(0, 0, 1));

      //RayInfo ri;
      //if (gServerContainer.castRay(e.pos, e.pos + e.vec * 10000.0f, StaticShapeObjectType, &ri))
      Point3F mouseFloorPos = Point3F::Zero;
      if (floorPlane.clipSegment(e.pos, e.pos + e.vec * 10000.0f, mouseFloorPos))
      {
         mHLGridPosition = Point2F(mouseFloorPos.x, mouseFloorPos.y);
      }

      //add the element to the list
      Tile* targetTile = getTileAtCoord(mHLGridPosition);

      Con::printf("Dragged tile position: %g %g", mHLGridPosition.x, mHLGridPosition.y);

      if (targetTile != nullptr)
      {
         if (mElementMode == PaintElementMode::Floor)
         {
            //check that we don't already have a floor
            S32 elementId = -1;
            bool hasElement = false;
            for (U32 i = 0; i < targetTile->elements.size(); ++i)
            {
               if ((mElementMode == PaintElementMode::Floor && targetTile->elements[i].type == TileElement::ElementType::Floor) ||
                     (mElementMode == PaintElementMode::Wall && targetTile->elements[i].type == TileElement::ElementType::Wall) ||
                     (mElementMode == PaintElementMode::Ceiling && targetTile->elements[i].type == TileElement::ElementType::Ceiling))
               {
                  hasElement = true;
                  elementId = i;
                  break;
               }
            }

            if (hasElement)
            {
               targetTile->elements[elementId].elementObject->deleteObject();
               targetTile->elements.erase(elementId);
            }
         }
      }
   }

   return true;
}

bool TilesetObjectEditorTool::onMouseUp(const Gui3DMouseEvent &e)
{
   if (!mUseMouseDown || mSelectedTileObject == nullptr)
      return false;

   mWorldEditor->mouseUnlock();

   if (mActionMode == ActionMode::Select)
   {
      //add the element to the list
      Tile* targetTile = getTileAtCoord(mHLGridPosition);

      if (targetTile != nullptr)
      {
         if (mElementMode == PaintElementMode::Floor)
         {
            for (U32 i = 0; i < targetTile->elements.size(); ++i)
            {
               if (targetTile->elements[i].type == TileElement::ElementType::Floor)
               {
                  targetTile->elements[i].selected = true;
               }
            }            
         }
      }
   }
   else if (mActionMode == ActionMode::Paint)
   {
      //add the element to the list
      Tile* targetTile = getTileAtCoord(mHLGridPosition);

      if (targetTile != nullptr)
      {
         //make a tile!
         TileElement newElement;

         if (mElementMode == PaintElementMode::Floor)
         {
            //check that we don't already have a floor
            bool hasElement = false;
            for (U32 i = 0; i < targetTile->elements.size(); ++i)
            {
               if (targetTile->elements[i].type == TileElement::ElementType::Floor)
               {
                  hasElement = true;
                  break;
               }
            }

            if (!hasElement)
            {
               SimGroup* misGroup;
               if (Sim::findObject("MissionCleanup", misGroup))
               {
                  newElement.type = TileElement::Floor;
                  newElement.selected = false;

                  TilePlaceholderMesh* elementPlaceholder = new TilePlaceholderMesh();
                  elementPlaceholder->registerObject();
                  elementPlaceholder->setSelectionEnabled(false);

                  elementPlaceholder->mMaterialName = "Grid512_Grey_Mat";

                  Point3F start = Point3F(targetTile->levelCoords.x, targetTile->levelCoords.y, mSelectedTileObject->mLevels[mCurrentLevel].floorHeight);
                  Point3F end = start + Point3F(mSelectedTileObject->mLevels[mCurrentLevel].tileWidth, mSelectedTileObject->mLevels[mCurrentLevel].tileWidth, 0.2);
                  Box3F floorSpace = Box3F(start, end);

                  elementPlaceholder->setPosition(floorSpace.getCenter());
                  elementPlaceholder->setScale(Point3F(mSelectedTileObject->mLevels[mCurrentLevel].tileWidth, mSelectedTileObject->mLevels[mCurrentLevel].tileWidth, 0.2));

                  elementPlaceholder->updateMaterial();
                  elementPlaceholder->createGeometry();

                  newElement.elementObject = elementPlaceholder;
               }
            }
         }

         else if(mElementMode == PaintElementMode::Wall)
         {
            Box3F wallBounds = getTileBoundsFaceBox(targetTile, e.pos, e.pos + e.vec * 10000.0f);

            if (wallBounds != Box3F::Zero)
            {
               /*newElement.type = TileElement::Wall;
               newElement.selected = false;
               newElement.bounds = wallBounds;*/

               SimGroup* misGroup;
               if (Sim::findObject("MissionCleanup", misGroup))
               {
                  newElement.type = TileElement::Wall;
                  newElement.selected = false;

                  TilePlaceholderMesh* elementPlaceholder = new TilePlaceholderMesh();
                  elementPlaceholder->registerObject();
                  elementPlaceholder->setSelectionEnabled(false);

                  elementPlaceholder->mMaterialName = "Grid512_Orange_Mat";

                  elementPlaceholder->setPosition(wallBounds.getCenter());
                  elementPlaceholder->setScale(wallBounds.getExtents());

                  elementPlaceholder->updateMaterial();
                  elementPlaceholder->createGeometry();

                  newElement.elementObject = elementPlaceholder;
               }
            }
         }

         targetTile->elements.push_back(newElement);
      }
   }

   /*if (!mMouseDragged)
   {
      //make a tile!
      Tile newTile;
      TileElement newElement;

      if (mElementMode == PaintElementMode::Floor)
      {
         F32 x = snap(mMouseDownPosition.x, mLevels[mCurrentLevel].tileWidth);
         F32 y = snap(mMouseDownPosition.y, mLevels[mCurrentLevel].tileWidth);
         newTile.levelCoords = Point2I(x, y);
         newTile.positionNudge = Point2F::Zero;

         newElement.type = TileElement::Floor;
      }

      newTile.elements.push_back(newElement);

      mLevels[mCurrentLevel].tiles.push_back(newTile);
   }
   else
   {
      Box3I selectArea = Box3I(Point3I(mMouseDownPosition.x, mMouseDownPosition.y, 0), Point3I(mHLGridPosition.x, mHLGridPosition.y, 0));

      for (U32 x = 0; x < selectArea.len_x(); ++x)
      {
         for (U32 y = 0; y < selectArea.len_y(); ++y)
         {
            //make a tile!
            Tile newTile;
            TileElement newElement;

            if (mElementMode == PaintElementMode::Floor)
            {
               F32 xPos = snap(selectArea.minExtents.x + x, mLevels[mCurrentLevel].tileWidth * 1.5);
               F32 yPos = snap(selectArea.minExtents.y + y, mLevels[mCurrentLevel].tileWidth * 1.5);
               newTile.levelCoords = Point2I(xPos, yPos);
               newTile.positionNudge = Point2F::Zero;

               newElement.type = TileElement::Floor;
            }

            newTile.elements.push_back(newElement);

            mLevels[mCurrentLevel].tiles.push_back(newTile);
         }
      }
   }*/

   mMouseDown = false;
   mMouseDragged = false;
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
bool TilesetObjectEditorTool::onRightMouseDown(const Gui3DMouseEvent &e)
{
   if (!mUseRightMouseDown)
      return false;

   Con::executef(this, "onRightMouseDown", e.mousePoint);
   return true;
}
bool TilesetObjectEditorTool::onRightMouseDragged(const Gui3DMouseEvent &e)
{
   Con::executef(this, "onRightMouseDragged", e.mousePoint);
   return true;
}
bool TilesetObjectEditorTool::onRightMouseUp(const Gui3DMouseEvent &e)
{
   if (!mUseRightMouseDown)
      return false;

   Con::executef(this, "onRightMouseUp", e.mousePoint);
   return true;
}

//
bool TilesetObjectEditorTool::onMiddleMouseDown(const Gui3DMouseEvent &e)
{
   if (!mUseMiddleMouseDown)
      return false;

   Con::executef(this, "onMiddleMouseDown", e.mousePoint);
   return true;
}
bool TilesetObjectEditorTool::onMiddleMouseDragged(const Gui3DMouseEvent &e)
{
   Con::executef(this, "onMiddleMouseDragged", e.mousePoint);
   return true;
}
bool TilesetObjectEditorTool::onMiddleMouseUp(const Gui3DMouseEvent &e)
{
   if (!mUseMiddleMouseDown)
      return false;

   Con::executef(this, "onMiddleMouseUp", e.mousePoint);
   return true;
}

//
bool TilesetObjectEditorTool::onInputEvent(const InputEventInfo &e)
{
   if (!mUseKeyInput)
      return false;

   if (e.modifier & SI_SHIFT)
      shiftMod = true;
   else
      shiftMod = false;

   if (e.modifier & SI_ALT)
      altMod = true;
   else
      altMod = false;

   if (e.ascii & KEY_UP)
   {
      if (altMod)
      {
         mCurrentLevel++;
         if (mSelectedTileObject->mLevels.size() <= mCurrentLevel)
            mSelectedTileObject->mLevels.increment();
      }
      else
      {
         mSelectedTileObject->mLevels[mCurrentLevel].floorHeight++;
      }
   }
   else if (e.ascii & KEY_DOWN)
   {
      if (altMod)
      {
         mCurrentLevel--;
         if (mCurrentLevel < 0)
            mCurrentLevel = 0;
      }
      else
      {
         mSelectedTileObject->mLevels[mCurrentLevel].floorHeight--;
      }
   }

   Con::executef(this, "onKeyPress", e.ascii, e.modifier);
   return true;
}

//
void TilesetObjectEditorTool::render()
{
   if (mSelectedTileObject == nullptr)
      return;

   /*if (mBrushObjHL)
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
   }*/

   renderGrid();

   GFXStateBlockDesc desc;
   desc.setBlend(true);
   desc.setZReadWrite(true, false);

   F32 halfTileWidth = mSelectedTileObject->mLevels[mCurrentLevel].tileWidth * 0.5;

   for (U32 i = 0; i < mSelectedTileObject->mLevels[mCurrentLevel].tiles.size(); ++i)
   {
      Tile* tile = &mSelectedTileObject->mLevels[mCurrentLevel].tiles[i];

      for (U32 e = 0; e < tile->elements.size(); ++e)
      {
         if (tile->elements[e].selected)
         {
            ColorI elementColor = ColorI(255, 192, 203, 128);

            Box3F elementBounds = tile->elements[e].elementObject->getWorldBox();

            elementBounds.minExtents += Point3F(-0.05, -0.05, -0.05);
            elementBounds.maxExtents += Point3F(0.05, 0.05, 0.05);

            GFX->getDrawUtil()->drawCube(desc, elementBounds, elementColor);
         }
      }
   }

   //render selection
   if (mElementMode == PaintElementMode::Floor)
   {
      if (!mMouseDown)
      {
         Point3F start = Point3F(snap(mHLGridPosition.x, mSelectedTileObject->mLevels[mCurrentLevel].tileWidth), snap(mHLGridPosition.y, mSelectedTileObject->mLevels[mCurrentLevel].tileWidth), mSelectedTileObject->mLevels[mCurrentLevel].floorHeight);
         Point3F end = start + Point3F(mSelectedTileObject->mLevels[mCurrentLevel].tileWidth, mSelectedTileObject->mLevels[mCurrentLevel].tileWidth, 0.2);
         GFX->getDrawUtil()->drawCube(desc, Box3F(start, end), ColorI(255, 0, 0, 128));
      }
      else
      {
         if (mActionMode != ActionMode::Paint)
         {
            if (mMouseDragged)
            {
               Point3F start = Point3F(snap(mMouseDownPosition.x, mSelectedTileObject->mLevels[mCurrentLevel].tileWidth), snap(mMouseDownPosition.y, mSelectedTileObject->mLevels[mCurrentLevel].tileWidth), mSelectedTileObject->mLevels[mCurrentLevel].floorHeight + 0.1);
               Point3F end = Point3F(snap(mHLGridPosition.x + mSelectedTileObject->mLevels[mCurrentLevel].tileWidth, mSelectedTileObject->mLevels[mCurrentLevel].tileWidth), snap(mHLGridPosition.y + mSelectedTileObject->mLevels[mCurrentLevel].tileWidth, mSelectedTileObject->mLevels[mCurrentLevel].tileWidth), mSelectedTileObject->mLevels[mCurrentLevel].floorHeight + 0.1);

               Box3F selectionArea = Box3F(start, end);

               //selectionArea.extend(start + Point3F(mLevels[mCurrentLevel].tileWidth, mLevels[mCurrentLevel].tileWidth, 0));
               //selectionArea.extend(end + Point3F(mLevels[mCurrentLevel].tileWidth, mLevels[mCurrentLevel].tileWidth, 0));

               GFX->getDrawUtil()->drawCube(desc, selectionArea, ColorI(0, 0, 255, 128));
            }
         }
      }
   }
   else if (mElementMode == PaintElementMode::Wall)
   {
      //if (!mMouseDown)
      {
         //Find our best wall hit
         //north wall
         //Point3F start = Point3F(snap(mHLGridPosition.x, mLevels[mCurrentLevel].tileWidth), snap(mHLGridPosition.y, mLevels[mCurrentLevel].tileWidth), mLevels[mCurrentLevel].floorHeight);
         //Point3F end = start + Point3F(mLevels[mCurrentLevel].tileWidth, mLevels[mCurrentLevel].tileWidth, mLevels[mCurrentLevel].tileHeight);

         //Box3F tileBounds = Box3F(start, end);

         Tile* hoveredTile = getTileAtCoord(mHLGridPosition);

         if (hoveredTile)
         {
            Box3F wallBounds = getTileBoundsFaceBox(hoveredTile, mouseStartPos, mouseStartPos + mouseVector * 10000.0f);

            if (wallBounds != Box3F::Zero)
            {
               GFX->getDrawUtil()->drawCube(desc, wallBounds, ColorI(255, 0, 0, 128));
            }
         }
      }
      /*else
      {
         Point3F start = Point3F(S32(mHLGridPosition.x), S32(mHLGridPosition.y), mLevels[mCurrentLevel].floorHeight + 0.1);
         Point3F end = Point3F(S32(mMouseDownPosition.x), S32(mMouseDownPosition.y), mLevels[mCurrentLevel].floorHeight + 0.1);
         GFX->getDrawUtil()->drawCube(desc, Box3F(start, end), ColorI(0, 0, 255, 128));
      }*/
   }
}

void TilesetObjectEditorTool::renderGrid()
{
   //if (!isOrthoDisplayType())
    //  return;

   if (mSelectedTileObject == nullptr)
      return;

   //GFXDEBUGEVENT_SCOPE(Editor_renderGrid, ColorI::WHITE);

   // Calculate the displayed grid size based on view
   F32 drawnGridSize = 100;// mGridPlaneSize;
   
   F32 minorTickSize = 0;
   F32 gridSize = drawnGridSize;
   /*U32 minorTickMax = mGridPlaneMinorTicks + 1;
   if (minorTickMax > 0)
   {
      minorTickSize = drawnGridSize;
      gridSize = drawnGridSize * minorTickMax;
   }*/

   // Build the view-based origin
   //VectorF dir;
  // smCamMatrix.getColumn(1, &dir);

   Point3F gridPlanePos = Point3F(0,0, mSelectedTileObject->mLevels[mCurrentLevel].floorHeight);//smCamPos + dir;
   Point2F size(2 * gridSize, 2 * gridSize);

   GFXStateBlockDesc desc;
   desc.setBlend(true);
   desc.setZReadWrite(true, false);

   GFXDrawUtil::Plane plane = GFXDrawUtil::PlaneXY;

   ColorI mGridPlaneColor = ColorI(128, 128, 128, 255);
   ColorI mGridPlaneMinorTickColor = ColorI(180, 180, 180, 255);
   
   //GFX->getDrawUtil()->drawPlaneGrid(desc, gridPlanePos, size, Point2F(minorTickSize, minorTickSize), mGridPlaneMinorTickColor, plane);
   GFX->getDrawUtil()->drawPlaneGrid(desc, gridPlanePos, size, Point2F(mSelectedTileObject->mLevels[mCurrentLevel].tileWidth, mSelectedTileObject->mLevels[mCurrentLevel].tileWidth), mGridPlaneColor, plane);
}

Tile* TilesetObjectEditorTool::getTileAtCoord(Point2F coord)
{
   for (U32 i = 0; i < mSelectedTileObject->mLevels[mCurrentLevel].tiles.size(); ++i)
   {
      Point2F levelCoord = mSelectedTileObject->mLevels[mCurrentLevel].tiles[i].levelCoords;

      Box3F tileBounds = Box3F(Point3F(levelCoord.x, levelCoord.y, mSelectedTileObject->mLevels[mCurrentLevel].floorHeight),
         Point3F(levelCoord.x + mSelectedTileObject->mLevels[mCurrentLevel].tileWidth, levelCoord.y + mSelectedTileObject->mLevels[mCurrentLevel].tileWidth, 
            mSelectedTileObject->mLevels[mCurrentLevel].floorHeight + mSelectedTileObject->mLevels[mCurrentLevel].tileHeight));

      if (tileBounds.isContained(Point3F(coord.x, coord.y, mSelectedTileObject->mLevels[mCurrentLevel].floorHeight + (mSelectedTileObject->mLevels[mCurrentLevel].tileHeight * 0.5))))
      {
         return &mSelectedTileObject->mLevels[mCurrentLevel].tiles[i];
      }
   }

   return nullptr;
}

Vector<Tile*> TilesetObjectEditorTool::getTilesInBox(Box3F box)
{
   /*for (U32 i = 0; i < mSelectedTileObject->mLevels[mCurrentLevel].tiles.size(); ++i)
   {
      Point2F levelCoord = mSelectedTileObject->mLevels[mCurrentLevel].tiles[i].levelCoords;

      Box3F tileBounds = Box3F(Point3F(levelCoord.x, levelCoord.y, mSelectedTileObject->mLevels[mCurrentLevel].floorHeight),
         Point3F(levelCoord.x + mSelectedTileObject->mLevels[mCurrentLevel].tileWidth, levelCoord.y + mSelectedTileObject->mLevels[mCurrentLevel].tileWidth,
            mSelectedTileObject->mLevels[mCurrentLevel].floorHeight + mSelectedTileObject->mLevels[mCurrentLevel].tileHeight));

      if (tileBounds.isContained(Point3F(coord.x, coord.y, mSelectedTileObject->mLevels[mCurrentLevel].floorHeight + (mSelectedTileObject->mLevels[mCurrentLevel].tileHeight * 0.5))))
      {
         return &mSelectedTileObject->mLevels[mCurrentLevel].tiles[i];
      }
   }*/

   Vector<Tile*> tmp;
   return tmp;
}

Box3F TilesetObjectEditorTool::getTileBoundsFaceBox(Tile *tile, Point3F rayStart, Point3F rayEnd)
{
   if (mRestrictWallsToFlooredTiles)
   {
      bool hasFloor = false;
      for (U32 e = 0; e < tile->elements.size(); ++e)
      {
         if (tile->elements[e].type == TileElement::Floor)
         {
            hasFloor = true;
            break;
         }
      }

      if (!hasFloor)
         return Box3F::Zero;
   }

   F32 floorHeight = mSelectedTileObject->mLevels[mCurrentLevel].floorHeight;
   F32 tileHeight = mSelectedTileObject->mLevels[mCurrentLevel].tileHeight;
   F32 wallThickness = 0.2;

   Point3F start = Point3F(snap(mHLGridPosition.x, mSelectedTileObject->mLevels[mCurrentLevel].tileWidth), snap(mHLGridPosition.y, mSelectedTileObject->mLevels[mCurrentLevel].tileWidth), mSelectedTileObject->mLevels[mCurrentLevel].floorHeight);
   Point3F end = start + Point3F(mSelectedTileObject->mLevels[mCurrentLevel].tileWidth, mSelectedTileObject->mLevels[mCurrentLevel].tileWidth, mSelectedTileObject->mLevels[mCurrentLevel].tileHeight);

   Box3F bounds(start, end);

   Box3F walls[4];

   walls[0].set(Point3F(bounds.minExtents.x, bounds.minExtents.y, floorHeight), Point3F(bounds.maxExtents.x, bounds.minExtents.y + wallThickness, floorHeight + tileHeight));
   walls[1].set(Point3F(bounds.maxExtents.x - wallThickness, bounds.minExtents.y, floorHeight), Point3F(bounds.maxExtents.x, bounds.maxExtents.y, floorHeight + tileHeight));
   walls[2].set(Point3F(bounds.minExtents.x, bounds.maxExtents.y - wallThickness, floorHeight), Point3F(bounds.maxExtents.x, bounds.maxExtents.y, floorHeight + tileHeight));
   walls[3].set(Point3F(bounds.minExtents.x, bounds.minExtents.y, floorHeight), Point3F(bounds.minExtents.x + wallThickness, bounds.maxExtents.y, floorHeight + tileHeight));

   F32 bestT = 1000;
   F32 t = 0;
   Point3F colPos;
   S32 bestPick = -1;
   for (U32 i = 0; i < 4; ++i)
   {
      if (walls[i].collideLine(rayStart, rayEnd, &t, &colPos))
      {
         if (t < bestT)
         {
            bestPick = i;
         }
      }
   }
   
   if (bestPick != -1)
      return walls[bestPick];

   return Box3F::Zero;
}

//Editor actions
bool TilesetObjectEditorTool::carveAction()
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

bool TilesetObjectEditorTool::addBoxBrush(Box3F newBrushBounds)
{
   //only box brush atm. types will be added later

   // Face Order:
   // Top, Bottom, Front, Back, Left, Right
   /*Brush* newBrush = new Brush();

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
   }*/

   //newBrush->addBrushFromSurfaces(surfaces);
   return true;
}

void TilesetObjectEditorTool::riseFloor()
{
   mSelectedTileObject->mLevels[mCurrentLevel].floorHeight++;

   for (U32 i = 0; i < mSelectedTileObject->mLevels[mCurrentLevel].tiles.size(); ++i)
   {
      Tile* tile = &mSelectedTileObject->mLevels[mCurrentLevel].tiles[i];

      for (U32 e = 0; e < tile->elements.size(); ++e)
      {
         Point3F pos = tile->elements[e].elementObject->getPosition();
         tile->elements[e].elementObject->setPosition(pos + Point3F(0, 0, 1));
      }
   }
}

void TilesetObjectEditorTool::lowerFloor()
{
   mSelectedTileObject->mLevels[mCurrentLevel].floorHeight--;

   if (mSelectedTileObject->mLevels[mCurrentLevel].floorHeight < 0)
      mSelectedTileObject->mLevels[mCurrentLevel].floorHeight = 0;
   else
   {
      for (U32 i = 0; i < mSelectedTileObject->mLevels[mCurrentLevel].tiles.size(); ++i)
      {
         Tile* tile = &mSelectedTileObject->mLevels[mCurrentLevel].tiles[i];

         for (U32 e = 0; e < tile->elements.size(); ++e)
         {
            Point3F pos = tile->elements[e].elementObject->getPosition();
            tile->elements[e].elementObject->setPosition(pos - Point3F(0, 0, 1));
         }
      }
   }
}

void TilesetObjectEditorTool::nextFloor()
{
   mCurrentLevel++;
   if (mSelectedTileObject->mLevels.size() <= mCurrentLevel)
   {
      Level* prevLevel = &mSelectedTileObject->mLevels[mCurrentLevel - 1];

      //no good, add a new floor
      Level newLevel;
      newLevel.floorHeight = prevLevel->floorHeight + prevLevel->tileHeight;
      newLevel.tileHeight = prevLevel->tileHeight;
      newLevel.tileWidth = prevLevel->tileWidth;

      S32 tileCountHalf = 50;

      for (S32 i = -tileCountHalf; i < tileCountHalf; ++i)
      {
         for (S32 j = -tileCountHalf; j < tileCountHalf; ++j)
         {
            //make a tile!
            Tile newTile;
            newTile.levelCoords = Point2F(i * newLevel.tileWidth, j * newLevel.tileWidth);

            newLevel.tiles.push_back(newTile);
         }
      }

      mSelectedTileObject->mLevels.push_back(newLevel);
   }
}

void TilesetObjectEditorTool::prevFloor()
{
   if (mCurrentLevel - 1 < 0)
      return;

   mCurrentLevel--;
}

void TilesetObjectEditorTool::setActiveTileObject(TilesetObject* tilesetObject)
{
   mSelectedTileObject = tilesetObject;
}

//Update world editor status bar
/*
SimObject *statusbar;
if ( Sim::findObject( "EditorGuiStatusBar", statusbar ) )
{
String text( "Sketch Tool." );
GizmoMode mode = mGizmo->getMode();

if ( mMouseDown && mGizmo->getSelection() != Gizmo::None && mConvexSEL )
{
Point3F delta;
String qualifier;

if ( mode == RotateMode )
{
if ( mSettingPivot )
delta = mGizmo->getPosition() - mSavedPivotPos;
else
delta = mGizmo->getDeltaTotalRot();
}
else if ( mode == MoveMode )
delta = mGizmo->getTotalOffset();
else if ( mode == ScaleMode )
delta = mGizmo->getDeltaTotalScale();

if ( mGizmo->getAlignment() == Object &&
mode != ScaleMode )
{
mConvexSEL->mWorldToObj.mulV( delta );
if ( mFaceSEL != -1 && mode != RotateMode )
{
MatrixF objToSurf( mConvexSEL->mSurfaces[ mFaceSEL ] );
objToSurf.scale( mConvexSEL->getScale() );
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
if ( mConvexSEL && mConvexSEL->getSurfaces().size() > ConvexShape::smMaxSurfaces )
{
text = "WARNING: Reduce the number of surfaces on the selected ConvexShape, only the first 100 will be saved!";
}

Con::executef( statusbar, "setInfo", text.c_str() );

Con::executef( statusbar, "setSelectionObjectsByCount", Con::getIntArg( mConvexSEL == NULL ? 0 : 1 ) );
}
*/

DefineConsoleFunction(toggleTilesetObjectEditor, void, (),,
   "Create a ConvexShape from the given polyhedral object.")
{
   TilesetObjectEditorTool* brushTool;
   if (!Sim::findObject("TilesetEditor", brushTool))
   {
      brushTool = new TilesetObjectEditorTool();
      brushTool->registerObject("TilesetEditor");
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

DefineEngineMethod(TilesetObjectEditorTool, riseFloor, void, (), ,
   "Create a ConvexShape from the given polyhedral object.")
{
   object->riseFloor();
}

DefineEngineMethod(TilesetObjectEditorTool, lowerFloor, void, (), ,
   "Create a ConvexShape from the given polyhedral object.")
{
   object->lowerFloor();
}

DefineEngineMethod(TilesetObjectEditorTool, nextFloor, void, (), ,
   "Create a ConvexShape from the given polyhedral object.")
{
   object->nextFloor();
}

DefineEngineMethod(TilesetObjectEditorTool, prevFloor, void, (), ,
   "Create a ConvexShape from the given polyhedral object.")
{
   object->prevFloor();
}

DefineEngineMethod(TilesetObjectEditorTool, setElementMode, void, (String modeName), ,
   "Create a ConvexShape from the given polyhedral object.")
{
   if (modeName == String("Floor"))
      object->setElementMode(TilesetObjectEditorTool::PaintElementMode::Floor);
   else if (modeName == String("Wall"))
      object->setElementMode(TilesetObjectEditorTool::PaintElementMode::Wall);
   else if (modeName == String("Ceiling"))
      object->setElementMode(TilesetObjectEditorTool::PaintElementMode::Ceiling);
   else if (modeName == String("FullTile"))
      object->setElementMode(TilesetObjectEditorTool::PaintElementMode::FullTile);
}

DefineEngineMethod(TilesetObjectEditorTool, setActionMode, void, (String modeName), ,
   "Create a ConvexShape from the given polyhedral object.")
{
   if (modeName == String("Select"))
      object->setActionMode(TilesetObjectEditorTool::ActionMode::Select);
   else if (modeName == String("Paint"))
      object->setActionMode(TilesetObjectEditorTool::ActionMode::Paint);
   else if (modeName == String("Erase"))
      object->setActionMode(TilesetObjectEditorTool::ActionMode::Erase);
   else if (modeName == String("PaintBucket"))
      object->setActionMode(TilesetObjectEditorTool::ActionMode::PaintBucket);
}

DefineEngineMethod(TilesetObjectEditorTool, setActiveObject, void, (TilesetObject* tileObject), (nullAsType<TilesetObject*>()),
   "Create a ConvexShape from the given polyhedral object.")
{
   object->setActiveTileObject(tileObject);
}

DefineEngineMethod(TilesetObjectEditorTool, getActiveTilesetObjectInfo, void, (GuiTreeViewCtrl* infoTree), (nullAsType<GuiTreeViewCtrl*>()),
   "Create a ConvexShape from the given polyhedral object.")
{
   if (infoTree == nullptr)
      return;

   TilesetObject* tilsetObj = object->getActiveTileObject();

   if (tilsetObj == nullptr)
      return;

   infoTree->clear();

   infoTree->insertItem(0, "Tileset Object", "");
     
   for (U32 i = 0; i < tilsetObj->getLevelsCount(); ++i)
   {
      Level* tileLevel = tilsetObj->getLevel(i);

      S32 parentLevel = infoTree->insertItem(1, String("Level") + String::ToString(i), "");

      for (U32 t = 0; t < tileLevel->tiles.size(); ++t)
      {
         if (!tileLevel->tiles[t].elements.empty())
         {
            String tileTitle = String("Tile") + String::ToString(t);
            S32 parentTile = infoTree->insertItem(parentLevel, tileTitle, "");

            for (U32 e = 0; e < tileLevel->tiles[t].elements.size(); ++e)
            {
               String type = "";
               if (tileLevel->tiles[t].elements[e].type == TileElement::Floor)
                  type = "Floor";
               else if (tileLevel->tiles[t].elements[e].type == TileElement::Wall)
                  type = "Wall";

               String elementTitle = type + "_" + String("Element") + String::ToString(e);
               infoTree->insertItem(parentTile, elementTitle, "");
            }
         }
      }
   }

   infoTree->buildVisibleTree(true);
}