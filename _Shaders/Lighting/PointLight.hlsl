#include "common.hlsl"
#include "PBR.hlsl"
TextureCube<float> PointShadowMapTexture : register( t4 );

/////////////////////////////////////////////////////////////////////////////
// constants
/////////////////////////////////////////////////////////////////////////////
cbuffer cbPointLightDomain : register( b0 )
{
	float4x4 LightProjection : packoffset( c0 );
}

cbuffer cbPointLightPixel : register( b1 )
{
	float3 PointLightPos			: packoffset( c0 );
	float PointLightRangeRcp		: packoffset( c0.w );
	float3 PointColor				: packoffset( c1 );
    float PointIntencity            : packoffset( c1.w);
	float2 LightPerspectiveValues	: packoffset( c2 );
}

/////////////////////////////////////////////////////////////////////////////
// Vertex shader
/////////////////////////////////////////////////////////////////////////////
float4 PointLightVS() : SV_Position
{
    return float4(0.0, 0.0, 0.0, 1.0); 
}

/////////////////////////////////////////////////////////////////////////////
// Hull shader
/////////////////////////////////////////////////////////////////////////////
struct HS_CONSTANT_DATA_OUTPUT
{
	float Edges[4] : SV_TessFactor;
	float Inside[2] : SV_InsideTessFactor;
};

HS_CONSTANT_DATA_OUTPUT PointLightConstantHS()
{
	HS_CONSTANT_DATA_OUTPUT Output;
	
	float tessFactor = 18.0;
	Output.Edges[0] = Output.Edges[1] = Output.Edges[2] = Output.Edges[3] = tessFactor;
	Output.Inside[0] = Output.Inside[1] = tessFactor;

	return Output;
}

struct HS_OUTPUT
{
	float3 HemiDir : POSITION;
};

static const float3 HemilDir[2] = {
	float3(1.0, 1.0,1.0),
	float3(-1.0, 1.0, -1.0)
};

[domain("quad")]
[partitioning("integer")]
[outputtopology("triangle_ccw")]
[outputcontrolpoints(4)]
[patchconstantfunc("PointLightConstantHS")]
HS_OUTPUT PointLightHS(uint PatchID : SV_PrimitiveID)
{
	HS_OUTPUT Output;

	Output.HemiDir = HemilDir[PatchID];

	return Output;
}

/////////////////////////////////////////////////////////////////////////////
// Domain Shader shader
/////////////////////////////////////////////////////////////////////////////
struct DS_OUTPUT
{
	float4 Position		: SV_POSITION;
	float3 PositionXYW	: TEXCOORD0;
};

[domain("quad")]
DS_OUTPUT PointLightDS( HS_CONSTANT_DATA_OUTPUT input, float2 UV : SV_DomainLocation, const OutputPatch<HS_OUTPUT, 4> quad)
{
	// Transform the UV's into clip-space
	float2 posClipSpace = UV.xy * 2.0 - 1.0;

	// Find the absulate maximum distance from the center
	float2 posClipSpaceAbs = abs(posClipSpace.xy);
	float maxLen = max(posClipSpaceAbs.x, posClipSpaceAbs.y);

	// Generate the final position in clip-space
	float3 normDir = normalize(float3(posClipSpace.xy, (maxLen - 1.0)) * quad[0].HemiDir);
	float4 posLS = float4(normDir.xyz, 1.0);
	
	// Transform all the way to projected space
	DS_OUTPUT Output;
	Output.Position = mul( posLS, LightProjection );

	// Store the clip space position
	Output.PositionXYW = Output.Position.xyw;

	return Output;
}

/////////////////////////////////////////////////////////////////////////////
// Pixel shader
/////////////////////////////////////////////////////////////////////////////

float PointShadowPCF(float3 ToPixel)
{
	float3 ToPixelAbs = abs(ToPixel);
	float Z = max(ToPixelAbs.x, max(ToPixelAbs.y, ToPixelAbs.z));
	float Depth = (LightPerspectiveValues.x * Z + LightPerspectiveValues.y) / Z;
	return PointShadowMapTexture.SampleCmpLevelZero(PCFSampler, ToPixel, Depth);
}

float3 CalcPointPBR(float3 position, Material mat)
{
    
    
    float3 diffusecolor = mat.diffuseColor.rgb;
    float3 normal = mat.normal.rgb;
    
    
    float3 lightDir = PointLightPos - position;
    float3 viewDir = EyePosition - position;
    
  
    float metallic = mat.Factors.y;
    float roughness = mat.Factors.x;
    
 
   
    float DistToLight = length(lightDir);
      
    lightDir /= DistToLight; // Normalize
    
    float3 H = normalize(viewDir + lightDir);
    //float3 R = normalize(reflect(-viewDir, mat.normal));
   
    float3 realAlbedo = RealAlbedo(diffusecolor, metallic);
    float3 realSpecularColor = RealSpecularColor(diffusecolor, metallic);
    
    float ndotl = NdotL(normal, lightDir);
    float ndoth = NdotH(normal, H);
    float ndotv = NdotV(normal, viewDir);
    float vdoth = VdotH(H, viewDir);
    float ldoth = LdotH(H, lightDir);
    
    float3 f0 = F0(diffusecolor, metallic);
   
  
    float alpha = Alpha(roughness);
      
 
    float3 diffuse = ndotl * Disney(ndotl, ldoth, ndotv, alpha) * PointColor.rgb;
    float3 specular = ndotl * GGX(ndotl, ndoth, ndotv, vdoth, ldoth, alpha, f0) * PointColor.rgb;
      
    float3 finalColor = realAlbedo * diffuse.rgb + (specular.rgb);
    finalColor = FinalGamma(finalColor);
       
    // Attenuation
    float DistToLightNorm = 1.0 - saturate(DistToLight / PointLightRangeRcp);
    float Attn = DistToLightNorm * DistToLightNorm;
    
    finalColor *= Attn;
    finalColor *= PointIntencity;
 
    return finalColor;
}


float4 PointLightCommonPS(DS_OUTPUT In, bool bUseShadow) : SV_TARGET
{
    Material mat = UnpackGBuffer(In.Position.xy);
   
    
	// Reconstruct the world position
	float3 position = CalcWorldPos(In.PositionXYW.xy / In.PositionXYW.z, mat.LinearDepth);
     [flatten]
    if (mat.TerrainMask == 0)
    {
        float3 lightDir = PointLightPos - position;
         
        float DistToLight = length(lightDir);
      
    
        float3 finalColor = mat.diffuseColor.rgb  * PointColor.rgb;
      //  finalColor = FinalGamma(finalColor);
       
    // Attenuation
       // float attenuation = max(0, 1.0f - (DistToLight / PointLightRangeRcp));
        float DistToLightNorm = 1.0 - saturate(DistToLight / PointLightRangeRcp);
        float Attn = DistToLightNorm * DistToLightNorm;
       
          
        finalColor *= Attn;
        finalColor *= PointIntencity;
       
        return float4(finalColor, 1.0f);
    }
      
	// Calculate the light contribution
    float3 finalColor = CalcPointPBR(position, mat);

	// return the final color
   
    return float4(finalColor * PointIntencity, 1.0);
}

float4 PointLightPS(DS_OUTPUT In) : SV_TARGET
{
	return PointLightCommonPS(In, false);
}

float4 PointLightShadowPS(DS_OUTPUT In) : SV_TARGET
{
	return PointLightCommonPS(In, true);
}