//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _COMPONENT_H_
#define _COMPONENT_H_

#ifndef _NETOBJECT_H_
#include "sim/netObject.h"
#endif

#include "component/components/componentInstance.h"

struct ComponentField
{
   StringTableEntry mFieldName;
   StringTableEntry mFieldDescription;

   StringTableEntry mFieldType;
   StringTableEntry mUserData;

   StringTableEntry mDefaultValue;

   StringTableEntry mGroup;

   StringTableEntry mDependency;

   bool mHidden;
};

//////////////////////////////////////////////////////////////////////////
/// 
/// 
//////////////////////////////////////////////////////////////////////////
class Component : public NetObject
{
   typedef NetObject Parent;
   friend class ComponentInstance; //TODO: Remove this

protected:
   StringTableEntry mFriendlyName;
   StringTableEntry mDescription;

   StringTableEntry mFromResource;
   StringTableEntry mComponentGroup;
   StringTableEntry mComponentType;
   StringTableEntry mNetworkType;
   StringTableEntry mTemplateName;

   Vector<StringTableEntry> mDependencies;
   Vector<ComponentField> mFields;

   bool mNetworked;

public:
   Component();
   virtual ~Component();
   DECLARE_CONOBJECT(Component);

   virtual bool onAdd();
   virtual void onRemove();
   static void initPersistFields();

   void pushUpdate();

   /// @name Creation Methods
   /// @{

   /// Create a ComponentInstance from this template
   /// @return   ComponentInstance   returns the newly created ComponentInstance object
   template <class T>
   ComponentInstance* createInstance();

   virtual ComponentInstance *createInstance();

   bool setupFields( ComponentInstance *bi, bool forceSetup = false );
   /// @}

   /// @name Adding Named Fields
   /// @{

   /// Adds a named field to a Component that can specify a description, data type, default value and userData
   ///
   /// @param   fieldName    The name of the Field
   /// @param   desc         The Description of the Field
   /// @param   type         The Type of field that this is, example 'Text' or 'Bool'
   /// @param   defaultValue The Default value of this field
   /// @param   userData     An extra optional field that can be used for user data
   void addComponentField(const char *fieldName, const char *desc, const char *type, const char *defaultValue = NULL, const char *userData = NULL, bool hidden = false);

   /// Returns the number of ComponentField's on this template
   inline S32 getComponentFieldCount() { return mFields.size(); };

   /// Gets a ComponentField by its index in the mFields vector 
   /// @param idx  The index of the field in the mField vector
   inline ComponentField *getComponentField(S32 idx)
   {
      if(idx < 0 || idx >= mFields.size())
         return NULL;

      return &mFields[idx];
   }

   ComponentField *getComponentField(const char* fieldName);

   const char* getComponentType() { return mComponentType; }

   const char *getDescriptionText(const char *desc);

   const char *getName() { return mTemplateName; }

   bool isNetworked() { return mNetworked; }

   void beginFieldGroup(const char* groupName);
   void endFieldGroup();

   void addDependency(StringTableEntry name);
   /// @}

   /// @name Description
   /// @{
   static bool setDescription( void *object, const char *index, const char *data );
   static const char* getDescription(void* obj, const char* data);

   /// @Primary usage functions
   /// @These are used by the various engine-based behaviors to integrate with the component classes
   enum NetMaskBits 
   {
      UpdateMask = BIT(0)
   };

   virtual U32 packUpdate(NetConnection *con, U32 mask, BitStream *stream);
   virtual void unpackUpdate(NetConnection *con, BitStream *stream);
   /// @}

};

template <class T>
ComponentInstance* Component::createInstance() 
{
   T* instance = new T(this);
   ComponentInstance* binstance = dynamic_cast<ComponentInstance*>(instance);
   if(binstance != NULL)
   {
      setupFields( binstance );

      if(binstance->registerObject())
         return binstance;
   }

   delete instance;
   delete binstance;
   return NULL;
}

#endif // _COMPONENT_H_
