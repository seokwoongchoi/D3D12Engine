//*********************************************************
//
// Copyright (c) Microsoft. All rights reserved.
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
//*********************************************************

static const float PI = 3.14159265358979323846;

Texture2D depthMap : register(t0);
Texture2D diffuseMap : register(t1);
Texture2D normalMap : register(t2);
Texture2D roughness_metalicMap : register(t3);


SamplerState sampleClamp : register(s0);

cbuffer SceneConstantBuffer : register(b0)
{
	float4 PerspectiveValues : packoffset(c0);
	float4x4 ViewInv : packoffset(c1);
	
	float4 DirToLight : packoffset(c5);
	float3 DirLightColor : packoffset(c6);
	float Time : packoffset(c6.w);
	
	float4x4 ToShadowSpace : packoffset(c7);
	float4 ToCascadeOffsetX : packoffset(c11);
	float4 ToCascadeOffsetY : packoffset(c12);
	float4 ToCascadeScale : packoffset(c13);
};

#define EyePosition (ViewInv[3].xyz)

static const float2 arrUV[4] =
{
	float2(0.0, 0.0),
    float2(1.0, 0.0),
	float2(0.0, 1.0),
	float2(1.0, 1.0),
};

struct PSInput
{
    float4 position : SV_POSITION;
	float2 cpPos : TEXCOORD0;
	float2 UV : TEXCOORD1;
  
};

PSInput VSMain(float4 position : POSITION,uint VertexID:SV_VertexID)
{
    PSInput result;

    result.position = position;
  
	result.cpPos = result.position.xy;
	result.UV = arrUV[VertexID].xy;

    return result;
}

// Analytic solution of in-scattering of light in participating media.
// Ref: http://blog.mmacklin.com/2010/05/29/in-scattering-demo/
// Models single scattering within isotropic media.
// Returns radiance intensity.
float InScatter(float3 rayStart, float3 rayDir, float3 lightPos, float dist)
{
    float scatterParamScale = 1.0f;
    float scatterProbability = 1.f / (4.f * PI); // normalization term for an isotropic phase function        
    float3 q = rayStart - lightPos;                // light to ray origin
    float b = dot(rayDir, q);
    float c = dot(q, q);

    // Evaluate integral.
    float s = 1.0f / sqrt(max(c - b * b, 0.00001f));
    float l = s * (atan((dist + b) * s) - atan(b*s));
    l *= scatterParamScale * scatterProbability;

    return l;
}



float ConvertZToLinearDepth(float depth)
{
	float linearDepth = PerspectiveValues.z / (depth + PerspectiveValues.w);
	return linearDepth;
}

float3 CalcWorldPos(float2 csPos, float depth)
{
	float4 position;

	position.xy = csPos.xy * PerspectiveValues.xy * depth;
	position.z = depth;
	position.w = 1.0;
	
	return mul(position, ViewInv).xyz;
}


struct Material
{
    
	float3 diffuseColor;
	float SkyMask;
    
	float3 Factors; //x=roughness,y=metallic
	float TerrainMask;

	float3 normal;
	float LinearDepth;
     
   
};

Material UnpackGBuffer(int2 location)
{
	Material Out;
	int3 location3 = int3(location, 0);
  
	float4 baseColorSpecInt = diffuseMap.Load(location3);
	Out.diffuseColor = float4(baseColorSpecInt.xyz, 1);
	Out.TerrainMask = baseColorSpecInt.w;
    
	float depth = depthMap.Load(location3).x;
	Out.LinearDepth = ConvertZToLinearDepth(depth);

    float4 normal = normalMap.Load(location3);
   	Out.normal = normalize(normal.xyz * 2.0 - 1.0);
	Out.SkyMask = normal.w;
    
    
	float2 Factors = roughness_metalicMap.Load(location3).xy;
	Out.Factors.x = Factors.x;
	Out.Factors.y = Factors.y;
    
   
	return Out;
}
float NdotL(float3 bump, float3 lightDir)
{
	return saturate(dot(bump, lightDir));
}


float3 CalcDirectional(float3 position, Material mat)
{
    
   
	float3 diffusecolor = mat.diffuseColor.rgb;
	float3 normal = mat.normal.rgb;
	float ndotl = NdotL(normal, DirToLight.xyz);
  
	float metallic = mat.Factors.y;

	float3 viewDir = EyePosition - position;
   
	float3 finalColor = mat.diffuseColor.rgb * ndotl;
    

    
     
   
    
	return float4(finalColor, 1.0f);
}

float4 PSMain(PSInput input) : SV_TARGET
{
  


	Material mat = UnpackGBuffer(input.position.xy);

	
	float3 position = CalcWorldPos(input.cpPos, mat.LinearDepth);
	float3 eyeToPixel = position - EyePosition;
   
	float3 finalColor = CalcDirectional(position, mat);
	
	return float4(finalColor, 1.0f);
	
}