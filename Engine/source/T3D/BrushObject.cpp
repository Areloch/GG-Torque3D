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

#include "platform/platform.h"
#include "T3D/brushObject.h"

#include "math/mathIO.h"
#include "scene/sceneRenderState.h"
#include "console/consoleTypes.h"
#include "core/stream/bitStream.h"
#include "materials/materialManager.h"
#include "materials/baseMatInstance.h"
#include "renderInstance/renderPassManager.h"
#include "lighting/lightQuery.h"
#include "console/engineAPI.h"
#include "math/mathIO.h"
#include "math/mathUtils.h"

#include "gfx/gfxDrawUtil.h"
#include "gfx/gfxTransformSaver.h"
#include "gfx/primBuilder.h"
#include "math/mathUtils.h"

IMPLEMENT_CO_NETOBJECT_V1(BrushObject);

ConsoleDocClass(BrushObject,
   "@brief An example scene object which renders a mesh.\n\n"
   "This class implements a basic SceneObject that can exist in the world at a "
   "3D position and render itself. There are several valid ways to render an "
   "object in Torque. This class implements the preferred rendering method which "
   "is to submit a MeshRenderInst along with a Material, vertex buffer, "
   "primitive buffer, and transform and allow the RenderMeshMgr handle the "
   "actual setup and rendering for you.\n\n"
   "See the C++ code for implementation details.\n\n"
   "@ingroup Examples\n");

GFXImplementVertexFormat(BrushVert)
{
   addElement("POSITION", GFXDeclType_Float3);
   addElement("COLOR", GFXDeclType_Color);
   addElement("NORMAL", GFXDeclType_Float3);
   addElement("TANGENT", GFXDeclType_Float3);
   addElement("TEXCOORD", GFXDeclType_Float2, 0);
};

//-----------------------------------------------------------------------------
// Object setup and teardown
//-----------------------------------------------------------------------------
BrushObject::BrushObject()
{
   // Flag this object so that it will always
   // be sent across the network to clients
   mNetFlags.set(Ghostable | ScopeAlways);

   // Set it as a "static" object that casts shadows
   mTypeMask |= StaticObjectType | StaticShapeObjectType;

   mBrushFile = "";
}

BrushObject::~BrushObject()
{
}

//-----------------------------------------------------------------------------
// Object Editing
//-----------------------------------------------------------------------------
void BrushObject::initPersistFields()
{
   addGroup("Brush");
   addField("brushFile", TypeFilename, Offset(mBrushFile, BrushObject),
      "The name of the material used to render the mesh.");
   endGroup("Brush");

   // SceneObject already handles exposing the transform
   Parent::initPersistFields();
}

void BrushObject::inspectPostApply()
{
   Parent::inspectPostApply();

   // Flag the network mask to send the updates
   // to the client object
   setMaskBits(UpdateMask);
}

bool BrushObject::onAdd()
{
   if (!Parent::onAdd())
      return false;

   loadBrushFile();

   // Add this object to the scene
   addToScene();

    return true;
}

void BrushObject::onRemove()
{
   // Remove this object from the scene
   removeFromScene();

   Parent::onRemove();
}

void BrushObject::loadBrushFile()
{
   /*mBrushes.clear();
   mSurfaceMaterials.clear();
   U32 BUFFER_SIZE = 65000;

   U32 timestamp = Sim::getCurrentTime();

   if (!mXMLReader)
   {
      SimXMLDocument *xmlrdr = new SimXMLDocument();
      xmlrdr->registerObject();

      mXMLReader = xmlrdr;
   }

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

            /*reader->pushFirstChildElement("UVs");

            String uvData = reader->getData();

            dSscanf(uvData, "%i %g %g %g %g %g %d %d", &uv.matID, &uv.offset.x, &uv.offset.y,
            &uv.scale.x, &uv.scale.y, &uv.zRot, &uv.horzFlip, &uv.vertFlip);

            reader->popElement();*/

            /*uv.matID = 0;
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
                     if (v==0)
                        tri.p0 = face.points.size() - 1;
                     else if (v==1)
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

void BrushObject::saveBrushFile()
{
   //prep an xml document reader so we can save to our brush file
   /*SimXMLDocument *xmlrdr = new SimXMLDocument();
   xmlrdr->registerObject();

   xmlrdr->pushNewElement("BrushObject");

   for (U32 i = 0; i < mSurfaceMaterials.size(); i++)
   {
      xmlrdr->pushNewElement("Material");
      xmlrdr->pushNewElement("materialName");
      xmlrdr->addData(mSurfaceMaterials[i].mMaterialName);
      xmlrdr->popElement();
      xmlrdr->popElement();
   }

   for (U32 i = 0; i < mBrushes.size(); i++)
   {
      xmlrdr->pushNewElement("Brush");

      for (U32 s = 0; s < mBrushes[i].mGeometry.faces.size(); s++)
      {
         Brush::Face& face = mBrushes[i].mGeometry.faces[s];

         xmlrdr->pushNewElement("Face");

         //Tangent
         xmlrdr->pushNewElement("Tangent");

         char tangentBuffer[256];
         dSprintf(tangentBuffer, 256, "%g %g %g", face.tangent.x, face.tangent.y, face.tangent.z);

         xmlrdr->addData(tangentBuffer);
         xmlrdr->popElement();

         //Normal
         xmlrdr->pushNewElement("Normal");

         char normalBuffer[256];
         dSprintf(normalBuffer, 256, "%g %g %g", face.normal.x, face.normal.y, face.normal.z);

         xmlrdr->addData(normalBuffer);
         xmlrdr->popElement();

         //Triangle count
         xmlrdr->pushNewElement("TriangleCount");

         char triCountBuffer[32];
         dSprintf(triCountBuffer, 32, "%i", face.triangles.size());

         xmlrdr->addData(triCountBuffer);
         xmlrdr->popElement();

         //Face UV adjustment info
         xmlrdr->pushNewElement("UVs");

         char uvBuffer[512];
         dSprintf(uvBuffer, 512, "%i %g %g %g %g %g %d %d",
            face.uvs.matID, face.uvs.offset.x, face.uvs.offset.y, face.uvs.scale.x,
            face.uvs.scale.y, face.uvs.zRot, face.uvs.horzFlip, face.uvs.vertFlip);

         xmlrdr->addData(uvBuffer);
         xmlrdr->popElement();

         for (U32 t = 0; t < face.triangles.size(); t++)
         {
            xmlrdr->pushNewElement("Triangle");

            for (U32 v = 0; v < 3; v++)
            {
               xmlrdr->pushNewElement("Vert");
               Point3F vertPos = mBrushes[i].mGeometry.points[face.points[face.triangles[t][v]]];
               Point2F texCoord = face.texcoords[face.triangles[t][v]];

               char vertBuffer[256];
               dSprintf(vertBuffer, 256, "%g %g %g %g %g",
                  vertPos.x, vertPos.y, vertPos.z, texCoord.x, texCoord.y);

               xmlrdr->addData(vertBuffer);
               xmlrdr->popElement();
            }

            xmlrdr->popElement();
         }

         xmlrdr->popElement();
      }      

      xmlrdr->popElement();
   }

   xmlrdr->saveFile(mBrushFile);*/
}

bool BrushObject::castRay(const Point3F &start, const Point3F &end, RayInfo *info)
{
   /*if (mBrushes.empty())
      return false;

   for (U32 b = 0; b < mBrushes.size(); b++)
   {
      int faceCount = mBrushes[b].mGeometry.faces.size();
      for (U32 i = 0; i < faceCount; i++)
      {
         PlaneF &facePlane = mBrushes[b].mGeometry.faces[i].plane;

         F32 t;
         F32 tmin = F32_MAX;
         S32 hitFace = -1;
         Point3F hitPnt, pnt;
         VectorF rayDir(end - start);
         rayDir.normalizeSafe();

         if (false)
         {
            PlaneF plane(Point3F(0, 0, 0), Point3F(0, 0, 1));
            Point3F sp(0, 0, -1);
            Point3F ep(0, 0, 1);

            F32 t = plane.intersect(sp, ep);
            Point3F hitPnt;
            hitPnt.interpolate(sp, ep, t);
         }

         // Don't hit the back-side of planes.
         if (mDot(rayDir, facePlane) >= 0.0f)
            continue;

         t = facePlane.intersect(start, end);

         if (t >= 0.0f && t <= 1.0f && t < tmin)
         {
            pnt.interpolate(start, end, t);

            bool validHit = false;
            for (U32 tri = 0; tri < mBrushes[b].mGeometry.faces[i].triangles.size(); tri++)
            {
               Brush::Triangle &triangle = mBrushes[b].mGeometry.faces[i].triangles[tri];
               Point3F t0 = mBrushes[b].mGeometry.points[triangle.p0];
               Point3F t1 = mBrushes[b].mGeometry.points[triangle.p1];
               Point3F t2 = mBrushes[b].mGeometry.points[triangle.p2];
               /*if (MathUtils::mLineTriangleCollide(start, end,
                  t0, t1, t2))
               {
                  validHit = true;
                  break;
               }*/

               //we have our point, check if it's inside the planar bounds of the line segment
               /*VectorF v0 = t2 - t0;
               VectorF v1 = t1 - t0;
               VectorF v2 = pnt - t1;

               // Compute dot products
               F32 dot00 = mDot(v0, v0);
               F32 dot01 = mDot(v0, v1);
               F32 dot02 = mDot(v0, v2);
               F32 dot11 = mDot(v1, v1);
               F32 dot12 = mDot(v1, v2);

               // Compute barycentric coordinates
               F32 invDenom = 1 / (dot00 * dot11 - dot01 * dot01);
               F32 u = (dot11 * dot02 - dot01 * dot12) * invDenom;
               F32 v = (dot00 * dot12 - dot01 * dot02) * invDenom;

               // Check if point is in triangle
               if ((u >= 0) && (v >= 0) && (u + v < 1))
               {
                  validHit = true;
                  break;
               }
            }

            //S32 j = 0;
           /* for (; j < faceCount; j++)
            {
               if (i == j)
                  continue;

               F32 dist = mBrushes[b].mGeometry.faces[j].plane.distToPlane(pnt);
               if (dist > 1.0e-004f)
                  break;
            }
            */
            /*if (validHit)
            {
               tmin = t;
               hitFace = i;
            }
         }

         if (hitFace == -1)
            return false;

         info->face = hitFace;
         info->material = mMaterialInst;
         info->normal = facePlane;
         info->object = this;
         info->t = tmin;
         info->userData = (void*)b;

         //mObjToWorld.mulV( info->normal );

         return true;
      }
   }*/

   return false;
}

void BrushObject::setTransform(const MatrixF & mat)
{
   // Let SceneObject handle all of the matrix manipulation
   Parent::setTransform(mat);

   // Dirty our network mask so that the new transform gets
   // transmitted to the client object
   setMaskBits(TransformMask);
}

U32 BrushObject::packUpdate(NetConnection *conn, U32 mask, BitStream *stream)
{
   // Allow the Parent to get a crack at writing its info
   U32 retMask = Parent::packUpdate(conn, mask, stream);

   // Write our transform information
   if (stream->writeFlag(mask & TransformMask))
   {
      mathWrite(*stream, getTransform());
      mathWrite(*stream, getScale());
   }

   /*if (stream->writeFlag(mask & UpdateMask))
   {
      stream->writeInt(mSurfaceMaterials.size(), 32);

      for (U32 i = 0; i < mSurfaceMaterials.size(); i++)
         stream->write(mSurfaceMaterials[i].mMaterialName);

      stream->writeString(mBrushFile);
   }*/

   return retMask;
}

void BrushObject::unpackUpdate(NetConnection *conn, BitStream *stream)
{
   // Let the Parent read any info it sent
   Parent::unpackUpdate(conn, stream);

   if (stream->readFlag())  // TransformMask
   {
      mathRead(*stream, &mObjToWorld);
      mathRead(*stream, &mObjScale);

      setTransform(mObjToWorld);
   }

   /*if (stream->readFlag()) // UpdateMask
   {
      mSurfaceMaterials.clear();
      U32 materialCount = stream->readInt(32);
      for (U32 i = 0; i < materialCount; i++)
      {
         String matName;
         stream->read(&matName);

         mSurfaceMaterials.increment();
         mSurfaceMaterials.last().mMaterialName = matName;
      }

      //if (isProperlyAdded())
      //   updateMaterials();

      StringTableEntry oldBrushFile = mBrushFile;
      mBrushFile = stream->readSTString();

      //if (dStrcmp(oldBrushFile,mBrushFile))
      //   loadBrushFile();
   }*/
}

void BrushObject::updateBounds(bool recenter)
{
   if (mBrushes.size() == 0)
      return;

   mObjBox.set(Point3F::Zero, Point3F::Zero);

   Vector<Point3F> points;
   for (U32 i = 0; i < mBrushes.size(); i++)
   {
      for (U32 v = 0; v < mBrushes[i].mCSGModel.vertices.size(); v++)
      {
         Point3F vPos;
         vPos.x = mBrushes[i].mCSGModel.vertices[v].pos.x;
         vPos.y = mBrushes[i].mCSGModel.vertices[v].pos.y;
         vPos.z = mBrushes[i].mCSGModel.vertices[v].pos.z;

         points.push_back(vPos);
      }
   }

   mObjBox = Box3F::aroundPoints(points.address(), points.size());

   resetWorldBox();
}

void BrushObject::writeFields(Stream &stream, U32 tabStop)
{
   Parent::writeFields(stream, tabStop);

   // Now write all planes.

   stream.write(2, "\r\n");

   saveBrushFile();
}

bool BrushObject::writeField(StringTableEntry fieldname, const char *value)
{
   if (fieldname == StringTable->insert("surface"))
      return false;

   return Parent::writeField(fieldname, value);
}

//-----------------------------------------------------------------------------
// Object Rendering
//-----------------------------------------------------------------------------
void BrushObject::createGeometry()
{
   // Server does not need to generate vertex/prim buffers.
   if (isServerObject())
      return;

   /*for (U32 i = 0; i < mBuffers.size(); i++)
   {
      for (U32 b = 0; b < mBuffers[i].buffers.size(); b++)
      {
         BufferSet::Buffers& buffers = mBuffers[i].buffers[b];

         //if there's no data to be had in this buffer, just skip it
         if (buffers.vertData.empty())
            continue;

         buffers.vertexBuffer.set(GFX, buffers.vertData.size(), GFXBufferTypeStatic);
         VertexType *pVert = buffers.vertexBuffer.lock();

         for (U32 v = 0; v < buffers.vertData.size(); v++)
         {
            pVert->normal = buffers.vertData[v].normal;
            pVert->tangent = buffers.vertData[v].tangent;
            pVert->color = buffers.vertData[v].color;
            pVert->point = buffers.vertData[v].point;
            pVert->texCoord = buffers.vertData[v].texCoord;

            pVert++;
         }

         buffers.vertexBuffer.unlock();

         // Allocate PB
         buffers.primitiveBuffer.set(GFX, buffers.primData.size(), buffers.primData.size() / 3, GFXBufferTypeStatic);

         U16 *pIndex;
         buffers.primitiveBuffer.lock(&pIndex);

         for (U16 i = 0; i < buffers.primData.size(); i++)
         {
            *pIndex = i;
            pIndex++;
         }

         buffers.primitiveBuffer.unlock();
      }
   }*/
}

void BrushObject::updateMaterials()
{
   // Server does not need to load materials
   /*if (isServerObject())
      return;

   for (U32 i = 0; i < mSurfaceMaterials.size(); i++)
   {
      if (mSurfaceMaterials[i].mMaterialName.isEmpty())
         continue;

      if (mSurfaceMaterials[i].mMaterialInst && 
         mMaterialName.equal(mSurfaceMaterials[i].mMaterialInst->getMaterial()->getName(), String::NoCase))
         continue;

      SAFE_DELETE(mSurfaceMaterials[i].mMaterialInst);

      mSurfaceMaterials[i].mMaterialInst = MATMGR->createMatInstance(mSurfaceMaterials[i].mMaterialName, getGFXVertexFormat< VertexType >());

      if (!mSurfaceMaterials[i].mMaterialInst)
         Con::errorf("BrushObject::updateMaterial - no Material called '%s'", mSurfaceMaterials[i].mMaterialName.c_str());
   }*/
}

void BrushObject::prepRenderImage(SceneRenderState *state)
{
   // Do a little prep work if needed
   /*if (mBuffers.empty() || !state)
      createGeometry();

   // If we don't have a material instance after the override then 
   // we can skip rendering all together.
   //BaseMatInstance *matInst = state->getOverrideMaterial(mMaterialInst ? mMaterialInst : MATMGR->getWarningMatInstance());
   //if (!matInst)
   //   return;

   bool debugDraw = true;
   if (state->isDiffusePass() && debugDraw)
   {
      ObjectRenderInst *ri2 = state->getRenderPass()->allocInst<ObjectRenderInst>();
      ri2->renderDelegate.bind(this, &BrushObject::_renderDebug);
      ri2->type = RenderPassManager::RIT_Editor;
      state->getRenderPass()->addInst(ri2);
   }

   // Get a handy pointer to our RenderPassmanager
   RenderPassManager *renderPass = state->getRenderPass();

   // Set up our transforms
   MatrixF objectToWorld = getRenderTransform();
   objectToWorld.scale(getScale());

   for (U32 i = 0; i < mBuffers.size(); i++)
   {
      for (U32 b = 0; b < mBuffers[i].buffers.size(); b++)
      {
         if (mBuffers[i].buffers[b].vertData.empty())
            continue;

         MeshRenderInst *ri = renderPass->allocInst<MeshRenderInst>();

         // Set our RenderInst as a standard mesh render
         ri->type = RenderPassManager::RIT_Mesh;

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
         //MatrixF objectToWorld = getRenderTransform();
         //objectToWorld.scale(getScale());

         ri->objectToWorld = renderPass->allocUniqueXform(objectToWorld);
         //ri->objectToWorld = renderPass->allocUniqueXform(MatrixF::Identity);
         ri->worldToCamera = renderPass->allocSharedXform(RenderPassManager::View);
         ri->projection = renderPass->allocSharedXform(RenderPassManager::Projection);

         // Make sure we have an up-to-date backbuffer in case
         // our Material would like to make use of it
         // NOTICE: SFXBB is removed and refraction is disabled!
         //ri->backBuffTex = GFX->getSfxBackBuffer();

         // Set our Material
         ri->matInst = state->getOverrideMaterial(mSurfaceMaterials[mBuffers[i].surfaceMaterialId].mMaterialInst ?
            mSurfaceMaterials[mBuffers[i].surfaceMaterialId].mMaterialInst : MATMGR->getWarningMatInstance());
         if (ri->matInst == NULL)
            continue; //if we still have no valid mat, skip out

         // If we need lights then set them up.
         if (ri->matInst->isForwardLit())
         {
            LightQuery query;
            query.init(getWorldSphere());
            query.getLights(ri->lights, 8);
         }

         if (ri->matInst->getMaterial()->isTranslucent())
         {
            ri->translucentSort = true;
            ri->type = RenderPassManager::RIT_Translucent;
         }

         // Set up our vertex buffer and primitive buffer
         ri->vertBuff = &mBuffers[i].buffers[b].vertexBuffer;
         ri->primBuff = &mBuffers[i].buffers[b].primitiveBuffer;

         ri->prim = renderPass->allocPrim();
         ri->prim->type = GFXTriangleList;
         ri->prim->minIndex = 0;
         ri->prim->startIndex = 0;
         ri->prim->numPrimitives = mBuffers[i].buffers[b].primData.size() / 3;
         ri->prim->startVertex = 0;
         ri->prim->numVertices = mBuffers[i].buffers[b].vertData.size();

         // We sort by the material then vertex buffer.
         ri->defaultKey = ri->matInst->getStateHint();
         ri->defaultKey2 = (uintptr_t)ri->vertBuff; // Not 64bit safe!

         // Submit our RenderInst to the RenderPassManager
         state->getRenderPass()->addInst(ri);
      }
   }*/

   // Get a handy pointer to our RenderPassmanager
   RenderPassManager *renderPass = state->getRenderPass();

   // Set up our transforms
   MatrixF objectToWorld = getRenderTransform();
   objectToWorld.scale(getScale());

   for (U32 i = 0; i < mBrushes.size(); i++)
   {
      MeshRenderInst *ri = renderPass->allocInst<MeshRenderInst>();

      // Set our RenderInst as a standard mesh render
      ri->type = RenderPassManager::RIT_Mesh;

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
      //MatrixF objectToWorld = getRenderTransform();
      //objectToWorld.scale(getScale());

      ri->objectToWorld = renderPass->allocUniqueXform(objectToWorld);
      //ri->objectToWorld = renderPass->allocUniqueXform(MatrixF::Identity);
      ri->worldToCamera = renderPass->allocSharedXform(RenderPassManager::View);
      ri->projection = renderPass->allocSharedXform(RenderPassManager::Projection);

      // Make sure we have an up-to-date backbuffer in case
      // our Material would like to make use of it
      // NOTICE: SFXBB is removed and refraction is disabled!
      //ri->backBuffTex = GFX->getSfxBackBuffer();

      // Set our Material
      /*Material *material;

      if (!Sim::findObject(mBrushes[i].mMaterialName, material))
         Sim::findObject("WarningMaterial", material);

      mBrushes[i].mMaterialInst = material->createMatInstance();*/

      ri->matInst = MATMGR->getWarningMatInstance();// state->getOverrideMaterial(mBrushes[i].mMaterialInst ? mBrushes[i].mMaterialInst : MATMGR->getWarningMatInstance());
      if (ri->matInst == NULL)
         continue; //if we still have no valid mat, skip out

                     // If we need lights then set them up.
      if (ri->matInst->isForwardLit())
      {
         LightQuery query;
         query.init(getWorldSphere());
         query.getLights(ri->lights, 8);
      }

      if (ri->matInst->getMaterial()->isTranslucent())
      {
         ri->translucentSort = true;
         ri->type = RenderPassManager::RIT_Translucent;
      }

      // Set up our vertex buffer and primitive buffer
      ri->vertBuff = &mBrushes[i].mVertexBuffer;
      ri->primBuff = &mBrushes[i].mPrimitiveBuffer;

      ri->prim = renderPass->allocPrim();
      ri->prim->type = GFXTriangleList;
      ri->prim->minIndex = 0;
      ri->prim->startIndex = 0;
      ri->prim->numPrimitives = mBrushes[i].mPrimCount;
      ri->prim->startVertex = 0;
      ri->prim->numVertices = mBrushes[i].mVertCount;

      // We sort by the material then vertex buffer.
      ri->defaultKey = ri->matInst->getStateHint();
      ri->defaultKey2 = (uintptr_t)ri->vertBuff; // Not 64bit safe!

                                                   // Submit our RenderInst to the RenderPassManager
      state->getRenderPass()->addInst(ri);
   }
}

DefineEngineMethod(BrushObject, postApply, void, (), ,
   "A utility method for forcing a network update.\n")
{
   object->inspectPostApply();
}

DefineEngineFunction(makeBrushFile, void, (String fileName), ("levels/brushFileTest.brush"), "")
{
   /*U32 size = 10;

   BrushObject* newBrushObj = new BrushObject();
   newBrushObj->registerObject();

   //add 3 materials
   BrushObject::SurfaceMaterials mat1, mat2, mat3;

   mat1.mMaterialName = "orangeGrid";
   mat2.mMaterialName = "grayGrid";
   mat3.mMaterialName = "greenGrid";

   newBrushObj->mSurfaceMaterials.clear();

   newBrushObj->mSurfaceMaterials.push_back(mat1);
   newBrushObj->mSurfaceMaterials.push_back(mat2);
   newBrushObj->mSurfaceMaterials.push_back(mat3);

   //16 * 16 * 256 is the size of a minecraft chunk
   /*for (U32 i = 0; i < size; i++)
   {
      for (U32 j = 0; j < size; j++)
      {
         for (U32 k = 0; k < size; k++)
         {
            newBrushObj->addBoxBrush(Point3F(i, j, k));
         }
      }
   }*/

  /* newBrushObj->addBoxBrush(Point3F(0,0,0));

   newBrushObj->mBrushFile = "levels/brushFileTest.brush";

   newBrushObj->saveBrushFile();*/
}