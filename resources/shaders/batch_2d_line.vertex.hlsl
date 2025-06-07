struct Camera
{
    float4x4 viewProjection;
    float3 position;
};

cbuffer CameraBuffer : register(b0)
{
    Camera camera;
}

struct VSInput
{
    float3 position     : POSITION;
    float4 color        : COLOR;
    uint entityID       : ENTITYID;
};

struct PSInput
{
    float4 position     : SV_POSITION;
    float4 color        : COLOR;
    uint entityID       : ENTITYID;
};

PSInput main(VSInput input)
{
    PSInput output;
    float4 pos          = float4(input.position.x, input.position.y, input.position.z, 1.0f);
    output.position     = mul(camera.viewProjection, pos);
    output.color        = input.color;
    output.entityID     = input.entityID;
    return output;
}