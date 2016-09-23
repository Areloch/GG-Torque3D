// Only load these functions if an Oculus VR device is present
//if(!isFunction(OpenVR::isDeviceActive))
//  return;

$Video::VREnabled = false;

function setupOpenVRActionMaps()
{
   new ActionMap(VRCanvasMap);

   VRCanvasMap.bind( mouse, xaxis, vrCanvasYaw );
   VRCanvasMap.bind( mouse, yaxis, vrCanvasPitch );
   VRCanvasMap.bind( mouse, button0, vrCanvasClick );
}

function setupDefaultOpenVRBinds(%moveMap)
{
   %moveMap.bind( oculusvr, ovr_sensorrot0, OVRSensorRotAA );
   %moveMap.bind( openvr, opvr_sensorrot0, OVRSensorRotAA );
}

function OVRSensorRotAA(%x, %y, %z, %a)
{
   $mvRotX0 = %x;
   $mvRotY0 = %y;
   $mvRotZ0 = %z;
   $mvRotA0 = %a;
}

function vrCanvasYaw(%val)
{
   VRCanvas.cursorNudge(%val * 0.10, 0);
}

function vrCanvasPitch(%val)
{
   VRCanvas.cursorNudge(0, %val * 0.10);
}

function vrCanvasClick(%active)
{
   VRCanvas.cursorClick(0, %active);  
}

function GuiOffscreenCanvas::checkCursor(%this)
{
   %count = %this.getCount();
   for(%i = 0; %i < %count; %i++)
   {
      %control = %this.getObject(%i);
      if ((%control.noCursor $= "") || !%control.noCursor)
      {
         %this.cursorOn();
         return true;
      }
   }
   // If we get here, every control requested a hidden cursor, so we oblige.

   %this.cursorOff();
   return false;
}

function GuiOffscreenCanvas::pushDialog(%this, %ctrl, %layer, %center)
{
   Parent::pushDialog(%this, %ctrl, %layer, %center);
   %cursorVisible = %this.checkCursor();

   if (%cursorVisible)
   {
      echo("OffscreenCanvas visible");
      VRCanvasMap.pop();
      VRCanvasMap.push();
   }
   else
   {
      echo("OffscreenCanvas not visible");
      VRCanvasMap.pop();
   }
}

function GuiOffscreenCanvas::popDialog(%this, %ctrl)
{
   Parent::popDialog(%this, %ctrl);
   %cursorVisible = %this.checkCursor();

   if (%cursorVisible)
   {
      echo("OffscreenCanvas visible");
      VRCanvasMap.pop();
      VRCanvasMap.push();
   }
   else
   {
      echo("OffscreenCanvas not visible");
      VRCanvasMap.pop();
   }
}


//-----------------------------------------------------------------------------

function oculusSensorMetricsCallback()
{
   return ovrDumpMetrics(0);
}

//-----------------------------------------------------------------------------

// Call this function from createCanvas() to have the Canvas attach itself
// to the Rift's display.  The Canvas' window will still open on the primary
// display if that is different from the Rift, but it will move to the Rift
// when it goes full screen.  If the Rift is not connected then nothing
// will happen.
function pointCanvasToVRDisplay()
{
   $Video::forceDisplayAdapter = OpenVR::getDisplayDeviceId();
}

//-----------------------------------------------------------------------------

// Call this function from GameConnection::initialControlSet() just before
// your "Canvas.setContent(PlayGui);" call, or at any time you wish to switch
// to a side-by-side rendering and the appropriate barrel distortion.  This
// will turn on side-by-side rendering and tell the GameConnection to use the
// Rift as its display device.
// Parameters:
// %gameConnection - The client GameConnection instance
// %trueStereoRendering - If true will enable stereo rendering with an eye
// offset for each viewport.  This will render each frame twice.  If false
// then a pseudo stereo rendering is done with only a single render per frame.
function enableOpenVRDisplay(%gameConnection, %trueStereoRendering)
{
   ovrResetAllSensors();
   OpenVRResetSensors();

   $mvRotIsEuler0 = false;
   $OculusVR::GenerateAngleAxisRotationEvents = true;
   $OculusVR::GenerateEulerRotationEvents = false;

   OpenVR::setHMDAsGameConnectionDisplayDevice(%gameConnection);
   PlayGui.renderStyle = "stereo side by side";
   //PlayGui.renderStyle =  "stereo separate";

   VRSetupOverlay();
   
   //$gfx::wireframe = true;
   // Reset all sensors
   openvrResetAllSensors();
}

function VRSetupOverlay()
{
   if (!isObject(VRCanvas))
   {
      new GuiOffscreenCanvas(VRCanvas) {
         targetSize = "512 512";
         targetName = "VRCanvas";
         dynamicTarget = true;
      };
   }

   if (!isObject(VROverlay))
   {
      exec("core/art/gui/VROverlay.gui");
   }

   VRCanvas.setContent(VROverlay);
   VRCanvas.setCursor(DefaultCursor);
   PlayGui.setStereoGui(VRCanvas);
   VRCanvas.setCursorPos("128 128");
   VRCanvas.cursorOff();
   $GameCanvas = VRCanvas;

   %ext = Canvas.getExtent();
   $VRMouseScaleX = 512.0 / 1920.0;
   $VRMouseScaleY = 512.0 / 1060.0;
}

// Call this function when ever you wish to turn off the stereo rendering
// and barrel distortion for the Rift.
function disableOpenVRDisplay(%gameConnection)
{
   VRCanvas.popDialog();
   $GameCanvas = Canvas;

   if (isObject(gameConnection))
   {
      %gameConnection.clearDisplayDevice();
   }
   PlayGui.renderStyle = "standard";
}

// Helper function to set the standard Rift control scheme.  You could place
// this function in GameConnection::initialControlSet() at the same time
// you call enableOculusVRDisplay().
function setStandardOpenVRControlScheme(%gameConnection)
{
   if($OpenVR::SimulateInput)
   {
      // We are simulating a HMD so allow the mouse and gamepad to control
      // both yaw and pitch.
      %gameConnection.setControlSchemeParameters(true, true, true);
   }
   else
   {
      // A HMD is connected so have the mouse and gamepad only add to yaw
      %gameConnection.setControlSchemeParameters(true, true, false);
   }
}

//-----------------------------------------------------------------------------

// Helper function to set the resolution for the Rift.
// Parameters:
// %fullscreen - If true then the display will be forced to full screen.  If
// pointCanvasToOculusVRDisplay() was called before the Canvas was created, then
// the full screen display will appear on the Rift.
function setVideoModeForOculusVRDisplay(%fullscreen)
{
   //%res = getOVRHMDResolution(0);
   //Canvas.setVideoMode(%res.x, %res.y, %fullscreen, 32, 4);
}

//-----------------------------------------------------------------------------

// Reset all Oculus Rift sensors.  This will make the Rift's current heading
// be considered the origin.
function resetOpenVRSensors()
{
   OpenVR::resetSensors();
}


// GuiTSCtrl optimal render style helper
function GuiTSCtrl::setOptimalRenderStyle()
{
   if ($Video::VREnabled)
   {
      %openvrEnabled = OpenVR::isDeviceActive();

      if (%openvrEnabled)
      {
         echo("VR: Using stereo separate view");
         %this.renderStyle = "stereo separate";
         return;
      }
   }

   echo("VR: using non-vr render style");
   %this.renderStyle = "standard";
}
