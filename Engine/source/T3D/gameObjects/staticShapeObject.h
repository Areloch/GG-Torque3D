#pragma once

#include "T3D/entity.h"
#include "T3D/components/render/meshComponent.h"
#include "T3D/components/collision/shapeCollisionComponent.h"
#include "T3D/components/animation/animationComponent.h"

class StaticShapeObject : public Entity
{
   typedef Entity Parent;

   MeshComponent* mMeshComponent;
   ShapeCollisionComponent* mCollisionComponent;
   AnimationComponent* mAnimationComponent;

public:
   StaticShapeObject();
   ~StaticShapeObject();

   virtual bool onAdd();
   virtual void onRemove();

   DECLARE_CONOBJECT(StaticShapeObject);
};