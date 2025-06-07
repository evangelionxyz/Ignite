cbuffer OutlineConstants : register(b0)
{
    float4 silhouetteColor; // Usually white (1,1,1,1)
};

struct PSInput
{
    float4 position : SV_POSITION;
    float2 texCoord : TEXCOORD;
};

float4 main(PSInput input) : SV_TARGET
{
    return float4(1.0f, 1.0f, 1.0f, 1.0f);
}