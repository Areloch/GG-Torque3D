//*****************************************************************************
// Torque -- HLSL procedural shader
//*****************************************************************************

// Dependencies:
#include "shaders/common/torque.hlsl"

// Features:
// Vert Position
// Render Target Output = 0.0, output mask b
// Base Texture
// Diffuse Color
// Deferred Shading: Specular Map
// Deferred Shading: Mat Info Flags
// Bumpmap [Deferred]
// Reflect Cube
// Eye Space Depth (Out)
// Visibility
// GBuffer Conditioner
// Deferred Material
// DXTnm
// Substance Workaround

struct VertData
{
   float3 position        : POSITION;
   float tangentW        : TEXCOORD3;
   float3 normal          : NORMAL;
   float3 T               : TANGENT;
   float2 texCoord        : TEXCOORD0;
};


struct ConnectData
{
   float4 hpos            : POSITION;
   float2 out_texCoord    : TEXCOORD0;
   float3x3 outViewToTangent : TEXCOORD1;
   float3 reflectVec      : TEXCOORD4;
   float4 wsEyeVec        : TEXCOORD5;
};


//-----------------------------------------------------------------------------
// Main
//-----------------------------------------------------------------------------
ConnectData main( VertData IN,
                  uniform float4x4 modelview       : register(C0),
                  uniform float4x4 viewToObj       : register(C4),
                  uniform float4x4 objTrans        : register(C8),
                  uniform float3   eyePosWorld     : register(C12)
)
{
   ConnectData OUT;

   // Vert Position
   OUT.hpos = mul(modelview, float4(IN.position.xyz,1));
   
   // Render Target Output = 0.0, output mask b
   
   // Base Texture
   OUT.out_texCoord = (float2)IN.texCoord;
   
   // Diffuse Color
   
   // Deferred Shading: Specular Map
   
   // Deferred Shading: Mat Info Flags
   
   // Bumpmap [Deferred]
   float3x3 objToTangentSpace;
   objToTangentSpace[0] = IN.T;
   objToTangentSpace[1] = cross( IN.T, normalize(IN.normal) ) * IN.tangentW;
   objToTangentSpace[2] = normalize(IN.normal);
   float3x3 viewToTangent = mul( objToTangentSpace, (float3x3)viewToObj );
   OUT.outViewToTangent = viewToTangent;
   
   // Reflect Cube
   float3 cubeVertPos = mul(objTrans, float4(IN.position,1)).xyz;
   float3 cubeNormal = ( mul( (objTrans),  float4(IN.normal, 0) ) ).xyz;
   cubeNormal = bool(length(cubeNormal)) ? normalize(cubeNormal) : cubeNormal;
   float3 eyeToVert = cubeVertPos - eyePosWorld;
   OUT.reflectVec = reflect(eyeToVert, cubeNormal);
   
   // Eye Space Depth (Out)
   float3 depthPos = mul( objTrans, float4( IN.position.xyz, 1 ) ).xyz;
   OUT.wsEyeVec = float4( depthPos.xyz - eyePosWorld, 1 );
   
   // Visibility
   
   // GBuffer Conditioner
   
   // Deferred Material
   
   // DXTnm
   
   // Substance Workaround
   
   return OUT;
}
