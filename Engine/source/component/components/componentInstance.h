//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------
#ifndef _COMPONENTINSTANCE_H_
#define _COMPONENTINSTANCE_H_

#ifndef _NETOBJECT_H_
#include "sim/netObject.h"
#endif

#ifndef _COMPONENT_OBJECT_H_
#include "component/ComponentObject.h"
#endif
#ifndef _COMPONENT_H_
#include "component/components/component.h"
#endif
#ifndef _STOCK_INTERFACES_H_
#include "component/components/stockInterfaces.h"
#endif

// Forward refs
class Component;
class ComponentObject;
struct ComponentField;

//////////////////////////////////////////////////////////////////////////
/// An Instance of a given Object Behavior
///
/// A ComponentInstance object is created from a Component object 
/// that defines it's Default Values, 
class ComponentInstance : public NetObject, public ICallMethod,
   public UpdateInterface
{
   typedef NetObject Parent;

protected:
   Component		 *mTemplate;
   ComponentObject        *mOwner;
   //Used for compound namespaces for the callbacks
   Namespace			 *mTemplateNameSpace;
   bool					 mHidden;
   bool					 mEnabled;
   bool					 mInitialized;

   //This is for dynamic fields we may want to save out to file, etc.
   struct behaviorFields
   {
      StringTableEntry mFieldName;
      StringTableEntry mDefaultValue;
   };

   Vector<ComponentField> mComponentFields;

   StringTableEntry mComponentGroup;

public:
   ComponentInstance(Component *btemplate = NULL);
   virtual ~ComponentInstance();
   DECLARE_CONOBJECT(ComponentInstance);

   virtual bool onAdd();
   virtual void onRemove();
   static void initPersistFields();

   virtual bool isMethod( const char* methodName ); //this is used because some behaviors have compound namespaces
   Namespace* getTemplateNamespace() { return mTemplateNameSpace; }

   // This must be called before the object is registered with the sim
   void setBehaviorClass(StringTableEntry className, StringTableEntry superClass)
   {
      mClassName = className;
      mSuperClassName = superClass;
   }

   void setOwner( ComponentObject* pOwner ) { mOwner = pOwner; };
   inline ComponentObject *getOwner() { return mOwner ? mOwner : NULL; };

   // Read-Only field accessor (never allows write)
   static bool setOwner( void *object, const char *index, const char *data ) { return true; };

   Component *getTemplate()        { return mTemplate; }
   const char *getTemplateName();

   const char * checkDependencies();

   bool	isEnabled() { return mEnabled; }
   void setEnabled(bool toggle) { mEnabled = toggle; setMaskBits(EnableMask); }
   bool  isInitalized() { return mInitialized; }

   virtual void packToStream( Stream &stream, U32 tabStop, S32 behaviorID, U32 flags = 0 );

   void addComponentField(const char* fieldName, const char* value);
   void removeBehaviorField(const char* fieldName);

   void addComponentField(const char *fieldName, const char *desc, const char *type, const char *defaultValue = NULL, const char *userData = NULL, bool hidden = false);
   void addComponentField(ComponentField newField);

   inline S32 getComponentFieldCount() { return mComponentFields.size(); };

   /// Gets a ComponentField by its index in the mFields vector 
   /// @param idx  The index of the field in the mField vector
   inline ComponentField *getComponentField(S32 idx)
   {
      if(idx < 0 || idx >= mComponentFields.size())
         return NULL;

      return &mComponentFields[idx];
   }

   ComponentField *getComponentField(const char* fieldName);

   virtual void onStaticModified( const char* slotName, const char* newValue ); ///< Called when a static field is modified.
   virtual void onDynamicModified(const char* slotName, const char*newValue = NULL); ///< Called when a dynamic field is modified.
   /// This is what we actually use to check if the modified field is one of our behavior fields. If it is, we update and make the correct callbacks
   void checkBehaviorFieldModified( const char* slotName, const char* newValue );

   //not used here, basically only exists to smooth the usage of custom behavior instances that might
   enum NetMaskBits 
   {
      InitialUpdateMask = BIT(0),
      UpdateMask = BIT(1),
      EnableMask = BIT(2),
      NextFreeMask = BIT(3)
   };

   /*enum
   {
   MaxRemoteCommandArgs = 20
   };*/

   virtual U32 packUpdate(NetConnection *con, U32 mask, BitStream *stream);
   virtual void unpackUpdate(NetConnection *con, BitStream *stream);
   void pushUpdate();

   virtual void update();

   virtual void onComponentAdd();
   virtual void onComponentRemove();

   virtual void processTick(const Move* move);
   virtual void interpolateTick(F32 dt){}
   virtual void advanceTime(F32 dt){}

   /// TODO: make this an interface
   virtual bool castRay(const Point3F &start, const Point3F &end, RayInfo* info){ return false; }

   //callOn_ functions
   void callOnServer(S32 argc, const char **argv);
   void callOnClient(S32 argc, const char **argv);
   void sendCommand(NetConnection *conn, S32 argc, const char **argv);

   virtual bool handlesConsoleMethod(const char * fname, S32 * routingId);
   const char* callMethodArgList(U32 argc, ConsoleValueRef argv[], bool callThis = true);
   const char* callMethod( S32 argc, const char* methodName, ... );

   virtual void handleEvent(const char* eventName, Vector<const char*> eventParams){ }

   Signal< void(SimObject*, String, String) > onDataSet;
   virtual void setDataField(StringTableEntry slotName, const char *array, const char *value);

   //Callbacks
   DECLARE_CALLBACK( void, Update, ( const char* move, const char* rot/*const Point3F& move, const Point3F& rot*/ ) );
   DECLARE_CALLBACK( void, onTrigger, ( U32 triggerID ) );
};
#endif // _COMPONENTINSTANCE_H_
