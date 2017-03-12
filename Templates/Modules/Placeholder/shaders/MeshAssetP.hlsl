// Dependencies:
#include "shaders/common/torque.hlsl"
#include "shaders/common/lighting.hlsl"

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

struct ConnectData
{
   float3 pos             : POSITION;
   float3 normal          : NORMAL;
   float2 uv0             : TEXCOORD0;
   float2 uv1             : TEXCOORD1;
   float4 color           : COLOR;
};

/*struct Fragout
{
   float4 col : COLOR0;
   float4 col1 : COLOR1;
   float4 col2 : COLOR2;
   float4 col3 : COLOR3;
};*/

//TORQUE_UNIFORM_SAMPLER2D(diffuseMap,0);
//TORQUE_UNIFORM_SAMPLER2D(specularMap,1);
//TORQUE_UNIFORM_SAMPLER2D(bumpMap,2);

//uniform float4    diffuseMaterialColor;
//uniform float     matInfoFlags;
//uniform float3    vEye;
//uniform float4    oneOverFarplane;

float4 main( ConnectData IN)  : COLOR0
{
   /*Fragout OUT;

   // Vert Position
   
   // Render Target Output = 0.0, output mask b
   OUT.col3 = 0.00001;
   
   // Base Texture
   float4 diffuseColor = TORQUE_TEX2D(diffuseMap, IN.texCoord);
   diffuseColor = toLinear(diffuseColor);
   OUT.col1 = diffuseColor;
   
   // Diffuse Color
   OUT.col1 *= diffuseMaterialColor;
   
   // Deferred Shading: Specular Map
   float metalness = TORQUE_TEX2D(specularMap, IN.texCoord).r;
   float smoothness = TORQUE_TEX2D(specularMap, IN.texCoord).b;
   smoothness = 1.0-smoothness;
   float4 specularColor = TORQUE_TEX2D(specularMap, IN.texCoord).ggga;
   OUT.col2.bga = float3(smoothness,specularColor.g,metalness);
   
   // Deferred Shading: Mat Info Flags
   OUT.col2.r = matInfoFlags;
   
   // Bumpmap [Deferred]
   float4 bumpNormal = TORQUE_TEX2D(bumpMap, IN.texCoord);
   bumpNormal.xyz = bumpNormal.xyz * 2.0 - 1.0;
   half3 gbNormal = (half3)mul( bumpNormal.xyz, IN.viewToTangent );
   
   // Visibility
   fizzle( IN.vpos.xy, IN.visibility );
   
   // Eye Space Depth (Out)
#ifndef CUBE_SHADOW_MAP
   float eyeSpaceDepth = dot(vEye, (IN.wsEyeVec.xyz / IN.wsEyeVec.w));
#else
   float eyeSpaceDepth = length( IN.wsEyeVec.xyz / IN.wsEyeVec.w ) * oneOverFarplane.x;
#endif
   
   // GBuffer Conditioner
   float4 normal_depth = float4(normalize(gbNormal), eyeSpaceDepth);

   // output buffer format: GFXFormatR16G16B16A16F
   // g-buffer conditioner: float4(normal.X, normal.Y, depth Hi, depth Lo)
   float4 _gbConditionedOutput = float4(sqrt(half(2.0/(1.0 - normal_depth.y))) * half2(normal_depth.xz), 0.0, normal_depth.a);
   
   // Encode depth into hi/lo
   float2 _tempDepth = frac(normal_depth.a * float2(1.0, 65535.0));
   _gbConditionedOutput.zw = _tempDepth.xy - _tempDepth.yy * float2(1.0/65535.0, 0.0);

   OUT.col = _gbConditionedOutput;
   
   // Substance Workaround
   
   // Deferred Material
   
   // Hardware Instancing
   
   // Roughest = 1.0
   
   //float4 diffuseColor = TORQUE_TEX2D(diffuseMap, IN.texCoord);
   float4 diffuseColor = float4(1,0,0,1);
   //diffuseColor = toLinear(diffuseColor);
   OUT.col1 = diffuseColor;
   
   OUT.col = float4(1,0,0,1);
   OUT.col2 = float4(1,0,0,1);
   OUT.col3 = float4(1,0,0,1);*/

   return float4(0,0,0,1);
}
