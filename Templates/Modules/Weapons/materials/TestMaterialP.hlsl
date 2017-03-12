//*****************************************************************************
// Torque -- HLSL procedural shader
//*****************************************************************************

// Dependencies:
#include "shaders/common/torque.hlsl"

struct ConnectData
{
   float2 texCoord	: TEXCOORD0;
   float4 screenspacePos	: TEXCOORD1;
   float2 vpos	: VPOS;
};

struct Fragout
{
   float4 color	: COLOR0;
};

TORQUE_UNIFORM_SAMPLER2D(diffuseMap,0);
TORQUE_UNIFORM_SAMPLER2D(bumpMap,1);
TORQUE_UNIFORM_SAMPLER2D(lightInfoBuffer,2);
uniform float4 specularColor;
uniform float specularPower;
uniform float specularStrength;
uniform float visibility;
uniform float4 rtParamslightInfoBuffer;


//-----------------------------------------------------------------------------
// Main
//-----------------------------------------------------------------------------
float4 main( ConnectData IN )
{
   Fragout OUT;

   vec4 diffuseColor = toLinear(tex2D(diffuseMap, IN.texCoord));
   OUT_color = diffuseColor;
   vec4 diffuseColor = toLinear(tex2D(bumpMap, IN.texCoord));
   OUT_norm = diffuseColor;
}
