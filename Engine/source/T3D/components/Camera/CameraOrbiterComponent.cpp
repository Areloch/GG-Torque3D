//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------
#include "T3D/Components/Camera/CameraOrbiterComponent.h"
#include "core/util/safeDelete.h"
#include "console/consoleTypes.h"
#include "console/consoleObject.h"
#include "core/stream/bitStream.h"
#include "console/engineAPI.h"
#include "sim/netConnection.h"
#include "math/mathUtils.h"

//////////////////////////////////////////////////////////////////////////
// Constructor/Destructor
//////////////////////////////////////////////////////////////////////////

CameraOrbiterComponent::CameraOrbiterComponent() : Component()
{
   mMinOrbitDist = 0.0f;
   mMaxOrbitDist = 0.0f;
   mCurOrbitDist = 8.0f;
   mPosition.set(0.0f, 0.0f, 0.0f);

   mMaxPitchAngle = 70;
   mMinPitchAngle = -10;

   mRotation.set(0, 0, 0);

   mCamera = NULL;
}

CameraOrbiterComponent::~CameraOrbiterComponent()
{
}

IMPLEMENT_CO_NETOBJECT_V1(CameraOrbiterComponent);

bool CameraOrbiterComponent::onAdd()
{
   if (!Parent::onAdd())
      return false;

   return true;
}

void CameraOrbiterComponent::onRemove()
{
   Parent::onRemove();
}
void CameraOrbiterComponent::initPersistFields()
{
   Parent::initPersistFields();

   addField("orbitDistance", TypeF32, Offset(mCurOrbitDist, CameraOrbiterComponent), "Object world orientation.");
   addField("Rotation", TypeRotationF, Offset(mRotation, CameraOrbiterComponent), "Object world orientation.");
   addField("maxPitchAngle", TypeF32, Offset(mMaxPitchAngle, CameraOrbiterComponent), "Object world orientation.");
   addField("minPitchAngle", TypeF32, Offset(mMinPitchAngle, CameraOrbiterComponent), "Object world orientation.");
}

//This is mostly a catch for situations where the behavior is re-added to the object and the like and we may need to force an update to the behavior
void CameraOrbiterComponent::onComponentAdd()
{
   Parent::onComponentAdd();

   CameraComponent *cam = mOwner->getComponent<CameraComponent>();
   if (cam)
   {
      mCamera = cam;
   }
}

void CameraOrbiterComponent::onComponentRemove()
{
   Parent::onComponentRemove();
}

U32 CameraOrbiterComponent::packUpdate(NetConnection *con, U32 mask, BitStream *stream)
{
   U32 retMask = Parent::packUpdate(con, mask, stream);
   return retMask;
}

void CameraOrbiterComponent::unpackUpdate(NetConnection *con, BitStream *stream)
{
   Parent::unpackUpdate(con, stream);
}

void CameraOrbiterComponent::processTick()
{
   Parent::processTick();

   if (!mOwner)
      return;

   if (mCamera)
   {
      Point3F pos;

      EulerF tempEul = mRotation.asEulerF();
      //tempEul.x = mClampF(tempEul.x, mDegToRad(mMinPitchAngle), mDegToRad(mMaxPitchAngle));

      //mRotation = tempEul;

      MatrixF ownerTrans = mOwner->getRenderTransform();
      Point3F ownerPos = ownerTrans.getPosition();

      pos.x = mCurOrbitDist * mSin(tempEul.x + mDegToRad(90.0f)) * mCos(-1.0f * (tempEul.z + mDegToRad(90.0f)));
      pos.y = mCurOrbitDist * mSin(tempEul.x + mDegToRad(90.0f)) * mSin(-1.0f * (tempEul.z + mDegToRad(90.0f)));
      pos.z = mCurOrbitDist * mSin(tempEul.x);

      RotationF xRot = EulerF(tempEul.x, 0, 0);
      RotationF zRot = EulerF(0, 0, tempEul.z);

      //orient the camera towards the owner
      /*VectorF ownerVec = ownerPos - pos;
      ownerVec.normalize();

      MatrixF xRot, zRot, cameraMatrix;
      xRot.set(EulerF(tempEul.x, 0.0f, 0.0f));
      zRot.set(EulerF(0.0f, 0.0f, tempEul.z));

      cameraMatrix.mul(zRot, xRot);
      cameraMatrix.getColumn(1, &ownerVec);
      cameraMatrix.setColumn(3, pos - ownerVec * pos);*/

      //RotationF camRot = RotationF(cameraMatrix);

      RotationF camRot;

      //camRot.lookAt(pos, ownerPos);

      camRot = zRot + xRot;

      //RotationF camRot = tempEul;

      EulerF camRotEul = camRot.asEulerF(RotationF::Degrees);

      //if (camRot != mCamera->getRotOffset())
         mCamera->setRotation(camRot);

      //
      //Verify we don't push through anything if we're not supposed to
      // Use the eye transform to orient the camera
      /*Point3F dir;
      mat->getColumn(1, &dir);
      if (mMaxOrbitDist - mMinOrbitDist > 0.0f)
         pos *= mMaxOrbitDist - mMinOrbitDist;

      // Use the camera node's pos.
      Point3F startPos = getRenderPosition();
      Point3F endPos;

      // Make sure we don't extend the camera into anything solid
      RayInfo collision;
      U32 mask = TerrainObjectType |
         WaterObjectType |
         StaticShapeObjectType |
         PlayerObjectType |
         ItemObjectType |
         VehicleObjectType;

      SceneContainer* pContainer = isServerObject() ? &gServerContainer : &gClientContainer;
      if (!pContainer->castRay(startPos, startPos - dir * 2.5 * pos, mask, &collision))
         endPos = startPos - dir * pos;
      else
      {
         float dot = mDot(dir, collision.normal);
         if (dot > 0.01f)
         {
            F32 colDist = mDot(startPos - collision.point, dir) - (1 / dot) * CameraRadius;
            if (colDist > pos)
               colDist = pos;
            if (colDist < 0.0f)
               colDist = 0.0f;
            endPos = startPos - dir * colDist;
         }
         else
            endPos = startPos - dir * pos;
      }
      mat->setColumn(3, endPos);*/
      //
      //if (pos != mCamera->getPosOffset())
         mCamera->setPosition(pos);
   }
}