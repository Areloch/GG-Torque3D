function WorldEditor::onClick( %this, %obj )
{
   Inspector.inspect( %obj );
   
    if (%obj.getDatablock().getName() $= "AIZone")
    {
     if ($zoneSetup != true)
     {
      error("You can activate the adjacency editor by setting $zoneSetup to true in the console.");
      $curSetupZone = 0;
     }
     else
     {
      if ($curSetupZone != 0)
      {
       %curZone = $curSetUpZone;
       if (%curZone != %obj) // If we selected a zone that is not the zone we're setting up, then set the zone we click to be
       {                     // adjacent to the one we're setting up.
        error("Setting zone " @ %obj.getName() @ " to be adjacent to " @ %curZone.getName());
        %curZone.adjacentZones = %obj.getName() SPC %curZone.adjacentZones;
        if (!$noMutualAdjacency)
        {
         %obj.adjacentZones = %curZone.getName() SPC %obj.adjacentZones;
         warn("  Mutual -- Setting zone " @ %curZone.getName() @ " to be adjacent to " @ %obj.getName());
        }
       }
       else
       {
        warn("Clicked curSetUpZone " @ %curZone @ ", resetting curSetUpZone");
        $curSetUpZone = 0;
        return;
       }
      }
      
      if (!$curSetupZone)
      {
       // We just selected a trigger, so set it to be the curSetUpZone
       $curSetupZone = %obj;
       error("Setting $curSetUpZone to be " @ %obj);
      }
     }
    }
    else
    {
     $curSetupzone = 0;
    }
}