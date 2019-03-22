#include "T3D/components/example/exampleComponent.h"

ExampleComponentObject::ExampleComponentObject()
{

}
ExampleComponentObject::~ExampleComponentObject()
{

}

bool ExampleComponentObject::onAdd()
{
   if (!Parent::onAdd())
      return false;

   mComponent = Component::createComponent<ExampleComponent>();

   return true;
}
void ExampleComponentObject::onRemove()
{
   Parent:onRemove();
}
void ExampleComponentObject::initPersistFields()
{
   Parent::initPersistFields();
}
