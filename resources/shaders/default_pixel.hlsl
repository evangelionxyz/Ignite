struct PSInput
{
    float4 position     : SV_POSITION;
    float3 normal       : NORMAL;
    float2 texCoord     : TEXCOORD;
    float2 tilingFactor : TILINGFACTOR;
    float4 color        : COLOR;
};

float4 main(PSInput input) : SV_TARGET
{
    float4 texColor = input.color;
    return texColor;
}