#include "include/binding_helpers.hlsli"

#define VERTEX_MAX_BONES 4 // bone influences
#define MAX_BONES 150

struct Camera
{
    float4x4 viewProjection;
    float4 position;
};

struct Object
{
    float4x4 transformMatrix;
    float4x4 normalMatrix;
    float4x4 boneTransforms[MAX_BONES];
};

cbuffer CameraBuffer : register(b0) { Camera camera; }
cbuffer ObjectBuffer : register(b3) { Object object; }

struct VSInput
{
    float3 position     : POSITION;
    float3 normal       : NORMAL;
    float2 UV           : TEXCOORD;
    float2 tilingFactor : TILINGFACTOR;
    float4 color        : COLOR;
    uint4 boneIDs       : BONEIDS;
    float4 weights      : WEIGHTS;
};

struct PSInput
{
    float4 position     : SV_POSITION;
    float3 normal       : NORMAL;
    float3 worldPos     : WORLDPOS;
    float2 UV           : TEXCOORD;
    float2 tilingFactor : TILINGFACTOR;
    float4 color        : COLOR;
};

PSInput main(VSInput input)
{
    PSInput output;

    // Initialize with zero
    float4 posL = float4(0.0f, 0.0f, 0.0f, 0.0f);
    float3 normalL = float3(0.0f, 0.0f, 0.0f);

    // Calculate skinned position and normal
    for (int i = 0; i < VERTEX_MAX_BONES; ++i)
    {
        float weight = input.weights[i];
        if (weight > 0.0f)
        {
            uint boneId = input.boneIDs[i];
            float4x4 transform = object.boneTransforms[boneId];

            posL += weight * mul(transform, float4(input.position, 1.0));
            normalL += weight * mul((float3x3)transform, input.normal);
        }
    }

    // Ensure we have a valid position
    if (length(posL) < 0.00001f)
    {
        // Fallback to no skinning if weights don't sum to a significant value
        posL = float4(input.position, 1.0f);
        normalL = input.normal;
    }

    float4 worldPos     = mul(object.transformMatrix, posL);
    float3 worldNormal = normalize(mul((float3x3)object.normalMatrix, normalL));

    output.position     = mul(camera.viewProjection, worldPos);
    output.normal       = worldNormal;
    output.worldPos     = worldPos.xyz;
    output.UV     = input.UV;
    output.tilingFactor = input.tilingFactor;
    output.color        = input.color;
    return output;
}