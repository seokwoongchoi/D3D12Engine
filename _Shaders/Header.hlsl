#define MAX_MODEL_INSTANCE 20

cbuffer CB_World : register(b0)
{
   static matrix World : packoffset(c0);
};

cbuffer CB_View : register(b1)
{
    matrix VP : packoffset(c0);
    //float4 EyePos : packoffset(c4);
  
 
};

cbuffer CB_Bone : register(b2)
{
    uint BoneIndex : packoffset(c0.x);
    uint actorIndex : packoffset(c0.y);
    uint drawCount : packoffset(c0.z);
    uint prevDrawCount : packoffset(c0.w);
};


cbuffer CB_Material : register(b0)
{
    float4 DiffuseFactor : packoffset(c0);
    float4 LightDir : packoffset(c1);
   
};
SamplerState LinearSampler : register(s0);


Texture2D DiffuseMap : register(t0);
Texture2D NormalMap : register(t1);
Texture2D RoughnessMap : register(t2);
Texture2D MatallicMap : register(t3);




struct VertexModelOutput
{
    float4 Position : SV_Position0;
    float2 Uv : Uv0;
    float3 Normal : Normal0;
    float3 Tangent : Tangent0;
   
   // float4 Cull : SV_CullDistance0;
};

struct VertexModelParallaxMappingOutput
{
    float4 Position : SV_Position0;
    float2 Uv : Uv0;
    
    float3 Normal : Normal0;
    float3 Tangent : Tangent0;
    float3 ViewWorldSpace : View;
  
   
   // float4 Cull : SV_CullDistance0;
};

struct VertexModelOutputReflection
{
    float4 Position : SV_Position0;
    float2 Uv : Uv0;
    float3 Normal : Normal0;
    float3 Tangent : Tangent0;
   
    float4 Cull : SV_CullDistance0;
};
Texture2D<float4> InstTransforms : register(t0);

///////////////////////////////////////////////////////////////////////////////


float4 WorldPosition(float4 position)
{
    return mul(position, World);
}


float4 ViewProjection(float4 position)
{
    
    return mul(position, VP);
}


float3 WorldNormal(float3 normal)
{
    return mul(normal, (float3x3) World);
}

float3 WorldTangent(float3 tangent)
{
    return mul(tangent, (float3x3) World);
}



///////////////////////////////////////////////////////////////////////////////

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
    Out.Normal = float4(normalize(Normal.rgb * 0.5 + 0.5), 1.0);
    return Out;
}

void GetBumpMapCoord(float2 uv, float3 normal, float3 tangent, inout float3 bump)
{
  

    float3 normalMap = NormalMap.Sample(LinearSampler, uv).rgb;
   
    [flatten]
    if (any(normalMap)==false)
    {
        return;
    }
    // Expand the range of the normal value from (0, +1) to (-1, +1).
    normalMap = normalize(normalMap * 2.0f - 1.0f);
    float3 N = normal; //Z
    float3 T = normalize(tangent - dot(tangent, N) * N); //X
    float3 BiNor = cross(N, T); //Y
    // Calculate the normal from the data in the bump map.
    float3x3 TBN = float3x3(T, BiNor, N);
   
    bump = mul(normalMap, TBN);
	
    // Normalize the resulting bump normal.
   
 
}

PS_GBUFFER_OUT PS(VertexModelOutput In)
{
  
    float3 DiffuseColor = DiffuseMap.Sample(LinearSampler, In.Uv).rgb;
   
    float metallic = MatallicMap.Sample(LinearSampler, In.Uv).r;
    float roughness = RoughnessMap.Sample(LinearSampler, In.Uv).r;
   
 
    float3 bump = 0;
    GetBumpMapCoord(In.Uv, normalize(In.Normal), normalize(In.Tangent), bump);
       
    DiffuseColor *= DiffuseFactor.xyz;
 
    roughness *= DiffuseFactor.w;
    metallic *= LightDir.w;
    
     
    
    return PackGBuffer(DiffuseColor, normalize(bump), metallic, roughness);
}
PS_GBUFFER_OUT TreePS(VertexModelOutput In)
{
  
    float4 DiffuseColor = DiffuseMap.Sample(LinearSampler, In.Uv);
   
    float metallic = MatallicMap.Sample(LinearSampler, In.Uv).r;
    float roughness = RoughnessMap.Sample(LinearSampler, In.Uv).r;
   
 
    float3 bump = 0;
    GetBumpMapCoord(In.Uv, normalize(In.Normal), normalize(In.Tangent), bump);
       
    DiffuseColor.xyz *= DiffuseFactor.xyz;
 
    roughness *= DiffuseFactor.w;
    metallic *= LightDir.w;
    
    //   [flatten]
    //if (DiffuseColor.r < 0.01f)
    //    discard;
    PS_GBUFFER_OUT Out;
    Out.ColorSpecInt = DiffuseColor;
    Out.Specular = float4(roughness, metallic, 0.0, 0.0);
    Out.Normal = float4(normalize(bump.rgb * 0.5 + 0.5), 1.0);
    return Out;
    //return PackGBuffer(DiffuseColor, normalize(bump), metallic, roughness);
}


float4 ReflectionPS(VertexModelOutput In) : SV_Target
{
  
  
    float3 DiffuseColor = DiffuseMap.Sample(LinearSampler, In.Uv).rgb;
    float3 bump = 0;
    GetBumpMapCoord(In.Uv, In.Normal, In.Tangent, bump);
    DiffuseColor *= DiffuseFactor.xyz;
    
     
    float3 lightDir = -LightDir.xyz;
    float ndotl = dot(bump, lightDir);
    
    float3 finalColor = DiffuseColor * ndotl;
    return float4(finalColor, 1.0f);
}


float4 ForwardPS(VertexModelOutput In) : SV_Target
{
 
    float3 DiffuseColor = DiffuseMap.Sample(LinearSampler, In.Uv).rgb;
    float3 bump = 0;
    GetBumpMapCoord(In.Uv, In.Normal, In.Tangent, bump);
    DiffuseColor *= DiffuseFactor.xyz;
    
     
    float3 lightDir = -LightDir.xyz;
    float ndotl = dot(bump, lightDir);
    
    float3 finalColor = DiffuseColor * ndotl;
    return float4(DiffuseColor, 1.0f);
}



////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//PS_GBUFFER_OUT ParallaxMappingPS(VertexModelParallaxMappingOutput In)
//{
   
   
//    float3 N = normalize(In.Normal); //Z
//    float3 T = normalize(In.Tangent - dot(In.Tangent, N) * N); //X
//    float3 BiNor = cross(N, T); //Y
  
//    float3x3 TBN = float3x3(T, BiNor, N);
   
//   // float3 ViewTangentSpace = normalize(mul(In.ViewWorldSpace, TBN));
    
//    float3 vViewWS = normalize(In.ViewWorldSpace);
//    float3 viewTS = mul(TBN, vViewWS);
     
//    float2 ParallaxDirection = normalize(viewTS.xy);
    
//    float fLength = length(viewTS);
//    float fParallaxLength = sqrt(fLength * fLength - viewTS.z * viewTS.z) / viewTS.z;
       
//    // Compute the actual reverse parallax displacement vector:
//    float2 ParallaxOffsetTS = ParallaxDirection * fParallaxLength;
//    //ParallaxOffsetTS *= 5.0f;
    
//    float2 fTexCoordsPerSize = In.Uv / float2(1280, 720);

  
//    float2 dxSize, dySize;
//    float2 dx, dy;

//    float4(dxSize, dx) = ddx(float4(fTexCoordsPerSize, In.Uv));
//    float4(dySize, dy) = ddy(float4(fTexCoordsPerSize, In.Uv));
                  
//    float fMipLevel;
//    float fMipLevelInt;
//    float fMipLevelFrac;

//    float fMinTexCoordDelta;
//    float2 dTexCoords;

  
//    dTexCoords = dxSize * dxSize + dySize * dySize;
//    fMinTexCoordDelta = max(dTexCoords.x, dTexCoords.y);
//    fMipLevel = max(0.5 * log2(fMinTexCoordDelta), 0);
    
//    float2 texSample = In.Uv;
   
//    float4 cLODColoring = float4(1, 1, 3, 1);

//    float fOcclusionShadow = 1.0;
//    float g_nLODThreshold = 3;
//    int g_nMaxSamples = 8;
//    int g_nMinSamples = 50;
//    bool g_bVisualizeLOD = true;
//    if (fMipLevel <= (float) g_nLODThreshold)
//    {
     
//        int nNumSteps = (int) lerp(g_nMaxSamples, g_nMinSamples, dot(vViewWS, In.Normal));

//        float fCurrHeight = 0.0;
//        float fStepSize = 1.0 / (float) nNumSteps;
//        float fPrevHeight = 1.0;
//        float fNextHeight = 0.0;

//        int nStepIndex = 0;
//        bool bCondition = true;

//        float2 vTexOffsetPerStep = fStepSize * ParallaxOffsetTS;
//        float2 vTexCurrentOffset = In.Uv;
//        float fCurrentBound = 1.0;
//        float fParallaxAmount = 0.0;

//        float2 pt1 = 0;
//        float2 pt2 = 0;
       
//        float2 texOffset2 = 0;

//        while (nStepIndex < nNumSteps)
//        {
//            vTexCurrentOffset -= vTexOffsetPerStep;

        
//            fCurrHeight = NormalMap.SampleGrad(LinearSampler, vTexCurrentOffset, dx, dy).a;
//           // tex2Dgrad(NormalMap, vTexCurrentOffset, dx, dy).r;

//            fCurrentBound -= fStepSize;

//            if (fCurrHeight > fCurrentBound)
//            {
//                pt1 = float2(fCurrentBound, fCurrHeight);
//                pt2 = float2(fCurrentBound + fStepSize, fPrevHeight);

//                texOffset2 = vTexCurrentOffset - vTexOffsetPerStep;

//                nStepIndex = nNumSteps + 1;
//                fPrevHeight = fCurrHeight;
//            }
//            else
//            {
//                nStepIndex++;
//                fPrevHeight = fCurrHeight;
//            }
//        }

//        float fDelta2 = pt2.x - pt2.y;
//        float fDelta1 = pt1.x - pt1.y;
      
//        float fDenominator = fDelta2 - fDelta1;
     
//        if (fDenominator == 0.0f)
//        {
//            fParallaxAmount = 0.0f;
//        }
//        else
//        {
//            fParallaxAmount = (pt1.x * fDelta2 - pt2.x * fDelta1) / fDenominator;
//        }
      
//        float2 vParallaxOffset = ParallaxOffsetTS * (1 - fParallaxAmount);

//        float2 texSampleBase = In.Uv - vParallaxOffset;
//        texSample = texSampleBase;

        
     

//        if (fMipLevel > (float) (g_nLODThreshold - 1))
//        {
        
//            fMipLevelFrac = modf(fMipLevel, fMipLevelInt);

//            texSample = lerp(texSampleBase, In.Uv, fMipLevelFrac);

//        }
      
      
//    }
  
  
//   // float3 DiffuseColor = ComputeIllumination(texSample, vLightTS, ViewTS, 1.0f).rgb;
//    float3 DiffuseColor = DiffuseMap.Sample(LinearSampler, texSample).rgb;
//    float metallic = MatallicMap.Sample(LinearSampler, texSample).r;
//    float roughness = RoughnessMap.Sample(LinearSampler, texSample).r;
    
   
//    float3 normal = normalize(NormalMap.Sample(LinearSampler, texSample).rgb * 2.0f - 1.0f);
//    normal = mul(normal, TBN);
       
//    DiffuseColor *= DiffuseFactor.xyz;
 
//    roughness *= DiffuseFactor.w;
//    metallic *= LightDir.w;
    
//    PS_GBUFFER_OUT Out;
//    Out.ColorSpecInt = float4(DiffuseColor.rgb, 1.0);
//    Out.Specular = float4(roughness, metallic, 0.0, 0.0);
//    Out.Normal = float4(normalize(normal.rgb * 0.5 + 0.5), 1.0);
//    return Out;
  
   
//}


//PS_GBUFFER_OUT ParallaxMappingPS(VertexModelParallaxMappingOutput In)
//{
   
   
//    float3 ViewTangentSpace = normalize(In.ViewWorldSpace);
    
//    float fCurrentHeight = normalize(NormalMap.Sample(LinearSampler, In.Uv).a);
    
//    float fHeight = fCurrentHeight  + 0.01f;
     
//    fHeight /= ViewTangentSpace.z;
//    float2 texSample = In.Uv + ViewTangentSpace.xy * fHeight;
   
    
  
//   // float3 DiffuseColor = ComputeIllumination(texSample, vLightTS, ViewTS, 1.0f).rgb;
//    float3 DiffuseColor = DiffuseMap.Sample(LinearSampler, texSample).rgb;
//    float metallic = MatallicMap.Sample(LinearSampler, texSample).r;
//    float roughness = RoughnessMap.Sample(LinearSampler, texSample).r;
    
   
//    float3 normal = normalize(NormalMap.Sample(LinearSampler, texSample).rgb * 2.0f - 1.0f);
          
//    DiffuseColor *= DiffuseFactor.xyz;
 
//    roughness *= DiffuseFactor.w;
//    metallic *= LightDir.w;
    
//    PS_GBUFFER_OUT Out;
//    Out.ColorSpecInt = float4(DiffuseColor.rgb, 1.0);
//    Out.Specular = float4(roughness, metallic, 0.0, 0.0);
//    Out.Normal = float4(normalize(normal.rgb * 0.5 + 0.5), 1.0);
//    return Out;
   
//}
