#pragma once

#ifndef TWEEN_ENGINE_H
#define TWEEN_ENGINE_H

#include "platform/platform.h"
#include "console/consoleTypes.h"
#include "console/engineAPI.h"
#include "console/simObject.h"
#include "core\iTickable.h"
#include "math/mConstants.h"
#include "math/mMathFn.h"
#include "math/mEase.h"

class Tween : public SimObject, public ITickable
{
   typedef SimObject Parent;

   enum TweenState {
      Idle,
      Playing,
      Paused,
      PlayingReversed
   };

public:
   Tween();
   void SetValueByTime(F32 time);
   void SetTargetField(F32 value);
   void SetGlobalField(F32 value);
   void SetInitialValue();

   static bool setTargetProperty(void *obj, const char *index, const char *db);

   void Play();
   void Reverse();
   void Rewind();
   void Pause();

   //-------------------------------------------
   // SimObject
   //-------------------------------------------
   virtual bool onAdd();
   static void initPersistFields();

   //-------------------------------------------
   // ITickable
   //-------------------------------------------
   virtual void advanceTime(F32 timeDelta);
   virtual void interpolateTick(F32 delta) {};
   virtual void processTick() {};

   // Fields ------------------------------------
   F32 mDuration;
   F32 mCurrentTime;
   SimObject* mTarget;
   const char* mValueName;
   F32 mValueTarget;
   F32 mInitialValue;
   Ease::enumDirection mEaseDirection;
   Ease::enumType mEaseType;
   TweenState mState;

   DECLARE_CALLBACK(void, onFinished, ());
   DECLARE_CONOBJECT(Tween);
};

#endif // TWEEN_ENGINE_H