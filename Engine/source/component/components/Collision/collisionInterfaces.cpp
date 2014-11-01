#include "component/components/collision/collisionInterfaces.h"
#include "scene/sceneObject.h"
#include "T3D/Entity.h"

void CollisionInterface::handleCollisionList( CollisionList &collisionList, VectorF velocity )
{
   Collision col;

   if (collisionList.getCount() > 0)
      col = collisionList[collisionList.getCount() - 1];

   for (U32 i=0; i < collisionList.getCount(); ++i)
   {
      Collision& colCheck = collisionList[i];
      if (colCheck.object)
      {
         if (col.object->getTypeMask() & PlayerObjectType)
         {
            handleCollision( colCheck, velocity );
         }
         else
         {
            col = colCheck;
         }

         Entity* ent = dynamic_cast<Entity*>(col.object);
         if(ent)
         {
            CollisionInterface *cI = ent->getComponent<CollisionInterface>();
            if(cI)
            {
               //convert us to our component
               ComponentInstance *component = dynamic_cast<ComponentInstance*>(this);
               if(component)
               {
                  cI->onCollisionSignal.trigger(component->getOwner());
               }
            }
         }
         //if we ended up colliding, trigger our signal to notify any components that may be waiting on it
         //onCollisionSignal.trigger(col.object);
      }
   }

   handleCollision( col, velocity );
}

void CollisionInterface::handleCollision( Collision &col, VectorF velocity )
{
   if ( col.object)
      queueCollision( col.object, velocity - col.object->getVelocity() );
}

Chunker<CollisionInterface::CollisionTimeout> sTimeoutChunker;
CollisionInterface::CollisionTimeout* CollisionInterface::sFreeTimeoutList = 0;

void CollisionInterface::queueCollision( SceneObject *obj, const VectorF &vec)
{
   // Add object to list of collisions.
   SimTime time = Sim::getCurrentTime();
   S32 num = obj->getId();

   CollisionTimeout** adr = &mTimeoutList;
   CollisionTimeout* ptr = mTimeoutList;
   while (ptr) {
      if (ptr->objectNumber == num) {
         if (ptr->expireTime < time) {
            ptr->expireTime = time + CollisionTimeoutValue;
            ptr->object = obj;
            ptr->vector = vec;
         }
         return;
      }
      // Recover expired entries
      if (ptr->expireTime < time) {
         CollisionTimeout* cur = ptr;
         *adr = ptr->next;
         ptr = ptr->next;
         cur->next = sFreeTimeoutList;
         sFreeTimeoutList = cur;
      }
      else {
         adr = &ptr->next;
         ptr = ptr->next;
      }
   }

   // New entry for the object
   if (sFreeTimeoutList != NULL)
   {
      ptr = sFreeTimeoutList;
      sFreeTimeoutList = ptr->next;
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
   ptr->next = mTimeoutList;

   mTimeoutList = ptr;
}

bool CollisionInterface::checkEarlyOut(Point3F start, VectorF velocity, F32 time, Box3F objectBox, Point3F objectScale, 
														Box3F collisionBox, U32 collisionMask, CollisionWorkingList &colWorkingList)
{
   Point3F end = start + velocity * time;
   Point3F distance = end - start;

   Box3F scaledBox = objectBox;
   scaledBox.minExtents.convolve( objectScale );
   scaledBox.maxExtents.convolve( objectScale );

   if (mFabs(distance.x) < objectBox.len_x() &&
      mFabs(distance.y) < objectBox.len_y() &&
      mFabs(distance.z) < objectBox.len_z())
   {
      // We can potentially early out of this.  If there are no polys in the clipped polylist at our
      //  end position, then we can bail, and just set start = end;
      Box3F wBox = collisionBox;  
      wBox.minExtents += distance;  
      wBox.maxExtents += distance;

      static EarlyOutPolyList eaPolyList;
      eaPolyList.clear();
      eaPolyList.mNormal.set(0.0f, 0.0f, 0.0f);
      eaPolyList.mPlaneList.clear();
      eaPolyList.mPlaneList.setSize(6);
      eaPolyList.mPlaneList[0].set(wBox.minExtents,VectorF(-1.0f, 0.0f, 0.0f));
      eaPolyList.mPlaneList[1].set(wBox.maxExtents,VectorF(0.0f, 1.0f, 0.0f));
      eaPolyList.mPlaneList[2].set(wBox.maxExtents,VectorF(1.0f, 0.0f, 0.0f));
      eaPolyList.mPlaneList[3].set(wBox.minExtents,VectorF(0.0f, -1.0f, 0.0f));
      eaPolyList.mPlaneList[4].set(wBox.minExtents,VectorF(0.0f, 0.0f, -1.0f));
      eaPolyList.mPlaneList[5].set(wBox.maxExtents,VectorF(0.0f, 0.0f, 1.0f));

      // Build list from convex states here...
      CollisionWorkingList& rList = colWorkingList;
      CollisionWorkingList* pList = rList.wLink.mNext;
      while (pList != &rList) {
         Convex* pConvex = pList->mConvex;
         if (pConvex->getObject()->getTypeMask() & collisionMask) {
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

Collision* CollisionInterface::getCollision(S32 col) 
{ 
   if(col < mCollisionList.getCount() && col >= 0) 
      return &mCollisionList[col];
   else 
      return NULL; 
}