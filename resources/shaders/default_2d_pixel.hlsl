struct PSInput
{
    float4 position     : SV_POSITION;
    float2 texCoord     : TEXCOORD;
    float2 tilingFactor : TILINGFACTOR;
    float4 color        : COLOR;
    uint texIndex       : TEXINDEX;
};

Texture2D textures[16] : register(t0);
SamplerState samplerState : register(s0);

float4 main(PSInput input) : SV_TARGET
{
    float4 texColor = textures[input.texIndex].Sample(samplerState, input.texCoord * input.tilingFactor);
    return input.color * texColor;
}