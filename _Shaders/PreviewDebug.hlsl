
cbuffer CB_VP : register(b0)
{
    matrix WVP : packoffset(c0);
  
};

struct VertexColor
{
    float4 Position : POSITION0;
    float4 Color : Color0;
 
};
struct VertexOutput
{
    float4 Position : SV_Position0;
    float4 Color : Color0;
 
};

Texture2D<float4> Input : register(t0);
VertexOutput BoneBoxVS(VertexColor input, uint InstID:SV_InstanceID)
{
  
    VertexOutput output;
    //mul : Çà·Ä°öÀÌÀÚ º¤ÅÍ°ö
    uint index = input.Color.a;
  
    matrix World;
    World._11_12_13_14 = Input[int2(0 + (4 * InstID), index)];
    World._21_22_23_24 = Input[int2(1 + (4 * InstID), index)];
    World._31_32_33_34 = Input[int2(2 + (4 * InstID), index)];
    World._41_42_43_44 = Input[int2(3 + (4 * InstID), index)];
    //output.Position = mul(input.Position, Input[input.Color.a].Bone);
    output.Position = mul(input.Position, World);
    output.Position = mul(output.Position, WVP);
    output.Color = input.Color;
   // output.Color = Input[input.Color.a].BoneColor;
   
    
    return output;
}



VertexOutput DebugVS(VertexColor input)
{
   
    VertexOutput output;
   output.Position = mul(input.Position, WVP);
    
   
    output.Color = input.Color;
   
    return output;
}

float4 DebugPS(VertexOutput In) : SV_Target
{
    // return float4(1, 0, 0, 1);
    return float4(In.Color.rgb, 1.0);

}

