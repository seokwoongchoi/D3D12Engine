/////////////////////////////////////////////////////////////////////////////
// GBuffer textures and Samplers
/////////////////////////////////////////////////////////////////////////////
Texture2D<float> DepthTexture         : register( t0 );
Texture2D<float4> ColorSpecIntTexture : register( t1 );
Texture2D<float4> NormalTexture       : register( t2 );
Texture2D<float2> FactorTexture       : register(t3);



/////////////////////////////////////////////////////////////////////////////
// Shadow sampler
/////////////////////////////////////////////////////////////////////////////


SamplerState LinearSampler : register(s0);
SamplerComparisonState PCFSampler : register( s1 );

/////////////////////////////////////////////////////////////////////////////
// constants
/////////////////////////////////////////////////////////////////////////////
cbuffer cbGBufferUnpack : register( b0 )
{
	float4 PerspectiveValues : packoffset( c0 );
	float4x4 ViewInv         : packoffset( c1 );
}

#define EyePosition (ViewInv[3].xyz)

static const float2 g_SpecPowerRange = { 10.0, 250.0 };

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
    
    float3 Factors;//x=roughness,y=metallic
    float TerrainMask;

    float3 normal;
    float LinearDepth;
     
   
};

Material UnpackGBuffer(int2 location)
{
    Material Out;
    int3 location3 = int3(location, 0);
  
    float4 baseColorSpecInt = ColorSpecIntTexture.Load(location3);
    Out.diffuseColor = float4(baseColorSpecInt.xyz, 1);
    Out.TerrainMask = baseColorSpecInt.w;
    
    float depth = DepthTexture.Load(location3).x;
    Out.LinearDepth = ConvertZToLinearDepth(depth);

  
    float2 Factors = FactorTexture.Load(location3).xy;
    Out.Factors.x = Factors.x;
    Out.Factors.y = Factors.y;
    
    //Out.roughness = Out.Factors.x;
    //Out.metallic = Out.Factors.y;
    
    float4 normal = NormalTexture.Load(location3);
    Out.SkyMask = normal.w;
    
    Out.normal = normalize(normal.xyz * 2.0 - 1.0);
    
   
    return Out;
}











/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////


cbuffer cbFog : register(b2)
{
    float3 FogColor : packoffset(c0);
    float FogStartDist : packoffset(c0.w);
    float3 FogHighlightColor : packoffset(c1);
    float FogGlobalDensity : packoffset(c1.w);
    float3 fogDirToLight : packoffset(c2);
    float FogHeightFalloff : packoffset(c2.w);
}

float3 ApplyFog(float3 originalColor, float eyePosY, float3 eyeToPixel,
float dist=0.0f,float density=1.0f,float height=0.0f)
{
    float pixelDist = length(eyeToPixel);
    float3 eyeToPixelNorm = eyeToPixel / pixelDist;

	// Find the fog staring distance to pixel distance
    float fogDist = max(pixelDist - (FogStartDist + dist), 0.0);

	// Distance based fog intensity
    float fogHeightDensityAtViewer = exp(-(FogHeightFalloff + height) * eyePosY);
    float fogDistInt = fogDist * fogHeightDensityAtViewer;

	// Height based fog intensity
    float eyeToPixelY = eyeToPixel.y * (fogDist / pixelDist);
    float t = (FogHeightFalloff + height) * eyeToPixelY;
    const float thresholdT = 0.01;
    float fogHeightInt = abs(t) > thresholdT ?
		(1.0 - exp(-t)) / t : 1.0;

    float fogDensity = FogGlobalDensity * density;
	// Combine both factors to get the final factor
    float fogFinalFactor = exp((-fogDensity * fogDistInt * fogHeightInt) );

	// Find the sun highlight and use it to blend the fog color
    float sunHighlightFactor = saturate(dot(eyeToPixelNorm, fogDirToLight));
    sunHighlightFactor = pow(sunHighlightFactor, 8.0);
    float3 fogFinalColor = lerp(FogColor, FogHighlightColor, sunHighlightFactor);

    return lerp(fogFinalColor, originalColor, fogFinalFactor);
}

