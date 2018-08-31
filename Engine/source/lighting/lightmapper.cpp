#include "lightmapper.h"
#include "scene/sceneManager.h"
#include "ts/tsMaterialList.h"
#include "math/mathUtils.h"
#include "gfx/gfxDrawUtil.h"

Lightmapper * Lightmapper::smLightmapper = NULL;

Lightmapper::Lightmapper()
{
   mStaticObjectMask = StaticObjectType | StaticShapeObjectType;
}

Lightmapper::~Lightmapper()
{

}

void Lightmapper::processTick()
{
   if (!mRunning)
      return;

   U32 startTime = Platform::getRealMilliseconds();

   U32 maxRayLen = 1000;

   Point3F startPos = Point3F(0, 0, 10);

   //We need some stupid high number for a bake, so bake it a biggun
   while (mRayCount > 0)
   {
      RayInfo ri;
      ri.generateTexCoord = true;

      Point3F dir = MathUtils::randomDir(startPos, 1, 359) * maxRayLen;
      gClientSceneGraph->getContainer()->castRayRendered(startPos, dir, mStaticObjectMask, &ri);

      //GFXDrawUtil* draw = GFX->getDrawUtil();

      //draw->drawLine(startPos, dir * 1000, ColorI::WHITE);

      //If we actually hit something
      if (ri.object)
      {
         TSStatic* st = dynamic_cast<TSStatic*>(ri.object);

         if (st)
         {
            StaticLMObject* lmObj = getLMObject(st);

            if (lmObj && ri.material)
            {
               //Material* mat = dynamic_cast<Material*>(ri.material->getMaterial());
               //GFXTexHandle tex(mat->mDiffuseMapFilename[0], &GFXRenderTargetSRGBProfile, "GroundCover texture aspect ratio check");
               //GBitmap* texBit = tex.getBitmap();
               //LinearColorF sampl = texBit->sampleTexel(ri.texCoord.x, ri.texCoord.y);

               LinearColorF sampl = LinearColorF::WHITE;

               U32 uvWidth = (ri.texCoord.x * lmObj->mLightmap->getWidth());
               U32 uvHeight = (ri.texCoord.y * lmObj->mLightmap->getHeight());

               lmObj->mLightmap->setColor(uvWidth, uvHeight, sampl.toColorI());

               //Con::printf("Sampled point for lightmapping");
               //lmObj->obj->getLightmapTex().set(lmObj->mLightmap, &GFXStaticTextureSRGBProfile, false, "Lightmap");
            }
         }
      }

      mRayCount--;

      U32 checkTime = Platform::getRealMilliseconds();

      if (checkTime - startTime > mMaxTimePerTick)
         break;
   }

   if (mRayCount == 0)
   {
      //We finished. Timestamp and all that junk here
      mRunning = false;
      endTimeMS = Platform::getRealMilliseconds();

      U32 timeDif = endTimeMS - startTimeMS;

      Con::printf("Lightmapper - finished bake - 5 million rays, %i milliseconds bake time", timeDif);

      //Update our LM's
      /*for (U32 i = 0; i < mStaticObjects.size(); i++)
      {
         mStaticObjects[i].obj->getLightmapTex().set(mStaticObjects[i].mLightmap, &GFXTexturePersistentSRGBProfile, true, "Lightmap");
      }*/
   }
}

void Lightmapper::bakeLightmaps()
{
   mMaxTimePerTick = 10; //5ms per tick is max we'll spend on a given update
   mRayCount = 100000; //5mil
   startTimeMS = Platform::getRealMilliseconds();

   mStaticObjects.clear();

   Vector<SceneObject*> staticObjects;

   gClientSceneGraph->getContainer()->findObjectList(mStaticObjectMask, &staticObjects);

   for (U32 i = 0; i < staticObjects.size(); i++)
   {
      SceneObject* scob = staticObjects[i];
      TSStatic* tss = dynamic_cast<TSStatic*>(scob);
      if (tss)
      {
         StaticLMObject newLMObj;
         newLMObj.obj = tss;

         //Prep the bitmap for the lightmap itself
         U32 res = 256;//tss->getLightmapRes();
         tss->setLightMap(new GBitmap(res, res, false, GFXFormatR8G8B8A8));

         newLMObj.mLightmap = tss->getLightMap();

         newLMObj.mLightmap->fill(ColorI::BLACK);

         mStaticObjects.push_back(newLMObj);

         newLMObj.obj->getLightmapTex().set(newLMObj.mLightmap, &GFXStaticTextureSRGBProfile, false, "Lightmap");
      }
   }

   if (mStaticObjects.empty())
      return;

   //Prep the materials list
   for (U32 i = 0; i < mStaticObjects.size(); i++)
   {
      TSMaterialList* matList = mStaticObjects[i].obj->getShape()->materialList;

      for (U32 m = 0; m < matList->size(); m++)
      {
         BaseMatInstance* matInst = matList->getMaterialInst(m);

         if (matInst)
         {
            Material* matDef = dynamic_cast<Material*>(matInst->getMaterial());

            if (matDef)
            {
               mStaticObjects[i].mMaterials.push_back_unique(matDef);
            }
         }
      }
   }

   //And away we go!
   mRunning = true;

   /*U32 dim = 1024;

   GBitmap * bitmap = new GBitmap(dim, dim, false, GFXFormatR8G8B8A8);

   bitmap->fill(ColorI::BLACK);

   for (U32 i = 0; i < raycount; i++)
   {
      RayInfo ri;
      ri.generateTexCoord = true;

      Point3F dir = MathUtils::randomDir(Point3F::One, 1, 359);
      castRayRendered(Point3F::Zero, dir * 1000, &ri);

      if (ri.material)
      {
         //Material* mat = dynamic_cast<Material*>(ri.material->getMaterial());
         //GFXTexHandle tex(mat->mDiffuseMapFilename[0], &GFXRenderTargetSRGBProfile, "GroundCover texture aspect ratio check");
         //GBitmap* texBit = tex.getBitmap();
         //LinearColorF sampl = texBit->sampleTexel(ri.texCoord.x, ri.texCoord.y);

         LinearColorF sampl = LinearColorF::WHITE;

         U32 uvWidth = (ri.texCoord.x * bitmap->getWidth());
         U32 uvHeight = (ri.texCoord.y * bitmap->getHeight());

         bitmap->setColor(uvWidth, uvHeight, sampl.toColorI());
      }
   }

   //bitmap->fill(ColorI::BLUE);

   mLightmap.set(bitmap, &GFXStaticTextureSRGBProfile, true, "Lightmap");

   bool trmp = true;*/
}

Lightmapper::StaticLMObject* Lightmapper::getLMObject(TSStatic* obj)
{
   for (U32 i = 0; i < mStaticObjects.size(); i++)
   {
      if (obj->getId() == mStaticObjects[i].obj->getId())
         return &mStaticObjects[i];
   }

   return nullptr;
}

DefineEngineFunction(bakeLightmaps, void, (), ,
   "@brief Remove a tagged string from the Net String Table\n\n"

   "@param tag The tag associated with the string\n\n"

   "@see \\ref syntaxDataTypes under Tagged %Strings\n"
   "@see addTaggedString()\n"
   "@see getTaggedString()\n"
   "@ingroup Networking\n")
{
   Lightmapper::get()->bakeLightmaps();
}