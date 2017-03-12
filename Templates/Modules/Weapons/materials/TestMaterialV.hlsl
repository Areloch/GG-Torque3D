//*****************************************************************************
// Torque -- HLSL procedural shader
//*****************************************************************************

// Dependencies:
#include "shaders/common/torque.hlsl"

struct VertexData
{
   float3 position : POSITION;
   float3 normal : NORMAL;
   float3 texCoord : TEXCOORD0;
   float3 T : TANGENT;
   float3 tangentW : TEXCOORD3;
};

struct ConnectData
{
   float2 texCoord	: TEXCOORD0;
   float4 screenspacePos	: TEXCOORD1;
   float2 vpos	: VPOS;
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

   //Position
   OUT.hpos = mul(modelview, float4(IN.position.xyz,1));

   //Base texture coords
   OUT.out_texCoord = (float2)IN.texCoord;

   //Normal mapping
   float3x3 objToTangentSpace;
   objToTangentSpace[0] = IN.T;
   objToTangentSpace[1] = cross( IN.T, normalize(IN.normal) ) * IN.tangentW;
   objToTangentSpace[2] = normalize(IN.normal);
   float3x3 viewToTangent = mul( objToTangentSpace, (float3x3)viewToObj );
   OUT.outViewToTangent = viewToTangent;

   //Depth
   float3 depthPos = mul( objTrans, float4( IN.position.xyz, 1 ) ).xyz;
   OUT.wsEyeVec = float4( depthPos.xyz - eyePosWorld, 1 );

}
