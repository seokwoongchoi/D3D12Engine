Texture2DArray<float> CascadeShadowMapTexture : register(t4);
SamplerComparisonState PCFSampler : register(s1);

cbuffer cbDirLight : register( b1 )
{

	float3 DirToLight			: packoffset( c0 );
	float3 DirLightColor		: packoffset( c1 );
	float4x4 ToShadowSpace		: packoffset( c2 );
	float4 ToCascadeOffsetX		: packoffset( c6 );
	float4 ToCascadeOffsetY		: packoffset( c7 );
	float4 ToCascadeScale		: packoffset( c8 );
}

static const float2 size = 1.0f / float2(1280, 720);

static const float2 offsets[] =
{
    float2(+2 * size.x, -2 * size.y), float2(+size.x, -2 * size.y), //float2(0.0f, -2 * size.y), float2(-size.x, -2 * size.y), float2(-2 * size.x, -2 * size.y),
    float2(+2 * size.x, -size.y),     float2(+size.x, -size.y),     //float2(0.0f, -size.y), float2(-size.x, -size.y), float2(-2 * size.x, -size.y),
    float2(+2 * size.x, 0.0f),        float2(+size.x, 0.0f),        //float2(0.0f, 0.0f), float2(-size.x, 0.0f), float2(-2 * size.x, 0.0f),
    float2(+2 * size.x, +size.y),     float2(+size.x, +size.y),     //float2(0.0f, +size.y), float2(-size.x, +size.y), float2(-2 * size.x, +size.y),
    float2(+2 * size.x, +2 * size.y), float2(+size.x, +2 * size.y), //float2(0.0f, +2 * size.y), float2(-size.x, +2 * size.y), float2(-2 * size.x, +2 * size.y),

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
     [unroll(10)]
    for (int i = 0; i < 10; i++)
    {
        uv = float3(UVD.xy + offsets[i], bestCascade);
        totalweight += weight[i];

       // sum += CascadeShadowMapTexture.GatherCmpRed(PCFSampler, float3(UVD.xy, bestCascade), UVD.z, offsets[i]))* weight[i];
        sum += CascadeShadowMapTexture.SampleCmpLevelZero(PCFSampler, float3(uv), UVD.z).r * weight[i];

    }
}
// Cascaded shadow calculation
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
   
	 
    float sum = 0.0f;
    float totalweight = 0.0f;

    float3 uv = 0.0f;
    
    Gaussianblur(totalweight, sum, UVD, bestCascade);
  
    float factor = sum / totalweight;
  
    factor = saturate(factor + UVD.z);
  
   
    return factor;
}