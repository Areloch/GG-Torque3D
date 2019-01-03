#include "aiControllerComponent.h"

#include "platform/platform.h"

#include "console/consoleInternal.h"
#include "math/mMatrix.h"
#include "T3D/gameBase/moveManager.h"
#include "console/engineAPI.h"

#include <cfloat>


static U32 sAIControllerComponentLoSMask = TerrainObjectType | StaticShapeObjectType | StaticObjectType | EntityObjectType;

AIControllerComponent::AIControllerComponent()
{

}

AIControllerComponent::~AIControllerComponent()
{

}

void AIControllerComponent::processTick()
{
   Parent::processTick();

   Move *movePtr = &mOwner->lastMove;

   //If we haven't dealt with a move yet and we're on the server, go ahead and do our AI control now
   if (!movePtr && isServerObject())
      getAIMove(movePtr);
}

/**
 * Sets the speed at which this AI moves
 *
 * @param speed Speed to move, default player was 10
 */
void AIControllerComponent::setMoveSpeed(F32 speed)
{
   mMoveSpeed = getMax(0.0f, getMin(1.0f, speed));
}

/**
 * Stops movement for this AI
 */
void AIControllerComponent::stopMove()
{
   mMoveState = ModeStop;
#ifdef TORQUE_NAVIGATION_ENABLED
   clearPath();
   clearCover();
   clearFollow();
#endif
}

/**
 * Sets how far away from the move location is considered
 * "on target"
 *
 * @param tolerance Movement tolerance for error
 */
void AIControllerComponent::setMoveTolerance(const F32 tolerance)
{
   mMoveTolerance = getMax(0.1f, tolerance);
}

/**
 * Sets the location for the bot to run to
 *
 * @param location Point to run to
 */
void AIControllerComponent::setMoveDestination(const Point3F &location, bool slowdown)
{
   mMoveDestination = location;
   mMoveState = ModeMove;
   mMoveSlowdown = slowdown;
   mMoveStuckTestCountdown = mMoveStuckTestDelay;
}

/**
 * Sets the object the bot is targeting
 *
 * @param targetObject The object to target
 */
void AIControllerComponent::setAimObject(GameBase *targetObject)
{
   mAimObject = targetObject;
   mTargetInLOS = false;
   mAimOffset = Point3F(0.0f, 0.0f, 0.0f);
}

/**
 * Sets the object the bot is targeting and an offset to add to target location
 *
 * @param targetObject The object to target
 * @param offset       The offest from the target location to aim at
 */
void AIControllerComponent::setAimObject(GameBase *targetObject, const Point3F& offset)
{
   mAimObject = targetObject;
   mTargetInLOS = false;
   mAimOffset = offset;
}

/**
 * Sets the location for the bot to aim at
 *
 * @param location Point to aim at
 */
void AIControllerComponent::setAimLocation(const Point3F &location)
{
   mAimObject = 0;
   mAimLocationSet = true;
   mAimLocation = location;
   mAimOffset = Point3F(0.0f, 0.0f, 0.0f);
}

/**
 * Clears the aim location and sets it to the bot's
 * current destination so he looks where he's going
 */
void AIControllerComponent::clearAim()
{
   mAimObject = 0;
   mAimLocationSet = false;
   mAimOffset = Point3F(0.0f, 0.0f, 0.0f);
}

/**
 * Sets the correct aim for the bot to the target
 */
void AIControllerComponent::getMuzzleVector(U32 imageSlot, VectorF* vec)
{
   MatrixF mat;
   getMuzzleTransform(imageSlot, &mat);

   MountedImage& image = mMountedImageList[imageSlot];

   if (image.dataBlock->correctMuzzleVector)
   {
      disableHeadZCalc();
      if (getCorrectedAim(mat, vec))
      {
         enableHeadZCalc();
         return;
      }
      enableHeadZCalc();

   }
   mat.getColumn(1, vec);
}

/**
 * Set the state of a movement trigger.
 *
 * @param slot The trigger slot to set
 * @param isSet set/unset the trigger
 */
void AIControllerComponent::setMoveTrigger(U32 slot, const bool isSet)
{
   if (slot >= MaxTriggerKeys)
   {
      Con::errorf("Attempting to set an invalid trigger slot (%i)", slot);
   }
   else
   {
      mMoveTriggers[slot] = isSet;   // set the trigger
      setMaskBits(NoWarpMask);         // force the client to updateMove
   }
}

/**
 * Get the state of a movement trigger.
 *
 * @param slot The trigger slot to query
 * @return True if the trigger is set, false if it is not set
 */
bool AIControllerComponent::getMoveTrigger(U32 slot) const
{
   if (slot >= MaxTriggerKeys)
   {
      Con::errorf("Attempting to get an invalid trigger slot (%i)", slot);
      return false;
   }
   else
   {
      return mMoveTriggers[slot];
   }
}

/**
 * Clear the trigger state for all movement triggers.
 */
void AIControllerComponent::clearMoveTriggers()
{
   for (U32 i = 0; i < MaxTriggerKeys; i++)
      setMoveTrigger(i, false);
}

/**
 * This method calculates the moves for the AI player
 *
 * @param movePtr Pointer to move the move list into
 */
bool AIControllerComponent::getAIMove(Move *movePtr)
{
   *movePtr = NullMove;

   // Use the eye as the current position.
   MatrixF eye;
   getEyeTransform(&eye);
   Point3F location = eye.getPosition();
   Point3F rotation = mOwner->getRotation().asEulerF();

#ifdef TORQUE_NAVIGATION_ENABLED
   if (mMoveState != ModeStop)
      updateNavMesh();
   if (!mFollowData.object.isNull())
   {
      if (mPathData.path.isNull())
      {
         if ((mOwner->getPosition() - mFollowData.object->getPosition()).len() > mFollowData.radius)
            followObject(mFollowData.object, mFollowData.radius);
      }
      else
      {
         if ((mPathData.path->mTo - mFollowData.object->getPosition()).len() > mFollowData.radius)
            repath();
         else if ((mOwner->getPosition() - mFollowData.object->getPosition()).len() < mFollowData.radius)
         {
            clearPath();
            mMoveState = ModeStop;
            throwCallback("onTargetInRange");
         }
         else if ((mOwner->getPosition() - mFollowData.object->getPosition()).len() < mAttackRadius)
         {
            throwCallback("onTargetInFiringRange");
         }
      }
   }
#endif // TORQUE_NAVIGATION_ENABLED

   // Orient towards the aim point, aim object, or towards
   // our destination.
   if (mAimObject || mAimLocationSet || mMoveState != ModeStop)
   {
      // Update the aim position if we're aiming for an object
      if (mAimObject)
         mAimLocation = mAimObject->getPosition() + mAimOffset;
      else
         if (!mAimLocationSet)
            mAimLocation = mMoveDestination;

      F32 xDiff = mAimLocation.x - location.x;
      F32 yDiff = mAimLocation.y - location.y;

      if (!mIsZero(xDiff) || !mIsZero(yDiff))
      {
         // First do Yaw
         // use the cur yaw between -Pi and Pi
         F32 curYaw = rotation.z;
         while (curYaw > M_2PI_F)
            curYaw -= M_2PI_F;
         while (curYaw < -M_2PI_F)
            curYaw += M_2PI_F;

         // find the yaw offset
         F32 newYaw = mAtan2(xDiff, yDiff);
         F32 yawDiff = newYaw - curYaw;

         // make it between 0 and 2PI
         if (yawDiff < 0.0f)
            yawDiff += M_2PI_F;
         else if (yawDiff >= M_2PI_F)
            yawDiff -= M_2PI_F;

         // now make sure we take the short way around the circle
         if (yawDiff > M_PI_F)
            yawDiff -= M_2PI_F;
         else if (yawDiff < -M_PI_F)
            yawDiff += M_2PI_F;

         movePtr->yaw = yawDiff;

         // Next do pitch.
         /*if (!mAimObject && !mAimLocationSet)
         {
            // Level out if were just looking at our next way point.
            Point3F headRotation = getHeadRotation();
            movePtr->pitch = -headRotation.x;
         }
         else
         {
            // This should be adjusted to run from the
            // eye point to the object's center position. Though this
            // works well enough for now.
            F32 vertDist = mAimLocation.z - location.z;
            F32 horzDist = mSqrt(xDiff * xDiff + yDiff * yDiff);
            F32 newPitch = mAtan2(horzDist, vertDist) - (M_PI_F / 2.0f);
            if (mFabs(newPitch) > 0.01f)
            {
               Point3F headRotation = getHeadRotation();
               movePtr->pitch = newPitch - headRotation.x;
            }
         }*/
      }
   }
   else
   {
      // Level out if we're not doing anything else
      //Point3F headRotation = getHeadRotation();
      //movePtr->pitch = -headRotation.x;
   }

   // Move towards the destination
   if (mMoveState != ModeStop)
   {
      F32 xDiff = mMoveDestination.x - location.x;
      F32 yDiff = mMoveDestination.y - location.y;

      // Check if we should mMove, or if we are 'close enough'
      if (mFabs(xDiff) < mMoveTolerance && mFabs(yDiff) < mMoveTolerance)
      {
         mMoveState = ModeStop;
         onReachDestination();
      }
      else
      {
         // Build move direction in world space
         if (mIsZero(xDiff))
            movePtr->y = (location.y > mMoveDestination.y) ? -1.0f : 1.0f;
         else
            if (mIsZero(yDiff))
               movePtr->x = (location.x > mMoveDestination.x) ? -1.0f : 1.0f;
            else
               if (mFabs(xDiff) > mFabs(yDiff))
               {
                  F32 value = mFabs(yDiff / xDiff);
                  movePtr->y = (location.y > mMoveDestination.y) ? -value : value;
                  movePtr->x = (location.x > mMoveDestination.x) ? -1.0f : 1.0f;
               }
               else
               {
                  F32 value = mFabs(xDiff / yDiff);
                  movePtr->x = (location.x > mMoveDestination.x) ? -value : value;
                  movePtr->y = (location.y > mMoveDestination.y) ? -1.0f : 1.0f;
               }

         // Rotate the move into object space (this really only needs
         // a 2D matrix)
         Point3F newMove;
         MatrixF moveMatrix;
         moveMatrix.set(EulerF(0.0f, 0.0f, -(rotation.z + movePtr->yaw)));
         moveMatrix.mulV(Point3F(movePtr->x, movePtr->y, 0.0f), &newMove);
         movePtr->x = newMove.x;
         movePtr->y = newMove.y;

         // Set movement speed.  We'll slow down once we get close
         // to try and stop on the spot...
         if (mMoveSlowdown)
         {
            F32 speed = mMoveSpeed;
            F32 dist = mSqrt(xDiff*xDiff + yDiff * yDiff);
            F32 maxDist = mMoveTolerance * 2;
            if (dist < maxDist)
               speed *= dist / maxDist;
            movePtr->x *= speed;
            movePtr->y *= speed;

            mMoveState = ModeSlowing;
         }
         else
         {
            movePtr->x *= mMoveSpeed;
            movePtr->y *= mMoveSpeed;

            mMoveState = ModeMove;
         }

         // Don't check for ai stuckness if animation during
         // an anim-clip effect override.
         
         if (mMoveStuckTestCountdown > 0)
            --mMoveStuckTestCountdown;
         else
         {
            // We should check to see if we are stuck...
            F32 locationDelta = (location - mLastLocation).len();
            if (locationDelta < mMoveStuckTolerance)
            {
               // If we are slowing down, then it's likely that our location delta will be less than
               // our move stuck tolerance. Because we can be both slowing and stuck
               // we should TRY to check if we've moved. This could use better detection.
               if (mMoveState != ModeSlowing || locationDelta == 0)
               {
                  mMoveState = ModeStuck;
                  onStuck();
               }
            }
         }
      }
   }

   // Test for target location in sight if it's an object. The LOS is
   // run from the eye position to the center of the object's bounding,
   // which is not very accurate.
   if (mAimObject)
   {
      if (checkInLos(mAimObject.getPointer()))
      {
         if (!mTargetInLOS)
         {
            throwCallback("onTargetEnterLOS");
            mTargetInLOS = true;
         }
      }
      else if (mTargetInLOS)
      {
         throwCallback("onTargetExitLOS");
         mTargetInLOS = false;
      }
   }

   // Replicate the trigger state into the move so that
   // triggers can be controlled from scripts.
   for (U32 i = 0; i < MaxTriggerKeys; i++)
      movePtr->trigger[i] = getImageTriggerState(i);

#ifdef TORQUE_NAVIGATION_ENABLED
   if (mJump == Now)
   {
      movePtr->trigger[2] = true;
      mJump = None;
   }
   else if (mJump == Ledge)
   {
      // If we're not touching the ground, jump!
      RayInfo info;
      if (!mOwner->getContainer()->castRay(mOwner->getPosition(), mOwner->getPosition() - Point3F(0, 0, 0.4f), StaticShapeObjectType, &info))
      {
         movePtr->trigger[2] = true;
         mJump = None;
      }
   }
#endif // TORQUE_NAVIGATION_ENABLED

   mLastLocation = location;

   return true;
}

void AIControllerComponent::updateMove(const Move* move)
{
   if (!mOwner->getControllingClient() && mOwner->isGhost())
      return;

   //Parent::updateMove(move);
}

/**
 * Utility function to throw callbacks. Callbacks always occure
 * on the datablock class.
 *
 * @param name Name of script function to call
 */
void AIControllerComponent::throwCallback(const char *name)
{
   Con::executef(this, name, getIdString());
}

/**
 * Called when we get within mMoveTolerance of our destination set using
 * setMoveDestination(). Only fires the script callback if we are at the end
 * of a pathfinding path, or have no pathfinding path.
 */
void AIControllerComponent::onReachDestination()
{
#ifdef TORQUE_NAVIGATION_ENABLED
   if (!mPathData.path.isNull())
   {
      if (mPathData.index == mPathData.path->size() - 1)
      {
         // Handle looping paths.
         if (mPathData.path->mIsLooping)
            moveToNode(0);
         // Otherwise end path.
         else
         {
            clearPath();
            throwCallback("onReachDestination");
         }
      }
      else
      {
         moveToNode(mPathData.index + 1);
         // Throw callback every time if we're on a looping path.
         //if(mPathData.path->mIsLooping)
            //throwCallback("onReachDestination");
      }
   }
   else
#endif
      throwCallback("onReachDestination");
}

/**
 * Called when we move less than mMoveStuckTolerance in a tick, signalling
 * that some obstacle is preventing us from getting where we need to go.
 */
void AIControllerComponent::onStuck()
{
#ifdef TORQUE_NAVIGATION_ENABLED
   if (!mPathData.path.isNull())
      repath();
   else
#endif
      throwCallback("onMoveStuck");
}

#ifdef TORQUE_NAVIGATION_ENABLED
// --------------------------------------------------------------------------------------------
// Pathfinding
// --------------------------------------------------------------------------------------------

void AIControllerComponent::clearPath()
{
   // Only delete if we own the path.
   if (!mPathData.path.isNull() && mPathData.owned)
      mPathData.path->deleteObject();
   // Reset path data.
   mPathData = PathData();
}

void AIControllerComponent::clearCover()
{
   // Notify cover that we are no longer on our way.
   if (!mCoverData.cover.isNull())
      mCoverData.cover->setOccupied(false);
   mCoverData = CoverData();
}

void AIControllerComponent::clearFollow()
{
   mFollowData = FollowData();
}

void AIControllerComponent::moveToNode(S32 node)
{
   if (mPathData.path.isNull())
      return;

   // -1 is shorthand for 'last path node'.
   if (node == -1)
      node = mPathData.path->size() - 1;

   // Consider slowing down on the last path node.
   setMoveDestination(mPathData.path->getNode(node), false);

   // Check flags for this segment.
   if (mPathData.index)
   {
      U16 flags = mPathData.path->getFlags(node - 1);
      // Jump if we must.
      if (flags & LedgeFlag)
         mJump = Ledge;
      else if (flags & JumpFlag)
         mJump = Now;
      else
         // Catch pathing errors.
         mJump = None;
   }

   // Store current index.
   mPathData.index = node;
}

bool AIControllerComponent::setPathDestination(const Point3F &pos)
{
   // Pathfinding only happens on the server.
   if (!isServerObject())
      return false;

   if (!getNavMesh())
      updateNavMesh();
   // If we can't find a mesh, just move regularly.
   if (!getNavMesh())
   {
      //setMoveDestination(pos);
      throwCallback("onPathFailed");
      return false;
   }

   // Create a new path.
   NavPath *path = new NavPath();

   path->mMesh = getNavMesh();
   path->mFrom = mOwner->getPosition();
   path->mTo = pos;
   path->mFromSet = path->mToSet = true;
   path->mAlwaysRender = true;
   path->mLinkTypes = mLinkTypes;
   path->mXray = true;
   // Paths plan automatically upon being registered.
   if (!path->registerObject())
   {
      delete path;
      return false;
   }

   if (path->success())
   {
      // Clear any current path we might have.
      clearPath();
      clearCover();
      clearFollow();
      // Store new path.
      mPathData.path = path;
      mPathData.owned = true;
      // Skip node 0, which we are currently standing on.
      moveToNode(1);
      throwCallback("onPathSuccess");
      return true;
   }
   else
   {
      // Just move normally if we can't path.
      //setMoveDestination(pos, true);
      //return;
      throwCallback("onPathFailed");
      path->deleteObject();
      return false;
   }
}

DefineEngineMethod(AIControllerComponent, setPathDestination, bool, (Point3F goal), ,
   "@brief Tells the AI to find a path to the location provided\n\n"

   "@param goal Coordinates in world space representing location to move to.\n"
   "@return True if a path was found.\n\n"

   "@see getPathDestination()\n"
   "@see setMoveDestination()\n")
{
   return object->setPathDestination(goal);
}

Point3F AIControllerComponent::getPathDestination() const
{
   if (!mPathData.path.isNull())
      return mPathData.path->mTo;
   return Point3F(0, 0, 0);
}

DefineEngineMethod(AIControllerComponent, getPathDestination, Point3F, (), ,
   "@brief Get the AIControllerComponent's current pathfinding destination.\n\n"

   "@return Returns a point containing the \"x y z\" position "
   "of the AIControllerComponent's current path destination. If no path destination "
   "has yet been set, this returns \"0 0 0\"."

   "@see setPathDestination()\n")
{
   return object->getPathDestination();
}

void AIControllerComponent::followNavPath(NavPath *path)
{
   if (!isServerObject())
      return;

   // Get rid of our current path.
   clearPath();
   clearCover();
   clearFollow();

   // Follow new path.
   mPathData.path = path;
   mPathData.owned = false;
   // Start from 0 since we might not already be there.
   moveToNode(0);
}

DefineEngineMethod(AIControllerComponent, followNavPath, void, (SimObjectId obj), ,
   "@brief Tell the AIControllerComponent to follow a path.\n\n"

   "@param obj ID of a NavPath object for the character to follow.")
{
   NavPath *path;
   if (Sim::findObject(obj, path))
      object->followNavPath(path);
}

void AIControllerComponent::followObject(SceneObject *obj, F32 radius)
{
   if (!isServerObject())
      return;

   if ((mFollowData.lastPos - obj->getPosition()).len() < mMoveTolerance)
      return;

   if (setPathDestination(obj->getPosition()))
   {
      clearCover();
      mFollowData.object = obj;
      mFollowData.radius = radius;
      mFollowData.lastPos = obj->getPosition();
   }
}

DefineEngineMethod(AIControllerComponent, followObject, void, (SimObjectId obj, F32 radius), ,
   "@brief Tell the AIControllerComponent to follow another object.\n\n"

   "@param obj ID of the object to follow.\n"
   "@param radius Maximum distance we let the target escape to.")
{
   SceneObject *follow;
   object->clearPath();
   object->clearCover();
   object->clearFollow();

   if (Sim::findObject(obj, follow))
      object->followObject(follow, radius);
}

void AIControllerComponent::repath()
{
   // Ineffectual if we don't have a path, or are using someone else's.
   if (mPathData.path.isNull() || !mPathData.owned)
      return;

   // If we're following, get their position.
   if (!mFollowData.object.isNull())
      mPathData.path->mTo = mFollowData.object->getPosition();
   // Update from position and replan.
   mPathData.path->mFrom = mOwner->getPosition();
   mPathData.path->plan();
   // Move to first node (skip start pos).
   moveToNode(1);
}

DefineEngineMethod(AIControllerComponent, repath, void, (), ,
   "@brief Tells the AI to re-plan its path. Does nothing if the character "
   "has no path, or if it is following a mission path.\n\n")
{
   object->repath();
}

struct CoverSearch
{
   Point3F loc;
   Point3F from;
   F32 dist;
   F32 best;
   CoverPoint *point;
   CoverSearch() : loc(0, 0, 0), from(0, 0, 0)
   {
      best = -FLT_MAX;
      point = NULL;
      dist = FLT_MAX;
   }
};

static void findCoverCallback(SceneObject *obj, void *key)
{
   CoverPoint *p = dynamic_cast<CoverPoint*>(obj);
   if (!p || p->isOccupied())
      return;
   CoverSearch *s = static_cast<CoverSearch*>(key);
   Point3F dir = s->from - p->getPosition();
   dir.normalizeSafe();
   // Score first based on angle of cover point to enemy.
   F32 score = mDot(p->getNormal(), dir);
   // Score also based on distance from seeker.
   score -= (p->getPosition() - s->loc).len() / s->dist;
   // Finally, consider cover size.
   score += (p->getSize() + 1) / CoverPoint::NumSizes;
   score *= p->getQuality();
   if (score > s->best)
   {
      s->best = score;
      s->point = p;
   }
}

bool AIControllerComponent::findCover(const Point3F &from, F32 radius)
{
   if (radius <= 0)
      return false;

   // Create a search state.
   CoverSearch s;
   s.loc = mOwner->getPosition();
   s.dist = radius;
   // Direction we seek cover FROM.
   s.from = from;

   // Find cover points.
   Box3F box(radius * 2.0f);
   box.setCenter(mOwner->getPosition());
   mOwner->getContainer()->findObjects(box, MarkerObjectType, findCoverCallback, &s);

   // Go to cover!
   if (s.point)
   {
      // Calling setPathDestination clears cover...
      bool foundPath = setPathDestination(s.point->getPosition());
      // Now store the cover info.
      mCoverData.cover = s.point;
      s.point->setOccupied(true);
      return foundPath;
   }
   return false;
}

DefineEngineMethod(AIControllerComponent, findCover, S32, (Point3F from, F32 radius), ,
   "@brief Tells the AI to find cover nearby.\n\n"

   "@param from   Location to find cover from (i.e., enemy position).\n"
   "@param radius Distance to search for cover.\n"
   "@return Cover point ID if cover was found, -1 otherwise.\n\n")
{
   if (object->findCover(from, radius))
   {
      CoverPoint* cover = object->getCover();
      return cover ? cover->getId() : -1;
   }
   else
   {
      return -1;
   }
}

NavMesh *AIControllerComponent::findNavMesh() const
{
   // Search for NavMeshes that contain us entirely with the smallest possible
   // volume.
   NavMesh *mesh = NULL;
   SimSet *set = NavMesh::getServerSet();
   for (U32 i = 0; i < set->size(); i++)
   {
      NavMesh *m = static_cast<NavMesh*>(set->at(i));
      if (m->getWorldBox().isContained(mOwner->getWorldBox()))
      {
         // Check that mesh size is appropriate.
         if (mOwner->getObjectMount()) // Should use isMounted() but it's not const. Grr.
         {
            if (!m->mVehicles)
               continue;
         }
         else
         {
            if ((getNavSize() == Small && !m->mSmallCharacters) ||
               (getNavSize() == Regular && !m->mRegularCharacters) ||
               (getNavSize() == Large && !m->mLargeCharacters))
               continue;
         }
         if (!mesh || m->getWorldBox().getVolume() < mesh->getWorldBox().getVolume())
            mesh = m;
      }
   }
   return mesh;
}

DefineEngineMethod(AIControllerComponent, findNavMesh, S32, (), ,
   "@brief Get the NavMesh object this AIControllerComponent is currently using.\n\n"

   "@return The ID of the NavPath object this character is using for "
   "pathfinding. This is determined by the character's location, "
   "navigation type and other factors. Returns -1 if no NavMesh is "
   "found.")
{
   NavMesh *mesh = object->getNavMesh();
   return mesh ? mesh->getId() : -1;
}

void AIControllerComponent::updateNavMesh()
{
   NavMesh *old = mNavMesh;
   if (mNavMesh.isNull())
      mNavMesh = findNavMesh();
   else
   {
      if (!mNavMesh->getWorldBox().isContained(mOwner->getWorldBox()))
         mNavMesh = findNavMesh();
   }
   // See if we need to update our path.
   if (mNavMesh != old && !mPathData.path.isNull())
   {
      setPathDestination(mPathData.path->mTo);
   }
}

DefineEngineMethod(AIControllerComponent, getNavMesh, S32, (), ,
   "@brief Return the NavMesh this AIControllerComponent is using to navigate.\n\n")
{
   NavMesh *m = object->getNavMesh();
   return m ? m->getId() : 0;
}

DefineEngineMethod(AIControllerComponent, setNavSize, void, (const char *size), ,
   "@brief Set the size of NavMesh this character uses. One of \"Small\", \"Regular\" or \"Large\".")
{
   if (!dStrcmp(size, "Small"))
      object->setNavSize(AIControllerComponent::Small);
   else if (!dStrcmp(size, "Regular"))
      object->setNavSize(AIControllerComponent::Regular);
   else if (!dStrcmp(size, "Large"))
      object->setNavSize(AIControllerComponent::Large);
   else
      Con::errorf("AIControllerComponent::setNavSize: no such size '%s'.", size);
}

DefineEngineMethod(AIControllerComponent, getNavSize, const char*, (), ,
   "@brief Return the size of NavMesh this character uses for pathfinding.")
{
   switch (object->getNavSize())
   {
      case AIControllerComponent::Small:
         return "Small";
      case AIControllerComponent::Regular:
         return "Regular";
      case AIControllerComponent::Large:
         return "Large";
   }
   return "";
}
#endif // TORQUE_NAVIGATION_ENABLED

// --------------------------------------------------------------------------------------------
// Console Functions
// --------------------------------------------------------------------------------------------

DefineEngineMethod(AIControllerComponent, stop, void, (), ,
   "@brief Tells the AIControllerComponent to stop moving.\n\n")
{
   object->stopMove();
}

DefineEngineMethod(AIControllerComponent, clearAim, void, (), ,
   "@brief Use this to stop aiming at an object or a point.\n\n"

   "@see setAimLocation()\n"
   "@see setAimObject()\n")
{
   object->clearAim();
}

DefineEngineMethod(AIControllerComponent, setMoveSpeed, void, (F32 speed), ,
   "@brief Sets the move speed for an AI object.\n\n"

   "@param speed A speed multiplier between 0.0 and 1.0.  "
   "This is multiplied by the AIControllerComponent's base movement rates (as defined in "
   "its PlayerData datablock)\n\n"

   "@see getMoveDestination()\n")
{
   object->setMoveSpeed(speed);
}

DefineEngineMethod(AIControllerComponent, getMoveSpeed, F32, (), ,
   "@brief Gets the move speed of an AI object.\n\n"

   "@return A speed multiplier between 0.0 and 1.0.\n\n"

   "@see setMoveSpeed()\n")
{
   return object->getMoveSpeed();
}

DefineEngineMethod(AIControllerComponent, setMoveDestination, void, (Point3F goal, bool slowDown), (true),
   "@brief Tells the AI to move to the location provided\n\n"

   "@param goal Coordinates in world space representing location to move to.\n"
   "@param slowDown A boolean value. If set to true, the bot will slow down "
   "when it gets within 5-meters of its move destination. If false, the bot "
   "will stop abruptly when it reaches the move destination. By default, this is true.\n\n"

   "@note Upon reaching a move destination, the bot will clear its move destination and "
   "calls to getMoveDestination will return \"0 0 0\"."

   "@see getMoveDestination()\n")
{
   object->setMoveDestination(goal, slowDown);
}

DefineEngineMethod(AIControllerComponent, getMoveDestination, Point3F, (), ,
   "@brief Get the AIControllerComponent's current destination.\n\n"

   "@return Returns a point containing the \"x y z\" position "
   "of the AIControllerComponent's current move destination. If no move destination "
   "has yet been set, this returns \"0 0 0\"."

   "@see setMoveDestination()\n")
{
   return object->getMoveDestination();
}

DefineEngineMethod(AIControllerComponent, setAimLocation, void, (Point3F target), ,
   "@brief Tells the AIControllerComponent to aim at the location provided.\n\n"

   "@param target An \"x y z\" position in the game world to target.\n\n"

   "@see getAimLocation()\n")
{
   object->setAimLocation(target);
}

DefineEngineMethod(AIControllerComponent, getAimLocation, Point3F, (), ,
   "@brief Returns the point the AIControllerComponent is aiming at.\n\n"

   "This will reflect the position set by setAimLocation(), "
   "or the position of the object that the bot is now aiming at.  "
   "If the bot is not aiming at anything, this value will "
   "change to whatever point the bot's current line-of-sight intercepts."

   "@return World space coordinates of the object AI is aiming at. Formatted as \"X Y Z\".\n\n"

   "@see setAimLocation()\n"
   "@see setAimObject()\n")
{
   return object->getAimLocation();
}

ConsoleDocFragment _setAimObject(
   "@brief Sets the AIControllerComponent's target object.  May optionally set an offset from target location\n\n"

   "@param targetObject The object to target\n"
   "@param offset Optional three-element offset vector which will be added to the position of the aim object.\n\n"

   "@tsexample\n"
   "// Without an offset\n"
   "%ai.setAimObject(%target);\n\n"
   "// With an offset\n"
   "// Cause our AI object to aim at the target\n"
   "// offset (0, 0, 1) so you don't aim at the target's feet\n"
   "%ai.setAimObject(%target, \"0 0 1\");\n"
   "@endtsexample\n\n"

   "@see getAimLocation()\n"
   "@see getAimObject()\n"
   "@see clearAim()\n",

   "AIControllerComponent",
   "void setAimObject(GameBase targetObject, Point3F offset);"
);

DefineConsoleMethod(AIControllerComponent, setAimObject, void, (const char * objName, Point3F offset), (Point3F::Zero), "( GameBase obj, [Point3F offset] )"
   "Sets the bot's target object. Optionally set an offset from target location."
   "@hide")
{

   // Find the target
   GameBase *targetObject;
   if (Sim::findObject(objName, targetObject))
   {

      object->setAimObject(targetObject, offset);
   }
   else
      object->setAimObject(0, offset);
}

DefineEngineMethod(AIControllerComponent, getAimObject, S32, (), ,
   "@brief Gets the object the AIControllerComponent is targeting.\n\n"

   "@return Returns -1 if no object is being aimed at, "
   "or the SimObjectID of the object the AIControllerComponent is aiming at.\n\n"

   "@see setAimObject()\n")
{
   GameBase* obj = object->getAimObject();
   return obj ? obj->getId() : -1;
}

bool AIControllerComponent::checkInLos(GameBase* target, bool _useMuzzle)
{
   if (!isServerObject()) return false;
   if (!target)
   {
      target = mAimObject.getPointer();
      if (!target)
         return false;
   }

   RayInfo ri;

   mOwner->disableCollision();

   S32 mountCount = target->getMountedObjectCount();
   for (S32 i = 0; i < mountCount; i++)
   {
      target->getMountedObject(i)->disableCollision();
   }

   Point3F checkPoint;
   if (_useMuzzle)
      getMuzzlePointAI(0, &checkPoint);
   else
   {
      MatrixF eyeMat;
      getEyeTransform(&eyeMat);
      eyeMat.getColumn(3, &checkPoint);
   }

   bool hit = !gServerContainer.castRay(checkPoint, target->getBoxCenter(), sAIControllerComponentLoSMask, &ri);
   mOwner->enableCollision();

   for (S32 i = 0; i < mountCount; i++)
   {
      target->getMountedObject(i)->enableCollision();
   }
   return hit;
}

bool AIControllerComponent::checkInFoV(GameBase* target, F32 camFov)
{
   if (!isServerObject()) return false;
   if (!target)
   {
      target = mAimObject.getPointer();
      if (!target)
         return false;
   }

   MatrixF cam = mOwner->getTransform();
   Point3F camPos;
   VectorF camDir;

   cam.getColumn(3, &camPos);
   cam.getColumn(1, &camDir);

   camFov = mDegToRad(camFov) / 2;

   Point3F shapePos = target->getBoxCenter();
   VectorF shapeDir = shapePos - camPos;
   // Test to see if it's within our viewcone, this test doesn't
   // actually match the viewport very well, should consider
   // projection and box test.
   shapeDir.normalize();
   F32 dot = mDot(shapeDir, camDir);
   return (dot > mCos(camFov));
}

DefineEngineMethod(AIControllerComponent, checkInLos, bool, (GameBase* obj, bool useMuzzle), (nullAsType<GameBase*>(), false),
   "@brief Check whether an object is in line of sight.\n"
   "@obj Object to check. (If blank, it will check the current target).\n"
   "@useMuzzle Use muzzle position. Otherwise use eye position. (defaults to false).\n"
   "@checkEnabled check whether the object can take damage and if so is still alive.(Defaults to false)\n")
{
   return object->checkInLos(obj, useMuzzle);
}

DefineEngineMethod(AIControllerComponent, checkInFoV, bool, (GameBase* obj, F32 fov), (nullAsType<GameBase*>(), 45.0f),
   "@brief Check whether an object is within a specified veiw cone.\n"
   "@obj Object to check. (If blank, it will check the current target).\n"
   "@fov view angle in degrees.(Defaults to 45)\n"
   "@checkEnabled check whether the object can take damage and if so is still alive.(Defaults to false)\n")
{
   return object->checkInFoV(obj, fov);
}

DefineEngineMethod(AIControllerComponent, setMoveTrigger, void, (U32 slot), ,
   "@brief Sets a movement trigger on an AI object.\n\n"
   "@param slot The trigger slot to set.\n"
   "@see getMoveTrigger()\n"
   "@see clearMoveTrigger()\n"
   "@see clearMoveTriggers()\n")
{
   object->setMoveTrigger(slot, true);
}

DefineEngineMethod(AIControllerComponent, clearMoveTrigger, void, (U32 slot), ,
   "@brief Clears a movement trigger on an AI object.\n\n"
   "@param slot The trigger slot to set.\n"
   "@see setMoveTrigger()\n"
   "@see getMoveTrigger()\n"
   "@see clearMoveTriggers()\n")
{
   object->setMoveTrigger(slot, false);
}

DefineEngineMethod(AIControllerComponent, getMoveTrigger, bool, (U32 slot), ,
   "@brief Tests if a movement trigger on an AI object is set.\n\n"
   "@param slot The trigger slot to check.\n"
   "@return a boolean indicating if the trigger is set/unset.\n"
   "@see setMoveTrigger()\n"
   "@see clearMoveTrigger()\n"
   "@see clearMoveTriggers()\n")
{
   return object->getMoveTrigger(slot);
}

DefineEngineMethod(AIControllerComponent, clearMoveTriggers, void, (), ,
   "@brief Clear ALL movement triggers on an AI object.\n"
   "@see setMoveTrigger()\n"
   "@see getMoveTrigger()\n"
   "@see clearMoveTrigger()\n")
{
   object->clearMoveTriggers();
}

F32 AIControllerComponent::getTargetDistance(GameBase* target)
{
   if (!isServerObject()) return false;
   if (!target)
   {
      target = mAimObject.getPointer();
      if (!target)
         return F32_MAX;
   }

   return (mOwner->getPosition() - target->getPosition()).len();
}

DefineEngineMethod(AIControllerComponent, getTargetDistance, F32, (GameBase* obj), (nullAsType<GameBase*>()),
   "@brief The distance to a given object.\n"
   "@obj Object to check. (If blank, it will check the current target).\n"
   "@checkEnabled check whether the object can take damage and if so is still alive.(Defaults to false)\n")
{
   return object->getTargetDistance(obj);
}