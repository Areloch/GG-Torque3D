//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------
#include "console/consoleTypes.h"
#include "component/components/game/TriggerComponent.h"
#include "core/util/safeDelete.h"
#include "console/consoleTypes.h"
#include "console/consoleObject.h"
#include "core/stream/bitStream.h"
#include "console/engineAPI.h"
#include "sim/netConnection.h"
#include "T3D/gameBase/gameConnection.h"
#include "component/components/stockInterfaces.h"
#include "math/mathUtils.h"
#include "collision/concretePolyList.h"
#include "collision/clippedPolyList.h"

#include "gfx/sim/debugDraw.h"

IMPLEMENT_CALLBACK( TriggerComponentInstance, onEnterViewCmd, void, 
   ( Entity* cameraEnt, bool firstTimeSeeing ), ( cameraEnt, firstTimeSeeing ),
   "@brief Called when an object enters the volume of the Trigger instance using this TriggerData.\n\n"

   "@param trigger the Trigger instance whose volume the object entered\n"
   "@param obj the object that entered the volume of the Trigger instance\n" );

IMPLEMENT_CALLBACK( TriggerComponentInstance, onExitViewCmd, void, 
   ( Entity* cameraEnt ), ( cameraEnt ),
   "@brief Called when an object enters the volume of the Trigger instance using this TriggerData.\n\n"

   "@param trigger the Trigger instance whose volume the object entered\n"
   "@param obj the object that entered the volume of the Trigger instance\n" );

IMPLEMENT_CALLBACK( TriggerComponentInstance, onUpdateInViewCmd, void, 
   ( Entity* cameraEnt ), ( cameraEnt ),
   "@brief Called when an object enters the volume of the Trigger instance using this TriggerData.\n\n"

   "@param trigger the Trigger instance whose volume the object entered\n"
   "@param obj the object that entered the volume of the Trigger instance\n" );

IMPLEMENT_CALLBACK( TriggerComponentInstance, onUpdateOutOfViewCmd, void, 
   ( Entity* cameraEnt ), ( cameraEnt ),
   "@brief Called when an object enters the volume of the Trigger instance using this TriggerData.\n\n"

   "@param trigger the Trigger instance whose volume the object entered\n"
   "@param obj the object that entered the volume of the Trigger instance\n" );

//////////////////////////////////////////////////////////////////////////
// Constructor/Destructor
//////////////////////////////////////////////////////////////////////////

TriggerComponent::TriggerComponent()
{
   mNetFlags.set(Ghostable | ScopeAlways);

   mFriendlyName = "Trigger";
   mComponentType = "Trigger";

   mDescription = getDescriptionText("Calls trigger events when a client starts and stops seeing it. Also ticks while visible to clients.");

   mNetworked = true;

   setScopeAlways();

   addComponentField("onEnterViewCmd", "Toggles if this behavior is active or not.", "Command", "", "");
   addComponentField("onExitViewCmd", "Toggles if this behavior is active or not.", "Command", "", "");
   addComponentField("onUpdateInViewCmd", "Toggles if this behavior is active or not.", "Command", "", "");
   addComponentField("onUpdateOutOfViewCmd", "Toggles if this behavior is active or not.", "Command", "", "");
}

TriggerComponent::~TriggerComponent()
{
   for(S32 i = 0;i < mFields.size();++i)
   {
      ComponentField &field = mFields[i];
      SAFE_DELETE_ARRAY(field.mFieldDescription);
   }

   SAFE_DELETE_ARRAY(mDescription);
}

IMPLEMENT_CO_NETOBJECT_V1(TriggerComponent);

//////////////////////////////////////////////////////////////////////////
ComponentInstance *TriggerComponent::createInstance()
{
   TriggerComponentInstance *instance = new TriggerComponentInstance(this);

   setupFields( instance );

   if(instance->registerObject())
      return instance;

   delete instance;
   return NULL;
}

bool TriggerComponent::onAdd()
{
   if(! Parent::onAdd())
      return false;

   return true;
}

void TriggerComponent::onRemove()
{
   Parent::onRemove();
}
void TriggerComponent::initPersistFields()
{
   Parent::initPersistFields();
}

U32 TriggerComponent::packUpdate(NetConnection *con, U32 mask, BitStream *stream)
{
   U32 retMask = Parent::packUpdate(con, mask, stream);
   return retMask;
}

void TriggerComponent::unpackUpdate(NetConnection *con, BitStream *stream)
{
   Parent::unpackUpdate(con, stream);
}

//==========================================================================================
//==========================================================================================
TriggerComponentInstance::TriggerComponentInstance( Component *btemplate ) 
{
   mTemplate = btemplate;
   mOwner = NULL;

   mObjectList.clear();

   mVisible = false;

   mNetFlags.set(Ghostable);

   mOnEnterViewCmd = StringTable->insert("");
   mOnExitViewCmd = StringTable->insert("");
   mOnUpdateInViewCmd = StringTable->insert("");
}

TriggerComponentInstance::~TriggerComponentInstance()
{
}
IMPLEMENT_CO_NETOBJECT_V1(TriggerComponentInstance);

bool TriggerComponentInstance::onAdd()
{
   if(! Parent::onAdd())
      return false;

   return true;
}

void TriggerComponentInstance::onRemove()
{
   Parent::onRemove();
}

//This is mostly a catch for situations where the behavior is re-added to the object and the like and we may need to force an update to the behavior
void TriggerComponentInstance::onComponentAdd()
{
   Parent::onComponentAdd();

   CollisionInterface *colInt = mOwner->getComponent<CollisionInterface>();

   if(colInt)
   {
      colInt->onCollisionSignal.notify(this, &TriggerComponentInstance::potentialEnterObject);
   }
}

void TriggerComponentInstance::onComponentRemove()
{
   Parent::onComponentRemove();

   CollisionInterface *colInt = mOwner->getComponent<CollisionInterface>();

   if(colInt)
   {
      colInt->onCollisionSignal.remove(this, &TriggerComponentInstance::potentialEnterObject);
   }
}

void TriggerComponentInstance::initPersistFields()
{
   Parent::initPersistFields();

   addField("visibile",   TypeBool,  Offset( mVisible, TriggerComponentInstance ), "" );

   addField("onEnterViewCmd",   TypeString,  Offset( mOnEnterViewCmd, TriggerComponentInstance ), "" );
   addField("onExitViewCmd",   TypeString,  Offset( mOnExitViewCmd, TriggerComponentInstance ), "" );
   addField("onUpdateInViewCmd",   TypeString,  Offset( mOnUpdateInViewCmd, TriggerComponentInstance ), "" );
}

U32 TriggerComponentInstance::packUpdate(NetConnection *con, U32 mask, BitStream *stream)
{
   U32 retMask = Parent::packUpdate(con, mask, stream);
   return retMask;
}

void TriggerComponentInstance::unpackUpdate(NetConnection *con, BitStream *stream)
{
   Parent::unpackUpdate(con, stream);
}

void TriggerComponentInstance::potentialEnterObject(SceneObject *collider)
{
   if(testObject(collider))
   {
      bool found = false;
      for(U32 i=0; i < mObjectList.size(); i++)
      {
         if(mObjectList[i]->getId() == collider->getId())
         {
            found = true;
            break;
         }
      }

      if(!found)
         mObjectList.push_back(collider);
   }
}

bool TriggerComponentInstance::testObject(SceneObject* enter)
{
   //First, test to early out
   Box3F enterBox = enter->getWorldBox();

   //if(!mOwner->getWorldBox().intersect(enterBox) || !)
   //   return false;

   //We're still here, so we should do actual work
   //We're going to be 
   ConcretePolyList mClippedList;

   SphereF sphere;
   sphere.center = (mOwner->getWorldBox().minExtents + mOwner->getWorldBox().maxExtents) * 0.5;
   VectorF bv = mOwner->getWorldBox().maxExtents - sphere.center;
   sphere.radius = bv.len();

   Entity* enterEntity = dynamic_cast<Entity*>(enter);
   if(enterEntity)
   {
      //check if the entity has a collision shape
      CollisionInterface *cI = enterEntity->getComponent<CollisionInterface>();
      if(cI)
      {
         cI->buildPolyList(PLC_Collision, &mClippedList, mOwner->getWorldBox(), sphere);

         if(!mClippedList.isEmpty())
         {
            //well, it's clipped with, or inside, our bounds
            //now to test the clipped list against our own collision mesh
            CollisionInterface *myCI = mOwner->getComponent<CollisionInterface>();

            //wait, how would we NOT have this?
            if(myCI)
            {
               //anywho, build our list and then we'll check intersections
               ClippedPolyList myList;
               myCI->buildPolyList(PLC_Collision, &myList, enterBox, sphere);
               if(myList.isEmpty())
                  int huh = 0;
               else
                  int wha = 0;
            }
         }
      }
   }

   return mClippedList.isEmpty() == false;
}

void TriggerComponentInstance::processTick(const Move* move)
{
   Parent::processTick(move);

	//get our list of active clients, and see if they have cameras, if they do, build a frustum and see if we exist inside that
   mVisible = false;
   if(isServerObject())
   {
      for(U32 i=0; i < mObjectList.size(); i++)
      {
         if(!testObject(mObjectList[i]))
         {
            mObjectList.erase(i);
         }
      }
   }
}

void TriggerComponentInstance::visualizeFrustums(F32 renderTimeMS)
{
   
}

GameConnection* TriggerComponentInstance::getConnection(S32 connectionID)
{
   for(NetConnection *conn = NetConnection::getConnectionList(); conn; conn = conn->getNext())  
   {  
      GameConnection* gameConn = dynamic_cast<GameConnection*>(conn);
  
      if (!gameConn || (gameConn && gameConn->isAIControlled()))
         continue; 

      if(connectionID == gameConn->getId())
         return gameConn;
   }

   return NULL;
}

void TriggerComponentInstance::addClient(S32 clientID)
{
   
}

void TriggerComponentInstance::removeClient(S32 clientID)
{

}

DefineEngineMethod( TriggerComponentInstance, addClient, void,
                   ( S32 clientID ), ( -1 ),
                   "@brief Mount objB to this object at the desired slot with optional transform.\n\n"

                   "@param objB  Object to mount onto us\n"
                   "@param slot  Mount slot ID\n"
                   "@param txfm (optional) mount offset transform\n"
                   "@return true if successful, false if failed (objB is not valid)" )
{
   if(clientID == -1)
      return;

   object->addClient( clientID );
}

DefineEngineMethod( TriggerComponentInstance, removeClient, void,
                   ( S32 clientID ), ( -1 ),
                   "@brief Mount objB to this object at the desired slot with optional transform.\n\n"

                   "@param objB  Object to mount onto us\n"
                   "@param slot  Mount slot ID\n"
                   "@param txfm (optional) mount offset transform\n"
                   "@return true if successful, false if failed (objB is not valid)" )
{
   if(clientID == -1)
      return;

   object->removeClient( clientID );
}

DefineEngineMethod( TriggerComponentInstance, visualizeFrustums, void,
                   (F32 renderTime), (1000),
                   "@brief Mount objB to this object at the desired slot with optional transform.\n\n"

                   "@param objB  Object to mount onto us\n"
                   "@param slot  Mount slot ID\n"
                   "@param txfm (optional) mount offset transform\n"
                   "@return true if successful, false if failed (objB is not valid)" )
{
   object->visualizeFrustums(renderTime);
}