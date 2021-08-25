
Texture2D<float4> HDRTex : register(t0);
Texture2D<float> DepthTex : register(t1);
Texture2D<float3> NoramlTex : register(t2);


///////////////////////////////////////////////////////////////////////////////////////////////
// Reflection copy
///////////////////////////////////////////////////////////////////////////////////////////////

static const float2 arrBasePos[4] = {
	float2(1.0, 1.0),
	float2(1.0, -1.0),
	float2(-1.0, 1.0),
	float2(-1.0, -1.0),
};
//static const float2 arrBasePos[4] =
//{
//    float2(-1.0, 1.0),
//	float2(1.0, 1.0),
//	float2(-1.0, -1.0),
//	float2(1.0, -1.0),
//};

float4 ReflectionBlendVS( uint VertexID : SV_VertexID ) : SV_Position
{
	// Return the quad position
	return float4( arrBasePos[VertexID].xy, 0.0, 1.0);
}

float4 ReflectionBlendPS(float4 Position : SV_Position) : SV_TARGET
{
    	return HDRTex.Load(int3(Position.xy, 0));
}
BlendState blendState
{
  
    BlendEnable[0] = true;
    DestBlend[0] = SRC_Alpha;
//SRC_COLOR;
    SrcBlend[0] = INV_SRC_Alpha;
    BlendOp[0] = Add;

    DestBlendAlpha[0] = SRC_Alpha;
    SrcBlendAlpha[0] = INV_SRC_Alpha;
    BlendOpAlpha[0] = Add;
    RenderTargetWriteMask[0] = 15;
};
technique11 T0
{
    
 ////////////////////////////////   /*Shadow*/   /////////////////////////////////
    pass P0
    {
        SetBlendState(blendState, float4(0, 0, 0, 0), 0xFF);
        SetVertexShader(CompileShader(vs_5_0, ReflectionBlendVS()));
        SetPixelShader(CompileShader(ps_5_0, ReflectionBlendPS()));
    }
}