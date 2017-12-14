#include "brushEditorTool.h"
#include "gfx/gfxDrawUtil.h"
#include "materials/materialDefinition.h"
//#include "T3D/convexShape.h"


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
   mDragging = false;

   mBrushObj = nullptr;

   mSelectedBrush = -1;
   mSelectedFace = -1;
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

   //Need at least one mat
   if (mSurfaceMaterials.empty())
   {
      SurfaceMaterials newSurfMat;
      newSurfMat.mMaterialName = "WarningMaterial";
      mSurfaceMaterials.push_back(newSurfMat);
   }

   if (mBrushObj == nullptr)
   {
      mBrushObj = new BrushObject();
      mBrushObj->registerObject();
   }
}

//Called when the tool is deactivated on the World Editor
void BrushEditorTool::onDeactivated()
{
   mWorldEditor = NULL;
   Con::executef(this, "onDeactivated");
}

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

   //_cursorCast(e, &hitShape, &hitBrush, &hitFace);

   /*
   PlaneF floorPlane = PlaneF(Point3F(0, 0, mSelectedTileObject->mLevels[mCurrentLevel].floorHeight), Point3F(0, 0, 1));

   //RayInfo ri;
   //if (gServerContainer.castRay(e.pos, e.pos + e.vec * 10000.0f, StaticShapeObjectType, &ri))
   Point3F mouseFloorPos = Point3F::Zero;
   if(floorPlane.clipSegment(e.pos, e.pos + e.vec * 10000.0f, mouseFloorPos))
   {
      mHLGridPosition = Point2F(mouseFloorPos.x, mouseFloorPos.y);
   }
   */

   mWorldEditor->getGizmo()->on3DMouseMove(e);

   return true;
}
bool BrushEditorTool::onMouseDown(const Gui3DMouseEvent &e)
{
   if (!mUseMouseDown)
      return false;

   if (e.modifier & SI_ALT || e.modifier & SI_CTRL)
   {
      PlaneF floorPlane = PlaneF(Point3F(0, 0, 0), Point3F(0, 0, 1));

      Point3F mouseFloorPos = Point3F::Zero;
      if (floorPlane.clipSegment(e.pos, e.pos + e.vec * 10000.0f, mouseFloorPos))
      {
         Point3F hitPos = Point3F(mouseFloorPos.x, mouseFloorPos.y, 0);

         EditBrush newBrush;

         bool isSubtract = e.modifier & SI_ALT ? false : true;

         Box3F box(1);
         box.setCenter(hitPos);
         newBrush = addBoxBrush(box, isSubtract);

         mBrushes.push_back(newBrush);

         compileGeometry();
      }
   }
   else
   {
      //normal clicks
      //see if we clicked on a brush
      mSelectedBrush = -1;
      mSelectedFace = -1;
      F32 t = 1;

      for (U32 i = 0; i < mBrushes.size(); i++)
      {
         Point3F normal;
         F32 tempT;
         if (mBrushes[i].mBounds.collideLine(e.pos, e.pos + e.vec * 10000.0f, &tempT, &normal))
         {
            if (tempT < t)
            {
               t = tempT;
               mSelectedBrush = i;
            }
         }
      }

      if (mSelectedBrush != -1)
      {
         //mWorldEditor->getGizmo()->getProfile()->flags = 0;
         MatrixF trans = MatrixF(true);
         trans.setPosition(mBrushes[mSelectedBrush].mBounds.getCenter());

         mWorldEditor->getGizmo()->set(trans, mBrushes[mSelectedBrush].mBounds.getCenter(), Point3F(1,1,1));
      }
   }

   mWorldEditor->getGizmo()->on3DMouseDown(e);
   return true;
}
bool BrushEditorTool::onMouseDragged(const Gui3DMouseEvent &e)
{
   mWorldEditor->getGizmo()->on3DMouseDragged(e);

   mDragging = true;

   if (mSelectedBrush != -1)
   {
      Point3F delta = mWorldEditor->getGizmo()->getOffset();

      mBrushes[mSelectedBrush].mBounds.setCenter(mWorldEditor->getGizmo()->getPosition());

      for (U32 i = 0; i < mBrushes[mSelectedBrush].mCSG.size(); i++)
      {
         mBrushes[mSelectedBrush].mCSG[i].plane.normal.x += delta.x;
         mBrushes[mSelectedBrush].mCSG[i].plane.normal.y += delta.y;
         mBrushes[mSelectedBrush].mCSG[i].plane.normal.z += delta.z;

         for (U32 v = 0; v < mBrushes[mSelectedBrush].mCSG[i].vertices.size(); v++)
         {
            mBrushes[mSelectedBrush].mCSG[i].vertices[v].pos.x += delta.x;
            mBrushes[mSelectedBrush].mCSG[i].vertices[v].pos.y += delta.y;
            mBrushes[mSelectedBrush].mCSG[i].vertices[v].pos.z += delta.z;
         }
      }
   }

   return true;
}

bool BrushEditorTool::onMouseUp(const Gui3DMouseEvent &e)
{
   if (!mUseMouseDown)
      return false;

   mWorldEditor->mouseUnlock();

   mMouseDown = false;
   mDragging = false;

   mWorldEditor->getGizmo()->on3DMouseUp(e);

   compileGeometry();
   
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
void BrushEditorTool::updateGizmo()
{
   GizmoProfile* gizProfile = mWorldEditor->getGizmo()->getProfile();
   gizProfile->restoreDefaultState();

   const GizmoMode &mode = gizProfile->mode;
   S32 &flags = gizProfile->flags;
   GizmoAlignment &align = gizProfile->alignment;

   U8 keys = Input::getModifierKeys();

   /*bool mCtrlDown = keys & (SI_LCTRL | SI_LSHIFT);

   bool altDown = keys & (SI_LALT);

   if (altDown)
   {
      flags = 0;
      return;
   }

   if (mSelectedFace != -1)
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

   if (mSelectedFace != -1 && mode == ScaleMode)
      flags &= ~GizmoProfile::CanScaleZ;
   else
      flags |= GizmoProfile::CanScaleZ;

   if (mSelectedFace != -1 && mode == MoveMode)
   {
      if (mCtrlDown)
         flags &= ~(GizmoProfile::CanTranslateX | GizmoProfile::CanTranslateY | GizmoProfile::PlanarHandlesOn);
      else
         flags |= (GizmoProfile::CanTranslateX | GizmoProfile::CanTranslateY | GizmoProfile::PlanarHandlesOn);
   }*/

   flags |= (GizmoProfile::CanTranslateX | GizmoProfile::CanTranslateY | GizmoProfile::CanTranslateZ | GizmoProfile::PlanarHandlesOn);
}

//
void BrushEditorTool::processBrushes()
{
   //clean reset all our brushes' models
   for (U32 i = 0; i < mBrushes.size(); i++)
   {
      mBrushes[i].mCSGModel = CSGUtils::csgjs_modelFromPolygons(mBrushes[i].mCSG);
   }

   for (U32 i = 0; i < mBrushes.size(); i++)
   {
      //find all subtractive brushes that overlap our additive brushes
      if (!mBrushes[i].mIsSubtract)
      {
         //bool wasModified = false;
         for (U32 s = 0; s < mBrushes.size(); s++)
         {
            if (!mBrushes[s].mIsSubtract)
               continue;

            if (!mBrushes[i].mBounds.isOverlapped(mBrushes[s].mBounds))
               continue;

            mBrushes[i].mCSGModel = CSGUtils::csgjs_difference(mBrushes[s].mCSGModel, mBrushes[i].mCSGModel);

            //wasModified = true;
         }

         /*if (wasModified)
         {
            //flip it!
            std::vector<CSGUtils::csgjs_polygon> tempPoly = CSGUtils::csgjs_modelToPolygons(mBrushes[i].mCSGModel);

            for (U32 f = 0; f < tempPoly.size(); f++)
            {
               tempPoly[f].flip();
            }

            //and get the return
            mBrushes[i].mCSGModel = CSGUtils::csgjs_modelFromPolygons(tempPoly);
         }*/
      }
   }
}

void BrushEditorTool::compileGeometry()
{
   //Set up our materials list
   mBuffers.clear();
   mPrimCount = 0;
   mVertCount = 0;

   for (U32 i = 0; i < mSurfaceMaterials.size(); i++)
   {
      mBuffers.increment();
      mBuffers.last().surfaceMaterialId = i;
   }

   //Process our brushes to do our CSG operations as needed
   processBrushes();

   //Build buffer data
   for (U32 i = 0; i < mBrushes.size(); i++)
   {
      if (mBrushes[i].mCSGModel.vertices.size() != 0)
      {
         mBrushes[i].mPrimCount = mBrushes[i].mCSGModel.indices.size() / 3;
         mBrushes[i].mVertCount = mBrushes[i].mCSGModel.vertices.size();

         // Allocate VB and copy in data.
         mBrushes[i].mVertexBuffer.set(GFX, mBrushes[i].mVertCount, GFXBufferTypeStatic);
         VertexType *pVert = mBrushes[i].mVertexBuffer.lock();

         for (size_t ind = 0; ind < mBrushes[i].mCSGModel.indices.size(); ind += 3)
         {
            for (int j = 0; j < 3; j++)
            {
               CSGUtils::csgjs_vertex v = mBrushes[i].mCSGModel.vertices[mBrushes[i].mCSGModel.indices[ind + j]];

               pVert->normal = Point3F(v.normal.x, v.normal.y, v.normal.z);
               pVert->tangent = Point3F(0, 0, 1);
               pVert->color = ColorI::BLACK;
               pVert->point = Point3F(v.pos.x, v.pos.y, v.pos.z);
               pVert->texCoord = Point2F(v.uv.x, v.uv.y);

               pVert++;
            }
         }

         mBrushes[i].mVertexBuffer.unlock();

         // Allocate PB

         mBrushes[i].mPrimitiveBuffer.set(GFX, mBrushes[i].mCSGModel.indices.size(), mBrushes[i].mPrimCount, GFXBufferTypeStatic);

         U16 *pIndex;
         mBrushes[i].mPrimitiveBuffer.lock(&pIndex);

         for (U16 ind = 0; ind < mBrushes[i].mCSGModel.indices.size(); ind++)
         {
            *pIndex = ind;
            pIndex++;
         }

         mBrushes[i].mPrimitiveBuffer.unlock();
      }
   }

   //copy over to the client

   BrushObject* clientBO = static_cast<BrushObject*>(mBrushObj->getClientObject());

   if (clientBO)
   {
      clientBO->mBrushes.clear();
      
      for (U32 i = 0; i < mBrushes.size(); i++)
      {
         if (mBrushes[i].mIsSubtract)
            continue;

         Brush newBrush;
         newBrush.mCSG = mBrushes[i].mCSG;
         newBrush.mCSGModel = mBrushes[i].mCSGModel;
         newBrush.mMaterialName = mBrushes[i].mMaterialName;
         newBrush.mPrimCount = mBrushes[i].mPrimCount;
         newBrush.mVertCount = mBrushes[i].mVertCount;
         newBrush.mPrimitiveBuffer = mBrushes[i].mPrimitiveBuffer;
         newBrush.mVertexBuffer = mBrushes[i].mVertexBuffer;

         clientBO->mBrushes.push_back(newBrush);
      }

      clientBO->updateBounds(true);
   }

   //if we're on the client, find our buffer data
   /*S32 bufferId;

   for (U32 i = 0; i < mBrushes.size(); i++)
   {
      if (mBrushes[i].mCSGModel.vertices.size() != 0)
      {
         mBrushes[i].mPrimCount = mBrushes[i].mCSGModel.indices.size() / 3;
         mBrushes[i].mVertCount = mBrushes[i].mCSGModel.vertices.size();

         // Allocate VB and copy in data.
         mBrushes[i].mVertexBuffer.set(GFX, mBrushes[i].mVertCount, GFXBufferTypeStatic);
         VertexType *pVert = mBrushes[i].mVertexBuffer.lock();

         for (size_t i = 0; i < mBrushes[i].mCSGModel.indices.size(); i += 3)
         {
            //This is a face
            bufferId = findBufferSetByMaterial(0); //todo, face mat ID

            if (mBuffers[bufferId].buffers.last().vertCount + triangleCount * 3 > BUFFER_SIZE
               || mBuffers[bufferId].buffers.last().primCount + triangleCount > BUFFER_SIZE)
            {
               //yep, we'll overstep with this, so spool up a new buffer in this set
               BufferSet::Buffers newBuffer = BufferSet::Buffers();
               mBuffers[bufferId].buffers.push_back(newBuffer);
               mBuffers[bufferId].buffers.last().vertStart = mVertCount;
               mBuffers[bufferId].buffers.last().primStart = mPrimCount;
            }

            mBuffers[bufferId].vertCount += triangleCount * 3;
            mBuffers[bufferId].primCount += triangleCount;
            mBuffers[bufferId].buffers.last().vertCount += triangleCount * 3;
            mBuffers[bufferId].buffers.last().primCount += triangleCount;

            //update the TOTAL prim and vert counts
            mPrimCount += triangleCount;
            mVertCount += triangleCount * 3;

            for (int j = 0; j < 3; j++)
            {
               CSGUtils::csgjs_vertex v = mBrushes[i].mCSGModel.vertices[mBrushes[i].mCSGModel.indices[i + j]];

               pVert->normal = Point3F(v.normal.x, v.normal.y, v.normal.z);
               pVert->tangent = Point3F(0, 0, 1);
               pVert->color = ColorI::BLACK;
               pVert->point = Point3F(v.pos.x, v.pos.y, v.pos.z);
               pVert->texCoord = Point2F(v.uv.x, v.uv.y);

               pVert++;
            }
         }

         mBrushes[i].mVertexBuffer.unlock();

         // Allocate PB

         mBrushes[i].mPrimitiveBuffer.set(GFX, newBrush.mCSGModel.indices.size(), newBrush.mPrimCount, GFXBufferTypeStatic);

         U16 *pIndex;
         mBrushes[i].mPrimitiveBuffer.lock(&pIndex);

         for (U16 i = 0; i < mBrushes[i].mCSGModel.indices.size(); i++)
         {
            *pIndex = i;
            pIndex++;
         }

         mBrushes[i].mPrimitiveBuffer.unlock();
      }
   }

   //see if we have a buffer set for this face's material
   bufferId = findBufferSetByMaterial(face.uvs.matID);

   //see if this would push us over our buffer size limit, if it is, make a new buffer for this set
   if (mBuffers[bufferId].buffers.last().vertCount + triangleCount * 3 > BUFFER_SIZE
      || mBuffers[bufferId].buffers.last().primCount + triangleCount > BUFFER_SIZE)
   {
      //yep, we'll overstep with this, so spool up a new buffer in this set
      BufferSet::Buffers newBuffer = BufferSet::Buffers();
      mBuffers[bufferId].buffers.push_back(newBuffer);
      mBuffers[bufferId].buffers.last().vertStart = mVertCount;
      mBuffers[bufferId].buffers.last().primStart = mPrimCount;
   }

   mBuffers[bufferId].vertCount += triangleCount * 3;
   mBuffers[bufferId].primCount += triangleCount;
   mBuffers[bufferId].buffers.last().vertCount += triangleCount * 3;
   mBuffers[bufferId].buffers.last().primCount += triangleCount;

   //update the TOTAL prim and vert counts
   mPrimCount += triangleCount;
   mVertCount += triangleCount * 3;


   bool foundTri = reader->pushFirstChildElement("Triangle");

   Brush::Triangle tri;

   {
      bool foundVert = reader->pushFirstChildElement("Vert");
      Point3F vertPos;
      Point2F texCoord;
      U32 vertIdx;
      const char* vertBuffer;

      for (U32 v = 0; v < 3; v++)
      {
         vertBuffer = reader->getData();

         dSscanf(vertBuffer, "%g %g %g %g %g",
            &vertPos.x, &vertPos.y, &vertPos.z, &texCoord.x, &texCoord.y);

         newBrush.mGeometry.points.push_back(vertPos);
         vertIdx = newBrush.mGeometry.points.size() - 1;

         face.points.push_back(vertIdx);
         face.texcoords.push_back(texCoord);
         if (v == 0)
            tri.p0 = face.points.size() - 1;
         else if (v == 1)
            tri.p1 = face.points.size() - 1;
         else
            tri.p2 = face.points.size() - 1;

         //if we're on the client, prep our buffer data
         if (!isServerObject())
         {
            VertexType bufVert;
            bufVert.normal = normal;
            bufVert.tangent = tangent;
            bufVert.texCoord = texCoord;
            bufVert.point = vertPos;

            mBuffers[bufferId].buffers.last().vertData.push_back(bufVert);
            U32 vertPrimId = mBuffers[bufferId].buffers.last().vertData.size() - 1;
            mBuffers[bufferId].buffers.last().primData.push_back(vertPrimId);
         }

         reader->nextSiblingElement("Vert");
      }

      reader->popElement();
   }

   face.triangles.push_back(tri);

   if (!reader->nextSiblingElement("Triangle"))
      break;


   //build the face's plane
   Point3F avg = Point3F::Zero;
   for (U32 p = 0; p < face.points.size(); p++)
   {
      avg += newBrush.mGeometry.points[face.points[p]];
   }

   avg /= face.points.size();

   PlaneF facePlane = PlaneF(avg, normal);

   face.plane = facePlane;

   newBrush.mGeometry.faces.push_back(face);

   if (!reader->nextSiblingElement("Face"))
      break;



   mBrushes.clear();
   mSurfaceMaterials.clear();
   U32 BUFFER_SIZE = 65000;

   U32 timestamp = Sim::getCurrentTime();

   bool hasStartState = false;

   if (!dStrIsEmpty(mBrushFile))
   {
      //use our xml reader to parse the file!
      SimXMLDocument *reader = mXMLReader.getObject();
      if (!reader->loadFile(mBrushFile))
         Con::errorf("Could not load state machine file: &s", mBrushFile);

      //BrushObject
      if (!reader->pushChildElement(0))
         return;

      //first, load materials
      while (true)
      {
         //Material
         reader->pushChildElement(0);

         //get the matname
         reader->pushFirstChildElement("materialName");
         String matName = reader->getData();

         //check if we've got any existing mats with that name
         bool found = false;
         for (U32 i = 0; i < mSurfaceMaterials.size(); i++)
         {
            if (!dStrcmp(mSurfaceMaterials[i].mMaterialName, matName))
            {
               found = true;
               break;
            }
         }

         if (!found)
         {
            SurfaceMaterials newSurfMat;
            newSurfMat.mMaterialName = matName;
            mSurfaceMaterials.push_back(newSurfMat);
         }

         reader->popElement();

         if (!reader->nextSiblingElement("Material"))
            break;
      }

      reader->popElement();

      //Set up our buffers if we're on the client
      if (!isServerObject())
      {
         mBuffers.clear();
         mPrimCount = 0;
         mVertCount = 0;

         for (U32 i = 0; i < mSurfaceMaterials.size(); i++)
         {
            mBuffers.increment();
            mBuffers.last().surfaceMaterialId = i;
         }
      }

      while (true)
      {
         bool foundBrush = reader->pushFirstChildElement("Brush");

         Brush newBrush;

         while (true)
         {
            Brush::Face face;

            bool foundFace = reader->pushFirstChildElement("Face");

            Point3F tangent, normal, centroid;
            Brush::FaceUV uv;

            reader->pushFirstChildElement("Tangent");

            String tangnetData = reader->getData();

            dSscanf(tangnetData, "%g %g %g", &tangent.x, &tangent.y, &tangent.z);

            reader->popElement();

            reader->pushFirstChildElement("Normal");

            String normalData = reader->getData();

            dSscanf(normalData, "%g %g %g", &normal.x, &normal.y, &normal.z);

            reader->popElement();

            reader->pushFirstChildElement("TriangleCount");

            String triCountStr = reader->getData();

            U32 triangleCount;

            dSscanf(triCountStr, "%i", &triangleCount);

            reader->popElement();

            uv.matID = 0;
            uv.offset = Point2F(0, 0);
            uv.scale = Point2F(0, 0);
            uv.zRot = 0;
            uv.horzFlip = false;
            uv.vertFlip = false;

            face.tangent = tangent;
            face.normal = normal;
            face.uvs = uv;

            //if we're on the client, find our buffer data
            S32 bufferId;
            if (!isServerObject())
            {
               //see if we have a buffer set for this face's material
               bufferId = findBufferSetByMaterial(face.uvs.matID);

               //see if this would push us over our buffer size limit, if it is, make a new buffer for this set
               if (mBuffers[bufferId].buffers.last().vertCount + triangleCount * 3 > BUFFER_SIZE
                  || mBuffers[bufferId].buffers.last().primCount + triangleCount > BUFFER_SIZE)
               {
                  //yep, we'll overstep with this, so spool up a new buffer in this set
                  BufferSet::Buffers newBuffer = BufferSet::Buffers();
                  mBuffers[bufferId].buffers.push_back(newBuffer);
                  mBuffers[bufferId].buffers.last().vertStart = mVertCount;
                  mBuffers[bufferId].buffers.last().primStart = mPrimCount;
               }

               mBuffers[bufferId].vertCount += triangleCount * 3;
               mBuffers[bufferId].primCount += triangleCount;
               mBuffers[bufferId].buffers.last().vertCount += triangleCount * 3;
               mBuffers[bufferId].buffers.last().primCount += triangleCount;

               //update the TOTAL prim and vert counts
               mPrimCount += triangleCount;
               mVertCount += triangleCount * 3;
            }

            //lastly do the verts
            while (true)
            {
               bool foundTri = reader->pushFirstChildElement("Triangle");

               Brush::Triangle tri;

               {
                  bool foundVert = reader->pushFirstChildElement("Vert");
                  Point3F vertPos;
                  Point2F texCoord;
                  U32 vertIdx;
                  const char* vertBuffer;

                  for (U32 v = 0; v < 3; v++)
                  {
                     vertBuffer = reader->getData();

                     dSscanf(vertBuffer, "%g %g %g %g %g",
                        &vertPos.x, &vertPos.y, &vertPos.z, &texCoord.x, &texCoord.y);

                     newBrush.mGeometry.points.push_back(vertPos);
                     vertIdx = newBrush.mGeometry.points.size() - 1;

                     face.points.push_back(vertIdx);
                     face.texcoords.push_back(texCoord);
                     if (v == 0)
                        tri.p0 = face.points.size() - 1;
                     else if (v == 1)
                        tri.p1 = face.points.size() - 1;
                     else
                        tri.p2 = face.points.size() - 1;

                     //if we're on the client, prep our buffer data
                     if (!isServerObject())
                     {
                        VertexType bufVert;
                        bufVert.normal = normal;
                        bufVert.tangent = tangent;
                        bufVert.texCoord = texCoord;
                        bufVert.point = vertPos;

                        mBuffers[bufferId].buffers.last().vertData.push_back(bufVert);
                        U32 vertPrimId = mBuffers[bufferId].buffers.last().vertData.size() - 1;
                        mBuffers[bufferId].buffers.last().primData.push_back(vertPrimId);
                     }

                     reader->nextSiblingElement("Vert");
                  }

                  reader->popElement();
               }

               face.triangles.push_back(tri);

               if (!reader->nextSiblingElement("Triangle"))
                  break;
            }

            reader->popElement();

            //build the face's plane
            Point3F avg = Point3F::Zero;
            for (U32 p = 0; p < face.points.size(); p++)
            {
               avg += newBrush.mGeometry.points[face.points[p]];
            }

            avg /= face.points.size();

            PlaneF facePlane = PlaneF(avg, normal);

            face.plane = facePlane;

            newBrush.mGeometry.faces.push_back(face);

            if (!reader->nextSiblingElement("Face"))
               break;
         }

         reader->popElement();

         mBrushes.push_back(newBrush);

         if (!reader->nextSiblingElement("Brush"))
            break;
      }
   }

   updateBounds(true);
   updateMaterials();
   createGeometry();

   U32 endTimestamp = Sim::getCurrentTime();

   U32 loadTime = endTimestamp - timestamp;

   bool tmp = true;*/
}
//
void BrushEditorTool::render()
{
   GFXDrawUtil *drawer = GFX->getDrawUtil();

   GFXStateBlockDesc desc;
   desc.setCullMode(GFXCullNone);
   desc.setFillModeWireframe();

   for (U32 i = 0; i < mBrushes.size(); i++)
   {
      if (mBrushes[i].mIsSubtract)
      {
         drawer->drawCube(desc, mBrushes[i].mBounds, ColorI(255,215,0));
      }
   }

   if (mSelectedBrush != -1)
   {
      drawer->drawCube(desc, mBrushes[mSelectedBrush].mBounds, ColorI(255, 20, 147));
   }

   mWorldEditor->getGizmo()->renderGizmo(mWorldEditor->getLastCameraQuery().cameraMatrix, mWorldEditor->getLastCameraQuery().fov);
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
}

//Editor actions
/*bool BrushEditorTool::carveAction()
{
   if (mSelectedBrushes.empty())
      return false;

   //first, we iterate through all of our selected brushes
   for (U32 i = 0; i < mSelectedBrushes.size(); i++)
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
   }

   return false;
}*/

BrushEditorTool::EditBrush BrushEditorTool::addBoxBrush(Box3F newBrushBounds, bool isSubtract)
{
   //only box brush atm. types will be added later

   // Face Order:
   // Top, Bottom, Front, Back, Left, Right
   EditBrush newBrush;

   newBrush.mMaterialName = "Grid512_Grey_Mat";

   newBrush.mIsSubtract = isSubtract;
   newBrush.mBounds = Box3F::Zero;

   //Rig the faces
   // X Axis
   static const Point3F cubeTangents[6] =
   {
      Point3F(1,  0,  0),
      Point3F(-1,  0,  0),
      Point3F(1,  0,  0),
      Point3F(-1,  0,  0),
      Point3F(0,  1,  0),
      Point3F(0, -1,  0)
   };

   // Y Axis
   static const Point3F cubeBinormals[6] =
   {
      Point3F(0,  1,  0),
      Point3F(0,  1,  0),
      Point3F(0,  0, -1),
      Point3F(0,  0, -1),
      Point3F(0,  0, -1),
      Point3F(0,  0, -1)
   };

   // Z Axis
   static const Point3F cubeNormals[6] =
   {
      Point3F(0,  0,  1),
      Point3F(0,  0, -1),
      Point3F(0,  1,  0),
      Point3F(0, -1,  0),
      Point3F(-1,  0,  0),
      Point3F(1,  0,  0),
   };

   mGeometry.surfaces.clear();
   mGeometry.faces.clear();
   mGeometry.points.clear();

   for (S32 i = 0; i < 6; i++)
   {
      mGeometry.surfaces.increment();
      MatrixF &surf = mGeometry.surfaces.last();

      surf.identity();

      surf.setColumn(0, cubeTangents[i]);
      surf.setColumn(1, cubeBinormals[i]);
      surf.setColumn(2, cubeNormals[i]);
      surf.setPosition(cubeNormals[i] * 0.5f);
   }

   Vector<PlaneF> planes;
   for (S32 i = 0; i < mGeometry.surfaces.size(); i++)
      planes.push_back(PlaneF(mGeometry.surfaces[i].getPosition(), mGeometry.surfaces[i].getUpVector()));

   Vector< Point3F > tangents;
   for (S32 i = 0; i < mGeometry.surfaces.size(); i++)
      tangents.push_back(mGeometry.surfaces[i].getRightVector());

   mGeometry.worldTransform = MatrixF(true);
   mGeometry.worldTransform.setPosition(newBrushBounds.getCenter());
   mGeometry.scale = Point3F::One;

   mGeometry.generate(planes, tangents);

   if (mGeometry.faces.size() == 0)
      return newBrush;

   //Build the CSG mesh data
   Vector<Point3F> points;
   for (U32 i = 0; i < mGeometry.faces.size(); i++)
   {
      S32 faceId = mGeometry.getFaceId(i);

      if (faceId == -1)
         continue;

      std::vector<CSGUtils::csgjs_vertex> facePoly;

      Vector<Point3F> facePoints;
      Vector<Point2F> faceCoords;
      Point3F faceNorm = mGeometry.faces[faceId].normal;

      mGeometry.getSurfaceVerts(faceId, &facePoints, &faceCoords, true);

      for (U32 v = 0; v < facePoints.size(); v++)
      {
         CSGUtils::csgjs_vertex vert;
         vert.pos.x = facePoints[v].x;
         vert.pos.y = facePoints[v].y;
         vert.pos.z = facePoints[v].z;

         vert.normal.x = faceNorm.x;
         vert.normal.y = faceNorm.y;
         vert.normal.z = faceNorm.z;

         vert.uv.x = faceCoords[v].x;
         vert.uv.y = faceCoords[v].y;

         facePoly.insert(facePoly.end(), vert);

         points.push_back(facePoints[v]);
      }

      newBrush.mCSG.insert(newBrush.mCSG.end(), facePoly);
   }

   //expand our bounds
   newBrush.mBounds = Box3F::aroundPoints(points.address(), points.size());

   newBrush.mCSGModel = CSGUtils::csgjs_modelFromPolygons(newBrush.mCSG);

   return newBrush;
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

void BrushEditorTool::Geometry::generate(const Vector< PlaneF > &planes, const Vector< Point3F > &tangents)
{
   PROFILE_SCOPE(BrushEditorTool_Geometry_generate);

   points.clear();
   faces.clear();

   AssertFatal(planes.size() == tangents.size(), "ConvexShape - incorrect plane/tangent count.");

#ifdef TORQUE_ENABLE_ASSERTS
   for (S32 i = 0; i < planes.size(); i++)
   {
      F32 dt = mDot(planes[i], tangents[i]);
      AssertFatal(mIsZero(dt, 0.0001f), "ConvexShape - non perpendicular input vectors.");
      AssertFatal(planes[i].isUnitLength() && tangents[i].isUnitLength(), "ConvexShape - non unit length input vector.");
   }
#endif

   const U32 planeCount = planes.size();

   Point3F linePt, lineDir;

   for (S32 i = 0; i < planeCount; i++)
   {
      Vector< MathUtils::Line > collideLines;

      // Find the lines defined by the intersection of this plane with all others.

      for (S32 j = 0; j < planeCount; j++)
      {
         if (i == j)
            continue;

         if (planes[i].intersect(planes[j], linePt, lineDir))
         {
            collideLines.increment();
            MathUtils::Line &line = collideLines.last();
            line.origin = linePt;
            line.direction = lineDir;
         }
      }

      if (collideLines.empty())
         continue;

      // Find edges and points defined by the intersection of these lines.
      // As we find them we fill them into our working ConvexShape::Face
      // structure.

      Face newFace;

      for (S32 j = 0; j < collideLines.size(); j++)
      {
         Vector< Point3F > collidePoints;

         for (S32 k = 0; k < collideLines.size(); k++)
         {
            if (j == k)
               continue;

            MathUtils::LineSegment segment;
            MathUtils::mShortestSegmentBetweenLines(collideLines[j], collideLines[k], &segment);

            F32 dist = (segment.p0 - segment.p1).len();

            if (dist < 0.0005f)
            {
               S32 l = 0;
               for (; l < planeCount; l++)
               {
                  if (planes[l].whichSide(segment.p0) == PlaneF::Front)
                     break;
               }

               if (l == planeCount)
                  collidePoints.push_back(segment.p0);
            }
         }

         //AssertFatal( collidePoints.size() <= 2, "A line can't collide with more than 2 other lines in a convex shape..." );

         if (collidePoints.size() != 2)
            continue;

         // Push back collision points into our points vector
         // if they are not duplicates and determine the id
         // index for those points to be used by Edge(s).    

         const Point3F &pnt0 = collidePoints[0];
         const Point3F &pnt1 = collidePoints[1];
         S32 idx0 = -1;
         S32 idx1 = -1;

         for (S32 k = 0; k < points.size(); k++)
         {
            if (pnt0.equal(points[k]))
            {
               idx0 = k;
               break;
            }
         }

         for (S32 k = 0; k < points.size(); k++)
         {
            if (pnt1.equal(points[k]))
            {
               idx1 = k;
               break;
            }
         }

         if (idx0 == -1)
         {
            points.push_back(pnt0);
            idx0 = points.size() - 1;
         }

         if (idx1 == -1)
         {
            points.push_back(pnt1);
            idx1 = points.size() - 1;
         }

         // Construct the Face::Edge defined by this collision.

         S32 localIdx0 = newFace.points.push_back_unique(idx0);
         S32 localIdx1 = newFace.points.push_back_unique(idx1);

         newFace.edges.increment();
         BrushEditorTool::Geometry::Edge &newEdge = newFace.edges.last();
         newEdge.p0 = localIdx0;
         newEdge.p1 = localIdx1;
      }

      if (newFace.points.size() < 3)
         continue;

      //AssertFatal( newFace.points.size() == newFace.edges.size(), "ConvexShape - face point count does not equal edge count." );


      // Fill in some basic Face information.

      newFace.id = i;
      newFace.normal = planes[i];
      newFace.tangent = tangents[i];


      // Make a working array of Point3Fs on this face.

      U32 pntCount = newFace.points.size();
      Point3F *workPoints = new Point3F[pntCount];

      for (S32 j = 0; j < pntCount; j++)
         workPoints[j] = points[newFace.points[j]];


      // Calculate the average point for calculating winding order.

      Point3F averagePnt = Point3F::Zero;

      for (S32 j = 0; j < pntCount; j++)
         averagePnt += workPoints[j];

      averagePnt /= pntCount;


      // Sort points in correct winding order.

      U32 *vertMap = new U32[pntCount];

      MatrixF quadMat(true);
      quadMat.setPosition(averagePnt);
      quadMat.setColumn(0, newFace.tangent);
      quadMat.setColumn(1, mCross(newFace.normal, newFace.tangent));
      quadMat.setColumn(2, newFace.normal);
      quadMat.inverse();

      // Transform working points into quad space 
      // so we can work with them as 2D points.

      for (S32 j = 0; j < pntCount; j++)
         quadMat.mulP(workPoints[j]);

      MathUtils::sortQuadWindingOrder(true, workPoints, vertMap, pntCount);

      // Save points in winding order.

      for (S32 j = 0; j < pntCount; j++)
         newFace.winding.push_back(vertMap[j]);

      // Calculate the area and centroid of the face.

      newFace.area = 0.0f;
      for (S32 j = 0; j < pntCount; j++)
      {
         S32 k = (j + 1) % pntCount;
         const Point3F &p0 = workPoints[vertMap[j]];
         const Point3F &p1 = workPoints[vertMap[k]];

         // Note that this calculation returns positive area for clockwise winding
         // and negative area for counterclockwise winding.
         newFace.area += p0.y * p1.x;
         newFace.area -= p0.x * p1.y;
      }

      //AssertFatal( newFace.area > 0.0f, "ConvexShape - face area was not positive." );
      if (newFace.area > 0.0f)
         newFace.area /= 2.0f;

      F32 factor;
      F32 cx = 0.0f, cy = 0.0f;

      for (S32 j = 0; j < pntCount; j++)
      {
         S32 k = (j + 1) % pntCount;
         const Point3F &p0 = workPoints[vertMap[j]];
         const Point3F &p1 = workPoints[vertMap[k]];

         factor = p0.x * p1.y - p1.x * p0.y;
         cx += (p0.x + p1.x) * factor;
         cy += (p0.y + p1.y) * factor;
      }

      factor = 1.0f / (newFace.area * 6.0f);
      newFace.centroid.set(cx * factor, cy * factor, 0.0f);
      quadMat.inverse();
      quadMat.mulP(newFace.centroid);

      delete[] workPoints;
      workPoints = NULL;

      // Make polygons / triangles for this face.

      const U32 polyCount = pntCount - 2;

      newFace.triangles.setSize(polyCount);

      for (S32 j = 0; j < polyCount; j++)
      {
         BrushEditorTool::Geometry::Triangle &poly = newFace.triangles[j];

         poly.p0 = vertMap[0];

         if (j == 0)
         {
            poly.p1 = vertMap[1];
            poly.p2 = vertMap[2];
         }
         else
         {
            poly.p1 = vertMap[1 + j];
            poly.p2 = vertMap[2 + j];
         }
      }

      delete[] vertMap;


      // Calculate texture coordinates for each point in this face.

      const Point3F binormal = mCross(newFace.normal, newFace.tangent);
      PlaneF planey(newFace.centroid - 0.5f * binormal, binormal);
      PlaneF planex(newFace.centroid - 0.5f * newFace.tangent, newFace.tangent);

      newFace.texcoords.setSize(newFace.points.size());

      for (S32 j = 0; j < newFace.points.size(); j++)
      {
         F32 x = planex.distToPlane(points[newFace.points[j]]);
         F32 y = planey.distToPlane(points[newFace.points[j]]);

         newFace.texcoords[j].set(-x, -y);
      }

      // Done with this Face.
      faces.push_back(newFace);
   }
}

void BrushEditorTool::Geometry::getSurfaceTriangles(S32 surfId, Vector< Point3F > *outPoints, Vector< Point2F > *outCoords, bool worldSpace)
{
   S32 faceId = -1;
   for (S32 i = 0; i < faces.size(); i++)
   {
      if (faces[i].id == surfId)
      {
         faceId = i;
         break;
      }
   }

   if (faceId == -1)
      return;

   const Face &face = faces[faceId];
   const Vector< Point3F > &pointList = points;

   const MatrixF &surfToObj = surfaces[faceId];
   MatrixF objToSurf(surfToObj);
   objToSurf.inverse();

   Point3F surfScale(1.5f, 1.5f, 1.0f);

   for (S32 i = 0; i < face.triangles.size(); i++)
   {
      for (S32 j = 0; j < 3; j++)
      {
         Point3F pnt(pointList[face.points[face.triangles[i][j]]]);

         objToSurf.mulP(pnt);
         pnt *= surfScale;
         surfToObj.mulP(pnt);

         outPoints->push_back(pnt);

         if (outCoords)
            outCoords->push_back(face.texcoords[face.triangles[i][j]]);
      }
   }

   if (worldSpace)
   {
      MatrixF objToWorld(worldTransform);
      objToWorld.scale(scale);

      for (S32 i = 0; i < outPoints->size(); i++)
         objToWorld.mulP((*outPoints)[i]);
   }
}

void BrushEditorTool::Geometry::getSurfaceVerts(U32 faceId, Vector< Point3F > *outPoints, Vector< Point2F > *outCoords, bool worldSpace)
{
   const Face &face = faces[faceId];
   const Vector< Point3F > &pointList = points;

   const MatrixF &surfToObj = surfaces[faceId];
   MatrixF objToSurf(surfToObj);
   objToSurf.inverse();

   //Point3F surfScale(1.5f, 1.5f, 1.0f);

   for (S32 i = 0; i < face.triangles.size(); i++)
   {
      for (S32 j = 0; j < 3; j++)
      {
         Point3F pnt(pointList[face.points[face.triangles[i][j]]]);

         /*objToSurf.mulP(pnt);
         pnt *= surfScale;
         surfToObj.mulP(pnt);*/

         outPoints->push_back_unique(pnt);

         if (outCoords)
            outCoords->push_back_unique(face.texcoords[face.triangles[i][j]]);
      }
   }

   if (worldSpace)
   {
      MatrixF objToWorld(worldTransform);
      objToWorld.scale(scale);

      for (S32 i = 0; i < outPoints->size(); i++)
         objToWorld.mulP((*outPoints)[i]);
   }
}

S32 BrushEditorTool::Geometry::getFaceId(U32 surfId)
{
   S32 faceId = -1;
   for (S32 i = 0; i < faces.size(); i++)
   {
      if (faces[i].id == surfId)
      {
         faceId = i;
         break;
      }
   }

   return faceId;
}