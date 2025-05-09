cbuffer Constants : register(b0)
{
    float4x4 viewProjection;
}

struct VSInput
{
    float3 position : POSITION;
};

struct PSInput
{
    float4 position : SV_POSITION;
    float3 UVW : UVW;
};

PSInput main(VSInput input)
{
    PSInput output;
    // Remove translation from the matrix: convert to mat3 and back
    float3x3 vpRotOnly = (float3x3)viewProjection;

    // Rebuild a float4x4 with zero translation
    float4x4 viewProjectionNoTranslation = {
        float4(vpRotOnly[0], 0.0f),
        float4(vpRotOnly[1], 0.0f),
        float4(vpRotOnly[2], 0.0f),
        float4(0.0f, 0.0f, 0.0f, 1.0f)
    };

    float4 pos = mul(viewProjectionNoTranslation, float4(input.position, 1.0f));

    // Push to far plane (w = z)
    output.position = float4(pos.xy, pos.z, pos.z);

    output.UVW = input.position;

    return output;
}
