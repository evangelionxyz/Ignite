cbuffer Constants : register(b0)
{
    float4 mvp;
}

struct VSInput
{
    float3 position : POSITION;
}

struct PSInput
{
    float4 position      : SV_POSITION;
    float3 worldPosition : WORLDPOS;
}

PSInput main(VSInput input)
{
    PSInput output;

    float4 worldPos = mul(mvp, float4(input.position, 1.0f));
    output.position = input.position;
    output.worldPosition = float3(worldPos);

    return output;
}