//*****************************************************************************
// Torque -- GLSL procedural shader
//*****************************************************************************
#include "shaders/common/gl/hlslCompat.glsl"

// Dependencies:
#include "shaders/common/gl/torque.glsl"

// Features:
// Vert Position
// Render Target Output = 0.0, output mask b
// Base Texture
// Diffuse Color
// Deferred Shading: Specular Explicit Numbers
// Deferred Shading: Mat Info Flags
// Reflect Cube
// Eye Space Depth (Out)
// Visibility
// GBuffer Conditioner
// Deferred Material
// Hardware Instancing

in vec2 _TEXCOORD0_;
in vec3 _TEXCOORD1_;
in float4 _TEXCOORD2_;
in float _TEXCOORD3_;
in vec4 _TEXCOORD4_;
in float3 _TEXCOORD5_;

// Struct defines

//Fragment shader OUT
out vec4 OUT_col;
out vec4 OUT_col1;
out vec4 OUT_col2;
out vec4 OUT_col3;


//-----------------------------------------------------------------------------
// Main                                                                        
//-----------------------------------------------------------------------------
uniform sampler2D diffuseMap     ;
uniform vec4     diffuseMaterialColor;
uniform float    metalness      ;
uniform float    smoothness     ;
uniform float    matInfoFlags   ;
uniform samplerCube cubeMap        ;
uniform float3   vEye           ;
uniform float4   oneOverFarplane;

void main()
{
   vec4 diffuseMaterialColor = diffuseMaterialColor;
   float metalness = metalness;
   float smoothness = smoothness;
   float matInfoFlags = matInfoFlags;
   float3 vEye = vEye;
   float4 oneOverFarplane = oneOverFarplane;

   //-------------------------
   vec2 IN_texCoord = _TEXCOORD0_;
   vec3 IN_reflectVec = _TEXCOORD1_;
   float4 IN_wsEyeVec = _TEXCOORD2_;
   float IN_visibility = _TEXCOORD3_;
   vec4 IN_inVpos = _TEXCOORD4_;
   float3 IN_gbNormal = _TEXCOORD5_;
   //-------------------------

   // Vert Position
   
   // Render Target Output = 0.0, output mask b
   OUT_col3 = vec4(0.00001);
   
   // Base Texture
   vec4 diffuseColor = tex2D(diffuseMap, IN_texCoord);
   diffuseColor = toLinear(diffuseColor);
   OUT_col1 = diffuseColor;
   
   // Diffuse Color
   OUT_col1 *= diffuseMaterialColor;
   
   // Deferred Shading: Specular Explicit Numbers
   OUT_col2.g = 1.0;
   OUT_col2.b = smoothness;
   OUT_col2.a = metalness;
   
   // Deferred Shading: Mat Info Flags
   OUT_col2.r = matInfoFlags;
   
   // Reflect Cube
   OUT_col3 = textureLod(  cubeMap, IN_reflectVec, min((1.0 - smoothness)*11.0 + 1.0, 8.0));
   
   // Eye Space Depth (Out)
#ifndef CUBE_SHADOW_MAP
   float eyeSpaceDepth = dot(vEye, (IN_wsEyeVec.xyz / IN_wsEyeVec.w));
#else
   float eyeSpaceDepth = length( IN_wsEyeVec.xyz / IN_wsEyeVec.w ) * oneOverFarplane.x;
#endif
   
   // Visibility
   vec2 vpos = IN_inVpos.xy / IN_inVpos.w;
   fizzle( vpos, IN_visibility );
   
   // GBuffer Conditioner
   float4 normal_depth = float4(normalize(IN_gbNormal), eyeSpaceDepth);

   // output buffer format: GFXFormatR16G16B16A16F
   // g-buffer conditioner: float4(normal.X, normal.Y, depth Hi, depth Lo)
   float4 _gbConditionedOutput = float4(sqrt(half(2.0/(1.0 - normal_depth.y))) * half2(normal_depth.xz), 0.0, normal_depth.a);
   
   // Encode depth into hi/lo
   float2 _tempDepth = frac(normal_depth.a * float2(1.0, 65535.0));
   _gbConditionedOutput.zw = _tempDepth.xy - _tempDepth.yy * float2(1.0/65535.0, 0.0);

   OUT_col = vec4(_gbConditionedOutput);
   
   // Deferred Material
   
   // Hardware Instancing
   
   
}
