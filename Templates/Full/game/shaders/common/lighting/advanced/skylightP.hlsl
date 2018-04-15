#include "../../shaderModelAutoGen.hlsl"

#include "farFrustumQuad.hlsl"
#include "lightingUtils.hlsl"
#include "../../lighting.hlsl"
#include "../../torque.hlsl"

struct ConvexConnectP
{
   float4 pos : TORQUE_POSITION;
   float4 wsEyeDir : TEXCOORD0;
   float4 ssPos : TEXCOORD1;
   float4 vsEyeDir : TEXCOORD2;
};

TORQUE_UNIFORM_SAMPLER2D(deferredBuffer, 0);
TORQUE_UNIFORM_SAMPLER2D(matInfoBuffer, 1);
TORQUE_UNIFORM_SAMPLERCUBE(cubeMap, 2);

uniform float4 rtParams0;

uniform float4 vsFarPlane;

uniform float4x4 invViewMat;

uniform float3 eyePosWorld;

//SHTerms
/*uniform float4 SHTerms0;
uniform float4 SHTerms1;
uniform float4 SHTerms2;
uniform float4 SHTerms3;
uniform float4 SHTerms4;
uniform float4 SHTerms5;
uniform float4 SHTerms6;
uniform float4 SHTerms7;
uniform float4 SHTerms8;

uniform float SHConsts0;
uniform float SHConsts1;
uniform float SHConsts2;
uniform float SHConsts3;
uniform float SHConsts4;

float4 decodeSH(float3 normal)
{
   float x = normal.x;
   float y = normal.y;
   float z = normal.z;

   float3 l00 = SHTerms0.rgb;

   float3 l10 = SHTerms1.rgb;
   float3 l11 = SHTerms2.rgb;
   float3 l12 = SHTerms3.rgb;

   float3 l20 = SHTerms4.rgb;
   float3 l21 = SHTerms5.rgb;
   float3 l22 = SHTerms6.rgb;
   float3 l23 = SHTerms7.rgb;
   float3 l24 = SHTerms8.rgb;

   float3 result = (
         l00 * SHConsts0 +

         l12 * SHConsts1 * x +
         l10 * SHConsts1 * y +
         l11 * SHConsts1 * z +

         l20 * SHConsts2 * x*y +
         l21 * SHConsts2 * y*z +
         l22 * SHConsts3 * (3.0*z*z - 1.0) +
         l23 * SHConsts2 * x*z +
         l24 * SHConsts4 * (x*x - y*y)
      );

    return float4(result,1);
}*/

float4 main( ConvexConnectP IN ) : TORQUE_TARGET0
{ 
    // Compute scene UV
    float3 ssPos = IN.ssPos.xyz / IN.ssPos.w; 

    //float4 hardCodedRTParams0 = float4(0,0.0277777780,1,0.972222209);
    float2 uvScene = getUVFromSSPos( ssPos, rtParams0 );

    // Matinfo flags
    float4 matInfo = TORQUE_TEX2D( matInfoBuffer, uvScene ); 

    // Sample/unpack the normal/z data
    float4 deferredSample = TORQUE_DEFERRED_UNCONDITION( deferredBuffer, uvScene );
    float3 normal = deferredSample.rgb;
    float depth = deferredSample.a;
    if (depth>0.9999)
        return float4(0,0,0,0); 

    // Need world-space normal.
    float3 wsNormal = mul(float4(normal, 1), invViewMat).rgb;

    float4 color = float4(1, 1, 1, 1);
    float4 ref = float4(0,0,0,0);
    float alpha = 0.3; //TODO: fix blending and bring this back to a real value

    float3 eyeRay = getDistanceVectorToPlane( -vsFarPlane.w, IN.vsEyeDir.xyz, vsFarPlane );
    float3 viewSpacePos = eyeRay * depth;

    float3 wsEyeRay = mul(float4(eyeRay, 1), invViewMat).rgb;

    // Use eye ray to get ws pos
    float3 worldPos = float3(eyePosWorld + wsEyeRay * depth);
    float smoothness = min((1.0 - matInfo.b)*11.0 + 1.0, 8.0);//bump up to 8 for finalization

    float3 reflectionVec = reflect(IN.wsEyeDir, float4(wsNormal,1)).xyz;

    ref = float4(reflectionVec, smoothness);

    color = TORQUE_TEXCUBELOD(cubeMap, ref);

    //float4 specularColor = (color);
    //float4 indirectColor = (decodeSH(wsNormal));

    //color.rgb = lerp(indirectColor.rgb * 1.5, specularColor.rgb * 1.5, matInfo.b);

    return float4(color.rgb, alpha);

}
