#pragma once
#ifndef LEVEL_CELL_MANAGER_H
#define LEVEL_CELL_MANAGER_H

#ifndef _SCENEOBJECT_H_
#include "scene/sceneObject.h"
#endif
#ifndef _GFXSTATEBLOCK_H_
#include "gfx/gfxStateBlock.h"
#endif
#ifndef _GFXVERTEXBUFFER_H_
#include "gfx/gfxVertexBuffer.h"
#endif
#ifndef _GFXPRIMITIVEBUFFER_H_
#include "gfx/gfxPrimitiveBuffer.h"
#endif
#ifndef _TDICTIONARY_H_
#include "core/util/tDictionary.h"
#endif

class BaseMatInstance;


//-----------------------------------------------------------------------------
// This class implements a basic SceneObject that can exist in the world at a
// 3D position and render itself. Note that LevelCellManager handles its own
// rendering by submitting itself as an ObjectRenderInst (see
// renderInstance\renderPassmanager.h) along with a delegate for its render()
// function. However, the preffered rendering method in the engine is to submit
// a MeshRenderInst along with a Material, vertex buffer, primitive buffer, and
// transform and allow the RenderMeshMgr handle the actual rendering. You can
// see this implemented in RenderMeshExample.
//-----------------------------------------------------------------------------

class LevelCellManager : public SceneObject
{
	typedef SceneObject Parent;

public:
	// Networking masks
	// We need to implement at least one of these to allow
	// the client version of the object to receive updates
	// from the server version (like if it has been moved
	// or edited)
	enum MaskBits
	{
		TransformMask = Parent::NextFreeMask << 0,
		NextFreeMask = Parent::NextFreeMask << 1
	};

	//--------------------------------------------------------------------------
	// Rendering variables
	//--------------------------------------------------------------------------
	// Define our vertex format here so we don't have to
	// change it in multiple spots later
	typedef GFXVertexPCN VertexType;

	// The handles for our StateBlocks
	GFXStateBlockRef mNormalSB;
	GFXStateBlockRef mReflectSB;

	// The GFX vertex and primitive buffers
	GFXVertexBufferHandle< VertexType > mVertexBuffer;
	
	//
	F32 mVoxelSize;

	struct LevelCell
	{
		Box3F bounds;
		
		RectF rect;

		bool mDirty;

		Vector<SceneObject*> objectList;

		LevelCell()
		{
		}
	};

	typedef HashTable<Point2I, LevelCell*> BucketTable;
	BucketTable mCellTable;

	static const U32 CELL_DIM = 50; //50M

public:
	LevelCellManager();
	virtual ~LevelCellManager();

	// Declare this object as a ConsoleObject so that we can
	// instantiate it into the world and network it
	DECLARE_CONOBJECT(LevelCellManager);

	//--------------------------------------------------------------------------
	// Object Editing
	// Since there is always a server and a client object in Torque and we
	// actually edit the server object we need to implement some basic
	// networking functions
	//--------------------------------------------------------------------------
	// Set up any fields that we want to be editable (like position)
	static void initPersistFields();

	// Handle when we are added to the scene and removed from the scene
	bool onAdd();
	void onRemove();

	// Override this so that we can dirty the network flag when it is called
	void setTransform(const MatrixF &mat);

	virtual void setScale(const VectorF &scale);

	// This function handles sending the relevant data from the server
	// object to the client object
	U32 packUpdate(NetConnection *conn, U32 mask, BitStream *stream);
	// This function handles receiving relevant data from the server
	// object and applying it to the client object
	void unpackUpdate(NetConnection *conn, BitStream *stream);

	//--------------------------------------------------------------------------
	// Object Rendering
	// Torque utilizes a "batch" rendering system. This means that it builds a
	// list of objects that need to render (via RenderInst's) and then renders
	// them all in one batch. This allows it to optimized on things like
	// minimizing texture, state, and shader switching by grouping objects that
	// use the same Materials. For this example, however, we are just going to
	// get this object added to the list of objects that handle their own
	// rendering.
	//--------------------------------------------------------------------------
	// Create the geometry for rendering
	void createGeometry();

	// This is the function that allows this object to submit itself for rendering
	void prepRenderImage(SceneRenderState *state);

	// This is the function that actually gets called to do the rendering
	// Note that there is no longer a predefined name for this function.
	// Instead, when we submit our ObjectRenderInst in prepRenderImage(),
	// we bind this function as our rendering delegate function
	void render(ObjectRenderInst *ri, SceneRenderState *state, BaseMatInstance *overrideMat);

	void _renderCellBounds(ObjectRenderInst *ri, SceneRenderState *state, BaseMatInstance *overrideMat);

	//
	void addObject(SceneObject* object);
	void removeObject(SceneObject* object);
	void objectTransformChanged(SceneObject* object);

	Point3I  getVoxelCount();
	Point3F  getWorldSpaceVoxelSize();
	Point3I getVoxel(Point3F position);
	S32 getVoxelIndex(U32 x, U32 y, U32 z)
	{
		Point3I voxels = getVoxelCount();
		S32 offset = -1;

		if (x < voxels.x && y < voxels.y && z < voxels.z)
		{
			/*offset = (x*voxels.y*voxels.z + y*voxels.z + z);

			U32 otheroffset = x * (voxels.y * voxels.z) + y * (voxels.z) + z;

			bool hurp = true;*/

			offset = (voxels.x * voxels.y * z) + (voxels.x * y) + x;
		}

		return offset;
	}

	void getCells(Vector<LevelCell*> *outCells) const
	{
		PROFILE_SCOPE(LevelCellManager_getCells_nofrustum);

		BucketTable::ConstIterator iter = mCellTable.begin();
		for (; iter != mCellTable.end(); ++iter)
			outCells->push_back(iter->value);
	}

	RectF getAdjacentRect(Point3F position, U32 adjacentCellRadius) const
	{
		Point2I posKey = _getBucketKey(position);

		S32 x = posKey.x - adjacentCellRadius * CELL_DIM;
		S32 y = posKey.y - adjacentCellRadius * CELL_DIM;
		S32 ex = posKey.x + adjacentCellRadius * CELL_DIM * 2;
		S32 ey = posKey.y + adjacentCellRadius * CELL_DIM * 2;

		RectF adjRect = RectF(F32(x), F32(y), F32(ex), F32(ey));

		return adjRect;
	}

	void getAdjacentCells(Vector<LevelCell*> *outCells, RectF adjRect) const
	{
		PROFILE_SCOPE(LevelCellManager_getCells_nofrustum);

		BucketTable::ConstIterator iter = mCellTable.begin();
		for (; iter != mCellTable.end(); ++iter)
		{
			if(iter->value->rect.overlaps(adjRect))
				outCells->push_back(iter->value);
		}
	}

	inline Point2I _getBucketKey(const Point3F &pos) const
	{
		return Point2I((S32)mFloor(pos.x / CELL_DIM) * CELL_DIM,
			(S32)mFloor(pos.y / CELL_DIM) * CELL_DIM);
	}

	LevelCell* _findBucket(const Point2I &key) const
	{
		BucketTable::ConstIterator iter = mCellTable.find(key);

		if (iter != mCellTable.end())
			return iter->value;
		else
			return NULL;
	}

	inline LevelCell* _findBucket(const Point3F &pos) const
	{
		return _findBucket(_getBucketKey(pos));
	}

	LevelCell* _findOrCreateCell(const Point3F &pos)
	{
		// Look it up.
		const Point2I key = _getBucketKey(pos);
		BucketTable::Iterator iter = mCellTable.find(key);

		LevelCell *cell = NULL;
		if (iter != mCellTable.end())
			cell = iter->value;
		else
		{
			cell = new LevelCell();
			cell->rect = RectF(key.x, key.y, CELL_DIM, CELL_DIM);
			cell->bounds = Box3F::Invalid;
			mCellTable.insertUnique(key, cell);
			//mIsDirty = true;
		}

		return cell;
	}

	LevelCell* findCellByObject(SceneObject* obj) const
	{
		BucketTable::ConstIterator iter = mCellTable.begin();
		for (; iter != mCellTable.end(); ++iter)
		{
			for (U32 i = 0; i < iter->value->objectList.size(); i++)
			{
				if (iter->value->objectList[i]->getId() == obj->getId())
					return iter->value;
			}
		}

		return nullptr;
	}
};

#endif // _LevelCellManager_H_