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

#include "T3D/components/collision/shapeCollisionComponent.h"
#include "T3D/components/collision/shapeCollisionComponent_ScriptBinding.h"
#include "T3D/components/physics/physicsComponent.h"
#include "console/consoleTypes.h"
#include "core/util/safeDelete.h"
#include "core/resourceManager.h"
#include "console/consoleTypes.h"
#include "console/consoleObject.h"
#include "core/stream/bitStream.h"
#include "scene/sceneRenderState.h"
#include "gfx/gfxTransformSaver.h"
#include "gfx/gfxDrawUtil.h"
#include "console/engineAPI.h"
#include "T3D/physics/physicsPlugin.h"
#include "T3D/physics/physicsBody.h"
#include "T3D/physics/physicsCollision.h"
#include "T3D/gameBase/gameConnection.h"
#include "collision/extrudedPolyList.h"
#include "math/mathIO.h"
#include "gfx/sim/debugDraw.h"  
#include "collision/concretePolyList.h"

#include "T3D/trigger.h"
#include "opcode/Opcode.h"
#include "opcode/Ice/IceAABB.h"
#include "opcode/Ice/IcePoint.h"
#include "opcode/OPC_AABBTree.h"
#include "opcode/OPC_AABBCollider.h"

#include "math/mathUtils.h"
#include "materials/baseMatInstance.h"
#include "collision/vertexPolyList.h"

extern bool gEditingMission;

static bool sRenderColliders = false;

//Docs
ConsoleDocClass(ShapeCollisionComponent,
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
   "new ShapeCollisionComponentInstance()\n"
   "{\n"
   "   template = ShapeCollisionComponentTemplate;\n"
   "   colliderSize = \"1 1 2\";\n"
   "   blockColldingObject = \"1\";\n"
   "};\n"
   "@endtsexample\n"

   "@see SimplePhysicsBehavior\n"
   "@ingroup Collision\n"
   "@ingroup Components\n"
   );
//Docs

/////////////////////////////////////////////////////////////////////////
ImplementEnumType(CollisionMeshMeshType,
   "Type of mesh data available in a shape.\n"
   "@ingroup gameObjects")
{ ShapeCollisionComponent::None, "None", "No mesh data." },
{ ShapeCollisionComponent::Bounds, "Bounds", "Bounding box of the shape." },
{ ShapeCollisionComponent::CollisionMesh, "Collision Mesh", "Specifically desingated \"collision\" meshes." },
{ ShapeCollisionComponent::VisibleMesh, "Visible Mesh", "Rendered mesh polygons." },
EndImplementEnumType;

//
ShapeCollisionComponent::ShapeCollisionComponent() : CollisionComponent()
{
   mFriendlyName = "Shape Collision Component";

   mFriendlyName = "Physics Component";
   mComponentType = "Physics";

   mDescription = getDescriptionText("A stub component class that physics components should inherit from.");

   mOwnerRenderInterface = NULL;
   mOwnerPhysicsComp = NULL;

   mBlockColliding = true;

   mCollisionType = CollisionMesh;
   mLOSType = CollisionMesh;
   mDecalType = CollisionMesh;

   colisionMeshPrefix = StringTable->insert("Collision");

   CollisionMoveMask = (TerrainObjectType | PlayerObjectType |
      StaticShapeObjectType | VehicleObjectType |
      VehicleBlockerObjectType | DynamicShapeObjectType | StaticObjectType | EntityObjectType | TriggerObjectType);

   mAnimated = false;
}

ShapeCollisionComponent::~ShapeCollisionComponent()
{
   for (S32 i = 0; i < mFields.size(); ++i)
   {
      ComponentField &field = mFields[i];
      SAFE_DELETE_ARRAY(field.mFieldDescription);
   }

   SAFE_DELETE_ARRAY(mDescription);
}

IMPLEMENT_CONOBJECT(ShapeCollisionComponent);

void ShapeCollisionComponent::onComponentAdd()
{
   Parent::onComponentAdd();

   RenderComponentInterface *renderInterface = mOwner->getComponent<RenderComponentInterface>();
   if (renderInterface)
   {
      renderInterface->onShapeInstanceChanged.notify(this, &ShapeCollisionComponent::targetShapeChanged);
      mOwnerRenderInterface = renderInterface;
   }

   //physicsInterface
   PhysicsComponent *physicsComp = mOwner->getComponent<PhysicsComponent>();
   if (physicsComp)
   {
      mOwnerPhysicsComp = physicsComp;
   }
   else
   {
      if(PHYSICSMGR)
         mPhysicsRep = PHYSICSMGR->createBody();
   }

   prepCollision();
}

void ShapeCollisionComponent::onComponentRemove()
{
   SAFE_DELETE(mPhysicsRep);
   SAFE_DELETE(mConvexList);

   mOwnerPhysicsComp = nullptr;

   Parent::onComponentRemove();
}

void ShapeCollisionComponent::componentAddedToOwner(Component *comp)
{
   if (comp->getId() == getId())
      return;

   //test if this is a shape component!
   RenderComponentInterface *renderInterface = dynamic_cast<RenderComponentInterface*>(comp);
   if (renderInterface)
   {
      renderInterface->onShapeInstanceChanged.notify(this, &ShapeCollisionComponent::targetShapeChanged);
      mOwnerRenderInterface = renderInterface;
      prepCollision();
   }

   PhysicsComponent *physicsComp = dynamic_cast<PhysicsComponent*>(comp);
   if (physicsComp)
   {
      if (mPhysicsRep)
         SAFE_DELETE(mPhysicsRep);

      mOwnerPhysicsComp = physicsComp;

      prepCollision();
   }
}

void ShapeCollisionComponent::componentRemovedFromOwner(Component *comp)
{
   if (comp->getId() == getId()) //?????????
      return;

   //test if this is a shape component!
   RenderComponentInterface *renderInterface = dynamic_cast<RenderComponentInterface*>(comp);
   if (renderInterface)
   {
      renderInterface->onShapeInstanceChanged.remove(this, &ShapeCollisionComponent::targetShapeChanged);
      mOwnerRenderInterface = NULL;
      prepCollision();
   }

   //physicsInterface
   PhysicsComponent *physicsComp = dynamic_cast<PhysicsComponent*>(comp);
   if (physicsComp)
   {
      mPhysicsRep = PHYSICSMGR->createBody();

      mOwnerPhysicsComp = nullptr;

      prepCollision();
   }
}

void ShapeCollisionComponent::checkDependencies()
{
}

void ShapeCollisionComponent::initPersistFields()
{
   Parent::initPersistFields();

   addGroup("Collision");

      addField("CollisionType", TypeCollisionMeshMeshType, Offset(mCollisionType, ShapeCollisionComponent),
         "The type of mesh data to use for collision queries.");

      addField("LineOfSightType", TypeCollisionMeshMeshType, Offset(mLOSType, ShapeCollisionComponent),
         "The type of mesh data to use for collision queries.");

      addField("DecalType", TypeCollisionMeshMeshType, Offset(mDecalType, ShapeCollisionComponent),
         "The type of mesh data to use for collision queries.");

      addField("CollisionMeshPrefix", TypeString, Offset(colisionMeshPrefix, ShapeCollisionComponent),
         "The type of mesh data to use for collision queries.");

      addField("BlockCollisions", TypeBool, Offset(mBlockColliding, ShapeCollisionComponent), "");

   endGroup("Collision");
}

void ShapeCollisionComponent::inspectPostApply()
{
   // Apply any transformations set in the editor
   Parent::inspectPostApply();

   if (isServerObject())
   {
      setMaskBits(ColliderMask);
      prepCollision();
   }
}

U32 ShapeCollisionComponent::packUpdate(NetConnection *con, U32 mask, BitStream *stream)
{
   U32 retMask = Parent::packUpdate(con, mask, stream);

   if (stream->writeFlag(mask & (ColliderMask | InitialUpdateMask)))
   {
      stream->write((U32)mCollisionType);
      stream->writeString(colisionMeshPrefix);
   }

   return retMask;
}

void ShapeCollisionComponent::unpackUpdate(NetConnection *con, BitStream *stream)
{
   Parent::unpackUpdate(con, stream);

   if (stream->readFlag()) // UpdateMask
   {
      U32 collisionType = CollisionMesh;

      stream->read(&collisionType);

      // Handle it if we have changed CollisionType's
      if ((MeshType)collisionType != mCollisionType)
      {
         mCollisionType = (MeshType)collisionType;

         prepCollision();
      }

      char readBuffer[1024];

      stream->readString(readBuffer);
      colisionMeshPrefix = StringTable->insert(readBuffer);
   }
}

void ShapeCollisionComponent::ownerTransformSet(MatrixF *mat)
{
   if (mPhysicsRep)
      mPhysicsRep->setTransform(mOwner->getTransform());
}

//Setup
void ShapeCollisionComponent::targetShapeChanged(RenderComponentInterface* instanceInterface)
{
   prepCollision();
}

void ShapeCollisionComponent::prepCollision()
{
   if (!mOwner)
      return;

   // Let the client know that the collision was updated
   setMaskBits(ColliderMask);

   mOwner->disableCollision();

   if (mConvexList != NULL)
      mConvexList->nukeList();

   if (mCollisionType == None)
      return;

   if (!PHYSICSMGR)
   {
      //stock physics
      if (mCollisionType == Bounds)
      {
         BoxConvex* bC = new BoxConvex();
         bC->init(mOwner);
         bC->mSize = mOwner->getScale();
         mOwner->getObjBox().getCenter(&bC->mCenter);
         mConvexList = bC;

         Box3F colliderBounds = bC->getBoundingBox(mOwner->getTransform(), mOwner->getScale());

         Box3F ownerBounds = mOwner->getWorldBox();

         if (!ownerBounds.isContained(colliderBounds))
         {
            Box3F newBounds = ownerBounds;

            newBounds.extend(colliderBounds.minExtents);
            newBounds.extend(colliderBounds.maxExtents);

            mOwner->setObjectBox(newBounds);
         }
      }
      else if (mCollisionType == CollisionMesh)
      {
         if (mOwnerRenderInterface == nullptr)
            return;


      }
      else if (mCollisionType == VisibleMesh)
      {
         if (mOwnerRenderInterface == nullptr)
            return;

         // Cleanup any old collision data
         mCollisionDetails.clear();

         /*if (mConvexList != NULL)
            mConvexList->nukeList();

         Vector<S32> losDetails;

         mOwnerRenderInterface->getShape()->findColDetails(true, &mCollisionDetails, &losDetails);

         if (!mCollisionDetails.empty())
         {
            mOwner->enableCollision();

            mConvexList = new MeshColliderPolysoupConvex;

            Convex *c = new Convex();
            for (U32 i = 0; i < mCollisionDetails.size(); i++)
               buildConvexOpcode(mOwnerRenderInterface->getShapeInstance(), mCollisionDetails[i], mOwner->getWorldBox(), c, mConvexList);
         }
         else
            mOwner->disableCollision();*/
      }
   }
   else
   {
      //Physics API
      PhysicsCollision *colShape = NULL;

      if (mCollisionType == Bounds)
      {
         MatrixF offset(true);

         if (mOwnerRenderInterface && mOwnerRenderInterface->getShape())
            offset.setPosition(mOwnerRenderInterface->getShape()->center);

         colShape = PHYSICSMGR->createCollision();
         colShape->addBox(mOwner->getObjBox().getExtents() * 0.5f * mOwner->getScale(), offset);
      }
      else if (mCollisionType == CollisionMesh || (mCollisionType == VisibleMesh /*&& !mOwner->getComponent<AnimatedMesh>()*/))
      {
         colShape = buildColShapes();
      }

      if (colShape)
      {
         mPhysicsWorld = PHYSICSMGR->getWorld(isServerObject() ? "server" : "client");

         if (mPhysicsRep)
         {
            if (mBlockColliding)
               mPhysicsRep->init(colShape, 0, 0, mOwner, mPhysicsWorld);
            else
               mPhysicsRep->init(colShape, 0, PhysicsBody::BF_TRIGGER, mOwner, mPhysicsWorld);

            mPhysicsRep->setTransform(mOwner->getTransform());
         }
      }

      mOwner->enableCollision();

      onCollisionChanged.trigger(colShape);
   }
}

//Update
void ShapeCollisionComponent::processTick()
{
   if (!isActive())
      return;

   //ProcessTick is where our collision testing begins!
   if (!PHYSICSMGR)
   {
      //If we're using stock physics, do our function call
      if (mOwnerPhysicsComp)
      {
         Point3F velocity = mOwnerPhysicsComp->getVelocity();

         checkCollisions(TickSec, &velocity, mOwner->getPosition());

         mOwnerPhysicsComp->setVelocity(velocity);
      }
   }

   //callback if we have a persisting contact
   if (mContactInfo.contactObject)
   {
      if (mContactInfo.contactTimer > 0)
      {
         if (isMethod("updateContact"))
            Con::executef(this, "updateContact");

         if (mOwner->isMethod("updateContact"))
            Con::executef(mOwner, "updateContact");
      }

      ++mContactInfo.contactTimer;
   }
   else if (mContactInfo.contactTimer != 0)
      mContactInfo.clear();
}

void ShapeCollisionComponent::updatePhysics()
{
   
}

bool ShapeCollisionComponent::checkCollisions(const F32 travelTime, Point3F *velocity, Point3F start)
{
   //Check collisions is inherited down from our interface. It's the main method for invoking collisions checks from other components

   //First, the obvious confirmation that we even have our convexes set up. If not, we can't be collided with, so bail out now.
   if (!mConvexList || !mConvexList->getObject())
      return false;

   //The way this is checked, is we build an estimate of where the end point of our owner will be based on the provided starting point, velocity and time
   Point3F end = start + *velocity * travelTime;

   //Build our vector of movement off that
   VectorF vector = end - start;

   //Now, as we're doing a new collision, any old collision information we had is no longer needed, so we can clear our collisionList in anticipation
   //of it being updated
   //mCollisionList.clear();

   U32 sCollisionMoveMask = TerrainObjectType |
      WaterObjectType |
      PlayerObjectType |
      StaticShapeObjectType |
      VehicleObjectType |
      PhysicalZoneObjectType |
      TriggerObjectType;

   //We proceed to use our info above with our owner's properties to do an early out test against our working set we created earlier.
   //This is a simplified version of the checks we do later, so it's cheaper. If we can early out on the cheaper check now, we save a lot
   //of time and processing.
   //We only do the 'real' collision checks if we detect we might hit something after all when doing the early out.
   //See the checkEarlyOut function in the CollisionInterface for an explination on how it works.
   if (checkEarlyOut(start, *velocity, travelTime, mOwner->getObjBox(), mOwner->getScale(), mConvexList->getBoundingBox(), 0xFFFFFF, mConvexList->getWorkingList()))
      //if (checkEarlyOut(start, *velocity, travelTime, mOwner->getObjBox(), mOwner->getScale(), mConvexList->getBoundingBox(), sCollisionMoveMask, mConvexList->getWorkingList()))
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

bool ShapeCollisionComponent::updateCollisions(F32 time, VectorF vector, VectorF velocity)
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

      if (dynamic_cast<Entity*>(scObj))
         bool der = true;

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

   if (mCollisionList.getCount() > 0)
   {
      for (U32 i = 0; i < mCollisionList.getCount(); i++)
      {
         if (dynamic_cast<Trigger*>(mCollisionList[i].object))
            return false;
      }

      return true;
   }

   return false;
}

void ShapeCollisionComponent::updateWorkingCollisionSet(const U32 mask)
{
   if (mConvexList == nullptr)
      return;

   //UpdateWorkingCollisionSet
   //What we do here, is check our estimated path along the current tick based on our
   //position, velocity, and tick time
   //From that information, we check other entities in our bin that we may potentially collide against

   // First, we need to adjust our velocity for possible acceleration.  It is assumed
   // that we will never accelerate more than 20 m/s for gravity, plus 10 m/s for
   // jetting, and an equivalent 10 m/s for jumping.  We also assume that the
   // working list is updated on a Tick basis, which means we only expand our
   // box by the possible movement in that tick.
   VectorF velocity = Point3F(0, 0, 0);

   if (!PHYSICSMGR)
   {
      if (mOwnerPhysicsComp)
         velocity = mOwnerPhysicsComp->getVelocity();
   }

   Point3F scaledVelocity = velocity * TickSec;
   F32 len = scaledVelocity.len();
   F32 newLen = len + (10.0f * TickSec);

   // Check to see if it is actually necessary to construct the new working list,
   // or if we can use the cached version from the last query.  We use the x
   // component of the min member of the mWorkingQueryBox, which is lame, but
   // it works ok.
   bool updateSet = false;

   Box3F convexBox = mConvexList->getBoundingBox(mOwner->getTransform(), mOwner->getScale());
   F32 l = (newLen * 1.1f) + 0.1f;  // from Convex::updateWorkingList
   const Point3F  lPoint(l, l, l);
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
      const Point3F  twolPoint(2.0f * l, 2.0f * l, 2.0f * l);
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

PhysicsCollision* ShapeCollisionComponent::getCollisionData()
{
   if ((!PHYSICSMGR || mCollisionType == None) || mOwnerRenderInterface == NULL)
      return NULL;

   PhysicsCollision *colShape = NULL;
   if (mCollisionType == Bounds)
   {
      MatrixF offset(true);
      offset.setPosition(mOwnerRenderInterface->getShape()->center);
      colShape = PHYSICSMGR->createCollision();
      colShape->addBox(mOwner->getObjBox().getExtents() * 0.5f * mOwner->getScale(), offset);
   }
   else if (mCollisionType == CollisionMesh || (mCollisionType == VisibleMesh/* && !mOwner->getComponent<AnimatedMesh>()*/))
   {
      colShape = buildColShapes();
      //colShape = mOwnerShapeInstance->getShape()->buildColShape(mCollisionType == VisibleMesh, mOwner->getScale());
   }
   /*else if (mCollisionType == VisibleMesh && !mOwner->getComponent<AnimatedMesh>())
   {
   //We don't have support for visible mesh collisions with animated meshes currently in the physics abstraction layer
   //so we don't generate anything if we're set to use a visible mesh but have an animated mesh component.
   colShape = mOwnerShapeInstance->getShape()->buildColShape(mCollisionType == VisibleMesh, mOwner->getScale());
   }*/
   else if (mCollisionType == VisibleMesh/* && mOwner->getComponent<AnimatedMesh>()*/)
   {
      Con::printf("ShapeCollisionComponent::updatePhysics: Cannot use visible mesh collisions with an animated mesh!");
   }

   return colShape;
}

bool ShapeCollisionComponent::castRay(const Point3F &start, const Point3F &end, RayInfo* info)
{
   if (!mCollisionType == None)
   {
      if (mPhysicsWorld)
      {
         return mPhysicsWorld->castRay(start, end, info, Point3F::Zero);
      }
   }

   return false;
}

PhysicsCollision* ShapeCollisionComponent::buildColShapes()
{
   PROFILE_SCOPE(ShapeCollisionComponent_buildColShapes);

   PhysicsCollision *colShape = NULL;
   U32 surfaceKey = 0;

   TSShape* shape = mOwnerRenderInterface->getShape();

   if (mCollisionType == VisibleMesh)
   {
      // Here we build triangle collision meshes from the
      // visible detail levels.

      // A negative subshape on the detail means we don't have geometry.
      const TSShape::Detail &detail = shape->details[0];
      if (detail.subShapeNum < 0)
         return NULL;

      // We don't try to optimize the triangles we're given
      // and assume the art was created properly for collision.
      ConcretePolyList polyList;
      polyList.setTransform(&MatrixF::Identity, mOwner->getScale());

      // Create the collision meshes.
      S32 start = shape->subShapeFirstObject[detail.subShapeNum];
      S32 end = start + shape->subShapeNumObjects[detail.subShapeNum];
      for (S32 o = start; o < end; o++)
      {
         const TSShape::Object &object = shape->objects[o];
         if (detail.objectDetailNum >= object.numMeshes)
            continue;

         // No mesh or no verts.... nothing to do.
         TSMesh *mesh = shape->meshes[object.startMeshIndex + detail.objectDetailNum];
         if (!mesh || mesh->mNumVerts == 0)
            continue;

         // Gather the mesh triangles.
         polyList.clear();
         mesh->buildPolyList(0, &polyList, surfaceKey, NULL);

         // Create the collision shape if we haven't already.
         if (!colShape)
            colShape = PHYSICSMGR->createCollision();

         // Get the object space mesh transform.
         MatrixF localXfm;
         shape->getNodeWorldTransform(object.nodeIndex, &localXfm);

         colShape->addTriangleMesh(polyList.mVertexList.address(),
            polyList.mVertexList.size(),
            polyList.mIndexList.address(),
            polyList.mIndexList.size() / 3,
            localXfm);
      }

      // Return what we built... if anything.
      return colShape;
   }
   else if (mCollisionType == CollisionMesh)
   {

      // Scan out the collision hulls...
      //
      // TODO: We need to support LOS collision for physics.
      //
      for (U32 i = 0; i < shape->details.size(); i++)
      {
         const TSShape::Detail &detail = shape->details[i];
         const String &name = shape->names[detail.nameIndex];

         // Is this a valid collision detail.
         if (!dStrStartsWith(name, colisionMeshPrefix) || detail.subShapeNum < 0)
            continue;

         // Now go thru the meshes for this detail.
         S32 start = shape->subShapeFirstObject[detail.subShapeNum];
         S32 end = start + shape->subShapeNumObjects[detail.subShapeNum];
         if (start >= end)
            continue;

         for (S32 o = start; o < end; o++)
         {
            const TSShape::Object &object = shape->objects[o];
            const String &meshName = shape->names[object.nameIndex];

            if (object.numMeshes <= detail.objectDetailNum)
               continue;

            // No mesh, a flat bounds, or no verts.... nothing to do.
            TSMesh *mesh = shape->meshes[object.startMeshIndex + detail.objectDetailNum];
            if (!mesh || mesh->getBounds().isEmpty() || mesh->mNumVerts == 0)
               continue;

            // We need the default mesh transform.
            MatrixF localXfm;
            shape->getNodeWorldTransform(object.nodeIndex, &localXfm);

            // We have some sort of collision shape... so allocate it.
            if (!colShape)
               colShape = PHYSICSMGR->createCollision();

            // Any other mesh name we assume as a generic convex hull.
            //
            // Collect the verts using the vertex polylist which will 
            // filter out duplicates.  This is importaint as the convex
            // generators can sometimes fail with duplicate verts.
            //
            VertexPolyList polyList;
            MatrixF meshMat(localXfm);

            Point3F t = meshMat.getPosition();
            t.convolve(mOwner->getScale());
            meshMat.setPosition(t);

            polyList.setTransform(&MatrixF::Identity, mOwner->getScale());
            mesh->buildPolyList(0, &polyList, surfaceKey, NULL);
            colShape->addConvex(polyList.getVertexList().address(),
               polyList.getVertexList().size(),
               meshMat);
         } // objects
      } // details
   }

   return colShape;
}