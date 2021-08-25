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

Texture2D diffuseMap : register(t0);
Texture2D normalMap : register(t1);


SamplerState sampleWrap1 : register(s0);
SamplerState sampleWrap : register(s1);

#ifndef WITH_CLIPDISTANCE
#define WITH_CLIPDISTANCE 0
#endif

struct PSInput
{
    float4 position : SV_POSITION;
    float4 worldpos : POSITION;
    float2 uv : TEXCOORD0;
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
#if WITH_CLIPDISTANCE
    float clipDistance : SV_CLIPDISTANCE0;
#endif
};

#include "Common.hlsli"

PSInput VSMain(float3 position : POSITION, float3 normal : NORMAL, float2 uv : TEXCOORD0, float3 tangent : TANGENT)
{
    PSInput result;

    float4 inputPosition = float4(position, 1.0f);

    normal.z *= -1.0f;

    float4 newPosition = mul(inputPosition, model);
    result.worldpos = newPosition;

    newPosition = mul(newPosition, view);
    newPosition = mul(newPosition, projection);

    result.position = newPosition;
    result.uv = uv;
    result.normal = normal;
    result.tangent = tangent;

#if WITH_CLIPDISTANCE
    result.clipDistance = dot(result.worldpos, clipPlane);
#endif

    return result;
}

struct PS_GBUFFER_OUT
{
	float4 ColorSpecInt : SV_Target0;
	float4 Normal : SV_Target1;
	float4 Specular : SV_Target2;
   
};

PS_GBUFFER_OUT PackGBuffer(float3 BaseColor, float3 Normal, float metallic, float roughness, float terrainMask = 1)
{
	PS_GBUFFER_OUT Out;
	
	Out.ColorSpecInt = float4(BaseColor.rgb, terrainMask);
	Out.Normal = float4(normalize(Normal.rgb * 0.5 + 0.5), 1.0);
	Out.Specular = float4(roughness, metallic, 0.0, 0.0);
	return Out;
}


PS_GBUFFER_OUT PSMain(PSInput input)
{
	float4 diffuseColor = diffuseMap.Sample(sampleWrap, input.uv);
	float3 pixelNormal = CalcPerPixelNormal(input.uv, input.normal, input.tangent);
	float4 totalLight = ambientColor;

    
	return PackGBuffer(diffuseColor.rgb, pixelNormal, 0, 0, 0);

}

