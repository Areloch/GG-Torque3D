//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _CAMERA_BEHAVIOR_H_
#define _CAMERA_BEHAVIOR_H_

#include "component/components/component.h"

#ifndef __RESOURCE_H__
#include "core/resource.h"
#endif
#ifndef _TSSHAPE_H_
#include "ts/tsShape.h"
#endif
#ifndef _SCENERENDERSTATE_H_
#include "scene/sceneRenderState.h"
#endif
#ifndef _MBOX_H_
#include "math/mBox.h"
#endif
#ifndef _ACTOR_H_
#include "T3D/Entity.h"
#endif
#ifndef _STOCK_INTERFACES_H_
#include "component/components/stockInterfaces.h"
#endif

class TSShapeInstance;
class SceneRenderState;
class cameraComponentInstance;
struct CameraScopeQuery;
//////////////////////////////////////////////////////////////////////////
/// 
/// 
//////////////////////////////////////////////////////////////////////////
class cameraComponent : public Component
{
   typedef Component Parent;

public:
   cameraComponent();
   virtual ~cameraComponent();
   DECLARE_CONOBJECT(cameraComponent);

   virtual bool onAdd();
   virtual void onRemove();
   static void initPersistFields();

   virtual U32 packUpdate(NetConnection *con, U32 mask, BitStream *stream);
   virtual void unpackUpdate(NetConnection *con, BitStream *stream);

   //override to pass back a cameraComponentInstance
   virtual ComponentInstance *createInstance();
};

class cameraComponentInstance : public ComponentInstance,
   public CameraInterface
{
   typedef ComponentInstance Parent;

protected:
   F32  mCameraFov;           ///< The camera vertical FOV in degrees.

   Point2F mClientScreen;     ///< The dimensions of the client's screen. Used to calculate the aspect ratio.

   F32 mCameraDefaultFov;            ///< Default vertical FOV in degrees.
   F32 mCameraMinFov;                ///< Min vertical FOV allowed in degrees.
   F32 mCameraMaxFov;                ///< Max vertical FOV allowed in degrees.

   enum
   {
      FOVMask = Parent::NextFreeMask,
      NextFreeMask = Parent::NextFreeMask << 1,
   };

public:
   cameraComponentInstance(Component *btemplate = NULL);
   virtual ~cameraComponentInstance();
   DECLARE_CONOBJECT(cameraComponentInstance);

   virtual bool onAdd();
   virtual void onRemove();
   static void initPersistFields();

   virtual void onComponentAdd();
   virtual void onComponentRemove();

   virtual U32 packUpdate(NetConnection *con, U32 mask, BitStream *stream);
   virtual void unpackUpdate(NetConnection *con, BitStream *stream);

   static bool _setCameraFov( void *object, const char *index, const char *data );

   /// Gets the minimum viewing distance, maximum viewing distance, camera offsetand rotation
   /// for this object, if the world were to be viewed through its eyes
   /// @param   min   Minimum viewing distance
   /// @param   max   Maximum viewing distance
   /// @param   offset Offset of the camera from the origin in local space
   /// @param   rot   Rotation matrix
   virtual void getCameraParameters(F32 *min,F32* max,Point3F* offset,MatrixF* rot);

   /// Gets the camera to world space transform matrix
   /// @todo Find out what pos does
   /// @param   pos   TODO: Find out what this does
   /// @param   mat   Camera transform (out)
   virtual bool getCameraTransform(F32* pos,MatrixF* mat);

   /// Returns the vertical field of view in degrees for 
   /// this object if used as a camera.
   virtual F32 getCameraFov() { return mCameraFov; }

   /// Returns the default vertical field of view in degrees
   /// if this object is used as a camera.
   virtual F32 getDefaultCameraFov() { return mCameraDefaultFov; }

   /// Sets the vertical field of view in degrees for this 
   /// object if used as a camera.
   /// @param   yfov  The vertical FOV in degrees to test.
   virtual void setCameraFov(F32 fov);

   /// Returns true if the vertical FOV in degrees is within 
   /// allowable parameters of the datablock.
   /// @param   yfov  The vertical FOV in degrees to test.
   /// @see ShapeBaseData::cameraMinFov
   /// @see ShapeBaseData::cameraMaxFov
   virtual bool isValidCameraFov(F32 fov);
   /// @}

   /// Control object scoping
   void onCameraScopeQuery(NetConnection *cr, CameraScopeQuery *camInfo);

   virtual Frustum getFrustum();

protected:
   DECLARE_CALLBACK( F32, validateCameraFov, (F32 fov) );

};

#endif // _COMPONENT_H_
