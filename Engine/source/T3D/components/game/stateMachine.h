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

#ifndef STATE_MACHINE_H
#define STATE_MACHINE_H

#ifndef _SIMBASE_H_
#include "console/simBase.h"
#endif
#ifndef _OBJECTTYPES_H_
#include "T3D/objectTypes.h"
#endif
#ifndef _MMATH_H_
#include "math/mMath.h"
#endif
#ifndef _XMLDOC_H_
#include "console/SimXMLDocument.h"
#endif

class StateMachine
{
public:
   struct Field
   {
      bool 			 boolVal;
      float 		 numVal;
      Point3F 		 vectorVal;
      String 		 stringVal;

      enum Type
      {
         BooleanType = 0,
         NumberType,
         VectorType,
         StringType
      }fieldType;
   };

   struct StateField
   {
      StringTableEntry name;
      
      Field data;
      //Field defaultData;
   };

   struct UniqueReference
   {
      SimObject* referenceObj;
      const char* referenceVar;
      const char* uniqueName;
   };

   struct StateTransition
   {
      struct Condition
      {
         enum ComparitorType
         {
            Equals = 0,
            GreaterThan,
            LessThan,
            GreaterOrEqual,
            LessOrEqual,
            True,
            False,
            Positive,
            Negative,
            DoesNotEqual
         };

         U32 targetFieldIdx;

         Field triggerValue;

         ComparitorType triggerComparitor;

         //UniqueReference      *valUniqueRef;
      };

      StringTableEntry	mName;
      U32	mStateTarget;
      Vector<Condition>	mTransitionRules;
   };

   struct State 
   {
      Vector<StateTransition> mTransitions;

      StringTableEntry stateName;

      StringTableEntry callbackName;
   };

   StringTableEntry		mStateMachineFile;

   SimObject* mOwnerObject;

protected:
   Vector<State> mStates;

   Vector<StateField> mFields;

   Vector<UniqueReference> mUniqueReferences;

   State mCurrentState;

   F32 mStateStartTime;
   F32 mStateTime;

   StringTableEntry mStartingState;

   State *mCurCreateSuperState;
   State *mCurCreateState;

   SimObjectPtr<SimXMLDocument> mXMLReader;

public:
   StateMachine();
   virtual ~StateMachine();

   void update();

   void loadStateMachineFile();

   void setState(U32 stateId, bool clearFields = true);

   const char* getCurrentStateName() { return mCurrentState.stateName; }
   State& getCurrentState() {
      return mCurrentState;
   }

   S32 getStateCount() { return mStates.size(); }
   const char* getStateByIndex(S32 index);
   State& getStateByName(const char* name);

   void checkTransitions();

   bool passComparitorCheck(StateTransition::Condition transitionRule);

   S32 findFieldByName(const char* name);
   StringTableEntry findFieldByIndex(U32 index);

   void setField(const char* fieldName, const char* value);

   S32 getFieldsCount() { return mFields.size(); }

   void setField(StateField *newField, const char* fieldValue);

   StateField getField(U32 index)
   {
      if (index <= mFields.size())
         return mFields[index];
   }

   Signal< void(StateMachine*, S32 stateIdx) > onStateChanged;

private:
   S32 parseComparitor(const char* comparitorName);
};

#endif