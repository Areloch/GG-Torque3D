#include "core/shaders/shaderModelAutoGen.hlsl"

#include "farFrustumQuad.hlsl"
#include "lightingUtils.hlsl"
#include "core/shaders/lighting.hlsl"
#include "core/shaders/torque.hlsl"

struct ConvexConnectP
{
   float4 pos : TORQUE_POSITION;
   float4 wsEyeDir : TEXCOORD0;
   float4 ssPos : TEXCOORD1;
   float4 vsEyeDir : TEXCOORD2;
};

TORQUE_UNIFORM_SAMPLER2D(deferredBuffer, 0);
TORQUE_UNIFORM_SAMPLER2D(lightBuffer, 1);
TORQUE_UNIFORM_SAMPLER2D(colorBuffer, 2);
TORQUE_UNIFORM_SAMPLER2D(matInfoBuffer, 3);

uniform float4 rtParams0;
uniform float4 lightColor;

uniform float  lightBrightness;
uniform float3 lightPosition;

uniform float4 lightMapParams;
uniform float4 vsFarPlane;
uniform float4 lightParams;

uniform float  lightRange;
uniform float shadowSoftness;
uniform float2 lightAttenuation;

uniform float3x3 viewToLightProj;
uniform float3x3 dynamicViewToLightProj;

uniform float4x4 invViewMat;

/*uniform float3 eyePosWorld;
uniform float3 volumeStart;
uniform float3 volumeSize;
uniform float4x4 invViewMat;
uniform float4 SkyColor;
uniform float4 GroundColor;
uniform float Intensity;

uniform float useCubemap;
TORQUE_UNIFORM_SAMPLERCUBE(cubeMap, 1);*/

uniform float4 SkyColor;
uniform float4 GroundColor;
uniform float useCubemap;
uniform float Intensity;
TORQUE_UNIFORM_SAMPLERCUBE(cubeMap, 4);

float4 main( ConvexConnectP IN ) : TORQUE_TARGET0
{ 
   // Compute scene UV
   float3 ssPos = IN.ssPos.xyz / IN.ssPos.w;
   float2 uvScene = getUVFromSSPos( ssPos, rtParams0 );
   
   // Matinfo flags
   float4 matInfo = TORQUE_TEX2D( matInfoBuffer, uvScene ); 

   // Sample/unpack the normal/z data
   float4 deferredSample = TORQUE_DEFERRED_UNCONDITION( deferredBuffer, uvScene );
   float3 normal = deferredSample.rgb;
   float depth = deferredSample.a;
   if (depth>0.9999)
      return float4(0,0,0,0);
   
   // Eye ray - Eye -> Pixel
   float3 eyeRay = getDistanceVectorToPlane( -vsFarPlane.w, IN.vsEyeDir.xyz, vsFarPlane );
   float3 viewSpacePos = eyeRay * depth;
      
   // Build light vec, get length, clip pixel if needed
   float3 lightVec = lightPosition - viewSpacePos;
   float lenLightV = length( lightVec );
   clip( lightRange - lenLightV );

   // Get the attenuated falloff.
   float atten = attenuate( lightColor, lightAttenuation, lenLightV );
   clip( atten - 1e-6 );
   
   float4 color = float4(1, 1, 1, 1);

   // Need world-space normal.
   float3 wsNormal = mul(float4(normal, 1), invViewMat).rgb;
   //float3 wsNormal = mul(normal, IN.wsEyeDir.rgb);
   //float3 wsNormal = float3(0, 0, 1);

   //color = float4(wsNormal, 1);
   
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
      float3 reflectionVec = reflect(IN.wsEyeDir, float4(normalize(wsNormal),1)).rgb;
      float smoothness = min((1.0 - matInfo.b)*11.0 + 1.0, 1.0);//bump up to 8 for finalization
      float4 ref = float4(reflectionVec, smoothness);
      color = TORQUE_TEXCUBELOD(cubeMap, ref);
      //color = TORQUE_TEXCUBE(cubeMap, reflectionVec);
      color.a = 1;

      color *= Intensity;
   }
   //return hdrEncode(float4(color.rgb, 0.0));

   return saturate(toLinear(color));
}
