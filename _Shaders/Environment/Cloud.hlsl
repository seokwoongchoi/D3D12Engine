static const float ONE = 0.00390625;
static const float ONEHALF = 0.001953125;
static const float2 FirstOffset = float2(0.0, 0.0);
SamplerState LinearSampler : register(s0);


cbuffer CB_VS : register(b0)
{
    float4x4 WVP : packoffset(c0);
   // float CloudTiles; //vs
};


cbuffer CB_PS:register(b0)
{
    float Time : packoffset(c0.x);
    float CloudCover : packoffset(c0.y); //ps
    float CloudSharpness : packoffset(c0.z); //ps
    float CloudSpeed : packoffset(c0.w); //ps
    float2 SecondOffset : packoffset(c1.x); //ps
    float2 CloudTiles : packoffset(c1.z); //ps
   
};
Texture2D<float4> CloudMap : register(t0);
Texture2D Cloud1 : register(t1);
Texture2D Cloud2 : register(t2);

struct VertexTexture
{
    float4 Position : Position;
    float2 Uv : Uv;
};


struct VertexOutput_Cloud
{
    float4 Position : SV_Position0;
     float2 Uv : Uv0;
  
};
float Fade(float t)
{
  //return t * t * (3.0 - 2.0 * t);
    return t * t * t * (t * (t * 6.0 - 15.0) + 10.0);
}

float Noise(float2 P)
{
   

    float2 Pi = ONE * floor(P) + ONEHALF;
    float2 Pf = frac(P);

   
    float2 grad00 = CloudMap.Sample(LinearSampler, Pi).rg * 4.0 - 1.0;
    float n00 = dot(grad00, Pf);

    float2 grad10 = CloudMap.Sample(LinearSampler, Pi + float2(ONE, 0.0)).rg * 4.0 - 1.0;
    float n10 = dot(grad10, Pf - float2(1.0, 0.0));

    float2 grad01 = CloudMap.Sample(LinearSampler, Pi + float2(0.0, ONE)).rg * 4.0 - 1.0;
    float n01 = dot(grad01, Pf - float2(0.0, 1.0));

    float2 grad11 = CloudMap.Sample(LinearSampler, Pi + float2(ONE, ONE)).rg * 4.0 - 1.0;
    float n11 = dot(grad11, Pf - float2(1.0, 1.0));

    float2 n_x = lerp(float2(n00, n01), float2(n10, n11), Fade(Pf.x));

    float n_xy = lerp(n_x.x, n_x.y, Fade(Pf.y));

    return n_xy;
}

float4 MakeCloudUseTexture(float2 uv)
{
    float2 sampleLocation;
    float4 textureColor1;
    float4 textureColor2;
    float4 finalColor;
    
    float n = Noise(uv);
    float n2 = Noise(uv);
    float n3 = Noise(uv);
    float n4 = Noise(uv);
    
     float nFinal = n + (n2 / 2) + (n3 / 4) + (n4 / 8);
 
    float c = CloudCover - nFinal;
      
    c = lerp(c*1.5 , 0, c < 0);

    float density = 1.0 - pow(CloudSharpness, c);
    // Translate the position where we sample the pixel from using the first texture translation values.
    sampleLocation.x = uv.x + FirstOffset.x + density;
    sampleLocation.y = uv.y + FirstOffset.y + density;

    // Sample the pixel color from the first cloud texture using the sampler at this texture coordinate location.
    textureColor1 = Cloud1.Sample(LinearSampler, sampleLocation);
    
    // Translate the position where we sample the pixel from using the second texture translation values.
    sampleLocation.x = uv.x + SecondOffset.x+density;
    sampleLocation.y = uv.y + SecondOffset.y+density;

    // Sample the pixel color from the second cloud texture using the sampler at this texture coordinate location.
    textureColor2 = Cloud2.Sample(LinearSampler, sampleLocation);

    // Combine the two cloud textures evenly.
    finalColor = lerp(textureColor1, textureColor2, density);

    // Reduce brightness of the combined cloud textures by the input brightness value.
    finalColor = finalColor * density;

    return finalColor;
}
VertexOutput_Cloud VS_Cloud(VertexTexture input)
{
    VertexOutput_Cloud output;

    output.Position = mul(input.Position, WVP);
     
    output.Uv = input.Uv;
   

    return output;
}


float4 PS_Cloud(VertexOutput_Cloud input) : SV_Target0
{ 
    input.Uv = input.Uv * CloudTiles;
     float2 textureUV = input.Uv + Time * CloudSpeed;
    float4 color = MakeCloudUseTexture(textureUV) ;
    return color*0.85f;
 
}