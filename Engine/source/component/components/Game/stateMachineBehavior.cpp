//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "component/components/game/StateMachineBehavior.h"
#include "component/components/render/renderShapeBehavior.h"

#include "platform/platform.h"
#include "console/consoleTypes.h"
#include "core/util/safeDelete.h"
#include "core/resourceManager.h"
#include "core/stream/fileStream.h"
#include "console/consoleTypes.h"
#include "console/consoleObject.h"
#include "ts/tsShapeInstance.h"
#include "core/stream/bitStream.h"
//#include "console/consoleInternal.h"
#include "gfx/gfxTransformSaver.h"
#include "console/engineAPI.h"
#include "lighting/lightQuery.h"

IMPLEMENT_CALLBACK( StateMachineBehaviorInstance, onStateChange, void, (), (),
                   "@brief Called when we collide with another object.\n\n"
                   "@param obj The ShapeBase object\n"
                   "@param collObj The object we collided with\n"
                   "@param vec Collision impact vector\n"
                   "@param len Length of the impact vector\n" );

//////////////////////////////////////////////////////////////////////////
// Constructor/Destructor
//////////////////////////////////////////////////////////////////////////

StateMachineBehavior::StateMachineBehavior()
{
   mNetFlags.set(Ghostable | ScopeAlways);

   mFriendlyName = "State Machine";
   mComponentType = "Game";

   mDescription = getDescriptionText("A generic state machine.");

   addComponentField("startingState", "The initial state for the machine", "", "");
}

StateMachineBehavior::~StateMachineBehavior()
{
   for(S32 i = 0;i < mFields.size();++i)
   {
      ComponentField &field = mFields[i];
      SAFE_DELETE_ARRAY(field.mFieldDescription);
   }

   SAFE_DELETE_ARRAY(mDescription);
}

IMPLEMENT_CO_NETOBJECT_V1(StateMachineBehavior);

//////////////////////////////////////////////////////////////////////////
ComponentInstance *StateMachineBehavior::createInstance()
{
   StateMachineBehaviorInstance *instance = new StateMachineBehaviorInstance(this);

   setupFields( instance );

   if(instance->registerObject())
      return instance;

   delete instance;
   return NULL;
}

bool StateMachineBehavior::onAdd()
{
   if(! Parent::onAdd())
      return false;

   return true;
}

void StateMachineBehavior::onRemove()
{
   Parent::onRemove();
}
void StateMachineBehavior::initPersistFields()
{
   Parent::initPersistFields();
}

U32 StateMachineBehavior::packUpdate(NetConnection *con, U32 mask, BitStream *stream)
{
   U32 retMask = Parent::packUpdate(con, mask, stream);
   return retMask;
}

void StateMachineBehavior::unpackUpdate(NetConnection *con, BitStream *stream)
{
   Parent::unpackUpdate(con, stream);
}

//==========================================================================================
//==========================================================================================
StateMachineBehaviorInstance::StateMachineBehaviorInstance( Component *btemplate ) 
{
   mTemplate = btemplate;
   mOwner = NULL;

   mCurrentState = NULL;
   mStateStartTime = 0;
   mStateTime = 0;

   mStartingState = "";

   mCurCreateState = NULL;

   mNetFlags.set(Ghostable);
}

StateMachineBehaviorInstance::~StateMachineBehaviorInstance()
{
}
IMPLEMENT_CO_NETOBJECT_V1(StateMachineBehaviorInstance);

bool StateMachineBehaviorInstance::onAdd()
{
   if(! Parent::onAdd())
      return false;

   return true;
}

void StateMachineBehaviorInstance::onRemove()
{
   Parent::onRemove();
}

//This is mostly a catch for situations where the behavior is re-added to the object and the like and we may need to force an update to the behavior
void StateMachineBehaviorInstance::onComponentAdd()
{
   Parent::onComponentAdd();
}

void StateMachineBehaviorInstance::onComponentRemove()
{
   Parent::onComponentRemove();
}

void StateMachineBehaviorInstance::initPersistFields()
{
   Parent::initPersistFields();

   addField("stateStartTime", TypeF32, Offset(mStateStartTime, StateMachineBehaviorInstance), "The sim time of when we started this state");
   addField("stateTime", TypeF32, Offset(mStateStartTime, StateMachineBehaviorInstance), "The current amount of time we've spent in this state");
   addField("startingState", TypeString, Offset(mStartingState, StateMachineBehaviorInstance), "The current amount of time we've spent in this state");
}

U32 StateMachineBehaviorInstance::packUpdate(NetConnection *con, U32 mask, BitStream *stream)
{
   U32 retMask = Parent::packUpdate(con, mask, stream);

   return retMask;
}

void StateMachineBehaviorInstance::unpackUpdate(NetConnection *con, BitStream *stream)
{
   Parent::unpackUpdate(con, stream);
}

void StateMachineBehaviorInstance::processTick(const Move* move)
{
   if (!isServerObject())
      return;

   if (mCurrentState == NULL)
   {
      //if we don't have a state, pick our default state
      if (!dStrcmp(mStartingState,""))
         mCurrentState = getStateByName(getStateByIndex(0)); 
      else
         mCurrentState = getStateByName(mStartingState);

      if (mCurrentState == NULL)
         return;
      else
         mStateStartTime = Sim::getCurrentTime();
   }
   //we always check if there's a timout transition, as that's the most generic transition possible.
   F32 curTime = Sim::getCurrentTime();

   mStateTime = curTime - mStateStartTime;

   char buffer[64];
   dSprintf( buffer, sizeof( buffer ), "%g", mStateTime );

   checkTransitions( "stateTime", buffer );
}

void StateMachineBehaviorInstance::onDynamicModified( const char* slotName, const char* newValue )
{
   Parent::onDynamicModified(slotName, newValue);

   checkTransitions(slotName, newValue);
}

void StateMachineBehaviorInstance::onStaticModified( const char* slotName, const char* newValue )
{
   Parent::onStaticModified(slotName, newValue);

   checkTransitions(slotName, newValue);
}

void StateMachineBehaviorInstance::checkTransitions( const char* slotName, const char* newValue )
{
   //because we use our current state's fields as dynamic fields on the instance
   //we'll want to catch any fields being set so we can treat changes as transition triggers if
   //any of the transitions on this state call for it

   //One example would be in order to implement burst fire on a weapon state machine.
   //The behavior instance has a dynamic variable set up like: GunStateMachine.burstShotCount = 0;

   //We also have a transition in our fire state, as: GunStateMachine.addTransition("FireState", "burstShotCount", "DoneShooting", 3);
   //What that does is for our fire state, we check the dynamicField burstShotCount if it's equal or greater than 3. If it is, we perform the transition.

   //As state fields are handled as dynamicFields for the instance, regular dynamicFields are processed as well as state fields. So we can use the regular 
   //dynamic fields for our transitions, to act as 'global' variables that are state-agnostic. Alternately, we can use state-specific fields, such as a transition
   //like this:
   //GunStateMachine.addTransition("IdleState", "Fidget", "Timeout", ">=", 5000);

   //That uses the the timeout field, which is reset each time the state changes, and so state-specific, to see if it's been 5 seconds. If it has been, we transition
   //to our fidget state

   //so, lets check our current transitions
   if(mCurrentState)
   {
      //now that we have the type, check our transitions!
      for(U32 t = 0; t < mCurrentState->mTransitions.size(); t++)
      {
         if(!dStrcmp(mCurrentState->mTransitions[t].mName,slotName))
         {
            //found a transition looking for this variable, so do work
            //first, figure out what data type thie field is
            //S32 type = getVariableType(newValue);

            bool fail = false;
            for(U32 r = 0; r < mCurrentState->mTransitions[t].mTransitionRules.size(); r++)
            {
               //now, check the value with the comparitor and see if we do the transition.
               if(!passComparitorCheck(newValue, mCurrentState->mTransitions[t].mTransitionRules[r])){
                  fail = true;
                  break;
               }
            }

            if(!fail)
               setState(mCurrentState->mTransitions[t].mStateTarget);

            return;
         }
      }
   }
}

S32 StateMachineBehaviorInstance::getVariableType(const char* var)
{
   S32 type = -1;

   Point3F vector;
   F32 number;

   //bail pre-emptively if it's nothing
   if(!var || !var[0])
      return -1;

   if(dSscanf( var, "%g %g %g", &vector.x, &vector.y, &vector.z ) == 3)
      type = StateTransition::Rule::VectorType;
   else
   {
      if(dSscanf( var, "%g", &number ))
         type = StateTransition::Rule::NumberType;
      else
         type = StateTransition::Rule::StringType;
   }

   //final check if it's a number or string: see if it's actually a boolean!
   if(type == StateTransition::Rule::NumberType)
   {
      number = dAtoi(var);
      if(number == 0 || number == 1)
         type = StateTransition::Rule::BooleanType;
   }
   else if(type == StateTransition::Rule::StringType)
   {
      if(!dStrcmp("false", var) || !dStrcmp("true", var))
         type = StateTransition::Rule::BooleanType;
   }

   return type;
}

bool StateMachineBehaviorInstance::passComparitorCheck(const char* var, StateTransition::Rule transitionRule)
{
   F32 num = dAtof(var);
   switch(transitionRule.valueType)
   {
   case StateTransition::Rule::VectorType:
      switch(transitionRule.triggerTarget)
      {
      case StateTransition::Rule::Equals:
      case StateTransition::Rule::GeaterThan:
      case StateTransition::Rule::GreaterOrEqual:
      case StateTransition::Rule::LessThan:
      case StateTransition::Rule::LessOrEqual:
      case StateTransition::Rule::DoesNotEqual:
         //do
         break;
      default:
         return false;
      };
   case StateTransition::Rule::StringType:
      switch(transitionRule.triggerTarget)
      {
      case StateTransition::Rule::Equals:
         if(!dStrcmp(var, transitionRule.triggerStringVal))
            return true;
         else
            return false;
      case StateTransition::Rule::DoesNotEqual:
         if(dStrcmp(var, transitionRule.triggerStringVal))
            return true;
         else
            return false;
      default:
         return false;
      };
   case StateTransition::Rule::BooleanType:
      switch(transitionRule.triggerTarget)
      {
      case StateTransition::Rule::triggerValueTarget::True:
         if(dAtob(var))
            return true;
         else
            return false;
      case StateTransition::Rule::triggerValueTarget::False:
         if(dAtob(var))
            return false;
         else
            return true;
      default:
         return false;
      };
   case StateTransition::Rule::NumberType:
      switch(transitionRule.triggerTarget)
      {
      case StateTransition::Rule::triggerValueTarget::Equals:
         if(num == transitionRule.triggerNumVal)
            return true;
         else 
            return false;
      case StateTransition::Rule::triggerValueTarget::GeaterThan:
         if(num > transitionRule.triggerNumVal)
            return true;
         else 
            return false;
      case StateTransition::Rule::triggerValueTarget::GreaterOrEqual:
         if(num >= transitionRule.triggerNumVal)
            return true;
         else 
            return false;
      case StateTransition::Rule::triggerValueTarget::LessThan:
         if(num < transitionRule.triggerNumVal)
            return true;
         else 
            return false;
      case StateTransition::Rule::triggerValueTarget::LessOrEqual:
         if(num <= transitionRule.triggerNumVal)
            return true;
         else 
            return false;
      case StateTransition::Rule::triggerValueTarget::DoesNotEqual:
         if(num != transitionRule.triggerNumVal)
            return true;
         else 
            return false;
      case StateTransition::Rule::triggerValueTarget::Positive:
         if(num > 0)
            return true;
         else 
            return false;
      case StateTransition::Rule::triggerValueTarget::Negative:
         if(num < 0)
            return true;
         else 
            return false;
      default:
         return false;
      };
   default:
      return false;
   };
}

//
void StateMachineBehaviorInstance::addState(const char* stateName)
{
   State *newState = new State;
   newState->stateName = StringTable->insert(stateName);

   //check if it already exists
   State *gtStt = getStateByName(stateName);
   if(!gtStt)
      mStates.push_back(newState);
}

void StateMachineBehaviorInstance::setState(const char* stateName, bool clearFields)
{
   State* oldState = mCurrentState;
   for(U32 i=0; i < mStates.size(); i++)
   {
      if(!dStrcmp(mStates[i]->stateName, stateName))
      {
         mCurrentState = mStates[i];
         mStateStartTime = Sim::getCurrentTime();

         //clear the current dynamic fields from the old state
         //we have a variable here so we can override the clearing of fields if needed.
         //This allows for 'compound states', where some elements from each state are active.
         //This isn't normally used, but look at the playerController behavior's setup to see
         //it in action with the stances system
         if(clearFields)
         {
            if(oldState)
            {
               for(U32 s=0; s < oldState->mProperties.size(); s++)
               {
                  setDataField( oldState->mProperties[s].name, NULL, "" );
               }
            }
         }

         //now add the new state's fields as dynamic fields
         for(U32 p=0; p < mCurrentState->mProperties.size(); p++)
         {
            setDataField( mCurrentState->mProperties[p].name, NULL, mCurrentState->mProperties[p].value );
         }

         onStateChange_callback();

         //if we have a callback function for this state, call it now
         if (mCurrentState->callbackName != "")
            Con::executef(this, mCurrentState->callbackName);

         return;
      }
   }
}

const char* StateMachineBehaviorInstance::getStateByIndex(S32 index)
{ 
   if(index >= 0 && mStates.size() > index) 
      return mStates[index]->stateName;
   else
      return "";
}

StateMachineBehaviorInstance::State* StateMachineBehaviorInstance::getStateByName(const char* name)
{ 
   StringTableEntry stateName = StringTable->insert(name);

   for(U32 i=0; i < mStates.size(); i++)
   {
      if(!dStrcmp(stateName, mStates[i]->stateName))
         return mStates[i];
   }

   return NULL;
}

void StateMachineBehaviorInstance::setStateName(const char* stateName, const char* newName)
{
   for(U32 i=0; i < mStates.size(); i++)
   {
      if(!dStrcmp(mStates[i]->stateName, stateName))
      {
         mStates[i]->stateName = StringTable->insert(newName);
         return;
      }
   }
}

void StateMachineBehaviorInstance::startSuperState(const char* stateName)
{
   /*if (dStrcmp(mCurCreateSuperState, ""))
   {
      Con::errorf("Component: attempting to begin new field group with a group already begun!");
      return;
   }

   mCurCreateSuperState = StringTable->insert(stateName);*/
}

void StateMachineBehaviorInstance::endSuperState()
{
   //mCurCreateSuperState = StringTable->insert("");
}

void StateMachineBehaviorInstance::startState(const char* stateName)
{
   if (mCurCreateState != NULL)
   {
      Con::errorf("Component: attempting to begin new field group with a group already begun!");
      return;
   }

   State *newState = new State();
   newState->stateName = StringTable->insert(stateName);

   mStates.push_back(newState);

   mCurCreateState = newState;
}

void StateMachineBehaviorInstance::endState()
{
   mCurCreateState = NULL;
}

void StateMachineBehaviorInstance::setCallback(const char* functionName)
{
   State* state = mCurCreateState;
   if (!state)
      return;

   state->callbackName = StringTable->insert(functionName);
}

bool StateMachineBehaviorInstance::addObjVarReference(SimObject *target, const char* targetVarName, const char* uniqueName)
{
   if (target == NULL || !dStrcmp(targetVarName, "") || !dStrcmp(uniqueName, ""))
      return false;

   Entity *entTarget = dynamic_cast<Entity*>(target);

   if (entTarget != NULL)
   {
      entTarget->onDataSet.notify(this, &StateMachineBehaviorInstance::onObjVarRefChanged);

      UniqueReference newRef;
      newRef.referenceObj = target;
      newRef.referenceVar = targetVarName;
      newRef.uniqueName = uniqueName;

      mUniqueReferences.push_back(newRef);
      return true;
   }
   
   ComponentInstance* compInst = dynamic_cast<ComponentInstance*>(target);

   if (compInst != NULL)
   {
      compInst->onDataSet.notify(this, &StateMachineBehaviorInstance::onObjVarRefChanged);

      UniqueReference newRef;
      newRef.referenceObj = target;
      newRef.referenceVar = targetVarName;
      newRef.uniqueName = uniqueName;

      mUniqueReferences.push_back(newRef);
      return true;
   }

   return false;
}

void StateMachineBehaviorInstance::onObjVarRefChanged(SimObject *referenceObj, String targetVarName, String newValue)
{
   for (U32 i = 0; i < mUniqueReferences.size(); i++)
   {
      if (mUniqueReferences[i].referenceObj == referenceObj && !dStrcmp(mUniqueReferences[i].referenceVar, targetVarName))
      {
         //we have a hit, so do a transition check
         checkTransitions(mUniqueReferences[i].uniqueName, newValue);
      }
   }
}

//
void StateMachineBehaviorInstance::addStateField(const char* stateName, const char* fieldName, const char* type, const char* value)
{
   StateField newField;
   newField.name = StringTable->insert(fieldName);
   newField.type = StringTable->insert(type);
   newField.value = StringTable->insert(value);

   for(U32 i=0; i < mStates.size(); i++)
   {
      //find the state
      if(!dStrcmp(stateName, mStates[i]->stateName))
      {
         mStates[i]->mProperties.push_back(newField);
         return;
      }
   }	
}

void StateMachineBehaviorInstance::addTransition(const char* stateName, const char* fieldName, const char* targetStateName, const char* eventTrigger)
{
   /*StateField newField;
   newField.name = StringTable->insert(fieldName);
   newField.type = StringTable->insert(eventTrigger);
   newField.value = StringTable->insert(targetStateName);

   for(U32 i=0; i < mStates.size(); i++)
   {
   //find the state
   if(!dStrcmp(stateName, mStates[i]->stateName))
   {
   mStates[i]->mTransitions.push_back(newField);
   return;
   }
   }*/
}

//////////////
void StateMachineBehaviorInstance::addStringTransition(const char* fieldName, const char* targetStateName, const char* valueTrigger, S32 valueComparitor)
{
   State* state = mCurCreateState;
   if(!state)
      return;

   StringTableEntry targetName = StringTable->insert(targetStateName);

   //check if we already have a transition from this state to our target.
   //if we do, just add another rule to the transition
   for(U32 i=0; i < state->mTransitions.size(); i++)
   {
      if(!dStrcmp(state->mTransitions[i].mStateTarget, targetName))
      {
         //yup, there it is
         //just build a new rule then
         StateTransition::Rule newRule;

         newRule.triggerStringVal = StringTable->insert(valueTrigger);
         newRule.triggerTarget = static_cast<StateTransition::Rule::triggerValueTarget>(valueComparitor);
         newRule.valueType = StateTransition::Rule::StringType;

         state->mTransitions[i].mTransitionRules.push_back(newRule);
         return;
      }
   }

   //welp, didn't find a matching existing one, so make a new transition
   StateTransition newTransition;
   newTransition.mName = StringTable->insert(fieldName);
   newTransition.mStateTarget = targetName;

   StateTransition::Rule newRule;
   newRule.triggerStringVal = StringTable->insert(valueTrigger);
   newRule.triggerTarget = static_cast<StateTransition::Rule::triggerValueTarget>(valueComparitor);
   newRule.valueType = StateTransition::Rule::StringType;

   newTransition.mTransitionRules.push_back(newRule);

   state->mTransitions.push_back(newTransition);
}

void StateMachineBehaviorInstance::addNumericTransition(const char* fieldName, const char* targetStateName, F32 valueTrigger, S32 valueComparitor)
{
   State* state = mCurCreateState;
   if(!state)
      return;

   StringTableEntry targetName = StringTable->insert(targetStateName);

   //check if we already have a transition from this state to our target.
   //if we do, just add another rule to the transition
   for(U32 i=0; i < state->mTransitions.size(); i++)
   {
      if(!dStrcmp(state->mTransitions[i].mStateTarget, targetName))
      {
         //yup, there it is
         //just build a new rule then
         StateTransition::Rule newRule;

         newRule.triggerNumVal = valueTrigger;
         newRule.triggerTarget = static_cast<StateTransition::Rule::triggerValueTarget>(valueComparitor);
         newRule.valueType = StateTransition::Rule::NumberType;

         state->mTransitions[i].mTransitionRules.push_back(newRule);
         return;
      }
   }

   //welp, didn't find a matching existing one, so make a new transition
   StateTransition newTransition;
   newTransition.mName = StringTable->insert(fieldName);
   newTransition.mStateTarget = targetName;

   StateTransition::Rule newRule;
   newRule.triggerNumVal = valueTrigger;
   newRule.triggerTarget = static_cast<StateTransition::Rule::triggerValueTarget>(valueComparitor);
   newRule.valueType = StateTransition::Rule::NumberType;

   newTransition.mTransitionRules.push_back(newRule);

   state->mTransitions.push_back(newTransition);
}

void StateMachineBehaviorInstance::addBooleanTransition(const char* fieldName, const char* targetStateName, bool valueTrigger, S32 valueComparitor)
{
   State* state = mCurCreateState;
   if(!state)
      return;

   StringTableEntry targetName = StringTable->insert(targetStateName);

   //check if we already have a transition from this state to our target.
   //if we do, just add another rule to the transition
   for(U32 i=0; i < state->mTransitions.size(); i++)
   {
      if(!dStrcmp(state->mTransitions[i].mStateTarget, targetName))
      {
         //yup, there it is
         //just build a new rule then
         StateTransition::Rule newRule;

         newRule.triggerBoolVal = valueTrigger;
         newRule.triggerTarget = static_cast<StateTransition::Rule::triggerValueTarget>(valueComparitor);
         newRule.valueType = StateTransition::Rule::BooleanType;

         state->mTransitions[i].mTransitionRules.push_back(newRule);
         return;
      }
   }

   //welp, didn't find a matching existing one, so make a new transition
   StateTransition newTransition;
   newTransition.mName = StringTable->insert(fieldName);
   newTransition.mStateTarget = targetName;

   StateTransition::Rule newRule;
   newRule.triggerBoolVal = valueTrigger;
   newRule.triggerTarget = static_cast<StateTransition::Rule::triggerValueTarget>(valueComparitor);
   newRule.valueType = StateTransition::Rule::BooleanType;

   newTransition.mTransitionRules.push_back(newRule);

   state->mTransitions.push_back(newTransition);
}

void StateMachineBehaviorInstance::addVectorTransition(const char* fieldName, const char* targetStateName, Point3F valueTrigger, S32 valueComparitor)
{
   State* state = mCurCreateState;
   if(!state)
      return;

   StringTableEntry targetName = StringTable->insert(targetStateName);

   //check if we already have a transition from this state to our target.
   //if we do, just add another rule to the transition
   for(U32 i=0; i < state->mTransitions.size(); i++)
   {
      if(!dStrcmp(state->mTransitions[i].mStateTarget, targetName))
      {
         //yup, there it is
         //just build a new rule then
         StateTransition::Rule newRule;

         newRule.triggerVectorVal = valueTrigger;
         newRule.triggerTarget = static_cast<StateTransition::Rule::triggerValueTarget>(valueComparitor);
         newRule.valueType = StateTransition::Rule::VectorType;

         state->mTransitions[i].mTransitionRules.push_back(newRule);
         return;
      }
   }

   //welp, didn't find a matching existing one, so make a new transition
   StateTransition newTransition;
   newTransition.mName = StringTable->insert(fieldName);
   newTransition.mStateTarget = targetName;

   StateTransition::Rule newRule;
   newRule.triggerVectorVal = valueTrigger;
   newRule.triggerTarget = static_cast<StateTransition::Rule::triggerValueTarget>(valueComparitor);
   newRule.valueType = StateTransition::Rule::VectorType;

   newTransition.mTransitionRules.push_back(newRule);

   state->mTransitions.push_back(newTransition);
}

/////////
void StateMachineBehaviorInstance::handleEvent(const char* eventName, Vector<const char*> eventParams)
{
   //check transitions
   /*for(U32 t = 0; t < mCurrentState->mTransitions.size(); t++)
   {
   if(!dStrcmp(mCurrentState->mTransitions[t].type, eventName))
   {
   //we have a match on the event, so switch states
   setState(mCurrentState->mTransitions[t].value);
   }
   }*/
}

DefineEngineMethod( StateMachineBehaviorInstance, setState, void,
                   ( const char* stateName, bool clearFields ), ( "", true ),
                   "@brief Mount objB to this object at the desired slot with optional transform.\n\n"

                   "@param objB  Object to mount onto us\n"
                   "@param slot  Mount slot ID\n"
                   "@param txfm (optional) mount offset transform\n"
                   "@return true if successful, false if failed (objB is not valid)" )
{
   object->setState( stateName, clearFields );
}

DefineEngineMethod( StateMachineBehaviorInstance, getState, const char*, (),,
                   "@brief Mount objB to this object at the desired slot with optional transform.\n\n"

                   "@param objB  Object to mount onto us\n"
                   "@param slot  Mount slot ID\n"
                   "@param txfm (optional) mount offset transform\n"
                   "@return true if successful, false if failed (objB is not valid)" )
{
   return object->getCurrentStateName();
}

DefineEngineMethod( StateMachineBehaviorInstance, getStateCount, S32,
                   ( ),,
                   "@brief Mount objB to this object at the desired slot with optional transform.\n\n"

                   "@param objB  Object to mount onto us\n"
                   "@param slot  Mount slot ID\n"
                   "@param txfm (optional) mount offset transform\n"
                   "@return true if successful, false if failed (objB is not valid)" )
{
   return object->getStateCount();
}

DefineEngineMethod( StateMachineBehaviorInstance, getStateByIndex, const char*,
                   ( S32 index ),,
                   "@brief Mount objB to this object at the desired slot with optional transform.\n\n"

                   "@param objB  Object to mount onto us\n"
                   "@param slot  Mount slot ID\n"
                   "@param txfm (optional) mount offset transform\n"
                   "@return true if successful, false if failed (objB is not valid)" )
{
   return object->getStateByIndex( index );
}

DefineEngineMethod(StateMachineBehaviorInstance, addObjectVarReference, bool,
   (SimObject* target, const char* targetVarName, const char* uniqueName), (NULL, "", ""),
   "@brief Mount objB to this object at the desired slot with optional transform.\n\n"

   "@param objB  Object to mount onto us\n"
   "@param slot  Mount slot ID\n"
   "@param txfm (optional) mount offset transform\n"
   "@return true if successful, false if failed (objB is not valid)")
{
   return object->addObjVarReference(target, targetVarName, uniqueName);
}

DefineEngineMethod(StateMachineBehaviorInstance, StartState, void,
                   ( const char* stateName ), ( "" ),
                   "@brief Mount objB to this object at the desired slot with optional transform.\n\n"

                   "@param objB  Object to mount onto us\n"
                   "@param slot  Mount slot ID\n"
                   "@param txfm (optional) mount offset transform\n"
                   "@return true if successful, false if failed (objB is not valid)" )
{
   object->startState(stateName);
}

DefineEngineMethod(StateMachineBehaviorInstance, EndState, void,
   (),,
   "@brief Mount objB to this object at the desired slot with optional transform.\n\n"

   "@param objB  Object to mount onto us\n"
   "@param slot  Mount slot ID\n"
   "@param txfm (optional) mount offset transform\n"
   "@return true if successful, false if failed (objB is not valid)")
{
   object->endState();
}

DefineEngineMethod( StateMachineBehaviorInstance, removeState, void,
                   ( const char* stateName ), ( "" ),
                   "@brief Mount objB to this object at the desired slot with optional transform.\n\n"

                   "@param objB  Object to mount onto us\n"
                   "@param slot  Mount slot ID\n"
                   "@param txfm (optional) mount offset transform\n"
                   "@return true if successful, false if failed (objB is not valid)" )
{
   object->removeState( stateName );
}

DefineEngineMethod( StateMachineBehaviorInstance, renameState, void,
                   ( const char* stateName, const char* newName ),,
                   "@brief Mount objB to this object at the desired slot with optional transform.\n\n"

                   "@param objB  Object to mount onto us\n"
                   "@param slot  Mount slot ID\n"
                   "@param txfm (optional) mount offset transform\n"
                   "@return true if successful, false if failed (objB is not valid)" )
{
   object->setStateName( stateName, newName );
}

DefineEngineMethod( StateMachineBehaviorInstance, addStateField, void,
                   ( const char* stateName, const char* fieldName, const char* type, const char* value ), 
                   ( "", "", "", "" ),
                   "@brief Mount objB to this object at the desired slot with optional transform.\n\n"

                   "@param objB  Object to mount onto us\n"
                   "@param slot  Mount slot ID\n"
                   "@param txfm (optional) mount offset transform\n"
                   "@return true if successful, false if failed (objB is not valid)" )
{
   object->addStateField( stateName, fieldName, type, value );
}

DefineEngineMethod(StateMachineBehaviorInstance, setCallback, void,
   (const char* functionName), (""),
   "@brief Mount objB to this object at the desired slot with optional transform.\n\n"

   "@param objB  Object to mount onto us\n"
   "@param slot  Mount slot ID\n"
   "@param txfm (optional) mount offset transform\n"
   "@return true if successful, false if failed (objB is not valid)")
{
   if (!dStrcmp(functionName, ""))
      return;

   object->setCallback(functionName);
}

DefineEngineMethod( StateMachineBehaviorInstance, removeStateField, void,
                   ( const char* stateName, const char* fieldName, const char* type, const char* value ), 
                   ( "", "", "", "" ),
                   "@brief Mount objB to this object at the desired slot with optional transform.\n\n"

                   "@param objB  Object to mount onto us\n"
                   "@param slot  Mount slot ID\n"
                   "@param txfm (optional) mount offset transform\n"
                   "@return true if successful, false if failed (objB is not valid)" )
{
   object->addStateField( stateName, fieldName, type, value );
}

///////////////
DefineEngineMethod( StateMachineBehaviorInstance, addTransition, void,
                   ( const char* targetStateName, const char* fieldName, const char* valueComparitor, const char* valueTarget ), 
                   ( "", "", "String", "" ),
                   "@brief Mount objB to this object at the desired slot with optional transform.\n\n"

                   "@param objB  Object to mount onto us\n"
                   "@param slot  Mount slot ID\n"
                   "@param txfm (optional) mount offset transform\n"
                   "@return true if successful, false if failed (objB is not valid)" )
{
   S32 type = -1;
   S32 targetType = -1;

   Point3F vector;
   F32 number;
   if(dSscanf( valueTarget, "%g %g %g", &vector.x, &vector.y, &vector.z ) == 3)
      type = StateMachineBehaviorInstance::StateTransition::Rule::VectorType;
   else
   {
      if(dSscanf( valueTarget, "%g", &number ))
         type = StateMachineBehaviorInstance::StateTransition::Rule::NumberType;
      else
         type = StateMachineBehaviorInstance::StateTransition::Rule::StringType;
   }

   if(!dStrcmp(">", valueComparitor))
      targetType = StateMachineBehaviorInstance::StateTransition::Rule::GeaterThan;
   else if(!dStrcmp(">=", valueComparitor))
      targetType = StateMachineBehaviorInstance::StateTransition::Rule::GreaterOrEqual;
   else if(!dStrcmp("<", valueComparitor))
      targetType = StateMachineBehaviorInstance::StateTransition::Rule::LessThan;
   else if(!dStrcmp("<=", valueComparitor))
      targetType = StateMachineBehaviorInstance::StateTransition::Rule::LessOrEqual;
   else if(!dStrcmp("==", valueComparitor))
      targetType = StateMachineBehaviorInstance::StateTransition::Rule::Equals;
   else if(!dStrcmp("true", valueComparitor))
      targetType = StateMachineBehaviorInstance::StateTransition::Rule::True;
   else if(!dStrcmp("false", valueComparitor))
      targetType = StateMachineBehaviorInstance::StateTransition::Rule::False;
   else if(!dStrcmp("-", valueComparitor))
      targetType = StateMachineBehaviorInstance::StateTransition::Rule::Negative;
   else if(!dStrcmp("+", valueComparitor))
      targetType = StateMachineBehaviorInstance::StateTransition::Rule::Positive;
   else if(!dStrcmp("!=", valueComparitor))
      targetType = StateMachineBehaviorInstance::StateTransition::Rule::DoesNotEqual;

   //final check if it's a number or string: see if it's actually a boolean!
   if(type == StateMachineBehaviorInstance::StateTransition::Rule::NumberType)
   {
      number = dAtoi(valueTarget);
      if(number == 0 || number == 1)
         type = StateMachineBehaviorInstance::StateTransition::Rule::BooleanType;

   }
   else if(type == StateMachineBehaviorInstance::StateTransition::Rule::NumberType)
   {
      if(!dStrcmp("false", valueTarget) || !dStrcmp("true", valueTarget))
         type = StateMachineBehaviorInstance::StateTransition::Rule::BooleanType;
   }

   if(type == StateMachineBehaviorInstance::StateTransition::Rule::StringType){
      object->addStringTransition( fieldName, targetStateName, valueTarget, targetType );
   }
   else if(type == StateMachineBehaviorInstance::StateTransition::Rule::BooleanType){
      object->addBooleanTransition( fieldName, targetStateName, dAtob(valueTarget), targetType );
   }
   else if(type == StateMachineBehaviorInstance::StateTransition::Rule::NumberType){
      object->addNumericTransition( fieldName, targetStateName, number, targetType );
   }
   else if(type == StateMachineBehaviorInstance::StateTransition::Rule::VectorType){
      object->addVectorTransition( fieldName, targetStateName, vector, targetType );
   }
}