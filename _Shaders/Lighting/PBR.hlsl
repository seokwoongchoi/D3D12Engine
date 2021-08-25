
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////PBR//////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////

float3 RealSpecularColor(float3 diffuse, float metallic)
{
    float3 g_diffuse = pow(diffuse, 2.2f);
    float3 realSpecularColor = lerp(0.03f, g_diffuse, metallic);
    
    return realSpecularColor;

}
float NdotL(float3 bump, float3 lightDir)
{
    return saturate(dot(bump, lightDir));
}

float NdotH(float3 bump, float3 H)
{
    return saturate(dot(bump, H));
}

float NdotV(float3 bump, float3 viewDir)
{
    return saturate(dot(bump, viewDir));
}

float VdotH(float3 H, float3 viewDir)
{
    return saturate(dot(viewDir, H));
}

float LdotH(float3 H, float3 lightDir)
{
    return saturate(dot(lightDir, H));
}

float3 F0(float3 diffuse, float metallic)
{
    float3 g_diffuse = pow(diffuse, 2.2f).rgb;
    float3 F0 = float3(0.08f, 0.08f, 0.08f);
    F0 = lerp(F0, g_diffuse, metallic);
    
    return F0;
}

float Alpha(float roughness)
{
    float alpha = max(0.001f, roughness * roughness);
    return alpha;
}
float3 FresnelSchlickRoughness(float cosTheta, float3 F0, float roughness)   // cosTheta is n.v and F0 is the base reflectivity
{
    return F0 + (max(float3(1.0f - roughness, 1.0f - roughness, 1.0f - roughness), F0) - F0) * pow(1.0 - cosTheta, 5.0f);
}

float FresnelSchlick(float f0, float fd90, float view)
{
    return f0 + (fd90 - f0) * pow(max(1.0f - view, 0.1f), 5.0f);
}
float Disney(float NdotL, float LdotH, float NdotV, float roughness)
{
   
    float energyBias = lerp(0.0f, 0.5f, roughness);
    float energyFactor = lerp(1.0f, 1.0f / 1.51f, roughness);
    float fd90 = energyBias + 2.0f * (LdotH * LdotH) * roughness;
    float f0 = 1.0f;

    float lightScatter = FresnelSchlick(f0, fd90, NdotL);
    float viewScatter = FresnelSchlick(f0, fd90, NdotV);

    return lightScatter * viewScatter * energyFactor;
}
static const float PI = 3.14159265f;

float3 GGX(float NdotL, float NdotH, float NdotV, float VdotH, float LdotH, float alpha, float3 realSpecularColor)
{
    
   
    
    float rough4 = alpha * alpha;

    float ndoth = max(NdotH, 0.0f);
    float NdotH2 = ndoth * ndoth;
    float nom = rough4;
    float denom = (NdotH2 * (rough4 - 1.0f) + 1.0f);
    denom = PI * denom * denom;

  
   // float d = (NdotH * rough4 - NdotH) * NdotH + 1.0f;
   // float D = rough4 / (PI * (d * d));
    float D = nom / denom;
    float vdoth = max(VdotH, 0.0f);
   
	// Fresnel

   
    float3 reflectivity = realSpecularColor;
    float fresnel = 1.0;
   
   // float3 F = reflectivity + (fresnel - fresnel * reflectivity) * exp2((-5.55473f * LdotH - 6.98316f) * LdotH);
    float3 F = (reflectivity + (1.0f - reflectivity) * pow(1.0 - VdotH, 5.0f));
	// geometric / visibility
    //float k = alpha * 0.5f;
    //float G_SmithL = NdotL * (1.0f - k) + k;
    //float G_SmithV = NdotV * (1.0f - k) + k;
    //float G = 0.25f / (G_SmithL * G_SmithV);
   
    float ndotv = max(NdotV, 0.0f);
    float ndotl = max(NdotL, 0.0f);

    float r = alpha + 1.0f;
    float k = (r * r) / 8.0f;

    float nom1 = NdotV;
    float denom1 = NdotV * (1.0f - k) + k;
    float ggx1 = nom1 / denom1;

    float nom2 = NdotL;
    float denom2 = NdotL * (1.0f - k) + k;
    float ggx2 = nom2 / denom2;
   
    
    float G = ggx1 * ggx2;
    return G * D * F;
   
}
float3 FinalGamma(float3 color)
{
    return pow(color, 1.0f /2.2f);
}

float3 RealAlbedo(float3 diffuse, float metallic)
{
    float3 g_diffuse = pow(diffuse, 2.2f);
  
    float3 realAlbedo = g_diffuse - (g_diffuse  * metallic);
    realAlbedo = saturate(realAlbedo);
    
    return realAlbedo;

}
float LightFactor(float lightDirY)
{
    
    float lightFactor = saturate(lightDirY * 10 + 1);
    
    return lightFactor;

}


float DistributionGGX(float NdotH, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float demon = NdotH * NdotH * (a2 - 1.0f) + 1.0f;
    demon = PI * demon * demon;
    return a2 / max(demon, 0.0000001); // Prevent divide by 0
}
float GeometrySmith(float NdotV, float NdotL, float roughness)
{
    float r = roughness + 1.0f;
    float k = (r * r) / 8.0f;
    float ggx1 = NdotV / (NdotV * (1.0f - k) + k); // Schlick GGX
    float ggx2 = NdotL / (NdotL * (1.0f - k) + k);
    return ggx1 * ggx2;
}
float3 FresnelSchlick(float HdotV, float3 baseReflectivity)
{
	// Base reflectivity in range from 0 to 1
	// returns range of base reflectivity to 1
	// inclreases as HdotV decreases (more reflectiviy when surface viewed at larger angles)
    return baseReflectivity + (1.0f - baseReflectivity) * pow(1.0f - HdotV, 5.0f);
}
float3 FresnelSchlickRoughness2(float HdotV, float3 F0, float roughness)
{
	// Base reflectivity in range from 0 to 1
	// returns range of base reflectivity to 1
	// inclreases as HdotV decreases (more reflectiviy when surface viewed at larger angles)
    return F0 + (max(float3(1.0f - roughness, 1.0f - roughness, 1.0f - roughness), F0) - F0) * pow(1.0f - HdotV, 5.0f);
}