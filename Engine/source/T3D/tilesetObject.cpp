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

#include "T3D/tilesetObject.h"

#include "math/mathIO.h"
#include "scene/sceneRenderState.h"
#include "core/stream/bitStream.h"
#include "materials/sceneData.h"
#include "gfx/gfxDebugEvent.h"
#include "gfx/gfxTransformSaver.h"
#include "renderInstance/renderPassManager.h"


IMPLEMENT_CO_NETOBJECT_V1(TilesetObject);

ConsoleDocClass( TilesetObject, 
   "@brief An example scene object which renders using a callback.\n\n"
   "This class implements a basic SceneObject that can exist in the world at a "
   "3D position and render itself. Note that TilesetObject handles its own "
   "rendering by submitting itself as an ObjectRenderInst (see "
   "renderInstance\renderPassmanager.h) along with a delegate for its render() "
   "function. However, the preffered rendering method in the engine is to submit "
   "a MeshRenderInst along with a Material, vertex buffer, primitive buffer, and "
   "transform and allow the RenderMeshMgr handle the actual rendering. You can "
   "see this implemented in RenderMeshExample.\n\n"
   "See the C++ code for implementation details.\n\n"
   "@ingroup Examples\n" );

//-----------------------------------------------------------------------------
// Object setup and teardown
//-----------------------------------------------------------------------------
TilesetObject::TilesetObject()
{
   // Flag this object so that it will always
   // be sent across the network to clients
   mNetFlags.set( Ghostable | ScopeAlways );

   // Set it as a "static" object
   mTypeMask |= StaticObjectType | StaticShapeObjectType;
}

TilesetObject::~TilesetObject()
{
}

//-----------------------------------------------------------------------------
// Object Editing
//-----------------------------------------------------------------------------
void TilesetObject::initPersistFields()
{
   // SceneObject already handles exposing the transform
   Parent::initPersistFields();
}

bool TilesetObject::onAdd()
{
   if ( !Parent::onAdd() )
      return false;

   // Set up a 1x1x1 bounding box
   mObjBox.set( Point3F( -0.5f, -0.5f, -0.5f ),
                Point3F(  0.5f,  0.5f,  0.5f ) );

   resetWorldBox();

   // Add this object to the scene
   addToScene();

   if (mLevels.empty())
   {
      mLevels.increment();

      S32 tileCountHalf = 50;

      for (S32 i = -tileCountHalf; i < tileCountHalf; ++i)
      {
         for (S32 j = -tileCountHalf; j < tileCountHalf; ++j)
         {
            //make a tile!
            Tile newTile;
            newTile.levelCoords = Point2F(i * mLevels[0].tileWidth, j * mLevels[0].tileWidth);

            mLevels[0].tiles.push_back(newTile);
         }
      }
   }

   return true;
}

void TilesetObject::onRemove()
{
   // Remove this object from the scene
   removeFromScene();

   Parent::onRemove();
}

void TilesetObject::setTransform(const MatrixF & mat)
{
   // Let SceneObject handle all of the matrix manipulation
   Parent::setTransform( mat );

   // Dirty our network mask so that the new transform gets
   // transmitted to the client object
   setMaskBits( TransformMask );
}

U32 TilesetObject::packUpdate( NetConnection *conn, U32 mask, BitStream *stream )
{
   // Allow the Parent to get a crack at writing its info
   U32 retMask = Parent::packUpdate( conn, mask, stream );

   // Write our transform information
   if ( stream->writeFlag( mask & TransformMask ) )
   {
      mathWrite(*stream, getTransform());
      mathWrite(*stream, getScale());
   }

   return retMask;
}

void TilesetObject::unpackUpdate(NetConnection *conn, BitStream *stream)
{
   // Let the Parent read any info it sent
   Parent::unpackUpdate(conn, stream);

   if ( stream->readFlag() )  // TransformMask
   {
      mathRead(*stream, &mObjToWorld);
      mathRead(*stream, &mObjScale);

      setTransform( mObjToWorld );
   }
}