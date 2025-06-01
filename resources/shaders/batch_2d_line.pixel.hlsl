#include "include/binding_helpers.hlsli"

struct PSInput
{
    float4 position     : SV_POSITION;
    float4 color        : COLOR;
};

float4 main(PSInput input) : SV_TARGET
{
    // Discard pixel if alpha is zero
    clip(input.color.a == 0.0f ? -1.0f : 1.0f);
    return input.color;
}
