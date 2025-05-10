#include "include/tonemapping.hlsli"
#include "include/srgb_to_linear.hlsli"
#include "include/pbr.hlsli"
#include "include/binding_helpers.hlsli"

struct Camera
{
    float4x4 viewProjection;
    float4 position;
};

struct Object
{
    float4x4 transformMatrix;
    float4x4 normalMatrix;
};

struct Material
{
    float4 baseColor;
    float4 diffuseColor;
    float metallic;
    float roughness;
    float emissive;
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

// push constant buffers
cbuffer CameraBuffer : register(b0) { Camera camera; }
cbuffer ObjectBuffer : register(b1) { Object object; }
cbuffer MaterialBuffer : register(b2) { Material material; }
cbuffer DirLightBuffer : register(b3) { DirLight dirLight; }
cbuffer EnvironmentBuffer : register(b4) { Environment env; }

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
sampler sampler0 : register(s0);

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
    float3 roughColTex = roughnessTex.Sample(sampler0, input.uv).rgb;
    float3 normalColTex = normalTex.Sample(sampler0, input.uv).rgb;

    float3 normal = normalize(input.normal);
    float3 viewDir = normalize(camera.position.xyz - input.worldPos);
    float3 lightDir = normalize(dirLight.direction.xyz);

    // Combine material + texture values
    float3 diffuseColor = diffColTex * material.diffuseColor.rgb;
    float3 specularColor = lerp(float3(0.04f, 0.04f, 0.04f), specColTex, material.metallic);
    float roughness = clamp(material.roughness * roughColTex.r, dirLight.angularSize / 90.0f, 1.0f); // prevent 0 roughness

    // ===== AMBIENT =====
    float3 ambient = dirLight.color.rgb * dirLight.ambientIntensity * diffuseColor;

    // ===== IRRADIANCE =====
    float3 irradiance = dirLight.color.rgb * dirLight.intensity;

    // ===== LIGHTING (GGX) =====
    lighting = GGX(
        normal,
        lightDir,
        viewDir,
        irradiance,
        diffuseColor,
        specularColor,
        roughness
    );

    lighting += ambient;

    // ===== EMISSIVE =====
    float3 emissive = TextureEmissive(emissiveCol, material.emissive);

    // ===== TONE MAPPING & OUTPUT =====
    float3 toneMapped = tonemap(float4(lighting + emissive, 1.0f), env.exposure, env.gamma).rgb;
    toneMapped = SRGBToLinear(toneMapped);

    return float4(toneMapped * material.baseColor.rgb, 1.0f);

    // normal visual debug
    // return float4(normal * 0.5 + 0.5, 1.0); // Visual debug
}