#include "../../../gl/hlslCompat.glsl"
#include "shadergen:/autogenConditioners.h"

#include "farFrustumQuad.glsl"
#include "lightingUtils.glsl"
#include "../../../gl/lighting.glsl"
#include "../../../gl/torque.glsl"
#line 6

in vec4 pos;
in vec4 wsEyeDir;
in vec4 ssPos;
in vec4 vsEyeDir;

uniform sampler2D deferredBuffer;
uniform sampler2D lightBuffer;
uniform sampler2D colorBuffer;
uniform sampler2D matInfoBuffer;

uniform vec4 rtParams0;
uniform vec4 lightColor;

uniform float lightBrightness;
uniform vec3 lightPosition;

uniform vec4 lightMapParams;
uniform vec4 vsFarPlane;
uniform vec4 lightParams;

uniform float lightRange;
uniform float shadowSoftness;
uniform vec2 lightAttenuation;

uniform mat3 viewToLightProj;
uniform mat3 dynamicViewToLightProj;

uniform mat4 invViewMat;

uniform vec4 SkyColor;
uniform vec4 GroundColor;
uniform float useCubemap;
uniform float Intensity;
uniform samplerCube cubeMap;

out vec4 OUT_col;

void main()
{ 
   // Compute scene UV
   vec3 ssPos = ssPos.xyz / ssPos.w;
   vec2 uvScene = getUVFromSSPos( ssPos, rtParams0 );
   
   // Matinfo flags
   vec4 matInfo = texture( matInfoBuffer, uvScene ); 

   // Sample/unpack the normal/z data
   vec4 deferredSample = deferredUncondition( deferredBuffer, uvScene );
   vec3 normal = deferredSample.rgb;
   float depth = deferredSample.a;
   if (depth>0.9999)
   {
      OUT_col = vec4(0,0,0,0);
      return;
   }
   
   // Eye ray - Eye -> Pixel
   vec3 eyeRay = getDistanceVectorToPlane( -vsFarPlane.w, vsEyeDir.xyz, vsFarPlane );
   vec3 viewSpacePos = eyeRay * depth;
      
   // Build light vec, get length, clip pixel if needed
   vec3 lightVec = lightPosition - viewSpacePos;
   float lenLightV = length( lightVec );
   clip( lightRange - lenLightV );

   // Get the attenuated falloff.
   float atten = attenuate( lightColor, lightAttenuation, lenLightV );
   clip( atten - 1e-6 );
   
   vec4 color = vec4(1, 1, 1, 1);

   // Need world-space normal.
   vec3 wsNormal = vec4(vec4(normal, 1) * invViewMat).rgb;
   //vec3 wsNormal = mul(normal, IN.wsEyeDir.rgb);
   //vec3 wsNormal = vec3(0, 0, 1);

   //color = vec4(wsNormal, 1);
   
   if (!useCubemap)
   {
      // Set the direction to the sky
      vec3 SkyDirection = vec3(0.0f, 0.0f, 1.0f);

      // Set ground color
      vec4 Gc = GroundColor;

      // Set sky color
      vec4 Sc = SkyColor;

      // Set the intensity of the hemisphere color
      float Hi = Intensity;

      float w = Hi * (1.0 + dot(SkyDirection, normal));
      color = (w * Sc + (1.0 - w) * Gc) * color;
   }
   else
   {
      vec3 reflectionVec = reflect(wsEyeDir, vec4(normalize(wsNormal),1)).rgb;
      float smoothness = min((1.0 - matInfo.b)*11.0 + 1.0, 1.0);//bump up to 8 for finalization
      vec3 ref = vec4(reflectionVec, smoothness).rgb;
      vec4 tmpColor = texture(cubeMap, ref); 
      color = tmpColor;
      //
      color.a = 1;

      color *= Intensity;
   }
   //return hdrEncode(vec4(color.rgb, 0.0));

   OUT_col = saturate(toLinear(color));
}
