#include "include/binding_helpers.hlsli"

struct PSInput
{
    float4 position     : SV_POSITION;
    float4 color        : COLOR;
    uint entityID       : ENTITYID;
};

struct PSOutput
{
    float4 color : SV_TARGET0;
    uint4 entityID : SV_TARGET1;
};


PSOutput main(PSInput input)
{
    // Discard pixel if alpha is zero
    clip(input.color.a == 0.0f ? -1.0f : 1.0f);
    
    PSOutput result;
    result.color = input.color;
    result.entityID = uint4(input.entityID, input.entityID, input.entityID, input.entityID);
    
    return result;
}
