// Copyright (c) 2011 NVIDIA Corporation. All rights reserved.
//
// TO  THE MAXIMUM  EXTENT PERMITTED  BY APPLICABLE  LAW, THIS SOFTWARE  IS PROVIDED
// *AS IS*  AND NVIDIA AND  ITS SUPPLIERS DISCLAIM  ALL WARRANTIES,  EITHER  EXPRESS
// OR IMPLIED, INCLUDING, BUT NOT LIMITED  TO, NONINFRINGEMENT,IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.  IN NO EVENT SHALL  NVIDIA 
// OR ITS SUPPLIERS BE  LIABLE  FOR  ANY  DIRECT, SPECIAL,  INCIDENTAL,  INDIRECT,  OR  
// CONSEQUENTIAL DAMAGES WHATSOEVER (INCLUDING, WITHOUT LIMITATION,  DAMAGES FOR LOSS 
// OF BUSINESS PROFITS, BUSINESS INTERRUPTION, LOSS OF BUSINESS INFORMATION, OR ANY 
// OTHER PECUNIARY LOSS) ARISING OUT OF THE  USE OF OR INABILITY  TO USE THIS SOFTWARE, 
// EVEN IF NVIDIA HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.
//
// Please direct any bugs or questions to SDKFeedback@nvidia.com

#define PI 3.1415926536f
#define BLOCK_SIZE_X 16
#define BLOCK_SIZE_Y 16

cbuffer CB_Immutable
{
	uint g_ActualDim;
	uint g_InWidth;
	uint g_OutWidth;
	uint g_OutHeight;
	uint g_DtxAddressOffset;
    
	uint g_DtyAddressOffset;
};

cbuffer CB_Perframe
{
	float g_Time;
	float g_ChoppyScale;
    float g_GridLen;
};

StructuredBuffer<float2>	g_InputH0;
StructuredBuffer<float>		g_InputOmega;
RWStructuredBuffer<float2>	g_OutputHt;


//---------------------------------------- Compute Shaders -----------------------------------------

// Pre-FFT data preparation:

// Notice: In CS5.0, we can output up to 8 RWBuffers but in CS4.x only one output buffer is allowed,
// that way we have to allocate one big buffer and manage the offsets manually. The restriction is
// not caused by NVIDIA GPUs and does not present on NVIDIA GPUs when using other computing APIs like
// CUDA and OpenCL.

// H(0) -> H(t)
[numthreads(BLOCK_SIZE_X, BLOCK_SIZE_Y, 1)]
void UpdateSpectrumCS(uint3 DTid : SV_DispatchThreadID)
{
	int in_index = DTid.y * g_InWidth + DTid.x;
	int in_mindex = (g_ActualDim - DTid.y) * g_InWidth + (g_ActualDim - DTid.x);
	int out_index = DTid.y * g_OutWidth + DTid.x;

	// H(0) -> H(t)
	float2 h0_k  = g_InputH0[in_index];
	float2 h0_mk = g_InputH0[in_mindex];
	float sin_v, cos_v;
	sincos(g_InputOmega[in_index] * g_Time, sin_v, cos_v);

	float2 ht;
	ht.x = (h0_k.x + h0_mk.x) * cos_v - (h0_k.y + h0_mk.y) * sin_v;
	ht.y = (h0_k.x - h0_mk.x) * sin_v + (h0_k.y - h0_mk.y) * cos_v;

	// H(t) -> Dx(t), Dy(t)
	float kx = DTid.x - g_ActualDim * 0.5f;
	float ky = DTid.y - g_ActualDim * 0.5f;
	float sqr_k = kx * kx + ky * ky;
	float rsqr_k = 0;
	if (sqr_k > 1e-12f)
		rsqr_k = 1 / sqrt(sqr_k);
	//float rsqr_k = 1 / sqrtf(kx * kx + ky * ky);
	kx *= rsqr_k;
	ky *= rsqr_k;
	float2 dt_x = float2(ht.y * kx, -ht.x * kx);
	float2 dt_y = float2(ht.y * ky, -ht.x * ky);

    if ((DTid.x < g_OutWidth) && (DTid.y < g_OutHeight))
	{
        g_OutputHt[out_index] = ht;
		g_OutputHt[out_index + g_DtxAddressOffset] = dt_x;
		g_OutputHt[out_index + g_DtyAddressOffset] = dt_y;
	}
}



//----------------------------------------- Pixel Shaders ------------------------------------------


    
struct VS_QUAD_OUTPUT
{
    float4 Position : SV_POSITION; // vertex position
    float2 TexCoord : Uv0; // vertex texture coords 
};

VS_QUAD_OUTPUT QuadVS(float4 vPos : POSITION)
{
    VS_QUAD_OUTPUT Output;

    Output.Position = vPos;
    Output.TexCoord.x = 0.5f + vPos.x * 0.5f;
    Output.TexCoord.y = 0.5f - vPos.y * 0.5f;

    return Output;
}

//----------------------------------------- Pixel Shaders ------------------------------------------

// Textures and sampling states
Texture2D g_samplerDisplacementMap;

SamplerState LinearSampler
{
    Filter = MIN_MAG_MIP_LINEAR;
    AddressU = WRAP;
    AddressV = WRAP;
};


// The following three should contains only real numbers. But we have only C2C FFT now.
StructuredBuffer<float2> g_InputDxyz;


// Post-FFT data wrap up: Dx, Dy, Dz -> Displacement
float4 UpdateDisplacementPS(VS_QUAD_OUTPUT In) : SV_Target
{
    uint index_x = (uint) (In.TexCoord.x * (float) g_OutWidth);
    uint index_y = (uint) (In.TexCoord.y * (float) g_OutHeight);
    uint addr = g_OutWidth * index_y + index_x;

	// cos(pi * (m1 + m2))
    int sign_correction = ((index_x + index_y) & 1) ? -1 : 1;

    float dx = g_InputDxyz[addr + g_DtxAddressOffset].x * sign_correction * g_ChoppyScale;
    float dy = g_InputDxyz[addr + g_DtyAddressOffset].x * sign_correction * g_ChoppyScale;
    float dz = g_InputDxyz[addr].x * sign_correction;

    return float4(dx, dy, dz, 1);
}

// Displacement -> Normal, Folding
float4 GenGradientFoldingPS(VS_QUAD_OUTPUT In) : SV_Target
{
	// Sample neighbour texels
    float2 one_texel = float2(1.0f / (float) g_OutWidth, 1.0f / (float) g_OutHeight);

    float2 tc_left = float2(In.TexCoord.x - one_texel.x, In.TexCoord.y);
    float2 tc_right = float2(In.TexCoord.x + one_texel.x, In.TexCoord.y);
    float2 tc_back = float2(In.TexCoord.x, In.TexCoord.y - one_texel.y);
    float2 tc_front = float2(In.TexCoord.x, In.TexCoord.y + one_texel.y);

    float3 displace_left = g_samplerDisplacementMap.Sample(LinearSampler, tc_left).xyz;
    float3 displace_right = g_samplerDisplacementMap.Sample(LinearSampler, tc_right).xyz;
    float3 displace_back = g_samplerDisplacementMap.Sample(LinearSampler, tc_back).xyz;
    float3 displace_front = g_samplerDisplacementMap.Sample(LinearSampler, tc_front).xyz;
	
	// Do not store the actual normal value. Using gradient instead, which preserves two differential values.
    float2 gradient = { -(displace_right.z - displace_left.z), -(displace_front.z - displace_back.z) };
	

	// Calculate Jacobian corelation from the partial differential of height field
    float2 Dx = (displace_right.xy - displace_left.xy) * g_ChoppyScale * g_GridLen;
    float2 Dy = (displace_front.xy - displace_back.xy) * g_ChoppyScale * g_GridLen;
    float J = (1.0f + Dx.x) * (1.0f + Dy.y) - Dx.y * Dy.x;

	// Practical subsurface scale calculation: max[0, (1 - J) + Amplitude * (2 * Coverage - 1)].
    float fold = max(1.0f - J, 0);

	// Output
    //gradient = float2(1, 0);
   // return float4(1,0, 0, 1);
    return float4(gradient, 0, fold);
}

technique11 T0
{
   
    pass p0
    {
        SetVertexShader(CompileShader(vs_5_0, QuadVS()));
      
        SetPixelShader(CompileShader(ps_5_0, UpdateDisplacementPS()));
    }

    pass p1
    {
        SetVertexShader(CompileShader(vs_5_0, QuadVS()));
      
        SetPixelShader(CompileShader(ps_5_0, GenGradientFoldingPS()));
    }
    pass p2
    {
        SetVertexShader(NULL);

        SetPixelShader(NULL);

        SetComputeShader(CompileShader(cs_5_0, UpdateSpectrumCS()));
    }
}