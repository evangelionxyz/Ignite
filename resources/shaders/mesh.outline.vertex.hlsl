#include "include/binding_helpers.hlsli"

#define VERTEX_MAX_BONES 4 // bone influences
#define MAX_BONES 200

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

cbuffer CameraBuffer : register(b0)
{
    Camera camera;
}
cbuffer ObjectBuffer : register(b1)
{
    Object object;
}

struct VSInput
{
    float3 position : POSITION;
    uint4 boneIDs : BONEIDS;
    float4 weights : WEIGHTS;
};

struct PSInput
{
    float4 position : SV_POSITION;
};

PSInput main(VSInput input)
{
    PSInput output;
    
    float4 posL = float4(0.0f, 0.0f, 0.0f, 0.0f);
    
    for (int i = 0; i < VERTEX_MAX_BONES; ++i)
    {
        float weight = input.weights[i];
        if (weight > 0.0f)
        {
            uint boneId = input.boneIDs[i];
            float4x4 transform = object.boneTransforms[boneId];
            posL += weight * mul(transform, float4(input.position, 1.0));
        }
    }
    
    if (length(posL) < 0.00001f)
    {
        posL = float4(input.position, 1.0f);
    }

    float4 worldPos = mul(object.transformMatrix, posL);
    output.position = mul(camera.viewProjection, worldPos);
    return output;
}