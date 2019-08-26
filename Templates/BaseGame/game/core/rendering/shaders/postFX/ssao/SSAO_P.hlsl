#include "./../postFx.hlsl"
#include "../../shaderModelAutoGen.hlsl"

TORQUE_UNIFORM_SAMPLER2D(inputTex, 0);
TORQUE_UNIFORM_SAMPLER2D(deferredBuffer, 1);
TORQUE_UNIFORM_SAMPLER2D(randNormalTex, 2);

uniform float2 targetSize;

// script constants
uniform float radius;
uniform float intensity;
uniform float scale;
uniform float bias;

float3 getPosition(in float2 uv)
{ 
   return TORQUE_TEX2D(inputTex,uv).xyz;
}

float2 getRandom(in float2 uv)
{ 
   return normalize(TORQUE_TEX2D(randNormalTex, targetSize * uv / 64.0).xy * 2.0f - 1.0f);
}

float doAmbientOcclusion(in float2 tcoord,in float2 uv, in float3 p, in float3 cnorm)
{ 
   float3 diff = getPosition(tcoord + uv) - p; 
   const float3 v = normalize(diff); 
   const float d = length(diff)*scale; 
   return max(0.0,dot(cnorm,v)-bias)*(1.0/(1.0+d))*intensity;
}

float4 main(PFXVertToPix IN) : TORQUE_TARGET0
{
   float4 normDepth = TORQUE_DEFERRED_UNCONDITION(deferredBuffer, IN.uv0.xy);
   float3 n = normDepth.xzy;   // Note the swap of the green and blue channels here
   float depth = normDepth.a;

   // Early out if too far away.
   if(depth > 0.99999999)
      return float4(0,0,0,0);
   
   const float2 vec[4] = {float2(1,0),float2(-1,0),            
		          float2(0,1),float2(0,-1)};

   float3 p = getPosition(IN.uv0); 
   float2 rand = getRandom(IN.uv0); 

   float ao = 0.0f; 
   float rad = radius/p.z; 

   // Add early-out if radius is below a certain threshold (point is very far away)
   
   //**SSAO Calculation**// 
   int iterations = 4; 
   [unroll]
   for (int j = 0; j < iterations; ++j) 
   {  
      float2 coord1 = reflect(vec[j],rand)*rad;  
      float2 coord2 = float2(coord1.x*0.707 - coord1.y*0.707,              
		     coord1.x*0.707 + coord1.y*0.707);

      ao += doAmbientOcclusion(IN.uv0,coord1*0.25, p, n);  
      ao += doAmbientOcclusion(IN.uv0,coord2*0.5, p, n);  
      ao += doAmbientOcclusion(IN.uv0,coord1*0.75, p, n);  
      ao += doAmbientOcclusion(IN.uv0,coord2, p, n); 
   }  

   ao /= (float)iterations*4.0; 
   //**END**// 

   return ao;
}