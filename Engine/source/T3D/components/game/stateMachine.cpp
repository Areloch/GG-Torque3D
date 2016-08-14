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

#include "T3D/components/game/stateMachine.h"

StateMachine::StateMachine()
{
   mStateStartTime = -1;
   mStateTime = 0;

   mStartingState = "";

   mCurCreateState = NULL;
}

StateMachine::~StateMachine()
{
}

void StateMachine::loadStateMachineFile()
{
   if (!mXMLReader)
   {
      SimXMLDocument *xmlrdr = new SimXMLDocument();
      xmlrdr->registerObject();

      mXMLReader = xmlrdr;
   }

   bool hasStartState = false;
   U32 startingState = 0;

   if (!dStrIsEmpty(mStateMachineFile))
   {
      //use our xml reader to parse the file!
      SimXMLDocument *reader = mXMLReader.getObject();
      if (!reader->loadFile(mStateMachineFile))
         Con::errorf("Could not load state machine file: &s", mStateMachineFile);

      if (!reader->pushFirstChildElement("StateMachine"))
         return;

      //Add a dummy field for the stateTime, since we special-case handle that, but need to hold the slot
      //because stateTime will always exist in the SM.
      StateField stateTimeField;
      stateTimeField.name = "stateTime";
      mFields.push_back(stateTimeField);

      if (reader->pushFirstChildElement("Fields"))
      {
         S32 fieldsCount = 0;
         while (reader->pushChildElement(fieldsCount))
         {
            StateField newField;

            newField.name = reader->attribute("name");
            const char* defaultVal = reader->attribute("defaultValue");

            setField(&newField, defaultVal);

            mFields.push_back(newField);

            ++fieldsCount;
            reader->popElement();
         }

         reader->popElement();
      }

      if (reader->pushFirstChildElement("States"))
      {
         S32 statesCount = 0;
         while (reader->pushChildElement(statesCount))
         {
            State newState;
            newState.stateName = reader->attribute("name");
            newState.callbackName = reader->attribute("scriptFunction");

            String starting = reader->attribute("starting");
            if (starting.isNotEmpty())
            {
               startingState = statesCount;
               hasStartState = true;
            }

            mStates.push_back(newState);

            ++statesCount;
            reader->popElement();
         }

         reader->popElement();
      }

      if (reader->pushFirstChildElement("Transitions"))
      {
         S32 transitionsCount = 0;
         while (reader->pushChildElement(transitionsCount))
         {
            StateTransition newTransition;

            U32 ownerStateId = dAtoi(reader->attribute("ownerState"));
            U32 targetStateId = dAtoi(reader->attribute("stateTarget"));

            newTransition.mStateTarget = targetStateId;

            //
            U32 ruleCount = 0;
            while (reader->pushChildElement(ruleCount))
            {
               StateTransition::Condition newCondition;
               StateField newField;

               newCondition.targetFieldIdx = dAtoi(reader->attribute("fieldId"));
               const char* comparitor = reader->attribute("comparitor");

               newCondition.triggerComparitor = (StateTransition::Condition::ComparitorType)parseComparitor(comparitor);

               const char* value = reader->attribute("value");

               setField(&newField, value);

               newCondition.triggerValue = newField.data;

               newTransition.mTransitionRules.push_back(newCondition);

               ++ruleCount;
               reader->popElement();
            }
            //

            State* ownerState = &mStates[ownerStateId];

            ownerState->mTransitions.push_back(newTransition);

            ++transitionsCount;
            reader->popElement();
         }

         reader->popElement();
      }
   }

   if (hasStartState)
      setState(startingState);
   else
      setState(0);

   mStateStartTime = -1;
   mStateTime = 0;
}

void StateMachine::setField(StateField *newField, const char* fieldValue)
{
   S32 type = -1;

   Point3F vector;
   F32 number;

   //bail pre-emptively if it's nothing
   if (!fieldValue || !fieldValue[0])
      return;

   String fieldVal = String::ToLower(fieldValue);

   if (dSscanf(fieldVal, "%g %g %g", &vector.x, &vector.y, &vector.z) == 3)
   {
      newField->data.fieldType = Field::VectorType;
      newField->data.vectorVal = vector;
   }
   else
   {
      if (dSscanf(fieldVal, "%g", &number))
      {
         newField->data.fieldType = Field::NumberType;
         newField->data.numVal = number;
      }
      else
      {
         newField->data.fieldType = Field::StringType;
         newField->data.stringVal = fieldValue;
      }
   }

   //final check if it's a string: see if it's actually a boolean!
   if (newField->data.fieldType == Field::StringType)
   {
      if (!dStrcmp("false", fieldVal))
      {
         newField->data.fieldType = Field::BooleanType;
         newField->data.boolVal = false;
      }
      else if (!dStrcmp("true", fieldVal))
      {
         newField->data.fieldType = Field::BooleanType;
         newField->data.boolVal = true;
      }
   }
}

S32 StateMachine::parseComparitor(const char* comparitorName)
{
   S32 targetType = -1;

   if (!dStrcmp("GreaterThan", comparitorName))
      targetType = StateMachine::StateTransition::Condition::GreaterThan;
   else if (!dStrcmp("GreaterOrEqual", comparitorName))
      targetType = StateMachine::StateTransition::Condition::GreaterOrEqual;
   else if (!dStrcmp("LessThan", comparitorName))
      targetType = StateMachine::StateTransition::Condition::LessThan;
   else if (!dStrcmp("LessOrEqual", comparitorName))
      targetType = StateMachine::StateTransition::Condition::LessOrEqual;
   else if (!dStrcmp("Equals", comparitorName))
      targetType = StateMachine::StateTransition::Condition::Equals;
   else if (!dStrcmp("True", comparitorName))
      targetType = StateMachine::StateTransition::Condition::True;
   else if (!dStrcmp("False", comparitorName))
      targetType = StateMachine::StateTransition::Condition::False;
   else if (!dStrcmp("Negative", comparitorName))
      targetType = StateMachine::StateTransition::Condition::Negative;
   else if (!dStrcmp("Positive", comparitorName))
      targetType = StateMachine::StateTransition::Condition::Positive;
   else if (!dStrcmp("DoesNotEqual", comparitorName))
      targetType = StateMachine::StateTransition::Condition::DoesNotEqual;

   return targetType;
}

void StateMachine::update()
{
   //we always check if there's a timout transition, as that's the most generic transition possible.
   F32 curTime = Sim::getCurrentTime();

   if (mStateStartTime == -1)
      mStateStartTime = curTime;

   mStateTime = curTime - mStateStartTime;

   char buffer[64];
   dSprintf(buffer, sizeof(buffer), "%g", mStateTime);

   setField("stateTime", buffer);

   checkTransitions();
}

void StateMachine::checkTransitions()
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
   //now that we have the type, check our transitions!
   for (U32 t = 0; t < mCurrentState.mTransitions.size(); t++)
   {
      //found a transition looking for this variable, so do work
      //first, figure out what data type thie field is
      //S32 type = getVariableType(newValue);

      StateTransition trnsn = mCurrentState.mTransitions[t];

      bool fail = false;
      bool match = true;
      S32 ruleCount = mCurrentState.mTransitions[t].mTransitionRules.size();

      //Every rule needs to succeed in order to actually trigger the transiton
      for (U32 r = 0; r < ruleCount; r++)
      {
         StateTransition::Condition rl = mCurrentState.mTransitions[t].mTransitionRules[r];
         /*const char* fieldName = findFieldByIndex(mCurrentState.mTransitions[t].mTransitionRules[r].targetFieldIdx);
         if (!dStrcmp(fieldName, slotName))
         {
            //now, check the value with the comparitor and see if we do the transition.
            if (!passComparitorCheck(newValue, mCurrentState.mTransitions[t].mTransitionRules[r]))
            {
               fail = true;
               break;
            }
         }
         else
         {*/
            //check our owner object for if it has the rule's field, and if so, check that
            if (mOwnerObject)
            {
               if (!passComparitorCheck(mCurrentState.mTransitions[t].mTransitionRules[r]))
               {
                  fail = true;
                  break;
               }
            }

            //we have to match all conditions
            //match = false;
            //break;
        // }
      }

      //If we do have a transition rule for this field, and we didn't fail on the condition, go ahead and switch states
      if (/*match &&*/ !fail)
      {
         setState(mCurrentState.mTransitions[t].mStateTarget);

         return;
      }
   }
}

bool StateMachine::passComparitorCheck(StateTransition::Condition transitionRule)
{
   StateField* field = &mFields[transitionRule.targetFieldIdx];

   //if the field and rule don't match, bail
   if (field->data.fieldType != transitionRule.triggerValue.fieldType)
      return false;

   switch (transitionRule.triggerValue.fieldType)
   {
   case Field::VectorType:
      switch (transitionRule.triggerComparitor)
      {
      case StateTransition::Condition::Equals:
      case StateTransition::Condition::GreaterThan:
      case StateTransition::Condition::GreaterOrEqual:
      case StateTransition::Condition::LessThan:
      case StateTransition::Condition::LessOrEqual:
      case StateTransition::Condition::DoesNotEqual:
         //do
         break;
      default:
         return false;
      };
   case Field::StringType:
      switch (transitionRule.triggerComparitor)
      {
      case StateTransition::Condition::Equals:
         if (!dStrcmp(field->data.stringVal, transitionRule.triggerValue.stringVal))
            return true;
         else
            return false;
      case StateTransition::Condition::DoesNotEqual:
         if (dStrcmp(field->data.stringVal, transitionRule.triggerValue.stringVal))
            return true;
         else
            return false;
      default:
         return false;
      };
   case Field::BooleanType:
      switch (transitionRule.triggerComparitor)
      {
      case StateTransition::Condition::True:
         if (field->data.boolVal)
            return true;
         else
            return false;
      case StateTransition::Condition::False:
         if (!field->data.boolVal)
            return true;
         else
            return false;
      default:
         return false;
      };
   case Field::NumberType:
      switch (transitionRule.triggerComparitor)
      {
      case StateTransition::Condition::Equals:
         if (field->data.numVal == transitionRule.triggerValue.numVal)
            return true;
         else
            return false;
      case StateTransition::Condition::GreaterThan:
         if (field->data.numVal > transitionRule.triggerValue.numVal)
            return true;
         else
            return false;
      case StateTransition::Condition::GreaterOrEqual:
         if (field->data.numVal >= transitionRule.triggerValue.numVal)
            return true;
         else
            return false;
      case StateTransition::Condition::LessThan:
         if (field->data.numVal < transitionRule.triggerValue.numVal)
            return true;
         else
            return false;
      case StateTransition::Condition::LessOrEqual:
         if (field->data.numVal <= transitionRule.triggerValue.numVal)
            return true;
         else
            return false;
      case StateTransition::Condition::DoesNotEqual:
         if (field->data.numVal != transitionRule.triggerValue.numVal)
            return true;
         else
            return false;
      case StateTransition::Condition::Positive:
         if (field->data.numVal > 0)
            return true;
         else
            return false;
      case StateTransition::Condition::Negative:
         if (field->data.numVal < 0)
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

void StateMachine::setState(U32 stateId, bool clearFields)
{
   if (stateId >= 0 || mStates.size() > stateId)
   {
      mCurrentState = mStates[stateId];
      mStateStartTime = Sim::getCurrentTime();

      onStateChanged.trigger(this, stateId);
   }
}

const char* StateMachine::getStateByIndex(S32 index)
{
   if (index >= 0 && mStates.size() > index)
      return mStates[index].stateName;
   else
      return "";
}

StateMachine::State& StateMachine::getStateByName(const char* name)
{
   StringTableEntry stateName = StringTable->insert(name);

   for (U32 i = 0; i < mStates.size(); i++)
   {
      if (!dStrcmp(stateName, mStates[i].stateName))
         return mStates[i];
   }
}

S32 StateMachine::findFieldByName(const char* name)
{
   for (U32 i = 0; i < mFields.size(); i++)
   {
      if (!dStrcmp(mFields[i].name, name))
         return i;
   }

   return -1;
}

void StateMachine::setField(const char* fieldName, const char* value)
{
   S32 fieldIdx = findFieldByName(fieldName);

   if (fieldIdx != -1)
   {
      F32 number = 0;

      switch (mFields[fieldIdx].data.fieldType)
      {
         case Field::BooleanType:
            if (dIsalpha(value[0]))
            {
               mFields[fieldIdx].data.boolVal = dAtob(value);
            }
            else
            {
               number = dAtoi(value);
               mFields[fieldIdx].data.boolVal = number == 0 ? false : true;
            }
            break;
         case Field::NumberType:
            number = dAtoi(value);
            mFields[fieldIdx].data.numVal = number;
            break;
         default:
            mFields[fieldIdx].data.stringVal = value;
            break;
      }
   }
}

StringTableEntry StateMachine::findFieldByIndex(U32 index)
{
   if (index >= 0 && index < mFields.size())
   {
      return mFields[index].name;
   }

   return StringTable->EmptyString();
}