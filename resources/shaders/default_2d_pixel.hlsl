struct PSInput
{
    float4 position : SV_POSITION;
    float2 texCoord : TEXCOORD;
    float4 color : COLOR;
};

Texture2D texture : register(t0);
SamplerState samplerState : register(s0);

float4 main(PSInput input) : SV_TARGET
{
    float4 texColor = texture.Sample(samplerState, input.texCoord);
    return input.color * texColor;
}