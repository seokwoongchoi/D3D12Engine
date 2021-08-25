Texture2D<float> Input : register(t0);
RWTexture2D<float> Output : register(u0);
SamplerState LinearSampler : register(s0);
cbuffer cb0 : register(b0)
{
    static const int2 Res = int2(640, 360); // Resulotion of the down scaled image: x - width, y - height
}
static const float SampleWeights[13] =
{
    0.05f,
    0.05f,
    0.1f,
    0.1f,
    0.1f,
    0.2f,
    0.1f,
    0.1f,
    0.1f,
    0.05f,
    0.05f,
    0.05f, 0.05f,
 
};
//static const float SampleWeights[13] =
//{
//    0.002216,
//    0.008764,
//    0.026995,
//    0.064759,
//    0.120985,
//    0.176033,
//    0.199471,
//    0.176033,
//    0.120985,
//    0.064759,
//    0.026995,
//    0.008764,
//    0.002216,
//};

#define kernelhalf 6
#define groupthreads 128
groupshared float SharedInput[groupthreads];

[numthreads(groupthreads, 1, 1)]
void VerticalFilter(uint3 Gid : SV_GroupID, uint GI : SV_GroupIndex)
{
    int2 coord = int2(Gid.x, GI - kernelhalf + (groupthreads - kernelhalf * 2) * Gid.y);
    coord = clamp(coord, int2(0, 0), int2(Res.x - 1, Res.y - 1));
    SharedInput[GI].r = Input.Load(int3(coord, 0)).r;

    GroupMemoryBarrierWithGroupSync();

    // Vertical blur
    [branch]
    if (GI >= kernelhalf && GI < (groupthreads - kernelhalf) &&
         ((GI - kernelhalf + (groupthreads - kernelhalf * 2) * Gid.y) < Res.y))
    {
        float vOut = 0;
        
        [unroll(12)]
        for (int i = -kernelhalf; i <= kernelhalf; ++i)
        {
            vOut += SharedInput[GI + i].r * SampleWeights[i + kernelhalf];
        }

        Output[coord].r = Input.Load(int3(coord, 0)).r;
    }
}

[numthreads(groupthreads, 1, 1)]
void HorizFilter(uint3 Gid : SV_GroupID, uint GI : SV_GroupIndex)
{
    int2 coord = int2(GI - kernelhalf + (groupthreads - kernelhalf * 2) * Gid.x, Gid.y);
    coord = clamp(coord, int2(0, 0), int2(Res.x - 1, Res.y - 1));
    SharedInput[GI].r = Input.Load(int3(coord, 0)).r;

    GroupMemoryBarrierWithGroupSync();

    // Horizontal blur
       [branch]
    if (GI >= kernelhalf && GI < (groupthreads - kernelhalf) &&
         ((Gid.x * (groupthreads - 2 * kernelhalf) + GI - kernelhalf) < Res.x))
    {
        float vOut = 0;
        
        [unroll(12)]
        for (int i = -kernelhalf; i <= kernelhalf; ++i)
            vOut += SharedInput[GI + i].r * SampleWeights[i + kernelhalf];

        Output[coord].r = Input.Load(int3(coord, 0)).r;
    }
}
//static const float SampleWeights[11] =
//{
//    0.05f,
//    0.05f,
//    0.1f,
//    0.1f,
//    0.1f,
//    0.2f,
//    0.1f,
//    0.1f,
//    0.1f,
//    0.05f,
//    0.05f,
   
//};


//#define N 256
//#define CacheSize (N+2*BlurRadius)
//groupshared float4 SharedInput[CacheSize];

//[numthreads(1, N, 1)]
//void VerticalFilter(int3 groupThreadID:SV_GroupThreadID,int3 dispatchThreadID:SV_DispatchThreadID)
//{
//    if (groupThreadID.y < BlurRadius)
//    {
//        int y = max(dispatchThreadID.y - BlurRadius, 0);
//        SharedInput[groupThreadID.y].r = Input[int2(dispatchThreadID.x, y)].r;

//    }
    
    
//    if (groupThreadID.y >=N- BlurRadius)
//    {
//        int y = min(dispatchThreadID.y + BlurRadius, Res.y-1);
//        SharedInput[groupThreadID.y+2*BlurRadius].r = Input[int2(dispatchThreadID.x, y)].r;
//    }
    
    
//    SharedInput[groupThreadID.y + BlurRadius].r = Input[min(dispatchThreadID.xy  , Res.xy - 1)].r;
//    GroupMemoryBarrierWithGroupSync();
    
//    float blurColor = 0.0f;
    
//    [unroll(10)]
//    for (int i = -BlurRadius; i <= BlurRadius;++i)
//    {
//        int k = groupThreadID.y + BlurRadius + i;
//        blurColor += SampleWeights[i + BlurRadius].r * SharedInput[k].r;

//    }
//    Output[dispatchThreadID.xy].r = blurColor;
//}

//[numthreads(N, 1, 1)]
//void HorizFilter(int3 groupThreadID : SV_GroupThreadID, int3 dispatchThreadID : SV_DispatchThreadID)
//{
//    if (groupThreadID.x < BlurRadius)
//    {
//        int x = max(dispatchThreadID.x - BlurRadius, 0);
//        SharedInput[groupThreadID.x].r = Input[int2(x, dispatchThreadID.y)].r;

//    }
    
    
//    if (groupThreadID.x >= N - BlurRadius)
//    {
//        int x = min(dispatchThreadID.x + BlurRadius, Res.x - 1);
//        SharedInput[groupThreadID.x + 2 * BlurRadius].r = Input[int2(x, dispatchThreadID.y)].r;
//    }
    
    
//    SharedInput[groupThreadID.x + BlurRadius].r = Input[min(dispatchThreadID.xy, Res.xy - 1)].r;
    
//    GroupMemoryBarrierWithGroupSync();
    
//    float blurColor = 0.0f;
    
//    [unroll(10)]
//    for (int i = -BlurRadius; i <= BlurRadius; ++i)
//    {
//        int k = groupThreadID.x + BlurRadius + i;
//        blurColor += SampleWeights[i + BlurRadius].r * SharedInput[k].r;

//    }
    
//    Output[dispatchThreadID.xy].r = blurColor;
//}