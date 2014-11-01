#ifndef _STOCK_INTERFACES_H_
#define _STOCK_INTERFACES_H_

#ifndef _SCENERENDERSTATE_H_
#include "scene/sceneRenderState.h"
#endif
/*#ifndef _GEOMETRY_H_
#include "math/mGeometry.h"
#endif*/

//Basically a file for generic interfaces that many behaviors may make use of
class SetTransformInterface
{
public:
   virtual void setTransform( MatrixF transform );
   virtual void setTransform( Point3F pos, EulerF rot );
   //void setTransform( TransformF transform );
};

class UpdateInterface
{
public:
   virtual void processTick(const Move* move){}
   virtual void interpolateTick(F32 dt){}
   virtual void advanceTime(F32 dt){}
};

class BehaviorFieldInterface
{
public:
   virtual void onFieldChange(const char* fieldName, const char* newValue){};
};

class CameraInterface
{
public:
   virtual bool getCameraTransform(F32* pos,MatrixF* mat)=0;
   virtual void onCameraScopeQuery(NetConnection *cr, CameraScopeQuery * query)=0;
   virtual Frustum getFrustum()=0;
   virtual F32 getCameraFov()=0;
   virtual void setCameraFov(F32 fov)=0;

   virtual bool isValidCameraFov(F32 fov)=0;
};

class CastRayInterface
{
public:
   virtual bool castRay(const Point3F &start, const Point3F &end, RayInfo* info)=0;
};

class EditorInspectInterface
{
public:
   virtual void onInspect()=0;
   virtual void onEndInspect()=0;
};

#endif