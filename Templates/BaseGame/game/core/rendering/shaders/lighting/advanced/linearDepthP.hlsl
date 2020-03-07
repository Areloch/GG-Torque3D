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

#include "../../torque.hlsl"

struct ConnectData
{
    float4 hpos    : TORQUE_POSITION;
    float2 uv      : TEXCOORD;
};

uniform float2 nearFar;

TORQUE_UNIFORM_SAMPLER2D(sceneDepth, 0);

float4 main(ConnectData IN) : TORQUE_TARGET0
{
    /*
    n = near
    f = far
    z = depth buffer Z-value
    EZ  = eye Z value
    LZ  = depth buffer Z-value remapped to a linear [0..1] range (near plane to far plane)
    LZ2 = depth buffer Z-value remapped to a linear [0..1] range (eye to far plane)
    */
    float n = nearFar.x;
    float f = nearFar.y;
    float z = TORQUE_TEX2D( sceneDepth, IN.uv ).x;

    float eyeZ  = (n * f) / (f - z * (f - n));
    float LZ = (eyeZ - n) / (f - n);
    float LZ2 = eyeZ / f;

    return float4(z,0,0,1);
}