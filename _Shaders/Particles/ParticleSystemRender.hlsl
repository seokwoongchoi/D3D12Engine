#define IDENTITY_MATRIX matrix(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1)
cbuffer CB_Draw:register(b0)
{
    float4x4 ParticleViewProjection : packoffset(c0);
    float PointSize : packoffset(c4.x);
    uint readOffset : packoffset(c4.y);
    uint positionIndex : packoffset(c4.z);
    float intensity : packoffset(c4.w);
};



const static float4 texcoords[4] =
{
    float4(0, 0, 0, 0),
    float4(0, 1, 0, 0),
    float4(1, 0, 0, 0),
    float4(1, 1, 0, 0),
};


StructuredBuffer<float4> SimulationState : register(t0);
//StructuredBuffer<float3> PostionTexture : register(t1);
Texture2D<float4> PostionTexture : register(t1);
Texture2D       ParticleTexture : register( t2 );      

SamplerState    LinearSampler : register( s0 );

struct VertexPosInst
{
   // float3 Position : Position0;
    uint VertexID : SV_VertexID;
    uint InstID : SV_InstanceID;

};
struct DisplayVS_OUTPUT
{
    float4 Position : SV_POSITION; // vertex position 
    float2 uv : Uv;
    float PointSize : PSIZE; // point size;
};
DisplayVS_OUTPUT VSMAIN(VertexPosInst input)
{
    DisplayVS_OUTPUT Output;
	
  
    float4 pos = float4(SimulationState[readOffset + input.VertexID].xyz, 1.0f);
  
    matrix World = IDENTITY_MATRIX;
    World._41_42_43_44 = float4(PostionTexture[uint2(positionIndex, input.InstID)].xyz, 1.0f);
  //  World._41_42_43_44 = float4(PostionTexture[input.InstID].xyz, 1.0f);
    World = mul(World, ParticleViewProjection);
    Output.Position = mul(pos, World);
  
    Output.PointSize = PointSize;

   
    Output.uv = float2(intensity , 0.0f);
	return Output;
}


DisplayVS_OUTPUT PreviewVS(uint vertexid : SV_VertexID)
{
    DisplayVS_OUTPUT Output;
	
  
    float4 pos = float4(SimulationState[readOffset + vertexid].xyz, 1.0f);
  //  float4 pos = float4(float3(0, 1.0f + vertexid, 0), 1.0f);
    Output.Position = mul(pos, ParticleViewProjection);
  
    Output.PointSize = PointSize;

   
    Output.uv = float2(intensity, 0.0f);
    return Output;
}

const static float4 positions[4] =
{
    float4( 0.5, -0.5, 0, 0),
    float4( 0.5,  0.5, 0, 0),
    float4(-0.5, -0.5, 0, 0),
    float4(-0.5,  0.5, 0, 0),
};
//--------------------------------------------------------------------------------
[maxvertexcount(4)]
void GSMAIN(point DisplayVS_OUTPUT input[1], inout TriangleStream<DisplayVS_OUTPUT> SpriteStream)
{
    DisplayVS_OUTPUT output;
  
	[unroll(4)]
    for (int i = 0; i < 4; i++)
    {
        output.Position = input[0].Position + float4(positions[i].xy * input[0].PointSize, 0, 0);
           
        output.PointSize = input[0].uv.x;
        output.uv = texcoords[i];
    

        SpriteStream.Append(output);
    }
    SpriteStream.RestartStrip();
}
//--------------------------------------------------------------------------------
float4 PSMAIN(in DisplayVS_OUTPUT input) : SV_Target
{
    float4 tex = ParticleTexture.Sample(LinearSampler, input.uv);
	 [flatten]
    if (tex.x < 0.05) 
        discard;
   // tex *= input.PointSize;
     [flatten]
    if (input.PointSize > 1.5)
    {
        tex.x += 0.8f;

    }
    return tex;

	
}
//--------------------------------------------------------------------------------
