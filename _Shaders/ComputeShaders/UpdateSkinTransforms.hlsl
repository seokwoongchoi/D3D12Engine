#define MAX_MODEL_TRANSFORMS 500
#define MAX_MODEL_KEYFRAMES 300
#define MAX_MODEL_INSTANCE 100
struct Keyframe
{
    int Clip;

    uint CurrFrame;
    uint NextFrame;

    float Time;
    float RunningTime;

    int IsUpdated;
    float2 Padding;
};

struct Tween
{
    float TakeTweenTime;
    float TweenTime;
    float2 Padding;

    Keyframe Curr;
    Keyframe Next;
};

cbuffer CB_AnimationFrame
{
    Tween Tweenframes[MAX_MODEL_INSTANCE];
  
  
};
cbuffer CB_Clip
{
  
   
    float frameRate;
    float frameCount;
    float time;
    uint pad;
    
  
};
struct ClipFrameBoneMatrix
{
    matrix Bone;
};

struct ResultMatrix
{
    matrix Result;
    
};

RWStructuredBuffer<ResultMatrix> Output;
Texture2DArray InputMap;
float4 LoadAnimTransforms(uint boneIndices, uint frame, uint clip)
{
    return InputMap.Load(uint4(boneIndices, frame, clip, 0));
}

matrix GetCurrAnimTransforms(float4 boneIndices, uint index, uint InstID)
{
    
    float4 c0, c1, c2, c3;
    float4 n0, n1, n2, n3;

    matrix curr = 0;
    matrix next = 0;
    matrix anim = 0;
    
    c0 = LoadAnimTransforms(boneIndices[index] * 4 + 0, Tweenframes[InstID].Curr.CurrFrame, Tweenframes[InstID].Curr.Clip);
    c1 = LoadAnimTransforms(boneIndices[index] * 4 + 1, Tweenframes[InstID].Curr.CurrFrame, Tweenframes[InstID].Curr.Clip);
    c2 = LoadAnimTransforms(boneIndices[index] * 4 + 2, Tweenframes[InstID].Curr.CurrFrame, Tweenframes[InstID].Curr.Clip);
    c3 = LoadAnimTransforms(boneIndices[index] * 4 + 3, Tweenframes[InstID].Curr.CurrFrame, Tweenframes[InstID].Curr.Clip);
    curr = matrix(c0, c1, c2, c3);
                                                   
    n0 = LoadAnimTransforms(boneIndices[index] * 4 + 0, Tweenframes[InstID].Curr.NextFrame, Tweenframes[InstID].Curr.Clip);
    n1 = LoadAnimTransforms(boneIndices[index] * 4 + 1, Tweenframes[InstID].Curr.NextFrame, Tweenframes[InstID].Curr.Clip);
    n2 = LoadAnimTransforms(boneIndices[index] * 4 + 2, Tweenframes[InstID].Curr.NextFrame, Tweenframes[InstID].Curr.Clip);
    n3 = LoadAnimTransforms(boneIndices[index] * 4 + 3, Tweenframes[InstID].Curr.NextFrame, Tweenframes[InstID].Curr.Clip);
    next = matrix(n0, n1, n2, n3);

    return anim = lerp(curr, next, (matrix) Tweenframes[InstID].Curr.Time);
}

matrix GetNextAnimTransforms(float4 boneIndices, uint index, uint InstID)
{
    
    float4 c0, c1, c2, c3;
    float4 n0, n1, n2, n3;

    matrix curr = 0;
    matrix next = 0;
    matrix anim = 0;
    
    c0 = LoadAnimTransforms(boneIndices[index] * 4 + 0, Tweenframes[InstID].Next.CurrFrame, Tweenframes[InstID].Next.Clip);
    c1 = LoadAnimTransforms(boneIndices[index] * 4 + 1, Tweenframes[InstID].Next.CurrFrame, Tweenframes[InstID].Next.Clip);
    c2 = LoadAnimTransforms(boneIndices[index] * 4 + 2, Tweenframes[InstID].Next.CurrFrame, Tweenframes[InstID].Next.Clip);
    c3 = LoadAnimTransforms(boneIndices[index] * 4 + 3, Tweenframes[InstID].Next.CurrFrame, Tweenframes[InstID].Next.Clip);
    curr = matrix(c0, c1, c2, c3);
                                                   
    n0 = LoadAnimTransforms(boneIndices[index] * 4 + 0, Tweenframes[InstID].Next.NextFrame, Tweenframes[InstID].Next.Clip);
    n1 = LoadAnimTransforms(boneIndices[index] * 4 + 1, Tweenframes[InstID].Next.NextFrame, Tweenframes[InstID].Next.Clip);
    n2 = LoadAnimTransforms(boneIndices[index] * 4 + 2, Tweenframes[InstID].Next.NextFrame, Tweenframes[InstID].Next.Clip);
    n3 = LoadAnimTransforms(boneIndices[index] * 4 + 3, Tweenframes[InstID].Next.NextFrame, Tweenframes[InstID].Next.Clip);
    next = matrix(n0, n1, n2, n3);

    return anim = lerp(curr, next, (matrix) Tweenframes[InstID].Next.Time);
}

[numthreads(100, 1, 1)]
void UpdateSkinTransform(uint3 DispatchThreadID : SV_DispatchThreadID
,uint GroupIndex : SV_GroupIndex, uint3 GroupID : SV_GroupID)
{
   
   
        uint index = GroupID.x;
        uint bone = GroupIndex;
    

        uint boneIndex[2];
        matrix result = 0;
        matrix curr = 0;
        matrix next = 0;
    
        uint clip[2];
        uint currFrame[2];
        uint nextFrame[2];
        float time[2];

        clip[0] = Tweenframes[index].Curr.Clip;
        currFrame[0] = Tweenframes[index].Curr.CurrFrame;
        nextFrame[0] = Tweenframes[index].Curr.NextFrame;
        time[0] = Tweenframes[index].Curr.Time;

        clip[1] = Tweenframes[index].Next.Clip;
        currFrame[1] = Tweenframes[index].Next.CurrFrame;
        nextFrame[1] = Tweenframes[index].Next.NextFrame;
        time[1] = Tweenframes[index].Next.Time;
    
        float4 c0, c1, c2, c3;
        float4 n0, n1, n2, n3;

    [unroll(4)]
        for (int i = 0; i < 4; i++)
        {
            c0 = InputMap.Load(int4(bone * 4 + 0, currFrame[0], clip[0], 0));
            c1 = InputMap.Load(int4(bone * 4 + 1, currFrame[0], clip[0], 0));
            c2 = InputMap.Load(int4(bone * 4 + 2, currFrame[0], clip[0], 0));
            c3 = InputMap.Load(int4(bone * 4 + 3, currFrame[0], clip[0], 0));
            curr = matrix(c0, c1, c2, c3);

            n0 = InputMap.Load(int4(bone * 4 + 0, nextFrame[0], clip[0], 0));
            n1 = InputMap.Load(int4(bone * 4 + 1, nextFrame[0], clip[0], 0));
            n2 = InputMap.Load(int4(bone * 4 + 2, nextFrame[0], clip[0], 0));
            n3 = InputMap.Load(int4(bone * 4 + 3, nextFrame[0], clip[0], 0));
            next = matrix(n0, n1, n2, n3);

            result = lerp(curr, next, time[0]);

        
        [flatten]
            if (clip[1] >= 0)
            {
                c0 = InputMap.Load(int4(bone * 4 + 0, currFrame[1], clip[1], 0));
                c1 = InputMap.Load(int4(bone * 4 + 1, currFrame[1], clip[1], 0));
                c2 = InputMap.Load(int4(bone * 4 + 2, currFrame[1], clip[1], 0));
                c3 = InputMap.Load(int4(bone * 4 + 3, currFrame[1], clip[1], 0));
                curr = matrix(c0, c1, c2, c3);

                n0 = InputMap.Load(int4(bone * 4 + 0, nextFrame[1], clip[1], 0));
                n1 = InputMap.Load(int4(bone * 4 + 1, nextFrame[1], clip[1], 0));
                n2 = InputMap.Load(int4(bone * 4 + 2, nextFrame[1], clip[1], 0));
                n3 = InputMap.Load(int4(bone * 4 + 3, nextFrame[1], clip[1], 0));
                next = matrix(n0, n1, n2, n3);

                matrix nextAnim = lerp(curr, next, time[1]);

                result = lerp(result, nextAnim, Tweenframes[index].TweenTime);
            }
        }
    
        Output[index].Result = result;

}




////////////////////////////////////////////////////////////////////////////////////////////
technique11 T0
{
   
    pass p0
    {
        SetVertexShader(NULL);
        SetPixelShader(NULL);
        SetComputeShader(CompileShader(cs_5_0, UpdateSkinTransform()));
    }
   
   
}
