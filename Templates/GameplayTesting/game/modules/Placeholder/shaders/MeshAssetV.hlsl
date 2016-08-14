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
// Visibility
// Eye Space Depth (Out)
// GBuffer Conditioner
// Substance Workaround
// Deferred Material
// Hardware Instancing
// Roughest = 1.0

struct VertData
{
   float3 position        : POSITION;
   float tangentW        : TEXCOORD3;
   float3 normal          : NORMAL;
   float3 T               : TANGENT;
   float2 texCoord        : TEXCOORD0;
   float4 inst_objectTrans[4] : TEXCOORD4;
   float inst_visibility : TEXCOORD8;
};


struct ConnectData
{
   float4 hpos            : POSITION;
   float2 out_texCoord    : TEXCOORD0;
   float3x3 outViewToTangent : TEXCOORD1;
   float visibility      : TEXCOORD4;
   float4 wsEyeVec        : TEXCOORD5;
};

uniform float4x4 viewProj;
uniform float4x4 worldToCamera;
uniform float3   eyePosWorld;

//-----------------------------------------------------------------------------
// Main
//-----------------------------------------------------------------------------
ConnectData main( VertData IN )
{
   ConnectData OUT;

   // Vert Position
   float4x4 objTrans = { // Instancing!
      IN.inst_objectTrans[0],
      IN.inst_objectTrans[1],
      IN.inst_objectTrans[2],
      IN.inst_objectTrans[3] };
   float4x4 modelview = mul( viewProj, objTrans ); // Instancing!
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
   float4x4 worldViewOnly = mul( worldToCamera, objTrans ); // Instancing!
   float3x3 viewToObj = transpose( (float3x3)worldViewOnly ); // Instancing!
   float3x3 viewToTangent = mul( objToTangentSpace, (float3x3)viewToObj );
   OUT.outViewToTangent = viewToTangent;
   
   // Visibility
   OUT.visibility = IN.inst_visibility; // Instancing!
   
   // Eye Space Depth (Out)
   float3 depthPos = mul( objTrans, float4( IN.position.xyz, 1 ) ).xyz;
   OUT.wsEyeVec = float4( depthPos.xyz - eyePosWorld, 1 );
   
   // GBuffer Conditioner
   
   // Substance Workaround
   
   // Deferred Material
   
   // Hardware Instancing
   
   // Roughest = 1.0
   
   return OUT;
}
