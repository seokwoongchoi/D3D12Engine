#include "Header.hlsl"


Texture2D<float4> BoneTransforms : register(t1);

void SetModelWorld(int InstID)
{
    float4 m1 = BoneTransforms[int2(BoneIndex * 4 + 0, actorIndex)];
    float4 m2 = BoneTransforms[int2(BoneIndex * 4 + 1, actorIndex)];
    float4 m3 = BoneTransforms[int2(BoneIndex * 4 + 2, actorIndex)];
    float4 m4 = BoneTransforms[int2(BoneIndex * 4 + 3, actorIndex)];
       
    uint instId = prevDrawCount + InstID;
  
    float4 inst1 = InstTransforms[int2(instId * 4 + 0, 0)];
    float4 inst2 = InstTransforms[int2(instId * 4 + 1, 0)];
    float4 inst3 = InstTransforms[int2(instId * 4 + 2, 0)];
    float4 inst4 = InstTransforms[int2(instId * 4 + 3, 0)];
    World = mul(matrix(m1, m2, m3, m4), matrix(inst1, inst2, inst3, inst4));
}
struct VertexPosInst
{
    float4 Position : Position0;
    uint InstID : SV_InstanceID;

};
float4 CascadedShadowGenVS(VertexPosInst input) : SV_Position
{
    
  
    SetModelWorld(input.InstID);
  
    float4 Position = WorldPosition(input.Position);
   
  
    return Position;

}

struct VertexStatic
{
    float4 Position : Position0;
    float2 Uv : Uv0;
    float3 Normal : Normal0;
    float3 Tangent : Tangent0;
   
      uint InstID : SV_InstanceID;
};




///////////////////////////////////////////////////////////////////////////////



VertexModelOutput VS(VertexStatic input)
{
    VertexModelOutput output;

   
    SetModelWorld(input.InstID);
  
    output.Position = WorldPosition(input.Position);
    output.Position = ViewProjection(output.Position);
    output.Normal = WorldNormal(input.Normal);
    output.Tangent = WorldTangent(input.Tangent);
    
    output.Uv = input.Uv;
    
    //output.Cull.x = dot(float4(worldPosition - ViewPosition(), 1.0f), -FrustumNormals[0]);
    //output.Cull.y = dot(float4(worldPosition - ViewPosition(), 1.0f), -FrustumNormals[1]);
    //output.Cull.z = dot(float4(worldPosition - ViewPosition(), 1.0f), -FrustumNormals[2]);
    //output.Cull.w = dot(float4(worldPosition - ViewPosition(), 1.0f), -FrustumNormals[3]);
    return output;
}


const static float4 plane = float4(0, 1, 0, 0.1);
VertexModelOutputReflection ReflectionVS(VertexStatic input)
{
    VertexModelOutputReflection output;

   
    SetModelWorld(input.InstID);
  
    output.Position = WorldPosition(input.Position);
   
    output.Cull = dot(output.Position, plane);
    
    output.Position = ViewProjection(output.Position);
    output.Normal = WorldNormal(input.Normal);
    output.Tangent = WorldTangent(input.Tangent);
    
    output.Uv = input.Uv;
    
  
  
    return output;
}


//VertexModelParallaxMappingOutput ParallaxMappingVS(VertexStatic input)
//{
//    VertexModelParallaxMappingOutput output;

   
//    SetModelWorld(input.InstID);
  
//    output.Position = WorldPosition(input.Position);
//    float4 PositionWS = output.Position;
//    output.Position = ViewProjection(output.Position);
 
//    float3 view = EyePos.xyz - PositionWS.xyz;
    
    
//    output.Uv = input.Uv;
    
//    output.Normal = WorldNormal(input.Normal);
//    output.Tangent = WorldTangent(input.Tangent);
    
   
//    output.ViewWorldSpace =  view;
   
       
  
//    return output;
//}