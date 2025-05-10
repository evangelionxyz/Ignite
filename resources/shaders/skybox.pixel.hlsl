#include "include/tonemapping.hlsli"
#include "include/srgb_to_linear.hlsli"

struct Environment
{
    float exposure;
    float gamma;
};

cbuffer ParamsConstants : register(b1) { Environment env; }

struct PSInput
{
    float4 position : SV_POSITION;
    float3 UVW      : UVW;
};

TextureCube texture0 : register(t0);
sampler sampler0 : register(s0);

float4 main(PSInput input) : SV_TARGET
{
    float4 color = texture0.SampleLevel(sampler0, input.UVW, 1.5f);
    color = tonemap(color, env.exposure, env.gamma);
    color = float4(SRGBToLinear(color.rgb), 1.0f);
    return color;
}