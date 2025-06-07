#include "include/binding_helpers.hlsli"

struct PSInput
{
    float4 position     : SV_POSITION;
    float2 texCoord     : TEXCOORD;
    float2 tilingFactor : TILINGFACTOR;
    float4 color        : COLOR;
    uint texIndex       : TEXINDEX;
    uint entityID       : ENTITYID;
};

Texture2D texture    : register(t0);
SamplerState samplerState : register(s0);

struct PSOutput
{
    float4 color : SV_TARGET0;
    uint4 entityID : SV_TARGET1;
};


PSOutput main(PSInput input)
{
    float4 texColor = texture.Sample(samplerState, input.texCoord * input.tilingFactor);
    float4 finalColor = input.color * texColor;
    
    // Discard pixel if alpha is zero
    clip(finalColor.a == 0.0f ? -1.0f : 1.0f);
    
    PSOutput result;
    result.color = finalColor;
    result.entityID = uint4(input.entityID, input.entityID, input.entityID, input.entityID);

    return result;
}