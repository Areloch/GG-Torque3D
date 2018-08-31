#pragma once

#include "console/engineAPI.h"
#include "T3D/tsStatic.h"
#include "materials/materialDefinition.h"

#ifndef _CONSOLEINTERNAL_H_
#include "console/consoleInternal.h"
#endif

#ifndef _ITICKABLE_H_
#include "core/iTickable.h"
#endif

#include "gfx/bitmap/gBitmap.h"

class Lightmapper : public virtual ITickable
{
   struct StaticLMObject
   {
      TSStatic* obj;
      GBitmap* mLightmap;
      Vector<Material*> mMaterials;
   };

private:
   static Lightmapper * smLightmapper; ///< Global GFXDevice 

   Vector<StaticLMObject> mStaticObjects;

   bool mRunning;
   U32 startTimeMS;
   U32 endTimeMS;

   U32 mMaxTimePerTick;

   U32 mRayCount;

   U32 mStaticObjectMask;

public:
   Lightmapper();
   ~Lightmapper();

   void bakeLightmaps();

   virtual void interpolateTick(F32 delta) {}
   virtual void processTick();
   virtual void advanceTime(F32 timeDelta) {}

   StaticLMObject* getLMObject(TSStatic* obj);

   static Lightmapper *get() 
   { 
      if (smLightmapper == NULL)
         smLightmapper = new Lightmapper();

      return smLightmapper; 
   }
};