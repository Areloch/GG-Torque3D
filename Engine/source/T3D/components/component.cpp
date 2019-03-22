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
#include "console/simBase.h"
#include "console/consoleTypes.h"
#include "T3D/components/component.h"
#include "core/util/safeDelete.h"
#include "core/resourceManager.h"
#include "core/stream/fileStream.h"
#include "core/stream/bitStream.h"
#include "console/engineAPI.h"
#include "sim/netConnection.h"
#include "console/consoleInternal.h"
#include "T3D/assets/MaterialAsset.h"

//SystemImplementation
void ComponentSystem::tick()
{
   Component::componentItr itr = Component::components.find(getTypeIndex<Component>());
   while (itr != Component::components.end())
   {
      const Component& comp = itr->second;

      if (comp.isServerObject() && comp.isEnabled())
      {
         if (comp.hasOwner() && comp.mComponentObj != nullptr && comp.mComponentObj->isMethod("Update"))
            Con::executef(comp.mComponentObj, "Update");
      }

      itr++;
   }
}

//////////////////////////////////////////////////////////////////////////
// Constructor/Destructor
//////////////////////////////////////////////////////////////////////////

ComponentObject::ComponentObject()
{
   mFriendlyName = StringTable->EmptyString();
   mFromResource = StringTable->EmptyString();
   mComponentType = StringTable->EmptyString();
   mComponentGroup = StringTable->EmptyString();
   mNetworkType = StringTable->EmptyString();
   mTemplateName = StringTable->EmptyString();
   //mDependency = StringTable->EmptyString();

   mNetworked = false;

   // [tom, 1/12/2007] We manage the memory for the description since it
   // could be loaded from a file and thus massive. This is accomplished with
   // protected fields, but since they still call Con::getData() the field
   // needs to always be valid. This is pretty lame.
   mDescription = new char[1];
   ((char *)mDescription)[0] = 0;

   mOwner = NULL;

   mCanSaveFieldDictionary = false;

   mOriginatingAssetId = StringTable->EmptyString();

   mIsServerObject = true;

   componentIdx = 0;

   mHidden = false;
   mEnabled = true;

   mDirtyMaskBits = 0;
}

ComponentObject::~ComponentObject()
{
   for (S32 i = 0; i < mFields.size(); ++i)
   {
      ComponentField &field = mFields[i];
      SAFE_DELETE_ARRAY(field.mFieldDescription);
   }

   SAFE_DELETE_ARRAY(mDescription);
}

IMPLEMENT_CO_NETOBJECT_V1(ComponentObject);

//////////////////////////////////////////////////////////////////////////

void ComponentObject::initPersistFields()
{
   addGroup("ComponentObject");
   addField("componentType", TypeCaseString, Offset(mComponentType, ComponentObject), "The type of behavior.", AbstractClassRep::FieldFlags::FIELD_HideInInspectors);
   addField("networkType", TypeCaseString, Offset(mNetworkType, ComponentObject), "The type of behavior.", AbstractClassRep::FieldFlags::FIELD_HideInInspectors);
   addField("friendlyName", TypeCaseString, Offset(mFriendlyName, ComponentObject), "Human friendly name of this behavior", AbstractClassRep::FieldFlags::FIELD_HideInInspectors);
      addProtectedField("description", TypeCaseString, Offset(mDescription, ComponentObject), &setDescription, &getDescription,
         "The description of this behavior which can be set to a \"string\" or a fileName\n", AbstractClassRep::FieldFlags::FIELD_HideInInspectors);

      addField("networked", TypeBool, Offset(mNetworked, ComponentObject), "Is this behavior ghosted to clients?", AbstractClassRep::FieldFlags::FIELD_HideInInspectors);

      addProtectedField("Owner", TypeSimObjectPtr, Offset(mOwner, ComponentObject), &setOwner, &defaultProtectedGetFn, "", AbstractClassRep::FieldFlags::FIELD_HideInInspectors);

      //addField("hidden", TypeBool, Offset(mHidden, ComponentObject), "Flags if this behavior is shown in the editor or not", AbstractClassRep::FieldFlags::FIELD_HideInInspectors);
      addProtectedField("enabled", TypeBool, Offset(mEnabled, ComponentObject), &_setEnabled, &defaultProtectedGetFn, "");

      addField("originatingAsset", TypeComponentAssetPtr, Offset(mOriginatingAsset, ComponentObject),
         "Asset that spawned this component, used for tracking/housekeeping", AbstractClassRep::FieldFlags::FIELD_HideInInspectors);

   endGroup("ComponentObject");

   Parent::initPersistFields();

   //clear out irrelevent fields
   removeField("name");
   //removeField("internalName");
   removeField("parentGroup");
   //removeField("class");
   removeField("superClass");
   removeField("hidden");
   removeField("canSave");
   removeField("canSaveDynamicFields");
   removeField("persistentId");
}

bool ComponentObject::_setEnabled(void *object, const char *index, const char *data)
{
   ComponentObject *c = static_cast<ComponentObject*>(object);

   c->mEnabled = dAtob(data);
   c->setMaskBits(EnableMask);

   return true;
}

//////////////////////////////////////////////////////////////////////////

bool ComponentObject::setDescription(void *object, const char *index, const char *data)
{
   ComponentObject *bT = static_cast<ComponentObject *>(object);
   SAFE_DELETE_ARRAY(bT->mDescription);
   bT->mDescription = bT->getDescriptionText(data);

   // We return false since we don't want the console to mess with the data
   return false;
}

const char * ComponentObject::getDescription(void* obj, const char* data)
{
   ComponentObject *object = static_cast<ComponentObject *>(obj);

   return object->mDescription ? object->mDescription : "";
}

//////////////////////////////////////////////////////////////////////////
bool ComponentObject::onAdd()
{
   if (!Parent::onAdd())
      return false;

   mComponent = Component::createComponent<Component>();

   setMaskBits(UpdateMask);
   setMaskBits(NamespaceMask);

   return true;
}

void ComponentObject::onRemove()
{
   onDataSet.removeAll();

   if (mComponent.isValid() && mComponent->hasOwner())
   {
      //notify our removal to the owner, so we have no loose ends
      mComponent->getOwner()->removeComponent(mComponent, false);
   }

   Parent::onRemove();
}

void ComponentObject::onComponentAdd()
{
   if (!mComponent.isValid())
      return;

   if (mComponent->isServerObject())
   {
      if (isMethod("onAdd"))
         Con::executef(this, "onAdd");
   }

   mComponent->setEnabled(true);
}

void ComponentObject::onComponentRemove()
{
   if (!mComponent.isValid())
      return;

   mComponent->setEnabled(false);

   if (mComponent->isServerObject())
   {
      if (isMethod("onRemove"))
         Con::executef(this, "onRemove");
   }

   if (mComponent->hasOwner())
   {
      Entity* owner = mComponent->getOwner();
      owner->onComponentAdded.remove(this, &ComponentObject::componentAddedToOwner);
      owner->onComponentRemoved.remove(this, &ComponentObject::componentRemovedFromOwner);
   }

   mComponent->setOwner(nullptr);
   setDataField("owner", NULL, "");
}

void ComponentObject::setOwner(Entity* owner)
{
   if (!mComponent.isValid())
      return;

   //first, catch if we have an existing owner, and we're changing from it
   if (mComponent->hasOwner() && mComponent->getOwner() != owner)
   {
      Entity* owner = mComponent->getOwner();
      owner->onComponentAdded.remove(this, &ComponentObject::componentAddedToOwner);
      owner->onComponentRemoved.remove(this, &ComponentObject::componentRemovedFromOwner);

      owner->removeComponent(mComponent, false);
   }

   mComponent->setOwner(owner);

   if (owner != nullptr)
   {
      owner->onComponentAdded.notify(mComponent, &ComponentObject::componentAddedToOwner);
      owner->onComponentRemoved.notify(mComponent, &ComponentObject::componentRemovedFromOwner);
   }

   if (mComponent->isServerObject())
   {
      setMaskBits(OwnerMask);

      //if we have any outstanding maskbits, push them along to have the network update happen on the entity
      if (mDirtyMaskBits != 0 && owner)
      {
         owner->setMaskBits(Entity::ComponentsUpdateMask);
      }
   }
}

void ComponentObject::componentAddedToOwner(Component *comp)
{
   return;
}

void ComponentObject::componentRemovedFromOwner(Component *comp)
{
   return;
}

void ComponentObject::setMaskBits(U32 orMask)
{
   if (!mComponent.isValid())
      return;

   AssertFatal(orMask != 0, "Invalid net mask bits set.");
   
   if (mComponent->hasOwner())
      mComponent->getOwner()->setComponentNetMask(mComponent, orMask);
}

U32 ComponentObject::packUpdate(NetConnection *con, U32 mask, BitStream *stream)
{
   U32 retMask = 0;

   /*if (mask & OwnerMask)
   {
      if (mOwner != NULL)
      {
         S32 ghostIndex = con->getGhostIndex(mOwner);

         if (ghostIndex == -1)
         {
            stream->writeFlag(false);
            retMask |= OwnerMask;
         }
         else
         {
            stream->writeFlag(true);
            stream->writeFlag(true);
            stream->writeInt(ghostIndex, NetConnection::GhostIdBitSize);
         }
      }
      else
      {
         stream->writeFlag(true);
         stream->writeFlag(false);
      }
   }
   else
      stream->writeFlag(false);*/

   if (stream->writeFlag(mask & EnableMask))
   {
      stream->writeFlag(mComponent->isEnabled());
   }

   /*if (stream->writeFlag(mask & NamespaceMask))
   {
      const char* name = getName();
      if (stream->writeFlag(name && name[0]))
         stream->writeString(String(name));

      if (stream->writeFlag(mSuperClassName && mSuperClassName[0]))
         stream->writeString(String(mSuperClassName));

      if (stream->writeFlag(mClassName && mClassName[0]))
         stream->writeString(String(mClassName));
   }*/

   return retMask;
}

void ComponentObject::unpackUpdate(NetConnection *con, BitStream *stream)
{
   /*if (stream->readFlag())
   {
      if (stream->readFlag())
      {
         //we have an owner object, so fetch it
         S32 gIndex = stream->readInt(NetConnection::GhostIdBitSize);

         Entity *e = dynamic_cast<Entity*>(con->resolveGhost(gIndex));
         if (e)
            e->addComponent(this);
      }
      else
      {
         //it's being nulled out
         setOwner(NULL);
      }
   }*/

   if (stream->readFlag())
   {
      mComponent->setEnabled(stream->readFlag());
   }

   /*if (stream->readFlag())
   {
      if (stream->readFlag())
      {
         char name[256];
         stream->readString(name);
         assignName(name);
      }

      if (stream->readFlag())
      {
         char superClassname[256];
         stream->readString(superClassname);
       mSuperClassName = superClassname;
      }

      if (stream->readFlag())
      {
         char classname[256];
         stream->readString(classname);
         mClassName = classname;
      }

      linkNamespaces();
   }*/
}

void ComponentObject::packToStream(Stream &stream, U32 tabStop, S32 behaviorID, U32 flags /* = 0  */)
{
   char buffer[1024];

   writeFields(stream, tabStop);

   // Write out the fields which the behavior template knows about
   for (int i = 0; i < getComponentFieldCount(); i++)
   {
      ComponentField *field = getComponentField(i);
      const char *objFieldValue = getDataField(field->mFieldName, NULL);

      // If the field holds the same value as the template's default value than it
      // will get initialized by the template, and so it won't be included just
      // to try to keep the object files looking as non-horrible as possible.
      if (dStrcmp(field->mDefaultValue, objFieldValue) != 0)
      {
         dSprintf(buffer, sizeof(buffer), "%s = \"%s\";\n", field->mFieldName, (dStrlen(objFieldValue) > 0 ? objFieldValue : "0"));

         stream.writeTabs(tabStop);
         stream.write(dStrlen(buffer), buffer);
      }
   }
}

void ComponentObject::processTick()
{
   if (isServerObject() && mEnabled)
   {
      if (mOwner != NULL && isMethod("Update"))
         Con::executef(this, "Update");
   }
}

void ComponentObject::setDataField(StringTableEntry slotName, const char *array, const char *value)
{
   Parent::setDataField(slotName, array, value);

   onDataSet.trigger(this, slotName, value);
}

StringTableEntry ComponentObject::getComponentName()
{
   return getNamespace()->getName();
}

//catch any behavior field updates
void ComponentObject::onStaticModified(const char* slotName, const char* newValue)
{
   Parent::onStaticModified(slotName, newValue);

   //If we don't have an owner yet, then this is probably the initial setup, so we don't need the console callbacks yet.
   if (!mOwner)
      return;

   onDataSet.trigger(this, slotName, newValue);

   checkComponentFieldModified(slotName, newValue);
}

void ComponentObject::onDynamicModified(const char* slotName, const char* newValue)
{
   Parent::onDynamicModified(slotName, newValue);

   //If we don't have an owner yet, then this is probably the initial setup, so we don't need the console callbacks yet.
   if (!mOwner)
      return;

   checkComponentFieldModified(slotName, newValue);
}

void ComponentObject::checkComponentFieldModified(const char* slotName, const char* newValue)
{
   StringTableEntry slotNameEntry = StringTable->insert(slotName);

   //find if it's a behavior field
   for (int i = 0; i < mFields.size(); i++)
   {
      ComponentField *field = getComponentField(i);
      if (field->mFieldName == slotNameEntry)
      {
         //we have a match, do the script callback that we updated a field
         if (isMethod("onInspectorUpdate"))
            Con::executef(this, "onInspectorUpdate", slotName);

         return;
      }
   }
}
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
void ComponentObject::addComponentField(const char *fieldName, const char *desc, const char *type, const char *defaultValue /* = NULL */, const char *userData /* = NULL */, /*const char* dependency /* = NULL *//*,*/ bool hidden /* = false */, const char* customLabel /* = ""*/)
{
   StringTableEntry stFieldName = StringTable->insert(fieldName);

   for (S32 i = 0; i < mFields.size(); ++i)
   {
      if (mFields[i].mFieldName == stFieldName)
         return;
   }

   ComponentField field;

   if (customLabel != "")
      field.mFieldLabel = customLabel;
   else
      field.mFieldLabel = stFieldName;

   field.mFieldName = stFieldName;

   //find the field type
   S32 fieldTypeMask = -1;
   StringTableEntry fieldType = StringTable->insert(type);

   if (fieldType == StringTable->insert("int"))
      fieldTypeMask = TypeS32;
   else if (fieldType == StringTable->insert("float"))
      fieldTypeMask = TypeF32;
   else if (fieldType == StringTable->insert("vector"))
      fieldTypeMask = TypePoint3F;
   else if (fieldType == StringTable->insert("material"))
      fieldTypeMask = TypeMaterialAssetPtr;
   else if (fieldType == StringTable->insert("image"))
      fieldTypeMask = TypeImageFilename;
   else if (fieldType == StringTable->insert("shape"))
      fieldTypeMask = TypeShapeFilename;
   else if (fieldType == StringTable->insert("bool"))
      fieldTypeMask = TypeBool;
   else if (fieldType == StringTable->insert("object"))
      fieldTypeMask = TypeSimObjectPtr;
   else if (fieldType == StringTable->insert("string"))
      fieldTypeMask = TypeString;
   else if (fieldType == StringTable->insert("colorI"))
      fieldTypeMask = TypeColorI;
   else if (fieldType == StringTable->insert("colorF"))
      fieldTypeMask = TypeColorF;
   else if (fieldType == StringTable->insert("ease"))
      fieldTypeMask = TypeEaseF;
   else if (fieldType == StringTable->insert("gameObject"))
      fieldTypeMask = TypeGameObjectAssetPtr;
   else
      fieldTypeMask = TypeString;
   field.mFieldTypeName = fieldType;

   field.mFieldType = fieldTypeMask;

   field.mUserData = StringTable->insert(userData ? userData : "");
   field.mDefaultValue = StringTable->insert(defaultValue ? defaultValue : "");
   field.mFieldDescription = getDescriptionText(desc);

   field.mGroup = mComponentGroup;

   field.mHidden = hidden;

   mFields.push_back(field);

   //Before we set this, we need to do a test to see if this field was already set, like from the mission file or a taml file
   const char* curFieldData = getDataField(field.mFieldName, NULL);

   if (dStrIsEmpty(curFieldData))
      setDataField(field.mFieldName, NULL, field.mDefaultValue);
}

ComponentField* ComponentObject::getComponentField(const char *fieldName)
{
   StringTableEntry stFieldName = StringTable->insert(fieldName);

   for (S32 i = 0; i < mFields.size(); ++i)
   {
      if (mFields[i].mFieldName == stFieldName)
         return &mFields[i];
   }

   return NULL;
}

//////////////////////////////////////////////////////////////////////////

const char * ComponentObject::getDescriptionText(const char *desc)
{
   if (desc == NULL)
      return NULL;

   char *newDesc = "";

   // [tom, 1/12/2007] If it isn't a file, just do it the easy way
   if (!Platform::isFile(desc))
   {
      dsize_t newDescLen = dStrlen(desc) + 1;
      newDesc = new char[newDescLen];
      dStrcpy(newDesc, desc, newDescLen);

      return newDesc;
   }

   FileStream str;
   str.open(desc, Torque::FS::File::Read);

   Stream *stream = &str;
   if (stream == NULL){
      str.close();
      return NULL;
   }

   U32 size = stream->getStreamSize();
   if (size > 0)
   {
      newDesc = new char[size + 1];
      if (stream->read(size, (void *)newDesc))
         newDesc[size] = 0;
      else
      {
         SAFE_DELETE_ARRAY(newDesc);
      }
   }

   str.close();
   //delete stream;

   return newDesc;
}
//////////////////////////////////////////////////////////////////////////
void ComponentObject::beginFieldGroup(const char* groupName)
{
   if (dStrcmp(mComponentGroup, ""))
   {
      Con::errorf("ComponentObject: attempting to begin new field group with a group already begun!");
      return;
   }

   mComponentGroup = StringTable->insert(groupName);
}

void ComponentObject::endFieldGroup()
{
   mComponentGroup = StringTable->insert("");
}

void ComponentObject::addDependency(StringTableEntry name)
{
   mDependencies.push_back_unique(name);
}
