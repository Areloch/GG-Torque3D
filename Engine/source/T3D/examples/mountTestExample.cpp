//-----------------------------------------------------------------------------
// Copyright (c) 2012 GarageGames, LLC
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.
//-----------------------------------------------------------------------------

#include "T3D/examples/MountTestExample.h"

#include "math/mathIO.h"
#include "sim/netConnection.h"
#include "scene/sceneRenderState.h"
#include "console/consoleTypes.h"
#include "core/resourceManager.h"
#include "core/stream/bitStream.h"
#include "gfx/gfxTransformSaver.h"
#include "renderInstance/renderPassManager.h"
#include "lighting/lightQuery.h"


IMPLEMENT_CO_NETOBJECT_V1(MountTestExample);

ConsoleDocClass( MountTestExample, 
   "@brief An example scene object which renders a DTS.\n\n"
   "This class implements a basic SceneObject that can exist in the world at a "
   "3D position and render itself. There are several valid ways to render an "
   "object in Torque. This class makes use of the 'TS' (three space) shape "
   "system. TS manages loading the various mesh formats supported by Torque as "
   "well was rendering those meshes (including LOD and animation...though this "
   "example doesn't include any animation over time).\n\n"
   "See the C++ code for implementation details.\n\n"
   "@ingroup Examples\n" );

//-----------------------------------------------------------------------------
// Object setup and teardown
//-----------------------------------------------------------------------------
MountTestExample::MountTestExample()
{
   // Flag this object so that it will always
   // be sent across the network to clients
   mNetFlags.set( Ghostable | ScopeAlways );

   // Set it as a "static" object.
   mTypeMask |= StaticObjectType | StaticShapeObjectType;

   // Make sure to initialize our TSShapeInstance to NULL
   mShapeInstance = NULL;

   mPos = Point3F::Zero;
   mRot = Point3F::Zero;
}

MountTestExample::~MountTestExample()
{
}

//-----------------------------------------------------------------------------
// Object Editing
//-----------------------------------------------------------------------------
void MountTestExample::initPersistFields()
{
   addGroup( "Rendering" );
   addField( "shapeFile",      TypeStringFilename, Offset( mShapeFile, MountTestExample ),
      "The path to the DTS shape file." );
   endGroup( "Rendering" );

   removeField("Position");
   addProtectedField("Position", TypePoint3F, Offset(mPos, MountTestExample), &_setPosition, &_getPosition, "Object world orientation.");

   removeField("Rotation");
   addProtectedField("Rotation", TypeRotationF, Offset(mRot, MountTestExample), &_setRotation, &_getRotation, "Object world orientation.");

   // SceneObject already handles exposing the transform
   Parent::initPersistFields();
}

void MountTestExample::inspectPostApply()
{
   Parent::inspectPostApply();

   // Flag the network mask to send the updates
   // to the client object
   setMaskBits( UpdateMask );
}

bool MountTestExample::_setPosition(void *object, const char *index, const char *data)
{
   MountTestExample* so = static_cast<MountTestExample*>(object);
   if (so)
   {
      Point3F pos;

      if (!dStrcmp(data, ""))
         pos = Point3F(0, 0, 0);
      else
         Con::setData(TypePoint3F, &pos, 0, 1, &data);

      so->setTransform(pos, so->mRot);
   }
   return false;
}

const char * MountTestExample::_getPosition(void* obj, const char* data)
{
   MountTestExample* so = static_cast<MountTestExample*>(obj);
   if (so)
   {
      Point3F pos = so->getPosition();

      static const U32 bufSize = 256;
      char* returnBuffer = Con::getReturnBuffer(bufSize);
      dSprintf(returnBuffer, bufSize, "%g %g %g", pos.x, pos.y, pos.z);
      return returnBuffer;
   }
   return "0 0 0";
}

bool MountTestExample::_setRotation(void *object, const char *index, const char *data)
{
   MountTestExample* so = static_cast<MountTestExample*>(object);
   if (so)
   {
      RotationF rot;
      Con::setData(TypeRotationF, &rot, 0, 1, &data);

      //so->mRot = rot;
      //MatrixF mat = rot.asMatrixF();
      //mat.setPosition(so->getPosition());
      //so->setTransform(mat);
      so->setTransform(so->getPosition(), rot);
   }
   return false;
}

const char * MountTestExample::_getRotation(void* obj, const char* data)
{
   MountTestExample* so = static_cast<MountTestExample*>(obj);
   if (so)
   {
      EulerF eulRot = so->mRot.asEulerF();

      static const U32 bufSize = 256;
      char* returnBuffer = Con::getReturnBuffer(bufSize);
      dSprintf(returnBuffer, bufSize, "%g %g %g", mRadToDeg(eulRot.x), mRadToDeg(eulRot.y), mRadToDeg(eulRot.z));
      return returnBuffer;
   }
   return "0 0 0";
}


bool MountTestExample::onAdd()
{
   if ( !Parent::onAdd() )
      return false;

   // Set up a 1x1x1 bounding box
   mObjBox.set( Point3F( -0.5f, -0.5f, -0.5f ),
                Point3F(  0.5f,  0.5f,  0.5f ) );

   resetWorldBox();

   // Add this object to the scene
   addToScene();

   setProcessTick(true);

   // Setup the shape.
   createShape();

   return true;
}

void MountTestExample::onRemove()
{
   // Remove this object from the scene
   removeFromScene();

   // Remove our TSShapeInstance
   if ( mShapeInstance )
      SAFE_DELETE( mShapeInstance );

   Parent::onRemove();
}

void MountTestExample::setTransform(const MatrixF & mat)
{
   // This test is a bit expensive so turn it off in release.   
#ifdef TORQUE_DEBUG
   //AssertFatal( mat.isAffine(), "SceneObject::setTransform() - Bad transform (non affine)!" );
#endif

   //PROFILE_SCOPE(SceneObject_setTransform);

   // Update the transforms.

   mObjToWorld = mWorldToObj = mat;
   mWorldToObj.affineInverse();

   mPos = mObjToWorld.getPosition();
   mRot = mObjToWorld;

   // Update the world-space AABB.

   resetWorldBox();

   // If we're in a SceneManager, sync our scene state.

   if (mSceneManager != NULL)
      mSceneManager->notifyObjectDirty(this);

   setRenderTransform(mat);

   // Dirty our network mask so that the new transform gets
   // transmitted to the client object
   setMaskBits( TransformMask );
}

void MountTestExample::setRenderTransform(const MatrixF & mat)
{
   //PROFILE_START(SceneObj_setRenderTransform);
   mRenderObjToWorld = mRenderWorldToObj = mat;
   mRenderWorldToObj.affineInverse();

   AssertFatal(mObjBox.isValidBox(), "Bad object box!");
   resetRenderWorldBox();
   //PROFILE_END();
}

void MountTestExample::setTransform(Point3F position, RotationF rotation)
{
   mPos = position;
   mRot = rotation;

   MatrixF mat = rotation.asMatrixF();
   mat.setPosition(position);
   mObjToWorld = mat;
   setTransform(mObjToWorld);
   setRenderTransform(mObjToWorld);
   return;

   MatrixF oldTransform = getTransform();

   if (isMounted())
   {
      mPos = position;
      mRot = rotation;

      RotationF addRot = mRot + RotationF(mMount.object->getTransform());
      MatrixF transf = addRot.asMatrixF();
      transf.setPosition(mPos + mMount.object->getPosition());

      Parent::setTransform(transf);

      if (transf != oldTransform)
         setMaskBits(TransformMask);
   }
   else
   {
      /*MatrixF newMat, imat, xmat, ymat, zmat;
      Point3F radRot = Point3F(mDegToRad(rotation.x), mDegToRad(rotation.y), mDegToRad(rotation.z));
      xmat.set(EulerF(radRot.x, 0, 0));
      ymat.set(EulerF(0.0f, radRot.y, 0.0f));
      zmat.set(EulerF(0, 0, radRot.z));
      imat.mul(zmat, xmat);
      newMat.mul(imat, ymat);*/

      MatrixF newMat = rotation.asMatrixF();

      newMat.setColumn(3, position);

      mPos = position;
      mRot = rotation;

      //if (isServerObject())
      //   setMaskBits(TransformMask);

      //setTransform(temp);

      // This test is a bit expensive so turn it off in release.   
#ifdef TORQUE_DEBUG
      //AssertFatal( mat.isAffine(), "SceneObject::setTransform() - Bad transform (non affine)!" );
#endif

      //PROFILE_SCOPE(Entity_setTransform);

      // Update the transforms.

      Parent::setTransform(newMat);

      Point3F newPos = newMat.getPosition();
      RotationF newRot = newMat;

      Point3F oldPos = oldTransform.getPosition();
      RotationF oldRot = oldTransform;

      if (newPos != oldPos || newRot != oldRot)
         setMaskBits(TransformMask);

      /*mObjToWorld = mWorldToObj = newMat;
      mWorldToObj.affineInverse();
      // Update the world-space AABB.
      resetWorldBox();
      // If we're in a SceneManager, sync our scene state.
      if (mSceneManager != NULL)
      mSceneManager->notifyObjectDirty(this);
      setRenderTransform(newMat);*/
   }
}

void MountTestExample::setRenderTransform(Point3F position, RotationF rotation)
{
   MatrixF mat = rotation.asMatrixF();
   mat.setPosition(position);
   Parent::setRenderTransform(mat);
   return;

   if (isMounted())
   {
      mPos = position;
      mRot = rotation;

      RotationF addRot = mRot + RotationF(mMount.object->getTransform());
      MatrixF transf = addRot.asMatrixF();
      transf.setPosition(mPos + mMount.object->getPosition());

      Parent::setRenderTransform(transf);
   }
   else
   {
      MatrixF newMat = rotation.asMatrixF();

      newMat.setColumn(3, position);

      mPos = position;
      mRot = rotation;

      Parent::setRenderTransform(newMat);
   }
}

U32 MountTestExample::packUpdate( NetConnection *conn, U32 mask, BitStream *stream )
{
   // Allow the Parent to get a crack at writing its info
   U32 retMask = Parent::packUpdate( conn, mask, stream );

   // Write our transform information
   if ( stream->writeFlag( mask & TransformMask ) )
   {
      //mathWrite(*stream, getTransform());
      stream->writeCompressedPoint(mPos);
      mathWrite(*stream, mRot);
      mathWrite(*stream, getScale());
   }

   // Write out any of the updated editable properties
   if ( stream->writeFlag( mask & UpdateMask ) )
   {
      stream->write( mShapeFile );

      // Allow the server object a chance to handle a new shape
      createShape();
   }

   return retMask;
}

void MountTestExample::unpackUpdate(NetConnection *conn, BitStream *stream)
{
   // Let the Parent read any info it sent
   Parent::unpackUpdate(conn, stream);

   if ( stream->readFlag() )  // TransformMask
   {
      //mathRead(*stream, &mObjToWorld);
      Point3F pos;
      RotationF rot;
      stream->readCompressedPoint(&pos);
      mathRead(*stream, &rot);

      setTransform(pos, rot);
      mathRead(*stream, &mObjScale);

      //setTransform( mObjToWorld );
   }

   if ( stream->readFlag() )  // UpdateMask
   {
      stream->read( &mShapeFile );

      if ( isProperlyAdded() )
         createShape();
   }
}

//-----------------------------------------------------------------------------
// Object Rendering
//-----------------------------------------------------------------------------
void MountTestExample::createShape()
{
   if ( mShapeFile.isEmpty() )
      return;

   // If this is the same shape then no reason to update it
   if ( mShapeInstance && mShapeFile.equal( mShape.getPath().getFullPath(), String::NoCase ) )
      return;

   // Clean up our previous shape
   if ( mShapeInstance )
      SAFE_DELETE( mShapeInstance );
   mShape = NULL;

   // Attempt to get the resource from the ResourceManager
   mShape = ResourceManager::get().load( mShapeFile );

   if ( !mShape )
   {
      Con::errorf( "MountTestExample::createShape() - Unable to load shape: %s", mShapeFile.c_str() );
      return;
   }

   // Attempt to preload the Materials for this shape
   if ( isClientObject() && 
        !mShape->preloadMaterialList( mShape.getPath() ) && 
        NetConnection::filesWereDownloaded() )
   {
      mShape = NULL;
      return;
   }

   // Update the bounding box
   mObjBox = mShape->bounds;
   resetWorldBox();
   setRenderTransform(mObjToWorld.getPosition(), mObjToWorld);

   // Create the TSShapeInstance
   mShapeInstance = new TSShapeInstance( mShape, isClientObject() );
}

void MountTestExample::prepRenderImage( SceneRenderState *state )
{
   // Make sure we have a TSShapeInstance
   if ( !mShapeInstance )
      return;

   // Calculate the distance of this object from the camera
   Point3F cameraOffset;
   getRenderTransform().getColumn( 3, &cameraOffset );
   cameraOffset -= state->getDiffuseCameraPosition();
   F32 dist = cameraOffset.len();
   if ( dist < 0.01f )
      dist = 0.01f;

   // Set up the LOD for the shape
   F32 invScale = ( 1.0f / getMax( getMax( mObjScale.x, mObjScale.y ), mObjScale.z ) );

   mShapeInstance->setDetailFromDistance( state, dist * invScale );

   // Make sure we have a valid level of detail
   if ( mShapeInstance->getCurrentDetail() < 0 )
      return;

   // GFXTransformSaver is a handy helper class that restores
   // the current GFX matrices to their original values when
   // it goes out of scope at the end of the function
   GFXTransformSaver saver;

   // Set up our TS render state      
   TSRenderState rdata;
   rdata.setSceneState( state );
   rdata.setFadeOverride( 1.0f );

   // We might have some forward lit materials
   // so pass down a query to gather lights.
   LightQuery query;
   query.init( getWorldSphere() );
   rdata.setLightQuery( &query );

   // Set the world matrix to the objects render transform
   MatrixF mat = getRenderTransform();
   mat.scale( mObjScale );
   GFX->setWorldMatrix( mat );

   if (isMounted())
   {
      Point3F pos = mat.getPosition();

      bool tur = true;
   }

   // Animate the the shape
   mShapeInstance->animate();

   // Allow the shape to submit the RenderInst(s) for itself
   mShapeInstance->render( rdata );
}

void MountTestExample::processTick(const Move* move)
{
   if (isMounted()) 
   {
      MatrixF mat;
      mMount.object->getMountTransform(mMount.node, mMount.xfm, &mat);
      Parent::setTransform(mat);
   }
}

void MountTestExample::advanceTime(F32 dt)
{
   if (isMounted()) 
   {
      MatrixF mat;
      mMount.object->getRenderMountTransform(0.0f, mMount.node, mMount.xfm, &mat);
      Parent::setRenderTransform(mat);
   }
}

void MountTestExample::interpolateTick(F32 dt)
{
   Parent::interpolateTick(dt);
}

void MountTestExample::mountObject(SceneObject* objB, MatrixF txfm)
{
   Parent::mountObject(objB, -1, txfm);
   Parent::addObject(objB);
}

void MountTestExample::getMountTransform(S32 index, const MatrixF &xfm, MatrixF *outMat)
{
   if (mShapeInstance)
   {
      mShapeInstance->animate();
      S32 nodeCount = mShapeInstance->getShape()->nodes.size();

      if (index >= 0 && index < nodeCount)
      {
         MatrixF mountTransform = mShapeInstance->mNodeTransforms[index];
         mountTransform.mul(xfm);
         const Point3F& scale = getScale();

         // The position of the mount point needs to be scaled.
         Point3F position = mountTransform.getPosition();
         position.convolve(scale);
         mountTransform.setPosition(position);

         // Also we would like the object to be scaled to the model.
         outMat->mul(mObjToWorld, mountTransform);
         return;
      }
   }

   // Then let SceneObject handle it.
   Parent::getMountTransform(index, xfm, outMat);
}

void MountTestExample::getRenderMountTransform(F32 delta, S32 index, const MatrixF &xfm, MatrixF *outMat)
{
   if(mShapeInstance)
   {
      mShapeInstance->animate();
      S32 nodeCount = mShapeInstance->getShape()->nodes.size();

      if (index >= 0 && index < nodeCount)
      {
         MatrixF mountTransform = mShapeInstance->mNodeTransforms[index];
         mountTransform.mul(xfm);
         const Point3F& scale = getScale();

         // The position of the mount point needs to be scaled.
         Point3F position = mountTransform.getPosition();
         position.convolve(scale);
         mountTransform.setPosition(position);

         // Also we would like the object to be scaled to the model.
         outMat->mul(getRenderTransform(), mountTransform);
         return;
      }
   }

   // Then let SceneObject handle it.
   Parent::getMountTransform(index, xfm, outMat);
}

DefineEngineMethod(MountTestExample, mountObject, bool, (SceneObject* objB), (nullAsType<SceneObject*>()),
"@brief Mount objB to this object at the desired slot with optional transform.\n\n"

"@param objB  Object to mount onto us\n"
"@param slot  Mount slot ID\n"
"@param txfm (optional) mount offset transform\n"
"@return true if successful, false if failed (objB is not valid)")
{
   if (objB)
   {
      object->mountObject(objB, MatrixF::Identity);
      return true;
   }

   return false;
}