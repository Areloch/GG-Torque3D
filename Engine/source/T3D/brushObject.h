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

#include "math/util/csg.hpp"

class BaseMatInstance;
class brushEditorTool;

GFXDeclareVertexFormat(BrushVert)
{
   Point3F point;
   GFXVertexColor color;
   Point3F normal;
   Point3F tangent;
   Point2F texCoord;
};
typedef BrushVert VertexType;

struct Brush
{
   CSG mCSG;
   CSGModel mCSGModel;

   bool mIsSubtract;

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

class BrushObject : public SceneObject
{
   typedef SceneObject Parent;
   friend class brushEditorTool;

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
   Vector<Brush> mBrushes;

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

   bool castRay(const Point3F &start, const Point3F &end, RayInfo *info);

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

   void loadBrushFile();
   void saveBrushFile();
};

#endif // _BRUSH_OBJECT_H_