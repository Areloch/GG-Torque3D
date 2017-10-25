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

#ifndef _BRUSH_EDITOR_TOOL_
#define _BRUSH_EDITOR_TOOL_

#ifndef _EDITOR_TOOL_
#include "gui/worldEditor/tools/editorTool.h"
#endif

#ifndef _BRUSH_OBJECT_H_
#include "T3D/BrushObject.h"
#endif

#include "math/util/CSG.h"

class Brush;

class BrushEditorTool : public EditorTool
{
   typedef EditorTool Parent;

   struct EditBrush
   {
      std::vector<CSGUtils::csgjs_polygon> mCSG;
      CSGUtils::csgjs_model mCSGModel;

      bool mIsSubtract;

      Box3F mBounds;

      // The name of the Material we will use for rendering
      String            mMaterialName;

      // The actual Material instance
      BaseMatInstance*  mMaterialInst;

      // The GFX vertex and primitive buffers
      GFXVertexBufferHandle< VertexType > mVertexBuffer;
      GFXPrimitiveBufferHandle            mPrimitiveBuffer;

      U32 mVertCount;
      U32 mPrimCount;
   };

private:
   struct BrushSelection
   {
      U32 brush;
   };
   struct FaceSelection
   {
      U32 brush;
      U32 face;
   };
   struct VertSelection
   {
      U32 brush;
      U32 vert;
   };

   Vector<BrushSelection> mSelectedBrushes;
   Vector<FaceSelection> mSelectedFaces;
   Vector<VertSelection> mVertBrushes;

   RayInfo mLastRayInfo;

   bool mMouseDown;

   Vector<EditBrush> mBrushes;

   BrushObject* mBrushObj;

   //Convex generation geometry
   struct Geometry
   {
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
      };

      void generate(const Vector< PlaneF > &planes, const Vector< Point3F > &tangents);

      void getSurfaceTriangles(S32 surfId, Vector< Point3F > *outPoints, Vector< Point2F > *outCoords, bool worldSpace);
      void getSurfaceVerts(U32 faceId, Vector< Point3F > *outPoints, Vector< Point2F > *outCoords, bool worldSpace);
      S32 getFaceId(U32 surfId);

      Vector< Point3F > points;
      Vector< Face > faces;

      Vector<MatrixF> surfaces;

      MatrixF worldTransform;
      Point3F scale;
   };
   Geometry mGeometry;

   //
   //Compiling stuff
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

   U32 mPrimCount;
   U32 mVertCount;

public:
   BrushEditorTool();
   ~BrushEditorTool(){}

   DECLARE_CONOBJECT(BrushEditorTool);

   bool onAdd();
   void onRemove();

   //Called when the tool is activated on the World Editor
   virtual void onActivated(WorldEditor*);

   //Called when the tool is deactivated on the World Editor
   virtual void onDeactivated();

   //
   virtual bool onMouseMove(const Gui3DMouseEvent &);
   virtual bool onMouseDown(const Gui3DMouseEvent &);
   virtual bool onMouseDragged(const Gui3DMouseEvent &);
   virtual bool onMouseUp(const Gui3DMouseEvent &);

   //
   virtual bool onRightMouseDown(const Gui3DMouseEvent &);
   virtual bool onRightMouseDragged(const Gui3DMouseEvent &);
   virtual bool onRightMouseUp(const Gui3DMouseEvent &);

   //
   virtual bool onMiddleMouseDown(const Gui3DMouseEvent &);
   virtual bool onMiddleMouseDragged(const Gui3DMouseEvent &);
   virtual bool onMiddleMouseUp(const Gui3DMouseEvent &);

   //
   virtual bool onInputEvent(const InputEventInfo &);

   //
   virtual void render();

   //
   void compileGeometry();
   void processBrushes();

   U32 findBufferSetByMaterial(U32 matId)
   {
      for (U32 i = 0; i < mBuffers.size(); i++)
      {
         if (mBuffers[i].surfaceMaterialId == matId)
            return i;
      }

      return -1;
   }

   //Creation functions
   EditBrush addBoxBrush(Box3F, bool);
};

#endif