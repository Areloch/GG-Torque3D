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

uniform float3    inProbePos[4];
uniform float     inProbeRadius[4];
uniform float3    inProbeBoxMin[4];
uniform float3    inProbeBoxMax[4];
uniform float     inProbeIsSphere[4];
uniform float3    inProbeLocalPos[4];

struct PS_OUTPUT
{
    float4 diffuse: TORQUE_TARGET0;
    float4 spec: TORQUE_TARGET1;
};

PS_OUTPUT main(ConnectData IN)
{
    PS_OUTPUT Output = (PS_OUTPUT)1;
    
    Output.diffuse = float4(1,0,0,1);
    Output.spec = float4(0,1,0,1);
    
    return Output;
}