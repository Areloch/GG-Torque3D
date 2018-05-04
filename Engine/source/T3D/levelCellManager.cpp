#include "T3D/levelCellManager.h"

#include "math/mathIO.h"
#include "scene/sceneRenderState.h"
#include "core/stream/bitStream.h"
#include "materials/sceneData.h"
#include "gfx/gfxDebugEvent.h"
#include "gfx/gfxTransformSaver.h"
#include "renderInstance/renderPassManager.h"
#include "gfx/gfxDrawUtil.h"
#include "gfx/primBuilder.h"
#include "math/mathUtils.h"

IMPLEMENT_CO_NETOBJECT_V1(LevelCellManager);

//-----------------------------------------------------------------------------
// Object setup and teardown
//-----------------------------------------------------------------------------
LevelCellManager::LevelCellManager()
{
	// Flag this object so that it will always
	// be sent across the network to clients
	mNetFlags.set(Ghostable | ScopeAlways);

	// Set it as a "static" object
	mTypeMask |= StaticObjectType | StaticShapeObjectType;

	mVoxelSize = 16; //16meters default
}

LevelCellManager::~LevelCellManager()
{
}

//-----------------------------------------------------------------------------
// Object Editing
//-----------------------------------------------------------------------------
void LevelCellManager::initPersistFields()
{
	// SceneObject already handles exposing the transform
	Parent::initPersistFields();
}

bool LevelCellManager::onAdd()
{
	if (!Parent::onAdd())
		return false;

	setGlobalBounds();

	// Set up a 1x1x1 bounding box
	mObjBox.set(Point3F(-0.5f, -0.5f, -0.5f),
		Point3F(0.5f, 0.5f, 0.5f));

	resetWorldBox();

	// Add this object to the scene
	addToScene();

	SceneObject::smSceneObjectAdd.notify(this, &LevelCellManager::addObject);
	SceneObject::smSceneObjectRemove.notify(this, &LevelCellManager::removeObject);
	SceneObject::smSceneObjectTransformChanged.notify(this, &LevelCellManager::objectTransformChanged);

	return true;
}

void LevelCellManager::onRemove()
{
	// Remove this object from the scene
	removeFromScene();

	Parent::onRemove();
}

void LevelCellManager::setTransform(const MatrixF & mat)
{
	// Let SceneObject handle all of the matrix manipulation
	Parent::setTransform(mat);

	// Dirty our network mask so that the new transform gets
	// transmitted to the client object
	setMaskBits(TransformMask);
}

void LevelCellManager::setScale(const VectorF &scale)
{
	Parent::setScale(scale);
}

U32 LevelCellManager::packUpdate(NetConnection *conn, U32 mask, BitStream *stream)
{
	// Allow the Parent to get a crack at writing its info
	U32 retMask = Parent::packUpdate(conn, mask, stream);

	// Write our transform information
	if (stream->writeFlag(mask & TransformMask))
	{
		mathWrite(*stream, getTransform());
		mathWrite(*stream, getScale());
	}

	return retMask;
}

void LevelCellManager::unpackUpdate(NetConnection *conn, BitStream *stream)
{
	// Let the Parent read any info it sent
	Parent::unpackUpdate(conn, stream);

	if (stream->readFlag())  // TransformMask
	{
		mathRead(*stream, &mObjToWorld);
		mathRead(*stream, &mObjScale);

		setTransform(mObjToWorld);
	}
}

//-----------------------------------------------------------------------------
// Object Rendering
//-----------------------------------------------------------------------------
void LevelCellManager::createGeometry()
{
	static const Point3F cubePoints[8] =
	{
		Point3F(1.0f, -1.0f, -1.0f), Point3F(1.0f, -1.0f,  1.0f),
		Point3F(1.0f,  1.0f, -1.0f), Point3F(1.0f,  1.0f,  1.0f),
		Point3F(-1.0f, -1.0f, -1.0f), Point3F(-1.0f,  1.0f, -1.0f),
		Point3F(-1.0f, -1.0f,  1.0f), Point3F(-1.0f,  1.0f,  1.0f)
	};

	static const Point3F cubeNormals[6] =
	{
		Point3F(1.0f,  0.0f,  0.0f), Point3F(-1.0f,  0.0f,  0.0f),
		Point3F(0.0f,  1.0f,  0.0f), Point3F(0.0f, -1.0f,  0.0f),
		Point3F(0.0f,  0.0f,  1.0f), Point3F(0.0f,  0.0f, -1.0f)
	};

	static const ColorI cubeColors[3] =
	{
		ColorI(255,   0,   0, 255),
		ColorI(0, 255,   0, 255),
		ColorI(0,   0, 255, 255)
	};

	static const U32 cubeFaces[36][3] =
	{
		{ 3, 0, 0 },{ 0, 0, 0 },{ 1, 0, 0 },
		{ 2, 0, 0 },{ 0, 0, 0 },{ 3, 0, 0 },
		{ 7, 1, 0 },{ 4, 1, 0 },{ 5, 1, 0 },
		{ 6, 1, 0 },{ 4, 1, 0 },{ 7, 1, 0 },
		{ 3, 2, 1 },{ 5, 2, 1 },{ 2, 2, 1 },
		{ 7, 2, 1 },{ 5, 2, 1 },{ 3, 2, 1 },
		{ 1, 3, 1 },{ 4, 3, 1 },{ 6, 3, 1 },
		{ 0, 3, 1 },{ 4, 3, 1 },{ 1, 3, 1 },
		{ 3, 4, 2 },{ 6, 4, 2 },{ 7, 4, 2 },
		{ 1, 4, 2 },{ 6, 4, 2 },{ 3, 4, 2 },
		{ 2, 5, 2 },{ 4, 5, 2 },{ 0, 5, 2 },
		{ 5, 5, 2 },{ 4, 5, 2 },{ 2, 5, 2 }
	};

	// Fill the vertex buffer
	VertexType *pVert = NULL;

	mVertexBuffer.set(GFX, 36, GFXBufferTypeStatic);
	pVert = mVertexBuffer.lock();

	Point3F halfSize = getObjBox().getExtents() * 0.5f;

	for (U32 i = 0; i < 36; i++)
	{
		const U32& vdx = cubeFaces[i][0];
		const U32& ndx = cubeFaces[i][1];
		const U32& cdx = cubeFaces[i][2];

		pVert[i].point = cubePoints[vdx] * halfSize;
		pVert[i].normal = cubeNormals[ndx];
		pVert[i].color = cubeColors[cdx];
	}

	mVertexBuffer.unlock();

	// Set up our normal and reflection StateBlocks
	GFXStateBlockDesc desc;

	// The normal StateBlock only needs a default StateBlock
	mNormalSB = GFX->createStateBlock(desc);

	// The reflection needs its culling reversed
	desc.cullDefined = true;
	desc.cullMode = GFXCullCW;
	mReflectSB = GFX->createStateBlock(desc);
}

void LevelCellManager::prepRenderImage(SceneRenderState *state)
{
	// Do a little prep work if needed
	if (mVertexBuffer.isNull())
		createGeometry();

	// Allocate an ObjectRenderInst so that we can submit it to the RenderPassManager
	ObjectRenderInst *ri = state->getRenderPass()->allocInst<ObjectRenderInst>();

	// Now bind our rendering function so that it will get called
	ri->renderDelegate.bind(this, &LevelCellManager::render);

	// Set our RenderInst as a standard object render
	ri->type = RenderPassManager::RIT_Object;

	// Set our sorting keys to a default value
	ri->defaultKey = 0;
	ri->defaultKey2 = 0;

	// Submit our RenderInst to the RenderPassManager
	state->getRenderPass()->addInst(ri);
}

void LevelCellManager::render(ObjectRenderInst *ri, SceneRenderState *state, BaseMatInstance *overrideMat)
{
	if (overrideMat)
		return;

	if (mVertexBuffer.isNull())
		return;

	PROFILE_SCOPE(LevelCellManager_Render);

	// Set up a GFX debug event (this helps with debugging rendering events in external tools)
	GFXDEBUGEVENT_SCOPE(LevelCellManager_Render, ColorI::RED);

	// GFXTransformSaver is a handy helper class that restores
	// the current GFX matrices to their original values when
	// it goes out of scope at the end of the function
	GFXTransformSaver saver;

	// Calculate our object to world transform matrix
	MatrixF objectToWorld = getRenderTransform();
	objectToWorld.scale(getScale());

	// Apply our object transform
	GFX->multWorld(objectToWorld);

	// Deal with reflect pass otherwise
	// set the normal StateBlock
	if (state->isReflectPass())
		GFX->setStateBlock(mReflectSB);
	else
		GFX->setStateBlock(mNormalSB);

	// Set up the "generic" shaders
	// These handle rendering on GFX layers that don't support
	// fixed function. Otherwise they disable shaders.
	GFX->setupGenericShaders(GFXDevice::GSModColorTexture);

	// Set the vertex buffer
	GFX->setVertexBuffer(mVertexBuffer);

	// Draw our triangles
	GFX->drawPrimitive(GFXTriangleList, 0, 12);

	// Got debug drawing to do?
	if (state->isDiffusePass())
	{
		ObjectRenderInst *ri = state->getRenderPass()->allocInst<ObjectRenderInst>();
		ri->renderDelegate.bind(this, &LevelCellManager::_renderCellBounds);
		ri->type = RenderPassManager::RIT_Editor;
		state->getRenderPass()->addInst(ri);
	}
}

void LevelCellManager::_renderCellBounds(ObjectRenderInst *ri, SceneRenderState *state, BaseMatInstance *overrideMat)
{
	PROFILE_SCOPE(Forest_RenderCellBounds);

	if (overrideMat)
		return;

	GFXTransformSaver saver;

	MatrixF projBias(true);
	const Frustum frustum = GFX->getFrustum();
	MathUtils::getZBiasProjectionMatrix(0.001f, frustum, &projBias);
	GFX->setProjectionMatrix(projBias);

	VectorF extents;
	Point3F pos;

	U32 adjacentCellRadius = 5;

	Point3F camPos = state->getCameraPosition();

	RectF adjacentCellRect = getAdjacentRect(camPos, adjacentCellRadius);

	// Get top level cells
	Vector<LevelCell*> cellStack;
	getAdjacentCells(&cellStack, adjacentCellRect);

	// Holds child cells we need to render as we encounter them.
	Vector<LevelCell*> frontier;

	GFXDrawUtil *drawer = GFX->getDrawUtil();

	GFXStateBlockDesc desc;
	desc.setZReadWrite(true, false);
	desc.setBlend(true);
	desc.setFillModeWireframe();

	F32 cellHeight = 1000;

	while (!cellStack.empty())
	{
		while (!cellStack.empty())
		{
			const LevelCell *cell = cellStack.last();
			cellStack.pop_back();

			Box3F box = cell->bounds;

			RectF rect = cell->rect;

			box.minExtents.set(rect.point.x, rect.point.y, -cellHeight/2);
			box.maxExtents.set(rect.point.x + rect.extent.x, rect.point.y + rect.extent.y, cellHeight / 2);

			if (cell->objectList.empty())
				drawer->drawCube(desc, box, ColorI::RED);
			else
				drawer->drawCube(desc, box, ColorI::GREEN);

			// If this cell has children, add them to the frontier.
			//if (!cell->isLeaf())
			//	cell->getChildren(&frontier);
		}

		// Now the frontier becomes the cellStack and we empty the frontier.
		cellStack = frontier;
		frontier.clear();
	}

	//last, draw our adjacent cell zone
	Box3F box;
	box.minExtents.set(adjacentCellRect.point.x, adjacentCellRect.point.y, -cellHeight / 2);
	box.maxExtents.set(adjacentCellRect.point.x + adjacentCellRect.extent.x, adjacentCellRect.point.y + adjacentCellRect.extent.y, cellHeight / 2);

	drawer->drawCube(desc, box, ColorI::BLUE);
}

//Level Cell work
void LevelCellManager::addObject(SceneObject* object)
{
	if (getId() == object->getId() || object->isClientObject())
		return;

	//Ok, get the object's position, and find out what cell it should be added to
	LevelCell* cell = _findOrCreateCell(object->getPosition());

	if (cell != nullptr)
	{
		cell->objectList.push_back(object);

		cell->bounds.intersect(object->getWorldBox());
	}
}

void LevelCellManager::removeObject(SceneObject* object)
{

}

void LevelCellManager::objectTransformChanged(SceneObject* object)
{
	//Lazy, for now just push it into the new cell, we'll worry about proper cleanup behavior next
	if (getId() == object->getId() || object->isClientObject())
		return;

	//Ok, get the object's position, and find out what cell it should be added to
	LevelCell* cell = _findOrCreateCell(object->getPosition());

	if (cell != nullptr)
	{
		bool found = false;
		for (U32 i = 0; i < cell->objectList.size(); i++)
		{
			if (cell->objectList[i]->getId() == object->getId())
			{
				found = true;
				break;
			}
		}

		if(!found)
			cell->objectList.push_back(object);

		cell->bounds.intersect(object->getWorldBox());
	}

	//track down any cells this used to be in, and clear it from there
	LevelCell* priorCell = findCellByObject(object);

	if (priorCell && priorCell != cell)
	{
		priorCell->objectList.remove(object);
	}
}

Point3I LevelCellManager::getVoxel(Point3F position)
{
	Box3F worldBox = getWorldBox();
	Point3F center = worldBox.getCenter();
	Point3F bottom_corner = worldBox.minExtents;
	Point3F top_corner = worldBox.maxExtents;

	Point3F offset = position - bottom_corner;

	Point3I index = Point3I(mAbs(offset.x / mVoxelSize), mAbs(offset.y / mVoxelSize), mAbs(offset.z / mVoxelSize));

	Box3F voxBox = Box3F(bottom_corner + Point3F(mVoxelSize * index.x, mVoxelSize * index.y, mVoxelSize * index.z),
		bottom_corner + Point3F(mVoxelSize * (index.x + 1), mVoxelSize * (index.y + 1), mVoxelSize * (index.z + 1)));

	if (!voxBox.isContained(position))
		return Point3I(-1, -1, -1);

	return index;
}

Point3I LevelCellManager::getVoxelCount()
{
	Point3I voxelCount;
	Box3F worldBox = getWorldBox();

	voxelCount.x = worldBox.len_x() / mVoxelSize;
	voxelCount.y = worldBox.len_y() / mVoxelSize;
	voxelCount.z = worldBox.len_z() / mVoxelSize;

	return voxelCount;
}

// Returns a Point3F with the world space dimensions of a voxel.
Point3F LevelCellManager::getWorldSpaceVoxelSize()
{
	Box3F worldBox = getWorldBox();
	Point3F bottom_corner = worldBox.minExtents;
	Point3F top_corner = worldBox.maxExtents;
	Point3I voxelCount = getVoxelCount();
	Point3F difference = (top_corner - bottom_corner);
	difference.x = difference.x / voxelCount.x;
	difference.y = difference.y / voxelCount.y;
	difference.z = difference.z / voxelCount.z;
	return difference;
}