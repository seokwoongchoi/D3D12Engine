#include "common.hlsl"
#include "PBR.hlsl"
Texture2DArray<float> CascadeShadowMapTexture : register( t4 );
Texture2D<float4> SsaoTexture : register(t5);
Texture2D PreintegratedFG : register(t6);
TextureCube skyIR : register(t7);
/////////////////////////////////////////////////////////////////////////////
// shader input/output structure
/////////////////////////////////////////////////////////////////////////////

cbuffer cbDirLight : register( b1 )
{

	float3 DirToLight			: packoffset( c0 );
    float Time                  : packoffset(c0.w);
	float3 DirLightColor		: packoffset( c1 );
	float4x4 ToShadowSpace		: packoffset( c2 );
	float4 ToCascadeOffsetX		: packoffset( c6 );
	float4 ToCascadeOffsetY		: packoffset( c7 );
	float4 ToCascadeScale		: packoffset( c8 );
}

static const float2 arrBasePos[4] = {
	float2(-1.0, 1.0),
	float2(1.0, 1.0),
	float2(-1.0, -1.0),
	float2(1.0, -1.0),
};
static const float2 arrUV[4] =
{
    float2(0.0, 0.0),
    float2(1.0, 0.0),
	float2(0.0, 1.0),
	float2(1.0, 1.0),
};

/////////////////////////////////////////////////////////////////////////////
// Vertex shader
/////////////////////////////////////////////////////////////////////////////
struct VS_OUTPUT
{
    float4 Position : SV_Position; // vertex position 
	float2 cpPos	: TEXCOORD0;
    float2 UV : TEXCOORD1;
};

VS_OUTPUT DirLightVS( uint VertexID : SV_VertexID )
{
	VS_OUTPUT Output;

	Output.Position = float4( arrBasePos[VertexID].xy, 0.0, 1.0);
	Output.cpPos = Output.Position.xy;
    Output.UV = arrUV[VertexID].xy;

	return Output;    
}

/////////////////////////////////////////////////////////////////////////////
// Pixel shaders
/////////////////////////////////////////////////////////////////////////////

static const float2 size = 1.0f / float2(1280.0f, 720.0f);

static const float2 offsets[] =
{
    float2(+2 * size.x, -2 * size.y), float2(+size.x, -2 * size.y), float2(0.0f, -2 * size.y), float2(-size.x, -2 * size.y), float2(-2 * size.x, -2 * size.y),
    float2(+2 * size.x, -size.y),     float2(+size.x, -size.y),     float2(0.0f, -size.y), float2(-size.x, -size.y), float2(-2 * size.x, -size.y),
    float2(+2 * size.x, 0.0f),        float2(+size.x, 0.0f),        float2(0.0f, 0.0f), float2(-size.x, 0.0f), float2(-2 * size.x, 0.0f),
    float2(+2 * size.x, +size.y),     float2(+size.x, +size.y),     float2(0.0f, +size.y), float2(-size.x, +size.y), float2(-2 * size.x, +size.y),
    float2(+2 * size.x, +2 * size.y), float2(+size.x, +2 * size.y), float2(0.0f, +2 * size.y), float2(-size.x, +2 * size.y), float2(-2 * size.x, +2 * size.y),

};
static const float weight[] =
{
    1, 1, 2, 1, 1,
    1, 2, 4, 2, 1,
    2, 4, 8, 4, 2,
    1, 2, 4, 2, 1,
    1, 1, 2, 1, 1,
};


void Gaussianblur(inout float totalweight, inout float sum, float3 UVD, float bestCascade)
{
    float3 uv = 0;
     [unroll(25)]
    for (int i = 0; i < 25; i++)
    {
        uv = float3(UVD.xy + offsets[i], bestCascade);
        totalweight += weight[i];

        sum += CascadeShadowMapTexture.SampleCmpLevelZero(PCFSampler, float3(uv), UVD.z).r * weight[i];
      
    }
}


float CascadedShadow(float3 position)
{
	// Transform the world position to shadow space
	float4 posShadowSpace = mul(float4(position, 1.0), ToShadowSpace);

	// Transform the shadow space position into each cascade position
	float4 posCascadeSpaceX = (ToCascadeOffsetX + posShadowSpace.xxxx) * ToCascadeScale;
	float4 posCascadeSpaceY = (ToCascadeOffsetY + posShadowSpace.yyyy) * ToCascadeScale;

	// Check which cascade we are in
	float4 inCascadeX = abs(posCascadeSpaceX) <= 1.0;
	float4 inCascadeY = abs(posCascadeSpaceY) <= 1.0;
	float4 inCascade = inCascadeX * inCascadeY;

	// Prepare a mask for the highest quality cascade the position is in
	float4 bestCascadeMask = inCascade;
	bestCascadeMask.yzw = (1.0 - bestCascadeMask.x) * bestCascadeMask.yzw;
	bestCascadeMask.zw = (1.0 - bestCascadeMask.y) * bestCascadeMask.zw;
	bestCascadeMask.w = (1.0 - bestCascadeMask.z) * bestCascadeMask.w;
	float bestCascade = dot(bestCascadeMask, float4(0.0, 1.0, 2.0, 3.0));

	// Pick the position in the selected cascade
	float3 UVD;
	UVD.x = dot(posCascadeSpaceX, bestCascadeMask);
	UVD.y = dot(posCascadeSpaceY, bestCascadeMask);
	UVD.z = posShadowSpace.z;

	// Convert to shadow map UV values
	UVD.xy = 0.5 * UVD.xy + 0.5;
	UVD.y = 1.0 - UVD.y;
   // Compute the hardware PCF value
    //float shadow = CascadeShadowMapTexture.SampleCmpLevelZero(PCFSampler, float3(UVD.xy, bestCascade), UVD.z);
    //shadow = saturate(shadow + 1.0 - any(bestCascadeMask));
    //return shadow;
	
   
    float sum = 0.0f;
    float totalweight = 0.0f;

    Gaussianblur(totalweight, sum, UVD, bestCascade);
  
    float factor = sum / totalweight;
  
    factor = saturate(factor + UVD.z);
  
  
   
    return factor;
}
float3 RadianceIBLIntegration(float NdotV, float roughness, float3 specular)
{
    float2 preintegratedFG = PreintegratedFG.Sample(LinearSampler, float2(roughness, NdotV)).rg;
    return specular * preintegratedFG.r + preintegratedFG.g;
}

float3 BRDFLUT(float NdotV, float roughness, float3 specular)
{
    float2 preintegratedFG = PreintegratedFG.Sample(LinearSampler, float2(max(NdotV, 0), roughness)).rg;
    return specular*preintegratedFG.r + preintegratedFG.g;
}

float3 IBL(Material material, float3 eye, float3 specular)
{
    float3 g_diffuse = pow(material.diffuseColor, 2.2f);
 
    float NdotV = saturate(dot(material.normal, eye));

    float3 reflectionVector = normalize(reflect(-eye, material.normal));
    float smoothness = 1.0f - material.Factors.x;
    float mipLevel = (1.0 - smoothness * smoothness) * 10.0f;
   
    float4 cs = skyIR.SampleLevel(LinearSampler, reflectionVector, mipLevel);
 
    float3 result = pow(cs.xyz, 2.2f) * RadianceIBLIntegration(NdotV, material.Factors.x, specular);

    float3 diffuseDominantDirection = material.normal;
    float diffuseLowMip = 9.6f;
    
    //float3 f0 = F0(material.diffuseColor, material.Factors.y);
    //float3 F_IBL = FresnelSchlickRoughness(1 - NdotV, f0, material.Factors.x);
    //float3 kD_IBL = (1.0f - F_IBL) * (1.0f - material.Factors.y);
    
    float3 diffuseImageLighting = skyIR.SampleLevel(LinearSampler, diffuseDominantDirection, diffuseLowMip).rgb;
   // diffuseImageLighting *= kD_IBL;
 //   textureLod(u_EnvironmentMap, diffuseDominantDirection, diffuseLowMip).rgb;
    diffuseImageLighting = pow(diffuseImageLighting, 2.2f);
    float3 final = (result) + (diffuseImageLighting) * g_diffuse;


    return final;
   // return (result * float3(0.74f, 0.59f, 0.247f)) + (diffuseImageLighting * float3(0.74f, 0.59f, 0.247f)) * g_diffuse;
}


float3 CalcDirectional(float3 position, Material mat)
{
    
   
    float3 diffusecolor = mat.diffuseColor.rgb;
    float3 normal = mat.normal.rgb;
    float ndotl = NdotL(normal, DirToLight);
  
    float metallic = mat.Factors.y;

    float3 viewDir = EyePosition - position;
   
    float3 H = normalize(viewDir - DirToLight);
    float3 realAlbedo = RealAlbedo(diffusecolor, metallic);
    float3 realSpecularColor = RealSpecularColor(diffusecolor, metallic);
 
  
    float ndoth = NdotH(normal, H);
  
    float ndotv = NdotV(normal, viewDir);
    float vdoth = VdotH(H, viewDir);
    float ldoth = LdotH(H, DirToLight);
    

    float3 f0 = F0(diffusecolor, metallic);
    float roughness = mat.Factors.x;
  
    float alpha = Alpha(roughness);
    
    float3 diffuse = ndotl * Disney(ndotl, ldoth, ndotv, alpha) * DirLightColor;
    float3 specular = ndotl * GGX(ndotl, ndoth, ndotv, vdoth, ldoth, alpha, f0);
    
   
    float3 ibl = IBL(mat, viewDir, realSpecularColor);
    float3 finalColor = (realAlbedo * diffuse.rgb) + (ibl + specular.rgb);
   
    finalColor = FinalGamma(finalColor);
   
    
     
   
    
    return float4(finalColor, 1.0f);
}


float3 GammaToLinear(float3 color)
{
    return float3(color.x * color.x, color.y * color.y, color.z * color.z);

}
float3 Ambient(float bumpY, float3 diffuse)
{
    
    float3 AmbientLowerColor = GammaToLinear(float3(0.4f, 0.4f, 0.4f));
  
   
    float3 AmbientUpperColor = GammaToLinear(float3(0.6f, 0.6f, 0.63f));
  
  
    float3 AmbientRange = AmbientUpperColor - AmbientLowerColor;
   
    // Normalize the vertical axis
    float up = bumpY * 0.5 + 0.5;

	// Calcualte the ambient light
    float3 ambient = AmbientLowerColor + up * AmbientRange;
    ambient *= diffuse;
    ambient *= AmbientUpperColor;
    
    return ambient;
}
float4 DirLightPS( VS_OUTPUT In) : SV_TARGET
{
    Material mat = UnpackGBuffer(In.Position.xy);
 
   // float pixelLength = length(eyeToPixel);
    [flatten]
    if (mat.SkyMask < 1)
    {
        return float4(mat.diffuseColor, 1.0);
    }
    
    float3 position = CalcWorldPos(In.cpPos, mat.LinearDepth);
    float3 eyeToPixel = position - EyePosition;
   
    float3 finalColor = lerp(mat.diffuseColor.rgb, CalcDirectional(position, mat), mat.TerrainMask);
   
    float ao = SsaoTexture.Sample(LinearSampler, In.UV).r;
    finalColor *= ao;
    finalColor += Ambient(mat.normal.y, finalColor.rgb);
    finalColor *= CascadedShadow(position);
    finalColor *= LightFactor(DirToLight.y);
    finalColor = ApplyFog(finalColor, EyePosition.y , eyeToPixel);
   

  
    return float4(finalColor, 1.0);
}
float4 DirLightNOAOPS(VS_OUTPUT In) : SV_TARGET
{
   float ao = SsaoTexture.GatherRed(LinearSampler, In.UV).r;
   return float4(ao, ao, ao, 1.0f);
    
   //Material mat = UnpackGBuffer(In.Position.xy);
  
   //float3 position = CalcWorldPos(In.cpPos, mat.LinearDepth);
  
  
   //float shadow = CascadedShadow(position);
   //float lightFactor = LightFactor(DirToLight.y);
   //[branch]
   //if (mat.TerrainMask == 0)
   //{
   //    return float4(mat.diffuseColor.rgb *  shadow * lightFactor, 1.0f);
   //}
   
   //float3 finalColor = CalcDirectional(position, mat) *  shadow * lightFactor;
  
  
  
    //// Apply the fog to the final color
    //float3 eyeToPixel = position - EyePosition;
    //finalColor = ApplyFog(finalColor, EyePosition.y, eyeToPixel);

 
  
   // return float4(finalColor.xyz, 1.0);
}


/////////////////////////////////////////////////////////////////////////////
// Debug cascades
/////////////////////////////////////////////////////////////////////////////

float4 CascadeShadowDebugPS( VS_OUTPUT In ) : SV_TARGET
{
	// Unpack the GBuffer
    Material mat = UnpackGBuffer(In.Position.xy);

	// Reconstruct the world position
    float3 position = CalcWorldPos(In.cpPos, mat.LinearDepth);

	// Transform the world position to shadow space
	float4 posShadowSpace = mul(float4(position, 1.0), ToShadowSpace);

	// Transform the shadow space position into each cascade position
	float4 posCascadeSpaceX = (ToCascadeOffsetX + posShadowSpace.xxxx) * ToCascadeScale;
	float4 posCascadeSpaceY = (ToCascadeOffsetY + posShadowSpace.yyyy) * ToCascadeScale;

	// Check which cascade we are in
	float4 inCascadeX = abs(posCascadeSpaceX) <= 1.0;
	float4 inCascadeY = abs(posCascadeSpaceY) <= 1.0;
	float4 inCascade = inCascadeX * inCascadeY;

	// Prepare a mask for the highest quality cascade the position is in
	float4 bestCascadeMask = inCascade;
	bestCascadeMask.yzw = (1.0 - bestCascadeMask.x) * bestCascadeMask.yzw;
	bestCascadeMask.zw = (1.0 - bestCascadeMask.y) * bestCascadeMask.zw;
	bestCascadeMask.w = (1.0 - bestCascadeMask.z) * bestCascadeMask.w;

	return 0.5 * bestCascadeMask;
}
struct BrushDesc 
{
    float3 Color; 
    uint Range;
    
    float3 Location;
    uint Shape;
};

cbuffer CB_TerrainBrush : register(b2)//struct로 선언하면 지역변수 cbuffer로 하면 전역으로 잡힌다.(쉐이더에서)
{
    BrushDesc TerrainBrush;
};

float3 GetBrushColor(float3 wPosition)
{
    [flatten]
    if (TerrainBrush.Shape == 0)
        return float3(0, 0, 0);
     [flatten]
    if (TerrainBrush.Shape == 1)
    {
        [flatten]
        if ((wPosition.x > (TerrainBrush.Location.x - TerrainBrush.Range)) &&
            (wPosition.x < (TerrainBrush.Location.x + TerrainBrush.Range)) &&
            (wPosition.z > (TerrainBrush.Location.z - TerrainBrush.Range)) &&
            (wPosition.z < (TerrainBrush.Location.z + TerrainBrush.Range)))
        {
            return TerrainBrush.Color;
        }
    }
     [flatten]
    if (TerrainBrush.Shape == 2)
    {
        float dx = wPosition.x - TerrainBrush.Location.x;
        float dz = wPosition.z - TerrainBrush.Location.z;

        float dist = sqrt(dx * dx + dz * dz);

           [flatten]
        if (dist <= TerrainBrush.Range)
            return TerrainBrush.Color;

    }
    return float3(0, 0, 0);
}

float4 RenderBrushPS(VS_OUTPUT In) : SV_TARGET
{
	// Unpack the GBuffer
    Material mat = UnpackGBuffer(In.Position.xy);

	// Reconstruct the world position
    float3 position = CalcWorldPos(In.cpPos, mat.LinearDepth);

    float3 finalColor = GetBrushColor(position);
    return float4(finalColor, 1.0f);
}