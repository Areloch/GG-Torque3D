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

//Following values will be changed during running of the benchmark
$BenchArrayCount = 0; //number of entries in the array for this run
$BenchRunNumber = 0; //current benchmark run number
$BenchRunsRequested = 1; //requested benchmark runs
$BenchId = 1; //id to select the benchmark path
$BenchWriteSummary = true;
$BenchWriteReport = true;

//Following values are user configurable values
$BenchOutDir = "benchmarks"; //output directory for benchmarking
$BenchSkipFrames = 10; //number of frames to skip when saving, often fps can be unsteady at first so this allows skipping a few frames
$BenchCsvSeperator = ","; // common values used for csv files include tab "\x09" , space " " or comma ","

//all arguments are optional
function Benchmark(%id,%numRuns,%writeSummary,%writeReport)
{     
   echo("Starting benchmark");

   //set some sane defaults
   if(%id $= "")
      %id = 1;

   if(%numRuns $= "")
      %numRuns = 1;

   if(%writeSummary $= "")
      %writeSummary = true;

   if(%writeReport $= "")
      %writeReport = true;

   //set our global params for this bench run
   $BenchRunsRequested = %numRuns;
   $BenchId = %id;
   $BenchWriteSummary = %writeSummary;
   $BenchWriteReport = %writeReport;

   if(!$BenchWriteSummary && !$BenchWriteReport)
      warn("Benchmark running without writing any results.");

   //check vsync setting
   if(!$pref::Video::disableVerticalSync)
   {
      error("Please disable vysnc for benchmarking. Aborting!");
      return;
   }
   //reset bench array count - we don't bother clearing previous results, they are over-written
   $BenchArrayCount=0;
   //store time string(with spaces removed) for consistent naming on multiple runs
   $BenchTimeStr = stripChars(getLocalTime()," ");
   commandToServer('RunBenchMark',%id);
}

function clientCmdBenchmarkStart(%camera)
{
   if(%camera.getClassName() !$= "PathCamera")
   {
      error("Invalid camera for benchmarking");
      return;
   }
   
   //enable the hudless gui
   HudlessPlayGui.toggle();
   //show benchmark gui
   Canvas.pushDialog(BenchmarkHud);
   
   //increment benchmark run number
   $BenchRunNumber++;
   echo("Starting benchmark run " @ $BenchRunNumber);
   
   // create our ScriptTickObject and set start processing ticks
   if(!isObject(BenchTicker))
   {
      %tickObj = new ScriptTickObject(BenchTicker);
      %tickObj.setProcessTicks(true);
   }
   $BenchCam = %camera;
}

function clientCmdBenchmarkEnd()
{
   echo("Benchmark complete, writing out results");
   processBenchmark();
   //more runs to do?
   if($BenchRunNumber < $BenchRunsRequested)
   {
      $BenchArrayCount=0;//reset array count
      commandToServer('RunBenchMark',$BenchId);
   }
   else
      $BenchRunNumber = 0;
   
   $BenchCam = 0;
   
   //don't want BenchTicker after we finsished the benchmark
   if(isObject(BenchTicker))
   {
      BenchTicker.delete();
   }
   
   //hide benchmark gui
   Canvas.popDialog(BenchmarkHud);
   //restore play gui
   HudlessPlayGui.toggle();
}

function clientCmdBenchmarkLogMessage(%msg)
{
   echo(%msg);
}

function BenchTicker::onInterpolateTick(%this, %timeDelta)
{
   //just store fps - we will calculate mspf later
   $BenchResults[0,$BenchArrayCount] = $fps::real;
   $BenchResults[1,$BenchArrayCount] = $GFXDeviceStatistics::polyCount;
   $BenchResults[2,$BenchArrayCount] = $GFXDeviceStatistics::drawCalls; 
   $BenchResults[3,$BenchArrayCount] = $BenchCam.position;
   $BenchArrayCount++;
   
   //update some gui stats - use rounding, easier to read
   BenchmarkFpsText.setValue("FPS: " @ mRound($fps::real));
   BenchmarkMspfText.setValue("MSPF: " @ mRound(1000 / $fps::real));
}

function processBenchmark()
{
   //if we are not writing anything than exit
   if(!$BenchWriteSummary && !$BenchWriteReport)
      return;

   echo("Writing benchmark results for run " @ $BenchRunNumber);
   if($BenchArrayCount < 1)
   {
      error("Benchmark array has no results");
      return;
   }

   %arrayCount = $BenchArrayCount-$BenchSkipFrames;

   //add mspf
   %index = 0;
   %mspfResults = 0;
   for (%i=0; %i<$BenchArrayCount; %i++)
   {
      %mspfResults[%index++] = 1000 / $BenchResults[0,%i];
   }
   %missionFile = fileBase($Client::MissionFile);
   %benchFile = "/" @ %missionFile  @ "_" @ $BenchTimeStr @ ".results" @ $BenchRunNumber @ ".csv";
   if ( !IsDirectory( $BenchOutDir ) )
   {
      if(!createPath( $BenchOutDir ))
      {
         error("Failed to create benchmark directory: " @ $BenchOutDir);
         return;
      }
   }

   if($BenchWriteReport)
   {
      %outFileHandle = new FileObject();
      %outFileName = $BenchOutDir @ %benchFile;
      if(!%outFileHandle.openForWrite(%outFileName))
      {
         error("Failed to open benchmarking output file: " @ %outFileName );
         return;
      }
      //write out results to csv format
      %outFileHandle.writeLine("FPS" @ $BenchCsvSeperator @ "MSPF" @ $BenchCsvSeperator @ "polycount" @ $BenchCsvSeperator @ "drawcalls"
         @ $BenchCsvSeperator @ "positionX" @ $BenchCsvSeperator @ "positionY" @ $BenchCsvSeperator @ "positionZ" );
      for (%i=$BenchSkipFrames; %i < %arrayCount; %i++)
      {
         %pos = $BenchResults[3,%i];
         %outFileHandle.writeLine($BenchResults[0,%i] @ $BenchCsvSeperator @ %mspfResults[%i] @ $BenchCsvSeperator @ $BenchResults[1,%i] @ $BenchCsvSeperator @ $BenchResults[2,%i]
          @ $BenchCsvSeperator @ %pos.x  @ $BenchCsvSeperator @ %pos.y @ $BenchCsvSeperator @ %pos.z);
      }

      %outFileHandle.close();
      %outFileHandle.delete();
   }

   //writing out summary
   if($BenchWriteSummary)
   {
      %benchFile = "/" @ %missionFile  @ "_" @ $BenchTimeStr @ ".summary" @ $BenchRunNumber @ ".txt";
      %outFileHandle = new FileObject();
      %outFileName = $BenchOutDir @ %benchFile;

      if(!%outFileHandle.openForWrite(%outFileName))
      {
         error("Failed to open benchmarking output file: " @ %outFileName );
         return;
      }
      //work out averages
      %avgFps = 0;%avgDraws=0;
      for (%i=$BenchSkipFrames; %i < %arrayCount; %i++)
      {
         %avgFps += $BenchResults[0,%i];
         %avgDraws += $BenchResults[2,%i];
      }
      %avgFps = %avgFps / %arrayCount;
      %avgMspf = 1000 / %avgFps;
      %avgDraws = %avgDraws / %arrayCount;

      //work out min/max
      %bigNumber = 999999999;
      %minFps = %bigNumber;%minMspf = %bigNumber;%minDraws=%bigNumber;//set these crazy high for the getMin function to work
      %maxFps = 0;%maxMspf = 0;%maxDraws = 0;
      for (%i=$BenchSkipFrames; %i < %arrayCount; %i++)
      {
         %minFps = getMin(%minFps,$BenchResults[0,%i]);
         %minMspf = getMin(%minMspf,%mspfResults[%i]);
         %minDraws = getMin(%minDraws,$BenchResults[2,%i]);
         
         %maxFps = getMax(%maxFps,$BenchResults[0,%i]);
         %maxMspf = getMax(%maxMspf,%mspfResults[%i]);         
         %maxDraws = getMax(%maxDraws,$BenchResults[2,%i]);
      }

      %resX = getWord($pref::Video::mode, $WORD::RES_X);
      %resY = getWord($pref::Video::mode, $WORD::RES_Y);
      
      %shadowsEnabled = !$pref::Shadows::disable;
      %alFormatTokenEnabled = AL_FormatToken.enabled;

      %outFileHandle.writeLine("**************************************************");
      %outFileHandle.writeLine("*********** Torque3D Benchmark Summary ***********");
      %outFileHandle.writeLine("**************************************************");
      %outFileHandle.writeLine("                  Scene Details                   ");
      %outFileHandle.writeLine("**************************************************");
      %outFileHandle.writeLine("Mission File: " @ $Server::MissionFile);
      %outFileHandle.writeLine("Render API: " @ GFXCardProfilerAPI::getRenderer());
      %outFileHandle.writeLine("GPU model: " @ GFXCardProfilerAPI::getCard()); 
      %outFileHandle.writeLine("Window Size: " @ %resX @"x" @ %resY);
      %outFileHandle.writeLine("Anisotropy: " @ $pref::Video::defaultAnisotropy);
      if(%alFormatTokenEnabled $= "true")
         %outFileHandle.writeLine("AL Token Format: " @ AL_FormatToken.format);
      %outFileHandle.writeLine("Light Manager: " @ $pref::lightManager);
      %outFileHandle.writeLine("Shadows enabled: " @ boolToString(%shadowsEnabled));
      if(%shadowsEnabled)
         %outFileHandle.writeLine("Shadow quality: " @ $pref::shadows::filterMode);               
      %outFileHandle.writeLine("HDR enabled: " @ boolToString(HDRPostFX.isEnabled()));
      %outFileHandle.writeLine("SSAO enabled: " @ boolToString(SSAOPostFx.isEnabled()));
      %outFileHandle.writeLine("FXAA enabled: " @ boolToString(FXAA_PostEffect.isEnabled()));
      %outFileHandle.writeLine("Fog enabled: " @ boolToString(FogPostFx.isEnabled()));
      %outFileHandle.writeLine("DOF enabled: " @ boolToString(DOFPostEffect.isEnabled()));
      %outFileHandle.writeLine("Vignette enabled: " @ boolToString(VignettePostEffect.isEnabled()));
      %outFileHandle.writeLine("Sound provider: " @ $pref::SFX::provider);
      %outFileHandle.writeLine("Sound device: " @ $pref::SFX::device);   
      %outFileHandle.writeLine("**************************************************");           
      %outFileHandle.writeLine("                     Results                      ");
      %outFileHandle.writeLine("**************************************************");
      %outFileHandle.writeLine("Frames captured: " @ %arrayCount);     
      %outFileHandle.writeLine("FPS average: " @ %avgFps);
      %outFileHandle.writeLine("FPS max: " @ %maxFps);
      %outFileHandle.writeLine("FPS min: " @ %minFps);
      %outFileHandle.writeLine("MSPF average: " @ %avgMspf);
      %outFileHandle.writeLine("MSPF max: " @ %maxMspf);
      %outFileHandle.writeLine("MSPF min: " @ %minMspf);
      %outFileHandle.writeLine("Draw calls average: " @ %avgDraws);
      %outFileHandle.writeLine("Draw calls max: " @ %maxDraws);
      %outFileHandle.writeLine("Draw calls min: " @ %minDraws);     
      
      %outFileHandle.close();
      %outFileHandle.delete();
   }
}

//convert bool value to a string
function boolToString(%val)
{
   if(%val)
      return "true";
   else
      return "false";
}
