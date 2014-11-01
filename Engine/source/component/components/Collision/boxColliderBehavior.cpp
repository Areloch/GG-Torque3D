//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "component/components/Collision/boxColliderBehavior.h"
#include "component/components/Physics/physicsBehavior.h"

//#include "platform/platform.h"
#include "console/consoleTypes.h"
#include "core/util/safeDelete.h"
#include "core/resourceManager.h"
//#include "core/stream/fileStream.h"
#include "console/consoleTypes.h"
#include "console/consoleObject.h"
//#include "ts/tsShapeInstance.h"
#include "core/stream/bitStream.h"
#include "scene/sceneRenderState.h"
#include "gfx/gfxTransformSaver.h"
#include "gfx/gfxDrawUtil.h"
//#include "gfx/primBuilder.h"
#include "console/engineAPI.h"
//#include "lighting/lightQuery.h"
#include "T3D/physics/physicsPlugin.h"
#include "T3D/physics/physicsBody.h"
#include "T3D/physics/physicsCollision.h"
#include "T3D/gameBase/gameConnection.h"
#include "collision/extrudedPolyList.h"
#include "math/mathIO.h"
#include "gfx/sim/debugDraw.h"  

#include "component/components/Physics/physicsInterfaces.h"

#include "collision/concretePolyList.h"

extern bool gEditingMission;

static bool sRenderBoxColliders = false;

//Docs
ConsoleDocClass( BoxColliderBehavior,
   "@brief The Box Collider component uses a box or rectangular convex shape for collisions.\n\n"
   
   "Colliders are individualized components that are similarly based off the CollisionInterface core.\n"
   "They are basically the entire functionality of how Torque handles collisions compacted into a single component.\n"
   "A collider will both collide against and be collided with, other entities.\n"
   "Individual colliders will offer different shapes. This box collider will generate a box/rectangle convex, \n"
   "while the mesh collider will take the owner Entity's rendered shape and do polysoup collision on it, etc.\n\n"

   "The general flow of operations for how collisions happen is thus:\n"
   "  -When the component is added(or updated) prepCollision() is called.\n"
   "    This will set up our initial convex shape for usage later.\n\n"

   "  -When we update via processTick(), we first test if our entity owner is mobile.\n"
   "    If our owner isn't mobile(as in, they have no components that provide it a velocity to move)\n"
   "    then we skip doing our active collision checks. Collisions are checked by the things moving, as\n"
   "    opposed to being reactionary. If we're moving, we call updateWorkingCollisionSet().\n"
   "    updateWorkingCollisionSet() estimates our bounding space for our current ticket based on our position and velocity.\n"
   "    If our bounding space has changed since the last tick, we proceed to call updateWorkingList() on our convex.\n"
   "    This notifies any object in the bounding space that they may be collided with, so they will call buildConvex().\n"
   "    buildConvex() will set up our ConvexList with our collision convex info.\n\n"

   "  -When the component that is actually causing our movement, such as SimplePhysicsBehavior, updates, it will check collisions.\n"
   "    It will call checkCollisions() on us. checkCollisions() will first build a bounding shape for our convex, and test\n"
   "    if we can early out because we won't hit anything based on our starting point, velocity, and tick time.\n"
   "    If we don't early out, we proceed to call updateCollisions(). This builds an ExtrudePolyList, which is then extruded\n"
   "    based on our velocity. We then test our extruded polies on our working list of objects we build\n"
   "    up earlier via updateWorkingCollisionSet. Any collisions that happen here will be added to our mCollisionList.\n"
   "    Finally, we call handleCollisionList() on our collisionList, which then queues out the colliison notice\n"
   "    to the object(s) we collided with so they can do callbacks and the like. We also report back on if we did collide\n"
   "    to the physics component via our bool return in checkCollisions() so it can make the physics react accordingly.\n\n"
   
   "One interesting point to note is the usage of mBlockColliding.\n"
   "This is set so that it dictates the return on checkCollisions(). If set to false, it will ensure checkCollisions()\n"
   "will return false, regardless if we actually collided. This is useful, because even if checkCollisions() returns false,\n"
   "we still handle the collisions so the callbacks happen. This enables us to apply a collider to an object that doesn't block\n"
   "objects, but does have callbacks, so it can act as a trigger, allowing for arbitrarily shaped triggers, as any collider can\n"
   "act as a trigger volume(including MeshCollider).\n\n"

   "@tsexample\n"
   "new BoxColliderComponentInstance()\n"
   "{\n"
   "   template = BoxColliderComponentTemplate;\n"
   "   colliderSize = \"1 1 2\";\n"
   "   blockColldingObject = \"1\";\n"
   "};\n"
   "@endtsexample\n"
      
   "@see SimplePhysicsBehavior\n"
   "@ingroup Collision\n"
   "@ingroup Components\n"
);
//Docs

//////////////////////////////////////////////////////////////////////////
// Constructor/Destructor
//////////////////////////////////////////////////////////////////////////
static struct Face {
   S32 vertex[4];
   S32 axis;
   bool flip;
} sFace[] =
{
   { {0,4,5,1}, 1,true  },
   { {0,2,6,4}, 0,true  },
   { {3,7,6,2}, 1,false },
   { {3,1,5,7}, 0,false },
   { {0,1,3,2}, 2,true  },
   { {4,6,7,5}, 2,false },
};

Point3F bBoxConvex::support(const VectorF& v) const
{
   Point3F p = mCenter;
   p.x += (v.x >= 0)? mSize.x: -mSize.x;
   p.y += (v.y >= 0)? mSize.y: -mSize.y;
   p.z += (v.z >= 0)? mSize.z: -mSize.z;
   return p;
}

Point3F bBoxConvex::getVertex(S32 v)
{
   Point3F p = mCenter;
   p.x += (v & 1)? mSize.x: -mSize.x;
   p.y += (v & 2)? mSize.y: -mSize.y;
   p.z += (v & 4)? mSize.z: -mSize.z;
   return p;
}

inline bool isOnPlane(Point3F p,PlaneF& plane)
{
   F32 dist = mDot(plane,p) + plane.d;
   return dist < 0.1 && dist > -0.1;
}

void bBoxConvex::getFeatures(const MatrixF& mat,const VectorF& n, ConvexFeature* cf)
{
   cf->material = 0;
   cf->object = mObject;
   cf->mVertexList.setSize(8);
   cf->mEdgeList.setSize(12);
   cf->mFaceList.setSize(6);

   Point3F points[8] = { Point3F(mCenter + Point3F(-mSize.x, -mSize.y, -mSize.z)),
      Point3F(mCenter + Point3F(-mSize.x, -mSize.y,  mSize.z)),
      Point3F(mCenter + Point3F( mSize.x, -mSize.y,  mSize.z)),
      Point3F(mCenter + Point3F( mSize.x, -mSize.y, -mSize.z)),
      Point3F(mCenter + Point3F(-mSize.x,  mSize.y, -mSize.z)),
      Point3F(mCenter + Point3F(-mSize.x,  mSize.y,  mSize.z)),
      Point3F(mCenter + Point3F( mSize.x,  mSize.y,  mSize.z)),
      Point3F(mCenter + Point3F( mSize.x,  mSize.y, -mSize.z)) };

   Point2I edges[12] = { Point2I(0,1), Point2I(1,2), Point2I(2,3), Point2I(3,0),
      Point2I(4,5), Point2I(5,6), Point2I(6,7), Point2I(7,4),
      Point2I(0,4), Point2I(1,5), Point2I(2,6), Point2I(3,7) };

   Point3I faces[6] = { Point3I(0,1,2), Point3I(4,5,6), Point3I(0,1,4),
      Point3I(3,2,6), Point3I(0,4,3), Point3I(1,5,6) };


   for(U32 i = 0; i < 8; i++)
   {
      mat.mulP(points[i],&cf->mVertexList[i]);
   }

   //edges
   for(U32 i = 0; i < 12; i++)
   {
      ConvexFeature::Edge edge;
      edge.vertex[0] = edges[i].x;
      edge.vertex[1] = edges[i].y;
      cf->mEdgeList[i] = edge;
   }

   //face
   for(U32 i = 0; i < 6; i++)
   {
      ConvexFeature::Face face;
      face.normal = PlaneF(cf->mVertexList[faces[i].x], cf->mVertexList[faces[i].y], cf->mVertexList[faces[i].z]);

      face.vertex[0] = faces[i].x;
      face.vertex[1] = faces[i].y;
      face.vertex[2] = faces[i].z;

      cf->mFaceList[i] = face;
   }
}

void bBoxConvex::getPolyList(AbstractPolyList* list)
{
   list->setTransform(&getTransform(), getScale());
   list->setObject(getObject());

   Point3F centerPos = getObject()->getPosition();

   U32 base = list->addPoint(centerPos + Point3F(-mSize.x, -mSize.y, -mSize.z));
   list->addPoint(centerPos + Point3F( mSize.x, -mSize.y, -mSize.z));
   list->addPoint(centerPos + Point3F(-mSize.x,  mSize.y, -mSize.z));
   list->addPoint(centerPos + Point3F( mSize.x,  mSize.y, -mSize.z));
   list->addPoint(centerPos + Point3F(-mSize.x, -mSize.y,  mSize.z));
   list->addPoint(centerPos + Point3F( mSize.x, -mSize.y,  mSize.z));
   list->addPoint(centerPos + Point3F(-mSize.x,  mSize.y,  mSize.z));
   list->addPoint(centerPos + Point3F( mSize.x,  mSize.y,  mSize.z));

   for (U32 i = 0; i < 6; i++) {
      list->begin(0, i);
      list->vertex(base + sFace[i].vertex[0]);
      list->vertex(base + sFace[i].vertex[1]);
      list->vertex(base + sFace[i].vertex[2]);
      //list->vertex(base + sFace[i].vertex[3]);

      list->plane(base + sFace[i].vertex[0],
         base + sFace[i].vertex[1],
         base + sFace[i].vertex[2]);
      list->end();

      //
      list->begin(0, i+1);
      list->vertex(base + sFace[i].vertex[2]);
      list->vertex(base + sFace[i].vertex[3]);
      list->vertex(base + sFace[i].vertex[0]);
      //list->vertex(base + sFace[i].vertex[3]);

      list->plane(base + sFace[i].vertex[2],
         base + sFace[i].vertex[3],
         base + sFace[i].vertex[0]);
      list->end();
   }
}

//
BoxColliderBehavior::BoxColliderBehavior()
{
   mNetFlags.set(Ghostable | ScopeAlways);

   addComponentField("colliderSize", "If enabled, will stop colliding objects from passing through.", "Vector", "1 1 1", "");
   addComponentField("blockCollisions", "If enabled, will stop colliding objects from passing through.", "bool", "1", "");
}

BoxColliderBehavior::~BoxColliderBehavior()
{
   for(S32 i = 0;i < mFields.size();++i)
   {
      ComponentField &field = mFields[i];
      SAFE_DELETE_ARRAY(field.mFieldDescription);
   }

   SAFE_DELETE_ARRAY(mDescription);
}

IMPLEMENT_CO_NETOBJECT_V1(BoxColliderBehavior);

//////////////////////////////////////////////////////////////////////////
ComponentInstance *BoxColliderBehavior::createInstance()
{
   return Parent::createInstance<BoxColliderBehaviorInstance>();
}

void BoxColliderBehavior::initPersistFields()
{
   Parent::initPersistFields();
}

//==========================================================================================
//==========================================================================================
BoxColliderBehaviorInstance::BoxColliderBehaviorInstance( Component *btemplate ) 
{
   mTemplate = btemplate;
   mOwner = NULL;

   mConvexList = NULL;

   mTimeoutList = NULL;

   mBlockColliding = true;

   mColliderScale = Point3F(1,1,1);

   mWorkingQueryBox.minExtents.set(-1e9f, -1e9f, -1e9f);
   mWorkingQueryBox.maxExtents.set(-1e9f, -1e9f, -1e9f);

   mNetFlags.set(Ghostable);

   CollisionMoveMask = ( TerrainObjectType     | PlayerObjectType  | 
      StaticShapeObjectType | VehicleObjectType |
      VehicleBlockerObjectType | DynamicShapeObjectType | StaticObjectType | EntityObjectType);

}

BoxColliderBehaviorInstance::~BoxColliderBehaviorInstance()
{
   delete mConvexList;
   mConvexList = NULL;
 }
IMPLEMENT_CO_NETOBJECT_V1(BoxColliderBehaviorInstance);

bool BoxColliderBehaviorInstance::onAdd()
{
   if(! Parent::onAdd())
      return false;

   return true;
}

void BoxColliderBehaviorInstance::onRemove()
{
   if(mConvexList != NULL)
      mConvexList->nukeList();
   Parent::onRemove();
}

void BoxColliderBehaviorInstance::onComponentRemove()
{
   SAFE_DELETE( (dynamic_cast<Entity*>(mOwner))->mPhysicsRep );
   mOwner->disableCollision();

   Parent::onComponentRemove();
}

void BoxColliderBehaviorInstance::onComponentAdd()
{
   Parent::onComponentAdd();
   if(isServerObject())
      prepCollision();
}

void BoxColliderBehaviorInstance::initPersistFields()
{
   Parent::initPersistFields();
   //addField( "colliderSize", TypePoint3F, Offset(mColliderScale, BoxColliderBehaviorInstance), "");
   addProtectedField("colliderSize", TypePoint3F, Offset(mColliderScale, BoxColliderBehaviorInstance), &_setColliderSize, defaultProtectedGetFn);
   addField("blockCollisions", TypeBool, Offset(mBlockColliding, BoxColliderBehaviorInstance), "");
}

bool BoxColliderBehaviorInstance::_setColliderSize( void *object, const char *index, const char *data )
{
   BoxColliderBehaviorInstance *boxCollider = static_cast<BoxColliderBehaviorInstance*>(object);
   boxCollider->prepCollision();
   boxCollider->setMaskBits(ColliderMask);
   return true;
}

void BoxColliderBehaviorInstance::consoleInit()
{
   Con::addVariable("$BoxCollider::renderCollision", TypeBool, &sRenderBoxColliders, 
      "@brief Determines if the box colliders collision mesh should be rendered.\n\n"
      "This is mainly used for the tools and debugging.\n"
	   "@ingroup GameObjects\n");
}

U32 BoxColliderBehaviorInstance::packUpdate(NetConnection *con, U32 mask, BitStream *stream)
{
   U32 retMask = Parent::packUpdate(con, mask, stream);

   if ( mask & (ColliderMask | InitialUpdateMask))
   {
      if(mOwner){
         if(con->getGhostIndex(mOwner) != -1)
         {
            stream->writeFlag( true );
            mathWrite( *stream, mColliderScale );
         }
         else
         {
            retMask |= InitialUpdateMask; //try it again untill our dependency is ghosted
            stream->writeFlag( false );
         }
      }
      else
         stream->writeFlag( false );
   }

   return retMask;
}

void BoxColliderBehaviorInstance::unpackUpdate(NetConnection *con, BitStream *stream)
{
   Parent::unpackUpdate(con, stream);

   if ( stream->readFlag() ) // UpdateMask
   {
      mathRead(*stream, &mColliderScale);
      prepCollision();
   }
}

void BoxColliderBehaviorInstance::prepCollision()
{
   if(!mOwner)
      return;

   // Allow the ShapeInstance to prep its collision if it hasn't already
   if(mColliderScale.isZero()){
      mOwner->disableCollision();
      return;
   }

   if(mConvexList != NULL)
      mConvexList->nukeList();

   BoxConvex* bC = new BoxConvex();
   bC->init(mOwner);
   bC->mSize = mColliderScale;
   mOwner->getObjBox().getCenter(&bC->mCenter);
   mConvexList = bC;

   Box3F colliderBounds = bC->getBoundingBox(mOwner->getTransform(), mOwner->getScale());

   Box3F ownerBounds = mOwner->getWorldBox();

   if(!ownerBounds.isContained(colliderBounds))
   {
      Box3F newBounds = ownerBounds;

      newBounds.extend(colliderBounds.minExtents);
      newBounds.extend(colliderBounds.maxExtents);

      mOwner->setObjectBox(newBounds);
   }

   mOwner->enableCollision();
   _updatePhysics();
}

void BoxColliderBehaviorInstance::processTick(const Move* move)
{
   //ProcessTick is where our collision testing begins!

	//First, we clear our old collision list. We do this, because if we're still colliding with something, 
	//it'll be added back in, but we don't want to risk colliding with something we've moved past
   mCollisionList.clear();

   //So, US checking collisions is irrelevent if we're not moving
   //All collision detection is active. Our Entity that is moving will detect collisions against other objects.
   //If we're not moving, we don't do checks outselves, but other moving entities may check and find they'll collide 
   //with us.
   //As such, the first thing we need to do if we're going to check collisions is to see if we have a valid interface
   //to a component that would give us a means to move. This would be the VelocityInterface.
   //If we don't have this, we're not moving, so we can just be done here and now.
   if(VelocityInterface *vI = mOwner->getComponent<VelocityInterface>())
   {
	   //Welp, looks like we have another component on our entity that has velocity, so grab that.
	   //We'll be passing it along so we can correctly estimate our position along the next tick.
      VectorF velocity = vI->getVelocity();

      //If we're not moving, or we never made any convex elements, we won't bother updating out working set
      if(mConvexList && mConvexList->getObject() && !velocity.isZero())
         updateWorkingCollisionSet((U32)-1);
   }
}

bool BoxColliderBehaviorInstance::checkCollisions( const F32 travelTime, Point3F *velocity, Point3F start )
{
   //Check collisions is inherited down from our interface. It's the main method for invoking collisions checks from other components

   //First, the obvious confirmation that we even have our convexes set up. If not, we can't be collided with, so bail out now.
   if(!mConvexList || !mConvexList->getObject())
      return false;

   //The way this is checked, is we build an estimate of where the end point of our owner will be based on the provided starting point, velocity and time
   Point3F end = start + *velocity * travelTime;

   //Build our vector of movement off that
   VectorF vector = end - start;

   //Now, as we're doing a new collision, any old collision information we had is no longer needed, so we can clear our collisionList in anticipation
   //of it being updated
   mCollisionList.clear();

   //We proceed to use our info above with our owner's properties to do an early out test against our working set we created earlier.
   //This is a simplified version of the checks we do later, so it's cheaper. If we can early out on the cheaper check now, we save a lot
   //of time and processing.
   //We only do the 'real' collision checks if we detect we might hit something after all when doing the early out.
   //See the checkEarlyOut function in the CollisionInterface for an explination on how it works.
   if (checkEarlyOut(start, *velocity, travelTime, mOwner->getObjBox(), mOwner->getScale(), mConvexList->getBoundingBox(), 0xFFFFFF, mConvexList->getWorkingList()))
      return false;

   //If we've made it this far, there's a very good chance we can actually collide with something in our working list. 
   //As such, go ahead and do our real collision update now
   bool collided = updateCollisions(travelTime, vector, *velocity);

   //Once that's been done, we proceed to handle our collision list, to notify any collidees.
   handleCollisionList(mCollisionList, *velocity);

   //This is only partially implemented.
   //The idea will be that you can define on a collider if it actually blocks when a collision occurs.
   //This would let colliders act as normal collision objects, or act as triggers.
   return collided;
}

void BoxColliderBehaviorInstance::_updatePhysics()
{
   SAFE_DELETE( (dynamic_cast<Entity*>(mOwner))->mPhysicsRep );

   if ( !PHYSICSMGR )
      return;

   PhysicsCollision *colShape = NULL;
   MatrixF offset( true );
   offset.setPosition( mOwner->getPosition() );
   colShape = PHYSICSMGR->createCollision();
   //colShape->addBox( mOwner->getObjBox().getExtents() * 0.5f * mOwner->getScale(), offset ); 
   colShape->addBox( mColliderScale, offset );

   if ( colShape )
   {
      PhysicsWorld *world = PHYSICSMGR->getWorld( mOwner->isServerObject() ? "server" : "client" );
      (dynamic_cast<Entity*>(mOwner))->mPhysicsRep = PHYSICSMGR->createBody();
      (dynamic_cast<Entity*>(mOwner))->mPhysicsRep->init( colShape, 0, 0, mOwner, world );
      (dynamic_cast<Entity*>(mOwner))->mPhysicsRep->setTransform( mOwner->getTransform() );
   }
}

bool BoxColliderBehaviorInstance::buildConvex(const Box3F& box, Convex* convex)
{
   S32 id = mOwner->getId();

   bool svr = isServerObject();

   // These should really come out of a pool
   if(!mConvexList)
      return false;

   mConvexList->collectGarbage();

   Box3F realBox = box;
   mOwner->getWorldToObj().mul(realBox);
   realBox.minExtents.convolveInverse(mOwner->getScale());
   realBox.maxExtents.convolveInverse(mOwner->getScale());

   if (realBox.isOverlapped(mOwner->getObjBox()) == false)
      return false;

   // Just return a box convex for the entire shape...
   Convex* cc = 0;
   CollisionWorkingList& wl = convex->getWorkingList();
   for (CollisionWorkingList* itr = wl.wLink.mNext; itr != &wl; itr = itr->wLink.mNext)
   {
      if (itr->mConvex->getType() == BoxConvexType &&
         itr->mConvex->getObject() == mOwner)
      {
         cc = itr->mConvex;
         break;
      }
   }
   if (cc)
      return true;

   // Create a new convex.
   BoxConvex* cp = new BoxConvex;
   mConvexList->registerObject(cp);
   convex->addToWorkingList(cp);
   cp->init(mOwner);

   mOwner->getObjBox().getCenter(&cp->mCenter);
   cp->mSize = mColliderScale;

   return true;
}

bool BoxColliderBehaviorInstance::buildPolyList(PolyListContext context, AbstractPolyList* polyList, const Box3F &, const SphereF &)
{
   Point3F pos;
   MatrixF ownerTransform = mOwner->getTransform();
   polyList->setTransform(&ownerTransform, mOwner->getScale());
   polyList->setObject(mOwner);
   polyList->addBox(Box3F(-mColliderScale/2, mColliderScale/2));
   return true;
}

bool BoxColliderBehaviorInstance::castRay(const Point3F &start, const Point3F &end, RayInfo* info)
{
   F32 st, et, fst = 0.0f, fet = 1.0f;
   Box3F objectBox = Box3F(-mColliderScale,mColliderScale);
   F32 *bmin = &objectBox.minExtents.x;
   F32 *bmax = &objectBox.maxExtents.x;
   F32 const *si = &start.x;
   F32 const *ei = &end.x;

   for ( U32 i = 0; i < 3; i++ )
   {
      if (*si < *ei) 
      {
         if ( *si > *bmax || *ei < *bmin )
            return false;
         F32 di = *ei - *si;
         st = ( *si < *bmin ) ? ( *bmin - *si ) / di : 0.0f;
         et = ( *ei > *bmax ) ? ( *bmax - *si ) / di : 1.0f;
      }
      else 
      {
         if ( *ei > *bmax || *si < *bmin )
            return false;
         F32 di = *ei - *si;
         st = ( *si > *bmax ) ? ( *bmax - *si ) / di : 0.0f;
         et = ( *ei < *bmin ) ? ( *bmin - *si ) / di : 1.0f;
      }
      if ( st > fst ) fst = st;
      if ( et < fet ) fet = et;
      if ( fet < fst )
         return false;
      bmin++; bmax++;
      si++; ei++;
   }

   info->normal = start - end;
   info->normal.normalizeSafe();
   mOwner->getTransform().mulV( info->normal );

   info->t = fst;
   info->object = mOwner;
   info->point.interpolate( start, end, fst );
   info->material = NULL;

   //do our callback
   //onRaycastCollision_callback( this, enter );
   return true;
}

bool BoxColliderBehaviorInstance::updateCollisions(F32 time, VectorF vector, VectorF velocity)
{
   Polyhedron cPolyhedron;
   cPolyhedron.buildBox(mOwner->getTransform(), mOwner->getObjBox(), true);
   ExtrudedPolyList extrudePoly;

   extrudePoly.extrude(cPolyhedron, velocity);
   extrudePoly.setVelocity(velocity);
   extrudePoly.setCollisionList(&mCollisionList);

   Box3F plistBox = mOwner->getObjBox();
   mOwner->getTransform().mul(plistBox);
   Point3F oldMin = plistBox.minExtents;
   Point3F oldMax = plistBox.maxExtents;
   plistBox.minExtents.setMin(oldMin + (velocity * time) - Point3F(0.1f, 0.1f, 0.1f));
   plistBox.maxExtents.setMax(oldMax + (velocity * time) + Point3F(0.1f, 0.1f, 0.1f));

   // Build list from convex states here...
   CollisionWorkingList& rList = mConvexList->getWorkingList();
   CollisionWorkingList* pList = rList.wLink.mNext;
   while (pList != &rList) 
   {
      Convex* pConvex = pList->mConvex;
      SceneObject* scObj = pConvex->getObject();
      if (pConvex->getObject()->getTypeMask() & CollisionMoveMask) 
      {
         Box3F convexBox = pConvex->getBoundingBox();
         if (plistBox.isOverlapped(convexBox))
         {
            pConvex->getPolyList(&extrudePoly);
         }
      }
      pList = pList->wLink.mNext;
   }

   if(mCollisionList.getCount() > 0)
   {
      return true;
   }

   return false;
}

void BoxColliderBehaviorInstance::updateWorkingCollisionSet(const U32 mask)
{
   //UpdateWorkingCollisionSet
   //What we do here, is check our estimated path along the current tick based on our
   //position, velocity, and tick time
   //From that information, we check other entities in our bin that we may potentially collide against

   // First, we need to adjust our velocity for possible acceleration.  It is assumed
   // that we will never accelerate more than 20 m/s for gravity, plus 10 m/s for
   // jetting, and an equivalent 10 m/s for jumping.  We also assume that the
   // working list is updated on a Tick basis, which means we only expand our
   // box by the possible movement in that tick.
   VectorF velocity = Point3F(0,0,0);

   if(VelocityInterface *vI = mOwner->getComponent<VelocityInterface>())
      velocity = vI->getVelocity();

   Point3F scaledVelocity = velocity * TickSec;
   F32 len    = scaledVelocity.len();
   F32 newLen = len + (10.0f * TickSec);

   // Check to see if it is actually necessary to construct the new working list,
   // or if we can use the cached version from the last query.  We use the x
   // component of the min member of the mWorkingQueryBox, which is lame, but
   // it works ok.
   bool updateSet = false;

   Box3F convexBox = mConvexList->getBoundingBox(mOwner->getTransform(), mOwner->getScale());
   F32 l = (newLen * 1.1f) + 0.1f;  // from Convex::updateWorkingList
   const Point3F  lPoint( l, l, l );
   convexBox.minExtents -= lPoint;
   convexBox.maxExtents += lPoint;

   // Check containment
   if (mWorkingQueryBox.minExtents.x != -1e9f)
   {
      if (mWorkingQueryBox.isContained(convexBox) == false)
         // Needed region is outside the cached region.  Update it.
            updateSet = true;
   }
   else
   {
      // Must update
      updateSet = true;
   }
   // Actually perform the query, if necessary
   if (updateSet == true) {
      const Point3F  twolPoint( 2.0f * l, 2.0f * l, 2.0f * l );
      mWorkingQueryBox = convexBox;
      mWorkingQueryBox.minExtents -= twolPoint;
      mWorkingQueryBox.maxExtents += twolPoint;

      //So first we scale our workingQueryBox to catch everything we may potentially collide with this tick
	   //Then disable our entity owner so we don't run into it like an idiot
      mOwner->disableCollision();

      //Now we officially update our working list.
      //What this basically does is find any objects our working box(which, again, is our theoretical collision space)
      //and has them call buildConvex().
      //This makes them update their convex information so we can check if we actually collide with it for real)
      mConvexList->updateWorkingList(mWorkingQueryBox, U32(-1));

	  //Once we've updated, re-enable our entity owner's collision so that other things can collide with us if needed
      mOwner->enableCollision();
   }
}

void BoxColliderBehaviorInstance::prepRenderImage( SceneRenderState *state )
{
   if(sRenderBoxColliders)
   {
      if(mConvexList == NULL)
         return;

      ObjectRenderInst *ri = state->getRenderPass()->allocInst<ObjectRenderInst>();
      ri->renderDelegate.bind( this, &BoxColliderBehaviorInstance::renderConvex );
      ri->objectIndex = -1;
      ri->type = RenderPassManager::RIT_Editor;
      state->getRenderPass()->addInst( ri );
   }
}

void BoxColliderBehaviorInstance::renderConvex( ObjectRenderInst *ri, SceneRenderState *state, BaseMatInstance *overrideMat )
{
   GFX->enterDebugEvent( ColorI(255,0,255), "BoxColliderBehaviorInstance_renderConvex" );

   GFXStateBlockDesc desc;
   desc.setZReadWrite( true, false );
   desc.setBlend( true );
   desc.setCullMode( GFXCullNone );
   desc.fillMode = GFXFillWireframe;

   GFXStateBlockRef sb = GFX->createStateBlock( desc );
   GFX->setStateBlock( sb );

   ConcretePolyList polyList;

   MatrixF ownerTransform = mOwner->getTransform();

   Box3F convexBox = mConvexList->getBoundingBox(mOwner->getTransform(), mOwner->getScale());
   //Box3F convexBox = Box3F(-mColliderScale/2, mColliderScale/2);
   //ownerTransform.mul(convexBox);

   polyList.addBox(convexBox);

   PrimBuild::color3i( 255, 0, 255 );

   ConcretePolyList::Poly *p;
   Point3F *pnt;

   for ( p = polyList.mPolyList.begin(); p < polyList.mPolyList.end(); p++ )
   {
      PrimBuild::begin( GFXLineStrip, p->vertexCount + 1 );      

      for ( U32 i = 0; i < p->vertexCount; i++ )
      {
         pnt = &polyList.mVertexList[polyList.mIndexList[p->vertexStart + i]];
         PrimBuild::vertex3fv( pnt );
      }

      pnt = &polyList.mVertexList[polyList.mIndexList[p->vertexStart]];
      PrimBuild::vertex3fv( pnt );

      PrimBuild::end();
   }  

   GFX->leaveDebugEvent();
}