cbuffer PushConstants : register(b1)
{
    float4x4 mvp;
};

struct VSInput
{
    float2 position : POSITION;
    float2 texCoord : TEXCOORD;
    float4 color : COLOR;
};

struct PSInput
{
    float4 position : SV_POSITION;
    float2 texCoord : TEXCOORD;
    float4 color : COLOR;
};

PSInput main(VSInput input)
{
    PSInput output;
    float4 pos = float4(input.position.x, input.position.y, 0.0f, 1.0f);
    output.position = mul(mvp, pos);
    output.color = input.color;
    output.texCoord = input.texCoord;
    return output;
}