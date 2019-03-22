#pragma once

#include "T3D/components/component.h"

struct ExampleComponent : public Component
{
   ECS_DECLARE_TYPE;

   ExampleComponent(float x, float y) : x(x), y(y) {}
   ExampleComponent() {}

   float x;
   float y;
};

ECS_DEFINE_TYPE(ExampleComponent);

//
class ExampleComponentObject : public ComponentObject
{
   typedef ComponentObject Parent;

   ComponentHandle<ExampleComponent> mComponent;

public:
   ExampleComponentObject();
   virtual ~ExampleComponentObject();
   DECLARE_CONOBJECT(ExampleComponentObject);

   virtual bool onAdd();
   virtual void onRemove();
   static void initPersistFields();
};

