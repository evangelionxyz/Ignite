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

struct Debug
{
    int renderIndex;
};

// push constant buffers
cbuffer CameraBuffer : register(b0) { Camera camera; }
cbuffer DirLightBuffer : register(b1) { DirLight dirLight; }
cbuffer EnvironmentBuffer : register(b2) { Environment env; }
cbuffer ObjectBuffer : register(b3) { Object object; }
cbuffer MaterialBuffer : register(b4) { Material material; }
cbuffer DebugBuffer : register(b5) { Debug debug; }

struct PSInput
{
    float4 position     : SV_POSITION;
    float3 normal       : NORMAL;
    float3 worldPos     : WORLDPOS;
    float2 uv           : TEXCOORD;
    float2 tilingFactor : TILINGFACTOR;
    float4 color        : COLOR;
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

float4 main(PSInput input) : SV_TARGET
{
    float3 lighting = float3(0.0f, 0.0f, 0.0f);

    float3 diffColTex = diffuseTex.Sample(sampler0, input.uv).rgb;
    float3 specColTex = specularTex.Sample(sampler0, input.uv).rgb;
    float3 emissiveCol = emissiveTex.Sample(sampler0, input.uv).rgb;
    float roughColTex = roughnessTex.Sample(sampler0, input.uv).r;
    float3 normalColTex = normalTex.Sample(sampler0, input.uv).rgb;

    float3 normal = normalize(input.normal);
    float3 viewDir = normalize(camera.position.xyz - input.worldPos);
    float3 lightDir = normalize(dirLight.direction.xyz);

    float finalRoughness = max(material.roughness * roughColTex, 0.01f);
    float finalMetallic = clamp(material.metallic, 0.0f, 1.0f);

    float3 albedo = diffColTex * material.baseColor.rgb;
    float3 diffuseColor = albedo * (1.0f - finalMetallic);
    float3 specularColor = lerp(float3(0.04f, 0.04f, 0.04f), albedo, finalMetallic);

    float3 reflectDir = reflect(-viewDir, normal);
    float mipLevel = finalRoughness * 5.0f;
    float3 reflectRadiance = SampleSphericalMap(environTex, sampler0, reflectDir);

    float reflectionStrength = lerp(0.001f, 1.0f, finalMetallic);
    reflectionStrength *= (1.0f - finalRoughness);

    float3 reflectedSpecular = GGXReflect(normal, viewDir, reflectDir, reflectRadiance, specularColor, material.roughness);
    reflectedSpecular *= reflectionStrength;

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

    switch (debug.renderIndex)
    {
        case 0:
        default:
        {
            // ===== TONE MAPPING & OUTPUT =====
            lighting= FilmicTonemap(lighting, env.exposure, env.gamma);
            return float4(lighting, 1.0f);
        }
        case 1:
            return float4(normal * 0.5 + 0.5, 1.0);
        case 2:
            // Debug view for roughness
            return float4(finalRoughness, finalRoughness, finalRoughness, 1.0);
        case 3:
            // Debug view for metallic
            return float4(finalMetallic, finalMetallic, finalMetallic, 1.0);
        case 4:
            // Debug reflection only
            return float4(reflectedSpecular, 1.0);
    }
}