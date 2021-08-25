
cbuffer CB_Box
{
    matrix boxWorld;
};
struct VertexOutput
{
    float4 Position : SV_Position0;
    float4 Color : Color0;
   
};

VertexOutput DebugVS(VertexColor input)
{
    //WVP ��ȯ�� VS����
    VertexOutput output;
    //mul : ��İ����� ���Ͱ�
  //  output.Position = mul(input.Position, boxWorld);
    output.Position = WorldPosition(input.Position);
    output.Position = OrbitViewProjection(output.Position);

    output.Color =input.Color;

   
    
    return output;
}

float4 DebugPS(VertexOutput input) : SV_Target0
{
  

   return input.Color;
   

}

