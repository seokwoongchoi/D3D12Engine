#include "000_Matrix.fx"
#define MAX_MODEL_TRANSFORMS 450
#define MAX_MODEL_KEYFRAMES 350
#define MAX_MODEL_INSTANCE 50
#define MAX_ACTOR_BONECOLLIDER 2
Texture2DArray BoneTransforms;
RWTexture2D<float4> Output;

RWTexture2D<float4> EffectOutput;

Texture2D InstInput;

struct AnimationFrame
{
    int Clip;

    uint CurrFrame;
    uint NextFrame;

    float Time;
    float Running;

    float3 Padding;
};

struct TweenFrame
{
    float TakeTime;
    float TweenTime;
    
    int IsCulled;
    float Padding;

    AnimationFrame Curr;
    AnimationFrame Next;
};


//RWStructuredBuffer<TweenFrame> OutTween;

//StructuredBuffer<TweenFrame> InTween;
cbuffer CB_AnimationFrame
{
    TweenFrame Tweenframes[MAX_MODEL_INSTANCE];
};

cbuffer CB_DrawCount
{
    uint drawCount;
    uint actorIndex;
    uint particleIndex;
    float pad0;
};

struct AttachData
{
    matrix local;
    matrix BoneScale;
    
    int index;
    
    float2 padding;
};


cbuffer CB_Attach
{
    AttachData BoneCollider[MAX_ACTOR_BONECOLLIDER];
  
};

struct EffectData
{
    matrix effectlocal;
  
    int EffectIndex;
    float3 pad0;
};

cbuffer CB_EffectAttach
{
    EffectData effectData[MAX_ACTOR_BONECOLLIDER];
   
};

cbuffer CB_Box
{
   
    matrix BoxBound;
   
};
matrix LoadAnimTransforms(uint boneIndices, uint frame, uint clip)
{
    float4 c0, c1, c2, c3;
    c0 = BoneTransforms.Load(uint4(boneIndices * 4 + 0, frame, clip, 0));
    c1 = BoneTransforms.Load(uint4(boneIndices * 4 + 1, frame, clip, 0));
    c2 = BoneTransforms.Load(uint4(boneIndices * 4 + 2, frame, clip, 0));
    c3 = BoneTransforms.Load(uint4(boneIndices * 4 + 3, frame, clip, 0));
    
    return matrix(c0, c1, c2, c3);
}

matrix GetCurrAnimTransforms(uint index, uint AttachBoneIndex)
{
    
    
    //uint boneIndex[2];
    //boneIndex[0] = (Tweenframes[index].Curr.Clip * MAX_MODEL_KEYFRAMES * MAX_MODEL_TRANSFORMS);
    //boneIndex[0] += (Tweenframes[index].Curr.CurrFrame * MAX_MODEL_TRANSFORMS);
    //boneIndex[0] += AttachBoneIndex;

    //boneIndex[1] = (Tweenframes[index].Curr.Clip * MAX_MODEL_KEYFRAMES * MAX_MODEL_TRANSFORMS);
    //boneIndex[1] += (Tweenframes[index].Curr.NextFrame * MAX_MODEL_TRANSFORMS);
    //boneIndex[1] += AttachBoneIndex;

    //matrix currFrame = Input[boneIndex[0]].Bone;
    //matrix nextFrame = Input[boneIndex[1]].Bone;
    
    matrix curr = LoadAnimTransforms(AttachBoneIndex, Tweenframes[index].Curr.CurrFrame, Tweenframes[index].Curr.Clip);
    matrix next = LoadAnimTransforms(AttachBoneIndex, Tweenframes[index].Curr.NextFrame, Tweenframes[index].Curr.Clip);
    return lerp(curr, next, (matrix) Tweenframes[index].Curr.Time);
    
   // return lerp(currFrame, nextFrame, Tweenframes[index].Curr.Time);
}

matrix GetNextAnimTransforms(uint index, uint AttachBoneIndex)
{
    //matrix result = 0;
    //uint boneIndex[2];
    //boneIndex[0] = (Tweenframes[index].Next.Clip * MAX_MODEL_KEYFRAMES * MAX_MODEL_TRANSFORMS);
    //boneIndex[0] += (Tweenframes[index].Next.CurrFrame * MAX_MODEL_TRANSFORMS);
    //boneIndex[0] += AttachBoneIndex;

    //boneIndex[1] = (Tweenframes[index].Next.Clip * MAX_MODEL_KEYFRAMES * MAX_MODEL_TRANSFORMS);
    //boneIndex[1] += (Tweenframes[index].Next.NextFrame * MAX_MODEL_TRANSFORMS);
    //boneIndex[1] += AttachBoneIndex;

    //matrix currFrame = Input[boneIndex[0]].Bone;
    //matrix nextFrame = Input[boneIndex[1]].Bone;
    
    matrix curr = LoadAnimTransforms(AttachBoneIndex, Tweenframes[index].Next.CurrFrame, Tweenframes[index].Next.Clip);
    matrix next = LoadAnimTransforms(AttachBoneIndex, Tweenframes[index].Next.NextFrame, Tweenframes[index].Next.Clip);
    return lerp(curr, next, (matrix) Tweenframes[index].Next.Time);
   
   // return lerp(currFrame, nextFrame, Tweenframes[index].Next.Time);
     
}
void GetInstMatrix(inout matrix transform,uint index)
{
    float4 m1 = InstInput.Load(int3(0, index,  0));
    float4 m2 = InstInput.Load(int3(1, index,  0));
    float4 m3 = InstInput.Load(int3(2, index,  0));
    float4 m4 = InstInput.Load(int3(3, index,  0));
        
    transform = matrix(m1, m2, m3, m4);
}

[numthreads(1, 1, 1)]
void CS(uint3 globalThreadId : SV_DispatchThreadID)
{
    
    {
      
        matrix result = 0;
        matrix computeMatrix = 0;
        matrix final = 0;
       
        matrix transform = 0;
        float3 position = 0;
        float4 q = 0;
        float3 scale = 0;
        matrix compute2 = 0;
        GroupMemoryBarrierWithGroupSync();
        result = lerp(
        GetCurrAnimTransforms(globalThreadId.x, BoneCollider[0].index),
        lerp(GetCurrAnimTransforms(globalThreadId.x, BoneCollider[0].index), GetNextAnimTransforms(globalThreadId.x, BoneCollider[0].index),
        Tweenframes[globalThreadId.x].TweenTime),
        saturate(Tweenframes[globalThreadId.x].Next.Clip + 1)
        );
              
        
        computeMatrix = mul(BoneCollider[0].local, result);
        GetInstMatrix(transform, globalThreadId.x);
        final = mul(computeMatrix, transform);
        decompose(final, position, q, scale);
        
       
        matrix compute = mul(BoneCollider[0].BoneScale, compose(position, q, float3(1, 1, 1)));
        
        [flatten]
        if (BoneCollider[1].index > 0)
        {
            matrix result2 = lerp(
            GetCurrAnimTransforms(globalThreadId.x, BoneCollider[1].index),
            lerp(GetCurrAnimTransforms(globalThreadId.x, BoneCollider[1].index), GetNextAnimTransforms(globalThreadId.x, BoneCollider[1].index),
            Tweenframes[globalThreadId.x].TweenTime),
            saturate(Tweenframes[globalThreadId.x].Next.Clip + 1)
            );
       
            matrix computeMatrix2 = mul(BoneCollider[1].local, result2);
            matrix final2 = mul(computeMatrix2, transform);
              
            float3 position2 = 0;
            float4  q2 = 0;
            float3  scale2 = 0;
        
            decompose(final2, position2, q2, scale2);
            compute2 = mul(BoneCollider[1].BoneScale, compose(position2, q2, float3(1, 1, 1)));
        }
        
      
        GroupMemoryBarrierWithGroupSync();

       
      
           
        uint factor = lerp(drawCount, drawCount + Output[uint2(9, actorIndex - 1)].b, saturate(actorIndex));
       
        Output[uint2(9, actorIndex)] = float4(factor, 0, 0, 0);
        
        uint index = lerp(globalThreadId.x, Output[uint2(9, actorIndex - 1)].r + globalThreadId.x, saturate(actorIndex));
        
        Output[uint2(0, index)] = float4(compute._11, compute._12, compute._13, compute._14);
        Output[uint2(1, index)] = float4(compute._21, compute._22, compute._23, compute._24);
        Output[uint2(2, index)] = float4(compute._31, compute._32, compute._33, compute._34);
        Output[uint2(3, index)] = float4(compute._41, compute._42, compute._43, compute._44);
        
        Output[uint2(4, index)] = float4(compute2._11, compute2._12, compute2._13, compute2._14);
        Output[uint2(5, index)] = float4(compute2._21, compute2._22, compute2._23, compute2._24);
        Output[uint2(6, index)] = float4(compute2._31, compute2._32, compute2._33, compute2._34);
        Output[uint2(7, index)] = float4(compute2._41, compute2._42, compute2._43, compute2._44);
        
        Output[uint2(8, index)] = float4(actorIndex, globalThreadId.x, drawCount, 0);
     
     
        
      
     
        
        //Output[actorIndex].Factor.a = lerp(drawCount, drawCount + Output[actorIndex - 1].Factor.a, saturate(actorIndex));
        //uint inedx = lerp(globalThreadId.x, Output[actorIndex - 1].Factor.a + globalThreadId.x, saturate(actorIndex));
        //Output[inedx].Result = compute;
     
        //Output[inedx].Factor.r = actorIndex;
        //Output[inedx].Factor.g = globalThreadId.x;
 
       
   
    }
   
}

[numthreads(1, 1, 1)]
void CS2(uint3 globalThreadId : SV_DispatchThreadID)
{
    
  
    
      
      
    matrix transform = 0;
    GetInstMatrix(transform, globalThreadId.x);
    matrix final = mul(BoxBound, transform);
    
    uint factor = lerp(drawCount, drawCount + Output[uint2(9, actorIndex - 1)].b, saturate(actorIndex));
       
    Output[uint2(9, actorIndex)] = float4(factor,0, 0, 0);
        
    uint index = lerp(globalThreadId.x, Output[uint2(9, actorIndex - 1)].r + globalThreadId.x, saturate(actorIndex));
  
 
    
    Output[uint2(0, index)] = float4(final._11, final._12, final._13, final._14);
    Output[uint2(1, index)] = float4(final._21, final._22, final._23, final._24);
    Output[uint2(2, index)] = float4(final._31, final._32, final._33, final._34);
    Output[uint2(3, index)] = float4(final._41, final._42, final._43, final._44);
    
    Output[uint2(4, index)] = 0;
    Output[uint2(5, index)] = 0;
    Output[uint2(6, index)] = 0;
    Output[uint2(7, index)] = 0;
    
    Output[uint2(8, index)] = float4(actorIndex, globalThreadId.x, drawCount, 1);
     
   
}

[numthreads(1, 1, 1)]
void EffectCS(uint3 globalThreadId : SV_DispatchThreadID)
{
    
   
   
   matrix result3 = lerp
         (
         GetCurrAnimTransforms(globalThreadId.x, effectData[particleIndex].EffectIndex),
         lerp
         (
         GetCurrAnimTransforms(globalThreadId.x, effectData[particleIndex].EffectIndex),
         GetNextAnimTransforms(globalThreadId.x, effectData[particleIndex].EffectIndex),
         Tweenframes[globalThreadId.x].TweenTime
         ),
         saturate(Tweenframes[globalThreadId.x].Next.Clip + 1)
         );
    //matrix local = mul(effectData[particleIndex].effectTranslation,effectData[particleIndex].effectlocal);
    matrix computeMatrix3 = mul(effectData[particleIndex].effectlocal, result3);
   // GetInstMatrix(transform, globalThreadId.x);
   
            
     
   
   float3 position = 0;
    float4 q = float4(0.0f, 0.0f, 0.0f,0.0f);
   float3 scale = 0;
        
    decompose(computeMatrix3, position, q, scale);
    matrix result = compose( position, q, float3(1, 1, 1));
   //
        

     
    EffectOutput[uint2(0, globalThreadId.x)] = float4(result._11, result._12, result._13, result._14);
    EffectOutput[uint2(1, globalThreadId.x)] = float4(result._21, result._22, result._23, result._24);
    EffectOutput[uint2(2, globalThreadId.x)] = float4(result._31, result._32, result._33, result._34);
    EffectOutput[uint2(3, globalThreadId.x)] = float4(result._41, result._42, result._43, result._44);
        
}




technique11 T0
{
    pass P0
    {
        SetVertexShader(NULL);
        SetPixelShader(NULL);

        SetComputeShader(CompileShader(cs_5_0, CS()));
    }

    pass P1
    {
        SetVertexShader(NULL);
        SetPixelShader(NULL);

        SetComputeShader(CompileShader(cs_5_0, CS2()));
    }

    pass P2
    {
        SetVertexShader(NULL);
        SetPixelShader(NULL);

        SetComputeShader(CompileShader(cs_5_0, EffectCS()));
    }


   
}