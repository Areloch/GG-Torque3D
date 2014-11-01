//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "component/components/component.h"

#ifndef _StateMachineBehavior_H_
#define _StateMachineBehavior_H_

#ifndef _ENTITY_H_
#include "T3D/Entity.h"
#endif
#ifndef _RENDERSHAPEBEHAVIOR_H_
#include "component/components/render/renderShapeBehavior.h"
#endif

class TSShapeInstance;
class TSThread;
class SceneRenderState;
class StateMachineBehaviorInstance;
//////////////////////////////////////////////////////////////////////////
/// 
/// 
//////////////////////////////////////////////////////////////////////////
class StateMachineBehavior : public Component
{
   typedef Component Parent;

public:
   StateMachineBehavior();
   virtual ~StateMachineBehavior();
   DECLARE_CONOBJECT(StateMachineBehavior);

   virtual bool onAdd();
   virtual void onRemove();
   static void initPersistFields();

   virtual U32 packUpdate(NetConnection *con, U32 mask, BitStream *stream);
   virtual void unpackUpdate(NetConnection *con, BitStream *stream);

   //override to pass back a AnimationBehaviorInstance
   virtual ComponentInstance *createInstance();
};

class StateMachineBehaviorInstance : public ComponentInstance
{
   typedef ComponentInstance Parent;

public:
   struct StateField
   {
      StringTableEntry name;
      StringTableEntry value;
      StringTableEntry type;
   };

   struct UniqueReference
   {
      SimObject* referenceObj;
      const char* referenceVar;
      const char* uniqueName;
   };

   struct StateTransition
   {
      struct Rule
      {
         enum triggerValueTarget
         {
            Equals = 0,
            GeaterThan,
            LessThan,
            GreaterOrEqual,
            LessOrEqual,
            True,
            False,
            Positive,
            Negative,
            DoesNotEqual
         };

         enum triggerValueType
         {
            BooleanType = 0,
            NumberType,
            VectorType,
            StringType
         };

         triggerValueType		valueType;
         triggerValueTarget	triggerTarget;

         bool 			 triggerBoolVal;
         float 		 triggerNumVal;
         Point3F 		 triggerVectorVal;
         String 		 triggerStringVal;
         
         UniqueReference *valUniqueRef;
      };

      StringTableEntry	mName;
      StringTableEntry	mStateTarget;
      Vector<Rule>		mTransitionRules;
   };

   struct State {
      Vector<StateTransition> mTransitions;

      Vector<StateField> mProperties;

      StringTableEntry stateName;

      StringTableEntry callbackName;
   };

protected:
   Vector<State*> mStates;

   Vector<UniqueReference> mUniqueReferences;

   State* mCurrentState;

   F32 mStateStartTime;
   F32 mStateTime;

   StringTableEntry mStartingState;

   State *mCurCreateSuperState;
   State *mCurCreateState;

public:
   StateMachineBehaviorInstance(Component *btemplate = NULL);
   virtual ~StateMachineBehaviorInstance();
   DECLARE_CONOBJECT(StateMachineBehaviorInstance);

   virtual bool onAdd();
   virtual void onRemove();
   static void initPersistFields();

   virtual void onComponentAdd();
   virtual void onComponentRemove();

   //virtual bool setBehaviorSubField( const char *data );

   //shortcut function. finds if our owner has a renderShape behavior and gets the shape instance
   //RenderShapeBehaviorInstance* getShapeBehavior();

   virtual void processTick(const Move* move);

   virtual U32 packUpdate(NetConnection *con, U32 mask, BitStream *stream);
   virtual void unpackUpdate(NetConnection *con, BitStream *stream);

   virtual void handleEvent(const char* eventName, Vector<const char*> eventParams);

   void setState(const char* stateName, bool clearFields = true);
   const char* getCurrentStateName() { return mCurrentState->stateName; }

   void addState(const char* stateName);
   void removeState(const char* stateName){}
   S32 getStateCount() { return mStates.size(); }
   const char* getStateByIndex(S32 index);
   State* getStateByName(const char* name);
   void setStateName(const char* stateName, const char* newStateName);

   void startSuperState(const char* stateName);
   void endSuperState();

   void startState(const char* stateName);
   void endState();

   bool addObjVarReference(SimObject *target, const char* targetVarName, const char* uniqueName);
   void onObjVarRefChanged(SimObject *referenceObj, String targetVarName, String newValue);

   void setCallback(const char* functionName);

   void addStateField(const char* stateName, const char* fieldName, const char* type, const char* value);
   void addTransition(const char* stateName, const char* fieldName, const char* targetStateName, const char* eventTrigger);

   void addStringTransition(const char* fieldName, const char* targetStateName, const char* valueTrigger, S32 valueComparitor);
   void addNumericTransition(const char* fieldName, const char* targetStateName, F32 valueTrigger, S32 valueComparitor);
   void addBooleanTransition(const char* fieldName, const char* targetStateName, bool valueTrigger, S32 valueComparitor);
   void addVectorTransition(const char* fieldName, const char* targetStateName, Point3F valueTrigger, S32 valueComparitor);

   void checkTransitions( const char* slotName, const char* newValue );

   virtual void onDynamicModified( const char* slotName, const char* newValue );
   virtual void onStaticModified( const char* slotName, const char* newValue );
   S32 getVariableType(const char* var);
   bool passComparitorCheck(const char* var, StateTransition::Rule transitionRule);

   //Callbacks
   DECLARE_CALLBACK( void, onStateChange, () );
};

#endif // _COMPONENT_H_