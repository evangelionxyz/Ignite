#include "include/tonemapping.hlsli"
#include "include/srgb_to_linear.hlsli"

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
    float2 texCoord     : TEXCOORD;
    float2 tilingFactor : TILINGFACTOR;
    float4 color        : COLOR;
};

Texture2D diffuseTexture : register(t0);
sampler sampler0 : register(s0);

#define M_RCPPI 0.31830988618379067153776752674503
#define M_PI 3.1415926535897932384626433832795

float3 LightFalloff(float3 lightIntensity, float3 fallOff, float3 lightPosition, float3 position)
{
    float dist = distance(lightPosition, position);
    float fo = fallOff.x + (fallOff.y * dist) + (fallOff.z * dist * dist);
    return lightIntensity / fo;
}

float3 SchlickFresnel(float3 lightDirection, float3 normal, float3 specularColor)
{
    float LH = dot(lightDirection, normal);
    return saturate(specularColor + (1.0f - specularColor) * pow(1.0f - LH, 5.0f));
}

float TRDistribution(float3 normal, float3 halfVector, float roughness)
{
    float NSq = roughness * roughness;
    float NH = max(dot(normal, halfVector), 0.0f);
    float denom = NH * NH * (NSq - 1.0f) + 1.0f;
    return NSq / (M_PI * denom * denom);
}

float GGXVisiblity(float3 normal, float3 lightDirection, float3 viewDirection, float roughness)
{
    float NL = max(dot(normal, lightDirection), 0.0f);
    float NV = max(dot(normal, viewDirection), 0.0f);
    float RSq = roughness * roughness;
    float RMod = 1.0f - RSq;
    float recipG1 = NL * sqrt(RSq + (RMod * NL * NL));
    float recipG2 = NV * sqrt(RSq + (RMod * NV * NV));
    return 1.0f / (recipG1 * recipG2);
}

float3 GGXReflect(float3 normal, float3 reflectDirection, float3 viewDirection, in float3 reflectRadiance, float3 specularColor, float roughness)
{
    float3 F = SchlickFresnel(reflectDirection, normal, specularColor);
    float V = GGXVisiblity(normal, reflectDirection, viewDirection, roughness);
    float3 retColor = F * V;
    retColor *= 4.0f * dot(viewDirection, normal);
    retColor *= max(dot(normal, reflectDirection), 0.0f);
    retColor *= reflectRadiance;
    return retColor;
}

float3 TextureEmissive(float3 diffuseColor, float emissive)
{
    return emissive * diffuseColor;
}

float3 GGX(float3 normal, float3 lightDirection, float3 viewDirection, float3 lightIrradiance, float3 diffuseColor, float3 specularColor, float roughness)
{
    float3 diffuse = mul(diffuseColor, M_RCPPI);
    float3 halfVector = normalize(viewDirection + lightDirection);
    float3 F = SchlickFresnel(lightDirection, halfVector, specularColor);
    float D = TRDistribution(normal, viewDirection, roughness);
    float V = GGXVisiblity(normal, lightDirection, viewDirection, roughness);
    float3 retColor = diffuse + (F * D * V);
    retColor *= max(dot(normal, lightDirection), 0.0f);
    retColor *= lightIrradiance;
    return retColor;
}

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

    float3 diffCol = diffuseTexture.Sample(sampler0, input.texCoord).rgb;
    float3 normal = normalize(input.normal);
    float3 viewDir = normalize(cameraPosition.xyz - input.worldPos);

    // directional light
    float3 lightDir = normalize(float3(0.0f, 1.0f, 0.4f));
    float3 lightColor = float3(1.0f, 0.8f, 0.9f);

    // PBR parameters
    float roughness = 1.0f;
    float3 specularCol = float3(0.8f, 0.8f, 0.8f); // dielectric specular
    float3 irradiance = lightColor * 2.0f; // light intensity

    lighting = GGX(
        normal,
        lightDir,
        viewDir,
        irradiance,
        diffCol,
        specularCol,
        roughness
    );

    float exposure = 4.5f;
    float gamma = 2.2f;
    float3 toneMapped = tonemap(float4(lighting, 1.0f), exposure, gamma).rgb;
    toneMapped = SRGBToLinear(toneMapped.rgb);

    return float4(toneMapped * diffCol * baseColor.rgb, 1.0f);
}