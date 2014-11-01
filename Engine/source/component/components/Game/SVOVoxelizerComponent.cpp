//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------
#include "console/consoleTypes.h"
#include "component/components/game/SVOVoxelizerComponent.h"
#include "core/util/safeDelete.h"
#include "console/consoleTypes.h"
#include "console/consoleObject.h"
#include "core/stream/bitStream.h"
#include "console/engineAPI.h"
#include "sim/netConnection.h"

//////////////////////////////////////////////////////////////////////////
// Constructor/Destructor
//////////////////////////////////////////////////////////////////////////

SVOVoxelizerComponent::SVOVoxelizerComponent()
{
   mNetFlags.set(Ghostable | ScopeAlways);
}

SVOVoxelizerComponent::~SVOVoxelizerComponent()
{
   for(S32 i = 0;i < mFields.size();++i)
   {
      ComponentField &field = mFields[i];
      SAFE_DELETE_ARRAY(field.mFieldDescription);
   }

   SAFE_DELETE_ARRAY(mDescription);
}

IMPLEMENT_CO_NETOBJECT_V1(SVOVoxelizerComponent);

//////////////////////////////////////////////////////////////////////////
ComponentInstance *SVOVoxelizerComponent::createInstance()
{
   SVOVoxelizerComponentInstance *instance = new SVOVoxelizerComponentInstance(this);

   setupFields( instance );

   if(instance->registerObject())
      return instance;

   delete instance;
   return NULL;
}

bool SVOVoxelizerComponent::onAdd()
{
   if(! Parent::onAdd())
      return false;

   return true;
}

void SVOVoxelizerComponent::onRemove()
{
   Parent::onRemove();
}
void SVOVoxelizerComponent::initPersistFields()
{
   Parent::initPersistFields();
}

U32 SVOVoxelizerComponent::packUpdate(NetConnection *con, U32 mask, BitStream *stream)
{
   U32 retMask = Parent::packUpdate(con, mask, stream);
   return retMask;
}

void SVOVoxelizerComponent::unpackUpdate(NetConnection *con, BitStream *stream)
{
   Parent::unpackUpdate(con, stream);
}

//==========================================================================================
//==========================================================================================
SVOVoxelizerComponentInstance::SVOVoxelizerComponentInstance( Component *btemplate ) 
{
   mTemplate = btemplate;
   mOwner = NULL;

   mNetFlags.set(Ghostable);
}

SVOVoxelizerComponentInstance::~SVOVoxelizerComponentInstance()
{
}
IMPLEMENT_CO_NETOBJECT_V1(SVOVoxelizerComponentInstance);

bool SVOVoxelizerComponentInstance::onAdd()
{
   if(! Parent::onAdd())
      return false;

   return true;
}

void SVOVoxelizerComponentInstance::onRemove()
{
   Parent::onRemove();
}

//This is mostly a catch for situations where the behavior is re-added to the object and the like and we may need to force an update to the behavior
void SVOVoxelizerComponentInstance::onComponentAdd()
{
   Parent::onComponentAdd();
}

void SVOVoxelizerComponentInstance::onComponentRemove()
{
   Parent::onComponentRemove();
}

void SVOVoxelizerComponentInstance::initPersistFields()
{
   Parent::initPersistFields();
}

U32 SVOVoxelizerComponentInstance::packUpdate(NetConnection *con, U32 mask, BitStream *stream)
{
   U32 retMask = Parent::packUpdate(con, mask, stream);
   return retMask;
}

void SVOVoxelizerComponentInstance::unpackUpdate(NetConnection *con, BitStream *stream)
{
   Parent::unpackUpdate(con, stream);
}

/*void SVOVoxelizerComponentInstance::buildSVO()
{
   Parent::unpackUpdate(con, stream);
}*/
