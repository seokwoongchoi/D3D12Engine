// Copyright (c) 2011 NVIDIA Corporation. All rights reserved.
//
// TO  THE MAXIMUM  EXTENT PERMITTED  BY APPLICABLE  LAW, THIS SOFTWARE  IS PROVIDED
// *AS IS*  AND NVIDIA AND  ITS SUPPLIERS DISCLAIM  ALL WARRANTIES,  EITHER  EXPRESS
// OR IMPLIED, INCLUDING, BUT NOT LIMITED  TO, NONINFRINGEMENT,IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.  IN NO EVENT SHALL  NVIDIA 
// OR ITS SUPPLIERS BE  LIABLE  FOR  ANY  DIRECT, SPECIAL,  INCIDENTAL,  INDIRECT,  OR  
// CONSEQUENTIAL DAMAGES WHATSOEVER (INCLUDING, WITHOUT LIMITATION,  DAMAGES FOR LOSS 
// OF BUSINESS PROFITS, BUSINESS INTERRUPTION, LOSS OF BUSINESS INFORMATION, OR ANY 
// OTHER PECUNIARY LOSS) ARISING OUT OF THE  USE OF OR INABILITY  TO USE THIS SOFTWARE, 
// EVEN IF NVIDIA HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.
//
// Please direct any bugs or questions to SDKFeedback@nvidia.com

//-----------------------------------------------------------------------------
// Global variables
//-----------------------------------------------------------------------------
#include "000_Header.fx"
#define PATCH_BLEND_BEGIN		1
#define PATCH_BLEND_END			20000
static const float2 arrUV[4] =
{
    float2(0.0, 0.0),
	float2(1.0, 0.0),
	float2(0.0, 1.0),
	float2(1.0, 1.0),
};
// Shading parameters
cbuffer cbShading
{
	// Water-reflected sky color
    float3 g_SkyColor;
	// The color of bottomless water body
    float3 g_WaterbodyColor;

	// The strength, direction and color of sun streak
    float g_Shineness;
    float3 g_SunDir;
    float3 g_SunColor;
	
	// The parameter is used for fixing an artifact
    float3 g_BendParam;

	// Perlin noise for distant wave crest
    float g_PerlinSize;
    float3 g_PerlinAmplitude;
    float3 g_PerlinOctave;
    float3 g_PerlinGradient;

	// Constants for calculating texcoord from position
    float g_TexelLength_x2;
    float g_UVScale;
    float g_UVOffset;
};

// Per draw call constants
cbuffer cbChangePerCall
{
	// Transform matrices
	float4x4	g_matLocal;
	float4x4	g_matWorldViewProj;
   
	// Misc per draw call constants
	float2		g_UVBase;
	float2		g_PerlinMovement;
	float3		g_LocalEye;
}



//-----------------------------------------------------------------------------------
// Texture & Samplers
//-----------------------------------------------------------------------------------
//Texture2D ReflectionMap;
Texture2D g_texDisplacement; // FFT wave displacement map in VS
Texture2D g_texPerlin; // FFT wave gradient map in PS
Texture2D g_texGradient; // Perlin wave displacement & gradient map in both VS & PS
Texture1D g_texFresnel; // Fresnel factor lookup table
TextureCube g_texReflectCube; // A small skybox cube texture for reflection
// FFT wave displacement map in VS, XY for choppy field, Z for height field
SamplerState g_samplerDisplacement;
// Perlin noise for composing distant waves, W for height field, XY for gradient
SamplerState g_samplerPerlin;
// FFT wave gradient map, converted to normal value in PS
SamplerState g_samplerGradient;
// Fresnel factor lookup table
SamplerState g_samplerFresnel;
// A small sky cubemap for reflection
SamplerState g_samplerCube;



struct VS_OceanOutput
{
    float4 Position : SV_Position0;
   
    float2 Uv : Uv0;
    float3 oPosition : Uv1;
    float2 arrUv : Uv2;
   
};

VS_OceanOutput OceanSurfVS(VertexTexture input)
{
    VS_OceanOutput Output;
   
	// Local position
   
    float4 pos_local = mul(float4(input.Position.xy, 0, 1), g_matLocal);
    //pos_local = ViewProjection(pos_local);
    float2 uv_local = pos_local.xy * g_UVScale + g_UVOffset;

	// Blend displacement to avoid tiling artifact
    float3 eye_vec = pos_local.xyz - g_LocalEye;
    float dist_2d = length(eye_vec.xy);
    float blend_factor = (PATCH_BLEND_END - dist_2d) / (PATCH_BLEND_END - PATCH_BLEND_BEGIN);
    blend_factor = clamp(blend_factor, 0, 1);

	// Add perlin noise to distant patches
    float perlin = 0;
    [flatten]
    if (blend_factor < 1)
    {
        float2 perlin_tc = uv_local * g_PerlinSize + g_UVBase;
        float perlin_0 = g_texPerlin.SampleLevel(g_samplerPerlin, perlin_tc * g_PerlinOctave.x + g_PerlinMovement, 0).w;
        float perlin_1 = g_texPerlin.SampleLevel(g_samplerPerlin, perlin_tc * g_PerlinOctave.y + g_PerlinMovement, 0).w;
        float perlin_2 = g_texPerlin.SampleLevel(g_samplerPerlin, perlin_tc * g_PerlinOctave.z + g_PerlinMovement, 0).w;
		
        perlin = perlin_0 * g_PerlinAmplitude.x + perlin_1 * g_PerlinAmplitude.y + perlin_2 * g_PerlinAmplitude.z;
    }

   

    float3 displacement = 0;
    if (blend_factor > 0)
        displacement = g_texDisplacement.SampleLevel(g_samplerDisplacement, uv_local, 0).xyz;
    displacement = lerp(float3(0, 0, perlin), displacement, blend_factor);
    pos_local.xyz += displacement;
    
    Output.Position = mul(pos_local, g_matWorldViewProj);
    Output.oPosition = pos_local.xyz;
    Output.Uv = uv_local;
  
    return Output;
}


//-----------------------------------------------------------------------------
// Name: OceanSurfPS
// Type: Pixel shader                                      
// Desc: Ocean shading pixel shader. Check SDK document for more details
//-----------------------------------------------------------------------------

float2 ComposePerlin(float blend_factor,float2 Uv)
{
// Compose perlin waves from three octaves
    float2 perlin_tc = Uv * g_PerlinSize + g_UVBase;
  float2 perlin_tc0 = (blend_factor < 1) ? perlin_tc * g_PerlinOctave.x + g_PerlinMovement : 0;
  float2 perlin_tc1 = (blend_factor < 1) ? perlin_tc * g_PerlinOctave.y + g_PerlinMovement : 0;
  float2 perlin_tc2 = (blend_factor < 1) ? perlin_tc * g_PerlinOctave.z + g_PerlinMovement : 0;

  float2 perlin_0 = g_texPerlin.Sample(g_samplerPerlin, perlin_tc0).xy;
  float2 perlin_1 = g_texPerlin.Sample(g_samplerPerlin, perlin_tc1).xy;
  float2 perlin_2 = g_texPerlin.Sample(g_samplerPerlin, perlin_tc2).xy;
    return (perlin_0 * g_PerlinGradient.x + perlin_1 * g_PerlinGradient.y + perlin_2 * g_PerlinGradient.z);
}


float3 ReflectedColor(float3 reflect_vec, float cos_angle)
{
    float4 ramp = g_texFresnel.Sample(g_samplerFresnel, cos_angle).xyzw;
	
    [flatten]
    if (reflect_vec.z < g_BendParam.x)
        ramp = lerp(ramp, g_BendParam.z, (g_BendParam.x - reflect_vec.z) / (g_BendParam.x - g_BendParam.y));
    reflect_vec.z = max(0, reflect_vec.z);

    float3 reflection = g_texReflectCube.Sample(g_samplerCube, reflect_vec).xyz;

    reflection = reflection * reflection * 2.5f;
    
    float3 reflected_color = lerp(g_SkyColor, g_SunColor * reflection, ramp.y);
    return lerp(g_WaterbodyColor, reflected_color, ramp.x);
}

float2 Texcoordmashoptimization(float blend_factor,float2 Uv)
{
    float2 fft_tc = (blend_factor > 0) ? Uv : 0;

    float2 grad = g_texGradient.Sample(g_samplerGradient, fft_tc).xy;
    grad = lerp(ComposePerlin(blend_factor, Uv), grad, blend_factor);
    return grad;
}
//Sun spots
SamplerState SamplerAnisotropicWrap
{
    Filter = ANISOTROPIC;
    AddressU = Wrap;
    AddressV = Wrap;
    MaxAnisotropy = 16;
};

float4 OceanSurfPS(VS_OceanOutput In) : SV_Target
{
    float3 eye_vec = g_LocalEye - In.oPosition;
    float3 eye_dir = normalize(eye_vec);
	
    float dist_2d = length(eye_vec.xy);
    float blend_factor = (PATCH_BLEND_END - dist_2d) / (PATCH_BLEND_END - PATCH_BLEND_BEGIN);
    blend_factor = clamp(blend_factor * blend_factor * blend_factor, 0, 1);

    float2 grad = Texcoordmashoptimization(blend_factor, In.Uv);
    float3 normal = normalize(float3(grad, g_TexelLength_x2));
    float3 reflect_vec = reflect(-eye_dir, normal);
    float cos_angle = dot(normal, eye_dir);
   

    float3 water_color = ReflectedColor(reflect_vec, cos_angle);

    float cos_spec = clamp(dot(reflect_vec, g_SunDir), 0, 1);
  
    float sun_spot = pow(cos_spec, g_Shineness);
    
    water_color += float3(0.23f, 0.23f, 0.2f) * sun_spot;
   
    return float4(water_color, 1);
    
  
}

RasterizerState OceanRS
{
    //FillMode = WireFrame;
    CullMode = None;
    //FrontCounterClockwise = false;
    DepthBias = 0;
    SlopeScaledDepthBias = 0.0f;
    DepthBiasClamp = 0.0f;
    DepthClipEnable = true;
    ScissorEnable = false;
    MultisampleEnable = true;
    AntialiasedLineEnable = false;
};
BlendState NoblendState
{
  
    BlendEnable[0] = false;
 
};

BlendState blendState
{
  
    BlendEnable[0] = true;
    DestBlend[0] = one;
  //  DestBlend[0] = SRC_COLOR;
    //SRC_COLOR;
    SrcBlend[0] = ONE;
    BlendOp[0] = Add;
    DestBlendAlpha[0] = ONE;
    //SrcBlendAlpha[0] = Zero;
    SrcBlendAlpha[0] = ONE;
    BlendOpAlpha[0] = Add;
    RenderTargetWriteMask[0] = 15;
};

technique11 T0
{
    
  
    pass P0
    {
        SetRasterizerState(OceanRS);
        SetBlendState(blendState, float4(0, 0, 0, 0), 0xFF);
        SetVertexShader(CompileShader(vs_5_0, OceanSurfVS()));
        SetPixelShader(CompileShader(ps_5_0, OceanSurfPS()));
    }

    pass P1
    {
        SetRasterizerState(OceanRS);
        SetBlendState(NoblendState, float4(0, 0, 0, 0), 0xFF);
        SetVertexShader(CompileShader(vs_5_0, OceanSurfVS()));
        SetPixelShader(CompileShader(ps_5_0, OceanSurfPS()));
    }
   
}
