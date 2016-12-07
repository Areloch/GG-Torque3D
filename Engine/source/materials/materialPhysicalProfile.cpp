#include "materials/materialPhysicalProfile.h"
#include "materials/materialManager.h"
#include "console/typeValidators.h"
#include "console/consoleTypes.h"
#include "sfx/sfxTypes.h"
#include "T3D/decal/decalData.h"
#include "T3D/fx/explosion.h"
#include "T3D/fx/particleEmitter.h"
#include "T3D/levelInfo.h"
#include "core/stream/bitStream.h"

IMPLEMENT_CO_DATABLOCK_V1(materialPhysicalProfile);
IMPLEMENT_CO_NETOBJECT_V1(matPhysProfileManager);

materialPhysicalProfile::materialPhysicalProfile()
{
   //sound
   footStepSoundId = 0;
   impactSoundId = 0;
   heavyImpactSoundId = 0;

   mFootstepSound = NULL;
   mImpactSound = NULL;
   mHeavyImpactSound = NULL;
   mDirectSoundOcclusion = 0.5;
   mReverbSoundOcclusion = 0.5;

   //decals
   decalId = 0;
   heavyDecalId = 0;
   footStepDecalId = 0;

   mDecal = NULL;
   mHeavyDecal = NULL;
   mFootstepDecal = NULL;
   mShowFootprints = false;
   mShowImpactDecals = true;

   //impacts
   explosionId = 0;
   heavyExplosionId = 0;
   footStepEffectId = 0;

   mExplosion = NULL;
   mHeavyExplosion = NULL;
   mFootstepEffect = NULL;
   mShowDust = false;
   mShowImpact = true;

   //physical durability
   maxHealth = 1;
   restitutionRate = 1;
   heavyDamagePercent = 0.5f;
   penetrationResistance = 0.1f;
   minPenetrationAngle = 80.f;
   mFriction = 1;

   collidable = true;
   collideBlocks = true;

   materialColor = ColorI(128, 128, 128);
}

bool materialPhysicalProfile::onAdd()
{
   if (!Parent::onAdd())
      return false;

   //sounds
   /*if (!mDecal && decalId != 0)
   {
   if (!Sim::findObject( SimObjectId( decalId ), mDecal ))
   Con::errorf( ConsoleLogEntry::General, "PhysicalProfile::onAdd: Invalid packet, bad datablockId(particle emitter): 0x%x", decalId);
   }
   if (!mHeavyDecal && heavyDecalId != 0)
   {
   if (!Sim::findObject( SimObjectId( heavyDecalId ), mHeavyDecal ))
   Con::errorf( ConsoleLogEntry::General, "PhysicalProfile::onAdd: Invalid packet, bad datablockId(particle emitter): 0x%x", heavyDecalId);
   }
   if (!mFootstepDecal && footStepDecalId != 0)
   {
   if (!Sim::findObject( SimObjectId( footStepDecalId ), mFootstepDecal ))
   Con::errorf( ConsoleLogEntry::General, "PhysicalProfile::onAdd: Invalid packet, bad datablockId(particle emitter): 0x%x", footStepDecalId);
   }*/

   //For the level to work, there has to be a LevelInfo, so we're almost 100% guarenteed for it to exist, if not, we just don't add anything
   matPhysProfileManager* mngr = matPhysProfileManager::getServerObject();

   if (mngr && mngr->isProperlyAdded())
   {
      index = mngr->mPhysicalProfiles.size() + 1; //increment our current list and assign as our index
      mngr->addPhysicalProfile(this);
   }
   //if it doesn't exist yet, we should add it! it has all sorts of important info
   else
   {
      mngr = new matPhysProfileManager();
      mngr->registerObject();
      mngr->addToScene();

      index = mngr->mPhysicalProfiles.size() + 1; //increment our current list and assign as our index
      mngr->addPhysicalProfile(this);
   }

   //index = MATMGR->instance()->mPhysicalProfiles.size() + 1; //increment our current list and assign as our index
   //MATMGR->instance()->addPhysicalProfile(this);
   return true;
}

bool materialPhysicalProfile::preload(bool server, String &errorStr)
{
   if (Parent::preload(server, errorStr) == false)
      return false;

   if (!server)
   {
      //get the datablocks for our effects and sounds
      //explosions
      if (!mExplosion && explosionId != 0) {
         if (!Sim::findObject(SimObjectId(explosionId), mExplosion))
            Con::errorf(ConsoleLogEntry::General, "PhysicalProfile::onAdd: Invalid packet, bad datablockId(particle emitter): 0x%x", explosionId);
      }
      if (!mHeavyExplosion && heavyExplosionId != 0) {
         if (!Sim::findObject(SimObjectId(heavyExplosionId), mHeavyExplosion))
            Con::errorf(ConsoleLogEntry::General, "PhysicalProfile::onAdd: Invalid packet, bad datablockId(particle emitter): 0x%x", heavyExplosionId);
      }
      if (!mFootstepEffect && footStepEffectId != 0) {
         if (!Sim::findObject(SimObjectId(footStepEffectId), mFootstepEffect))
            Con::errorf(ConsoleLogEntry::General, "PhysicalProfile::onAdd: Invalid packet, bad datablockId(particle emitter): 0x%x", footStepEffectId);
      }

      //decals
      if (!mDecal && decalId != 0) {
         if (!Sim::findObject(SimObjectId(decalId), mDecal))
            Con::errorf(ConsoleLogEntry::General, "PhysicalProfile::onAdd: Invalid packet, bad datablockId(particle emitter): 0x%x", decalId);
      }
      if (!mHeavyDecal && heavyDecalId != 0) {
         if (!Sim::findObject(SimObjectId(heavyDecalId), mHeavyDecal))
            Con::errorf(ConsoleLogEntry::General, "PhysicalProfile::onAdd: Invalid packet, bad datablockId(particle emitter): 0x%x", heavyDecalId);
      }
      if (!mFootstepDecal && footStepDecalId != 0) {
         if (!Sim::findObject(SimObjectId(footStepDecalId), mFootstepDecal))
            Con::errorf(ConsoleLogEntry::General, "PhysicalProfile::onAdd: Invalid packet, bad datablockId(particle emitter): 0x%x", footStepDecalId);
      }

      /*String errorStr;
      if( !sfxResolve( &sound, errorStr ) )
      Con::errorf(ConsoleLogEntry::General, "ProjectileData::preload: Invalid packet: %s", errorStr.c_str());*/
   }
   /*else
   {
   //first, check if we have a physical material manager
   matPhysProfileManager* mngr = matPhysProfileManager::getServerObject();

   if(!mngr )
   {
   matPhysProfileManager *MGR = new matPhysProfileManager();
   MGR->registerObject();
   MGR->addToScene();
   }
   }*/

   return true;
}

void materialPhysicalProfile::onRemove()
{
   mFootstepSound = NULL;
   mImpactSound = NULL;
   mHeavyImpactSound = NULL;

   //decals
   mDecal = NULL;
   mHeavyDecal = NULL;
   mFootstepDecal = NULL;

   //impacts
   mExplosion = NULL;
   mHeavyExplosion = NULL;
   mFootstepEffect = NULL;

   Parent::onRemove();
}

void materialPhysicalProfile::initPersistFields()
{
   addGroup("Sounds");
   addField("footstepSound", TypeSFXTrackName, Offset(mFootstepSound, materialPhysicalProfile));
   addField("impactSound", TypeSFXTrackName, Offset(mImpactSound, materialPhysicalProfile));
   addField("heavyImpactSound", TypeSFXTrackName, Offset(mHeavyImpactSound, materialPhysicalProfile));
   addField("directSoundOcclusion", TypeF32, Offset(mDirectSoundOcclusion, materialPhysicalProfile));
   addField("reverbSoundOcclusion", TypeF32, Offset(mReverbSoundOcclusion, materialPhysicalProfile));
   endGroup("Sounds");

   addGroup("Decals");
   addField("decal", TYPEID< DecalData >(), Offset(mDecal, materialPhysicalProfile));
   addField("heavyDecal", TYPEID< DecalData >(), Offset(mHeavyDecal, materialPhysicalProfile));
   addField("footstepDecal", TYPEID< DecalData >(), Offset(mFootstepDecal, materialPhysicalProfile));
   addField("showFootprints", TypeBool, Offset(mShowFootprints, materialPhysicalProfile));
   addField("showImpactDecals", TypeBool, Offset(mShowImpactDecals, materialPhysicalProfile));
   endGroup("Decals");

   addGroup("Impacts");
   addField("explosionEffect", TYPEID< ExplosionData >(), Offset(mExplosion, materialPhysicalProfile));
   addField("heavyExplosionEffect", TYPEID< ExplosionData >(), Offset(mHeavyExplosion, materialPhysicalProfile));
   addField("footstepEffect", TYPEID< ParticleEmitterData >(), Offset(mFootstepEffect, materialPhysicalProfile));
   addField("showDust", TypeBool, Offset(mShowDust, materialPhysicalProfile));
   addField("showImpact", TypeBool, Offset(mShowImpact, materialPhysicalProfile));
   endGroup("Impacts");

   addGroup("Physical");
   addField("maxHealth", TypeF32, Offset(maxHealth, materialPhysicalProfile));
   addField("restitutionRate", TypeF32, Offset(restitutionRate, materialPhysicalProfile));
   addField("heavyDamagePercent", TypeF32, Offset(heavyDamagePercent, materialPhysicalProfile));
   addField("penetrationResistance", TypeF32, Offset(penetrationResistance, materialPhysicalProfile));
   addField("minPenetrationAngle", TypeF32, Offset(minPenetrationAngle, materialPhysicalProfile));
   addField("friction", TypeF32, Offset(mFriction, materialPhysicalProfile));
   addField("collidable", TypeBool, Offset(collidable, materialPhysicalProfile));
   addField("collisionBlocks", TypeBool, Offset(collideBlocks, materialPhysicalProfile));
   endGroup("Physical");

   addGroup("misc");
   addField("color", TypeColorI, Offset(materialColor, materialPhysicalProfile));
   endGroup("misc");

   Parent::initPersistFields();
}

void materialPhysicalProfile::packData(BitStream* stream)
{
   Parent::packData(stream);

   //explosions
   if (stream->writeFlag(mExplosion != NULL))
      stream->writeRangedU32(mExplosion->getId(), DataBlockObjectIdFirst,
         DataBlockObjectIdLast);
   if (stream->writeFlag(mHeavyExplosion != NULL))
      stream->writeRangedU32(mHeavyExplosion->getId(), DataBlockObjectIdFirst,
         DataBlockObjectIdLast);
   if (stream->writeFlag(mFootstepEffect != NULL))
      stream->writeRangedU32(mFootstepEffect->getId(), DataBlockObjectIdFirst,
         DataBlockObjectIdLast);

   //decals
   if (stream->writeFlag(mDecal != NULL))
      stream->writeRangedU32(mDecal->getId(), DataBlockObjectIdFirst,
         DataBlockObjectIdLast);
   if (stream->writeFlag(mHeavyDecal != NULL))
      stream->writeRangedU32(mHeavyDecal->getId(), DataBlockObjectIdFirst,
         DataBlockObjectIdLast);
   if (stream->writeFlag(mFootstepDecal != NULL))
      stream->writeRangedU32(mFootstepDecal->getId(), DataBlockObjectIdFirst,
         DataBlockObjectIdLast);
}

void materialPhysicalProfile::unpackData(BitStream* stream)
{
   Parent::unpackData(stream);

   //explosions
   if (stream->readFlag())
      explosionId = stream->readRangedU32(DataBlockObjectIdFirst, DataBlockObjectIdLast);
   if (stream->readFlag())
      heavyExplosionId = stream->readRangedU32(DataBlockObjectIdFirst, DataBlockObjectIdLast);
   if (stream->readFlag())
      footStepEffectId = stream->readRangedU32(DataBlockObjectIdFirst, DataBlockObjectIdLast);

   //decals
   if (stream->readFlag())
      decalId = stream->readRangedU32(DataBlockObjectIdFirst, DataBlockObjectIdLast);
   if (stream->readFlag())
      heavyDecalId = stream->readRangedU32(DataBlockObjectIdFirst, DataBlockObjectIdLast);
   if (stream->readFlag())
      footStepDecalId = stream->readRangedU32(DataBlockObjectIdFirst, DataBlockObjectIdLast);
}

//==========================================================================================
//The Manager
bool matPhysProfileManager::onAdd()
{
   if (!Parent::onAdd())
      return false;

   mObjBox.minExtents.set(-0.5f, -0.5f, -0.5f);
   mObjBox.maxExtents.set(0.5f, 0.5f, 0.5f);
   resetWorldBox();

   if (!isClientObject())
      assignName("matPhysProfileManager");

   addToScene();
   return true;
}

void matPhysProfileManager::onRemove()
{
   removeFromScene();
   Parent::onRemove();
}

//physical properties for materials
void matPhysProfileManager::addPhysicalProfile(materialPhysicalProfile *physProfile)
{
   mPhysicalProfiles.push_back(physProfile);
}

DefineConsoleFunction(getPhysicalProfileCount, S32, (), ,
   "getPhysicalProfileCount( int ) Returns the number of stored physical profiles for materials")
{
   matPhysProfileManager *MGR = matPhysProfileManager::getServerObject();
   return MGR->getNumMaterialProfiles();
}

DefineConsoleFunction(getPhysicalProfile, String, (S32 materialID), (0),
   "getPhysicalProfileCount( int ) Returns the name of stored physical profile using an index")
{
   matPhysProfileManager *MGR = matPhysProfileManager::getServerObject();
   return Con::getReturnBuffer(MGR->getMaterialProfile(materialID));
}

//This is technically cheating really, really hard, but I'm not sure of a better way to globally access the level info
matPhysProfileManager * matPhysProfileManager::getServerObject()
{
   SimSet * scopeAlwaysSet = Sim::getGhostAlwaysSet();
   for (SimSet::iterator itr = scopeAlwaysSet->begin(); itr != scopeAlwaysSet->end(); itr++)
   {
      matPhysProfileManager * MGR = dynamic_cast<matPhysProfileManager*>(*itr);
      if (MGR)
      {
         AssertFatal(MGR->isServerObject(), "LevelInfo::getServerObject: found client object in ghost always set!");
         return(MGR);
      }
   }

   return NULL;

   //if we found nothing, then that is UNACCEPTABLE >:(
   //create a new one and add it, we'll just roll with default values
   /*if(!isServerObject())
   {
   matPhysProfileManager *MGR = new matPhysProfileManager();
   MGR->registerObject();
   MGR->addToScene();

   return (MGR);
   }*/
}