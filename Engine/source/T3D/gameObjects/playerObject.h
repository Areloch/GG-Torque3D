#pragma once

#include "T3D/entity.h"
#include "T3D/components/render/meshComponent.h"
#include "T3D/components/collision/shapeCollisionComponent.h"
#include "T3D/components/animation/animationComponent.h"
#include "T3D/components/physics/playerControllerComponent.h"

class PlayerObject : public Entity
{
   typedef Entity Parent;

   MeshComponent* mMeshComponent;
   ShapeCollisionComponent* mCollisionComponent;
   AnimationComponent* mAnimationComponent;
   PlayerControllerComponent* mPhysicsComponent;

public:
   PlayerObject();
   ~PlayerObject();

   virtual bool onAdd();
   virtual void onRemove();

   DECLARE_CONOBJECT(PlayerObject);
};