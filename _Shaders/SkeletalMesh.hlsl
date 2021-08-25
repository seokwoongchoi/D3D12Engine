#include "Header.hlsl"
Texture2D<float4> AnimBoneTransforms : register(t1);

struct VertexSkeletal
{
    float4 Position : Position0;
    float2 Uv : Uv0;
    float3 Normal : Normal0;
    float3 Tangent : Tangent0;
	float4 BlendIndices : BlendIndices0;
	float4 BlendWeights : BlendWeights0;

 
    uint InstID : SV_InstanceID;
};

///////////////////////////////////////////////////////////////////////////////

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
 
void SetAnimationWorld(float4 BlendIndices, float4 BlendWeights, uint InstID)
{
    matrix transform = 0;
  
  
    float4 boneIndices = BoneIndeces(BlendIndices);
    float4 boneWeights = BoneWeights(BlendWeights);
    uint instId = prevDrawCount + InstID;
   
    
  
    [unroll(4)]
    for (int i = 0; i < 4; i++)
    {
        float4 c0, c1, c2, c3;
        c0 = AnimBoneTransforms.Load(uint3(boneIndices[i] * 4 + 0,  instId, 0));
        c1 = AnimBoneTransforms.Load(uint3(boneIndices[i] * 4 + 1,  instId, 0));
        c2 = AnimBoneTransforms.Load(uint3(boneIndices[i] * 4 + 2,  instId, 0));
        c3 = AnimBoneTransforms.Load(uint3(boneIndices[i] * 4 + 3,  instId, 0));
    
       
        transform += mul(boneWeights[i], matrix(c0, c1, c2, c3));
    }
   
    float4 inst1 = InstTransforms[int2(instId * 4 + 0, 0)];
    float4 inst2 = InstTransforms[int2(instId * 4 + 1, 0)];
    float4 inst3 = InstTransforms[int2(instId * 4 + 2, 0)];
    float4 inst4 = InstTransforms[int2(instId * 4 + 3, 0)];
    
    World = mul(transform, matrix(inst1, inst2, inst3, inst4));
}

struct VertexPosBlendInst
{
    float4 Position : Position0;
    float4 BlendIndices : BlendIndices0;
    float4 BlendWeights : BlendWeights0;
    uint InstID : SV_InstanceID;

};
float4 CascadedShadowGenVS(VertexPosBlendInst input) : SV_Position
{
    
  
    SetAnimationWorld(input.BlendIndices, input.BlendWeights, input.InstID);
  
    float4 Position = WorldPosition(input.Position);
   
  
    return Position;

}
VertexModelOutput VS(VertexSkeletal input)
{
    VertexModelOutput output;
    SetAnimationWorld(input.BlendIndices,input.BlendWeights,input.InstID);
    output.Position = WorldPosition(input.Position);
    output.Position = ViewProjection(output.Position);
    output.Normal = WorldNormal(input.Normal);
    output.Tangent = WorldTangent(input.Tangent);
    output.Uv = input.Uv;
    //output.Cull.x = dot(float4(output.wPosition.xyz - ViewPosition(), 1.0f), -g_FrustumNormals[0]);
   //output.Cull.y = dot(float4(output.wPosition.xyz - ViewPosition(), 1.0f), -g_FrustumNormals[1]);
   //output.Cull.z = dot(float4(output.wPosition.xyz - ViewPosition(), 1.0f), -g_FrustumNormals[2]);
   //output.Cull.w = 0;
    return output;
}

//const static float4 plane = float4(0, 1, 0, 0.0);
VertexModelOutputReflection ReflectionVS(VertexSkeletal input)
{
    VertexModelOutputReflection output;
    SetAnimationWorld(input.BlendIndices, input.BlendWeights, input.InstID);
    output.Position = WorldPosition(input.Position);
    float4 plane = float4(0, 1, 0, output.Position.y + 0.6f);
    output.Cull = dot(output.Position, plane);
    output.Position = ViewProjection(output.Position);
    output.Normal = WorldNormal(input.Normal);
    output.Tangent = WorldTangent(input.Tangent);
    output.Uv = input.Uv;
    //output.Cull.x = dot(float4(output.wPosition.xyz - ViewPosition(), 1.0f), -g_FrustumNormals[0]);
   //output.Cull.y = dot(float4(output.wPosition.xyz - ViewPosition(), 1.0f), -g_FrustumNormals[1]);
   //output.Cull.z = dot(float4(output.wPosition.xyz - ViewPosition(), 1.0f), -g_FrustumNormals[2]);
   //output.Cull.w = 0;
    return output;
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////



