//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _BOX_COLLISION_BEHAVIOR_H_
#define _BOX_COLLISION_BEHAVIOR_H_

#ifndef __RESOURCE_H__
#include "core/resource.h"
#endif
#ifndef _TSSHAPE_H_
#include "ts/tsShape.h"
#endif
#ifndef _SCENERENDERSTATE_H_
#include "scene/sceneRenderState.h"
#endif
#ifndef _MBOX_H_
#include "math/mBox.h"
#endif
#ifndef _Entity_H_
#include "T3D/Entity.h"
#endif

#ifndef _RENDER_INTERFACES_H_
#include "component/components/render/renderInterfaces.h"
#endif

#ifndef _COLLISION_INTERFACES_H_
#include "component/components/collision/collisionInterfaces.h"
#endif

#ifndef _SCENERENDERSTATE_H_
#include "scene/sceneRenderState.h"
#endif
#ifndef _RENDERPASSMANAGER_H_
#include "renderInstance/renderPassManager.h"
#endif

class TSShapeInstance;
class SceneRenderState;
class MeshColliderBehaviorInstance;
//////////////////////////////////////////////////////////////////////////
/// 
/// 
//////////////////////////////////////////////////////////////////////////
class MeshColliderPolysoupConvex : public Convex
{
   typedef Convex Parent;
   friend class MeshColliderBehaviorInstance;

public:
   MeshColliderPolysoupConvex();
   ~MeshColliderPolysoupConvex() {};

public:
   Box3F                box;
   Point3F              verts[4];
   PlaneF               normal;
   S32                  idx;
   TSMesh               *mesh;

   static SceneObject* smCurObject;
   static TSShapeInstance* smCurShapeInstance;

   Convex* mNext;

public:

   void init(SceneObject* obj) { mObject = obj; }

   // Returns the bounding box in world coordinates
   Box3F getBoundingBox() const;
   Box3F getBoundingBox(const MatrixF& mat, const Point3F& scale) const;

   void getFeatures(const MatrixF& mat,const VectorF& n, ConvexFeature* cf);

   // This returns a list of convex faces to collide against
   void getPolyList(AbstractPolyList* list);

   // Renders the convex list. Used for debugging
   virtual void render();

   // This returns all the polies in our convex list
   // It's really meant for debugging purposes
   void getAllPolyList(AbstractPolyList *list);

   // This returns the furthest point from the input vector
   Point3F support(const VectorF& v) const;
};

class MeshColliderBehavior : public Component
{
   typedef Component Parent;

public:
   MeshColliderBehavior();
   virtual ~MeshColliderBehavior();
   DECLARE_CONOBJECT(MeshColliderBehavior);

   static void initPersistFields();

   //override to pass back a MeshColliderBehaviorInstance
   virtual ComponentInstance *createInstance();
};

class MeshColliderBehaviorInstance : public ComponentInstance,
   public CollisionInterface,
   public PrepRenderImageInterface,
   public BuildConvexInterface,
   public CastRayInterface
{
   typedef ComponentInstance Parent;

protected:
   Convex *mConvexList;

   Vector<S32> mCollisionDetails;

   struct DebugRenderStash
   {
      TSShapeInstance* shapeInstance;

      U32 triCount;

      Vector<Point3F> vertA;
      Vector<Point3F> vertB;
      Vector<Point3F> vertC;

      void clear()
      {
         vertA.clear();
         vertB.clear();
         vertC.clear();
         triCount = 0;
      }
   };

   DebugRenderStash mDebugRender;

public:
   MeshColliderBehaviorInstance(Component *btemplate = NULL);
   virtual ~MeshColliderBehaviorInstance();
   DECLARE_CONOBJECT(MeshColliderBehaviorInstance);

   virtual bool onAdd();
   virtual void onRemove();
   static void initPersistFields();

   static void consoleInit();

   virtual void update();

   virtual void processTick(const Move* move);

   virtual void prepRenderImage( SceneRenderState *state );
   virtual void renderConvex( ObjectRenderInst *ri, SceneRenderState *state, BaseMatInstance *overrideMat );

   virtual U32 packUpdate(NetConnection *con, U32 mask, BitStream *stream);
   virtual void unpackUpdate(NetConnection *con, BitStream *stream);

   virtual void onComponentRemove();
   virtual void onComponentAdd();

   void prepCollision();
   void _updatePhysics();

   virtual bool checkCollisions( const F32 travelTime, Point3F *velocity, Point3F start );

   virtual bool buildPolyList(PolyListContext context, AbstractPolyList* polyList, const Box3F &box, const SphereF &sphere);

   virtual bool buildConvex(const Box3F& box, Convex* convex);
   virtual bool castRay(const Point3F &start, const Point3F &end, RayInfo* info);

   bool buildConvexOpcode( TSShapeInstance* sI, S32 dl, const Box3F &bounds, Convex *c, Convex *list );

   bool buildMeshOpcode(  TSMesh *mesh, const MatrixF &meshToObjectMat, const Box3F &bounds, Convex *convex, Convex *list);

   bool castRayOpcode( S32 dl, const Point3F & startPos, const Point3F & endPos, RayInfo *info);
   bool castRayMeshOpcode(TSMesh *mesh, const Point3F &s, const Point3F &e, RayInfo *info, TSMaterialList *materials );

   virtual bool updateCollisions(F32 time, VectorF vector, VectorF velocity);

   virtual void updateWorkingCollisionSet(const U32 mask);

   TSShapeInstance* getShapeInstance();
};

#endif // _COMPONENT_H_
