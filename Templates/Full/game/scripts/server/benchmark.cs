//-----------------------------------------------------------------------------
// Copyright (c) 2012 GarageGames, LLC
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.
//-----------------------------------------------------------------------------

$BenchDisable = false; //enable/disable benchmarking server side

function serverCmdRunBenchMark(%client,%runId)
{
   if($BenchDisable == true)
   {
      commandToClient(%client,'BenchmarkLogMessage',"Benchmarking is disabled");
      return;
   }
   if (!isObject(%client.pathCamera))
   {
      if(!isObject(BenchmarkPathCamData))
      {
         %msg = "Benchmark could not find BenchmarkPathCamData datablock";
         //let client know
         commandToClient(%client,'BenchmarkLogMessage',%msg);
         return;
      }
      %client.pathCamera = spawnObject(PathCamera, BenchmarkPathCamData);
      MissionCleanup.add(%client.pathCamera);
      %client.pathCamera.scopeToClient(%client);
   }

   %camera = %client.pathCamera;
   %camera.client = %client;

   %benchmarkSimGroup = "MissionGroup/Benchmark";
   if(!isObject(%benchmarkSimGroup))
   {
      %msg = "Benchmark requires a Benchmark simgroup within MissionGroup";
      //let client know
      commandToClient(%client,'BenchmarkLogMessage',%msg);
      return; 
   }
      
   %control = %client.getControlObject();
   if (%control != %camera)
   {
      %client.prevControl = %control;
      %camera.reset();
      %camera.path = %benchmarkSimGroup @ "/BenchmarkPath" @ %runId;
      if(!isObject(%camera.path))
      {
         %msg = "Benchmark could not find a valid Path";
         //let client know
         commandToClient(%client,'BenchmarkLogMessage',%msg);
         return;
      }
      %count = %camera.path.getCount();
      for (%i=0; %i<%count; %i++)
      {
         %node = %camera.path.getObject(%i);
         %camera.pushBack(%node.getTransform(),%node.msToNext,%node.type, %node.smoothingType);
      }

      %camera.popFront();
      %camera.setPosition(0);
      %client.setControlObject(%camera);
      commandToClient(%client,'BenchmarkStart',%camera);
   }
   else
   {
      %client.setControlObject(%client.prevControl);
   }
}

//don't like this living in here
function PathCamera::onNode(%this, %nodeIndex)
{
 // Forward this callback to the datablock
 %this.getDatablock().onNode(%this, %nodeIndex);
}

function BenchmarkPathCamData::onNode(%this, %camera, %nodeIndex)
{
   %count = %camera.path.getCount() - 1;
   //check if we have reached the last node
   if(%count == %nodeIndex)
   {
      if(isObject(%camera.client))
      {
         //tell client we are done 
         commandToClient(%camera.client,'BenchmarkEnd');
         //reset control object
         %camera.client.setControlObject(%camera.client.prevControl);
      }
   }
}
