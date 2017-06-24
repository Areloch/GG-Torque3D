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
#include "gui/worldEditor/guiConvexShapeEditorCtrl.h"

#include "console/consoleTypes.h"
#include "console/engineAPI.h"
#include "T3D/convexShape.h"
#include "renderInstance/renderPassManager.h"
#include "collision/collision.h"
#include "math/util/frustum.h"
#include "math/mathUtils.h"
#include "gfx/gfxPrimitiveBuffer.h"
#include "gfx/gfxTextureHandle.h"
#include "gfx/gfxTransformSaver.h"
#include "gfx/primBuilder.h"
#include "gfx/gfxDrawUtil.h"
#include "scene/sceneRenderState.h"
#include "scene/sceneManager.h"
#include "gui/core/guiCanvas.h"
#include "gui/buttons/guiButtonCtrl.h"
#include "gui/worldEditor/undoActions.h"
#include "T3D/gameBase/gameConnection.h"
#include "gfx/sim/debugDraw.h"
#include "collision/optimizedPolyList.h"
#include "core/volume.h"
#include "gui/worldEditor/worldEditor.h"
#include "T3D/prefab.h"
#include "T3D/trigger.h"
#include "T3D/zone.h"
#include "T3D/portal.h"
#include "T3D/reflectionProbe.h"
#include "math/mPolyhedron.impl.h"

IMPLEMENT_CONOBJECT( GuiConvexEditorCtrl );

ConsoleDocClass( GuiConvexEditorCtrl,
   "@brief The base class for the sketch tool\n\n"
   "Editor use only.\n\n"
   "@internal"
);

//CSG Stuff
Polygon::Polygon()
{
   surfacePlane = PlaneF(Point3F(0, 0, 0), Point3F(0, 0, 1));
}

Polygon::Polygon(Vector<Point3F> _points)
{
   points = _points;
   surfacePlane = PlaneF(points[0], points[1], points[2]);
   //Simplify();
}

Vector<Polygon::Edge> Polygon::getEdges()
{
   Vector<Edge> edgeList;
   for (U32 i = 0; i < points.size(); ++i)
   {
      U32 v0 = i;
      U32 v1 = i + 1 == points.size() ? 0 : i + 1;
      Edge e = Edge(v0, v1);
      edgeList.push_back(e);
   }

   return edgeList;
}

bool Polygon::isValid()
{
   bool valid = true;
   for (U32 i = 0; i < points.size(); ++i)
   {
      if (surfacePlane.whichSide(points[i]) != PlaneF::Side::On)
      {
         valid = false;
         break;
      }
   }

   return valid;
}

void Polygon::simplify()
{

}

bool Polygon::isConvex()
{
   return true;
}

//Gets whether this polygon is in front of, behind, or on, or intersecting a plane
// Returns an enum to indicate relation to plane
Polygon::PlaneRelation Polygon::getPlaneSide(PlaneF p)
{
   U32 back = 0;
   U32 front = 0;
   U32 on = 0;
   U32 pointCount = points.size();

   for (U32 i = 0; i < pointCount; ++i)
   {
      PlaneF::Side s = p.whichSide(points[i]);

      if (s == PlaneF::Side::Back)
         back++;
      else if (s == PlaneF::Side::Front)
         front++;
      else
         on++;
   }

   if (back == pointCount)
      return PlaneRelation::Back;
   else if (front == pointCount)
      return PlaneRelation::Front;
   else if (on == pointCount)
      return PlaneRelation::On;
   else
      return PlaneRelation::Intersect;
}

bool Polygon::split(PlaneF clip)
{
   Polygon back, front;
   if (split(clip, &back, &front))
   {
      points = back.points;
      surfacePlane = back.surfacePlane;

      return true;
   }

   return false;
}

bool Polygon::split(PlaneF clip, Polygon* back, Polygon *front)
{
   Polygon cBack, cFront;
   return split(clip, back, front, &cBack, &cFront);
}

//Splits the polygon along a plane, if valid. If it's coplanar, the coplanar vars will be non-null
bool Polygon::split(PlaneF clip, Polygon* back, Polygon *front, Polygon* coplanarBack, Polygon* coplanarFront)
{
   PlaneRelation relation = getPlaneSide(clip);

   if (relation != PlaneRelation::Intersect)
   {
      //We don't split this, so just set appropriate returns and exit
      back = front = nullptr;
      coplanarBack = coplanarFront = nullptr;

      if (relation == PlaneRelation::Back)
         back = this;
      else if (relation == PlaneRelation::Front)
         front = this;
      else
         coplanarBack = coplanarFront = this;

      return false;
   }

   Vector<Point3F> backVerts;
   Vector<Point3F> frontVerts;

   Vector<Edge> edges = getEdges();

   for (U32 i = 0; i < edges.size(); ++i)
   {
      Point3F pointA = points[edges[i].points[0]];
      Point3F pointB = points[edges[i].points[1]];

      F32 t = clip.intersect(pointA, pointB);
      if (t != 0 && t != 1)
      {
         Point3F intersectPoint;
         intersectPoint.interpolate(pointA, pointB, t);

         backVerts.push_back(intersectPoint);
         frontVerts.push_back(intersectPoint);
      }

      PlaneF::Side pASide = clip.whichSide(pointA);
      PlaneF::Side pBSide = clip.whichSide(pointB);

      if (pASide == PlaneF::Side::Back)
         backVerts.push_back(pointA);
      else
         frontVerts.push_back(pointA);

      if (pBSide == PlaneF::Side::Back)
         backVerts.push_back(pointB);
      else
         frontVerts.push_back(pointB);
   }

   back = new Polygon(backVerts);
   front = new Polygon(frontVerts);
   coplanarBack = coplanarFront = nullptr;

   return true;
}

void Polygon::flip()
{
   points.reverse();
   surfacePlane = PlaneF(surfacePlane.getPosition(), -surfacePlane.getNormal());
}


CSGSolid::CSGSolid(Vector<Polygon> _polygons)
{
   polygons = _polygons;
}

CSGSolid::CSGSolid(ConvexShape* shape)
{
   Polyhedron polyList = convertToPolyhedron(shape);

   polygons.setSize(polyList.planeList.size());

   for (U32 i = 0; i < polyList.edgeList.size(); i++)
   {
      Point3F vertA = polyList.pointList[polyList.edgeList[i].vertex[0]];
      Point3F vertB = polyList.pointList[polyList.edgeList[i].vertex[1]];

      U32 planeIdx = polyList.edgeList[i].face[0];
      PlaneF plane = polyList.planeList[planeIdx];

      polygons[planeIdx].points.push_back_unique(vertA);
      polygons[planeIdx].points.push_back_unique(vertB);
      polygons[planeIdx].surfacePlane = PlaneF(plane.getPosition(), -plane.getNormal());
   }

   for (U32 i = 0; i < polygons.size(); i++)
   {
      U32 pointCount = polygons[i].points.size();
      for (U32 v = 0; v < polygons[i].points.size(); ++v)
      {
         Point3F p = polygons[i].points[v];

         bool t = true;
      }

      PlaneF srf = polygons[i].surfacePlane;
      Point3F srfPs = srf.getPosition();
      Point3F srfNrm = srf.getNormal();

      bool n = true;

      Vector<Polygon::Edge> edges = polygons[i].getEdges();

      for (U32 e = 0; e < edges.size(); ++e)
      {
         U32 pA = edges[e].points[0];
         U32 pB = edges[e].points[1];

         Point3F pointA = polygons[i].points[edges[e].points[0]];
         Point3F pointB = polygons[i].points[edges[e].points[1]];

         DebugDrawer::get()->drawLine(pointA, pointB, ColorF(1, 0.9, 0));
         DebugDrawer::get()->setLastTTL(30000);
      }
   }

   bool tmp = true;
}

CSGSolid::CSGSolid()
{
   polygons.clear();
}

CSGSolid CSGSolid::doUnion(CSGSolid* solid)
{
   /*CSGNode* a = new CSGNode(this);
   CSGNode* b = new CSGNode(solid);

   a->clipTo(b);
   b->clipTo(a);

   b->invert();
   b->clipTo(a);

   b->invert();
   a->build(b->allPolygons());

   return CSGSolid(a->allPolygons());*/

   return CSGSolid();
}

CSGSolid CSGSolid::doSubtract(CSGSolid* solid)
{
   /*CSGSolid* a = new CSGSolid(this->polygons);
   CSGSolid* b = new CSGSolid(solid->polygons);

   a->invert();
   a->clipTo(b);

   b->clipTo(a);
   b->invert();

   b->clipTo(a);
   b->invert();

   a->build(b->allPolygons());
   a->invert();

   return CSGSolid(a->allPolygons());*/

   return CSGSolid();
}

CSGSolid CSGSolid::doIntersect(CSGSolid* solid)
{
   /*CSGNode* a = new CSGNode(this);
   CSGNode* b = new CSGNode(solid);

   a->invert();
   b->clipTo(a);

   b->invert();
   a->clipTo(b);

   b->clipTo(a);
   a->build(b->allPolygons());
   a->invert();

   return CSGSolid(a->allPolygons());*/

   return CSGSolid();
}

Vector<Polygon> CSGSolid::clipPolygons(Vector<Polygon> &_polygons)
{
   //if (!plane)
      return polygons;

   /*Vector<Polygon> _front;
   Vector<Polygon> _back;

   for (U32 i = 0; i < _polygons.size(); ++i)
   {
      Polygon* b;
      Polygon* f;
      Polygon* cf;
      Polygon* cb;

      Polygon* poly = &_polygons[i];
      poly->split(plane, b, f, cb, cf);

      if (f)
         _front.push_back(*f);
      if (b)
         _front.push_back(*b);
      if (cf)
         _front.push_back(*cf);
      if (cb)
         _front.push_back(*cb);
   }

   if (front)
      _front = front->clipPolygons(_front);

   if (back)
      _back = back->clipPolygons(_back);
   else
      _back.clear();

   return _front*/
}

void CSGSolid::clipTo(CSGSolid* csg)
{
   polygons = csg->clipPolygons(polygons);

   /*if (front)
      front->clipTo(csg);

   if (back)
      back->clipTo(csg);*/
}

void CSGSolid::invert()
{
   for (U32 i = 0; i < polygons.size(); ++i)
   {
      Polygon* poly = &polygons[i];

      poly->flip();
   }

   /*plane = PlaneF(plane.getPosition(), -plane.getNormal());

   if (front)
      front->invert();

   if (back)
      back->invert();

   CSGNode* temp = front;
   front = back;
   back = temp;*/
}

void CSGSolid::build(Vector<Polygon> _polygons)
{
   if (_polygons.empty())
      return;

   /*plane = _polygons[0].surfacePlane;
   Vector<Polygon> _front;
   Vector<Polygon> _back;

   for (U32 i = 0; i < _polygons.size(); ++i)
   {
      Polygon* f = new Polygon();
      Polygon* b = new Polygon();
      Polygon* cf = new Polygon();
      Polygon* cb = new Polygon();

      Polygon* poly = &_polygons[i];
      poly->split(plane, b, f, cb, cf);

      if (b)
         _back.push_back(*b);
      if (f)
         _front.push_back(*f);
      if (cf)
         polygons.push_back(*cf);
      if (cb)
         polygons.push_back(*cb);
   }

   if (!_front.empty())
   {
      if (!front)
         front = new CSGNode();

      front->build(_front);
   }

   if (!_back.empty())
   {
      if (!back)
         back = new CSGNode();

      back->build(_back);
   }*/
}

Polyhedron CSGSolid::convertToPolyhedron(ConvexShape* targetBrush)
{
   // Extract the geometry.  Use the object-space bounding volumes
   // as we have moved the object to the origin for the moment.
   OptimizedPolyList polyList;
   if (!targetBrush->buildPolyList(PLC_Export, &polyList,
      targetBrush->getObjBox(), targetBrush->getObjBox().getBoundingSphere()))
   {
      Con::errorf("GuiConvexEditorCtrl::convertToPolyhedron - Failed to extract geometry!");
      return Polyhedron();
   }

   // Convert the polylist to a polyhedron.
   Polyhedron tempPoly = polyList.toPolyhedron();

   return tempPoly;
}

void CSGSolid::convertConvexShape(ConvexShape* outBrush)
{
   /*const U32 numPlanes = poly->getNumPlanes();
   if (!numPlanes)
   {
      Con::errorf("GuiConvexEditorCtrl::convertFromPolyhedron - not a valid polyhedron");
      return;
   }

   // Add all planes.
   for (U32 i = 0; i < numPlanes; ++i)
   {
      const PlaneF& plane = poly->getPlanes()[i];

      // Polyhedron planes are facing inwards so we need to
      // invert the normal here.

      Point3F normal = plane.getNormal();
      normal.neg();

      // Turn the orientation of the plane into a quaternion.
      // The normal is our up vector (that's what's expected
      // by ConvexShape for the surface orientation).

      MatrixF orientation(true);
      MathUtils::getMatrixFromUpVector(normal, &orientation);
      const QuatF quat(orientation);

      // Get the plane position.

      const Point3F position = plane.getPosition();

      // Turn everything into a "surface" property for the ConvexShape.

      char buffer[1024];
      dSprintf(buffer, sizeof(buffer), "%g %g %g %g %g %g %g %i %g %g %g %g %f %d %d",
         quat.x, quat.y, quat.z, quat.w,
         position.x, position.y, position.z,
         0, 0, 0, 1.0, 1.0, 0, true, false);

      // Add the surface.

      static StringTableEntry sSurface = StringTable->insert("surface");
      outBrush->setDataField(sSurface, NULL, buffer);
   }*/
}

//
//

GuiConvexEditorCtrl::GuiConvexEditorCtrl()
 : mIsDirty( false ),
   mFaceSEL( -1 ),
   mFaceHL( -1 ),
   mVertHL( -1 ),
   mFaceSavedXfm( true ),
   mSavedUndo( false ),
   mDragging( false ),
   mGizmoMatOffset( Point3F::Zero ),
   mPivotPos( Point3F::Zero ),
   mUsingPivot( false ),
   mSettingPivot( false ),
   mActiveTool( NULL ),
   mMouseDown( false ),
   mCreateTool( NULL ),
   mSavedGizmoFlags( -1 ),
   mHasCopied( false ),
   mLastUndo( NULL ),
   mUndoManager( NULL ),
   mCtrlDown( false ),
   mGridSnap(false),
   mSelectionMode(VertMode)
{   
	mMaterialName = StringTable->insert("Grid512_OrangeLines_Mat");
}

GuiConvexEditorCtrl::~GuiConvexEditorCtrl()
{
}

bool GuiConvexEditorCtrl::onAdd()
{
   if ( !Parent::onAdd() )
      return false;   

   SceneManager::getPreRenderSignal().notify( this, &GuiConvexEditorCtrl::_prepRenderImage );

   mCreateTool = new ConvexEditorCreateTool( this );

   return true;
}

void GuiConvexEditorCtrl::onRemove()
{
   SceneManager::getPreRenderSignal().remove( this, &GuiConvexEditorCtrl::_prepRenderImage );

   SAFE_DELETE( mCreateTool );

   Parent::onRemove();
}

void GuiConvexEditorCtrl::initPersistFields()
{   
   addField( "isDirty", TypeBool, Offset( mIsDirty, GuiConvexEditorCtrl ) );
	addField( "materialName", TypeString, Offset(mMaterialName, GuiConvexEditorCtrl) );

   Parent::initPersistFields();
}

bool GuiConvexEditorCtrl::onWake()
{
   if ( !Parent::onWake() )
      return false;

   SimGroup *missionGroup;
   if ( !Sim::findObject( "MissionGroup", missionGroup ) )
      return true;

   SimGroup::iterator itr = missionGroup->begin();
   for ( ; itr != missionGroup->end(); itr++ )
   {
      if ( dStrcmp( (*itr)->getClassName(), "ConvexShape" ) == 0 )
      {
         mConvexSEL = static_cast<ConvexShape*>( *itr );
         mGizmo->set( mConvexSEL->getTransform(), mConvexSEL->getPosition(), mConvexSEL->getScale() );
         return true;
      }
   }

   return true;   
}

void GuiConvexEditorCtrl::onSleep()
{
   Parent::onSleep();

   mConvexSEL = NULL;
   mConvexHL = NULL;   
}

void GuiConvexEditorCtrl::setVisible( bool val )
{
   //ConvexShape::smRenderEdges = value;

   if ( isProperlyAdded() )
   {
      if ( !val )
      {         
         mFaceHL = -1;
         mConvexHL = NULL;			

         setSelection( NULL, -1 );

         if ( mSavedGizmoFlags != -1 )
         {
            mGizmoProfile->flags = mSavedGizmoFlags;
            mSavedGizmoFlags = -1;
         }
         
         //Make our proxy objects "real"
         for (U32 i = 0; i < mProxyObjects.size(); ++i)
         {
            if (!mProxyObjects[i].shapeProxy || !mProxyObjects[i].targetObject)
               continue;

            AbstractClassRep* classRep = AbstractClassRep::findClassRep(mProxyObjects[i].targetObjectClass);
            if (!classRep)
            {
               Con::errorf("WorldEditor::createPolyhedralObject - No such class: %s", mProxyObjects[i].targetObjectClass);
               continue;
            }

            MatrixF savedTransform = mProxyObjects[i].shapeProxy->getTransform();
            Point3F savedScale = mProxyObjects[i].shapeProxy->getScale();

            Polyhedron polyhedron;
            convertToPolyhedron(mProxyObjects[i].shapeProxy, &polyhedron);

            // Create the object.
            SceneObject* object = dynamic_cast< SceneObject* >(classRep->create());
            if (!object)
            {
               Con::errorf("WorldEditor::createPolyhedralObject - Could not create SceneObject with class '%s'", "Zone");
               continue;
            }

            // Add the vertex data.
            const U32 numPoints = polyhedron.getNumPoints();
            const Point3F* points = polyhedron.getPoints();

            for (U32 p = 0; p < numPoints; ++p)
            {
               static StringTableEntry sPoint = StringTable->insert("point");
               object->setDataField(sPoint, NULL, EngineMarshallData(points[p]));
            }

            // Add the plane data.
            const U32 numPlanes = polyhedron.getNumPlanes();
            const PlaneF* planes = polyhedron.getPlanes();

            for (U32 p = 0; p < numPlanes; ++p)
            {
               static StringTableEntry sPlane = StringTable->insert("plane");
               const PlaneF& plane = planes[p];

               char buffer[1024];
               dSprintf(buffer, sizeof(buffer), "%g %g %g %g", plane.x, plane.y, plane.z, plane.d);

               object->setDataField(sPlane, NULL, buffer);
            }

            // Add the edge data.
            const U32 numEdges = polyhedron.getNumEdges();
            const Polyhedron::Edge* edges = polyhedron.getEdges();

            for (U32 e = 0; e < numEdges; ++e)
            {
               static StringTableEntry sEdge = StringTable->insert("edge");
               const Polyhedron::Edge& edge = edges[e];

               char buffer[1024];
               dSprintf(buffer, sizeof(buffer), "%i %i %i %i ",
                  edge.face[0], edge.face[1],
                  edge.vertex[0], edge.vertex[1]
               );

               object->setDataField(sEdge, NULL, buffer);
            }

            // Set the transform.
            object->setTransform(savedTransform);
            object->setScale(savedScale);

            // Register and return the object.

            if (!object->registerObject())
            {
               Con::errorf("WorldEditor::createPolyhedralObject - Failed to register object!");
               delete object;
               continue;
            }

            SimGroup* misGroup;
            if (!Sim::findObject("MissionGroup", misGroup))
            {
               return;
            }

            misGroup->addObject(object);

            //Now, remove the convex proxy
            mProxyObjects[i].shapeProxy->deleteObject();
            mProxyObjects[i].targetObject->deleteObject();
            mProxyObjects.erase(i);
            --i;
         }
      }
      else
      {
			mConvexHL = NULL;			
			mFaceHL = -1;

         setSelection( NULL, -1 );

			WorldEditor *wedit;
			if ( Sim::findObject( "EWorldEditor", wedit ) )
			{
				S32 count = wedit->getSelectionSize();
				for ( S32 i = 0; i < count; i++ )
				{
					S32 objId = wedit->getSelectObject(i);
					ConvexShape *pShape;
					if ( Sim::findObject( objId, pShape ) )
					{
						mConvexSEL = pShape;						
						wedit->clearSelection();
						wedit->selectObject( String::ToString("%i",objId) );
						break;
					}
				}
			}
         updateGizmoPos();
         mSavedGizmoFlags = mGizmoProfile->flags;

         //
         //Iterate through our mission group and find all valid objects we can proxy
         mProxyObjects;
         SimGroup* misGroup;
         if (!Sim::findObject("MissionGroup", misGroup))
         {
            return;
         }
         
         for (U32 c = 0; c < misGroup->size(); ++c)
         {
            if (Trigger* triggerObj = dynamic_cast<Trigger*>(misGroup->at(c)))
            {
            }
            else if (Portal* triggerObj = dynamic_cast<Portal*>(misGroup->at(c)))
            {
               if (!triggerObj)
               {
                  Con::errorf("WorldEditor::createConvexShapeFrom - Invalid object");
                  continue;
               }

               IScenePolyhedralObject* iPoly = dynamic_cast< IScenePolyhedralObject* >(triggerObj);
               if (!iPoly)
               {
                  Con::errorf("WorldEditor::createConvexShapeFrom - Not a polyhedral object!");
                  continue;
               }

               // Get polyhedron.
               AnyPolyhedron polyhedron = iPoly->ToAnyPolyhedron();
               ConvexShape* shape = new ConvexShape();

               convertFromPolyhedron(&polyhedron, shape);

               // Copy the transform.
               shape->setTransform(triggerObj->getTransform());
               shape->setScale(triggerObj->getScale());

               // Register the shape.
               if (!shape->registerObject())
               {
                  Con::errorf("WorldEditor::createConvexShapeFrom - Could not register ConvexShape!");
                  delete shape;
                  continue;
               }

               //Set the texture to a representatory one so we know what's what
               shape->mMaterialName = "PortalProxyMaterial";
               shape->_updateMaterial();

               //set up the proxy object
               ConvexShapeProxy newProxy;
               newProxy.shapeProxy = shape;
               newProxy.targetObject = triggerObj;
               newProxy.targetObjectClass = "Portal";

               mProxyObjects.push_back(newProxy);
            }
            else if (Zone* triggerObj = dynamic_cast<Zone*>(misGroup->at(c)))
            {
               if (!triggerObj)
               {
                  Con::errorf("WorldEditor::createConvexShapeFrom - Invalid object");
                  continue;
               }

               IScenePolyhedralObject* iPoly = dynamic_cast< IScenePolyhedralObject* >(triggerObj);
               if (!iPoly)
               {
                  Con::errorf("WorldEditor::createConvexShapeFrom - Not a polyhedral object!");
                  continue;
               }

               // Get polyhedron.
               AnyPolyhedron polyhedron = iPoly->ToAnyPolyhedron();
               ConvexShape* shape = new ConvexShape();

               convertFromPolyhedron(&polyhedron, shape);

               // Copy the transform.
               shape->setTransform(triggerObj->getTransform());
               shape->setScale(triggerObj->getScale());

               // Register the shape.

               if (!shape->registerObject())
               {
                  Con::errorf("WorldEditor::createConvexShapeFrom - Could not register ConvexShape!");
                  delete shape;
                  continue;
               }

               //Set the texture to a representatory one so we know what's what
               shape->mMaterialName = "ZoneProxyMaterial";
               shape->_updateMaterial();

               //set up the proxy object
               ConvexShapeProxy newProxy;
               newProxy.shapeProxy = shape;
               newProxy.targetObject = triggerObj;
               newProxy.targetObjectClass = "Zone";

               mProxyObjects.push_back(newProxy);
            }
            else if (ReflectionProbe* triggerObj = dynamic_cast<ReflectionProbe*>(misGroup->at(c)))
            {
            }
         }
      }
   }

   Parent::setVisible( val );
}

void GuiConvexEditorCtrl::on3DMouseDown(const Gui3DMouseEvent & event)
{
   mouseLock();   

   mMouseDown = true;

   if ( event.modifier & SI_ALT )
   {
      setActiveTool( mCreateTool );
      mActiveTool->on3DMouseDown( event );
      return;
   }

   if ( mConvexSEL && isShapeValid( mConvexSEL ) )      
      mLastValidShape = mConvexSEL->mSurfaces;  

   if ( mConvexSEL &&
        mFaceSEL != -1 &&
        mGizmo->getMode() == RotateMode &&
        mGizmo->getSelection() == Gizmo::Centroid )
   {      
      mSettingPivot = true;      
      mSavedPivotPos = mGizmo->getPosition();
      setPivotPos( mConvexSEL, mFaceSEL, event );
      updateGizmoPos();
      return;
   }

   mGizmo->on3DMouseDown( event );   
}

void GuiConvexEditorCtrl::on3DRightMouseDown(const Gui3DMouseEvent & event)
{
   return;

	/*
   if ( mConvexSEL && mFaceSEL != -1 && mFaceSEL == mFaceHL )
   {
      _submitUndo( "Split ConvexShape face." );

      const MatrixF &surf = mConvexSEL->mSurfaces[mFaceSEL];

      MatrixF newSurf( surf );

      MatrixF rotMat( EulerF( 0.0f, mDegToRad( 2.0f ), 0.0f ) );

      newSurf *= rotMat;           

      mConvexSEL->mSurfaces.insert( mFaceSEL+1, newSurf );
   }
	*/
}

void GuiConvexEditorCtrl::on3DRightMouseUp(const Gui3DMouseEvent & event)
{
   //ConvexShape *hitShape;
   //S32 hitFace;
   //bool hit = _cursorCast( event, &hitShape, &hitFace );
   //Con::printf( hit ? "HIT" : "MISS" );
}

void GuiConvexEditorCtrl::on3DMouseUp(const Gui3DMouseEvent & event)
{
   mouseUnlock();

   mMouseDown = false;

   mHasCopied = false;
   mHasGeometry = false;   

   if ( mActiveTool )
   {
      ConvexEditorTool::EventResult result = mActiveTool->on3DMouseUp( event );

      if ( result == ConvexEditorTool::Done )      
         setActiveTool( NULL );         
      
      return;
   }

   if ( !mSettingPivot && !mDragging && ( mGizmo->getSelection() == Gizmo::None || !mConvexSEL ) )
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
   mSavedPivotPos = mGizmo->getPosition();
   mSavedUndo = false;   

   mGizmo->on3DMouseUp( event );

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
}

void GuiConvexEditorCtrl::on3DMouseMove(const Gui3DMouseEvent & event)
{
   if ( mActiveTool )
   {
      // If we have an active tool pass this event to it.
      // If it handled it, consume the event.
      if ( mActiveTool->on3DMouseMove( event ) )
         return;
   }

   ConvexShape *hitShape = NULL;
   S32 hitFace = -1;
   
   _cursorCast( event, &hitShape, &hitFace );

   if (hitShape && mSelectionMode == VertMode)
   {
      S32 hoverNodeIdx = -1;
      F32 hoverNodeDist = F32_MAX;
      Point2I bestVertPos;

      Point2I mNodeHalfSize = Point2I(4, 4);

      for (U32 v = 0; v < hitShape->mGeometry.points.size(); ++v)
      {
         const Point3F &vertPos = hitShape->mGeometry.points[v];

         Point3F screenPos;
         project(vertPos, &screenPos);

         RectI rect(Point2I((S32)screenPos.x, (S32)screenPos.y) - mNodeHalfSize, mNodeHalfSize * 2);

         if (rect.pointInRect(event.mousePoint) && screenPos.z < hoverNodeDist)
         {
            hoverNodeDist = screenPos.z;
            hoverNodeIdx = v;
            bestVertPos = Point2I(screenPos.x, screenPos.y);
         }
      }

      mVertHL = hoverNodeIdx;
      hlvertpos = bestVertPos;
   }

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
      mGizmo->on3DMouseMove( event );
}

void GuiConvexEditorCtrl::on3DMouseDragged(const Gui3DMouseEvent & event)
{      
   if ( mActiveTool )
   {
      // If we have an active tool pass this event to it.
      // If it handled it, consume the event.
      if ( mActiveTool->on3DMouseDragged( event ) )
         return;
   }

   //mGizmoProfile->rotateScalar = 0.55f;
   //mGizmoProfile->scaleScalar = 0.55f;

   if ( !mConvexSEL )
      return;

   if ( mGizmo->getMode() == RotateMode &&
        mGizmo->getSelection() == Gizmo::Centroid )
   {            
      setPivotPos( mConvexSEL, mFaceSEL, event );      
      mDragging = true;
      return;
   }

   mGizmo->on3DMouseDragged( event );
      
   if ( event.modifier & SI_SHIFT && 
       ( mGizmo->getMode() == MoveMode || mGizmo->getMode() == RotateMode ) &&
        !mHasCopied )
   {
      if ( mFaceSEL != -1 )
      {
         ConvexShape *newShape = mCreateTool->extrudeShapeFromFace( mConvexSEL, mFaceSEL );
         //newShape->_updateGeometry();

         submitUndo( CreateShape, newShape );
         setSelection( newShape, 0 );         
         updateGizmoPos();

         mGizmo->on3DMouseDown( event );

         mHasCopied = true;
         mSavedUndo = true;
      }
      else
      {
         ConvexShape *newShape = new ConvexShape();
         newShape->setTransform( mConvexSEL->getTransform() );
         newShape->setScale( mConvexSEL->getScale() );
         newShape->mSurfaces.clear();
         newShape->mSurfaces.merge( mConvexSEL->mSurfaces );
         
         setupShape( newShape );

         newShape->setField("material", mConvexSEL->getMaterialName());

         submitUndo( CreateShape, newShape );

         setSelection( newShape, -1 );

         updateGizmoPos();

         mHasCopied = true;
         mSavedUndo = true;
      }

      return;
   }

   if ( mGizmo->getMode() == RotateMode &&
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
         return;

      mLastValidShape = mConvexSEL->mSurfaces;

      Point3F rot = mGizmo->getDeltaTotalRot();
      rot.normalize();
      rot *= mDegToRad( 10.0f );

      MatrixF rotMat( (EulerF)rot );

      MatrixF worldToObj( mConvexSEL->getTransform() );
      worldToObj.scale( mConvexSEL->getScale() );
      worldToObj.inverse();      

      mConvexSEL->mSurfaces.increment();
      MatrixF &newSurf = mConvexSEL->mSurfaces.last();
      newSurf = mConvexSEL->mSurfaces[mFaceSEL] * rotMat;
      
      //worldToObj.mul( mGizmo->getTransform() );
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
      
      return;
   }

   // If we are dragging, but no gizmo selection...
   // Then treat this like a regular mouse move, update the highlighted
   // convex/face under the cursor and handle onMouseUp as we normally would
   // to change the selection.
   if ( mGizmo->getSelection() == Gizmo::None )
   {
      ConvexShape *hitShape = NULL;
      S32 hitFace = -1;

      _cursorCast( event, &hitShape, &hitFace );
      mFaceHL = hitFace;
      mConvexHL = hitShape;      

      return;
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

      if ( mGizmo->getMode() == ScaleMode &&  !(event.modifier & SI_CTRL))
      {
         scaleFace( mConvexSEL, mFaceSEL, mGizmo->getScale() );
      }

	  else if ( mGizmo->getMode() == ScaleMode &&  (event.modifier & SI_CTRL) )
      {
          Point3F scale = mGizmo->getDeltaScale();

	     F32 scalar = 1;
		  mConvexSEL->mSurfaceUVs[mFaceSEL].scale += (Point2F(scale.x, scale.y) * scalar);

        if (mConvexSEL->mSurfaceUVs[mFaceSEL].scale.x < 0.01)
           mConvexSEL->mSurfaceUVs[mFaceSEL].scale.x = 0.01;

        if (mConvexSEL->mSurfaceUVs[mFaceSEL].scale.y < 0.01)
           mConvexSEL->mSurfaceUVs[mFaceSEL].scale.y = 0.01;

        if (mConvexSEL->mSurfaceUVs[mFaceSEL].scale.x > 100)
           mConvexSEL->mSurfaceUVs[mFaceSEL].scale.x = 100;

        if (mConvexSEL->mSurfaceUVs[mFaceSEL].scale.y > 100)
           mConvexSEL->mSurfaceUVs[mFaceSEL].scale.y = 100;

        Point2F test = mConvexSEL->mSurfaceUVs[mFaceSEL].scale;
		  mConvexSEL->setMaskBits( ConvexShape::UpdateMask );

		  updateShape( mConvexSEL, mFaceSEL );
     }
	  /*else if ( mGizmo->getMode() == MoveMode && event.modifier & SI_CTRL ) {
		  Point3F scale = mGizmo->getOffset();

          F32 scalar = 0.8;
		  mConvexSEL->mSurfaceTextures[mFaceSEL].offset += (Point2F(-scale.x, scale.z) * scalar);
		  mConvexSEL->setMaskBits( ConvexShape::UpdateMask );

		  updateShape( mConvexSEL, mFaceSEL );
	  }*/
      else
      {
         // Why does this have to be so ugly.
         if ( mGizmo->getMode() == RotateMode || 
              ( mGizmo->getMode() == MoveMode  && 
                ( event.modifier & SI_CTRL  ||
                  ( mGizmo->getSelection() == Gizmo::Axis_Z && mHasCopied ) 
                )
              )
            )
         {
            const MatrixF &gMat = mGizmo->getTransform();      
            MatrixF surfMat;
            surfMat.mul( mConvexSEL->mWorldToObj, gMat );

            MatrixF worldToObj ( mConvexSEL->getTransform() );
            worldToObj.scale( mConvexSEL->getScale() );
            worldToObj.inverse();

            Point3F newPos;            
            newPos = gMat.getPosition();      

            worldToObj.mulP( newPos );
            surfMat.setPosition( newPos );
            
            // Clear out floating point errors.
            cleanMatrix( surfMat );

            if (mGizmo->getSelection() == Gizmo::Axis_Z)
            {
               MatrixF curSurfMat = mConvexSEL->mSurfaces[mFaceSEL];
               EulerF curSufRot = curSurfMat.toEuler();

               EulerF newSufRot = surfMat.toEuler();

               float zRot = mRadToDeg(newSufRot.z - curSufRot.z);

               float curZRot = mConvexSEL->mSurfaceUVs[mFaceSEL].zRot;

               mConvexSEL->mSurfaceUVs[mFaceSEL].zRot += zRot;
            }

            mConvexSEL->mSurfaces[mFaceSEL] = surfMat;

            updateShape( mConvexSEL, mFaceSEL );         
         }
         else
         {
            // Translating a face in x/y/z

            translateFace( mConvexSEL, mFaceSEL, mGizmo->getTotalOffset() );
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

      return;
   }

   // Manipulating a whole Convex.

   if ( !mSavedUndo )
   {
      mSavedUndo = true;
      submitUndo( ModifyShape, mConvexSEL );
   }

   if ( mGizmo->getMode() == MoveMode )
   {
      //mConvexSEL->setPosition( mGizmo->getPosition() );

      //MatrixF mat = mGizmo->getTransform();
      Point3F wPos = mGizmo->getPosition();
      //mat.getColumn(3, &wPos);

      // adjust
      //wPos += offset;

      if (mGridSnap && mGridPlaneSize != 0.f)
      {
         wPos.x -= mFmod(wPos.x, mGridPlaneSize);
         wPos.y -= mFmod(wPos.y, mGridPlaneSize);
         wPos.z -= mFmod(wPos.z, mGridPlaneSize);
      }

      mConvexSEL->setPosition(wPos);
   }
   else if ( mGizmo->getMode() == RotateMode )
   {   
      mConvexSEL->setTransform( mGizmo->getTransform() );      
   }
   else
   {
      mConvexSEL->setScale( mGizmo->getScale() );
   }   

   if ( mConvexSEL->getClientObject() )
   {
      ConvexShape *clientObj = static_cast< ConvexShape* >( mConvexSEL->getClientObject() );
      clientObj->setTransform( mConvexSEL->getTransform() );
      clientObj->setScale( mConvexSEL->getScale() );
   }      
}

void GuiConvexEditorCtrl::on3DMouseEnter(const Gui3DMouseEvent & event)
{

}

void GuiConvexEditorCtrl::on3DMouseLeave(const Gui3DMouseEvent & event)
{

}

bool GuiConvexEditorCtrl::onKeyDown( const GuiEvent &evt )
{
   bool handled = false;

   switch ( evt.keyCode )
   {
   case KEY_ESCAPE:
      handled = handleEscape();      
      break;   
   case KEY_A:
      if ( evt.modifier & SI_ALT )
      {
         GizmoAlignment align = mGizmo->getProfile()->alignment;
         if ( align == World )
            mGizmo->getProfile()->alignment = Object;
         else
            mGizmo->getProfile()->alignment = World;
         handled = true;
      }
      break;
   case KEY_LCONTROL:
      //mCtrlDown = true;
      break;  
   default:
      break;
   }
   
   return handled;
}

bool GuiConvexEditorCtrl::onKeyUp( const GuiEvent &evt )
{
   bool handled = false;

   switch ( evt.keyCode )
   {      
   case KEY_LCONTROL:
      //mCtrlDown = false;
      break;   
   default:
      break;
   }

   return handled;
}

void GuiConvexEditorCtrl::get3DCursor( GuiCursor *&cursor, 
                                       bool &visible, 
                                       const Gui3DMouseEvent &event_ )
{
   //cursor = mAddNodeCursor;
   //visible = false;

   cursor = NULL;
   visible = false;

   GuiCanvas *root = getRoot();
   if ( !root )
      return;

   S32 currCursor = PlatformCursorController::curArrow;

   if ( root->mCursorChanged == currCursor )
      return;

   PlatformWindow *window = root->getPlatformWindow();
   PlatformCursorController *controller = window->getCursorController();

   // We've already changed the cursor, 
   // so set it back before we change it again.
   if( root->mCursorChanged != -1)
      controller->popCursor();

   // Now change the cursor shape
   controller->pushCursor(currCursor);
   root->mCursorChanged = currCursor;   
}

void GuiConvexEditorCtrl::updateGizmo()
{
   mGizmoProfile->restoreDefaultState();
   
   const GizmoMode &mode = mGizmoProfile->mode;
   S32 &flags = mGizmoProfile->flags;
   GizmoAlignment &align = mGizmoProfile->alignment;

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
         //hijacking the CTRL modifier for texture offsetting control
         //flags &= ~( GizmoProfile::CanTranslateX | GizmoProfile::CanTranslateY | GizmoProfile::PlanarHandlesOn ); 
         flags &= ~GizmoProfile::CanScaleZ;
      else      
         flags |= ( GizmoProfile::CanTranslateX | GizmoProfile::CanTranslateY | GizmoProfile::PlanarHandlesOn );      
   }
}

void GuiConvexEditorCtrl::renderScene(const RectI & updateRect)
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

   if ( mActiveTool )
      mActiveTool->renderScene( updateRect );

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

         Box3F objBox = mConvexSEL->getObjBox();
         objBox.scale( mConvexSEL->getScale() );

         const MatrixF &objMat = mConvexSEL->getTransform();

         Point3F boxPos = objBox.getCenter();
         objMat.mulP( boxPos );
         
         drawer->drawObjectBox( desc, objBox.getExtents(), boxPos, objMat, ColorI::WHITE );
      }
      else
      {
         mConvexSEL->renderFaceEdges( -1, colorNA );     

         drawFacePlane( mConvexSEL, mFaceSEL );         
      }

      if ( mConvexHL == mConvexSEL &&
           mFaceHL != -1 && 
           mFaceHL != mFaceSEL && 
           mGizmo->getSelection() == Gizmo::None )
      {
         mConvexSEL->renderFaceEdges( mFaceHL, colorHL );
      }
   }

   if ( mConvexHL && mConvexHL != mConvexSEL )
   {
      mConvexHL->renderFaceEdges( -1 );      
   }

   if ( mGizmo->getMode() != RotateMode && mUsingPivot )
   {
      mUsingPivot = false;
      updateGizmoPos();
   }

   F32 gizmoAlpha = 1.0f;
	if ( !mConvexSEL )
		gizmoAlpha = 0.0f;

   if ( mMouseDown && mGizmo->getSelection() != Gizmo::None && mConvexSEL )
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

         MatrixF objToWorld( mConvexSEL->getTransform() );
         objToWorld.scale( mConvexSEL->getScale() );      

         for ( S32 i = 0; i < lineList.size(); i++ )     
            objToWorld.mulP( lineList[i] );			

         for ( S32 i = 0; i < lineList.size() - 1; i++ )
         {
			   Point3F p0( lineList[i] );
			   Point3F p1( lineList[i+1] );

			   drawLine( p0, p1, colorSEL, 3.0f );
         }
	   }

      if ( gizmoAlpha == 1.0f )
      {
         if ( mGizmoProfile->mode != NoneMode )
            mGizmo->renderText( mSaveViewport, mSaveModelview, mSaveProjection );   	
      }

      if ( mActiveTool )
         mActiveTool->render2D();
   }

   if ( gizmoAlpha == 1.0f )   
      mGizmo->renderGizmo( mLastCameraQuery.cameraMatrix, mLastCameraQuery.fov );

   if (mVertHL != -1)
   {
      Point2I nodeHalfSize = Point2I(4, 4);
      drawer->drawRectFill(hlvertpos - nodeHalfSize, hlvertpos + nodeHalfSize, ColorI::RED);
   }
} 

void GuiConvexEditorCtrl::drawFacePlane( ConvexShape *shape, S32 faceId )
{
   // Build a vb of the face points ( in world space ) scaled outward in
   // the surface space in x/y with uv coords.

   /*
   Vector< Point3F > points;
   Vector< Point2F > coords;

   shape->getSurfaceTriangles( faceId, &points, &coords, false );

   if ( points.empty() )
      return;

   GFXVertexBufferHandle< GFXVertexPCT > vb;
   vb.set( GFX, points.size(), GFXBufferTypeVolatile );
   GFXVertexPCT *vert = vb.lock();

   for ( S32 i = 0; i < points.size(); i++ )
   {
      vert->point = points[i];
      vert->color.set( 255, 255, 255, 200 );
      vert->texCoord = coords[i];
      vert++;
   }

   vb.unlock();

   GFXTransformSaver saver;
   MatrixF renderMat( shape->getTransform() );
   renderMat.scale( shape->getScale() );
   GFX->multWorld( renderMat );

   GFXStateBlockDesc desc;
   desc.setBlend( true );
   desc.setCullMode( GFXCullNone );
   desc.setZReadWrite( true, false );
   desc.samplersDefined = true;
   desc.samplers[0] = GFXSamplerStateDesc::getWrapLinear();
   GFX->setStateBlockByDesc( desc );

   GFX->setVertexBuffer( vb );

   GFXTexHandle tex( "core/art/grids/512_transp", &GFXStaticTextureSRGBProfile, "ConvexEditor_grid" );
   GFX->setTexture( 0, tex );
   GFX->setupGenericShaders();
   GFX->drawPrimitive( GFXTriangleList, 0, points.size() / 3 );
   */
}


void GuiConvexEditorCtrl::scaleFace( ConvexShape *shape, S32 faceId, Point3F scale )
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
   
   ConvexShape::Face &face = shape->mGeometry.faces[faceId];

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

void GuiConvexEditorCtrl::translateFace( ConvexShape *shape, S32 faceId, const Point3F &displace )
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

   ConvexShape::Face &face = shape->mGeometry.faces[faceId];

   Vector< Point3F > &pointList = shape->mGeometry.points;

   AssertFatal( shape->mSurfaces[ face.id ].isAffine(), "ConvexShapeEditor - surface not affine." );

   Point3F modDisplace = Point3F(displace.x, displace.y, displace.z);

   //snapping
   if (mGridSnap && mGridPlaneSize != 0)
   {
      Point3F faceCenter = Point3F::Zero;

      for (S32 i = 0; i < face.points.size(); i++)
      {
         Point3F &pnt = pointList[face.points[i]];
         faceCenter += pnt;
      }

      faceCenter /= face.points.size();

      // Transform displacement into object space.    
      MatrixF objToWorld(shape->getWorldTransform());
      objToWorld.scale(shape->getScale());
      objToWorld.inverse();

      objToWorld.mulP(faceCenter);

      modDisplace = faceCenter + displace;
      Point3F fMod = Point3F::Zero;

      if (!mIsZero(displace.x))
         fMod.x = mFmod(modDisplace.x - (displace.x > 0 ? mGridPlaneSize : -mGridPlaneSize), mGridPlaneSize);

      if (!mIsZero(displace.y))
         fMod.y = mFmod(modDisplace.y - (displace.y > 0 ? mGridPlaneSize : -mGridPlaneSize), mGridPlaneSize);

      if (!mIsZero(displace.z))
         fMod.z = mFmod(modDisplace.z - (displace.z > 0 ? mGridPlaneSize : -mGridPlaneSize), mGridPlaneSize);

      modDisplace -= fMod;

      modDisplace -= faceCenter;
   }

   // Transform displacement into object space.    
   MatrixF worldToObj( shape->getTransform() );
   worldToObj.scale( shape->getScale() );
   worldToObj.inverse();

   Point3F displaceOS;
   worldToObj.mulV(modDisplace, &displaceOS);

   for ( S32 i = 0; i < face.points.size(); i++ )
   {                  
      Point3F &pnt = pointList[ face.points[i] ];   
      pnt += displaceOS;      
   }

   updateModifiedFace( shape, faceId );
}

void GuiConvexEditorCtrl::updateModifiedFace( ConvexShape *shape, S32 faceId )
{
   if ( shape->mGeometry.faces.size() <= faceId )
      return;

   ConvexShape::Face &face = shape->mGeometry.faces[faceId];

   Vector< Point3F > &pointList = shape->mGeometry.points;

   Vector< ConvexShape::Face > &faceList = shape->mGeometry.faces;

   for ( S32 i = 0; i < faceList.size(); i++ )
   {
      ConvexShape::Face &curFace = faceList[i];      
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

bool GuiConvexEditorCtrl::isShapeValid( ConvexShape *shape )
{
   // Test for no-geometry.
   if ( shape->mGeometry.points.empty() )
      return false;

   const Vector<Point3F> &pointList = shape->mGeometry.points;
   const Vector<ConvexShape::Face> &faceList = shape->mGeometry.faces;

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

void GuiConvexEditorCtrl::setupShape( ConvexShape *shape )
{
   shape->setField( "material", mMaterialName );
   shape->registerObject();
   updateShape( shape );

   SimGroup *group;
   if ( Sim::findObject( "missionGroup", group ) )
      group->addObject( shape );
}

void GuiConvexEditorCtrl::updateShape( ConvexShape *shape, S32 offsetFace )
{
   shape->_updateGeometry( true );

   /*
   if ( offsetFace != -1 )
   {
      shape->mSurfaces[ offsetFace ].setPosition( mPivotPos );
   }*/

   synchClientObject( shape );
}

void GuiConvexEditorCtrl::synchClientObject( const ConvexShape *serverConvex )
{
   if ( serverConvex->getClientObject() )
   {
      ConvexShape *clientConvex = static_cast< ConvexShape* >( serverConvex->getClientObject() );
      clientConvex->setScale( serverConvex->getScale() );
      clientConvex->setTransform( serverConvex->getTransform() );
      clientConvex->mSurfaces.clear();
      clientConvex->mSurfaces.merge( serverConvex->mSurfaces );
      clientConvex->_updateGeometry(true);
   }
}

void GuiConvexEditorCtrl::updateGizmoPos()
{
   if ( mConvexSEL )
   {
      if ( mFaceSEL != -1 )
      {
         MatrixF surfMat = mConvexSEL->getSurfaceWorldMat( mFaceSEL );  

         MatrixF objToWorld( mConvexSEL->getTransform() );
         objToWorld.scale( mConvexSEL->getScale() );

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

         mGizmo->set( surfMat, gizmoPos, Point3F::One );        
      }
      else
      {
         mGizmoMatOffset = Point3F::Zero;
         mGizmo->set( mConvexSEL->getTransform(), mConvexSEL->getPosition(), mConvexSEL->getScale() ); 
      }
   }   
}

bool GuiConvexEditorCtrl::setActiveTool( ConvexEditorTool *tool )
{   
   if ( mActiveTool == tool )
      return false;

   ConvexEditorTool *prevTool = mActiveTool;
   ConvexEditorTool *newTool = tool;

   if ( prevTool )
      prevTool->onDeactivated( newTool );

   mActiveTool = newTool;

   if ( newTool )
      newTool->onActivated( prevTool );

   return true;
}

bool GuiConvexEditorCtrl::handleEscape()
{
   if ( mActiveTool )
   {
      mActiveTool->onDeactivated( NULL );
      mActiveTool = NULL;

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

bool GuiConvexEditorCtrl::handleDelete()
{
   if ( mActiveTool )
   {
      mActiveTool->onDeactivated( NULL );
      mActiveTool = NULL;
   }

   if ( mConvexSEL )
   {
      if ( mFaceSEL != -1 )
      {
         submitUndo( ModifyShape, mConvexSEL );

         mConvexSEL->mSurfaces.erase_fast( mFaceSEL );
         updateShape( mConvexSEL );

         if ( !isShapeValid( mConvexSEL ) )
         {
            S32 selFace = mFaceSEL;
            mLastUndo->undo();
            mFaceSEL = selFace;
            updateShape( mConvexSEL );
            updateGizmoPos();
         }
         else
         {
            setSelection( mConvexSEL, -1 );
         }
      }
      else
      {
         // Grab the mission editor undo manager.
         UndoManager *undoMan = NULL;
         if ( !Sim::findObject( "EUndoManager", undoMan ) )
         {            
            Con::errorf( "GuiConvexEditorCtrl::on3DMouseDown() - EUndoManager not found!" );    
         }
         else
         {
            // Create the UndoAction.
            MEDeleteUndoAction *action = new MEDeleteUndoAction("Deleted ConvexShape");
            action->deleteObject( mConvexSEL );
            mIsDirty = true;
            
            mFaceHL = -1; 

            setSelection( NULL, -1 );

            // Submit it.               
            undoMan->addAction( action );
         }
      }
   }

   return true;
}

bool GuiConvexEditorCtrl::hasSelection() const
{
   return mConvexSEL != NULL;   
}

void GuiConvexEditorCtrl::clearSelection()
{
   mFaceHL = -1;
   mConvexHL = NULL;
   setSelection( NULL, -1 );
}

void GuiConvexEditorCtrl::handleDeselect()
{
   if ( mActiveTool )
   {
      mActiveTool->onDeactivated( NULL );
      mActiveTool = NULL;
   }

   mFaceHL = -1;
   mConvexHL = NULL;
   setSelection( NULL, -1 );
}

void GuiConvexEditorCtrl::setSelection( ConvexShape *shape, S32 faceId )
{
   mFaceSEL = faceId;
   mConvexSEL = shape;
   updateGizmoPos();

   Con::executef( this, "onSelectionChanged", shape ? shape->getIdString() : "", Con::getIntArg(faceId) );
}

void GuiConvexEditorCtrl::_prepRenderImage( SceneManager* sceneGraph, const SceneRenderState* state )
{   
   if ( !isAwake() )
      return;

   /*
   ObjectRenderInst *ri = state->getRenderPass()->allocInst<ObjectRenderInst>();
   ri->type = RenderPassManager::RIT_Editor;
   ri->renderDelegate.bind( this, &GuiConvexEditorCtrl::_renderObject );
   ri->defaultKey = 100;
   state->getRenderPass()->addInst( ri );   
   */
}

void GuiConvexEditorCtrl::_renderObject( ObjectRenderInst *ri, SceneRenderState *state, BaseMatInstance *matInst )
{  
}

void GuiConvexEditorCtrl::submitUndo( UndoType type, ConvexShape *shape )
{
   Vector< ConvexShape* > shapes;
   shapes.push_back( shape );
   submitUndo( type, shapes );
}

void GuiConvexEditorCtrl::submitUndo( UndoType type, const Vector<ConvexShape*> &shapes )
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
      GuiConvexEditorUndoAction *action = new GuiConvexEditorUndoAction( "Modified a ConvexShape" );

      ConvexShape *shape = shapes.first();

      action->mObjId = shape->getId();   
      action->mEditor = this;   
      action->mSavedObjToWorld = shape->getTransform();
      action->mSavedScale = shape->getScale();
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

bool GuiConvexEditorCtrl::_cursorCastCallback( RayInfo* ri )
{
   // Reject anything that's not a ConvexShape.
   return dynamic_cast< ConvexShape* >( ri->object );
}

bool GuiConvexEditorCtrl::_cursorCast( const Gui3DMouseEvent &event, ConvexShape **hitShape, S32 *hitFace )
{
   RayInfo ri;
   
   if ( gServerContainer.castRay( event.pos, event.pos + event.vec * 10000.0f, StaticShapeObjectType, &ri, &GuiConvexEditorCtrl::_cursorCastCallback ) &&
        dynamic_cast< ConvexShape* >( ri.object ) )
   {
      // Do not select or edit ConvexShapes that are within a Prefab.
      if ( Prefab::getPrefabByChild( ri.object ) )
         return false;

      *hitShape = static_cast< ConvexShape* >( ri.object );
      *hitFace = ri.face;
      mLastRayInfo = ri;

      return true;
   }

   return false;
}

void GuiConvexEditorCtrl::setPivotPos( ConvexShape *shape, S32 faceId, const Gui3DMouseEvent &event )
{
   PlaneF plane;
   mTransformPlane( shape->getTransform(), shape->getScale(), shape->mPlanes[ faceId ], &plane );

   Point3F start( event.pos );
   Point3F end( start + event.vec * 10000.0f );

   F32 t = plane.intersect( start, end );

   if ( t >= 0.0f && t <= 1.0f )
   {
      Point3F hitPos;
      hitPos.interpolate( start, end, t );

      mPivotPos = hitPos;
      mUsingPivot = true;

      MatrixF worldToObj( shape->getTransform() );
      worldToObj.scale( shape->getScale() );
      worldToObj.inverse();

      Point3F objPivotPos( mPivotPos );
      worldToObj.mulP( objPivotPos );

      updateGizmoPos();
   }
}

void GuiConvexEditorCtrl::cleanMatrix( MatrixF &mat )
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

S32 GuiConvexEditorCtrl::getEdgeByPoints( ConvexShape *shape, S32 faceId, S32 p0, S32 p1 )
{
   const ConvexShape::Face &face = shape->mGeometry.faces[faceId];

   for ( S32 i = 0; i < face.edges.size(); i++ )
   {
      const ConvexShape::Edge &edge = face.edges[i];

      if ( edge.p0 != p0 && edge.p0 != p1 )
         continue;
      if ( edge.p1 != p0 && edge.p1 != p1 )
         continue;

      return i;      
   }

   return -1;
}

bool GuiConvexEditorCtrl::getEdgesTouchingPoint( ConvexShape *shape, S32 faceId, S32 pId, Vector< U32 > &edgeIdxList, S32 excludeEdge )
{
   const ConvexShape::Face &face = shape->mGeometry.faces[faceId];   
   const Vector< ConvexShape::Edge > &edgeList = face.edges;

   for ( S32 i = 0; i < edgeList.size(); i++ )
   {
      if ( i == excludeEdge )
         continue;

      const ConvexShape::Edge &curEdge = edgeList[i];

      if ( curEdge.p0 == pId || curEdge.p1 == pId )      
         edgeIdxList.push_back(i);
   }

   return !edgeIdxList.empty();
}

Point2F GuiConvexEditorCtrl::getSelectedFaceUVOffset()
{
   if (mFaceSEL == -1 || mConvexSEL == NULL)
      return Point2F(0, 0);

   return mConvexSEL->mSurfaceUVs[mFaceSEL].offset;
}

Point2F GuiConvexEditorCtrl::getSelectedFaceUVScale()
{
   if (mFaceSEL == -1 || mConvexSEL == NULL)
      return Point2F(0, 0);

   return mConvexSEL->mSurfaceUVs[mFaceSEL].scale;
}

const char* GuiConvexEditorCtrl::getSelectedFaceMaterial()
{
   if (mFaceSEL == -1 || mConvexSEL == NULL)
      return "";

   if (mConvexSEL->mSurfaceUVs[mFaceSEL].matID == 0)
   {
      return mConvexSEL->mMaterialName;
   }
   else
   {
      return mConvexSEL->mSurfaceTextures[mConvexSEL->mSurfaceUVs[mFaceSEL].matID - 1].materialName;
   }
}

bool GuiConvexEditorCtrl::getSelectedFaceHorzFlip()
{
   if (mFaceSEL == -1 || mConvexSEL == NULL)
      return false;

   return mConvexSEL->mSurfaceUVs[mFaceSEL].horzFlip;
}

bool GuiConvexEditorCtrl::getSelectedFaceVertFlip()
{
   if (mFaceSEL == -1 || mConvexSEL == NULL)
      return false;

   return mConvexSEL->mSurfaceUVs[mFaceSEL].vertFlip;
}

float GuiConvexEditorCtrl::getSelectedFaceZRot()
{
   if (mFaceSEL == -1 || mConvexSEL == NULL)
      return false;

   return mConvexSEL->mSurfaceUVs[mFaceSEL].zRot;
}

void GuiConvexEditorCtrl::setSelectedFaceUVOffset(Point2F offset)
{
   if (mFaceSEL == -1 || mConvexSEL == NULL)
      return;

   mConvexSEL->mSurfaceUVs[mFaceSEL].offset = offset;

   mConvexSEL->setMaskBits(ConvexShape::UpdateMask);
}

void GuiConvexEditorCtrl::setSelectedFaceUVScale(Point2F scale)
{
   if (mFaceSEL == -1 || mConvexSEL == NULL)
      return;

   mConvexSEL->mSurfaceUVs[mFaceSEL].scale = scale;

   mConvexSEL->setMaskBits(ConvexShape::UpdateMask);
}

void GuiConvexEditorCtrl::setSelectedFaceMaterial(const char* materialName)
{
   if (mFaceSEL == -1 || mConvexSEL == NULL)
      return;

   if (mConvexSEL->mSurfaceUVs.size() < mFaceSEL)
      return;

   //first, see if the mat already exists in our list
   bool found = false;
   U32 oldmatID = mConvexSEL->mSurfaceUVs[mFaceSEL].matID;

   if (dStrcmp(materialName, mConvexSEL->getMaterialName().c_str()))
   {
      for (U32 i = 0; i < mConvexSEL->mSurfaceTextures.size(); i++)
      {
         if (!dStrcmp(mConvexSEL->mSurfaceTextures[i].materialName, materialName))
         {
            //found a match
            mConvexSEL->mSurfaceUVs[mFaceSEL].matID = i + 1;
            found = true;
         }
      }

      if (!found)
      {
         //add a new one
         ConvexShape::surfaceMaterial newMat;
         newMat.materialName = materialName;

         mConvexSEL->mSurfaceTextures.push_back(newMat);

         mConvexSEL->mSurfaceUVs[mFaceSEL].matID = mConvexSEL->mSurfaceTextures.size();
      }
   }
   else
   {
      mConvexSEL->mSurfaceUVs[mFaceSEL].matID = 0;
   }

   //run through and find out if there are any other faces still using the old mat texture
   if (oldmatID != 0)
   {
      S32 curMatCount = mConvexSEL->mSurfaceTextures.size();

      bool used = false;
      for (U32 i = 0; i < mConvexSEL->mSurfaceUVs.size(); i++)
      {
         if (mConvexSEL->mSurfaceUVs[i].matID == oldmatID)
         {
            used = true;
            break;
         }
      }

      if (!used)
      {
         //that was the last reference, so let's update the listings on the shape
         if (mConvexSEL->mSurfaceTextures[oldmatID - 1].materialInst)
            SAFE_DELETE(mConvexSEL->mSurfaceTextures[oldmatID - 1].materialInst);

         mConvexSEL->mSurfaceTextures.erase(oldmatID-1);

         for (U32 i = 0; i < mConvexSEL->mSurfaceUVs.size(); i++)
         {
            if (mConvexSEL->mSurfaceUVs[i].matID > oldmatID)
               mConvexSEL->mSurfaceUVs[i].matID--;
         }
      }
   }

   //mConvexSEL->mSurfaceUVs[mFaceSEL].materialName = materialName;

   mConvexSEL->setMaskBits(ConvexShape::UpdateMask);
}

void GuiConvexEditorCtrl::setSelectedFaceHorzFlip(bool flipped)
{
   if (mFaceSEL == -1 || mConvexSEL == NULL)
      return;

   mConvexSEL->mSurfaceUVs[mFaceSEL].horzFlip = flipped;

   mConvexSEL->setMaskBits(ConvexShape::UpdateMask);
}

void GuiConvexEditorCtrl::setSelectedFaceVertFlip(bool flipped)
{
   if (mFaceSEL == -1 || mConvexSEL == NULL)
      return;

   mConvexSEL->mSurfaceUVs[mFaceSEL].vertFlip = flipped;

   mConvexSEL->setMaskBits(ConvexShape::UpdateMask);
}

void GuiConvexEditorCtrl::setSelectedFaceZRot(float degrees)
{
   if (mFaceSEL == -1 || mConvexSEL == NULL)
      return;

   F32 oldRot = mDegToRad(mConvexSEL->mSurfaceUVs[mFaceSEL].zRot);
   mConvexSEL->mSurfaceUVs[mFaceSEL].zRot = degrees;

   EulerF curEul = mConvexSEL->mSurfaces[mFaceSEL].toEuler();

   MatrixF oldRotMat = MatrixF(EulerF(0, 0, -oldRot));

   mConvexSEL->mSurfaces[mFaceSEL].mul(oldRotMat);

   MatrixF newRotMat = MatrixF(EulerF(0, 0, mDegToRad(mConvexSEL->mSurfaceUVs[mFaceSEL].zRot)));

   mConvexSEL->mSurfaces[mFaceSEL].mul(newRotMat);

   //Point3F curPos = mConvexSEL->mSurfaces[mFaceSEL].getPosition();

   //we roll back our existing modified rotation before setting the new one, just to keep it consistent
   //const MatrixF &gMat = MatrixF(EulerF(curEul.x, curEul.y, curEul.z - oldRot + mDegToRad(mConvexSEL->mSurfaceUVs[mFaceSEL].zRot)), curPos);

   //mConvexSEL->mSurfaces[mFaceSEL] = MatrixF(EulerF(curEul.x, curEul.y, mDegToRad(mConvexSEL->mSurfaceUVs[mFaceSEL].zRot)), curPos);

   /*MatrixF surfMat;
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

   mConvexSEL->mSurfaces[mFaceSEL] = surfMat;*/

   updateShape(mConvexSEL, mFaceSEL);

   mConvexSEL->setMaskBits(ConvexShape::UpdateMask);

   /*
   const MatrixF &gMat = mGizmo->getTransform();
   MatrixF surfMat;
   surfMat.mul( mConvexSEL->mWorldToObj, gMat );

   MatrixF worldToObj ( mConvexSEL->getTransform() );
   worldToObj.scale( mConvexSEL->getScale() );
   worldToObj.inverse();

   Point3F newPos;
   newPos = gMat.getPosition();

   worldToObj.mulP( newPos );
   surfMat.setPosition( newPos );

   // Clear out floating point errors.
   cleanMatrix( surfMat );

   if (mGizmo->getSelection() == Gizmo::Axis_Z)
   {
   MatrixF curSurfMat = mConvexSEL->mSurfaces[mFaceSEL];
   EulerF curSufRot = curSurfMat.toEuler();

   EulerF newSufRot = surfMat.toEuler();

   float zRot = mRadToDeg(newSufRot.z - curSufRot.z);

   float curZRot = mConvexSEL->mSurfaceTextures[mFaceSEL].zRot;

   mConvexSEL->mSurfaceTextures[mFaceSEL].zRot += zRot;
   }

   mConvexSEL->mSurfaces[mFaceSEL] = surfMat;

   updateShape( mConvexSEL, mFaceSEL );
   */
}

void GuiConvexEditorCtrl::toggleGridSnapping()
{
   if (mGridSnap)
      mGridSnap = false;
   else
      mGridSnap = true;
}

void GuiConvexEditorCtrl::updateShape()
{
   if (mConvexSEL)
      mConvexSEL->inspectPostApply();
}

void GuiConvexEditorCtrl::setGridSnapSize(float gridSize)
{
   mGridPlaneSize = gridSize;
}

void GuiConvexEditorUndoAction::undo()
{
   ConvexShape *object = NULL;
   if ( !Sim::findObject( mObjId, object ) )
      return;

   // Temporarily save the ConvexShape current data.   
   Vector< MatrixF > tempSurfaces;   
   tempSurfaces.merge( object->mSurfaces );
   MatrixF tempObjToWorld( object->getTransform() );
   Point3F tempScale( object->getScale() );

   // Restore the Object to the UndoAction state.
   object->mSurfaces.clear();
   object->mSurfaces.merge( mSavedSurfaces );   
   object->setScale( mSavedScale );
   object->setTransform( mSavedObjToWorld );

   // Regenerate the ConvexShape and synch the client object.
   object->_updateGeometry();
   GuiConvexEditorCtrl::synchClientObject( object );

   // If applicable set the selected ConvexShape and face
   // on the editor.   
   mEditor->setSelection( object, -1 );
   mEditor->updateGizmoPos();

   // Now save the previous ConvexShape data in this UndoAction
   // since an undo action must become a redo action and vice-versa
   
   mSavedObjToWorld = tempObjToWorld;
   mSavedScale = tempScale;
   mSavedSurfaces.clear();
   mSavedSurfaces.merge( tempSurfaces );   
}

ConvexEditorCreateTool::ConvexEditorCreateTool( GuiConvexEditorCtrl *editor )
 : Parent( editor ),
   mStage( -1 ),
   mNewConvex( NULL )
{
}

void ConvexEditorCreateTool::onActivated( ConvexEditorTool *prevTool )
{
   mEditor->clearSelection();
   mStage = -1;
   mNewConvex = NULL;
}

void ConvexEditorCreateTool::onDeactivated( ConvexEditorTool *newTool )
{
   if ( mNewConvex )
      mNewConvex->deleteObject();

   mStage = -1;
   mNewConvex = NULL;
   mEditor->mouseUnlock();
}

ConvexEditorTool::EventResult ConvexEditorCreateTool::on3DMouseDown( const Gui3DMouseEvent &event )
{
   if ( mStage == -1 )
   {
      mEditor->setFirstResponder();
      mEditor->mouseLock();

      Point3F start( event.pos );
      Point3F end( event.pos + event.vec * 10000.0f );      
      RayInfo ri;
      
      bool hit = gServerContainer.castRay( event.pos, end, STATIC_COLLISION_TYPEMASK, &ri );

      MatrixF objMat( true );

      // Calculate the orientation matrix of the new ConvexShape
      // based on what has been clicked.

      if ( !hit )
      {
         objMat.setPosition( event.pos + event.vec * 100.0f );      
      }
      else
      {
         if ( dynamic_cast< ConvexShape* >( ri.object ) )
         {
            ConvexShape *hitShape = static_cast< ConvexShape* >( ri.object );
            objMat = hitShape->getSurfaceWorldMat( ri.face );
            objMat.setPosition( ri.point );
         }
         else
         {
            Point3F rvec;
            Point3F fvec( mEditor->getCameraMat().getForwardVector() );
            Point3F uvec( ri.normal );

            rvec = mCross( fvec, uvec );

            if ( rvec.isZero() )
            {
               fvec = mEditor->getCameraMat().getRightVector();
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

      mNewConvex = new ConvexShape();                       

      mNewConvex->setTransform( objMat );   
		
		mNewConvex->setField( "material", Parent::mEditor->mMaterialName );
		
      mNewConvex->registerObject();
      mPlaneSizes.set( 0.1f, 0.1f, 0.1f );
      mNewConvex->resizePlanes( mPlaneSizes );
      mEditor->updateShape( mNewConvex );
      
      mTransform = objMat;     

      mCreatePlane.set( objMat.getPosition(), objMat.getUpVector() );
   }
   else if ( mStage == 0 )
   {
      // Handle this on mouseUp
   }
   
   return Handled;
}

ConvexEditorTool::EventResult ConvexEditorCreateTool::on3DMouseUp( const Gui3DMouseEvent &event )
{
   if ( mNewConvex && mStage == -1 )
   {
      mStage = 0;      

      mCreatePlane = PlaneF( mNewConvex->getPosition(), mNewConvex->getTransform().getForwardVector() );

      mTransform.setPosition( mNewConvex->getPosition() );      

      return Handled;
   }
   else if ( mStage == 0 )
   {
      SimGroup *mg;
      Sim::findObject( "MissionGroup", mg );

      mg->addObject( mNewConvex );

      mStage = -1;

      // Grab the mission editor undo manager.
      UndoManager *undoMan = NULL;
      if ( !Sim::findObject( "EUndoManager", undoMan ) )
      {
         Con::errorf( "ConvexEditorCreateTool::on3DMouseDown() - EUndoManager not found!" );
         mNewConvex = NULL;
         return Failed;           
      }

      // Create the UndoAction.
      MECreateUndoAction *action = new MECreateUndoAction("Create ConvexShape");
      action->addObject( mNewConvex );

      // Submit it.               
      undoMan->addAction( action );

      mEditor->setField( "isDirty", "1" );

      mEditor->setSelection( mNewConvex, -1 );      

      mNewConvex = NULL;

      mEditor->mouseUnlock();

      return Done;
   }

   return Done;
}

ConvexEditorTool::EventResult ConvexEditorCreateTool::on3DMouseMove( const Gui3DMouseEvent &event )
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

      mEditor->updateShape( mNewConvex );

      Point3F pos( mTransform.getPosition() );
      pos += mPlaneSizes.z * 0.5f * mTransform.getUpVector();
      mNewConvex->setPosition( pos );
   }

   return Handled;
}

ConvexEditorTool::EventResult ConvexEditorCreateTool::on3DMouseDragged( const Gui3DMouseEvent &event )
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
   mEditor->updateShape( mNewConvex );   

   Point3F pos( mTransform.getPosition() );
   pos += mTransform.getRightVector() * hitPos.x * 0.5f;
   pos += mTransform.getForwardVector() * hitPos.y * 0.5f;

   mNewConvex->setPosition( pos );

   return Handled;
}

void ConvexEditorCreateTool::renderScene( const RectI &updateRect )
{
  
}

ConvexShape* ConvexEditorCreateTool::extrudeShapeFromFace( ConvexShape *inShape, S32 inFaceId )
{
   ConvexShape::Geometry &inShapeGeometry = inShape->getGeometry();
   ConvexShape::Face &inFace = inShapeGeometry.faces[inFaceId];
   Vector< Point3F > &inShapePointList = inShapeGeometry.points;
   Vector< MatrixF > &inShapeSurfaces = inShape->getSurfaces();
   
   S32 shapeFaceCount = inFace.edges.size() + 2;   
   
   MatrixF inShapeToWorld( inShape->getTransform() );
   inShapeToWorld.scale( inShape->getScale() );
   //MatrixF inWorldToShape( inShapeToWorld );
   //inWorldToShape.inverse();

   MatrixF shapeToWorld;
   shapeToWorld.mul( inShape->getTransform(), inShapeSurfaces[inFaceId] );
   Point3F tmp( inShapeSurfaces[inFaceId].getPosition() );
   inShapeToWorld.mulP( tmp );
   shapeToWorld.setPosition( tmp );
   MatrixF worldToShape( shapeToWorld );
   worldToShape.inverse();

   MatrixF inShapeToNewShape;
   inShapeToNewShape.mul( inShapeToWorld, worldToShape );   

   ConvexShape *newShape = new ConvexShape;   
   newShape->setTransform( shapeToWorld );   

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

	//newShape->setField( "material", Parent::mEditor->mMaterialName );
   newShape->setField("material", inShape->getMaterialName());

   newShape->registerObject();
   mEditor->updateShape( newShape );

   SimGroup *group;
   if ( Sim::findObject( "missionGroup", group ) )
      group->addObject( newShape );

   return newShape;
}

void GuiConvexEditorCtrl::hollowShape( ConvexShape *shape, F32 thickness )
{
   // Create a new Convex for each face of the original shape.
   // This is the same as an extrude from face operation going inward by the thickness
   // for every face.

   Vector< ConvexShape* > convexList;

   for ( S32 i = 0; i < shape->mGeometry.faces.size(); i++ )
   {
      ConvexShape *faceShape = mCreateTool->extrudeShapeFromFace( shape, i );
      MatrixF &inwardFace = faceShape->mSurfaces[1];
      //MatrixF &outwardFace = faceShape->mSurfaces[0];      

      Point3F invec = inwardFace.getUpVector();

      inwardFace.setPosition( inwardFace.getPosition() + invec * thickness );

      updateShape( faceShape );

      convexList.push_back( faceShape );
   }

   convexList.push_front( shape );
   submitUndo( HollowShape, convexList );   
}

void GuiConvexEditorCtrl::hollowSelection()
{
   if ( mConvexSEL )
   {
      hollowShape( mConvexSEL, 0.15f );
      setSelection( NULL, -1 );      
   }
}

void GuiConvexEditorCtrl::recenterSelection()
{
   if ( mConvexSEL )    
   {
      recenterShape( mConvexSEL );   
      updateGizmoPos();
   }
}

void GuiConvexEditorCtrl::recenterShape( ConvexShape *shape )
{
   submitUndo( ModifyShape, shape );
   shape->recenter();
   synchClientObject( shape );
}

void GuiConvexEditorCtrl::dropSelectionAtScreenCenter()
{
   // This code copied from WorldEditor.
   // All the dropping code would be moved to somewhere common, but its not.

   if ( !mConvexSEL )
      return;

   // Calculate the center of the screen (in global screen coordinates)
   Point2I offset = localToGlobalCoord(Point2I(0,0));
   Point3F sp(F32(offset.x + F32(getExtent().x / 2)), F32(offset.y + (getExtent().y / 2)), 1.0f);

   // Calculate the view distance to fit the selection
   // within the camera's view.
   const Box3F bounds = mConvexSEL->getWorldBox();
   F32 radius = bounds.len()*0.5f;
   F32 viewdist = calculateViewDistance(radius);

   // Be careful of infinite sized objects, or just large ones in general.
   if(viewdist > 100.0f )
      viewdist = 100.0f;

   // Position the selection   
   mConvexSEL->setPosition( smCamPos + smCamMatrix.getForwardVector() * viewdist );

   synchClientObject( mConvexSEL );

   updateGizmoPos();
}

void GuiConvexEditorCtrl::splitSelectedFace()
{
   if ( !mConvexSEL || mFaceSEL == -1 )
      return;

   if ( !isShapeValid( mConvexSEL ) )
      return;

   mLastValidShape = mConvexSEL->mSurfaces;

   const F32 radians = mDegToRad( 15.0f );
   Point3F rot( 0, 0, 0 );
   MatrixF rotMat( true );

   mConvexSEL->mSurfaces.increment();
   MatrixF &dstMat = mConvexSEL->mSurfaces.last();   
   const MatrixF &srcMat = mConvexSEL->mSurfaces[mFaceSEL];

   for ( S32 i = 0; i < 6; i++ )
   {
      F32 sign = i > 2 ? -1.0f : 1.0f;
      U32 idx = i % 3;

      rot.zero();
      rot[idx] = sign * radians;
      rotMat.set( (EulerF)rot );

      dstMat = srcMat * rotMat;

      updateShape( mConvexSEL );

      if ( isShapeValid( mConvexSEL ) )
      {
         mSavedSurfaces = mConvexSEL->mSurfaces;
         mConvexSEL->mSurfaces = mLastValidShape;

         submitUndo( ModifyShape, mConvexSEL );  

         mConvexSEL->mSurfaces = mSavedSurfaces;
         mLastValidShape = mSavedSurfaces;

         setSelection( mConvexSEL, mConvexSEL->mSurfaces.size() - 1 );

         return;
      }      
   }

   mConvexSEL->mSurfaces = mLastValidShape;
   updateShape( mConvexSEL );
   updateGizmoPos();
}

void GuiConvexEditorCtrl::CSGSubtractBrush()
{
   if (!mConvexSEL)
      return;

   SimGroup* misGroup;
   if (!Sim::findObject("MissionGroup", misGroup))
   {
      return;
   }

   CSGSolid clipSolid(mConvexSEL);

   return;

   Polyhedron clipPoly;
   convertToPolyhedron(mConvexSEL, &clipPoly);

   Vector<Polygon> clipPolygons;

   for (U32 i = 0; i < clipPoly.edgeList.size(); ++i)
   {
      Point3F pointA = clipPoly.pointList[clipPoly.edgeList[i].vertex[0]];
      Point3F pointB = clipPoly.pointList[clipPoly.edgeList[i].vertex[1]];

      DebugDrawer::get()->drawLine(pointA, pointB, ColorF(1, 0.9, 0));
      DebugDrawer::get()->setLastTTL(30000);
   }

   for (U32 c = 0; c < misGroup->size(); ++c)
   {
      ConvexShape* obj = dynamic_cast<ConvexShape*>(misGroup->at(c));
      if (obj)
      {
         //Don't want to CSG op on ourselves
         if (obj->getId() == mConvexSEL->getId())
            continue;

         //Early out if the shape doesn't even come close to overlapping
         if (!obj->getWorldBox().isOverlapped(mConvexSEL->getWorldBox()))
            continue;

         Polyhedron poly;
         convertToPolyhedron(obj, &poly);

         U32 back = 0;
         U32 front = 0;
         U32 on = 0;
         U32 pointCount = poly.pointList.size();

         ColorI vertColor;

         for (U32 i = 0; i < pointCount; ++i)
         {
            Point3F point = poly.pointList[i];
            
            if (clipPoly.isContained(point))
            {
               back++;
               vertColor = ColorI::BLUE;
            }
            else
            {
               front++;
               vertColor = ColorI::RED;
            }

            F32 squareSize = 0.1;
            DebugDrawer::get()->drawBox(point + Point3F(-squareSize, -squareSize, -squareSize), point + Point3F(squareSize, squareSize, squareSize), vertColor);
            DebugDrawer::get()->setLastTTL(15000);
         }

         //If we have points on the front and the back, it's an intersection,
         //if either has 0, we're not actually intersecting, so we'll return out here
         if (front == 0 || back == 0)
            continue;

         bool didSplit = false;
         for (U32 i = 0; i < clipPoly.planeList.size(); ++i)
         {
            Polyhedron backPoly;
            Polyhedron frontPoly;

            //
            PlaneF splitPlane = PlaneF(clipPoly.planeList[i].getPosition(), -clipPoly.planeList[i].getNormal());

            Point3F plnps = splitPlane.getPosition();

            Point3F plnnrm = splitPlane.getNormal();
            plnnrm.normalize();

            DebugDrawer::get()->drawLine(plnps, plnps + (plnnrm * 1), ColorI::RED);
            DebugDrawer::get()->setLastTTL(15000);

            if (CSGSplitBrush(obj, splitPlane, &backPoly, &frontPoly))
            {
               /*Point3F surfPos = worldSurface.getPosition();
               F32 planeSize = 10;
               Point3F corner = (mConvexSEL->mSurfaces[i].getRightVector() * planeSize) + (mConvexSEL->mSurfaces[i].getForwardVector() * planeSize);
               Point3F cornerB = -corner;

               DebugDrawer::get()->drawBox(surfPos + corner,  surfPos + cornerB, ColorF(0, 0, 1, 0.3));
               DebugDrawer::get()->setLastTTL(15000);

               Point3F surfaceNorm = mConvexSEL->mSurfaces[i].getUpVector();
               surfaceNorm.normalize();

               DebugDrawer::get()->drawLine(surfPos, surfPos + (surfaceNorm * 2), ColorF(1, 0, 0, 0.3));
               DebugDrawer::get()->setLastTTL(15000);*/

               //back poly
               obj->mSurfaces.clear();

               AnyPolyhedron tempPoly = backPoly;
               convertFromPolyhedron(&tempPoly, obj);
               updateShape(obj);
               recenterShape(obj);
               obj->setPosition(backPoly.getCenterPoint());

               //do a final validation
               //if the polyhedron is completely contained within the subtracting brush, drop it
               Polyhedron convSelPoly;
               convertToPolyhedron(mConvexSEL, &convSelPoly);

               if (!convSelPoly.isContained(frontPoly.pointList.address(), frontPoly.pointList.size()))
               {
                  ConvexShape* splitBrush = new ConvexShape();

                  //front poly
                  tempPoly = frontPoly;
                  convertFromPolyhedron(&tempPoly, splitBrush);
                  updateShape(splitBrush);
                  recenterShape(splitBrush);
                  splitBrush->setPosition(frontPoly.getCenterPoint());

                  splitBrush->registerObject();

                  misGroup->addObject(splitBrush);
               }
               break;
            }
         }
      }
   }
}

bool GuiConvexEditorCtrl::CSGSplitBrush(ConvexShape* targetBrush, PlaneF splitPlane, Polyhedron* backPolyhedron, Polyhedron* frontPolyhedron)
{
   if (!targetBrush || !backPolyhedron || !frontPolyhedron)
      return false;

   Polyhedron poly;
   convertToPolyhedron(targetBrush, &poly);

   Point3F splitPos = splitPlane.getPosition();
   Point3F splitNorm = splitPlane.getNormal();

   PlaneF invSplitPlane = PlaneF(splitPlane.getPosition(), -splitPlane.getNormal());

   //Early out if there's no intersection at all
   U32 back = 0;
   U32 front = 0;
   U32 on = 0;
   U32 pointCount = poly.pointList.size();

   for (U32 i = 0; i < pointCount; ++i)
   {
      Point3F point = poly.pointList[i];
      PlaneF::Side s = splitPlane.whichSide(point);

      ColorI vertColor;

      if (s == PlaneF::Side::Back)
         back++;
      else if (s == PlaneF::Side::Front)
         front++;
      else
         on++;
   }

   //If we have points on the front and the back, it's an intersection,
   //if either has 0, we're not actually intersecting, so we'll return out here
   if (front == 0 || back == 0)
   {
      return false;
   }

   /*for (U32 i = 0; i < pointCount; ++i)
   {
      Point3F point = poly.pointList[i];
      PlaneF::Side s = splitPlane.whichSide(point);

      ColorI vertColor;

      if (s == PlaneF::Side::Back)
      {
         back++;
         vertColor = ColorI::BLUE;
      }
      else if (s == PlaneF::Side::Front)
      {
         front++;
         vertColor = ColorI::RED;
      }
      else
      {
         on++;
         vertColor = ColorI::GREEN;
      }

      F32 squareSize = 0.1;
      DebugDrawer::get()->drawBox(point + Point3F(-squareSize, -squareSize, -squareSize), point + Point3F(squareSize, squareSize, squareSize), vertColor);
      DebugDrawer::get()->setLastTTL(15000);
   }*/

   Vector<PlaneF> validBackPlanes;
   Vector<PlaneF> validFrontPlanes;

   //We have a split, so time to rebuild the polyhedron with the new face and remove the clipped geometry
   //First, figure out what planes are specifically clipped
   for (U32 i = 0; i < poly.planeList.size(); ++i)
   {
      Polygon p;
      p.surfacePlane = poly.planeList[i];

      Vector<U32> faceIndicies;
      for (U32 e = 0; e < poly.edgeList.size(); ++e)
      {
         if (poly.edgeList[e].face[0] != i && poly.edgeList[e].face[1] != i)
            continue;

         Polyhedron::Edge edg = poly.edgeList[e];

         faceIndicies.push_back_unique(edg.vertex[0]);
         faceIndicies.push_back_unique(edg.vertex[1]);
      }

      //build the face so we can clip it
      Vector<Point3F> faceVerts;
      Point3F faceCenter = Point3F::Zero;
      for (U32 v = 0; v < faceIndicies.size(); ++v)
      {
         U32 index = faceIndicies[v];
         Point3F vertPos = poly.pointList[index];
         p.points.push_back(vertPos);

         faceCenter += vertPos;
      }
      faceCenter /= faceIndicies.size();

      Point3F faceNorm = p.surfacePlane.getNormal();

      Polygon::PlaneRelation planePos = p.getPlaneSide(splitPlane);

      if (planePos == Polygon::PlaneRelation::Back)
      {
         validBackPlanes.push_back(p.surfacePlane);
      }
      else if (planePos == Polygon::PlaneRelation::Intersect)
      {
         //split it

         //Do the target brush
         Point3F clippedVerts[256];
         U32 clippedVertsCount = splitPlane.clipPolygon(p.points.address(), p.points.size(), clippedVerts);

         if (clippedVertsCount == 0)
            continue;

         Point3F center = Point3F::Zero;
         for (U32 v = 0; v < clippedVertsCount; ++v)
         {
            Point3F vertPos = clippedVerts[v];
            center += vertPos;
         }

         center /= clippedVertsCount;

         PlaneF planeClipped = PlaneF(center, p.surfacePlane.getNormal());

         validBackPlanes.push_back(planeClipped);

         //And now the split brush
         clippedVertsCount = invSplitPlane.clipPolygon(p.points.address(), p.points.size(), clippedVerts);

         if (clippedVertsCount == 0)
            continue;

         center = Point3F::Zero;
         for (U32 v = 0; v < clippedVertsCount; ++v)
         {
            Point3F vertPos = clippedVerts[v];
            center += vertPos;
         }

         center /= clippedVertsCount;

         planeClipped = PlaneF(center, p.surfacePlane.getNormal());

         validFrontPlanes.push_back(planeClipped);
      }
      else if (planePos == Polygon::PlaneRelation::Front)
      {
         validFrontPlanes.push_back(p.surfacePlane);
      }
   }

   //never actually properly clipped any polygons, so bail out
   if (validBackPlanes.empty() || validFrontPlanes.empty())
      return false;

   //finally, add the splitting plane to plane sets
   validBackPlanes.push_back(invSplitPlane);
   validFrontPlanes.push_back(splitPlane);

   //Build the back polyhedron
   PlaneSetF backPolyPlanes = PlaneSetF(validBackPlanes.address(), validBackPlanes.size());
   
   backPolyhedron->pointList.clear();
   backPolyhedron->edgeList.clear();
   backPolyhedron->planeList.clear();
   backPolyhedron->buildFromPlanes(backPolyPlanes);

   if (backPolyhedron->planeList.empty())
      return false;

   for (U32 i = 0; i < backPolyhedron->edgeList.size(); ++i)
   {
      Point3F pointA = backPolyhedron->pointList[backPolyhedron->edgeList[i].vertex[0]];
      Point3F pointB = backPolyhedron->pointList[backPolyhedron->edgeList[i].vertex[1]];

      DebugDrawer::get()->drawLine(pointA, pointB);
      DebugDrawer::get()->setLastTTL(30000);
   }

   //and the front
   PlaneSetF frontPolyPlanes = PlaneSetF(validFrontPlanes.address(), validFrontPlanes.size());

   frontPolyhedron->pointList.clear();
   frontPolyhedron->edgeList.clear();
   frontPolyhedron->planeList.clear();
   frontPolyhedron->buildFromPlanes(frontPolyPlanes);

   if (frontPolyhedron->planeList.empty())
      return false;

   for (U32 i = 0; i < frontPolyhedron->edgeList.size(); ++i)
   {
      Point3F pointA = frontPolyhedron->pointList[frontPolyhedron->edgeList[i].vertex[0]];
      Point3F pointB = frontPolyhedron->pointList[frontPolyhedron->edgeList[i].vertex[1]];

      DebugDrawer::get()->drawLine(pointA, pointB);
      DebugDrawer::get()->setLastTTL(30000);
   }

   return true;

   //we objectively know it's a valid split so skip validation.
   //first, duplicate the brush
   /*ConvexShape* splitBrush = new ConvexShape();
   splitBrush->registerObject();

   SimGroup* misGroup;
   if (!Sim::findObject("MissionGroup", misGroup))
   {
      return false;
   }

   misGroup->addObject(splitBrush);

   splitBrush->setTransform(targetBrush->getTransform());
   splitBrush->mSurfaces = targetBrush->mSurfaces;
   splitBrush->mSurfaceUVs = targetBrush->mSurfaceUVs;
   splitBrush->mSurfaceTextures = targetBrush->mSurfaceTextures;

   //reject any planes that are past our splitting plane
   for (U32 i = 0; i < targetBrush->mSurfaces.size(); ++i)
   {
      MatrixF* p = &targetBrush->mSurfaces[i];

      PlaneF splitPlane = PlaneF(splitSurface.getPosition(), splitSurface.getUpVector());

      Point3F surfaceNorm = p->getUpVector();
      surfaceNorm.normalize();

      PlaneF surfacePlane = PlaneF(p->getPosition(), surfaceNorm);

      if (splitPlane.isParallelTo(surfacePlane) && splitPlane.getNormal() == surfaceNorm)
      {
         targetBrush->mSurfaces.erase(i);
         --i;
      }
   }

   //next, add the split plane to both brushes
   targetBrush->mSurfaces.push_back(splitSurface);

   //then, recenter and rebuild
   //updateShape(mConvexSEL);
   //mConvexSEL->_updateGeometry();
   //mConvexSEL->recenter();
   recenterShape(targetBrush);

   for (U32 i = 0; i < splitBrush->mSurfaces.size(); ++i)
   {
      MatrixF* p = &splitBrush->mSurfaces[i];

      PlaneF splitPlane = PlaneF(invSplit.getPosition(), invSplit.getUpVector());

      Point3F surfaceNorm = p->getUpVector();
      surfaceNorm.normalize();

      PlaneF surfacePlane = PlaneF(p->getPosition(), surfaceNorm);

      if (splitPlane.isParallelTo(surfacePlane) && splitPlane.getNormal() == surfaceNorm)
      {
         splitBrush->mSurfaces.erase(i);
         --i;
      }
   }

   //next, add the split plane to both brushes
   splitBrush->mSurfaces.push_back(invSplit);
   splitBrush->inspectPostApply();

   //then, recenter and rebuild
   //updateShape(splitBrush);
   //mConvexSEL->_updateGeometry();
   //splitBrush->recenter();
   recenterShape(splitBrush);

   return true;*/
}

void GuiConvexEditorCtrl::convertToPolyhedron(ConvexShape* targetBrush, Polyhedron* outPoly)
{
   /*MatrixF savedTransform = targetBrush->getTransform();
   Point3F savedScale = targetBrush->getScale();

   targetBrush->setTransform(MatrixF::Identity);
   targetBrush->setScale(Point3F(1.f, 1.f, 1.f));*/

   // Extract the geometry.  Use the object-space bounding volumes
   // as we have moved the object to the origin for the moment.

   OptimizedPolyList polyList;
   if (!targetBrush->buildPolyList(PLC_Export, &polyList,
      targetBrush->getObjBox(), targetBrush->getObjBox().getBoundingSphere()))
   {
      Con::errorf("GuiConvexEditorCtrl::convertToPolyhedron - Failed to extract geometry!");
      return;
   }

   // Restore the object's original transform.
   //targetBrush->setTransform(savedTransform);
   //targetBrush->setScale(savedScale);

   // Convert the polylist to a polyhedron.
   Polyhedron tempPoly = polyList.toPolyhedron();
   outPoly->planeList = tempPoly.planeList;
   outPoly->edgeList = tempPoly.edgeList;
   outPoly->pointList = tempPoly.pointList;
}

void GuiConvexEditorCtrl::convertFromPolyhedron(AnyPolyhedron* poly, ConvexShape* outBrush )
{
   const U32 numPlanes = poly->getNumPlanes();
   if (!numPlanes)
   {
      Con::errorf("GuiConvexEditorCtrl::convertFromPolyhedron - not a valid polyhedron");
      return;
   }

   // Add all planes.
   for (U32 i = 0; i < numPlanes; ++i)
   {
      const PlaneF& plane = poly->getPlanes()[i];

      // Polyhedron planes are facing inwards so we need to
      // invert the normal here.

      Point3F normal = plane.getNormal();
      normal.neg();

      // Turn the orientation of the plane into a quaternion.
      // The normal is our up vector (that's what's expected
      // by ConvexShape for the surface orientation).

      MatrixF orientation(true);
      MathUtils::getMatrixFromUpVector(normal, &orientation);
      const QuatF quat(orientation);

      // Get the plane position.

      const Point3F position = plane.getPosition();

      // Turn everything into a "surface" property for the ConvexShape.

      char buffer[1024];
      dSprintf(buffer, sizeof(buffer), "%g %g %g %g %g %g %g %i %g %g %g %g %f %d %d",
         quat.x, quat.y, quat.z, quat.w,
         position.x, position.y, position.z,
         0, 0, 0, 1.0, 1.0, 0, true, false);

      // Add the surface.

      static StringTableEntry sSurface = StringTable->insert("surface");
      outBrush->setDataField(sSurface, NULL, buffer);
   }
}

void GuiConvexEditorCtrl::convertToCSGSolid(ConvexShape* targetBrush, CSGSolid* outSolid)
{
   /*MatrixF savedTransform = targetBrush->getTransform();
   Point3F savedScale = targetBrush->getScale();

   targetBrush->setTransform(MatrixF::Identity);
   targetBrush->setScale(Point3F(1.f, 1.f, 1.f));*/

   // Extract the geometry.  Use the object-space bounding volumes
   // as we have moved the object to the origin for the moment.

   OptimizedPolyList polyList;
   if (!targetBrush->buildPolyList(PLC_Export, &polyList,
      targetBrush->getObjBox(), targetBrush->getObjBox().getBoundingSphere()))
   {
      Con::errorf("GuiConvexEditorCtrl::convertToPolyhedron - Failed to extract geometry!");
      return;
   }

   // Restore the object's original transform.
   //targetBrush->setTransform(savedTransform);
   //targetBrush->setScale(savedScale);

   // Convert the polylist to a polyhedron.
   Polyhedron tempPoly = polyList.toPolyhedron();

   Vector<Polygon> polies;
   for (U32 i = 0; i < tempPoly.planeList.size(); ++i)
   {
      U32 indicies[256];
      S32 indexCount;
      indexCount = tempPoly.extractFace(i, indicies, 256);

      Vector<Point3F> verts;
      for (U32 v = 0; v < indexCount; ++v)
      {
         Point3F vert = tempPoly.pointList[indicies[v]];
         verts.push_back(vert);
      }

      Polygon p = Polygon(verts);

      polies.push_back(p);
   }

   outSolid->polygons = polies;
}

DefineConsoleMethod( GuiConvexEditorCtrl, hollowSelection, void, (), , "" )
{
   object->hollowSelection();
}

DefineConsoleMethod( GuiConvexEditorCtrl, recenterSelection, void, (), , "" )
{
   object->recenterSelection();
}

DefineConsoleMethod( GuiConvexEditorCtrl, hasSelection, S32, (), , "" )
{
   return object->hasSelection();
}

DefineConsoleMethod( GuiConvexEditorCtrl, handleDelete, void, (), , "" )
{
   object->handleDelete();
}

DefineConsoleMethod( GuiConvexEditorCtrl, handleDeselect, void, (), , "" )
{
   object->handleDeselect();
}

DefineConsoleMethod( GuiConvexEditorCtrl, dropSelectionAtScreenCenter, void, (), , "" )
{
   object->dropSelectionAtScreenCenter();
}

DefineConsoleMethod( GuiConvexEditorCtrl, selectConvex, void, (ConvexShape *convex), , "( ConvexShape )" )
{
if (convex)
      object->setSelection( convex, -1 );
}

DefineConsoleMethod( GuiConvexEditorCtrl, splitSelectedFace, void, (), , "" )
{
   object->splitSelectedFace();
}

DefineEngineMethod(GuiConvexEditorCtrl, getSelectedFaceUVOffset, Point2F, (), ,
   "@brief Mount objB to this object at the desired slot with optional transform.\n\n"

   "@param objB  Object to mount onto us\n"
   "@param slot  Mount slot ID\n"
   "@param txfm (optional) mount offset transform\n"
   "@return true if successful, false if failed (objB is not valid)")
{
   //return Point2F(0, 0);
   return object->getSelectedFaceUVOffset();
}

DefineEngineMethod(GuiConvexEditorCtrl, getSelectedFaceUVScale, Point2F, (), ,
   "@brief Mount objB to this object at the desired slot with optional transform.\n\n"

   "@param objB  Object to mount onto us\n"
   "@param slot  Mount slot ID\n"
   "@param txfm (optional) mount offset transform\n"
   "@return true if successful, false if failed (objB is not valid)")
{
   //return Point2F(0, 0);
   return object->getSelectedFaceUVScale();
}

DefineEngineMethod(GuiConvexEditorCtrl, setSelectedFaceUVOffset, void, ( Point2F offset ), ( Point2F(0,0) ),
   "@brief Mount objB to this object at the desired slot with optional transform.\n\n"

   "@param objB  Object to mount onto us\n"
   "@param slot  Mount slot ID\n"
   "@param txfm (optional) mount offset transform\n"
   "@return true if successful, false if failed (objB is not valid)")
{
   //return Point2F(0, 0);
   return object->setSelectedFaceUVOffset(offset);
}

DefineEngineMethod(GuiConvexEditorCtrl, setSelectedFaceUVScale, void, (Point2F scale), (Point2F(0, 0)),
   "@brief Mount objB to this object at the desired slot with optional transform.\n\n"

   "@param objB  Object to mount onto us\n"
   "@param slot  Mount slot ID\n"
   "@param txfm (optional) mount offset transform\n"
   "@return true if successful, false if failed (objB is not valid)")
{
   //return Point2F(0, 0);
   return object->setSelectedFaceUVScale(scale);
}

DefineEngineMethod(GuiConvexEditorCtrl, setSelectedFaceMaterial, void, (const char* materialName), (""),
   "@brief Mount objB to this object at the desired slot with optional transform.\n\n"

   "@param objB  Object to mount onto us\n"
   "@param slot  Mount slot ID\n"
   "@param txfm (optional) mount offset transform\n"
   "@return true if successful, false if failed (objB is not valid)")
{
   //return Point2F(0, 0);
   if (!dStrcmp(materialName, ""))
      return;

   object->setSelectedFaceMaterial(materialName);
}

DefineEngineMethod(GuiConvexEditorCtrl, getSelectedFaceMaterial, const char*, (), ,
   "@brief Mount objB to this object at the desired slot with optional transform.\n\n"

   "@param objB  Object to mount onto us\n"
   "@param slot  Mount slot ID\n"
   "@param txfm (optional) mount offset transform\n"
   "@return true if successful, false if failed (objB is not valid)")
{
   return object->getSelectedFaceMaterial();
}

DefineEngineMethod(GuiConvexEditorCtrl, setSelectedFaceHorzFlip, void, (bool flipped), (false),
   "@brief Mount objB to this object at the desired slot with optional transform.\n\n"

   "@param objB  Object to mount onto us\n"
   "@param slot  Mount slot ID\n"
   "@param txfm (optional) mount offset transform\n"
   "@return true if successful, false if failed (objB is not valid)")
{
   object->setSelectedFaceHorzFlip(flipped);
}

DefineEngineMethod(GuiConvexEditorCtrl, setSelectedFaceVertFlip, void, (bool flipped), (false),
   "@brief Mount objB to this object at the desired slot with optional transform.\n\n"

   "@param objB  Object to mount onto us\n"
   "@param slot  Mount slot ID\n"
   "@param txfm (optional) mount offset transform\n"
   "@return true if successful, false if failed (objB is not valid)")
{
   object->setSelectedFaceVertFlip(flipped);
}

DefineEngineMethod(GuiConvexEditorCtrl, getSelectedFaceHorzFlip, bool, (), ,
   "@brief Mount objB to this object at the desired slot with optional transform.\n\n"

   "@param objB  Object to mount onto us\n"
   "@param slot  Mount slot ID\n"
   "@param txfm (optional) mount offset transform\n"
   "@return true if successful, false if failed (objB is not valid)")
{
   return object->getSelectedFaceHorzFlip();
}

DefineEngineMethod(GuiConvexEditorCtrl, getSelectedFaceVertFlip, bool, (), ,
   "@brief Mount objB to this object at the desired slot with optional transform.\n\n"

   "@param objB  Object to mount onto us\n"
   "@param slot  Mount slot ID\n"
   "@param txfm (optional) mount offset transform\n"
   "@return true if successful, false if failed (objB is not valid)")
{
   return object->getSelectedFaceVertFlip();
}

DefineEngineMethod(GuiConvexEditorCtrl, setSelectedFaceZRot, void, (float degrees), (0.0),
   "@brief Mount objB to this object at the desired slot with optional transform.\n\n"

   "@param objB  Object to mount onto us\n"
   "@param slot  Mount slot ID\n"
   "@param txfm (optional) mount offset transform\n"
   "@return true if successful, false if failed (objB is not valid)")
{
   object->setSelectedFaceZRot(degrees);
}

DefineEngineMethod(GuiConvexEditorCtrl, getSelectedFaceZRot, float, (), ,
   "@brief Mount objB to this object at the desired slot with optional transform.\n\n"

   "@param objB  Object to mount onto us\n"
   "@param slot  Mount slot ID\n"
   "@param txfm (optional) mount offset transform\n"
   "@return true if successful, false if failed (objB is not valid)")
{
   return object->getSelectedFaceZRot();
}

DefineEngineMethod(GuiConvexEditorCtrl, toggleGridSnapping, void, (),,
   "@brief Mount objB to this object at the desired slot with optional transform.\n\n"

   "@param objB  Object to mount onto us\n"
   "@param slot  Mount slot ID\n"
   "@param txfm (optional) mount offset transform\n"
   "@return true if successful, false if failed (objB is not valid)")
{
   object->toggleGridSnapping();
}

DefineEngineMethod(GuiConvexEditorCtrl, setGridSnapSize, void, (float gridSize), (1.0),
   "@brief Mount objB to this object at the desired slot with optional transform.\n\n"

   "@param objB  Object to mount onto us\n"
   "@param slot  Mount slot ID\n"
   "@param txfm (optional) mount offset transform\n"
   "@return true if successful, false if failed (objB is not valid)")
{
   object->setGridSnapSize(gridSize);
}

DefineEngineMethod(GuiConvexEditorCtrl, getGridSnapSize, float, (),,
   "@brief Mount objB to this object at the desired slot with optional transform.\n\n"

   "@param objB  Object to mount onto us\n"
   "@param slot  Mount slot ID\n"
   "@param txfm (optional) mount offset transform\n"
   "@return true if successful, false if failed (objB is not valid)")
{
   return object->getGridSnapSize();
}

DefineEngineMethod(GuiConvexEditorCtrl, updateShape, void, (),,
   "@brief Mount objB to this object at the desired slot with optional transform.\n\n"

   "@param objB  Object to mount onto us\n"
   "@param slot  Mount slot ID\n"
   "@param txfm (optional) mount offset transform\n"
   "@return true if successful, false if failed (objB is not valid)")
{
   //return Point2F(0, 0);
   return object->updateShape();
}

//ConvexEditorGui.CSGSubtractBrush()
DefineEngineMethod(GuiConvexEditorCtrl, CSGSubtractBrush, void, (), ,
   "@brief Mount objB to this object at the desired slot with optional transform.\n\n"

   "@param objB  Object to mount onto us\n"
   "@param slot  Mount slot ID\n"
   "@param txfm (optional) mount offset transform\n"
   "@return true if successful, false if failed (objB is not valid)")
{
   return object->CSGSubtractBrush();
}

DefineEngineMethod(GuiConvexEditorCtrl, CSGSplitBrush, void, (), ,
   "@brief Mount objB to this object at the desired slot with optional transform.\n\n"

   "@param objB  Object to mount onto us\n"
   "@param slot  Mount slot ID\n"
   "@param txfm (optional) mount offset transform\n"
   "@return true if successful, false if failed (objB is not valid)")
{
   return;
   //return object->CSGSplitBrush();
}

DefineEngineMethod(GuiConvexEditorCtrl, setVertMode, void, (), ,
   "@brief Mount objB to this object at the desired slot with optional transform.\n\n"

   "@param objB  Object to mount onto us\n"
   "@param slot  Mount slot ID\n"
   "@param txfm (optional) mount offset transform\n"
   "@return true if successful, false if failed (objB is not valid)")
{
   return object->setSelectMode(GuiConvexEditorCtrl::VertMode);
}

DefineEngineMethod(GuiConvexEditorCtrl, setFaceMode, void, (), ,
   "@brief Mount objB to this object at the desired slot with optional transform.\n\n"

   "@param objB  Object to mount onto us\n"
   "@param slot  Mount slot ID\n"
   "@param txfm (optional) mount offset transform\n"
   "@return true if successful, false if failed (objB is not valid)")
{
   return object->setSelectMode(GuiConvexEditorCtrl::FaceMode);
}

DefineEngineMethod(GuiConvexEditorCtrl, setBrushMode, void, (), ,
   "@brief Mount objB to this object at the desired slot with optional transform.\n\n"

   "@param objB  Object to mount onto us\n"
   "@param slot  Mount slot ID\n"
   "@param txfm (optional) mount offset transform\n"
   "@return true if successful, false if failed (objB is not valid)")
{
   return object->setSelectMode(GuiConvexEditorCtrl::BrushMode);
}