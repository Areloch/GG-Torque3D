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

#ifndef _BRUSH_OBJECT_H_
#define _BRUSH_OBJECT_H_

#ifndef _SCENEOBJECT_H_
#include "scene/sceneObject.h"
#endif
#ifndef _GFXVERTEXBUFFER_H_
#include "gfx/gfxVertexBuffer.h"
#endif
#ifndef _GFXPRIMITIVEBUFFER_H_
#include "gfx/gfxPrimitiveBuffer.h"
#endif
#ifndef _XMLDOC_H_
#include "console/SimXMLDocument.h"
#endif

class BaseMatInstance;


//-----------------------------------------------------------------------------
// This class implements a basic SceneObject that can exist in the world at a
// 3D position and render itself. There are several valid ways to render an
// object in Torque. This class implements the preferred rendering method which
// is to submit a MeshRenderInst along with a Material, vertex buffer,
// primitive buffer, and transform and allow the RenderMeshMgr handle the
// actual setup and rendering for you.
//-----------------------------------------------------------------------------
class Brush
{
public:
   struct Edge
   {
      U32 p0;
      U32 p1;
   };

   struct Triangle
   {
      U32 p0;
      U32 p1;
      U32 p2;

      U32 operator [](U32 index) const
      {
         AssertFatal(index >= 0 && index <= 2, "index out of range");
         return *((&p0) + index);
      }
   };

   struct FaceUV
   {
      S32 matID;
      Point2F offset;
      Point2F scale;
      float zRot;
      bool horzFlip;
      bool vertFlip;

      FaceUV()
      {
         matID = 0;
         offset = Point2F::Zero;
         scale = Point2F::One;
         zRot = 0;
         horzFlip = vertFlip = false;
      }
   };

   struct Face
   {
      Vector< Edge > edges;
      Vector< U32 > points;
      Vector< U32 > winding;
      Vector< Point2F > texcoords;
      Vector< Triangle > triangles;
      Point3F tangent;
      Point3F normal;
      Point3F centroid;
      F32 area;
      S32 id;
      U32 materialId;
      FaceUV uvs;
      PlaneF plane;
   };
   
   struct Geometry
   {
      void generate(const Vector< PlaneF > &planes, const Vector< Point3F > &tangentss, const Vector< Brush::FaceUV > &uvs);

      Vector< Point3F > points;
      Vector< Face > faces;
   };

   Geometry mGeometry;

   Vector< PlaneF > mPlanes;

   Vector< MatrixF > mSurfaces;

   Vector<FaceUV> mSurfaceUVs;

   Vector< Point3F > mFaceCenters;

   Box3F mBounds;

   void updateGeometry();
   void updateBounds(bool recenter);
};

GFXDeclareVertexFormat(BrushVert)
{
   Point3F point;
   GFXVertexColor color;
   Point3F normal;
   Point3F tangent;
   Point2F texCoord;
};
typedef BrushVert VertexType;

class BrushObject : public SceneObject
{
   typedef SceneObject Parent;

   // Networking masks
   // We need to implement a mask specifically to handle
   // updating our transform from the server object to its
   // client-side "ghost". We also need to implement a
   // maks for handling editor updates to our properties
   // (like material).
   enum MaskBits
   {
      TransformMask = Parent::NextFreeMask << 0,
      UpdateMask = Parent::NextFreeMask << 1,
      NextFreeMask = Parent::NextFreeMask << 2
   };

public:
   //--------------------------------------------------------------------------
   // Rendering variables
   //--------------------------------------------------------------------------
   // The name of the Material we will use for rendering
   String            mMaterialName;
   // The actual Material instance
   BaseMatInstance*  mMaterialInst;

   struct SurfaceMaterials
   {
      // The name of the Material we will use for rendering
      String            mMaterialName;
      // The actual Material instance
      BaseMatInstance*  mMaterialInst;

      SurfaceMaterials()
      {
         mMaterialName = "";
         mMaterialInst = NULL;
      }
   };

   Vector<SurfaceMaterials> mSurfaceMaterials;

   struct BufferSet
   {
      U32 surfaceMaterialId;

      U32 vertCount;
      U32 primCount;

      struct Buffers
      {
         U32 vertStart;
         U32 primStart;
         U32 vertCount;
         U32 primCount;

         Vector<VertexType> vertData;
         Vector<U32> primData;

         GFXVertexBufferHandle< VertexType > vertexBuffer;
         GFXPrimitiveBufferHandle            primitiveBuffer;

         Buffers()
         {
            vertStart = 0;
            primStart = 0;
            vertCount = 0;
            primCount = 0;

            vertexBuffer = NULL;
            primitiveBuffer = NULL;
         }
      };

      Vector<Buffers> buffers;

      BufferSet()
      {
         Buffers newBuffer;
         buffers.push_back(newBuffer);

         surfaceMaterialId = 0;

         vertCount = 0;
         primCount = 0;
      }
   };

   Vector<BufferSet>    mBuffers;

   Vector<Brush> mBrushes;

   U32 mVertCount;
   U32 mPrimCount;

   StringTableEntry		         mBrushFile;
   SimObjectPtr<SimXMLDocument>  mXMLReader;

public:
   BrushObject();
   virtual ~BrushObject();

   // Declare this object as a ConsoleObject so that we can
   // instantiate it into the world and network it
   DECLARE_CONOBJECT(BrushObject);

   //--------------------------------------------------------------------------
   // Object Editing
   // Since there is always a server and a client object in Torque and we
   // actually edit the server object we need to implement some basic
   // networking functions
   //--------------------------------------------------------------------------
   // Set up any fields that we want to be editable (like position)
   static void initPersistFields();

   // Allows the object to update its editable settings
   // from the server object to the client
   virtual void inspectPostApply();

   // Handle when we are added to the scene and removed from the scene
   bool onAdd();
   void onRemove();

   virtual void writeFields(Stream &stream, U32 tabStop);
   virtual bool writeField(StringTableEntry fieldname, const char *value);

   // Override this so that we can dirty the network flag when it is called
   void setTransform(const MatrixF &mat);

   // This function handles sending the relevant data from the server
   // object to the client object
   U32 packUpdate(NetConnection *conn, U32 mask, BitStream *stream);
   // This function handles receiving relevant data from the server
   // object and applying it to the client object
   void unpackUpdate(NetConnection *conn, BitStream *stream);

   virtual bool castRay(const Point3F &start, const Point3F &end, RayInfo *info);

   //--------------------------------------------------------------------------
   // Object Rendering
   // Torque utilizes a "batch" rendering system. This means that it builds a
   // list of objects that need to render (via RenderInst's) and then renders
   // them all in one batch. This allows it to optimized on things like
   // minimizing texture, state, and shader switching by grouping objects that
   // use the same Materials.
   //--------------------------------------------------------------------------
   // Create the geometry for rendering
   void createGeometry();

   // Get the Material instance
   void updateMaterials();

   void updateBounds(bool recenter);

   // This is the function that allows this object to submit itself for rendering
   void prepRenderImage(SceneRenderState *state);

   void _renderDebug(ObjectRenderInst *ri, SceneRenderState *state, BaseMatInstance *mat);

   void addBrush(Point3F center, const Vector<MatrixF> surfaces, const Vector<Brush::FaceUV> uvs);

   void addBoxBrush(Point3F center);

   void loadBrushFile();
   void saveBrushFile();

   U32 findBufferSetByMaterial(U32 matId)
   {
      for (U32 i = 0; i < mBuffers.size(); i++)
      {
         if (mBuffers[i].surfaceMaterialId == matId)
            return i;
      }

      return -1;
   }
};

#endif // _BRUSH_OBJECT_H_