#include "include/tonemapping.hlsli"
#include "include/srgb_to_linear.hlsli"
#include "include/pbr.hlsli"

cbuffer GlobalConstants : register(b0)
{
    float4x4 viewProjection;
    float4 cameraPosition;
};

cbuffer ModelConstants : register(b1)
{
    float4x4 transformMatrix;
    float4x4 normalMatrix;
}

cbuffer MaterialConstants : register(b2)
{
    float4 baseColor;
    float4 diffuseColor;
    float emissive;
}

cbuffer DirLightConstants : register(b3)
{
    float4 DLDirection;
    float4 DLColor;
}

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

    float3 diffCol = diffuseTex.Sample(sampler0, input.uv).rgb;
    float3 specCol = specularTex.Sample(sampler0, input.uv).rgb;
    float3 emissiveCol = emissiveTex.Sample(sampler0, input.uv).rgb;
    float3 roughCol = roughnessTex.Sample(sampler0, input.uv).rgb;

    float3 normal = normalize(input.normal);
    float3 viewDir = normalize(cameraPosition.xyz - input.worldPos);

    // directional light
    float3 lightDir = normalize(float3(0.0f, 1.0f, 0.4f));
    float3 lightColor = float3(1.0f, 0.8f, 0.9f);

    // PBR parameters
    float roughness = 1.0f;
    float3 irradiance = lightColor * 1.0f; // light intensity

    lighting = BinnPhong(
        normal,
        lightDir,
        viewDir,
        irradiance,
        diffCol,
        specCol,
        roughness
    );

    float exposure = 1.0f;
    float gamma = 2.2f;
    float3 toneMapped = tonemap(float4(lighting, 1.0f), exposure, gamma).rgb;
    toneMapped = SRGBToLinear(toneMapped.rgb);

    return float4(toneMapped * (diffCol + emissiveCol) * baseColor.rgb, 1.0f);

    // normal visual debug
    // return float4(normal * 0.5 + 0.5, 1.0); // Visual debug
}