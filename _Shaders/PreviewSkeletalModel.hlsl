
#include "PreviewHeader.hlsl"
#include "000_Math.hlsl"
struct VertexSkeletal
{
    float4 Position : Position0;
    float2 Uv : Uv0;
    float3 Normal : Normal0;
    float3 Tangent : Tangent0;
	float4 BlendIndices : BlendIndices0;
	float4 BlendWeights : BlendWeights0;

 
   };

//
struct Keyframe
{
    int Clip;
    uint CurrFrame;
    uint NextFrame;
    float Time;
  
    float4 padding1;
};

struct TweenFrame
{
    float TakeTime;
    float TweenTime;
    float2 Padding2;

    Keyframe Curr;
    Keyframe Next;

   
};
cbuffer CB_AnimationFrame : register(b3)
{
    TweenFrame Tweenframes;
    

};

Texture2DArray<float4> AnimBoneTransforms : register(t0);
float4 BoneIndeces(float4 BlendIndices)
{
    
    float4 indices = BlendIndices;
    indices.x = lerp(BoneIndex, indices.x, any(BlendIndices));
    return indices;
}
float4 BoneWeights(float4 BlendWeights)
{
    
    float4 weights = BlendWeights;
    weights.x = lerp(1.0f, weights.x, any(BlendWeights));
    return weights;
}
        
matrix LoadAnimTransforms(uint boneIndices, uint frame, uint clip)
{
    float4 c0, c1, c2, c3;
    c0 = AnimBoneTransforms.Load(uint4(boneIndices * 4 + 0, frame, clip, 0));
    c1 = AnimBoneTransforms.Load(uint4(boneIndices * 4 + 1, frame, clip, 0));
    c2 = AnimBoneTransforms.Load(uint4(boneIndices * 4 + 2, frame, clip, 0));
    c3 = AnimBoneTransforms.Load(uint4(boneIndices * 4 + 3, frame, clip, 0));
    
    return matrix(c0, c1, c2, c3);
}

matrix GetCurrAnimTransforms(uint boneIndices)
{
   
    matrix curr = LoadAnimTransforms(boneIndices, Tweenframes.Curr.CurrFrame, Tweenframes.Curr.Clip);
    matrix next = LoadAnimTransforms(boneIndices, Tweenframes.Curr.NextFrame, Tweenframes.Curr.Clip);
 
    return lerp(curr, next, Tweenframes.Curr.Time);
}

matrix GetNextAnimTransforms(uint boneIndices)
{
      
    matrix curr = LoadAnimTransforms(boneIndices, Tweenframes.Next.CurrFrame, Tweenframes.Next.Clip);
    matrix next = LoadAnimTransforms(boneIndices, Tweenframes.Next.NextFrame, Tweenframes.Next.Clip);
    return lerp(curr, next,  Tweenframes.Next.Time);
    
}

//matrix LerpCurrAnim_NextAnimTransform(uint boneIndices)
//{
 
//    matrix curr = GetCurrAnimTransforms(boneIndices);
//    Matrix currS;
//    matrix currT;
//    float4 currQ;
//    MatrixDecompose(currS, currQ, currT, curr);
//    float3 currPos = currT._41_42_43;
//    float3 currScale = currT._11_22_33;
  
//    matrix next = GetNextAnimTransforms(boneIndices);
//    Matrix nextS;
//    matrix nextT;
//    float4 nextQ;
//    MatrixDecompose(nextS, nextQ, nextT, next);
//    float3 nextPos = currT._41_42_43;
//    float3 nextScale = currT._11_22_33;
  
  
  
//    float3 finalS = normalize(lerp(currPos, nextPos, Tweenframes.TweenTime));
//    float3 finalT = normalize(lerp(currScale, nextScale, Tweenframes.TweenTime));
//    float4 finalQ = normalize(lerp(currQ, nextQ, Tweenframes.TweenTime));
//    matrix finalR = QuattoMat(finalQ);
//    finalR._41_42_43_44 = float4(finalT, 1.0);
//    finalR._11_22_33 = finalS;
  
//    return finalR;

//}
matrix LerpCurrAnim_NextAnimTransform(uint boneIndices)
{

    matrix curr = GetCurrAnimTransforms(boneIndices);
    matrix next = GetNextAnimTransforms(boneIndices);

    return lerp(curr, next, Tweenframes.TweenTime);

}
void SetAnimationWorld(inout matrix world, float4 BlendIndices, float4 BlendWeights)
{
    matrix transform = 0;
    matrix Anim = 0;

    float4 boneIndices = BoneIndeces(BlendIndices);
    float4 boneWeights = BoneWeights(BlendWeights);
 
     
    [unroll(4)]
    for (int i = 0; i < 4; i++)
    {
        Anim =
        lerp(
        GetCurrAnimTransforms(boneIndices[i]), LerpCurrAnim_NextAnimTransform(boneIndices[i]),
        saturate(Tweenframes.Next.Clip + 1)
        );
     
        transform += mul(boneWeights[i], Anim);
    }
   
    world = mul(transform, W);
}



VertexModelOutput VS(VertexSkeletal input)
{
    VertexModelOutput output;
    Matrix World;
    SetAnimationWorld(World, input.BlendIndices,input.BlendWeights);
    output.Position = mul(input.Position, World);
    output.Position = mul(output.Position, VP);
    output.Normal = mul(input.Normal, (float3x3) World);
    output.Tangent = mul(input.Tangent, (float3x3) World);
    output.Uv = input.Uv;
 
    return output;
}
