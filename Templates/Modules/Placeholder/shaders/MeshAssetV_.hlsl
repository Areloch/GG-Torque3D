#include "shaders/common/hlslstructs.hlsl"  
  
struct VertData
{
   float3 position        : POSITION;
   float4 color           : COLOR0;
   float3 normal          : NORMAL;   
   float2 texCoord        : TEXCOORD0;
};

struct ConnectData  
{  
   float4 hpos            : POSITION;
   float2 TexCoord        : TEXCOORD0; 
   float4 wsEyeVec        : TEXCOORD1;   
   //float4 hpos            : POSITION;
   //float2 out_texCoord    : TEXCOORD0;
   //float4 screenspacePos  : TEXCOORD1;
};  
  
//-----------------------------------------------------------------------------  
// Main  
//-----------------------------------------------------------------------------  
//ConnectData main( VertexIn_PNTTTB IN,  
//    uniform float4x4 modelview,  
//    uniform float4x4 objTrans,  
 //   uniform float3 eyePos)  
//ConnectData main( VertData IN,
//                  uniform float4x3 nodeTransforms[70] : register(C4),
//                  uniform float4x4 modelview       : register(C0))
ConnectData main( VertData IN,
                  uniform float4x3 nodeTransforms[70],
                  uniform float4x4 modelview,
                  uniform float4x4 objTrans,
                  uniform float3 eyePos)
{  
    ConnectData OUT;  

    OUT.TexCoord = IN.uv0;  
    //OUT.hpos = mul(modelview, float4(IN.pos.xyz,1));  

   //IN.pos = posePos;
   IN.normal = normalize(poseNormal);
   
   // Vert Position
   OUT.hpos = mul(modelview, float4(IN.pos.xyz,1));
   
   // Eye Space Depth (Out)
    float3 depthPos = mul( objTrans, float4( IN.pos.xyz, 1 ) ).xyz;
    OUT.wsEyeVec = float4( depthPos.xyz - eyePos, 1 );
   
   return OUT;
}  