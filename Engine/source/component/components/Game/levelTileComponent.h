//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------
#ifndef _LEVEL_TILE_COMPONENT_H_
#define _LEVEL_TILE_COMPONENT_H_

#ifndef _COMPONENT_H_
#include "component/components/component.h"
#endif

//////////////////////////////////////////////////////////////////////////
/// 
/// 
//////////////////////////////////////////////////////////////////////////
class LevelTileComponent : public Component
{
   typedef Component Parent;

public:
   LevelTileComponent();
   virtual ~LevelTileComponent();
   DECLARE_CONOBJECT(LevelTileComponent);

   virtual bool onAdd();
   virtual void onRemove();
   static void initPersistFields();

   virtual U32 packUpdate(NetConnection *con, U32 mask, BitStream *stream);
   virtual void unpackUpdate(NetConnection *con, BitStream *stream);

   //override to pass back a LevelTileComponentInstance
   virtual ComponentInstance *createInstance();
};

class LevelTileComponentInterface
{
public:
   virtual bool doSomething() = 0;
};

class LevelTileComponentInstance : public ComponentInstance,
   public LevelTileComponentInterface
{
   typedef ComponentInstance Parent;

protected:
   virtual bool doSomething() { return true; }

public:
   LevelTileComponentInstance(Component *btemplate = NULL);
   virtual ~LevelTileComponentInstance();
   DECLARE_CONOBJECT(LevelTileComponentInstance);

   virtual bool onAdd();
   virtual void onRemove();
   static void initPersistFields();

   virtual void onComponentAdd();
   virtual void onComponentRemove();

   virtual U32 packUpdate(NetConnection *con, U32 mask, BitStream *stream);
   virtual void unpackUpdate(NetConnection *con, BitStream *stream);
};

#endif // _LevelTileComponent_H_
