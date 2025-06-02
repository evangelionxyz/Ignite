#include "include/helpers.hlsli"
#include "include/pbr.hlsli"
#include "include/binding_helpers.hlsli"

struct Camera
{
    float4x4 viewProjection;
    float4 position;
};

struct DirLight
{
    float4 color;
    float4 direction;
    float intensity;
    float angularSize;
    float ambientIntensity;
    float shadowStrength;
};

struct Environment
{
    float exposure;
    float gamma;
};

struct Object
{
    float4x4 transformMatrix;
    float4x4 normalMatrix;
};

struct Material
{
    float4 baseColor;
    float metallic;
    float roughness;
    float emissive;
};

// push constant buffers
cbuffer CameraBuffer : register(b0) { Camera camera; }
cbuffer DirLightBuffer : register(b1) { DirLight dirLight; }
cbuffer EnvironmentBuffer : register(b2) { Environment env; }
cbuffer ObjectBuffer : register(b3) { Object object; }
cbuffer MaterialBuffer : register(b4) { Material material; }

struct PSInput
{
    float4 position     : SV_POSITION;
    float3 normal       : NORMAL;
    float3 worldPos     : WORLDPOS;
    float2 uv           : TEXCOORD;
    float2 tilingFactor : TILINGFACTOR;
    float4 color        : COLOR;
    uint entityID       : ENTITYID;
};

Texture2D diffuseTex : register(t0);
Texture2D specularTex : register(t1);
Texture2D emissiveTex : register(t2);
Texture2D roughnessTex : register(t3);
Texture2D normalTex : register(t4);
Texture2D environTex : register(t5);
SamplerState sampler0 : register(s0);

float3 CalcDirLight(float3 ldirection, float3 lcolor, float3 normal, float3 viewDirection, float3 diffTexColor, float shadow)
{
    float3 lightDirection = normalize(-ldirection);
    float3 normalizedViewDir = normalize(viewDirection);
    float ambientStrength = 0.1f;
    float3 ambientColor = ambientStrength * lcolor;
    float diffuse = max(dot(normal, lightDirection), 0.0f);
    float3 diffuseColor = diffuse * lcolor;

    float specularStrength = 0.5f;
    float3 reflectDir = reflect(-lightDirection, normal);
    float specular = pow(max(dot(normalizedViewDir, reflectDir), 0.0f), 32.0f);
    float3 specularColor = specularStrength * specular * lcolor;
    return (ambientColor + diffuseColor + specularColor) * diffTexColor;
}

struct PSOutput
{
    float4 color : SV_TARGET0;
    uint4 entityID : SV_TARGET1;
};

PSOutput main(PSInput input)
{
    float3 lighting = float3(0.0f, 0.0f, 0.0f);

    // ===== TEXTURES =====
    float3 diffColTex = diffuseTex.Sample(sampler0, input.uv).rgb;
    float3 specColTex = specularTex.Sample(sampler0, input.uv).rgb;
    float3 emissiveCol = emissiveTex.Sample(sampler0, input.uv).rgb;
    float roughColTex = roughnessTex.Sample(sampler0, input.uv).r;
    float3 normalColTex = normalTex.Sample(sampler0, input.uv).rgb;

    float3 normal = normalize(input.normal * normalColTex);
    float3 viewDir = normalize(camera.position.xyz - input.worldPos);
    float3 lightDir = normalize(dirLight.direction.xyz);

    // ===== SUN =====
    float sunAngularRadius = dirLight.angularSize * 0.5f;
    float sunSolidAngle = 2.0f * M_PI * (1.0f - cos(sunAngularRadius)); // Steradians
    
    // ===== ROUGHNESS =====
    float minRoughness = saturate(sqrt(sunSolidAngle / M_PI)); // Convert to minimum roughness
    float filteredRoughness = max(material.roughness * roughColTex, minRoughness);

    float finalMetallic = clamp(material.metallic, 0.0f, 1.0f);
    float3 albedo = diffColTex * material.baseColor.rgb;
    float3 diffuseColor = albedo * (1.0f - finalMetallic);
    float3 specularColor = lerp(float3(0.04f, 0.04f, 0.04f), albedo, finalMetallic);
    
    float3 reflectDir = reflect(-viewDir, normal);
    float mipLevel = filteredRoughness * 5.0f;
    float3 reflectRadiance = SampleSphericalMap(environTex, sampler0, reflectDir);
    reflectRadiance = reflectRadiance / (reflectRadiance + 1.0f); // soft clamp (ACES-like)

    float reflectionStrength = lerp(0.001f, 1.0f, finalMetallic);
    reflectionStrength *= (1.0f - filteredRoughness);

    float3 F = SchlickFresnel(viewDir, normal, specularColor);
    float NdotR = saturate(dot(normal, reflectDir));
    float3 reflectedSpecular = GGXReflect(normal, viewDir, reflectDir, reflectRadiance, specularColor, material.roughness);
    reflectedSpecular *= reflectionStrength * dirLight.intensity * NdotR * F;

    float3 ambient = dirLight.color.rgb * dirLight.ambientIntensity * diffuseColor;
    float3 irradiance = dirLight.color.rgb * dirLight.intensity;
    
    lighting = GGX(
        normal,
        lightDir,
        viewDir,
        irradiance,
        diffuseColor,
        specularColor,
        material.roughness
    );

    lighting += ambient;
    lighting += reflectedSpecular;

    // Add emissive
    float3 emissive = TextureEmissive(emissiveCol, material.emissive);
    lighting += emissive;
    
    PSOutput result;
    lighting = FilmicTonemap(lighting, env.exposure, env.gamma);
    result.color = float4(lighting, 1.0);
    result.entityID = uint4(input.entityID, input.entityID, input.entityID, input.entityID);
    
    return result;
}