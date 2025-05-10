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

float3 BinnPhong(float3 normal, float3 lightDirection, float3 viewDirection, float3 lightIrradiance, float3 diffuseColor, float3 specularColor, float roughness)
{
    float3 diffuse = diffuseColor;
    float3 halfVector = normalize(viewDirection + lightDirection);
    float roughnessPhong = (2.0f / (roughness * roughness)) - 2.0f;
    float3 specular = pow(max(dot(normal, halfVector), 0.0), roughnessPhong) * specularColor;
    diffuse *= M_RCPPI;
    specular *= (roughnessPhong + 8.0f) / (8.0f * M_PI);
    float3 retColor = diffuse + specular;
    retColor *= max(dot(normal, lightDirection), 0.0f);
    retColor *= lightIrradiance;
    return retColor;
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