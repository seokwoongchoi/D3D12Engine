
#include "PreviewHeader.hlsl"

struct VertexStatic
{
    float4 Position : Position0;
    float2 Uv : Uv0;
    float3 Normal : Normal0;
    float3 Tangent : Tangent0;
    

};


///////////////////////////////////////////////////////////////////////////////
Texture2D<float4> BoneTransforms : register(t0);

matrix SetModelWorld()
{
    
    float4 m1 = BoneTransforms[int2(BoneIndex * 4 + 0,0)];
    float4 m2 = BoneTransforms[int2(BoneIndex * 4 + 1,0)];
    float4 m3 = BoneTransforms[int2(BoneIndex * 4 + 2,0)];
    float4 m4 = BoneTransforms[int2(BoneIndex * 4 + 3,0)];
    
    return  mul(matrix(m1, m2, m3, m4), W);
}


VertexModelOutput VS(VertexStatic input)
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


