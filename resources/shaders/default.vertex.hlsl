cbuffer PushConstants : register(b0)
{
    float4x4 mvp;
    float4x4 modelMatrix;
    float4x4 normalMatrix;
    float4 cameraPos;
};

struct VSInput
{
    float3 position     : POSITION;
    float3 normal       : NORMAL;
    float2 texCoord     : TEXCOORD;
    float2 tilingFactor : TILINGFACTOR;
    float4 color        : COLOR;
};

struct PSInput
{
    float4 position     : SV_POSITION;
    float3 normal       : NORMAL;
    float3 worldPos     : WORLDPOS;
    float2 texCoord     : TEXCOORD;
    float2 tilingFactor : TILINGFACTOR;
    float4 color        : COLOR;
};

PSInput main(VSInput input)
{
    PSInput output;

    float4 worldPos     = mul(modelMatrix, float4(input.position, 1.0f));
    float3 worldNormal = normalize(mul((float3x3)normalMatrix, input.normal));

    output.position     = mul(mvp, worldPos);
    output.normal       = worldNormal;
    output.worldPos     = worldPos.xyz;
    output.texCoord     = input.texCoord;
    output.tilingFactor = input.tilingFactor;
    output.color        = input.color;
    return output;
}