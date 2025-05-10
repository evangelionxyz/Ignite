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

struct VSInput
{
    float3 position     : POSITION;
    float3 normal       : NORMAL;
    float2 UV           : TEXCOORD;
    float2 tilingFactor : TILINGFACTOR;
    float4 color        : COLOR;
};

struct PSInput
{
    float4 position     : SV_POSITION;
    float3 normal       : NORMAL;
    float3 worldPos     : WORLDPOS;
    float2 UV           : TEXCOORD;
    float2 tilingFactor : TILINGFACTOR;
    float4 color        : COLOR;
};

PSInput main(VSInput input)
{
    PSInput output;

    float4 worldPos     = mul(transformMatrix, float4(input.position, 1.0f));
    float3 worldNormal = normalize(mul((float3x3)normalMatrix, input.normal));

    output.position     = mul(viewProjection, worldPos);
    output.normal       = worldNormal;
    output.worldPos     = worldPos.xyz;
    output.UV     = input.UV;
    output.tilingFactor = input.tilingFactor;
    output.color        = input.color;
    return output;
}