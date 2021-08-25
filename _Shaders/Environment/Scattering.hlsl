

static const float PI = 3.14159265f;
static const float InnerRadius = 6356.7523142f;
static const float OuterRadius = 6356.7523142f * 1.0157313f;
static const float KrESun = 0.0025f * 30.0f; //0.0025f - 레일리 상수 * 태양의 밝기
static const float KmESun = 0.0010f * 30.0f; //0.0025f - 미 상수 * 태양의 밝기
static const float Kr4PI = 0.0025f * 4.0f * PI;
static const float Km4PI = 0.0010f * 4.0f * PI;
static const float2 RayleighMieScaleHeight = { 0.25f, 0.1f };
static const float Scale = 1.0f / (6356.7523142f * 1.0157313f - 6356.7523142f);
static const int SampleCount = 8;
static const float3 WaveLength = float3(0.65f, 0.57f, 0.475f);
static const  float3 InvWaveLength = 1.0f / pow(WaveLength, 4.0f);
static const  float3 WaveLengthMie = pow(WaveLength, -0.84f);
static const float g = -0.980f;
static const float g2 = -0.980f * -0.980f;

SamplerState LinearSampler : register(s0);


cbuffer CB_World : register(b0)
{
    float4x4 WVP : packoffset(c0);
  
};


cbuffer CB_PS : register(b0)
{
    float3 LightDir : packoffset(c0);
    float Time : packoffset(c0.w);
};

Texture2D<float4> StarMap : register(t0);

struct VertexTexture
{
    float4 Position : Position;
    float2 Uv : Uv0;
};

struct VertexOutput_Dome
{
    float4 Position : SV_Position0;
    float3 oPosition : Position1;
    float2 Uv : Uv0;
};
float GetRayleighPhase(float c)
{
    return 0.75f * (1.0f + c);
}
float GetMiePhase(float c, float c2)
{
   
   
    float3 result = 0;
    result.x = 1.5f * ((1.0f - g2) / (2.0f + g2));
    result.y = 1.0f + g2;
    result.z = 2.0f * g;

    return result.x * (1.0f + c2) / pow(result.y - result.z * c, 1.5f);
}
static const float Exposure=-0.5f;
float3 HDR(float3 LDR)
{
    
  
    return 1.0f - exp(Exposure * LDR);
}
float HitOuterSphere(float3 position, float3 direction)
{
   
    float3 light = -position;

    float b = dot(light, direction);
    float c = dot(light, light);

    float d = c - b * b;
    float q = sqrt(OuterRadius * OuterRadius - d);

    return b + q;
}
float2 GetDensityRatio(float height)
{
   
    float altitude = (height - InnerRadius) * Scale;
    return exp(-altitude / RayleighMieScaleHeight);
}
float2 GetDistance(float3 p1, float3 p2)
{
  
  
    
    float2 opticalDepth = 0;

    float3 temp = p2 - p1;
    float far = length(temp);
    float3 direction = temp / far;


    float sampleLength = far / SampleCount;
    float scaledLength = sampleLength * Scale;

    float3 sampleRay = direction * sampleLength;
    p1 += sampleRay * 0.5f;
    
    [unroll(8)]
    for (int i = 0; i < 8; i++)
    {
        float height = length(p1);
        opticalDepth += GetDensityRatio(height);

        p1 += sampleRay;
    }

    return opticalDepth * scaledLength;
}
void CalcMieRay(inout float3 rayleigh, inout float3 mie, float2 uv)
{
    
  
  
    float3 pointPv = float3(0, InnerRadius + 1e-3f, 0.0f);
    float angleXZ = PI * uv.y;
    float angleY = 100.0f * uv.x * PI / 180.0f;

    float3 direction;
    direction.x = sin(angleY) * cos(angleXZ);
    direction.y = cos(angleY);
    direction.z = sin(angleY) * sin(angleXZ);
    direction = normalize(direction);

    float farPvPa = HitOuterSphere(pointPv, direction);
    float3 ray = direction;

    float3 pointP = pointPv;
    float sampleLength = farPvPa / SampleCount;
    float scaledLength = sampleLength * Scale;
    float3 sampleRay = ray * sampleLength;
    pointP += sampleRay * 0.5f;

    float3 rayleighSum = 0;
    float3 mieSum = 0;
    float3 attenuation = 0;
    
    
    
  
 
    float3 sunDirection = -normalize(LightDir);

    [unroll(8)]
    for (int i = 0; i < 8; i++)
    {
        float pHeight = length(pointP);

        float2 densityRatio = GetDensityRatio(pHeight);
        densityRatio *= scaledLength;


        float2 viewerOpticalDepth = GetDistance(pointP, pointPv);

        float farPPc = HitOuterSphere(pointP, sunDirection);
        float2 sunOpticalDepth = GetDistance(pointP, pointP + sunDirection * farPPc);

        float2 opticalDepthP = sunOpticalDepth.xy + viewerOpticalDepth.xy;
        attenuation = exp(-Kr4PI * InvWaveLength * opticalDepthP.x - Km4PI * opticalDepthP.y);

        rayleighSum += densityRatio.x * attenuation;
        mieSum += densityRatio.y * attenuation;

        pointP += sampleRay;
    }
  

    float3 rayleigh1 = rayleighSum * KrESun;
    float3 mie1 = mieSum * KmESun;

    
   
  
    rayleigh = rayleigh1 * InvWaveLength;
    mie = mie1 * WaveLengthMie;
   
   

}



VertexOutput_Dome VS_Dome(VertexTexture input)
{
    VertexOutput_Dome output;

    output.oPosition =-input.Position;
    output.Position = mul(input.Position, WVP);
   
    output.Uv = input.Uv;

    return output;
}
struct PS_GBUFFER_OUT
{
    float4 ColorSpecInt : SV_Target0;
    float4 Specular : SV_Target1;
    float4 Normal : SV_Target2;
};

PS_GBUFFER_OUT PackGBuffer(float3 BaseColor, float3 Normal, float metallic, float roughness, float terrainMask = 1)
{
    PS_GBUFFER_OUT Out;
    Out.ColorSpecInt = float4(BaseColor.rgb, terrainMask);
    Out.Specular = float4(roughness, metallic, 0.0, 0.0);
    Out.Normal = float4(Normal.rgb * 0.5 + 0.5, 0.0);
    return Out;
}
float4 PS_Dome(VertexOutput_Dome input) : SV_Target
{
    float3 sunDirection = -normalize(LightDir);

    float temp = dot(sunDirection, input.oPosition) / length(input.oPosition);
    float temp2 = temp * temp;

   
    float3 ray = 0;
    float3 mie = 0;
  
    CalcMieRay(ray, mie, input.Uv);

    float3 rSamples = ray;
    float3 mSamples = mie;

 
    float3 color = 0;
    color = GetRayleighPhase(temp2) * rSamples * 0.5f;
   
    color = HDR(color);
    color += GetMiePhase(temp, temp2) * mSamples * 0.5f;
    color += max(0, (1 - color.rgb)) * float3(0.05f, 0.05f, 0.1f);
    float3 finalColor = color + StarMap.Sample(LinearSampler, input.Uv).rgb * saturate(-sunDirection.y);

   
    return float4(finalColor.rgb, 1.0f);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////

//struct VertexOutput_Cloud
//{
//    float4 Position : SV_Position0;
//      float2 Uv : Uv0;
  
//};
//float Fade(float t)
//{
//    return t * t * t * (t * (t * 6.0 - 15.0) + 10.0);
//}
//Texture2D NoiseMap : register(t1);
//Texture2D CloudMap1 : register(t2);
//Texture2D CloudMap2 : register(t3);;
//static const float ONE = 0.00390625;
//static const float ONEHALF = 0.001953125;

//float Noise(float2 P)
//{
   
//    float2 Pi = ONE * floor(P) + ONEHALF;
//    float2 Pf = frac(P);

   
//    float2 grad00 = NoiseMap.Sample(LinearSampler, Pi).rg * 4.0 - 1.0;
//    float n00 = dot(grad00, Pf);

//    float2 grad10 = NoiseMap.Sample(LinearSampler, Pi + float2(ONE, 0.0)).rg * 4.0 - 1.0;
//    float n10 = dot(grad10, Pf - float2(1.0, 0.0));

//    float2 grad01 = NoiseMap.Sample(LinearSampler, Pi + float2(0.0, ONE)).rg * 4.0 - 1.0;
//    float n01 = dot(grad01, Pf - float2(0.0, 1.0));

//    float2 grad11 = NoiseMap.Sample(LinearSampler, Pi + float2(ONE, ONE)).rg * 4.0 - 1.0;
//    float n11 = dot(grad11, Pf - float2(1.0, 1.0));

//    float2 n_x = lerp(float2(n00, n01), float2(n10, n11), Fade(Pf.x));

//    float n_xy = lerp(n_x.x, n_x.y, Fade(Pf.y));

//    return n_xy;
//}

//float4 MakeCloudUseTexture(float2 uv)
//{
//    float2 sampleLocation;
//    float4 textureColor1;
//    float4 textureColor2;
//    float4 finalColor;
    

//    // Translate the position where we sample the pixel from using the first texture translation values.
//    sampleLocation.x = uv.x + FirstOffset.x;
//    sampleLocation.y = uv.y + FirstOffset.y;

//    // Sample the pixel color from the first cloud texture using the sampler at this texture coordinate location.
//    textureColor1 = Cloud1.Sample(LinearSampler, sampleLocation);
    
//    // Translate the position where we sample the pixel from using the second texture translation values.
//    sampleLocation.x = uv.x + SecondOffset.x;
//    sampleLocation.y = uv.y + SecondOffset.y;

//    // Sample the pixel color from the second cloud texture using the sampler at this texture coordinate location.
//    textureColor2 = Cloud2.Sample(LinearSampler, sampleLocation);

//    // Combine the two cloud textures evenly.
//    finalColor = lerp(textureColor1, textureColor2, 0.5f);

//    // Reduce brightness of the combined cloud textures by the input brightness value.
//    finalColor = finalColor * 0.3f;

//    return finalColor;
//}
//VertexOutput_Cloud VS_Cloud(VertexTexture input)
//{
//    VertexOutput_Cloud output;

//    output.Position = mul(input.Position, World);
//    output.Position = ViewProjection(output.Position);
  
  
//    output.Uv = (input.Uv * CloudTiles);
   
//    return output;
//}
//float4 PS_Cloud(VertexOutput_Cloud input) : SV_Target0
//{
//    //input.Uv = input.Uv * CloudTiles;

//    float n = Noise(input.Uv + Time * CloudSpeed);
//    float n2 = Noise(input.Uv * 2 + Time * CloudSpeed);
//    float n3 = Noise(input.Uv * 4 + Time * CloudSpeed);
//    float n4 = Noise(input.Uv * 8 + Time * CloudSpeed);
 
//    float nFinal = n + (n2 / 2) + (n3 / 4) + (n4 / 8);
 
//    float c = CloudCover - nFinal;
    
//    [branch]
//    if (c < 0) 
//        c = 0;

//    float density = 1.0 - pow(CloudSharpness, c);
//  // float4 color = density;
//    input.Uv = input.Uv + Time * CloudSpeed;
//    float4 color = MakeCloudUseTexture(input.Uv) + density;
//    return color;
//   // return PackGBuffer(color.rgb, float4(1, 1, 1, 1), float3(0, 0, 0), float3(0, 0, 0), -1, -1, 1);
//}

