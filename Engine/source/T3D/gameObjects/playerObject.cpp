#include "playerObject.h"

IMPLEMENT_CO_NETOBJECT_V1(PlayerObject);

PlayerObject::PlayerObject()
   :  mMeshComponent(nullptr),
      mCollisionComponent(nullptr),
      mAnimationComponent(nullptr),
      mPhysicsComponent(nullptr)
{

}
PlayerObject::~PlayerObject()
{

}

bool PlayerObject::onAdd()
{
   if (!Parent::onAdd())
      return false;

   //Mesh
   mMeshComponent = new MeshComponent();
   if (!mMeshComponent->registerObject())
   {
      Con::errorf("PlayerObject::onAdd - unable to add MeshComponent!");
      return false;
   }

   mMeshComponent->setInternalName("meshComponent");
   
   addComponent(mMeshComponent);

   //Collision
   mCollisionComponent = new ShapeCollisionComponent();
   if (!mCollisionComponent->registerObject())
   {
      Con::errorf("PlayerObject::onAdd - unable to add ShapeCollisionComponent!");
      return false;
   }

   mCollisionComponent->setInternalName("collisionComponent");

   addComponent(mCollisionComponent);

   //Animation
   mAnimationComponent = new AnimationComponent();
   if (!mAnimationComponent->registerObject())
   {
      Con::errorf("PlayerObject::onAdd - unable to add AnimationComponent!");
      return false;
   }

   mAnimationComponent->setInternalName("animationComponent");

   addComponent(mAnimationComponent);

   //Physics control
   mPhysicsComponent = new PlayerControllerComponent();
   if (!mPhysicsComponent->registerObject())
   {
      Con::errorf("PlayerObject::onAdd - unable to add PhysicsComponent!");
      return false;
   }

   mPhysicsComponent->setInternalName("physicsComponent");

   addComponent(mPhysicsComponent);

   return true;
}

void PlayerObject::onRemove()
{
   Parent::onRemove();
}