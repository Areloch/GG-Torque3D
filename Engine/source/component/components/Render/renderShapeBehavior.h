//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _RENDERSHAPEBEHAVIOR_H_
#define _RENDERSHAPEBEHAVIOR_H_

#ifndef _COMPONENT_H_
#include "component/components/component.h"
#endif
#ifndef _COMPONENTINSTANCE_H_
#include "component/components/componentInstance.h"
#endif

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
#ifndef _ACTOR_H_
#include "T3D/Entity.h"
#endif

#ifndef _NETSTRINGTABLE_H_
   #include "sim/netStringTable.h"
#endif
/*#ifndef _PATH_H_
#include "core/util/path.h"
#endif*/

/*#ifndef _STOCK_INTERFACES_H_
#include "component/components/stockInterfaces.h"
#endif*/

#ifndef _RENDER_INTERFACES_H_
#include "component/components/Render/renderInterfaces.h"
#endif

class TSShapeInstance;
class SceneRenderState;
//////////////////////////////////////////////////////////////////////////
/// 
/// 
//////////////////////////////////////////////////////////////////////////
class RenderShapeBehavior : public Component
{
   typedef Component Parent;

public:
   RenderShapeBehavior();
   virtual ~RenderShapeBehavior();
   DECLARE_CONOBJECT(RenderShapeBehavior);

   virtual bool onAdd();
   virtual void onRemove();
   static void initPersistFields();

   virtual U32 packUpdate(NetConnection *con, U32 mask, BitStream *stream);
   virtual void unpackUpdate(NetConnection *con, BitStream *stream);

   //override to pass back a RenderBehaviorInstance
   virtual ComponentInstance *createInstance();
};

class RenderShapeBehaviorInstance : public ComponentInstance,
   public PrepRenderImageInterface,
   public TSShapeInstanceInterface,
   public TSShapeInterface,
   //public GeometryInterface,
   public EditorInspectInterface
{
   typedef ComponentInstance Parent;

   enum
   {
      ShapeMask = Parent::NextFreeMask,
      MaterialMask = Parent::NextFreeMask << 1, 
      NextFreeMask = Parent::NextFreeMask << 2,
   };

protected:
   StringTableEntry		mShapeName;
   Resource<TSShape>		mShape;
   TSShapeInstance *		mShapeInstance;
   Box3F						mShapeBounds;
   Point3F					mCenterOffset;

   struct matMap
   {
      String matName;
      U32 slot;
   };

   Vector<matMap>  mChangingMaterials;
   Vector<matMap>  mMaterials;

   class boneObject : public SimGroup
   {
      RenderShapeBehaviorInstance *mOwner;
   public:
      boneObject(RenderShapeBehaviorInstance *owner){ mOwner = owner;}

      StringTableEntry mBoneName;
      S32 mItemID;

      virtual void addObject(SimObject *obj);
   };

   Vector<boneObject*> mNodesList;

public:
   RenderShapeBehaviorInstance(Component *btemplate = NULL);
   virtual ~RenderShapeBehaviorInstance();
   DECLARE_CONOBJECT(RenderShapeBehaviorInstance);

   virtual bool onAdd();
   virtual void onRemove();
   static void initPersistFields();

   virtual void inspectPostApply();

   virtual void prepRenderImage(SceneRenderState *state );

   virtual U32 packUpdate(NetConnection *con, U32 mask, BitStream *stream);
   virtual void unpackUpdate(NetConnection *con, BitStream *stream);

   Box3F getShapeBounds() { return mShapeBounds; }

   MatrixF getNodeTransform(S32 nodeIdx);
   S32 getNodeByName(String nodeName);

   void updateShape();
   void updateMaterials();

   virtual void onComponentRemove();
   virtual void onComponentAdd();

   static bool _setShape( void *object, const char *index, const char *data );
   const char* _getShape( void *object, const char *data );

   virtual TSShape* getShape() { if(mShapeInstance)  return mShapeInstance->getShape(); else return NULL; }
   virtual TSShapeInstance* getShapeInstance() { return mShapeInstance; }

   void _onResourceChanged( const Torque::Path &path );

   virtual bool castRayRendered(const Point3F &start, const Point3F &end, RayInfo *info);

   void mountObjectToNode(SceneObject* objB, String node, MatrixF txfm);

   virtual void onDynamicModified(const char* slotName, const char* newValue);

   void changeMaterial(U32 slot, const char* newMat);

   //virtual Geometry* getGeometry();

   virtual void onInspect();
   virtual void onEndInspect();
};

#endif // _COMPONENT_H_
