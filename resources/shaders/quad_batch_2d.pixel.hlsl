#include "include/binding_helpers.hlsli"

struct PSInput
{
    float4 position     : SV_POSITION;
    float2 texCoord     : TEXCOORD;
    float2 tilingFactor : TILINGFACTOR;
    float4 color        : COLOR;
    uint texIndex       : TEXINDEX;
};

Texture2D textures[16]    : register(t0);
SamplerState samplerState : register(s0);

float4 main(PSInput input) : SV_TARGET
{
    float4 texColor = textures[input.texIndex].Sample(samplerState, input.texCoord * input.tilingFactor);
    
     // Discard pixel if alpha is zero
    clip(texColor.a == 0.0f ? -1.0f : 1.0f);

    return input.color * texColor;
}