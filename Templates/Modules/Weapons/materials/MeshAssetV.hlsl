#include "shaders/common/hlslstructs.h"  
  
struct VertData
{
   float3 pos             : POSITION;
   float3 normal          : NORMAL;
   float2 uv0             : TEXCOORD0;
   float2 uv1             : TEXCOORD1;
   float4 color           : COLOR;
   uint4 blendIndices0    : BLENDINDICES0;
   float4 blendWeight0    : BLENDWEIGHT0;
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

   // Hardware Skinning
   float3 posePos = 0.0;
   float3 poseNormal = 0.0;
   float4x3 poseMat;
   float3x3 poseRotMat;
   for (int i=0; i<4; i++) {
      int poseIdx = int(IN.blendIndices0[i]);
      float poseWeight = IN.blendWeight0[i];
      poseMat = nodeTransforms[poseIdx];
      poseRotMat = poseMat;
      posePos += (mul(float4(IN.pos, 1), poseMat)).xyz * poseWeight;
      poseNormal += (mul(IN.normal,poseRotMat) * poseWeight);
   }
   //IN.pos = posePos;
   IN.normal = normalize(poseNormal);
   
   // Vert Position
   OUT.hpos = mul(modelview, float4(IN.pos.xyz,1));
   
   // Eye Space Depth (Out)
    float3 depthPos = mul( objTrans, float4( IN.pos.xyz, 1 ) ).xyz;
    OUT.wsEyeVec = float4( depthPos.xyz - eyePos, 1 );
   
   return OUT;
}  