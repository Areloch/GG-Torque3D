//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------
#ifndef _SVOVOXELIZER_COMPONENT_H_
#define _SVOVOXELIZER_COMPONENT_H_

#ifndef _COMPONENT_H_
#include "component/components/component.h"
#endif

//////////////////////////////////////////////////////////////////////////
/// 
/// 
//////////////////////////////////////////////////////////////////////////
class SVOVoxelizerComponent : public Component
{
   typedef Component Parent;

public:
   SVOVoxelizerComponent();
   virtual ~SVOVoxelizerComponent();
   DECLARE_CONOBJECT(SVOVoxelizerComponent);

   virtual bool onAdd();
   virtual void onRemove();
   static void initPersistFields();

   virtual U32 packUpdate(NetConnection *con, U32 mask, BitStream *stream);
   virtual void unpackUpdate(NetConnection *con, BitStream *stream);

   //override to pass back a SVOVoxelizerComponentInstance
   virtual ComponentInstance *createInstance();
};

class SVOVoxelizerComponentInterface
{
public:
   virtual bool doSomething() = 0;
};

class SVOVoxelizerComponentInstance : public ComponentInstance,
   public SVOVoxelizerComponentInterface
{
   typedef ComponentInstance Parent;

protected:
   virtual bool doSomething() { return true; }

public:
   SVOVoxelizerComponentInstance(Component *btemplate = NULL);
   virtual ~SVOVoxelizerComponentInstance();
   DECLARE_CONOBJECT(SVOVoxelizerComponentInstance);

   virtual bool onAdd();
   virtual void onRemove();
   static void initPersistFields();

   virtual void onComponentAdd();
   virtual void onComponentRemove();

   virtual U32 packUpdate(NetConnection *con, U32 mask, BitStream *stream);
   virtual void unpackUpdate(NetConnection *con, BitStream *stream);
};

#endif // _SVOVoxelizerComponent_H_
