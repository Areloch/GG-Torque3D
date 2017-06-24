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

// This is the default save location for any Particle datablocks created in the
// Particle Editor (this script is executed from onServerCreated())


datablock ParticleData(fireplaceP : DefaultParticle)
{
   animateTexture = "1";
   animTexTiling = "8 5";
   animTexFrames = "0-40";
   textureName = "art/particles/fireplace.png";
   animTexName = "art/particles/fireplace.png";
   dragCoefficient = "0";
   inheritedVelFactor = "0";
   lifetimeMS = "1000";
   lifetimeVarianceMS = "0";
   spinRandomMin = "0";
   spinRandomMax = "1";
   framesPerSec = "40";
   spinSpeed = "0";
   colors[0] = "1 1 1 1";
   colors[1] = "1 1 1 1";
   sizes[0] = "2";
   sizes[1] = "2";
   times[1] = "1";
   times[2] = "1";
};
