//*****************************************************************************
// Torque -- GLSL procedural shader
//*****************************************************************************

// Dependencies:
#include "shaders/common/torque.glsl"

in float2 IN_texCoord;
in float4 IN_screenspacePos;
in float2 IN_vpos;

out float4 OUT_color;

uniform sampler2D diffuseMap;
uniform sampler2D bumpMap;
uniform sampler2D lightInfoBuffer;
uniform float4 specularColor;
uniform float specularPower;
uniform float specularStrength;
uniform float visibility;
uniform float4 rtParamslightInfoBuffer;


//-----------------------------------------------------------------------------
// Main
//-----------------------------------------------------------------------------
void main( )
{
   vec4 diffuseColor = toLinear(tex2D(diffuseMap, IN_texCoord));
   OUT_color = diffuseColor;
   vec4 diffuseColor = toLinear(tex2D(bumpMap, IN_texCoord));
   OUT_norm = diffuseColor;
}
