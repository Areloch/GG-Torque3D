function ProjectileComponent::onAdd(%this)
{
   %this.addComponentField(velocity, "How fast the projectile moves in meters per second", "float", "0", "");
   %this.addComponentField(lifeTime, "How long in seconds the projectile will exist before being deleted.", "float", "60", "");
   
   %this.addComponentField(collisionFilter, "What typemasks the projectile will collide with", "typeMask", "0", "");
   
   %this.collisionMask = $TypeMasks::EntityObjectType | 
                           $TypeMasks::StaticObjectType |
                           $TypeMasks::StaticShapeObjectType |
                           $TypeMasks::DynamicShapeObjectType |
                           $TypeMasks::InteriorObjectType |
                           $TypeMasks::TerrainObjectType |
                           $TypeMasks::VehicleObjectType;
}

function ProjectileComponent::onRemove(%this)
{

}

function ProjectileComponent::Update(%this)
{
   if(%this.velocity == 0)
      return;
      
   %forVec = %this.owner.getForwardVector();
   
   %tickRate = 0.032;
   
   %startPos = %this.owner.position;
   %newPos = VectorScale(%forVec, %this.velocity * %tickRate);
   
   %newPos = VectorAdd(%newPos, %startPos);
   
   %hit = containerRayCast(%startPos, %newPos, %this.collisionMask, %this.owner);
   
   if(%hit == 0)
   {
      %this.owner.position = %newPos;
   }
   else
   {
      //we have a hit!
      %this.velocity = 0;
      %this.owner.position = getWords(%hit, 1, 3);
      %this.enabled = false;
   }
}

function ProjectileComponent::simulate(%this)
{
   %velocity = %this.getVelocity();
   
   if (%this.trigger !$= "") //are we in a zone?
	{
		if (%this.trigger.bEnableWind)
		   %v = VectorSub(%velocity, TSSTD_Math::VectorDivF(TSSTD_Math::VectorMulF(EnvironmentInfo.Wind, 1.47), 2));
		else
			%v = %velocity;
		
		if (%this.trigger.Temperature != 0)
		{
			%p = ((70.748663101605 * EnvironmentInfo.Pressure) / ((%this.trigger.Temperature + 459.6) * 1715.7)) * 9.81;
			%mach = VectorLen(%v) / (mSqrt(1.4005 * 1715.7 * (%this.trigger.Temperature + 459.6)));
		}
		else
		{
			%p = Env.Density;
			%mach = VectorLen(%v) /EnvironmentInfo.SpeedOfSound;
		}
	}
	else
	{
		%v = VectorSub(%velocity, TSSTD_Math::VectorDivF(TSSTD_Math::VectorMulF(EnvironmentInfo.Wind, 1.47), 2));
		%p = EnvironmentInfo.Density;
		%mach = VectorLen(%v) / EnvironmentInfo.SpeedOfSound;
	}

	if (%this.dataBlock.DragFunction $= "G1")
		%cd = %this.DragG1(%mach);
	else if (%this.dataBlock.DragFunction $= "G7")
		%cd = %this.DragG7(%mach);

	%AD = -1 *%cd * %v / (%this.dataBlock.ballisticCoefficient * 2);

   %velocity = TSSTD_Math::VectorAddF(%velocity, %AD * $DeltaModifier); // Air Resistance
   setWord(%velocity, 2, (getWord(%velocity, 2) - 9.81 * $DeltaModifier)); // Gravity	
}


function ProjectileComponent::DragG7(%mach)
{
	if (%mach >= 5.00)
		return 0.1618;
	else if (%mach >= 4.80)
		return 0.1672;
	else if (%mach >= 4.60)
		return 0.1730;
	else if (%mach >= 4.40)
		return 0.1793;
	else if (%mach >= 4.20)
		return 0.1861;
	else if (%mach >= 4.00)
		return 0.1935;
	else if (%mach >= 3.90)
		return 0.1975;
	else if (%mach >= 3.80)
		return 0.2017;
	else if (%mach >= 3.70)
		return 0.2060;
	else if (%mach >= 3.60)
		return 0.2106;
	else if (%mach >= 3.50)
		return 0.2154;
	else if (%mach >= 3.40)
		return 0.2205;
	else if (%mach >= 3.30)
		return 0.2258;
	else if (%mach >= 3.20)
		return 0.2313;
	else if (%mach >= 3.10)
		return 0.2368;
	else if (%mach >= 3.00)
		return 0.2424;
	else if (%mach >= 2.95)
		return 0.2451;
	else if (%mach >= 2.90)
		return 0.2479;
	else if (%mach >= 2.85)
		return 0.2506;
	else if (%mach >= 2.80)
		return 0.2533;
	else if (%mach >= 2.75)
		return 0.2561;
	else if (%mach >= 2.70)
		return 0.2588;
	else if (%mach >= 2.65)
		return 0.2615;
	else if (%mach >= 2.60)
		return 0.2643;
	else if (%mach >= 2.55)
		return 0.2670;
	else if (%mach >= 2.50)
		return 0.2697;
	else if (%mach >= 2.45)
		return 0.2725;
	else if (%mach >= 2.40)
		return 0.2752;
	else if (%mach >= 2.35)
		return 0.2779;
	else if (%mach >= 2.30)
		return 0.2807;
	else if (%mach >= 2.25)
		return 0.2835;
	else if (%mach >= 2.20)
		return 0.2864;
	else if (%mach >= 2.15)
		return 0.2892;
	else if (%mach >= 2.10)
		return 0.2922;
	else if (%mach >= 2.05)
		return 0.2951;
	else if (%mach >= 2.00)
		return 0.2980;
	else if (%mach >= 1.95)
		return 0.3010;
	else if (%mach >= 1.90)
		return 0.3042;
	else if (%mach >= 1.85)
		return 0.3078;
	else if (%mach >= 1.80)
		return 0.3117;
	else if (%mach >= 1.75)
		return 0.3160;
	else if (%mach >= 1.70)
		return 0.3209;
	else if (%mach >= 1.65)
		return 0.3260;
	else if (%mach >= 1.60)
		return 0.3315;
	else if (%mach >= 1.55)
		return 0.3376;
	else if (%mach >= 1.50)
		return 0.3440;
	else if (%mach >= 1.40)
		return 0.3580;
	else if (%mach >= 1.35)
		return 0.3657;
	else if (%mach >= 1.30)
		return 0.3732;
	else if (%mach >= 1.25)
		return 0.3810;
	else if (%mach >= 1.20)
		return 0.3884;
	else if (%mach >= 1.15)
		return 0.3955;
	else if (%mach >= 1.125)
		return 0.3987;
	else if (%mach >= 1.10)
		return 0.4014;
	else if (%mach >= 1.075)
		return 0.4034;
	else if (%mach >= 1.05)
		return 0.4043;
	else if (%mach >= 1.025)
		return 0.4015;
	else if (%mach >= 1.0)
		return 0.3803;
	else if (%mach >= 0.975)
		return 0.2993;
	else if (%mach >= 0.95)
		return 0.2054;
	else if (%mach >= 0.925)
		return 0.1660;
	else if (%mach >= 0.90)
		return 0.1464;
	else if (%mach >= 0.875)
		return 0.1368;
	else if (%mach >= 0.85)
		return 0.1306;
	else if (%mach >= 0.825)
		return 0.1266;
	else if (%mach >= 0.80)
		return 0.1242;
	else if (%mach >= 0.775)
		return 0.1226;
	else if (%mach >= 0.75)
		return 0.1215;
	else if (%mach >= 0.725)
		return 0.1207;
	else if (%mach >= 0.70)
		return 0.1202;
	else if (%mach >= 0.65)
		return 0.1197;
	else if (%mach >= 0.60)
		return 0.1194;
	else if (%mach >= 0.55)
		return 0.1193;
	else if (%mach >= 0.50)
		return 0.1194;
	else if (%mach >= 0.45)
		return 0.1193;
	else if (%mach >= 0.40)
		return 0.1193;
	else if (%mach >= 0.35)
		return 0.1194;
	else if (%mach >= 0.30)
		return 0.1194;
	else if (%mach >= 0.25)
		return 0.1194;
	else if (%mach >= 0.20)
		return 0.1193;
	else if (%mach >= 0.15)
		return 0.1194;
	else if (%mach <= 0.10)
		return 0.1196;
	else if (%mach <= 0.05)
		return 0.1197;
	else
		return 0.1198;
}

//-----------------------------------------------------------------------------
// G1 - G1 drag function support.  Move to C++ later.
//-----------------------------------------------------------------------------

function ProjectileComponent::DragG1(%mach)
{
	if (%mach >= 5.00)
		return 0.4988;
	else if (%mach >= 4.80)
		return 0.4990;
	else if (%mach >= 4.60)
		return 0.4992;
	else if (%mach >= 4.40)
		return 0.4995;
	else if (%mach >= 4.20)
		return 0.4998;
	else if (%mach >= 4.00)
		return 0.5006;
	else if (%mach >= 3.90)
		return 0.5010;
	else if (%mach >= 3.80)
		return 0.5016;
	else if (%mach >= 3.70)
		return 0.5022;
	else if (%mach >= 3.60)
		return 0.5030;
	else if (%mach >= 3.50)
		return 0.5040;
	else if (%mach >= 3.40)
		return 0.5054;
	else if (%mach >= 3.30)
		return 0.5067;
	else if (%mach >= 3.20)
		return 0.5084;
	else if (%mach >= 3.10)
		return 0.5105;
	else if (%mach >= 3.00)
		return 0.5133;
	else if (%mach >= 2.90)
		return 0.5168;
	else if (%mach >= 2.80)
		return 0.5211;
	else if (%mach >= 2.70)
		return 0.5264;
	else if (%mach >= 2.60)
		return 0.5325;
	else if (%mach >= 2.50)
		return 0.5397;
	else if (%mach >= 2.45)
		return 0.5438;
	else if (%mach >= 2.40)
		return 0.5481;
	else if (%mach >= 2.35)
		return 0.5527;
	else if (%mach >= 2.30)
		return 0.5577;
	else if (%mach >= 2.25)
		return 0.5630;
	else if (%mach >= 2.20)
		return 0.5685;
	else if (%mach >= 2.15)
		return 0.5743;
	else if (%mach >= 2.10)
		return 0.5804;
	else if (%mach >= 2.05)
		return 0.5867;
	else if (%mach >= 2.00)
		return 0.5934;
	else if (%mach >= 1.95)
		return 0.6003;
	else if (%mach >= 1.90)
		return 0.6072;
	else if (%mach >= 1.85)
		return 0.6141;
	else if (%mach >= 1.80)
		return 0.6210;
	else if (%mach >= 1.75)
		return 0.6280;
	else if (%mach >= 1.70)
		return 0.6347;
	else if (%mach >= 1.65)
		return 0.6413;
	else if (%mach >= 1.60)
		return 0.6474;
	else if (%mach >= 1.55)
		return 0.6528;
	else if (%mach >= 1.50)
		return 0.6573;
	else if (%mach >= 1.45)
		return 0.6607;
	else if (%mach >= 1.40)
		return 0.6625;
	else if (%mach >= 1.35)
		return 0.6621;
	else if (%mach >= 1.30)
		return 0.6589;
	else if (%mach >= 1.25)
		return 0.6518;
	else if (%mach >= 1.20)
		return 0.6393;
	else if (%mach >= 1.15)
		return 0.6191;
	else if (%mach >= 1.125)
		return 0.6053;
	else if (%mach >= 1.10)
		return 0.5883;
	else if (%mach >= 1.075)
		return 0.5677;
	else if (%mach >= 1.05)
		return 0.5427;
	else if (%mach >= 1.025)
		return 0.5136;
	else if (%mach >= 1.0)
		return 0.4805;
	else if (%mach >= 0.975)
		return 0.4448;
	else if (%mach >= 0.95)
		return 0.4084;
	else if (%mach >= 0.925)
		return 0.3734;
	else if (%mach >= 0.90)
		return 0.3415;
	else if (%mach >= 0.875)
		return 0.3136;
	else if (%mach >= 0.85)
		return 0.2901;
	else if (%mach >= 0.825)
		return 0.2706;
	else if (%mach >= 0.80)
		return 0.2546;
	else if (%mach >= 0.775)
		return 0.2417;
	else if (%mach >= 0.75)
		return 0.2313;
	else if (%mach >= 0.725)
		return 0.2230;
	else if (%mach >= 0.70)
		return 0.2165;
	else if (%mach >= 0.60)
		return 0.2034;
	else if (%mach >= 0.55)
		return 0.2020;
	else if (%mach >= 0.50)
		return 0.2032;
	else if (%mach >= 0.45)
		return 0.2061;
	else if (%mach >= 0.40)
		return 0.2104;
	else if (%mach >= 0.35)
		return 0.2155;
	else if (%mach >= 0.30)
		return 0.2214;
	else if (%mach >= 0.25)
		return 0.2278;
	else if (%mach >= 0.20)
		return 0.2344;
	else if (%mach >= 0.15)
		return 0.2413;
	else if (%mach >= 0.10)
		return 0.2487;
	else if (%mach >= 0.05)
		return 0.2558;
	else
		return 0.2629;
}