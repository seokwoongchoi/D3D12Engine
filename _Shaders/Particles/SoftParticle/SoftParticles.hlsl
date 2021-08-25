//--------------------------------------------------------------------------------------
// File: SoftParticles10.1.fx
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

// structs
struct VSSceneIn
{
    float3 Pos            : POSITION;
    float3 Norm           : NORMAL;  
    float2 Tex            : TEXCOORD;
    float3 Tan            : TANGENT;
};

struct PSSceneIn
{
    float4 Pos : SV_Position;
    float3 Norm			  : NORMAL;
    float2 Tex			  : TEXCOORD0;
    float3 Tan			  : TEXCOORD1;
    float3 vPos			  : TEXCOORD2;
};

struct VSParticleIn
{
    float3 Pos			  : POSITION;
    float3 Vel			  : VELOCITY;
    float Life			  : LIFE;
    float Size			  : SIZE;
};

struct GSParticleIn
{
    float4 Pos : SV_Position;
    float Life            : LIFE;	//stage of animation we're in [0..1] 0 is first frame, 1 is last
    float Size            : SIZE;
};

struct PSParticleIn
{
    float4 Pos			  : SV_POSITION;
    float3 Tex			  : TEXCOORD0;
    float2 ScreenTex	  : TEXCOORD1;
    float2 Depth		  : TEXCOORD2;
    float  Size			  : TEXCOORD3;
    float3 worldPos		  : TEXCOORD4;
    float3 particleOrig	  : TEXCOORD5;
    float3 particleColor  : TEXCOORD6;
};

cbuffer cbPerObject : register(b0) //gs
{
    matrix WorldViewProj;
    matrix World;
    matrix InvView;
   
};

cbuffer cbUser : register(b1) //ps
{
    matrix InvProj;
    matrix WorldView;
    float4 OctaveOffsets[4];
    float3 ViewDir;
    float FadeDistance;
};

static const float3 positions[4] =
{
    float3(-1, 1, 0),
        float3(1, 1, 0),
        float3(-1, -1, 0),
        float3(1, -1, 0),
};
static const float2 texcoords[4] =
{
    float2(0, 0),
        float2(1, 0),
        float2(0, 1),
        float2(1, 1),
};
    




Texture2D txColorGradient : register(t0); //gs
Texture3D txVolumeDiff : register(t1); //ps
Texture2D txDepth : register(t2); //ps


SamplerState samLinearClamp : register(s0);//GS
SamplerState samVolume : register(s0); //ps
SamplerState samPoint : register(s1); //ps




GSParticleIn VS(VSParticleIn input)
{
    GSParticleIn output;
    
    output.Pos = float4(input.Pos, 1.0f);
    output.Life = input.Life;
    output.Size = input.Size;
    
    return output;
}

[maxvertexcount(4)]
void GS(point GSParticleIn input[1], inout TriangleStream<PSParticleIn> SpriteStream)
{
    PSParticleIn output;
    
    float4 orig = mul( input[0].Pos, World );
    output.particleOrig = orig.xyz;
    
   
    if( input[0].Life > -1 )
    {
        float3 particleColor = txColorGradient.SampleLevel( samLinearClamp, float2(input[0].Life,0), 0 );
      
        output.particleColor = particleColor;
      
        [unroll(4)] 
        for(int i=0; i<4; i++)
        {
            float3 position = positions[i]*input[0].Size;
            position = mul( position, (float3x3)InvView ) + input[0].Pos.xyz;
            output.Pos = mul( float4(position,1.0), WorldViewProj );
          
            output.Tex = float3(texcoords[i],input[0].Life);
            output.ScreenTex = output.Pos.xy/output.Pos.w;
        
            output.Depth = output.Pos.zw;
                    
            output.Size = input[0].Size;
                                    
            float4 posWorld = mul( float4(position,1.0), World );
            output.worldPos = posWorld;							
            
            SpriteStream.Append(output);
        }
        SpriteStream.RestartStrip();
    }
}
static const float SizeZScale = 1.0f / (1000.0f - 0.1f);
//static const float FadeDistance=5.0f;
float4 PS(PSParticleIn input) : SV_Target
{
    
    
    float2 screenTex = 0.5 * ((input.ScreenTex) + float2(1, 1));
    screenTex.y = 1 - screenTex.y;
    
    float4 particleSample = txVolumeDiff.Sample(samVolume, input.Tex);
    
    float size = SizeZScale * input.Size; //move the size into the depth buffer space
    float particleDepth = input.Depth.x - size * 2.0 * (particleSample.a); //augment it by the depth stored in the texture
    particleDepth /= input.Depth.y;
        
  
    
       
       
    float depthSample = txDepth.Sample(samPoint, screenTex);
       
        
    float4 depthViewSample = mul(float4(input.ScreenTex, depthSample, 1), InvProj);
    float4 depthViewParticle = mul(float4(input.ScreenTex, particleDepth, 1), InvProj);
        
    float depthDiff = depthViewSample.z / depthViewSample.w - depthViewParticle.z / depthViewParticle.w;
    if (depthDiff < 0)
        discard;
            
    float depthFade = saturate(depthDiff / FadeDistance);
    
        
   
    particleSample.rgb *= (input.particleColor);
    particleSample.a *= depthFade;
  
  
    return particleSample;
}
float4 PreviewPS(PSParticleIn input) : SV_Target
{
    
    
    float2 screenTex = 0.5 * ((input.ScreenTex) + float2(1, 1));
    screenTex.y = 1 - screenTex.y;
    
    float4 particleSample = txVolumeDiff.Sample(samVolume, input.Tex);
   
    
        
   
    particleSample.rgb *= (input.particleColor);
  
  
  
    return particleSample;
}


//#define DIST_BIAS 0.01
//bool RaySphereIntersect(float3 rO, float3 rD, float3 sO, float sR, inout float tnear, inout float tfar)
//{
//    float3 delta = rO - sO;
    
//    float A = dot(rD, rD);
//    float B = 2 * dot(delta, rD);
//    float C = dot(delta, delta) - sR * sR;
    
//    float disc = B * B - 4.0 * A * C;
//    if (disc < DIST_BIAS)
//    {
//        return false;
//    }
//    else
//    {
//        float sqrtDisc = sqrt(disc);
//        tnear = (-B - sqrtDisc) / (2 * A);
//        tfar = (-B + sqrtDisc) / (2 * A);
//        return true;
//    }
//}

//float4 Noise3D(float3 uv, int octaves)
//{
//    float4 noiseVal = float4(0, 0, 0, 0);
//    float4 octaveVal = float4(0, 0, 0, 0);
//    float3 uvOffset;
//    float freq = 1;
//    float pers = 1;

//    for (int i = 0; i < octaves; i++)
//    {
//        uvOffset = uv + OctaveOffsets[i].xyz;
//        octaveVal = txVolumeDiff.SampleLevel(samVolume, uvOffset * freq, 0);
//        noiseVal += pers * octaveVal;
        
//        freq *= 3.0;
//        pers *= 0.5;
//    }
    
//    noiseVal.a = abs(noiseVal.a); //turbulence
    
//    return noiseVal;
//}
//float stepSize = 0.01;
//float noiseSize = 40.0;
//float noiseOpacity = 20.0;
//float4 directional1 = float4(0.992, 1.0, 0.880, 0.0);
////
//// PS for the volume particles
////
//#define MAX_STEPS 8
//float4 PS(PSParticleIn input) : SV_Target
//{
   
//    float2 screenTex = 0.5 * ((input.ScreenTex) + float2(1, 1));
//    screenTex.y = 1 - screenTex.y;
    
//    float depthSample;
    
//    {
//        depthSample = txDepth.Sample(samPoint, screenTex);
//        //depthSample = 1.0f; 

//    }
   
    
//    float4 depthViewSample = mul(float4(input.ScreenTex, depthSample, 1), InvProj);
//    float sampleDepth = depthViewSample.z / depthViewSample.w;
    
//    // ray sphere intersection
//    float3 worldPos = input.worldPos;
//    float3 viewRay = ViewDir;
//    float3 sphereO = input.particleOrig;
//    float rad = input.Size;
//    float tnear, tfar;
    
//    if (!RaySphereIntersect(worldPos, viewRay, sphereO, rad, tnear, tfar))
//        discard;
        
//    float3 worldNear = worldPos + viewRay * tnear;
//    float3 worldFar = worldPos + viewRay * tfar;
//    float4 viewNear = mul(float4(worldNear, 1), WorldView);
//    float4 viewFar = mul(float4(worldFar, 1), WorldView);
//    float currentDepth = viewNear.z / viewNear.w;
//    float farDepth = viewFar.z / viewFar.w;
//    float lifePower = input.Tex.z; //*input.Tex.z;
    
//    float depthDiff = farDepth - sampleDepth;
//    if (depthDiff > 0)	//make sure we don't trace past the depth buffer
//    {
//        // if we do, adjust tfar accordingly
//        tfar -= depthDiff;
//        if (tfar < tnear)
//            discard;
//        worldFar = worldPos + viewRay * tfar;
//        farDepth = sampleDepth;
//    }
 
//    float3 unitTex = (worldNear - sphereO) / (rad);
//    float3 localTexNear, localTexFar;
//    if (false)
//    {
//        localTexNear = (worldNear - sphereO) / (rad * 2) + float3(0.5, 0.5, 0.5);
//        localTexFar = (worldFar - sphereO) / (rad * 2) + float3(0.5, 0.5, 0.5);
//    }
//    else
//    {
//        float fNoiseSizeAdjust = 1 / noiseSize;
//        localTexNear = worldNear * fNoiseSizeAdjust;
//        localTexFar = worldFar * fNoiseSizeAdjust;
//    }
    
//    // trace through the volume texture
//    int iSteps = length(localTexFar - localTexNear) / stepSize;
//    iSteps = min(iSteps, MAX_STEPS - 2) + 2;
//    float3 currentTex = localTexNear;
//    float3 localTexDelta = (localTexFar - localTexNear) / (iSteps - 1);
//    float depthDelta = (farDepth - currentDepth) / (iSteps - 1);
//    float opacityAdjust = noiseOpacity / (iSteps - 1);
//    float lightAdjust = 1.0 / (iSteps - 1);
    
//    float runningOpacity = 0;
//    float4 runningLight = float4(0, 0, 0, 0);
//    for (int i = 0; i < iSteps; i++)
//    {
//        float4 noiseCell = Noise3D(currentTex, 4);
//        noiseCell.xyz += normalize(unitTex);
//        noiseCell.xyz = normalize(noiseCell.xyz);
        
//        // fade out near edges
//        float depthFade = 1;
//       // if (bSoftParticles)
//        {
//            depthFade = saturate((sampleDepth - currentDepth) / FadeDistance);
//        }
        
//        //falloff as well
//        float lenSq = dot(unitTex, unitTex);
//        float falloff = 1.0 - lenSq; //1 - len^2 falloff
        
//        // calculate our local opacity for this point
//        float localOpacity = noiseCell.a * falloff * depthFade;
        
//        // add it to our running total
//        runningOpacity += localOpacity;
        
//        // calc lighting from our gradient map and add it to the running total
//        // dot*0.5 + 0.5 basically makes the dot product wrap around
//        // giving us more of a volumetric lighting effect
//        // Also just use one overhead directional light.  It gives more contrast and looks cooler.
//        float4 localLight = directional1 * saturate(dot(noiseCell.xyz, float3(0, 1, 0)) * 0.5 + 0.5);
        
//        //for rendering the particle alone
//        //float4 localLight = saturate( dot( noiseCell.xyz, float3(0,1,0) )*0.5 + 0.5 );	
                                                                                                     
//        runningLight += localLight;
        
//        currentTex += localTexDelta;
//        unitTex += localTexDelta;
//        currentDepth += depthDelta;
//    }
    
//    float4 col = float4(input.particleColor, 1) * (runningLight * lightAdjust) * 0.8 + 0.2; // + ambient;
//    runningOpacity = saturate(runningOpacity * opacityAdjust) * (1 - lifePower)- 0.5*lifePower;
    
//    //for rendering the particle alone
//    //float4 col = (runningLight*lightAdjust)*0.8 + 0.2;// + ambient;
//    //col.xyz = runningOpacity.rrr;
//    //runningOpacity = 1;
    
//    float4 color = float4(col.xyz, runningOpacity);
//    return color;
//}
