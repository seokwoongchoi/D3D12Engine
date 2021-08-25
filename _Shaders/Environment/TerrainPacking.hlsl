

//--------------------------------------------------------------------------------------
// Textures
//--------------------------------------------------------------------------------------

Texture2D LayerdefTexture : register(t0); //DS
Texture2D RockBumpTexture : register(t1); //ds
Texture2D SandBumpTexture : register(t2); //ds
Texture2D HeightfieldTextureDS : register(t3); // DS
Texture2D WaterNormalMapTexture : register(t4); //ds

Texture2D HeightfieldTextureHS : register(t0); //HS 


Texture2D RockMicroBumpTexture : register(t6); //ps
Texture2D RockDiffuseTexture : register(t7); //ps
Texture2D SandMicroBumpTexture : register(t8); //ps
Texture2D SandDiffuseTexture : register(t9); //ps
Texture2D GrassDiffuseTexture : register(t10); //ps
Texture2D SlopeDiffuseTexture : register(t11); //ps





SamplerState SamplerLinearWrap : register(s0); // ds 
SamplerState SamplerAnisotropicWrap : register(s1); //ps



//--------------------------------------------------------------------------------------
// Shader Inputs/Outputs
//--------------------------------------------------------------------------------------


struct PSIn_Diffuse
{
    float4 position : SV_Position;
    centroid float2 texcoord : TEXCOORD0;
    centroid float3 normal : NORMAL;
    centroid float3 positionWS : TEXCOORD1;
    centroid float4 layerdef : TEXCOORD2;
    centroid float4 depthmap_scaler : TEXCOORD3;
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

cbuffer CB_HS:register(b5)
{
 
    float4x4 ModelViewProjectionMatrixHS : packoffset(c0);
    float3 CameraPositionHS : packoffset(c4);

    float3 CameraDirection : packoffset(c5);
   
};


cbuffer CB_DS : register(b6)
{
    float4x4 ModelViewProjectionMatrixDS : packoffset(c0);
    float3 LightPositionDS : packoffset(c4);
   
    float3 CameraPositionDS : packoffset(c5);
    float RenderCaustics : packoffset(c5.w);
  
};
cbuffer CB_PS : register(b7)
{
    float3 LightPositionPS: packoffset(c0);
    float3 CameraPositionPS : packoffset(c1);
   
  
};
//--------------------------------------------------------------------------------------
// Misc functions
//--------------------------------------------------------------------------------------
// constants defining visual appearance
static const float2 DiffuseTexcoordScale = { 130.0, 130.0 };
static const float2 RockBumpTexcoordScale = { 10.0, 10.0 };
static const float RockBumpHeightScale = 3.0;
static const float2 SandBumpTexcoordScale = { 3.5, 3.5 };
static const float SandBumpHeightScale = 0.5;
static const float WaterHeightBumpScale = 1.0f;
static const  float  DynamicTessFactor = 50.0f;
static const float3 AtmosphereBrightColor = { 1.0, 1.1, 1.4 };
static const float3 AtmosphereDarkColor = { 0.6, 0.6, 0.7 };
static const float FogDensity = 1.0f / 700.0f;
static const float HeightFieldSize = 512;


// calculating tessellation factor. It is either constant or hyperbolic depending on UseDynamicLOD switch
float CalculateTessellationFactor(float distance)
{
    return  DynamicTessFactor * (1 / (0.015 * distance));
}

// to avoid vertex swimming while tessellation varies, one can use mipmapping for displacement maps
// it's not always the best choice, but it effificiently suppresses high frequencies at zero cost
float CalculateMIPLevelForDisplacementTextures(float distance)
{
    return log2(128 / CalculateTessellationFactor(distance));
}

// primitive simulation of non-uniform atmospheric fog
float3 CalculateFogColor(float3 pixel_to_light_vector, float3 pixel_to_eye_vector)
{
    return lerp(AtmosphereDarkColor, AtmosphereBrightColor, 0.5 * dot(pixel_to_light_vector, -pixel_to_eye_vector) + 0.5);
}


// calculating water refraction caustics intensity
float CalculateWaterCausticIntensity(float3 worldpos)//DS
{

    float distance_to_camera = length(CameraPositionDS - worldpos);

    float2 refraction_disturbance;
    float3 n;
    float m = 0.2;
    float cc = 0;
    float k = 0.15;
    float water_depth = 0.5 - worldpos.y;

    float3 pixel_to_light_vector = normalize(LightPositionDS - worldpos);

    worldpos.xz -= worldpos.y * pixel_to_light_vector.xz;
    float3 pixel_to_water_surface_vector = pixel_to_light_vector * water_depth;
    float3 refracted_pixel_to_light_vector;

	// tracing approximately refracted rays back to light
    [unroll(6)]
    for (float i = -3; i <= 3; i += 1)
        [unroll(6)]
        for (float j = -3; j <= 3; j += 1)
        {
            int2 uv = (worldpos.xz - CameraPositionDS.xz - float2(200.0, 200.0) + float2(i * k, j * k) * m * water_depth) / 400.0;
            n = 2.0f * WaterNormalMapTexture[uv].rgb - float3(1.0f, 1.0f, 1.0f);
            refracted_pixel_to_light_vector = m * (pixel_to_water_surface_vector + float3(i * k, 0, j * k)) - 0.5 * float3(n.x, 0, n.z);
            cc += 0.05 * max(0, pow(max(0, dot(normalize(refracted_pixel_to_light_vector), normalize(pixel_to_light_vector))), 500.0f));
        }
    return cc;
}




//--------------------------------------------------------------------------------------
// Heightfield shaders
//--------------------------------------------------------------------------------------

HSIn_Heightfield PassThroughVS(float4 PatchParams : PATCH_PARAMETERS)
{
    HSIn_Heightfield output;
    output.origin = PatchParams.xy;
    output.size = PatchParams.zw;
    return output;
}

PatchData PatchConstantHS(InputPatch<HSIn_Heightfield, 1> inputPatch)
{
    PatchData output;

    float distance_to_camera;
    float tesselation_factor;
    float inside_tessellation_factor = 0;
    float in_frustum = 0;

    output.origin = inputPatch[0].origin;
    output.size = inputPatch[0].size;

    float2 texcoord0to1 = (inputPatch[0].origin + inputPatch[0].size / 2.0) / HeightFieldSize;
    texcoord0to1.y = 1 - texcoord0to1.y;
	
	// conservative frustum culling
    float3 patch_center = float3(inputPatch[0].origin.x + inputPatch[0].size.x * 0.5, HeightfieldTextureHS[int2(texcoord0to1)].w, inputPatch[0].origin.y + inputPatch[0].size.y * 0.5);
    float3 camera_to_patch_vector = patch_center - CameraPositionHS;
    float3 patch_to_camera_direction_vector = CameraDirection * dot(camera_to_patch_vector, CameraDirection) - camera_to_patch_vector;
    float3 patch_center_realigned = patch_center + normalize(patch_to_camera_direction_vector) * min(2 * inputPatch[0].size.x, length(patch_to_camera_direction_vector));
    float4 patch_screenspace_center = mul(float4(patch_center_realigned, 1.0), ModelViewProjectionMatrixHS);

    [branch]
    if (((patch_screenspace_center.x / patch_screenspace_center.w > -1.0) && (patch_screenspace_center.x / patch_screenspace_center.w < 1.0)
		&& (patch_screenspace_center.y / patch_screenspace_center.w > -1.0) && (patch_screenspace_center.y / patch_screenspace_center.w < 1.0)
		&& (patch_screenspace_center.w > 0)) || (length(patch_center - CameraPositionHS) < 2 * inputPatch[0].size.x))
    {
        in_frustum = 1;
    }
    
  [branch]
    if ((in_frustum==false))
    {
        output.Edges[0] = -1;
        output.Edges[1] = -1;
        output.Edges[2] = -1;
        output.Edges[3] = -1;
        output.Inside[0] = -1;
        output.Inside[1] = -1;
        return output;
    }
    

    distance_to_camera = length(CameraPositionHS.xz - inputPatch[0].origin - float2(0, inputPatch[0].size.y * 0.5));
    tesselation_factor = CalculateTessellationFactor(distance_to_camera);
    output.Edges[0] = tesselation_factor;
    inside_tessellation_factor += tesselation_factor;


    distance_to_camera = length(CameraPositionHS.xz - inputPatch[0].origin - float2(inputPatch[0].size.x * 0.5, 0));
    tesselation_factor = CalculateTessellationFactor(distance_to_camera);
    output.Edges[1] = tesselation_factor;
    inside_tessellation_factor += tesselation_factor;

    distance_to_camera = length(CameraPositionHS.xz - inputPatch[0].origin - float2(inputPatch[0].size.x, inputPatch[0].size.y * 0.5));
    tesselation_factor = CalculateTessellationFactor(distance_to_camera);
    output.Edges[2] = tesselation_factor;
    inside_tessellation_factor += tesselation_factor;

    distance_to_camera = length(CameraPositionHS.xz - inputPatch[0].origin - float2(inputPatch[0].size.x * 0.5, inputPatch[0].size.y));
    tesselation_factor = CalculateTessellationFactor(distance_to_camera);
    output.Edges[3] = tesselation_factor;
    inside_tessellation_factor += tesselation_factor;
    output.Inside[0] = output.Inside[1] = inside_tessellation_factor * 0.25;
    return output;
}

[domain("quad")]
[partitioning("fractional_odd")]
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
    float4 base_texvalue;
    float2 texcoord0to1 = (input.origin + uv * input.size) / HeightFieldSize;
    float3 base_normal;
    float3 detail_normal;
    float3 detail_normal_rotated;
    float4 detail_texvalue;
    float detail_height;
    float3x3 normal_rotation_matrix;
    float4 layerdef;
    float distance_to_camera;
    float detailmap_miplevel;
    texcoord0to1.y = 1 - texcoord0to1.y;
	
	// fetching base heightmap,normal and moving vertices along y axis
    base_texvalue = HeightfieldTextureDS.SampleLevel(SamplerLinearWrap, texcoord0to1, 0);
    base_normal = base_texvalue.xyz;
    base_normal.z = -base_normal.z;
    vertexPosition.xz = input.origin + uv * input.size;
    vertexPosition.y = base_texvalue.w;

	// calculating MIP level for detail texture fetches
    distance_to_camera = length(CameraPositionDS - vertexPosition);
    detailmap_miplevel = CalculateMIPLevelForDisplacementTextures(distance_to_camera); //log2(1+distance_to_camera*3000/(HeightFieldSize*TessFactor));
	
	// fetching layer definition texture
    layerdef = LayerdefTexture[texcoord0to1];
	
	// default detail texture
    detail_texvalue = SandBumpTexture.SampleLevel(SamplerLinearWrap, texcoord0to1 * SandBumpTexcoordScale, detailmap_miplevel).rbga;
    detail_normal = normalize(2 * detail_texvalue.xyz - float3(1, 0, 1));
    detail_height = (detail_texvalue.w - 0.5) * SandBumpHeightScale;

	// rock detail texture
    detail_texvalue = RockBumpTexture.SampleLevel(SamplerLinearWrap, texcoord0to1 * RockBumpTexcoordScale, detailmap_miplevel).rbga;
    detail_normal = lerp(detail_normal, normalize(2 * detail_texvalue.xyz - float3(1, 1.4, 1)), layerdef.w);
    detail_height = lerp(detail_height, (detail_texvalue.w - 0.5) * RockBumpHeightScale, layerdef.w);

	// moving vertices by detail height along base normal
    vertexPosition += base_normal * detail_height;

	//calculating base normal rotation matrix
    normal_rotation_matrix[1] = base_normal;
    normal_rotation_matrix[2] = normalize(cross(float3(-1.0, 0.0, 0.0), normal_rotation_matrix[1]));
    normal_rotation_matrix[0] = normalize(cross(normal_rotation_matrix[2], normal_rotation_matrix[1]));

	//applying base rotation matrix to detail normal
    detail_normal_rotated = mul(detail_normal, normal_rotation_matrix);

	//adding refraction caustics
    float cc = 0;
	[flatten]
    if ((RenderCaustics > 0)) // doing it only for main
    {
        cc = CalculateWaterCausticIntensity(vertexPosition.xyz);
    }
	
	// fading caustics out at distance
    cc *= (200.0 / (200.0 + distance_to_camera));

	// fading caustics out as we're getting closer to water surface
    cc *= min(1, max(0, -WaterHeightBumpScale - vertexPosition.y));


	// writing output params
    output.position = mul(float4(vertexPosition, 1.0), ModelViewProjectionMatrixDS);
    output.texcoord = texcoord0to1 * DiffuseTexcoordScale;
    output.normal = detail_normal_rotated;
    output.positionWS = vertexPosition;
    output.layerdef = layerdef;
    output.depthmap_scaler = float4(1.0, 1.0, detail_height, cc);

    return output;
}


struct PS_GBUFFER_OUT
{
    float4 ColorSpecInt : SV_Target0;
    float4 Specular : SV_Target1;
    float4 Normal : SV_Target2;
};

PS_GBUFFER_OUT PackGBuffer(float3 BaseColor, float4 Specular, float3 Normal, float metallic, float roughness, float terrainMask = 1)
{
    PS_GBUFFER_OUT Out;
    Out.ColorSpecInt = float4(BaseColor.rgb, metallic);
    Out.Specular = float4(Specular.rgb, terrainMask);
    Out.Normal = float4(Normal.rgb * 0.5 + 0.5, roughness);
    return Out;
}
PS_GBUFFER_OUT HeightFieldPatchPacking(PSIn_Diffuse input)
{
    float4 color;
    float3 pixel_to_light_vector = normalize(LightPositionPS - input.positionWS);
    float3 pixel_to_eye_vector = normalize(CameraPositionPS - input.positionWS);
    float3 microbump_normal;

    float3x3 normal_rotation_matrix;

	
	
	// fetching default microbump normal
    microbump_normal = normalize(2 * SandMicroBumpTexture.Sample(SamplerAnisotropicWrap, input.texcoord).rbg - float3(1.0, 1.0, 1.0));
    microbump_normal = normalize(lerp(microbump_normal, 2 * RockMicroBumpTexture.Sample(SamplerAnisotropicWrap, input.texcoord).rbg - float3(1.0, 1.0, 1.0), input.layerdef.w));

	//calculating base normal rotation matrix
    normal_rotation_matrix[1] = input.normal;
    normal_rotation_matrix[2] = normalize(cross(float3(-1.0, 0.0, 0.0), normal_rotation_matrix[1]));
    normal_rotation_matrix[0] = normalize(cross(normal_rotation_matrix[2], normal_rotation_matrix[1]));
    microbump_normal = mul(microbump_normal, normal_rotation_matrix);

	// getting diffuse color
    color = SlopeDiffuseTexture.Sample(SamplerAnisotropicWrap, input.texcoord);
    color = lerp(color, SandDiffuseTexture.Sample(SamplerAnisotropicWrap, input.texcoord), input.layerdef.g * input.layerdef.g);
    color = lerp(color, RockDiffuseTexture.Sample(SamplerAnisotropicWrap, input.texcoord), input.layerdef.w * input.layerdef.w);
    color = lerp(color, GrassDiffuseTexture.Sample(SamplerAnisotropicWrap, input.texcoord), input.layerdef.b);

	// adding per-vertex lighting defined by displacement of vertex 
    color *= 0.5 + 0.5 * min(1.0, max(0.0, input.depthmap_scaler.b / 3.0f + 0.5f));

    
    float shadow_factor = 1.0f;
    color.rgb *= max(0, dot(pixel_to_light_vector, microbump_normal)) * shadow_factor + 0.2;


	// adding light from the sky
    color.rgb += (0.0 + 0.2 * max(0, (dot(float3(0, 1, 0), microbump_normal)))) * float3(0.2, 0.2, 0.3);

	// making all a bit brighter, simultaneously pretending the wet surface is darker than normal;
    color.rgb *= 0.5 + 0.8 * max(0, min(1, input.positionWS.y * 0.5 + 0.5));



	// applying refraction caustics
    color.rgb *= (1.0 + max(0, 0.4 + 0.6 * dot(pixel_to_light_vector, microbump_normal)) * input.depthmap_scaler.a * (0.4 + 0.6 * shadow_factor));

	// applying fog
   
    float fogDensity = 1 / 7000.0f;
    color.rgb = lerp(CalculateFogColor(pixel_to_light_vector, pixel_to_eye_vector).rgb, color.rgb, min(1, exp(-length(CameraPositionPS - input.positionWS) * fogDensity)));
    color.a = length(CameraPositionPS - input.positionWS);
   //return color;
    return PackGBuffer(color.rgb, float4(1, 1, 1, 1), normalize(microbump_normal),  1, 1, 0);
}

//--------------------------------------------------------------------------------------
// Water shaders
//--------------------------------------------------------------------------------------

