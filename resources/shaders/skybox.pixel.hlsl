#include "include/helpers.hlsli"

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

Texture2D texture0 : register(t0);
SamplerState sampler0 : register(s0);

float4 main(PSInput input) : SV_TARGET
{
    float3 dir = normalize(input.UVW);
    float3 color = SampleSphericalMap(texture0, sampler0, dir);
    color = FilmicTonemap(color, env.exposure, env.gamma);
    return float4(color, 1.0f);
}