#include "binding_helpers.hlsli"

struct VS_INPUT
{
    float2 position : POSITION;
    float4 color    : COLOR0;
};

struct PS_INPUT
{
    float4 position : SV_POSITION;
    float4 color    : COLOR0;
};

PS_INPUT main(VS_INPUT input)
{
    PS_INPUT output;
    output.position = float4(input.position.xy, 0.0, 1.0);
    output.color = input.color;
    return output;
}