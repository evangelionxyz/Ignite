cbuffer LightMatrices : register(b0)
{
    float4x4 lightViewProj;
}

struct VSInput
{
    float3 position : POSITION;
};

struct PSInput
{
    float4 position : SV_POSITION;
};

PSInput main(VSInput input)
{
    VSInput output;
    float4 worldPosition = float4(input.position, 1.0f);
    output.position = mul(worldPosition, lightViewProj);
    return output;
}