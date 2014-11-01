//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------
#include "console/consoleTypes.h"
#include "component/components/game/visibilityTriggerComponent.h"
#include "core/util/safeDelete.h"
#include "console/consoleTypes.h"
#include "console/consoleObject.h"
#include "core/stream/bitStream.h"
#include "console/engineAPI.h"
#include "sim/netConnection.h"
#include "T3D/gameBase/gameConnection.h"
#include "component/components/stockInterfaces.h"
#include "math/mathUtils.h"

#include "gfx/sim/debugDraw.h"

IMPLEMENT_CALLBACK( VisibilityTriggerComponentInstance, onEnterViewCmd, void, 
   ( Entity* cameraEnt, bool firstTimeSeeing ), ( cameraEnt, firstTimeSeeing ),
   "@brief Called when an object enters the volume of the Trigger instance using this TriggerData.\n\n"

   "@param trigger the Trigger instance whose volume the object entered\n"
   "@param obj the object that entered the volume of the Trigger instance\n" );

IMPLEMENT_CALLBACK( VisibilityTriggerComponentInstance, onExitViewCmd, void, 
   ( Entity* cameraEnt ), ( cameraEnt ),
   "@brief Called when an object enters the volume of the Trigger instance using this TriggerData.\n\n"

   "@param trigger the Trigger instance whose volume the object entered\n"
   "@param obj the object that entered the volume of the Trigger instance\n" );

IMPLEMENT_CALLBACK( VisibilityTriggerComponentInstance, onUpdateInViewCmd, void, 
   ( Entity* cameraEnt ), ( cameraEnt ),
   "@brief Called when an object enters the volume of the Trigger instance using this TriggerData.\n\n"

   "@param trigger the Trigger instance whose volume the object entered\n"
   "@param obj the object that entered the volume of the Trigger instance\n" );

IMPLEMENT_CALLBACK( VisibilityTriggerComponentInstance, onUpdateOutOfViewCmd, void, 
   ( Entity* cameraEnt ), ( cameraEnt ),
   "@brief Called when an object enters the volume of the Trigger instance using this TriggerData.\n\n"

   "@param trigger the Trigger instance whose volume the object entered\n"
   "@param obj the object that entered the volume of the Trigger instance\n" );

//////////////////////////////////////////////////////////////////////////
// Constructor/Destructor
//////////////////////////////////////////////////////////////////////////

VisibilityTriggerComponent::VisibilityTriggerComponent()
{
   mNetFlags.set(Ghostable | ScopeAlways);

   mFriendlyName = "Visibility Trigger";
   mComponentType = "Trigger";

   mDescription = getDescriptionText("Calls trigger events when a client starts and stops seeing it. Also ticks while visible to clients.");

   mNetworked = true;

   setScopeAlways();

   addComponentField("onEnterViewCmd", "Toggles if this behavior is active or not.", "Command", "", "");
   addComponentField("onExitViewCmd", "Toggles if this behavior is active or not.", "Command", "", "");
   addComponentField("onUpdateInViewCmd", "Toggles if this behavior is active or not.", "Command", "", "");
   addComponentField("onUpdateOutOfViewCmd", "Toggles if this behavior is active or not.", "Command", "", "");
}

VisibilityTriggerComponent::~VisibilityTriggerComponent()
{
   for(S32 i = 0;i < mFields.size();++i)
   {
      ComponentField &field = mFields[i];
      SAFE_DELETE_ARRAY(field.mFieldDescription);
   }

   SAFE_DELETE_ARRAY(mDescription);
}

IMPLEMENT_CO_NETOBJECT_V1(VisibilityTriggerComponent);

//////////////////////////////////////////////////////////////////////////
ComponentInstance *VisibilityTriggerComponent::createInstance()
{
   VisibilityTriggerComponentInstance *instance = new VisibilityTriggerComponentInstance(this);

   setupFields( instance );

   if(instance->registerObject())
      return instance;

   delete instance;
   return NULL;
}

bool VisibilityTriggerComponent::onAdd()
{
   if(! Parent::onAdd())
      return false;

   return true;
}

void VisibilityTriggerComponent::onRemove()
{
   Parent::onRemove();
}
void VisibilityTriggerComponent::initPersistFields()
{
   Parent::initPersistFields();
}

U32 VisibilityTriggerComponent::packUpdate(NetConnection *con, U32 mask, BitStream *stream)
{
   U32 retMask = Parent::packUpdate(con, mask, stream);
   return retMask;
}

void VisibilityTriggerComponent::unpackUpdate(NetConnection *con, BitStream *stream)
{
   Parent::unpackUpdate(con, stream);
}

//==========================================================================================
//==========================================================================================
VisibilityTriggerComponentInstance::VisibilityTriggerComponentInstance( Component *btemplate ) 
{
   mTemplate = btemplate;
   mOwner = NULL;

   mClientInfo.clear();

   mVisible = false;

   mNetFlags.set(Ghostable);

   mOnEnterViewCmd = StringTable->insert("");
   mOnExitViewCmd = StringTable->insert("");
   mOnUpdateInViewCmd = StringTable->insert("");
}

VisibilityTriggerComponentInstance::~VisibilityTriggerComponentInstance()
{
}
IMPLEMENT_CO_NETOBJECT_V1(VisibilityTriggerComponentInstance);

bool VisibilityTriggerComponentInstance::onAdd()
{
   if(! Parent::onAdd())
      return false;

   return true;
}

void VisibilityTriggerComponentInstance::onRemove()
{
   Parent::onRemove();
}

//This is mostly a catch for situations where the behavior is re-added to the object and the like and we may need to force an update to the behavior
void VisibilityTriggerComponentInstance::onComponentAdd()
{
   Parent::onComponentAdd();
}

void VisibilityTriggerComponentInstance::onComponentRemove()
{
   Parent::onComponentRemove();
}

void VisibilityTriggerComponentInstance::initPersistFields()
{
   Parent::initPersistFields();

   addField("visibile",   TypeBool,  Offset( mVisible, VisibilityTriggerComponentInstance ), "" );

   addField("onEnterViewCmd",   TypeString,  Offset( mOnEnterViewCmd, VisibilityTriggerComponentInstance ), "" );
   addField("onExitViewCmd",   TypeString,  Offset( mOnExitViewCmd, VisibilityTriggerComponentInstance ), "" );
   addField("onUpdateInViewCmd",   TypeString,  Offset( mOnUpdateInViewCmd, VisibilityTriggerComponentInstance ), "" );
}

U32 VisibilityTriggerComponentInstance::packUpdate(NetConnection *con, U32 mask, BitStream *stream)
{
   U32 retMask = Parent::packUpdate(con, mask, stream);
   return retMask;
}

void VisibilityTriggerComponentInstance::unpackUpdate(NetConnection *con, BitStream *stream)
{
   Parent::unpackUpdate(con, stream);
}

void VisibilityTriggerComponentInstance::processTick(const Move* move)
{
   Parent::processTick(move);

	//get our list of active clients, and see if they have cameras, if they do, build a frustum and see if we exist inside that
   mVisible = false;
   if(isServerObject())
   {
      for(U32 i=0; i < mClientInfo.size(); i++)
      {
         if(GameConnection* gameConn = getConnection(mClientInfo[i].clientID))
         {
            ComponentObject* cameraEntity = dynamic_cast<ComponentObject*>(gameConn->getCameraObject());

            if(cameraEntity)
            {
               //see if we have a camera-type component
               CameraInterface *camInterface = cameraEntity->getComponent<CameraInterface>();
               if(camInterface)
               {
                  Frustum visFrustum; 

                  visFrustum = camInterface->getFrustum();

                  bool culled = visFrustum.isCulled(mOwner->getWorldBox());
                  if(visFrustum.isContained(mOwner->getWorldBox()))
                  {
                     if(mClientInfo[i].triggerCurrentlySeen == false)
                     {
                        //just entered view
                        if(mOnEnterViewCmd != "")
                        {
                           String firstTime = mClientInfo[i].triggerEverSeen ? "false" : "true";
                           String command = String("%cameraEntity = ") + cameraEntity->getIdString() + ";" + 
                                            String("%clientID = ") + gameConn->getIdString() + ";" + 
                                            String("%firstTime = ") + firstTime + ";" + 
                                            String("%this = ") + getIdString() + ";" + mOnEnterViewCmd;
                           Con::evaluate(command.c_str());
                        }

                        onEnterViewCmd_callback(dynamic_cast<Entity*>(cameraEntity), !mClientInfo[i].triggerEverSeen);
                     }
                     //we're visible!
                     //do our callbacks and tick update!
                     mVisible = true;
                     mClientInfo[i].triggerCurrentlySeen = true;
                     mClientInfo[i].triggerEverSeen = true;
                     
                     if(mOnUpdateInViewCmd != "")
                     {
                        String command = String("%cameraEntity = ") + cameraEntity->getIdString() + ";" + 
                                          String("%clientID = ") + gameConn->getIdString() + ";" +
                                          String("%this = ") + getIdString() + ";" + mOnUpdateInViewCmd;
                        Con::evaluate(command.c_str());
                     }

                     onUpdateInViewCmd_callback(dynamic_cast<Entity*>(cameraEntity));
                  }
                  else if(mClientInfo[i].triggerCurrentlySeen)
                  {
                     mClientInfo[i].triggerCurrentlySeen = false;

                     //just entered view
                     if(mOnExitViewCmd != "")
                     {
                        String command = String("%cameraEntity = ") + cameraEntity->getIdString() + ";" + 
                                          String("%clientID = ") + gameConn->getIdString() + ";" +
                                          String("%this = ") + getIdString() + ";" + mOnExitViewCmd;
                        Con::evaluate(command.c_str());
                     }

                     onExitViewCmd_callback(dynamic_cast<Entity*>(cameraEntity));
                  }
               }

               //After we do our check, if our client isn't seeing it, we update this
               if(!mClientInfo[i].triggerCurrentlySeen)
               {
                  onUpdateOutOfViewCmd_callback(dynamic_cast<Entity*>(cameraEntity));
               }
            }
         }
      }
      /*for(NetConnection *conn = NetConnection::getConnectionList(); conn; conn = conn->getNext())  
      {  
         //if(conn->isServerConnection())  
			//	continue;  

         GameConnection* gameConn = dynamic_cast<GameConnection*>(conn);
  
			if (!gameConn || (gameConn && gameConn->isAIControlled()))
				continue; 

         S32 clientIndex = -1;
         for(U32 i=0; i < mClientInfo.size(); i++)
         {
            if(mClientInfo[i].clientID == gameConn->getId())
            {
               clientIndex = 1;
               break;
            }
         }

         if(clientIndex == -1)
            continue;

         ComponentObject* cameraEntity = dynamic_cast<ComponentObject*>(gameConn->getCameraObject());

         if(cameraEntity)
         {
            //see if we have a camera-type component
            CameraInterface *camInterface = cameraEntity->getComponent<CameraInterface>();
            if(camInterface)
            {
               Frustum visFrustum; 

               F32 left, right, top, bottom;
               F32 aspectRatio = mClientInfo[clientIndex].screenRes.x / mClientInfo[clientIndex].screenRes.y;
               MathUtils::makeFrustum( &left, &right, &top, &bottom, camInterface->, aspectRatio, 0.1f );
            }
         }
         
         GameBase* cameraObj = gameConn->getCameraObject();

         if(!cameraObj)
            continue;
        
			//conn->postNetEvent(new ObjectFreeEvent(object));  
		//} 
	//}*/
   }
}

void VisibilityTriggerComponentInstance::visualizeFrustums(F32 renderTimeMS)
{
   if(isServerObject())
   {
      for(U32 i=0; i < mClientInfo.size(); i++)
      {
         if(GameConnection* gameConn = getConnection(mClientInfo[i].clientID))
         {
            ComponentObject* cameraEntity = dynamic_cast<ComponentObject*>(gameConn->getCameraObject());

            if(cameraEntity)
            {
               //see if we have a camera-type component
               CameraInterface *camInterface = cameraEntity->getComponent<CameraInterface>();
               if(camInterface)
               {
                  Frustum visFrustum; 

                  visFrustum = camInterface->getFrustum();

                  DebugDrawer *ddraw = DebugDrawer::get();

                  const PolyhedronData::Edge* edges = visFrustum.getEdges();
                  const Point3F* points = visFrustum.getPoints();
                  for(U32 i=0; i < visFrustum.EdgeCount; i++)
                  {
                     Point3F vertA = points[edges[i].vertex[0]];
                     Point3F vertB = points[edges[i].vertex[1]];
                     ddraw->drawLine(vertA, vertB, ColorI(0,255,0,255));
                     ddraw->setLastTTL(renderTimeMS);
                  }
               }
            }
         }
      }
   }
}

GameConnection* VisibilityTriggerComponentInstance::getConnection(S32 connectionID)
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

void VisibilityTriggerComponentInstance::addClient(S32 clientID)
{
   bool found = false;
   for(U32 i=0; i < mClientInfo.size(); i++)
   {
      if(mClientInfo[i].clientID == clientID)
      {
         found = true;
         break;
      }
   }

   if(!found)
   {
      clientInfo newClient;
      newClient.clientID = clientID;

      mClientInfo.push_back(newClient);
   }
}

void VisibilityTriggerComponentInstance::removeClient(S32 clientID)
{
   for(U32 i=0; i < mClientInfo.size(); i++)
   {
      if(mClientInfo[i].clientID == clientID)
         mClientInfo.erase(i);
   }
}

DefineEngineMethod( VisibilityTriggerComponentInstance, addClient, void,
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

DefineEngineMethod( VisibilityTriggerComponentInstance, removeClient, void,
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

DefineEngineMethod( VisibilityTriggerComponentInstance, visualizeFrustums, void,
                   (F32 renderTime), (1000),
                   "@brief Mount objB to this object at the desired slot with optional transform.\n\n"

                   "@param objB  Object to mount onto us\n"
                   "@param slot  Mount slot ID\n"
                   "@param txfm (optional) mount offset transform\n"
                   "@return true if successful, false if failed (objB is not valid)" )
{
   object->visualizeFrustums(renderTime);
}