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

#ifndef _ENTITY_H_
#define _ENTITY_H_

#ifndef _GAMEBASE_H_
#include "T3D/gameBase/gameBase.h"
#endif
#ifndef _MOVEMANAGER_H_
#include "T3D/gameBase/moveManager.h"
#endif
#ifndef _COMPONENT_OBJECT_H_
#include "component/ComponentObject.h"
#endif
#ifndef _COMPONENTINSTANCE_H_
#include "component/components/componentInstance.h"
#endif
#ifndef _SIMSET_H_
#include "console/simSet.h"
#endif
#ifndef _STOCK_INTERFACES_H_
#include "component/components/stockInterfaces.h"
#endif

#ifndef _MROTATION_H_
#include "math/mRotation.h"
#endif

class PhysicsBody;
class SimGroup;

//**************************************************************************
// Entity
//**************************************************************************
class Entity : public ComponentObject
{
   typedef ComponentObject Parent;
   friend class ComponentInstance;

private:
   //Point3F				mRot;
   RotationF           mRot;

protected:

   virtual void   processTick(const Move* move);
   virtual void   advanceTime( F32 dt );
   virtual void   interpolateTick( F32 delta );

   void prepRenderImage(SceneRenderState *state);

   bool           onAdd();
   void           onRemove();

public:
   struct StateDelta 
   {
      Move move;                    ///< Last move from server
      F32 dt;                       ///< Last interpolation time
      // Interpolation data
      Point3F pos;
      Point3F posVec;
      QuatF rot[2];
      // Warp data
      S32 warpTicks;                ///< Number of ticks to warp
      S32 warpCount;                ///< Current pos in warp
      Point3F warpOffset;
      QuatF warpRot[2];
   };

   enum MaskBits 
   {
      TransformMask				 = Parent::NextFreeMask << 0,
      BoundsMask				    = Parent::NextFreeMask << 1,
      NextFreeMask              = Parent::NextFreeMask << 2
   };

   StateDelta mDelta;
   S32 mPredictionCount;            ///< Number of ticks to predict

   PhysicsBody *mPhysicsRep;		//Is fed data from behaviors. Entity never really sets this up itself.

   //
   Entity();
   ~Entity();

   static void    initPersistFields();

   virtual void setTransform(const MatrixF &mat);
   void setTransform(Point3F position, EulerF rotation);

   //void setTransform(Point3F position, RotationF rot);

   void setRotation(RotationF rotation);

   void setRotation(EulerF rotation);
   RotationF getRotation() { return mRot; }

   void setMountOffset(Point3F posOffset);
   void setMountRotation(EulerF rotOffset);

   //static bool _setEulerRotation( void *object, const char *index, const char *data );

   static bool _setRotation( void *object, const char *index, const char *data );
   static const char * _getRotation(void* obj, const char* data);

   virtual void getMountTransform( S32 index, const MatrixF &xfm, MatrixF *outMat );
   virtual void getRenderMountTransform( F32 delta, S32 index, const MatrixF &xfm, MatrixF *outMat );

   virtual void mountObject( SceneObject *obj, S32 node, const MatrixF &xfm = MatrixF::Identity );
   void mountObject(SceneObject* objB, MatrixF txfm);
   void onMount( SceneObject *obj, S32 node );
   void onUnmount( SceneObject *obj, S32 node );

   // NetObject
   U32 packUpdate( NetConnection *conn, U32 mask, BitStream *stream );
   void unpackUpdate( NetConnection *conn, BitStream *stream );

   //Behaviors
   ComponentInstance *behavior(const char *name);
   virtual bool deferAddingBehaviors() const { return true; }

   virtual void setObjectBox(Box3F objBox);

   void resetWorldBox() { Parent::resetWorldBox(); }
   void resetObjectBox() { Parent::resetObjectBox(); }
   void resetRenderWorldBox() { Parent::resetRenderWorldBox(); }

   //function redirects for collisions
   bool castRay(const Point3F &start, const Point3F &end, RayInfo* info);
   bool castRayRendered(const Point3F &start, const Point3F &end, RayInfo* info);
   bool buildPolyList(PolyListContext context, AbstractPolyList* polyList, const Box3F &box, const SphereF &sphere);
   virtual void buildConvex(const Box3F& box, Convex* convex);

   Signal< void(SimObject*, String, String) > onDataSet;
   virtual void setDataField(StringTableEntry slotName, const char *array, const char *value);
   virtual void onStaticModified(const char* slotName, const char* newValue);

   void pushEvent(const char* eventName, Vector<const char*> eventParams);

   //camera stuff
   virtual void getCameraTransform(F32* pos,MatrixF* mat);
   virtual void onCameraScopeQuery( NetConnection* connection, CameraScopeQuery* query );

   //Heirarchy stuff
   virtual void addObject( SimObject* object );
   virtual void removeObject( SimObject* object );

   void onInspect();
   void onEndInspect();

   DECLARE_CONOBJECT(Entity);

};

#endif //_ENTITY_H_
