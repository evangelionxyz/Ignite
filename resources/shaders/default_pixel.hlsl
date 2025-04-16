struct PSInput
{
    float4 position     : SV_POSITION;
    float3 normal       : NORMAL;
    float2 texCoord     : TEXCOORD;
    float2 tilingFactor : TILINGFACTOR;
    float4 color        : COLOR;
};

float4 main(PSInput input) : SV_TARGET
{
    float3 normal = normalize(input.normal);

    float3 lightDirection = normalize(float3(-0.5f, -1.0f, -3.0f));

    float diffuseIntensity = max(dot(normal, -lightDirection), 0.0f);

    float ambientIntensity = 0.2f;

    float lighting = saturate(ambientIntensity + diffuseIntensity);

    float4 finalColor = input.color * lighting;

    return finalColor;
}