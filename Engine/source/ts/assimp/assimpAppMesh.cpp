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

#include "platform/platform.h"
#include "ts/collada/colladaExtensions.h"
#include "ts/assimp/assimpAppMesh.h"
#include "ts/assimp/assimpAppNode.h"

// assimp include files. 
#include <assimp/cimport.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/types.h>

//------------------------------------------------------------------------------

AssimpAppMesh::AssimpAppMesh(const struct aiMesh* mesh, AssimpAppNode* node)
   : mMeshData(mesh), appNode(node)
{
   Con::printf("[ASSIMP] Mesh Created: %s", getName());
}

const char* AssimpAppMesh::getName(bool allowFixed)
{
   // Some exporters add a 'PIVOT' or unnamed node between the mesh and the
   // actual object node. Detect this and return the object node name instead
   // of the pivot node.
   const char* nodeName = appNode->getName();
   if ( dStrEqual(nodeName, "null") || dStrEndsWith(nodeName, "PIVOT") )
      nodeName = appNode->getParentName();

   // If all geometry is being fixed to the same size, append the size
   // to the name
   //return allowFixed && fixedSizeEnabled ? avar("%s %d", nodeName, fixedSize) : nodeName;
   return nodeName;
}

MatrixF AssimpAppMesh::getMeshTransform(F32 time)
{
   return appNode->getNodeTransform(time);
}

void AssimpAppMesh::lockMesh(F32 t, const MatrixF& objectOffset)
{
   // After this function, the following are expected to be populated:
   //     points, normals, uvs, primitives, indices
   // There is also colors and uv2s but those don't seem to be required.

   bool noUVFound = false;
   for ( U32 n = 0; n < mMeshData->mNumVertices; ++n)
   {
      // Points and Normals
      aiVector3D pt = mMeshData->mVertices[n];
      aiVector3D nrm = mMeshData->mNormals[n];

      // Check for Y Z Swap
      if ( Con::getBoolVariable("$Assimp::SwapYZ", false) )
      {
         points.push_back(Point3F(pt.x, pt.z, pt.y));
         normals.push_back(Point3F(nrm.x, nrm.z, nrm.y));
      } else {
         points.push_back(Point3F(pt.x, pt.y, pt.z));
         normals.push_back(Point3F(nrm.x, nrm.y, nrm.z));
      }

      // UVs
      //aiVector3D uv = mMeshData-> mTextureCoords[0][n];
      if ( mMeshData->HasTextureCoords(0) )
      {
         uvs.push_back(Point2F(mMeshData->mTextureCoords[0][n].x, mMeshData->mTextureCoords[0][n].y));
      } else {
         // I don't know if there's any solution to this issue.
         // If it's not mapped, it's not mapped.
         noUVFound = true;
         uvs.push_back(Point2F(1, 1));
      }

      // UV2s
      if ( mMeshData->HasTextureCoords(1) )
      {
         uv2s.push_back(Point2F(mMeshData->mTextureCoords[1][n].x, mMeshData->mTextureCoords[1][n].y));
      }

      // Vertex Colors
      if ( mMeshData->HasVertexColors(0) )
      {
         ColorF vColor(mMeshData->mColors[0][n].r,
            mMeshData->mColors[0][n].g,
            mMeshData->mColors[0][n].b,
            mMeshData->mColors[0][n].a);
         colors.push_back(vColor);
      }
   }

   for ( U32 n = 0; n < mMeshData->mNumFaces; ++n)
   {
      const struct aiFace* face = &mMeshData->mFaces[n];
      if ( face->mNumIndices == 3 )
      {
         // Create TSMesh primitive
         primitives.increment();
         TSDrawPrimitive& primitive = primitives.last();
         primitive.start = indices.size();
         primitive.matIndex = (TSDrawPrimitive::Triangles | TSDrawPrimitive::Indexed) | (S32)mMeshData->mMaterialIndex;
         primitive.numElements = 3;

         // Load the indices in.
         indices.push_back(face->mIndices[0]);
         indices.push_back(face->mIndices[1]);
         indices.push_back(face->mIndices[2]);
      } else {
         Con::printf("[ASSIMP] Non-Triangle Face Found. Indices: %d", face->mNumIndices);
      }
   }

   if ( noUVFound )
      Con::warnf("[ASSIMP] No UV Data for mesh.");
}

void AssimpAppMesh::lookupSkinData()
{

}

F32 AssimpAppMesh::getVisValue(F32 t)
{
   return 1.0f;
}