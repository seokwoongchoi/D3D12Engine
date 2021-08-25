


//--------------------------------------------------------------------------------------
// Shader Inputs/Outputs
//--------------------------------------------------------------------------------------


struct VSIn_Diffuse
{
    float3 position : POSITION;
    float2 texcoord : TEXCOORD;
    float3 normal : NORMAL;
};

struct PSIn_Diffuse
{
    float4 position : SV_Position;
    //centroid float2 texcoord : TEXCOORD0;
    //centroid float3 normal : NORMAL;
    //centroid float4 positionWS : TEXCOORD1;
    //centroid float4 layerdef : TEXCOORD2;
   
};

struct PSIn_Quad
{
    float4 position : SV_Position;
    float2 texcoord : TEXCOORD0;
};

struct VSIn_Default
{
    float4 position : POSITION;
    float2 texcoord : TEXCOORD;
};


struct DUMMY
{
    float Dummmy : DUMMY;
};

struct HSIn_Heightfield
{
    float2 origin : ORIGIN;
    float2 size : SIZE;
};


struct PatchData
{
    float Edges[4] : SV_TessFactor;
    float Inside[2] : SV_InsideTessFactor;

    float2 origin : ORIGIN;
    float2 size : SIZE;
};

//--------------------------------------------------------------------------------------
// Constant Buffers
//--------------------------------------------------------------------------------------

cbuffer CBuffer : register(b0)
{
	
	float4x4 view;
	float4x4 projection;
	
};



//--------------------------------------------------------------------------------------
// Heightfield shaders
//--------------------------------------------------------------------------------------


struct VS_IN
{
	float4 PatchParams : PATCH_PARAMETERS;
	
};

HSIn_Heightfield PassThroughVS(VS_IN input)
{
    HSIn_Heightfield output;
	output.origin = input.PatchParams.xy;
	output.size = input.PatchParams.zw;
    return output;
}
PatchData PatchConstantHS(InputPatch<HSIn_Heightfield, 1> inputPatch)
{
    PatchData output;

	output.origin = inputPatch[0].origin;
	output.size = inputPatch[0].size;
	output.Edges[0] = 1;
	output.Edges[1] = 1;
	output.Edges[2] = 1;
	output.Edges[3] = 1;
	output.Inside[0] = 1;
	output.Inside[1] = 1;


   
    return output;
}
[domain("quad")]
[partitioning("integer")]
[outputtopology("triangle_cw")]
[outputcontrolpoints(1)]
[patchconstantfunc("PatchConstantHS")]
DUMMY PatchHS(InputPatch<HSIn_Heightfield, 1> inputPatch)
{
    return (DUMMY) 0;
}

[domain("quad")]
PSIn_Diffuse HeightFieldPatchDS(PatchData input,
                                    float2 uv : SV_DomainLocation,
                                    OutputPatch<DUMMY, 1> inputPatch)
{
    PSIn_Diffuse output;
    float3 vertexPosition;

	   
    vertexPosition.xz = input.origin + uv * input.size;
    vertexPosition.y =0.0f ;
	output.position = mul(float4(vertexPosition, 1.0), view);
	output.position = mul(output.position, projection);
  
    return output;
}



/////////////////////////////////////////////////////////////////////////////////////////////////////////////////


struct PS_GBUFFER_OUT
{
    float4 ColorSpecInt : SV_Target0;
    float4 Normal : SV_Target1;
    float4 Specular : SV_Target2;
};

PS_GBUFFER_OUT PackGBuffer(float3 BaseColor, float3 Normal, float metallic, float roughness, float terrainMask = 1)
{
    PS_GBUFFER_OUT Out;
    Out.ColorSpecInt = float4(BaseColor.rgb, terrainMask);
    Out.Normal = float4(normalize(Normal.rgb + 0.5 + 0.5), 1.0);
    Out.Specular = float4(roughness, metallic, 0.0, 0.0);
    return Out;
}

PS_GBUFFER_OUT HeightFieldPatchPacking(PSIn_Diffuse input)
{
 	return PackGBuffer(float3(1, 1, 1), float3(1, 1, 1), 0.0, 0.0, 0);
}
