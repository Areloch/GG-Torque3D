datablock materialPhysicalProfile(DefaultMaterialProfile)
{
   directSoundOcclusion = 0.5;       
   reverbSoundOcclusion = 0.5;       

   //decals
   heavyDecal = PlayerFootprint;
   footstepDecal = PlayerFootprint;
   showFootprints = false;      //do we show footprints on this surface        
   showImpactDecals = true;	  //do we show bullet impact decals on this surface?		

   //impacts
   explosionEffect = "BulletDirtExplosion"; 
   footstepEffect = BulletDirtDustEmitter;  
   showDust = false;                //Do we show footstep dust effects
   showImpact = true;               //Do we show the projectile impact effects?

   //physical durability
   maxHealth = 1;		               //how much health the surface has. over this, and it risks being destroyed
   restitutionRate = 1;	            //how much projectiles 'bounce' off the surface if they don't penetrate
                                    //0 is all energy is lost, 1 is no energy lost
   heavyDamagePercent = 0.5;        //how much of the total health consistutes 'heavy' damage in a hit
   penetrationResistance = 0.1;     //0-1 resistance to penetration. 0 is it will always go through, 1 is never
   minimumPenetrationSpeed = 100;   //minimum speed for a projectile to penetrate the surface
   minPenetrationAngle = 80;        //if the angle is greater than this, we can penetrate
   friction = 1;                    //Friction of the surface if we have something sliding along it
   
   collidable = true;               //does anything hit this material?
   collisionBlocks = true;          //if we hit it, does the projectile still go through(this allows effects to happen
                                    //for a hit, without destroying the projectile
   
   color = "128 128 128";           //what color is this material, on average?
};

datablock materialPhysicalProfile(ConcreteMaterialProfile)
{
   footstepSound = concrete;  
   impactSound = concrete;  
   heavyImpactSound = concrete;
   directSoundOcclusion = 0.5;       
   reverbSoundOcclusion = 0.5;       

   //decals
   decal = "bulletHoleDecal";
   heavyDecal = PlayerFootprint;
   footstepDecal = ScorchRXDecal;
   showFootprints = false;            
   showImpactDecals = true;				

   //impacts
   explosionEffect = BulletDirtExplosion; 
   heavyExplosionEffect = RocketLauncherExplosion;  
   footstepEffect = "BulletDirtDustEmitter";  
   showDust = false;        
   showImpact = true;

   //physical durability
   maxHealth = 1;		
   restitutionRate = 1;	
   heavyDamagePercent = 0.5;
   penetrationResistance = 0.1;
   minPenetrationAngle = 80;
   friction = 1;   
   
   collidable = true;
   collisionBlocks = true;  
   
   color = "190 190 190";
};

datablock materialPhysicalProfile(WoodMaterialProfile)
{
   footstepSound = Bamboo;  
   impactSound = Bamboo;  
   heavyImpactSound = Bamboo;
   directSoundOcclusion = 0.5;       
   reverbSoundOcclusion = 0.5;       

   //decals
   decal = "bulletHoleDecal";
   heavyDecal = PlayerFootprint;
   footstepDecal = ScorchRXDecal;
   showFootprints = false;            
   showImpactDecals = true;				

   //impacts
   explosionEffect = BulletDirtExplosion; 
   heavyExplosionEffect = RocketLauncherExplosion;  
   footstepEffect = "BulletDirtDustEmitter";  
   showDust = false;        
   showImpact = true;

   //physical durability
   maxHealth = 1;		
   restitutionRate = 1;	
   heavyDamagePercent = 0.5;
   penetrationResistance = 0.1;
   minPenetrationAngle = 80;
   friction = 1;   
   
   collidable = true;
   collisionBlocks = true;
   
   color = "133 94 66"; 
};

datablock materialPhysicalProfile(testPhysMat : DefaultMaterialProfile)
{
   explosionEffect = "FragExplosion";
};
