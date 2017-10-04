//-----------------------------------------------------------------------------
// Tactical AI Kit
// Copyright (C) Bryce Carrington
//-----------------------------------------------------------------------------

datablock AIPathNodeData(PathNode)
{
   Scale = "2 2 2";
   shapeFile = "data/FPSGameplay/art/shapes/octahedron.dts";
   category="AI";
};

function AIPathNodeData::create(%block)
{
   %obj = new AIPathNode() {
      dataBlock = %block;
   };
   return(%obj);
}

function ProperlyRemoveNode(%obj)
{
  AIPaths.removeObjectSafe(%obj);
}
function DoDelayedNodeDelete(%obj)
{
  error("Delay delete " @ %obj);
}
  