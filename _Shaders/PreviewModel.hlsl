#include "Lighting/PBR.hlsl"

struct VertexSkeletal
{
    float4 Position : Position0;
    float2 Uv : Uv0;
    float3 Normal : Normal0;
    float3 Tangent : Tangent0;
	float4 BlendIndices : BlendIndices0;
	float4 BlendWeights : BlendWeights0;

 
   };

struct VertexStatic
{
    float4 Position : Position0;
    float2 Uv : Uv0;
    float3 Normal : Normal0;
    float3 Tangent : Tangent0;
    

};

struct VertexModelOutput
{
    float4 Position : SV_Position0;
    float2 Uv : Uv0;
    float3 Normal : Normal0;
    float3 Tangent : Tangent0;
   
};

cbuffer CB_WVP : register(b0)
{
    matrix W : packoffset(c0);
    matrix VP : packoffset(c4);
 
};

cbuffer CB_EyePosition : register(b1)
{
    float3 EyePosition : packoffset(c0);
};


cbuffer CB_Bone : register(b2)
{
    uint BoneIndex: packoffset(c0);
};


cbuffer CB_Material : register(b0)
{
    float4 DiffuseFactor : packoffset(c0);
    float4 SpecularFactor : packoffset(c1);
};
SamplerState LinearSampler : register(s0);


Texture2D DiffuseMap : register(t0);
Texture2D NormalMap : register(t1);
Texture2D RoughnessMap : register(t2);
Texture2D MatallicMap : register(t3);

Texture2D PreintegratedFG : register(t4);
TextureCube skyIR : register(t5);


///////////////////////////////////////////////////////////////////////////////
Texture2D<float4> BoneTransforms : register(t0);
Texture2DArray<float4> AnimBoneTransforms : register(t1);

matrix SetModelWorld()
{
    
    float4 m1 = BoneTransforms[int2(BoneIndex * 4 + 0,0)];
    float4 m2 = BoneTransforms[int2(BoneIndex * 4 + 1,0)];
    float4 m3 = BoneTransforms[int2(BoneIndex * 4 + 2,0)];
    float4 m4 = BoneTransforms[int2(BoneIndex * 4 + 3,0)];
    
    return  mul(matrix(m1, m2, m3, m4), W);
}


VertexModelOutput StaticVS(VertexStatic input)
{
    VertexModelOutput output;

    matrix World=SetModelWorld();
  
    output.Position = mul(input.Position, World);
    output.Position = mul(output.Position,VP);
    output.Normal = mul(input.Normal, (float3x3) World);
    output.Tangent = mul(input.Tangent, (float3x3) World);
    output.Uv = input.Uv;
    
 
    return output;
}

struct Material
{
    
    float4 diffuseColor;
    float3 normal;
 

     
    float roughness;
    float metallic;
};
void GetBumpMapCoord(float2 uv, float3 normal, float3 tangent, inout float3 bump)
{
    
   
    float3 normalMap = NormalMap.Sample(LinearSampler, uv);
    [branch]
    if (any(normalMap) == false)
        return;
   
    float3 N = normalize(normal); //Z
    float3 T = normalize(tangent - dot(tangent, N) * N); //X
    float3 BiTan = cross(N, T); //Y
    float3x3 TBN = float3x3(T, BiTan, N);

    //이미지로부터 노멀벡터 가져오기
    float3 coord = normalMap * 2.0f - 1.0f;

    //탄젠트 공간으로 변환
    coord = mul(coord, TBN);

    bump = coord;
}
float3 RadianceIBLIntegration(float NdotV, float roughness, float3 specular)
{
    float2 preintegratedFG = PreintegratedFG.Sample(LinearSampler, float2(roughness, NdotV)).rg;
    return specular * preintegratedFG.r + preintegratedFG.g;
}

float3 IBL(Material material, float3 eye, float3 KS)
{
    float3 g_diffuse = pow(material.diffuseColor, 2.2f);
 
    float NdotV = saturate(dot(material.normal, eye));

    float3 reflectionVector = normalize(reflect(-eye, material.normal));
    float smoothness = 1.0f - material.roughness;
    float mipLevel = (1.0 - smoothness * smoothness) * 10.0f;
   
    float4 cs = skyIR.SampleLevel(LinearSampler, reflectionVector, mipLevel);
 
    float3 result = pow(cs.xyz, 2.2f) * RadianceIBLIntegration(NdotV, material.roughness, KS);

    float3 diffuseDominantDirection = material.normal;
    float diffuseLowMip = 9.6f;
    float3 diffuseImageLighting = skyIR.SampleLevel(LinearSampler, diffuseDominantDirection, diffuseLowMip).rgb;

 //   textureLod(u_EnvironmentMap, diffuseDominantDirection, diffuseLowMip).rgb;
    diffuseImageLighting = pow(diffuseImageLighting, 2.2f);
    float3 final = (result) + (diffuseImageLighting) * g_diffuse;

    return final;
   // return (result * float3(0.74f, 0.59f, 0.247f)) + (diffuseImageLighting * float3(0.74f, 0.59f, 0.247f)) * g_diffuse;
}


float3 CalcDirectional(float3 position, Material mat, bool bUseShadow)
{
   
    float3 lightDir = -float3(1, -1, 1);
  
    float3 diffusecolor = mat.diffuseColor.rgb;
  
    float3 normal = mat.normal.rgb;
    float ndotl = NdotL(normal, lightDir);
  
    float metallic = mat.metallic;
  //calc the half vector
    float3 viewDir = EyePosition.xyz - position;
   
    float3 H = normalize(viewDir - lightDir);
    float3 realAlbedo = RealAlbedo(diffusecolor, metallic);
    float3 realSpecularColor = RealSpecularColor(diffusecolor, metallic);
 
  
    float ndoth = NdotH(normal, H);
  
    float ndotv = NdotV(normal, viewDir);
    float vdoth = VdotH(H, viewDir);
    float ldoth = LdotH(H, lightDir);
    

    float3 f0 = F0(diffusecolor, metallic);
    float roughness = mat.roughness;
   
    float alpha = Alpha(roughness);
    
    float3 diffuse = ndotl * Disney(ndotl, ldoth, ndotv, alpha) ;
    float3 specular = ndotl * GGX(ndotl, ndoth, ndotv, vdoth, ldoth, alpha, f0);
    
    float3 ibl = IBL(mat, viewDir, realSpecularColor);
    float3 finalColor = (realAlbedo * diffuse.rgb) + (ibl + specular.rgb);
   
    finalColor = FinalGamma(finalColor);
    
    
    return float4(finalColor, 1.0f);
}

float4 StaticPS(VertexModelOutput In) : SV_Target
{
    //return float4(1,0,0,1);
    float3 DiffuseColor = DiffuseMap.Sample(LinearSampler, In.Uv);
    float metallic = MatallicMap.GatherRed(LinearSampler, In.Uv).r;
    float roughness = RoughnessMap.GatherRed(LinearSampler, In.Uv).r;

    float3 bump = 0;
    GetBumpMapCoord(In.Uv, In.Normal, In.Tangent, bump);
    
    
    DiffuseColor *= DiffuseFactor.xyz;
      
    
    roughness *= DiffuseFactor.w;
    metallic *= SpecularFactor.w;
    
    Material mat;
    mat.normal = normalize(In.Normal);
    mat.diffuseColor.xyz = DiffuseColor.xyz;
    mat.diffuseColor.w = 1.0; // Fully opaque
   
    mat.roughness = roughness;
    mat.metallic = metallic;
	    
    float3 finalColor = CalcDirectional(In.Position.xyz, mat, 0);
    return float4(finalColor.rgb, 1.0f);

}


///////////////////////////////////////////////////////////////////////////////
//struct Keyframe
//{
//    int Clip;
//    uint CurrFrame;
//    uint NextFrame;

//    float Time;
//    float RunningTime;

//    float3 Padding;
//};

//struct TweenFrame
//{
//    float TakeTime;
//    float TweenTime;
//    float2 Padding;

//    Keyframe Curr;
//    Keyframe Next;

   
//};
//cbuffer CB_AnimationFrame : register(b5)
//{
//    TweenFrame Tweenframes[MAX_MODEL_INSTANCE];


//};

//float4 BoneIndeces(float4 BlendIndices)
//{
    
//    float4 indices = BlendIndices;
//    indices.x = lerp(BoneIndex, indices.x, any(BlendIndices));
//     return indices;
//}
//float4 BoneWeights(float4 BlendWeights)
//{
    
//    float4 weights = BlendWeights;
//    weights.x = lerp(1.0f, weights.x, any(BlendWeights));
//    return weights;
//}
        
//matrix LoadAnimTransforms(uint boneIndices, uint frame, uint clip)
//{
//    float4 c0, c1, c2, c3;
//    c0  =BoneTransforms.Load(uint4( boneIndices * 4 + 0, frame, clip, 0));
//    c1 = BoneTransforms.Load(uint4( boneIndices * 4 + 1, frame, clip, 0));
//    c2 = BoneTransforms.Load(uint4( boneIndices * 4 + 2, frame, clip, 0));
//    c3 = BoneTransforms.Load(uint4( boneIndices * 4 + 3, frame, clip, 0));
    
//    return matrix(c0, c1, c2, c3);
//}

//matrix GetCurrAnimTransforms(uint boneIndices, uint InstID)
//{
   
//    matrix curr = LoadAnimTransforms(boneIndices, Tweenframes[InstID].Curr.CurrFrame, Tweenframes[InstID].Curr.Clip);
//    matrix next = LoadAnimTransforms(boneIndices, Tweenframes[InstID].Curr.NextFrame, Tweenframes[InstID].Curr.Clip);
//    return lerp(curr, next,  Tweenframes[InstID].Curr.Time);
//}

//matrix GetNextAnimTransforms(uint boneIndices, uint InstID)
//{
      
//    matrix curr = LoadAnimTransforms(boneIndices, Tweenframes[InstID].Next.CurrFrame, Tweenframes[InstID].Next.Clip);
//    matrix next = LoadAnimTransforms(boneIndices, Tweenframes[InstID].Next.NextFrame, Tweenframes[InstID].Next.Clip);
//    return lerp(curr, next, (matrix) Tweenframes[InstID].Next.Time);
    
//}

//matrix LerpCurrAnim_NextAnimTransform(uint boneIndices,  uint InstID)
//{
   
    
//    matrix curr = GetCurrAnimTransforms(boneIndices,  InstID);
//    matrix next = GetNextAnimTransforms(boneIndices,  InstID);
//    return lerp(curr, next, Tweenframes[InstID].TweenTime);
  
//}

//void SetAnimationWorld(inout matrix world, VertexSkeletal input)
//{
//   matrix transform = 0;
//   matrix Anim = 0;
//   matrix nextAnim = 0;
//   float4 boneIndices = BoneIndeces(input.BlendIndices);
//   float4 boneWeights = BoneWeights(input.BlendWeights);
//   uint instId = input.InstID;
       
//    [unroll(4)]
//    for (int i = 0; i < 4; i++)
//    {
//         Anim =
//        lerp(
//        GetCurrAnimTransforms(boneIndices[i], instId), LerpCurrAnim_NextAnimTransform(boneIndices[i], instId),
//        saturate(Tweenframes[instId].Next.Clip + 1)
//        );
       
//        transform += mul(boneWeights[i], Anim);
//    }
     
//    world = mul(transform, InstTransform[instId]);
//}

//VertexModelOutput VS_Animation(VertexSkeletal input)
//{
//    VertexModelOutput output;
//	SetAnimationWorld(World, input);
//  	output.Position = WorldViewProjection(input.Position);
//  	output.Normal = WorldNormal(input.Normal);
//	output.Tangent = WorldTangent(input.Tangent);
//	output.Uv = input.Uv;
//    //output.Cull.x = dot(float4(output.wPosition.xyz - ViewPosition(), 1.0f), -g_FrustumNormals[0]);
//   //output.Cull.y = dot(float4(output.wPosition.xyz - ViewPosition(), 1.0f), -g_FrustumNormals[1]);
//   //output.Cull.z = dot(float4(output.wPosition.xyz - ViewPosition(), 1.0f), -g_FrustumNormals[2]);
//   //output.Cull.w = 0;
//  	return output;
//}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//float4 PS(VertexModelOutput In):SV_Target0
//{
    
    
//    return DiffuseMap.Sample(LinearSampler, In.Uv);
//}

