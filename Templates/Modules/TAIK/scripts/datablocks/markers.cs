//-----------------------------------------------------------------------------
// Torque
// Copyright GarageGames, LLC 2011
//-----------------------------------------------------------------------------

datablock MissionMarkerData(WayPointMarker)
{
   category = "Misc";
   shapeFile = "data/FPSGameplay/art/shapes/octahedron.dts";
};

datablock MissionMarkerData(SpawnSphereMarker)
{
   category = "Misc";
   shapeFile = "data/FPSGameplay/art/shapes/octahedron.dts";
};

datablock MissionMarkerData(CameraBookmarkMarker)
{
   category = "Misc";
   shapeFile = "data/FPSGameplay/art/shapes/camera.dts";
};

datablock StaticShapeData(CoverPosMarker)
{
   category = "Misc";
   shapeFile = "data/TAIK/art/shapes/taik/covermarker.dts";
};

function CoverPosMarker::onAdd(%this,%obj)
{
   %obj.isCoverMarker = true; // Makes identifying cover markers easier.
}

datablock StaticShapeData(CoverPosMarkerVisible)
{
   category = "Misc";
   shapeFile = "data/TAIK/art/shapes/taik/covermarker_green.dts";
};

function CoverPosMarkerVisible::onAdd(%this,%obj)
{
   %obj.isCoverMarker = true; // Makes identifying cover markers easier.
}

function StaticShapeData::create(%block)
{
   switch$(%block)
   {
      case "CoverPosMarker":
         %obj = new StaticShape() {
            dataBlock = %block;
         };
         return(%obj);
         
      case "CoverPosMarkerVisible":
         %obj = new StaticShape() {
            dataBlock = %block;
         };
         return(%obj);

      case "SpawnSphereMarker":
         %obj = new SpawnSphere() {
            datablock = %block;
         };
         return(%obj);
   }
   return(-1);
}
