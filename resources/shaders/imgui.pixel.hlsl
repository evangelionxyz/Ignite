#include "include/helpers.hlsli"

struct PS_INPUT
{
    float4 pos : SV_POSITION;
    float4 col : COLOR0;
    float2 uv  : TEXCOORD0;
};

Texture2D texture0 : register(t0);
sampler sampler0 : register(s0);

float4 main(PS_INPUT input) : SV_Target
{
    float4 texColor = texture0.Sample(sampler0, input.uv);
    texColor = float4(SRGBToLinear(texColor.rgb), texColor.a);
    float4 color = float4(SRGBToLinear(input.col.rgb), input.col.a);
    return color * texColor;
}