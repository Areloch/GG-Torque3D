#include "shadergen:/autogenConditioners.h"
#include "torque.hlsl"

struct Conn
{
   float4 position : POSITION;
   float2 uv0      : TEXCOORD0;
   float3 wsEyeRay : TEXCOORD1;
};

uniform sampler2D prePassBuffer : register(S0);
uniform sampler2D matInfoBuffer : register(S2);

uniform float3 eyePosWorld;
uniform float3 volumeStart;
uniform float3 volumeSize;
uniform float4x4 invViewMat;
uniform float4 SkyColor;
uniform float4 GroundColor;
uniform float Intensity;

uniform float useCubemap;
uniform samplerCUBE  cubeMap : register(S1);

float4 main( Conn IN ) : COLOR0
{ 
   float4 prepassSample = prepassUncondition( prePassBuffer, IN.uv0 );
   float3 normal = prepassSample.rgb;
   float depth = prepassSample.a;

   // Use eye ray to get ws pos
   float4 worldPos = float4(eyePosWorld + IN.wsEyeRay * depth, 1.0f);

   float3 volume_position = (worldPos.xyz - volumeStart) / volumeSize;
   if ( volume_position.x < 0 || volume_position.x > 1 || 
        volume_position.y < 0 || volume_position.y > 1 || 
        volume_position.z < 0 || volume_position.z > 1 )
   {
        return float4(0.0, 0.0, 0.0, 0.0); 
   }

   // Need world-space normal.
   float3 wsNormal = mul(normal, invViewMat);

   float4 color = float4(1, 1, 1, 1);

   if (!useCubemap)
   {
      // Set the direction to the sky
      float3 SkyDirection = float3(0.0f, 0.0f, 1.0f);

      // Set ground color
      float4 Gc = GroundColor;

      // Set sky color
      float4 Sc = SkyColor;

      // Set the intensity of the hemisphere color
      float Hi = Intensity;

      float w = Hi * (1.0 + dot(SkyDirection, normal));
      color = (w * Sc + (1.0 - w) * Gc) * color;
   }
   else
   {
      float3 reflectionVec = reflect(IN.wsEyeRay, wsNormal);

      float spec = (1 - tex2D(matInfoBuffer, IN.uv0).b) * 6.0;

      color = texCUBElod(cubeMap, float4(reflectionVec, spec));
      color.a = 1;

      //spec vis
      //float specl = tex2D(matInfoBuffer, IN.uv0).b;

      //color = float4(specl, specl, specl, 1);

      //color *= Intensity;
   }

   //return color;
   return hdrEncode(float4(color.rgb, 0.0));
}
