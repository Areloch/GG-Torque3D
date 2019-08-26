// SSAO: depth-to-viewspace-position pixel shader

#include "./../postFx.hlsl"
#include "../../shaderModelAutoGen.hlsl"

TORQUE_UNIFORM_SAMPLER2D(deferredBuffer, 0);

uniform float2 nearFar;

float4 main(PFXVertToPix IN) : TORQUE_TARGET0
{
   float4 normDepth = TORQUE_DEFERRED_UNCONDITION(deferredBuffer, IN.uv0.xy);
   float depth = normDepth.a;
   
   // Interpolated ray pointing to far plane in view space
   float3 frustumRayVS = normalize(IN.wsEyeRay);
   
   return float4(nearFar.y * depth * frustumRayVS, 1.0);
}