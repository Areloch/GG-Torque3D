#include "torque.hlsl"  
#include "lighting.hlsl"
  
uniform sampler diffuseMap: register( S0 );  
//uniform sampler NormalSampler: register( S1 );  
//uniform sampler SpecSampler: register( S2 );  
  
struct ConnectData  
{  
   float4 hpos            : POSITION;
   float2 TexCoord        : TEXCOORD0; 
   float4 wsEyeVec        : TEXCOORD1;  
};  

inline void Uncondition(in float4 bufferSample, out float3 lightColor, out float NL_att, out float specular)
{
   lightColor = bufferSample.rgb;
   NL_att = dot(bufferSample.rgb, float3(0.3576, 0.7152, 0.1192));
   specular = bufferSample.a;
}
  
float4 main( ConnectData IN ) : COLOR  
{  
   /*float2 TexUV = IN.TexCoord.xy;  
   float3 EV = normalize(IN.EyeVec);  
   float3 LV = normalize(IN.LightVec);  
   float3 WN = normalize(IN.WorldNormal);  
   float3 TN = normalize(IN.WorldTangent);  
   float3 BN = normalize(IN.WorldBinormal);  */
  
   float4 Diff = normalize(tex2D(diffuseMap, IN.TexCoord.xy));  
  
   /*float3 Normal = normalize(tex2D(NormalSampler, TexUV).xyz * 2.0 - 1.0);  
   float3 N = (WN * Normal.z ) + (Normal.x * BN + Normal.y * -TN);  
   float4 DotLN = dot(LV, normalize(N));  
  
   float4 Spec = SpecularFrontOn(N, EV, LV, 0.28, normalize(tex2D(SpecSampler, TexUV).a));  
  
   float4 OutColor = (Diff + (DotLN * Diff) + Spec);  */
   
   float4 OutColor = Diff;
  
   return hdrEncode(OutColor);  
}  