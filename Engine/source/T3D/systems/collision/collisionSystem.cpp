#include "T3D/systems/collision/collisionSystem.h"
#include "gfx/gfxTransformSaver.h"
#include "lighting/lightQuery.h"

#include "renderInstance/renderPassManager.h"
#include "materials/materialManager.h"
#include "materials/baseMatInstance.h"

#include "collision/extrudedPolyList.h"
#include "T3D/trigger.h"

Chunker<CollisionTimeout> sTimeoutChunker;
CollisionTimeout* CollisionSystemInterface::sFreeTimeoutList = 0;

bool CollisionSystem::checkCollisions(CollisionSystemInterface* colInterface, const F32 travelTime, Point3F *velocity, Point3F start)
{
	//Check collisions is inherited down from our interface. It's the main method for invoking collisions checks from other components

	//First, the obvious confirmation that we even have our convexes set up. If not, we can't be collided with, so bail out now.
	if (!colInterface->mConvexList || !colInterface->mConvexList->getObject())
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
	//See the checkEarlyOut function in the CollisionSystem for an explination on how it works.
	if (checkEarlyOut(start, *velocity, travelTime, 
      colInterface->mOwner->getOwner()->getObjBox(), colInterface->mOwner->getOwner()->getScale(), 
      colInterface->mConvexList->getBoundingBox(), 0xFFFFFF, colInterface->mConvexList->getWorkingList()))
		//if (checkEarlyOut(start, *velocity, travelTime, mOwner->getObjBox(), mOwner->getScale(), mConvexList->getBoundingBox(), sCollisionMoveMask, mConvexList->getWorkingList()))
		return false;

	//If we've made it this far, there's a very good chance we can actually collide with something in our working list. 
	//As such, go ahead and do our real collision update now
	bool collided = updateCollisions(colInterface, travelTime, *velocity);

	//Once that's been done, we proceed to handle our collision list, to notify any collidees.
	handleCollisionList(colInterface, *velocity);

	//This is only partially implemented.
	//The idea will be that you can define on a collider if it actually blocks when a collision occurs.
	//This would let colliders act as normal collision objects, or act as triggers.
	return collided;
}

void CollisionSystem::_updatePhysics()
{
	SAFE_DELETE((dynamic_cast<Entity*>(mOwner))->mPhysicsRep);

	if (!PHYSICSMGR)
		return;

	PhysicsCollision *colShape = NULL;
	MatrixF offset(true);
	offset.setPosition(mOwner->getPosition());
	colShape = PHYSICSMGR->createCollision();
	//colShape->addBox( mOwner->getObjBox().getExtents() * 0.5f * mOwner->getScale(), offset ); 
	colShape->addBox(mColliderScale, offset);

	if (colShape)
	{
		PhysicsWorld *world = PHYSICSMGR->getWorld(mOwner->isServerObject() ? "server" : "client");
		(dynamic_cast<Entity*>(mOwner))->mPhysicsRep = PHYSICSMGR->createBody();
		(dynamic_cast<Entity*>(mOwner))->mPhysicsRep->init(colShape, 0, 0, mOwner, world);
		(dynamic_cast<Entity*>(mOwner))->mPhysicsRep->setTransform(mOwner->getTransform());
	}
}

void CollisionSystem::buildConvex(const Box3F& box, Convex* convex)
{
	S32 id = mOwner->getId();

	bool svr = isServerObject();

	// These should really come out of a pool
	if (!mConvexList)
		return;

	mConvexList->collectGarbage();

	Box3F realBox = box;
	mOwner->getWorldToObj().mul(realBox);
	realBox.minExtents.convolveInverse(mOwner->getScale());
	realBox.maxExtents.convolveInverse(mOwner->getScale());

	if (realBox.isOverlapped(mOwner->getObjBox()) == false)
		return;

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
		return;

	// Create a new convex.
	BoxConvex* cp = new BoxConvex;
	mConvexList->registerObject(cp);
	convex->addToWorkingList(cp);
	cp->init(mOwner);

	mOwner->getObjBox().getCenter(&cp->mCenter);
	cp->mSize = mColliderScale;

	return;
}

bool CollisionSystem::buildPolyList(PolyListContext context, AbstractPolyList* polyList, const Box3F &, const SphereF &)
{
	Point3F pos;
	MatrixF ownerTransform = mOwner->getTransform();
	polyList->setTransform(&ownerTransform, mOwner->getScale());
	polyList->setObject(mOwner);
	polyList->addBox(Box3F(-mColliderScale / 2, mColliderScale / 2));
	return true;
}

bool CollisionSystem::castRay(const Point3F &start, const Point3F &end, RayInfo* info)
{
	F32 st, et, fst = 0.0f, fet = 1.0f;
	Box3F objectBox = Box3F(-mColliderScale, mColliderScale);
	F32 *bmin = &objectBox.minExtents.x;
	F32 *bmax = &objectBox.maxExtents.x;
	F32 const *si = &start.x;
	F32 const *ei = &end.x;

	for (U32 i = 0; i < 3; i++)
	{
		if (*si < *ei)
		{
			if (*si > *bmax || *ei < *bmin)
				return false;
			F32 di = *ei - *si;
			st = (*si < *bmin) ? (*bmin - *si) / di : 0.0f;
			et = (*ei > *bmax) ? (*bmax - *si) / di : 1.0f;
		}
		else
		{
			if (*ei > *bmax || *si < *bmin)
				return false;
			F32 di = *ei - *si;
			st = (*si > *bmax) ? (*bmax - *si) / di : 0.0f;
			et = (*ei < *bmin) ? (*bmin - *si) / di : 1.0f;
		}
		if (st > fst) fst = st;
		if (et < fet) fet = et;
		if (fet < fst)
			return false;
		bmin++; bmax++;
		si++; ei++;
	}

	info->normal = start - end;
	info->normal.normalizeSafe();
	mOwner->getTransform().mulV(info->normal);

	info->t = fst;
	info->object = mOwner;
	info->point.interpolate(start, end, fst);
	info->material = NULL;

	//do our callback
	//onRaycastCollision_callback( this, enter );
	return true;
}

bool CollisionSystem::updateCollisions(CollisionSystemInterface* colInterface, F32 time, VectorF velocity)
{
	Polyhedron cPolyhedron;
   Entity* ownerEntity = colInterface->mOwner->getOwner();
	cPolyhedron.buildBox(ownerEntity->getTransform(), ownerEntity->getObjBox(), true);
	ExtrudedPolyList extrudePoly;

	extrudePoly.extrude(cPolyhedron, velocity);
	extrudePoly.setVelocity(velocity);
	extrudePoly.setCollisionList(&colInterface->mCollisionList);

	Box3F plistBox = ownerEntity->getObjBox();
   ownerEntity->getTransform().mul(plistBox);
	Point3F oldMin = plistBox.minExtents;
	Point3F oldMax = plistBox.maxExtents;
	plistBox.minExtents.setMin(oldMin + (velocity * time) - Point3F(0.1f, 0.1f, 0.1f));
	plistBox.maxExtents.setMax(oldMax + (velocity * time) + Point3F(0.1f, 0.1f, 0.1f));

	// Build list from convex states here...
	CollisionWorkingList& rList = colInterface->mConvexList->getWorkingList();
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

	if (colInterface->mCollisionList.getCount() > 0)
	{
		for (U32 i = 0; i < colInterface->mCollisionList.getCount(); i++)
		{
			if (dynamic_cast<Trigger*>(colInterface->mCollisionList[i].object))
				return false;
		}

		return true;
	}

	return false;
}

void CollisionSystem::updateWorkingCollisionSet(CollisionSystemInterface* colInterface, const VectorF velocity, const U32 mask)
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
	Point3F scaledVelocity = velocity * TickSec;
	F32 len = scaledVelocity.len();
	F32 newLen = len + (10.0f * TickSec);

	// Check to see if it is actually necessary to construct the new working list,
	// or if we can use the cached version from the last query.  We use the x
	// component of the min member of the mWorkingQueryBox, which is lame, but
	// it works ok.
	bool updateSet = false;

	Box3F convexBox = colInterface->mConvexList->getBoundingBox(colInterface->mOwner->getOwner()->getTransform(), colInterface->mOwner->getOwner()->getScale());
	F32 l = (newLen * 1.1f) + 0.1f;  // from Convex::updateWorkingList
	const Point3F  lPoint(l, l, l);
	convexBox.minExtents -= lPoint;
	convexBox.maxExtents += lPoint;

	// Check containment
	if (colInterface->mWorkingQueryBox.minExtents.x != -1e9f)
	{
		if (colInterface->mWorkingQueryBox.isContained(convexBox) == false)
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
      colInterface->mWorkingQueryBox = convexBox;
      colInterface->mWorkingQueryBox.minExtents -= twolPoint;
      colInterface->mWorkingQueryBox.maxExtents += twolPoint;

		//So first we scale our workingQueryBox to catch everything we may potentially collide with this tick
		//Then disable our entity owner so we don't run into it like an idiot
      colInterface->mOwner->getOwner()->disableCollision();

		//Now we officially update our working list.
		//What this basically does is find any objects our working box(which, again, is our theoretical collision space)
		//and has them call buildConvex().
		//This makes them update their convex information so we can check if we actually collide with it for real)
      colInterface->mConvexList->updateWorkingList(colInterface->mWorkingQueryBox, U32(-1));

		//Once we've updated, re-enable our entity owner's collision so that other things can collide with us if needed
      colInterface->mOwner->getOwner()->enableCollision();
	}
}

//void CollisionSystem::renderConvex(ObjectRenderInst *ri, SceneRenderState *state, BaseMatInstance *overrideMat)
//{
	/*GFX->enterDebugEvent(ColorI(255, 0, 255), "BoxColliderBehaviorInstance_renderConvex");

	GFXStateBlockDesc desc;
	desc.setZReadWrite(true, false);
	desc.setBlend(true);
	desc.setCullMode(GFXCullNone);
	desc.fillMode = GFXFillWireframe;

	GFXStateBlockRef sb = GFX->createStateBlock(desc);
	GFX->setStateBlock(sb);

	ConcretePolyList polyList;

	MatrixF ownerTransform = mOwner->getTransform();

	Box3F convexBox = mConvexList->getBoundingBox(mOwner->getTransform(), mOwner->getScale());
	//Box3F convexBox = Box3F(-mColliderScale/2, mColliderScale/2);
	//ownerTransform.mul(convexBox);

	polyList.addBox(convexBox);

	PrimBuild::color3i(255, 0, 255);

	ConcretePolyList::Poly *p;
	Point3F *pnt;

	for (p = polyList.mPolyList.begin(); p < polyList.mPolyList.end(); p++)
	{
		PrimBuild::begin(GFXLineStrip, p->vertexCount + 1);

		for (U32 i = 0; i < p->vertexCount; i++)
		{
			pnt = &polyList.mVertexList[polyList.mIndexList[p->vertexStart + i]];
			PrimBuild::vertex3fv(pnt);
		}

		pnt = &polyList.mVertexList[polyList.mIndexList[p->vertexStart]];
		PrimBuild::vertex3fv(pnt);

		PrimBuild::end();
	}

	GFX->leaveDebugEvent();*/
//}

CollisionList *CollisionSystem::getCollisionList(CollisionSystemInterface* colInterface) 
{ 
   return &colInterface->mCollisionList; 
}

void CollisionSystem::clearCollisionList(CollisionSystemInterface* colInterface) 
{ 
   colInterface->mCollisionList.clear(); 
}

void CollisionSystem::clearCollisionNotifyList(CollisionSystemInterface* colInterface) 
{ 
   colInterface->mCollisionNotifyList.clear(); 
}

ContactInfo* CollisionSystem::getContactInfo(CollisionSystemInterface* colInterface) 
{ 
   return &colInterface->mContactInfo; 
}

Convex *CollisionSystem::getConvexList(CollisionSystemInterface* colInterface) 
{ 
   return colInterface->mConvexList; 
}

bool CollisionSystem::doesBlockColliding(CollisionSystemInterface* colInterface)
{ 
   return colInterface->mBlockColliding; 
}

void CollisionSystem::handleCollisionList(CollisionSystemInterface* colInterface, VectorF velocity)
{
	Collision bestCol;

	//if (collisionList.getCount() > 0)
	//   bestCol = collisionList[collisionList.getCount() - 1];

	for (U32 i = 0; i < colInterface->mCollisionList.getCount(); ++i)
	{
		Collision& colCheck = colInterface->mCollisionList[i];

		if (colCheck.object)
		{
			if (colCheck.object->getTypeMask() & TriggerObjectType)
			{
				// We've hit it's bounding box, that's close enough for triggers
				Trigger* pTrigger = static_cast<Trigger*>(colCheck.object);

				pTrigger->potentialEnterObject(colInterface->mOwner->getOwner());
			}
         else if (colCheck.object->getTypeMask() & EntityObjectType)
         {
            Entity* ent = static_cast<Entity*>(colCheck.object);

            CollisionSystemInterface *colObjectInterface = CollisionSystemInterface::getByOwner(ent);
            if (colObjectInterface)
            {
               //convert us to our component
               colObjectInterface->onCollisionSignal.trigger(colInterface->mOwner->getOwner());
            }
         }
			else
			{
				handleCollision(colInterface, colCheck, velocity);
			}
		}
	}
}

void CollisionSystem::handleCollision(CollisionSystemInterface* colInterface, Collision &col, VectorF velocity)
{
	if (col.object && (colInterface->mContactInfo.contactObject == NULL ||
		col.object->getId() != colInterface->mContactInfo.contactObject->getId()))
	{
		queueCollision(colInterface, col.object, velocity - col.object->getVelocity());

		//do the callbacks to script for this collision
		if (colInterface->mOwner->isMethod("onCollision"))
		{
			S32 matId = col.material != NULL ? col.material->getMaterial()->getId() : 0;
			Con::executef(colInterface->mOwner, "onCollision", col.object, col.normal, col.point, matId, velocity);
		}

		if (colInterface->mOwner->getOwner()->isMethod("onCollisionEvent"))
		{
			S32 matId = col.material != NULL ? col.material->getMaterial()->getId() : 0;
			Con::executef(colInterface->mOwner->getOwner(), "onCollisionEvent", col.object, col.normal, col.point, matId, velocity);
		}
	}
}

void CollisionSystem::handleCollisionNotifyList(CollisionSystemInterface* colInterface)
{
	//special handling for any collision components we should notify that a collision happened.
	for (U32 i = 0; i < colInterface->mCollisionNotifyList.size(); ++i)
	{
   	colInterface->mCollisionNotifyList[i]->onCollisionSignal.trigger(colInterface->mOwner->getOwner());
	}

	colInterface->mCollisionNotifyList.clear();
}

void CollisionSystem::queueCollision(CollisionSystemInterface* colInterface, SceneObject *obj, const VectorF &vec)
{
	// Add object to list of collisions.
	SimTime time = Sim::getCurrentTime();
	S32 num = obj->getId();

	CollisionTimeout** adr = &colInterface->mTimeoutList;
	CollisionTimeout* ptr = colInterface->mTimeoutList;
	while (ptr)
	{
		if (ptr->objectNumber == num)
		{
			if (ptr->expireTime < time)
			{
				ptr->expireTime = time + CollisionTimeoutValue;
				ptr->object = obj;
				ptr->vector = vec;
			}
			return;
		}
		// Recover expired entries
		if (ptr->expireTime < time)
		{
			CollisionTimeout* cur = ptr;
			*adr = ptr->next;
			ptr = ptr->next;
			cur->next = colInterface->sFreeTimeoutList;
         colInterface->sFreeTimeoutList = cur;
		}
		else
		{
			adr = &ptr->next;
			ptr = ptr->next;
		}
	}

	// New entry for the object
	if (colInterface->sFreeTimeoutList != NULL)
	{
		ptr = colInterface->sFreeTimeoutList;
      colInterface->sFreeTimeoutList = ptr->next;
		ptr->next = NULL;
	}
	else
	{
		ptr = sTimeoutChunker.alloc();
	}

	ptr->object = obj;
	ptr->objectNumber = obj->getId();
	ptr->vector = vec;
	ptr->expireTime = time + CollisionTimeoutValue;
	ptr->next = colInterface->mTimeoutList;

   colInterface->mTimeoutList = ptr;
}

bool CollisionSystem::checkEarlyOut(Point3F start, VectorF velocity, F32 time, Box3F objectBox, Point3F objectScale,
	Box3F collisionBox, U32 collisionMask, CollisionWorkingList &colWorkingList)
{
	Point3F end = start + velocity * time;
	Point3F distance = end - start;

	Box3F scaledBox = objectBox;
	scaledBox.minExtents.convolve(objectScale);
	scaledBox.maxExtents.convolve(objectScale);

	if (mFabs(distance.x) < objectBox.len_x() &&
		mFabs(distance.y) < objectBox.len_y() &&
		mFabs(distance.z) < objectBox.len_z())
	{
		// We can potentially early out of this.  If there are no polys in the clipped polylist at our
		//  end position, then we can bail, and just set start = end;
		Box3F wBox = scaledBox;
		wBox.minExtents += end;
		wBox.maxExtents += end;

		static EarlyOutPolyList eaPolyList;
		eaPolyList.clear();
		eaPolyList.mNormal.set(0.0f, 0.0f, 0.0f);
		eaPolyList.mPlaneList.clear();
		eaPolyList.mPlaneList.setSize(6);
		eaPolyList.mPlaneList[0].set(wBox.minExtents, VectorF(-1.0f, 0.0f, 0.0f));
		eaPolyList.mPlaneList[1].set(wBox.maxExtents, VectorF(0.0f, 1.0f, 0.0f));
		eaPolyList.mPlaneList[2].set(wBox.maxExtents, VectorF(1.0f, 0.0f, 0.0f));
		eaPolyList.mPlaneList[3].set(wBox.minExtents, VectorF(0.0f, -1.0f, 0.0f));
		eaPolyList.mPlaneList[4].set(wBox.minExtents, VectorF(0.0f, 0.0f, -1.0f));
		eaPolyList.mPlaneList[5].set(wBox.maxExtents, VectorF(0.0f, 0.0f, 1.0f));

		// Build list from convex states here...
		CollisionWorkingList& rList = colWorkingList;
		CollisionWorkingList* pList = rList.wLink.mNext;
		while (pList != &rList)
		{
			Convex* pConvex = pList->mConvex;

			if (pConvex->getObject()->getTypeMask() & collisionMask)
			{
				Box3F convexBox = pConvex->getBoundingBox();

				if (wBox.isOverlapped(convexBox))
				{
					// No need to separate out the physical zones here, we want those
					//  to cause a fallthrough as well...
					pConvex->getPolyList(&eaPolyList);
				}
			}
			pList = pList->wLink.mNext;
		}

		if (eaPolyList.isEmpty())
		{
			return true;
		}
	}

	return false;
}

Collision* CollisionSystem::getCollision(CollisionSystemInterface* colInterface, S32 col)
{
	if (col < colInterface->mCollisionList.getCount() && col >= 0)
		return &colInterface->mCollisionList[col];
	else
		return NULL;
}