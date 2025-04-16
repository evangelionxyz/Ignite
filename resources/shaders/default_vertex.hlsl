cbuffer PushConstants : register(b0)
{
    float4x4 mvp;
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
    float2 texCoord     : TEXCOORD;
    float2 tilingFactor : TILINGFACTOR;
    float4 color        : COLOR;
};

PSInput main(VSInput input)
{
    PSInput output;
    float4 pos          = float4(input.position.x, input.position.y, input.position.z, 1.0f);
    output.position     = mul(mvp, pos);
    output.normal       = input.normal;
    output.tilingFactor = input.tilingFactor;
    output.color        = input.color;
    return output;
}