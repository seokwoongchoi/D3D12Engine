

static const float2 arrBasePos[4] =
{
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
struct VS_OUTPUT
{
    float4 Position : SV_Position; // vertex position 
    float2 cpPos : Uv0;
    float2 Uv : Uv1;
   
};

SamplerState LinearSampler : register(s0);

//-----------------------------------------------------------------------------------------
// Occlusion
//-----------------------------------------------------------------------------------------

Texture2D<float> DepthTex : register(t0);
Texture2D scatterTexture : register(t1);
RWTexture2D<float> OcclusionRW : register(u0);

cbuffer OcclusionConstants : register(b0)
{
    uint2 Res : packoffset(c0);
    float occlusionFlag : packoffset(c0.z);
}



[numthreads(720,1, 1)]
void Occlussion(uint3 dispatchThreadId : SV_DispatchThreadID)
{
    
    uint3 CurPixel = uint3(dispatchThreadId.x % Res.x, dispatchThreadId.x / Res.y, 0);

	// Skip out of bound pixels
    [branch]
	if(CurPixel.y < Res.y)
	{
		// Get the depth
		float curDepth = DepthTex.Load(CurPixel);

		// Flag anything closer than the sky as occlusion
        OcclusionRW[CurPixel.xy].x = curDepth > occlusionFlag;
     
    }
}

//-----------------------------------------------------------------------------------------
// Ray tracing
//-----------------------------------------------------------------------------------------

cbuffer RayTraceConstants : register(b0)
{
    float2 SunPos : packoffset(c0);
    float InitDecay : packoffset(c0.z);
    float DistDecay : packoffset(c0.w);
    float3 RayColor : packoffset(c1);
    float MaxDeltaLen : packoffset(c1.w);
}

Texture2D<float> OcclusionTex : register(t0);

VS_OUTPUT RayTraceVS( uint VertexID : SV_VertexID )
{
    VS_OUTPUT Output;

	Output.Position = float4(arrBasePos[VertexID].xy, 1.0, 1.0);
	Output.Uv = arrUV[VertexID].xy;

	return Output;    
}

static const int NUM_STEPS =128;
static const float NUM_DELTA = 1.0 / 127.0f;

float GetRayleighPhase(float c)
{
    return 0.75f * (1.0f + c);
}
static const float g = -0.980f;
static const float g2 = -0.980f * -0.980f;
float GetMiePhase(float c, float c2)
{
   
   
    float3 result = 0;
    result.x = 1.5f * ((1.0f - g2) / (2.0f + g2));
    result.y = 1.0f + g2;
    result.z = 2.0f * g;

    return result.x * (1.0f + c2) / pow(result.y - result.z * c, 1.5f);
}
float4 RayTracePS( VS_OUTPUT In ) : SV_Target0
{
 
   
//// Find the direction and distance to the sun
    float2 dirToSun = (SunPos - In.Uv);
   float lengthToSun = length(dirToSun);
 
    dirToSun /= lengthToSun;

   
   float deltaLen = min(MaxDeltaLen, lengthToSun * NUM_DELTA);
    float2 rayDelta = dirToSun*deltaLen ;

    

    float stepDecay=   DistDecay  * deltaLen;
  
   float decay = InitDecay;
 
   float rayIntensity = 0.0f;

   float2 rayOffset = float2(0.0, 0.0);

    float dir = dirToSun / 1000.0f * -1.0f;
    float temp = dot(-dir, -In.Uv) / length(-In.Uv);
    float temp2 = temp * temp;
    
   [unroll(NUM_STEPS)]
    for (int i = 0; i < NUM_STEPS; i++)
   {
       float2 sampPos = In.Uv + rayOffset;
        float fCurIntensity = OcclusionTex.Sample(LinearSampler, sampPos).r ;
       
      
        float inten = lerp(fCurIntensity , fCurIntensity * GetMiePhase(temp, temp2), rayOffset*2.0);
        rayIntensity += (fCurIntensity) * decay;
       
       
        rayOffset += rayDelta;
     
       decay = saturate(decay - stepDecay);
    }
    
 
    return float4(rayIntensity, 0.0, 0.0,0.0);
 
}



//-----------------------------------------------------------------------------------------
// Combine results
//-----------------------------------------------------------------------------------------
Texture2D<float> LightRaysTex : register(t0);

//static const float2 size = 1.0f / float2(1280, 720);

//static const float2 offsets[] =
//{
//    float2(+2 * size.x, -2 * size.y), float2(+size.x, -2 * size.y), float2(0.0f, -2 * size.y), float2(-size.x, -2 * size.y),float2(-2 * size.x, -2 * size.y),
//    float2(+2 * size.x, -size.y),     float2(+size.x, -size.y),     float2(0.0f, -size.y),     float2(-size.x, -size.y),    float2(-2 * size.x, -size.y),
//    float2(+2 * size.x, 0.0f),        float2(+size.x, 0.0f),        float2(0.0f, 0.0f),        float2(-size.x, 0.0f),       float2(-2 * size.x, 0.0f),
//    float2(+2 * size.x, +size.y),     float2(+size.x, +size.y),     float2(0.0f, +size.y),     float2(-size.x, +size.y),    float2(-2 * size.x, +size.y),
//    float2(+2 * size.x, +2 * size.y), float2(+size.x, +2 * size.y), float2(0.0f, +2 * size.y), float2(-size.x, +2 * size.y),float2(-2 * size.x, +2 * size.y),

//};
//static const float weight[] =
//{
//    1, 1, 2, 1, 1,
//    1, 2, 4, 2, 1,
//    2, 4, 8, 4, 2,
//    1, 2, 4, 2, 1,
//    1, 1, 2, 1, 1,
//};

//void Gaussianblur(inout float totalweight, inout float sum,float2 inputUV)
//{
//    float2 uv = 0;
//     [unroll(25)]
//    for (int i = 0; i < 25; i++)
//    {
//        uv = float2(inputUV.xy + offsets[i]);
//        totalweight += weight[i];

    
//        sum += LightRaysTex.Sample(LinearSampler, uv).r* weight[i];

//    }
//}


 
static const float Pi = 6.28318530718; // Pi*2
    
    // GAUSSIAN BLUR SETTINGS {{{
static const float Directions = 16.0; // BLUR DIRECTIONS (Default 16.0 - More is better but slower)
static const float Quality = 4.0; // BLUR QUALITY (Default 4.0 - More is better but slower)
static const float Size = 8.0; // BLUR SIZE (Radius)
    // GAUSSIAN BLUR SETTINGS }}}
   
static const float2 Radius = Size / float2(640.0f, 360.0f);

float4 CombinePS( VS_OUTPUT In ) : SV_Target0
{
    
     
	
    float Color = 0;
    // Blur calculations
    [unroll(Directions)]
    for (float d = 0.0; d < Pi; d += Pi / Directions)
    {
        [unroll(Quality)]
        for (float i = 1.0 / Quality; i <= 1.0; i += 1.0 / Quality)
        {
            Color += LightRaysTex.Sample(LinearSampler, In.Uv + float2(cos(d), sin(d)) * Radius * i).r;
        }
    }
    
    // Output to screen
    Color /= Quality * Directions - 15.0;
    //float4 temp = scatterTexture.Sample(LinearSampler, In.Uv );
    //Color += temp;
    return float4(RayColor * Color, 1.0f);
    
 //   float sum = 0.0f;
 //   float totalweight = 0.0f;
     
 //   Gaussianblur(totalweight, sum, In.Uv);
 //   float factor = sum / totalweight;

	//// Return the color scaled by the intensity
  
 //   //return float4(rayIntensity, 0, 0, 1);
 //   return float4(RayColor * saturate(factor), 1.0);
}

