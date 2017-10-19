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

void Brush::updateGeometry()
{
   mPlanes.clear();

   for (S32 i = 0; i < mSurfaces.size(); i++)
      mPlanes.push_back(PlaneF(mSurfaces[i].getPosition(), mSurfaces[i].getUpVector()));

   Vector< Point3F > tangents;
   for (S32 i = 0; i < mSurfaces.size(); i++)
      tangents.push_back(mSurfaces[i].getRightVector());

   mGeometry.generate(mPlanes, tangents, mSurfaceUVs);

   AssertFatal(mGeometry.faces.size() <= mSurfaces.size(), "Got more faces than planes?");

   const Vector< Face > &faceList = mGeometry.faces;
   const Vector< Point3F > &pointList = mGeometry.points;

   // Reset our surface center points.

   for (S32 i = 0; i < faceList.size(); i++)
      mSurfaces[faceList[i].id].setPosition(faceList[i].centroid);

   mPlanes.clear();

   for (S32 i = 0; i < mSurfaces.size(); i++)
      mPlanes.push_back(PlaneF(mSurfaces[i].getPosition(), mSurfaces[i].getUpVector()));

   // Update bounding box.   
   updateBounds(false);
}

void Brush::updateBounds(bool recenter)
{
   if (mGeometry.points.size() == 0)
      return;

   Vector<Point3F> &pointListOS = mGeometry.points;
   U32 pointCount = pointListOS.size();

   Point3F volumnCenter(0, 0, 0);
   F32 areaSum = 0.0f;

   F32 faceCount = mGeometry.faces.size();

   for (S32 i = 0; i < faceCount; i++)
   {
      volumnCenter += mGeometry.faces[i].centroid * mGeometry.faces[i].area;
      areaSum += mGeometry.faces[i].area;
   }

   if (areaSum == 0.0f)
      return;

   volumnCenter /= areaSum;

   mBounds.minExtents = mBounds.maxExtents = Point3F::Zero;
   mBounds.setCenter(volumnCenter);

   for (S32 i = 0; i < pointCount; i++)
      mBounds.extend(pointListOS[i]);

   bool tmp = true;
   //resetWorldBox();
}

void Brush::Geometry::generate(const Vector< PlaneF > &planes, const Vector< Point3F > &tangents, const Vector< Brush::FaceUV > &uvs)
{
   PROFILE_SCOPE(Brush_Geometry_generate);

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
         Brush::Edge &newEdge = newFace.edges.last();
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
         Brush::Triangle &poly = newFace.triangles[j];

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

      // Data verification tests.
#ifdef TORQUE_ENABLE_ASSERTS
      //S32 triCount = newFace.triangles.size();
      //S32 edgeCount = newFace.edges.size();
      //AssertFatal( triCount == edgeCount - 2, "ConvexShape - triangle/edge count do not match." );

      /*
      for ( S32 j = 0; j < triCount; j++ )
      {
      F32 area = MathUtils::mTriangleArea( points[ newFace.points[ newFace.triangles[j][0] ] ],
      points[ newFace.points[ newFace.triangles[j][1] ] ],
      points[ newFace.points[ newFace.triangles[j][2] ] ] );
      AssertFatal( area > 0.0f, "ConvexShape - triangle winding bad." );
      }*/
#endif


      // Done with this Face.

      faces.push_back(newFace);
   }
}

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

   // Make sure we the Material instance to NULL
   // so we don't try to access it incorrectly
   mMaterialInst = NULL;

   mBrushFile = "";
}

BrushObject::~BrushObject()
{
   for (U32 i = 0; i < mSurfaceMaterials.size(); i++)
   {
      if (mSurfaceMaterials[i].mMaterialInst)
         SAFE_DELETE(mSurfaceMaterials[i].mMaterialInst);
   }
   if (mMaterialInst)
      SAFE_DELETE(mMaterialInst);
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

   addGroup("Rendering");
   addField("material", TypeMaterialName, Offset(mMaterialName, BrushObject),
      "The name of the material used to render the mesh.");
   endGroup("Rendering");

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
   mBrushes.clear();
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

   bool tmp = true;
}

void BrushObject::saveBrushFile()
{
   //prep an xml document reader so we can save to our brush file
   SimXMLDocument *xmlrdr = new SimXMLDocument();
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

   xmlrdr->saveFile(mBrushFile);
}

bool BrushObject::castRay(const Point3F &start, const Point3F &end, RayInfo *info)
{
   if (mBrushes.empty())
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
               VectorF v0 = t2 - t0;
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
            if (validHit)
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
   }

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

   if (stream->writeFlag(mask & UpdateMask))
   {
      stream->writeInt(mSurfaceMaterials.size(), 32);

      for (U32 i = 0; i < mSurfaceMaterials.size(); i++)
         stream->write(mSurfaceMaterials[i].mMaterialName);

      stream->writeString(mBrushFile);
   }

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

   if (stream->readFlag()) // UpdateMask
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
   }
}

void BrushObject::updateBounds(bool recenter)
{
   if (mBrushes.size() == 0)
      return;

   mObjBox.set(Point3F::Zero, Point3F::Zero);

   for (U32 i = 0; i < mBrushes.size(); i++)
   {
      if (mBrushes[i].mGeometry.points.size() == 0)
         return;

      Vector<Point3F> &pointListOS = mBrushes[i].mGeometry.points;
      U32 pointCount = pointListOS.size();

      for (S32 i = 0; i < pointCount; i++)
         mObjBox.extend(pointListOS[i]);

      resetWorldBox();
   }
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
void BrushObject::addBoxBrush(Point3F center)
{
   Brush newBrush;

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

   {
      for (S32 i = 0; i < 6; i++)
      {
         newBrush.mSurfaces.increment();
         MatrixF &surf = newBrush.mSurfaces.last();

         surf.identity();

         surf.setColumn(0, cubeTangents[i]);
         surf.setColumn(1, cubeBinormals[i]);
         surf.setColumn(2, cubeNormals[i]);
         surf.setPosition(cubeNormals[i] * 0.5f + center);

         newBrush.mSurfaceUVs.increment();
      }
   }

   /*for (S32 i = 0; i < 6; i++)
   {
      newBrush.mSurfaces.increment();
      MatrixF &surf = newBrush.mSurfaces.last();

      surf.identity();

      surf.setColumn(0, cubeTangents[i]);
      surf.setColumn(1, cubeBinormals[i]);
      surf.setColumn(2, cubeNormals[i]);
      surf.setPosition((cubeNormals[i] * 0.5f));

      newBrush.mSurfaceUVs.increment();
      Brush::FaceUV &uv = newBrush.mSurfaceUVs.last();
      uv.horzFlip = mRandF() > 0.5;
      uv.vertFlip = mRandF() > 0.5;
      uv.matID = mRandI(0, mSurfaceMaterials.size()-1);
      uv.offset = Point2F(mRandF(), mRandF());
      uv.scale = Point2F(mRandF(0.1, 2), mRandF(0.1, 2));
      uv.zRot = mRandF(0, 360);
   }*/

   newBrush.updateGeometry();

   mBrushes.push_back(newBrush);
}

void BrushObject::addBrush(Point3F center, const Vector<MatrixF> surfaces, const Vector<Brush::FaceUV> uvs)
{
   Brush newBrush;

   newBrush.mSurfaces = surfaces;

   newBrush.mSurfaceUVs = uvs;

   newBrush.updateGeometry();

   mBrushes.push_back(newBrush);
}

void BrushObject::createGeometry()
{
   // Server does not need to generate vertex/prim buffers.
   if (isServerObject())
      return;

   //if (updateCollision)
   //   _updateCollision();

   for (U32 i = 0; i < mBuffers.size(); i++)
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
   }
}

void BrushObject::updateMaterials()
{
   // Server does not need to load materials
   if (isServerObject())
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
   }
}

void BrushObject::prepRenderImage(SceneRenderState *state)
{
   // Do a little prep work if needed
   if (mBuffers.empty() || !state)
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
   }
}

void BrushObject::_renderDebug(ObjectRenderInst *ri, SceneRenderState *state, BaseMatInstance *mat)
{
   GFXDrawUtil *drawer = GFX->getDrawUtil();

   GFX->setTexture(0, NULL);

   for (U32 i = 0; i < mBrushes.size(); i++)
   {
      const Vector< Point3F > &pointList = mBrushes[i].mGeometry.points;
      const Vector< Brush::Face > &faceList = mBrushes[i].mGeometry.faces;

      // Render world box.
      if (false)
      {
         Box3F wbox = Box3F::Zero;
         
         for (U32 s = 0; s < faceList.size(); s++)
         {
            for (U32 p = 0; p < faceList[s].points.size(); p++)
            {
               wbox.extend(pointList[faceList[s].points[p]]);
            }
         }

         GFXStateBlockDesc desc;
         desc.setCullMode(GFXCullNone);
         desc.setFillModeWireframe();
         drawer->drawCube(desc, wbox, ColorI::WHITE);
      }

      // Render Edges.
      if (true)
      {
         GFXTransformSaver saver;
         //GFXFrustumSaver fsaver;

         MatrixF xfm(getRenderTransform());
         xfm.scale(getScale());
         GFX->multWorld(xfm);

         GFXStateBlockDesc desc;
         desc.setZReadWrite(true, false);
         desc.setBlend(true);
         GFX->setStateBlockByDesc(desc);

         //MathUtils::getZBiasProjectionMatrix( 0.01f, state->getFrustum(), )

         const Point3F &camFvec = state->getCameraTransform().getForwardVector();

         for (S32 f = 0; f < faceList.size(); f++)
         {
            const Brush::Face &face = faceList[f];

            for (U32 tri = 0; tri < face.triangles.size(); tri++)
            {
               PrimBuild::begin(GFXLineList, 6);

               PrimBuild::color(ColorI::WHITE * 0.8f);

               U32 p0 = face.triangles[tri].p0;
               U32 p1 = face.triangles[tri].p1;
               U32 p2 = face.triangles[tri].p2;

               PrimBuild::vertex3fv(pointList[face.triangles[tri].p0]);
               PrimBuild::vertex3fv(pointList[face.triangles[tri].p1]);
               PrimBuild::vertex3fv(pointList[face.triangles[tri].p1]);
               PrimBuild::vertex3fv(pointList[face.triangles[tri].p2]);
               PrimBuild::vertex3fv(pointList[face.triangles[tri].p2]);
               PrimBuild::vertex3fv(pointList[face.triangles[tri].p0]);

               PrimBuild::end();
            }
         }
      }

      ColorI faceColorsx[4] =
      {
         ColorI(255, 0, 0),
         ColorI(0, 255, 0),
         ColorI(0, 0, 255),
         ColorI(255, 0, 255)
      };

      MatrixF objToWorld(mObjToWorld);
      objToWorld.scale(mObjScale);

      // Render faces centers/colors.
      if (false)
      {
         GFXStateBlockDesc desc;
         desc.setCullMode(GFXCullNone);

         Point3F size(0.1f);

         for (S32 f = 0; f < faceList.size(); f++)
         {
            ColorI color = faceColorsx[f % 4];
            S32 div = (f / 4) * 4;
            if (div > 0)
               color /= div;
            color.alpha = 255;

            Point3F pnt;
            objToWorld.mulP(faceList[f].centroid, &pnt);
            drawer->drawCube(desc, size, pnt, color, NULL);
         }
      }

      // Render winding order.
      if (false)
      {
         GFXStateBlockDesc desc;
         desc.setCullMode(GFXCullNone);
         desc.setZReadWrite(true, false);
         GFX->setStateBlockByDesc(desc);

         U32 pointCount = 0;
         for (S32 f = 0; f < faceList.size(); f++)
            pointCount += faceList[f].winding.size();

         PrimBuild::begin(GFXLineList, pointCount * 2);

         for (S32 f = 0; f < faceList.size(); f++)
         {
            for (S32 j = 0; j < faceList[f].winding.size(); j++)
            {
               Point3F p0 = pointList[faceList[f].points[faceList[f].winding[j]]];
               Point3F p1 = p0 + mBrushes[i].mSurfaces[faceList[f].id].getUpVector() * 0.75f * (Point3F::One / mObjScale);

               objToWorld.mulP(p0);
               objToWorld.mulP(p1);

               ColorI color = faceColorsx[j % 4];
               S32 div = (j / 4) * 4;
               if (div > 0)
                  color /= div;
               color.alpha = 255;

               PrimBuild::color(color);
               PrimBuild::vertex3fv(p0);
               PrimBuild::color(color);
               PrimBuild::vertex3fv(p1);
            }
         }

         PrimBuild::end();
      }

      // Render Points.
      if (false)
      {
         /*
         GFXTransformSaver saver;

         MatrixF xfm( getRenderTransform() );
         xfm.scale( getScale() );
         GFX->multWorld( xfm );

         GFXStateBlockDesc desc;
         Point3F size( 0.05f );
         */
      }

      // Render surface transforms.
      if (false)
      {
         GFXStateBlockDesc desc;
         desc.setBlend(false);
         desc.setZReadWrite(true, true);

         F32 mNormalLength = 10;

         Point3F scale(mNormalLength);

         for (S32 s = 0; s < faceList.size(); s++)
         {
            MatrixF objToWorld(mObjToWorld);
            objToWorld.scale(mObjScale);

            //MatrixF faceSurface;
            
            Point3F average = Point3F::Zero;
            for (U32 p = 0; p < faceList[s].points.size(); p++)
            {
               average += pointList[faceList[s].points[p]];
               drawer->drawCube(desc, Box3F(pointList[faceList[s].points[p]] - Point3F(0.1, 0.1, 0.1), 
                  pointList[faceList[s].points[p]] + Point3F(0.1, 0.1, 0.1)), ColorI(0, 255, 0));
            }
            average /= faceList[s].points.size();

            //MatrixF renderMat;
            //renderMat.mul(objToWorld, mBrushes[i].mSurfaces[s]);

            //renderMat.setPosition(renderMat.getPosition() + renderMat.getUpVector() * 0.001f);

            Point3F normal = faceList[s].normal;
            normal.normalize();

            //drawer->drawCube(desc, Box3F(average - Point3F(0.1, 0.1, 0.1), average + Point3F(0.1, 0.1, 0.1)), ColorI(0, 255, 0));

            drawer->drawLine(average, average + normal, ColorI(0, 0, 255));

            drawer->drawLine(average, average + faceList[s].tangent, ColorI(255, 0, 0));

            //drawer->drawTransform(desc, renderMat, &scale, NULL);
         }
      }
   }
}

DefineEngineMethod(BrushObject, postApply, void, (), ,
   "A utility method for forcing a network update.\n")
{
   object->inspectPostApply();
}

DefineEngineFunction(makeBrushFile, void, (String fileName), ("levels/brushFileTest.brush"), "")
{
   U32 size = 10;

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

   newBrushObj->addBoxBrush(Point3F(0,0,0));

   newBrushObj->mBrushFile = "levels/brushFileTest.brush";

   newBrushObj->saveBrushFile();
}