//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------
#include "console/consoleTypes.h"
#include "component/components/game/LevelTileComponent.h"
#include "core/util/safeDelete.h"
#include "console/consoleTypes.h"
#include "console/consoleObject.h"
#include "core/stream/bitStream.h"
#include "console/engineAPI.h"
#include "sim/netConnection.h"

//////////////////////////////////////////////////////////////////////////
// Constructor/Destructor
//////////////////////////////////////////////////////////////////////////

LevelTileComponent::LevelTileComponent()
{
   mNetFlags.set(Ghostable | ScopeAlways);

   mFriendlyName = "Level Tile";
   mComponentType = "Game";

	mDescription = getDescriptionText("A tileset of objects assembled as a prefab.");

	addComponentField("Launch Tile Editor", "Launches the level tile editor tool.", "tileEditorButton", "", "");
}

LevelTileComponent::~LevelTileComponent()
{
   for(S32 i = 0;i < mFields.size();++i)
   {
      ComponentField &field = mFields[i];
      SAFE_DELETE_ARRAY(field.mFieldDescription);
   }

   SAFE_DELETE_ARRAY(mDescription);
}

IMPLEMENT_CO_NETOBJECT_V1(LevelTileComponent);

//////////////////////////////////////////////////////////////////////////
ComponentInstance *LevelTileComponent::createInstance()
{
   LevelTileComponentInstance *instance = new LevelTileComponentInstance(this);

   setupFields( instance );

   if(instance->registerObject())
      return instance;

   delete instance;
   return NULL;
}

bool LevelTileComponent::onAdd()
{
   if(! Parent::onAdd())
      return false;

   return true;
}

void LevelTileComponent::onRemove()
{
   Parent::onRemove();
}
void LevelTileComponent::initPersistFields()
{
   Parent::initPersistFields();
}

U32 LevelTileComponent::packUpdate(NetConnection *con, U32 mask, BitStream *stream)
{
   U32 retMask = Parent::packUpdate(con, mask, stream);
   return retMask;
}

void LevelTileComponent::unpackUpdate(NetConnection *con, BitStream *stream)
{
   Parent::unpackUpdate(con, stream);
}

//==========================================================================================
//==========================================================================================
LevelTileComponentInstance::LevelTileComponentInstance( Component *btemplate ) 
{
   mTemplate = btemplate;
   mOwner = NULL;

   mNetFlags.set(Ghostable);
}

LevelTileComponentInstance::~LevelTileComponentInstance()
{
}
IMPLEMENT_CO_NETOBJECT_V1(LevelTileComponentInstance);

bool LevelTileComponentInstance::onAdd()
{
   if(! Parent::onAdd())
      return false;

   return true;
}

void LevelTileComponentInstance::onRemove()
{
   Parent::onRemove();
}

//This is mostly a catch for situations where the behavior is re-added to the object and the like and we may need to force an update to the behavior
void LevelTileComponentInstance::onComponentAdd()
{
   Parent::onComponentAdd();
}

void LevelTileComponentInstance::onComponentRemove()
{
   Parent::onComponentRemove();
}

void LevelTileComponentInstance::initPersistFields()
{
   Parent::initPersistFields();
}

U32 LevelTileComponentInstance::packUpdate(NetConnection *con, U32 mask, BitStream *stream)
{
   U32 retMask = Parent::packUpdate(con, mask, stream);
   return retMask;
}

void LevelTileComponentInstance::unpackUpdate(NetConnection *con, BitStream *stream)
{
   Parent::unpackUpdate(con, stream);
}