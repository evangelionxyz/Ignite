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

struct PSOutput
{
    float4 color : SV_TARGET0;
    uint4 entityID : SV_TARGET1;
};

PSOutput main(PSInput input)
{
    PSOutput result;
    float3 dir = normalize(input.UVW);
    float3 color = SampleSphericalMap(texture0, sampler0, dir);

    result.color = float4(FilmicTonemap(color, env.exposure, env.gamma), 1.0f);
    result.entityID = uint4(-1, -1, -1, -1);

    return result;
}