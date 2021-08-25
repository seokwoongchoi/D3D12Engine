
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
    //WVP º¯È¯Àº VS¿¡¼­
    VertexOutput output;
    //mul : Çà·Ä°öÀÌÀÚ º¤ÅÍ°ö
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

