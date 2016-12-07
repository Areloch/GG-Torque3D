#pragma once
#ifndef _MATERIALPHYSICALPROFILE_H_
#define _MATERIALPHYSICALPROFILE_H_

#ifndef _GAMEBASE_H_
#include "T3D/gameBase/gameBase.h"
#endif
#ifndef __RESOURCE_H__
#include "core/resource.h"
#endif
#ifndef _TDICTIONARY_H_
#include "core/util/tDictionary.h"
#endif

class SFXTrack;
class ExplosionData;
class DecalData;
class ParticleEmitter;
class ParticleEmitterData;
//struct SceneGraphData;
//class MaterialSoundProfile;
//class MaterialPhysicsProfile;

//-JR
//container class that holds all the physical, non-rendering properties of the material
//we make use of NetObject only for the sake of properly being able to load the datablock properties, like explosions or decals.
//they need to transfer the datablock across to properly load on the client.
class materialPhysicalProfile : public GameBaseData
{
   typedef GameBaseData Parent;

   //protected:
public:
   //sound
   S32						footStepSoundId;
   S32						impactSoundId;
   S32						heavyImpactSoundId;

   SFXTrack*				mFootstepSound;
   SFXTrack*				mImpactSound;
   SFXTrack*				mHeavyImpactSound;
   F32						mDirectSoundOcclusion;       ///< Amount of volume occlusion on direct sounds.
   F32						mReverbSoundOcclusion;       ///< Amount of volume occlusion on reverb sounds.

                                                     //decals
   S32						decalId;
   S32						heavyDecalId;
   S32						footStepDecalId;

   DecalData*				mDecal;
   DecalData*				mHeavyDecal;
   DecalData*				mFootstepDecal;

   bool						mShowFootprints;            ///< If true, show footprints when walking on surface with this material.  Defaults to false.
   bool						mShowImpactDecals;				//do the impact decals happen on this surface?(useful for forcefields and whatnot

                                                      //impacts
   S32						explosionId;
   S32						heavyExplosionId;
   S32						footStepEffectId;

   ExplosionData*			mExplosion; // default, low-power one
   ExplosionData*			mHeavyExplosion;  //high damage dealt
   ParticleEmitterData*			mFootstepEffect;  //what effect spawns when we walk on this?
   bool						mShowDust;                  ///< If true, show dust emitters (footpuffs, hover trails, etc) when on surface with this material.  Defaults to false.
   bool						mShowImpact;

   //physical properties
   F32						maxHealth;				//used for destruction of the object based on it's material
   F32						restitutionRate;			//how fast does it disspipate energy from hits?(lower rate means that less rapid fire, or fewer heavy shots are needed
                                                //to destroy the object.
   F32						heavyDamagePercent;		//how much health lost on the object before we start using the 'heavyexplosion' effects?
   F32						penetrationResistance;	//how much energy is spent in an inch(2.5cm, or approx 0.025 meters, figuring on a 1 unit-to-1 meter scale)
                                                //if a projectile runs out of energy, it stops penetrating.(explosion effects are scaled up if this happens, due to
                                                //all the energy being expended by the bullet
   F32						minPenetrationAngle;		//what is the minimum angle required on the surface to penetrate and not deflect off?

   F32						mFriction;               ///< Friction coefficient when moving along surface.

   bool						collidable;				//do we run checks for actual collisions?
   bool						collideBlocks;			//if we have a collision, does this stop movement?

   ColorI					materialColor;			//the average color of the material

                                             /// Color to use for particle effects and such when located on this material.
                                             //ColorF mEffectColor[ NUM_EFFECT_COLOR_STAGES ];

                                             /// Footstep sound to play when walking on surface with this material.
                                             /// Numeric ID of footstep sound defined on player datablock (0 == soft,
                                             /// 1 == hard, 2 == metal, 3 == snow).
                                             /// Defaults to -1 which deactivates default sound.
                                             /// @see mFootstepSoundCustom
                                             //S32 mFootstepSoundId;
                                             //S32 mImpactSoundId;

                                             /// Sound effect to play when walking on surface with this material.
                                             /// If defined, overrides mFootstepSoundId.
                                             /// @see mFootstepSoundId
                                             //SFXTrack* mFootstepSoundCustom;
                                             //SFXTrack* mImpactSoundCustom;


public:
   materialPhysicalProfile();
   bool preload(bool server, String &errorStr);
   bool onAdd();
   void onRemove();
   static void initPersistFields();
   void packData(BitStream*);
   void unpackData(BitStream*);

   U32 index;						//a number we use to keep track of our particular property profile.

   DECLARE_CONOBJECT(materialPhysicalProfile); //to scriiiiiiiiiipt
};

//This is a floater class. It's added to the mission in the background, and basically acts like a global repository for the physical profiles
//This allows us to snag the physical profile info either in script or in the engine for whatever purposes we may need
//Either physics info, impact effects, etc.
class matPhysProfileManager : public SceneObject
{
   typedef SceneObject Parent;

protected:
   bool onAdd();
   void onRemove();
   void inspectPostApply() {};

public:
   matPhysProfileManager() { mNetFlags.set(Ghostable | ScopeAlways); };
   ~matPhysProfileManager() {};

   static void initPersistFields() {};

   U32  packUpdate(NetConnection *conn, U32 mask, BitStream *stream) { U32 retMask = Parent::packUpdate(conn, mask, stream); return retMask; };
   void unpackUpdate(NetConnection *conn, BitStream *stream) { Parent::unpackUpdate(conn, stream); };

   void addPhysicalProfile(materialPhysicalProfile *physProfile);

   //physical profile list. kept here so that we have 'global' access to a single list, that way
   //materials on their own only carry a index to point to the given profile.
   Vector<materialPhysicalProfile*> mPhysicalProfiles;

   S32 getNumMaterialProfiles() { return mPhysicalProfiles.size(); }
   String getMaterialProfile(S32 index) { if (index - 1 < 0 || index > mPhysicalProfiles.size()) return String::EmptyString; else return mPhysicalProfiles[index - 1]->getName(); }

   static matPhysProfileManager * getServerObject();

   // Declare Console Object.
   DECLARE_CONOBJECT(matPhysProfileManager);
};
//-JR

#endif _MATERIALPHYSICALPROFILE_H_
