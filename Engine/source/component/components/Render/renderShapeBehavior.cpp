//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "platform/platform.h"
#include "console/consoleTypes.h"
#include "component/components/Render/renderShapeBehavior.h"
#include "core/util/safeDelete.h"
#include "core/resourceManager.h"
#include "core/stream/fileStream.h"
#include "console/consoleTypes.h"
#include "console/consoleObject.h"
#include "ts/tsShapeInstance.h"
#include "core/stream/bitStream.h"
//#include "console/consoleInternal.h"
#include "sim/netConnection.h"
#include "gfx/gfxTransformSaver.h"
#include "console/engineAPI.h"
#include "lighting/lightQuery.h"
#include "math/mTransform.h"
#include "scene/sceneManager.h"

#include "gfx/sim/debugDraw.h"  

#include "materials/materialManager.h"
#include "materials/matInstance.h"

#include "core/strings/findMatch.h"
//
#include "gui/controls/guiTreeViewCtrl.h"

//////////////////////////////////////////////////////////////////////////
// Constructor/Destructor
//////////////////////////////////////////////////////////////////////////

RenderShapeBehavior::RenderShapeBehavior()
{
   mNetFlags.set(Ghostable | ScopeAlways);

   mFriendlyName = "Render Shape";
   mComponentType = "Render";

   mDescription = getDescriptionText("Causes the object to render a 3d shape using the file provided.");

   mNetworked = true;

   setScopeAlways();
}

RenderShapeBehavior::~RenderShapeBehavior(){}

IMPLEMENT_CO_NETOBJECT_V1(RenderShapeBehavior);

//////////////////////////////////////////////////////////////////////////
ComponentInstance *RenderShapeBehavior::createInstance()
{
   RenderShapeBehaviorInstance *instance = new RenderShapeBehaviorInstance(this);

   setupFields( instance );

   if(instance->registerObject())
      return instance;

   delete instance;
   return NULL;
}

bool RenderShapeBehavior::onAdd()
{
   if(! Parent::onAdd())
      return false;

   String narp = mTemplateName;

   return true;
}

void RenderShapeBehavior::onRemove()
{
   Parent::onRemove();
}
void RenderShapeBehavior::initPersistFields()
{
   Parent::initPersistFields();
}

U32 RenderShapeBehavior::packUpdate(NetConnection *con, U32 mask, BitStream *stream)
{
   U32 retMask = Parent::packUpdate(con, mask, stream);
   return retMask;
}

void RenderShapeBehavior::unpackUpdate(NetConnection *con, BitStream *stream)
{
   Parent::unpackUpdate(con, stream);
}

//==========================================================================================
void RenderShapeBehaviorInstance::boneObject::addObject(SimObject* object)
{
   //Parent::addObject(object);
   SceneObject* sc = dynamic_cast<SceneObject*>(object);

   if(sc && mOwner)
   {
      if(TSShape* shape = mOwner->getShape())
      {
         S32 nodeID = shape->findNode(mBoneName);

         //we may have a offset on the shape's center
         //so make sure we accomodate for that when setting up the mount offsets
         MatrixF mat = mOwner->getShapeInstance()->mNodeTransforms[nodeID];
         //mat.setPosition(mat.getPosition() + mOwner->getShape()->getShape()->center);

         mOwner->getOwner()->mountObject(sc, nodeID, mat);
      }
   }
}

//==========================================================================================
RenderShapeBehaviorInstance::RenderShapeBehaviorInstance( Component *btemplate ) 
{
   mTemplate = btemplate;
   mOwner = NULL;
   mShapeName = StringTable->insert("");
   mShapeInstance = NULL;

   mChangingMaterials.clear();

   mMaterials.clear();

   mNetFlags.set(Ghostable | ScopeAlways);
}

RenderShapeBehaviorInstance::~RenderShapeBehaviorInstance()
{
   mShapeInstance = NULL;
}
IMPLEMENT_CO_NETOBJECT_V1(RenderShapeBehaviorInstance);

bool RenderShapeBehaviorInstance::onAdd()
{
   if(! Parent::onAdd())
      return false;

   // Register for the resource change signal.
   ResourceManager::get().getChangedSignal().notify( this, &RenderShapeBehaviorInstance::_onResourceChanged );

   //get the default shape, if any
   //updateShape();

   return true;
}

void RenderShapeBehaviorInstance::onComponentAdd()
{
   Parent::onComponentAdd();

   //get the default shape, if any
   updateShape();
}

void RenderShapeBehaviorInstance::onRemove()
{
   if(mShapeInstance)
   {
      delete mShapeInstance;
      mShapeInstance = NULL;
   }

   Parent::onRemove();
}

void RenderShapeBehaviorInstance::onComponentRemove()
{
   if(mOwner)
   {
      Point3F pos = mOwner->getPosition(); //store our center pos
      mOwner->setObjectBox(Box3F(Point3F(-1,-1,-1), Point3F(1,1,1)));
      //mOwner->resetWorldBox();
      mOwner->setPosition(pos);
   }

   Parent::onComponentRemove();
}


void RenderShapeBehaviorInstance::initPersistFields()
{
   Parent::initPersistFields();

   //create a hook to our internal variables
   addField("shapeName",   TypeShapeFilename,  Offset( mShapeName, RenderShapeBehaviorInstance ), 
      "%Path and filename of the model file (.DTS, .DAE) to use for this TSStatic." );
   //addProtectedField("shapeName", TypeShapeFilename, Offset(mShapeName, RenderShapeBehaviorInstance), &_setShape, defaultProtectedGetFn);
}

bool RenderShapeBehaviorInstance::_setShape( void *object, const char *index, const char *data )
{
   RenderShapeBehaviorInstance *rbI = static_cast<RenderShapeBehaviorInstance*>(object);
   rbI->mShapeName = StringTable->insert(data);
   rbI->updateShape(); //make sure we force the update to resize the owner bounds
   rbI->setMaskBits(ShapeMask);

   return true;
}

void RenderShapeBehaviorInstance::_onResourceChanged( const Torque::Path &path )
{
   if ( path != Torque::Path( mShapeName ) )
      return;

   updateShape();
   setMaskBits(ShapeMask);
}

void RenderShapeBehaviorInstance::inspectPostApply()
{
   Parent::inspectPostApply();

   //updateShape();
   //setMaskBits(ShapeMask);

   //updateMaterials();
  // setMaskBits(MaterialsMask);
}

U32 RenderShapeBehaviorInstance::packUpdate(NetConnection *con, U32 mask, BitStream *stream)
{
   U32 retMask = Parent::packUpdate(con, mask, stream);

   if( mask & (ShapeMask | InitialUpdateMask))
   {
      if(!mOwner)
      {
         stream->writeFlag( false );
      }
      else if( con->getGhostIndex(mOwner) != -1 )
      {
         stream->writeFlag( true );
         stream->writeString( mShapeName );
      }
      else
      {
         retMask |= ShapeMask; //try it again untill our dependency is ghosted
         stream->writeFlag( false );
      }
   }

   if( mask & (MaterialMask | InitialUpdateMask))
   {
      if(!mOwner)
      {
         stream->writeFlag( false );
      }
      else if( con->getGhostIndex(mOwner) != -1 )
      {
         stream->writeFlag( true );

         stream->writeInt(mChangingMaterials.size(), 16);

         for(U32 i=0; i < mChangingMaterials.size(); i++)
         {
            stream->writeInt(mChangingMaterials[i].slot, 16);
            con->packNetStringHandleU(stream, NetStringHandle(mChangingMaterials[i].matName));
         }
         //stream->writeInt(mMaterialChangeSlot, 16);
         //con->packNetStringHandleU(stream, mMaterialChangeHandle);

         mChangingMaterials.clear();
         /*U32 materialCount = mMaterialHandles.size();

         int id = getId();
         bool serv = isServerObject();

         stream->writeInt(materialCount, 16);

         for(U32 i=0; i < materialCount; i++)
         {
            String test = String(mMaterialHandles[i].getString());
            con->packNetStringHandleU(stream, mMaterialHandles[i]);
         }*/
      }
      else
      {
         retMask |= MaterialMask; //try it again untill our dependency is ghosted
         stream->writeFlag( false );
      }
   }

   return retMask;
}

void RenderShapeBehaviorInstance::unpackUpdate(NetConnection *con, BitStream *stream)
{
   Parent::unpackUpdate(con, stream);

   if(stream->readFlag())
   {
      mShapeName = stream->readSTString();
      updateShape();
   }

   if(stream->readFlag())
   {
      mChangingMaterials.clear();
      U32 materialCount = stream->readInt(16);

      for(U32 i=0; i < materialCount; i++)
      {
         matMap newMatMap;
         newMatMap.slot = stream->readInt(16);
         newMatMap.matName = String(con->unpackNetStringHandleU(stream).getString());

         mChangingMaterials.push_back(newMatMap);
      }

      //U32 mMaterialChangeSlot = stream->readInt(16);
      //mMaterialChangeHandle = con->unpackNetStringHandleU(stream);
      /*for(U32 i=0; i < materialCount; i++)
      {
         NetStringHandle matHandle = con->unpackNetStringHandleU(stream);
         mMaterialHandles.push_back(matHandle);
      }*/

      updateMaterials();
   }
}

void RenderShapeBehaviorInstance::prepRenderImage( SceneRenderState *state )
{
   if(!mEnabled)
      return;

   // get shape detail...we might not even need to be drawn
   Entity *o = dynamic_cast<Entity*>(getOwner());
   Box3F box = getOwner()->getWorldBox();
   Point3F cameraOffset = getOwner()->getWorldBox().getClosestPoint( state->getDiffuseCameraPosition() ) - state->getDiffuseCameraPosition();
   F32 dist = cameraOffset.len();
   if (dist < 0.01f)
      dist = 0.01f;

   Point3F objScale = getOwner()->getScale();
   F32 invScale = (1.0f/getMax(getMax(objScale.x,objScale.y),objScale.z));

   if(mShapeInstance)
   {
      mShapeInstance->setDetailFromDistance( state, dist * invScale );
      if (mShapeInstance->getCurrentDetail() < 0 )
         return;
   }
   else if(!getOwner()->gShowBoundingBox)
      return;

   // Debug rendering of the shape bounding box.
   /*if ( getOwner()->gShowBoundingBox )
   {
   ObjectRenderInst *ri = state->getRenderPass()->allocInst<ObjectRenderInst>();
   ri->renderDelegate.bind( this, &ShapeBase::_renderBoundingBox );
   ri->objectIndex = -1;
   ri->type = RenderPassManager::RIT_Editor;
   state->getRenderPass()->addInst( ri );
   }*/

   if( mShapeInstance )
   {
      GFXTransformSaver saver;

      // Set up our TS render state. 
      TSRenderState rdata;
      rdata.setSceneState( state );

      // We might have some forward lit materials
      // so pass down a query to gather lights.
      LightQuery query;
      query.init( getOwner()->getWorldSphere() );
      rdata.setLightQuery( &query );

      MatrixF mat = getOwner()->getRenderTransform();
      Point3F center = mShapeInstance->getShape()->center;
      Point3F position = mat.getPosition();

      getOwner()->getObjToWorld().mulP(center);

      Point3F posOffset = position - center;

      //mat.setPosition(position + posOffset);
      mat.scale( objScale );

      GFX->setWorldMatrix( mat );

      mShapeInstance->animate();
      mShapeInstance->render( rdata );
   }

   //ColorI colr = mShapeInstance->getShape()->meshes[0]->mVertexData[0].color;
}

void RenderShapeBehaviorInstance::updateShape()
{
   if (mShapeName && mShapeName[0] != '\0')
   {
      mShape = ResourceManager::get().load(mShapeName);

      if(!mShape)
      {
         Con::errorf("RenderShapeBehavior::updateShape : failed to load shape file!");
         return; //if it failed to load, bail out
      }

      if(mShapeInstance)
         delete mShapeInstance;

      //mShapeInstance = new TSShapeInstance( mShape, isClientObject() );
      mShapeInstance = new TSShapeInstance( mShape, true );

      //Do this on both the server and client
      S32 materialCount = mShape->materialList->getMaterialNameList().size();

      //mMaterials.clear();

      /*for(U32 i=0; i < materialCount; i++)
      {
         String materialname = mShape->materialList->getMaterialName(i);
         if(materialname == String("ShapeBounds"))
            continue;

         matMap newMatMap;
         newMatMap.slot = i;
         newMatMap.matName = materialname;

         mMaterials.push_back(newMatMap);
      }*/

      if(isServerObject())
      {
         //we need to update the editor
         for(U32 i=0; i < mComponentFields.size(); i++)
         {
            //find any with the materialslot title and clear them out
            if(FindMatch::isMatch( "materialslot*", mComponentFields[i].mFieldName, false ))
            {
               mComponentFields.erase(i);
               continue;
            }
         }

         //next, get a listing of our materials in the shape, and build our field list for them
         char matFieldName[128];

         if(materialCount > 0)
            mComponentGroup = StringTable->insert("Materials");

         for(U32 i=0; i < materialCount; i++)
         {
            String materialname = mShape->materialList->getMaterialName(i);
            if(materialname == String("ShapeBounds"))
               continue;

            dSprintf(matFieldName, 128, "MaterialSlot%d", i);
            
            addComponentField(matFieldName, "A material used in the shape file", "material", materialname, "");
         }

         if(materialCount > 0)
            mComponentGroup = "";
      }

      if(mOwner != NULL)
      {
         Entity* e = dynamic_cast<Entity*>(mOwner);

         Point3F min, max, pos;
         pos = e->getPosition();

         e->getWorldToObj().mulP(pos);

         //min = mShape->bounds.minExtents - (pos + mShapeInstance->getShape()->center);
         //max = mShape->bounds.maxExtents - (pos + mShapeInstance->getShape()->center);

         min = mShape->bounds.minExtents;
         max = mShape->bounds.maxExtents;

         mShapeBounds.set(min, max);

         e->setObjectBox(Box3F(min, max));
         //e->setObjectBox(Box3F(min, max));

         //mOwner->setObjectBox(Box3F(Point3F(-3, -3, -3), Point3F(3, 3, 3)));
         //mOwner->resetWorldBox();
         //e->setMaskBits(Entity::BoundsMask);

         if( mOwner->getSceneManager() != NULL )
            mOwner->getSceneManager()->notifyObjectDirty( mOwner );
      }
   }
}

void RenderShapeBehaviorInstance::updateMaterials()
{
   if(mChangingMaterials.empty())
      return;

   if (getShapeInstance()->ownMaterialList() == false)
      getShapeInstance()->cloneMaterialList();

   TSMaterialList* pMatList = getShapeInstance()->getMaterialList();
   pMatList->setTextureLookupPath( getShapeInstance()->getShapeResource().getPath().getPath() );

   const Vector<String> &materialNames = pMatList->getMaterialNameList();
   for ( S32 i = 0; i < materialNames.size(); i++ )
   {
      const String &pName = materialNames[i];

      for(U32 m=0; m < mChangingMaterials.size(); m++)
      {
         if(mChangingMaterials[m].slot == i)
         {
            pMatList->renameMaterial( i, mChangingMaterials[m].matName );
         }
      }
      
      /*for(U32 i=0; i < mMaterials.size(); i++)
      {
         for(U32 m = 0; m < mChangingMaterials.size(); m++)
         {
            if(mMaterials[i].slot == mChangingMaterials[m].slot)
               mMaterials[i].matName = mChangingMaterials[m].matName;
         }
      }*/

      mChangingMaterials.clear();
   }

   // Initialize the material instances
   getShapeInstance()->initMaterialList();
}

MatrixF RenderShapeBehaviorInstance::getNodeTransform(S32 nodeIdx)
{
   if( mShapeInstance )
   {
      S32 nodeCount = mShapeInstance->getShape()->nodes.size();

      if(nodeIdx >= 0 && nodeIdx < nodeCount)
      {
         mShapeInstance->animate();
         MatrixF mountTransform = mShapeInstance->mNodeTransforms[nodeIdx];
         mountTransform.mul(mOwner->getTransform());

         return mountTransform;
      }
   }

   return MatrixF::Identity;
}

S32 RenderShapeBehaviorInstance::getNodeByName(String nodeName)
{
   if( mShapeInstance )
   {
      S32 nodeIdx = mShapeInstance->getShape()->findNode(nodeName);

      return nodeIdx;
   }

   return -1;
}

bool RenderShapeBehaviorInstance::castRayRendered(const Point3F &start, const Point3F &end, RayInfo *info)
{
   return false;
   /*if ( !mShapeInstance )
   return false;

   // Cast the ray against the currently visible detail
   RayInfo localInfo;
   bool res = mShapeInstance->castRayOpcode( mShapeInstance->getCurrentDetail(), start, end, &localInfo );

   if ( res )
   {
   *info = localInfo;
   info->object = mOwner;
   return true;
   }

   return false;*/
}

void RenderShapeBehaviorInstance::mountObjectToNode(SceneObject* objB, String node, MatrixF txfm)
{
   const char* test;
   test = node.c_str();
   if(dIsdigit(test[0]))
   {
      getOwner()->mountObject(objB, dAtoi(node), txfm);
   }
   else
   {
      if(TSShape* shape = getShape())
      {
         S32 idx = shape->findNode(node);
         getOwner()->mountObject(objB, idx, txfm);
      }
   }
}

/*Geometry* RenderShapeBehaviorInstance::getGeometry()
{
return NULL;
}*/

void RenderShapeBehaviorInstance::onDynamicModified(const char* slotName, const char* newValue)
{
   if(FindMatch::isMatch( "materialslot*", slotName, false ))
   {
      if(!getShape())
         return;

      S32 slot = -1;
      String outStr( String::GetTrailingNumber( slotName, slot ) );

      if(slot == -1)
         return;

      bool found = false;
      for(U32 i=0; i < mChangingMaterials.size(); i++)
      {
         if(mChangingMaterials[i].slot == slot)
         {
            mChangingMaterials[i].matName = String(newValue);
            found = true;
         }
      }

      if(!found)
      {
         matMap newMatMap;
         newMatMap.slot = slot;
         newMatMap.matName = String(newValue);

         mChangingMaterials.push_back(newMatMap);
      }

      setMaskBits(MaterialMask);
   }

   Parent::onDynamicModified(slotName, newValue);
}

void RenderShapeBehaviorInstance::changeMaterial(U32 slot, const char* newMat)
{
   /*bool server = isServerObject();
   int id = getId();

   if(!getShape())
      return;

   bool found = false;
   for(U32 i=0; i < mChangingMaterials.size(); i++)
   {
      if(mChangingMaterials[i].slot == slot)
      {
         mChangingMaterials[i].matName = String(newMat);
         found = true;
      }
   }

   if(!found)
   {
      matMap newMatMap;
      newMatMap.slot = slot;
      newMatMap.matName = String(newMat);

      mChangingMaterials.push_back(newMatMap);
   }

   //update our server-side list as well.
   /*for(U32 i=0; i < mMaterials.size(); i++)
   {
      if(mMaterials[i].slot == slot)
         mMaterials[i].matName = String(newMat);
   }*/

   char fieldName[512];

   //update our respective field
   dSprintf(fieldName, 512, "materialSlot%d", slot);
   setDataField(fieldName, NULL, newMat);

   /*if(slot < mMaterialHandles.size())
   {
      mMaterialHandles.erase(slot);

      NetStringHandle matHandle = NetStringHandle(newMat);
      mMaterialHandles.push_back(matHandle);
   }*/

   //setMaskBits(MaterialMask);
}

void RenderShapeBehaviorInstance::onInspect()
{
   return;
   //accumulate a temporary listing of objects to represent the bones
   //then we add these to our object here, and finally add our object to our owner specifically
   //so that we, and all the bones under us, show in the heirarchy of the scene
   //The objects we use are a special simgroup class just for us, that have specific callbacks
   //in the event an entity is mounted to it.

   //mOwner->addObject(this);
   /*GuiTreeViewCtrl *editorTree = dynamic_cast<GuiTreeViewCtrl*>(Sim::findObject("EditorTree"));
   if(!editorTree)
   return;

   if(mNodesList.empty())
   {
   if(!mShapeInstance)
   return;

   GuiTreeViewCtrl::Item *newItem, *parentItem;

   parentItem = editorTree->getItem(editorTree->findItemByObjectId(mOwner->getId()));

   S32 componentID = editorTree->insertItem(parentItem->getID(), "RenderShapeBehavior");

   newItem = editorTree->getItem(componentID);
   newItem->mInspectorInfo.mObject = this;
   newItem->mState.set(GuiTreeViewCtrl::Item::DenyDrag);

   TSShape* shape = mShapeInstance->getShape();
   S32 nodeCount = shape->nodes.size();

   String nodeName, parentName;

   for(U32 i=0; i < nodeCount; i++)
   {
   S32 nID = shape->nodes[i].nameIndex;
   S32 pID = shape->nodes[i].parentIndex;
   S32 parentItemID;

   nodeName = shape->getNodeName(shape->nodes[i].nameIndex);
   if(pID != -1)
   {
   bool found = false;
   for(U32 b=0; b < mNodesList.size(); b++)
   {
   if(!dStrcmp(mNodesList[b]->mBoneName, shape->getNodeName(pID)))
   {
   parentItemID = mNodesList[b]->mItemID;
   found = true;
   break;
   }
   }

   if(!found)
   parentItemID = componentID;
   }
   else
   {
   parentItemID = componentID;
   }

   S32 boneID = editorTree->insertItem(parentItemID, nodeName);
   newItem = editorTree->getItem(boneID);

   boneObject *b = new boneObject(this);
   b->mBoneName = nodeName;
   b->mItemID = boneID;

   mNodesList.push_back(b);

   newItem->mInspectorInfo.mObject = b;
   newItem->mState.set(GuiTreeViewCtrl::Item::ForceItemName);
   newItem->mState.set(GuiTreeViewCtrl::Item::InspectorData);
   newItem->mState.set(GuiTreeViewCtrl::Item::ForceDragTarget);
   newItem->mState.set(GuiTreeViewCtrl::Item::DenyDrag);

   //while we're here, check our parent to see if anything is mounted to this node.
   //if it is, hijack the item and move it under us!
   for (SceneObject* itr = mOwner->getMountList(); itr; itr = itr->getMountLink())
   {
   if(itr->getMountNode() == i)
   {
   newItem = editorTree->getItem(editorTree->findItemByObjectId(itr->getId()));
   newItem->mParent = editorTree->getItem(boneID);
   }
   }
   }

   /*GuiTreeViewCtrl::Item *newItem, *parentItem;

   parentItem = editorTree->getItem(editorTree->findItemByObjectId(mOwner->getId()));

   S32 componentID = editorTree->insertItem(parentItem->getID(), "RenderShapeBehavior");

   newItem = editorTree->getItem(componentID);
   newItem->mInspectorInfo.mObject = this;
   newItem->mState.set(GuiTreeViewCtrl::Item::DenyDrag);

   boneObject *b = new boneObject(this);
   b->mBoneName = StringTable->insert("Root");

   mNodesList.push_back(b);

   S32 boneID = editorTree->insertItem(componentID, b->mBoneName);
   newItem = editorTree->getItem(boneID);

   newItem->mInspectorInfo.mObject = b;
   newItem->mState.set(GuiTreeViewCtrl::Item::ForceItemName);
   newItem->mState.set(GuiTreeViewCtrl::Item::InspectorData);
   newItem->mState.set(GuiTreeViewCtrl::Item::ForceDragTarget);
   newItem->mState.set(GuiTreeViewCtrl::Item::DenyDrag);*/

   //editorTree->buildVisibleTree(true);
   //}

}

void RenderShapeBehaviorInstance::onEndInspect()
{
   //mOwner->removeObject(this);
}

DefineEngineMethod( RenderShapeBehaviorInstance, getShapeBounds, Box3F, (),,
                   "@brief Get the cobject we're in contact with.\n\n"

                   "The controlling client is the one that will send moves to us to act on.\n"

                   "@return the ID of the controlling GameConnection, or 0 if this object is not "
                   "controlled by any client.\n"

                   "@see GameConnection\n")
{
   return object->getShapeBounds();
}

DefineEngineMethod( RenderShapeBehaviorInstance, mountObject, bool,
                   ( SceneObject* objB, String node, TransformF txfm ), ( MatrixF::Identity ),
                   "@brief Mount objB to this object at the desired slot with optional transform.\n\n"

                   "@param objB  Object to mount onto us\n"
                   "@param slot  Mount slot ID\n"
                   "@param txfm (optional) mount offset transform\n"
                   "@return true if successful, false if failed (objB is not valid)" )
{
   if ( objB )
   {
      //BUG: Unsure how it broke, but atm the default transform passed in here is rotated 180 degrees. This doesn't happen
      //for the SceneObject mountobject method. Hackish, but for now, just default to a clean MatrixF::Identity
      object->mountObjectToNode( objB, node, /*MatrixF::Identity*/txfm.getMatrix() );
      return true;
   }
   return false;
}

DefineEngineMethod( RenderShapeBehaviorInstance, getNodeTransform, TransformF,
                   ( S32 node ), ( -1 ),
                   "@brief Mount objB to this object at the desired slot with optional transform.\n\n"

                   "@param objB  Object to mount onto us\n"
                   "@param slot  Mount slot ID\n"
                   "@param txfm (optional) mount offset transform\n"
                   "@return true if successful, false if failed (objB is not valid)" )
{
   if ( node != -1 )
   {

      //BUG: Unsure how it broke, but atm the default transform passed in here is rotated 180 degrees. This doesn't happen
      //for the SceneObject mountobject method. Hackish, but for now, just default to a clean MatrixF::Identity
      //object->mountObjectToNode( objB, node, /*MatrixF::Identity*/txfm.getMatrix() );
      MatrixF mat = object->getNodeTransform(node);
      return mat;
   }

   return TransformF::Identity;
}

DefineEngineMethod( RenderShapeBehaviorInstance, getNodeEulerRot, EulerF,
                   ( S32 node, bool radToDeg ), ( -1, true ),
                   "@brief Mount objB to this object at the desired slot with optional transform.\n\n"

                   "@param objB  Object to mount onto us\n"
                   "@param slot  Mount slot ID\n"
                   "@param txfm (optional) mount offset transform\n"
                   "@return true if successful, false if failed (objB is not valid)" )
{
   if ( node != -1 )
   {

      //BUG: Unsure how it broke, but atm the default transform passed in here is rotated 180 degrees. This doesn't happen
      //for the SceneObject mountobject method. Hackish, but for now, just default to a clean MatrixF::Identity
      //object->mountObjectToNode( objB, node, /*MatrixF::Identity*/txfm.getMatrix() );
      MatrixF mat = object->getNodeTransform(node);

      EulerF eul = mat.toEuler();
      if(radToDeg)
         eul = EulerF(mRadToDeg(eul.x), mRadToDeg(eul.y), mRadToDeg(eul.z));

      return eul;
   }

   return EulerF(0,0,0);
}

DefineEngineMethod( RenderShapeBehaviorInstance, getNodePosition, Point3F,
                   ( S32 node ), ( -1 ),
                   "@brief Mount objB to this object at the desired slot with optional transform.\n\n"

                   "@param objB  Object to mount onto us\n"
                   "@param slot  Mount slot ID\n"
                   "@param txfm (optional) mount offset transform\n"
                   "@return true if successful, false if failed (objB is not valid)" )
{
   if ( node != -1 )
   {

      //BUG: Unsure how it broke, but atm the default transform passed in here is rotated 180 degrees. This doesn't happen
      //for the SceneObject mountobject method. Hackish, but for now, just default to a clean MatrixF::Identity
      //object->mountObjectToNode( objB, node, /*MatrixF::Identity*/txfm.getMatrix() );
      MatrixF mat = object->getNodeTransform(node);

      return mat.getPosition();
   }

   return Point3F(0,0,0);
}

DefineEngineMethod( RenderShapeBehaviorInstance, getNodeByName, S32,
                   ( String nodeName ),,
                   "@brief Mount objB to this object at the desired slot with optional transform.\n\n"

                   "@param objB  Object to mount onto us\n"
                   "@param slot  Mount slot ID\n"
                   "@param txfm (optional) mount offset transform\n"
                   "@return true if successful, false if failed (objB is not valid)" )
{
   if ( !nodeName.isEmpty() )
   {
      //BUG: Unsure how it broke, but atm the default transform passed in here is rotated 180 degrees. This doesn't happen
      //for the SceneObject mountobject method. Hackish, but for now, just default to a clean MatrixF::Identity
      //object->mountObjectToNode( objB, node, /*MatrixF::Identity*/txfm.getMatrix() );
      S32 node = object->getNodeByName(nodeName);

      return node;
   }

   return -1;
}

DefineEngineMethod( RenderShapeBehaviorInstance, changeMaterial, void, ( U32 slot, const char* newMat ),(0, ""),
   "@brief Change one of the materials on the shape.\n\n")
{
   object->changeMaterial(slot, newMat);
}
/*DefineEngineMethod( RenderShapeBehaviorInstance, changeMaterial, void, ( const char* mapTo, Material* oldMat, Material* newMat ),("",NULL,NULL),
   "@brief Change one of the materials on the shape.\n\n"

   "This method changes materials per mapTo with others. The material that "
   "is being replaced is mapped to unmapped_mat as a part of this transition.\n"

   "@note Warning, right now this only sort of works. It doesn't do a live "
   "update like it should.\n"

   "@param mapTo the name of the material target to remap (from getTargetName)\n"
   "@param oldMat the old Material that was mapped \n"
   "@param newMat the new Material to map\n\n"

   "@tsexample\n"
      "// remap the first material in the shape\n"
      "%mapTo = %obj.getTargetName( 0 );\n"
      "%obj.changeMaterial( %mapTo, 0, MyMaterial );\n"
   "@endtsexample\n" )
{
   // if no valid new material, theres no reason for doing this
   if( !newMat )
   {
      Con::errorf("TSShape::changeMaterial failed: New material does not exist!");
      return;
   }

   // Check the mapTo name exists for this shape
   S32 matIndex = object->getShape()->materialList->getMaterialNameList().find_next(String(mapTo));
   if (matIndex < 0)
   {
      Con::errorf("TSShape::changeMaterial failed: Invalid mapTo name '%s'", mapTo);
      return;
   }

   // Lets remap the old material off, so as to let room for our current material room to claim its spot
   if( oldMat )
      oldMat->mMapTo = String("unmapped_mat");

   newMat->mMapTo = mapTo;

   // Map the material by name in the matmgr
   MATMGR->mapMaterial( mapTo, newMat->getName() );

   // Replace instances with the new material being traded in. Lets make sure that we only
   // target the specific targets per inst, this is actually doing more than we thought
   delete object->getShape()->materialList->mMatInstList[matIndex];
   object->getShape()->materialList->mMatInstList[matIndex] = newMat->createMatInstance();

   // Finish up preparing the material instances for rendering
   const GFXVertexFormat *flags = getGFXVertexFormat<GFXVertexPNTTB>();
   FeatureSet features = MATMGR->getDefaultFeatures();
   object->getShape()->materialList->getMaterialInst(matIndex)->init( features, flags );
}*/