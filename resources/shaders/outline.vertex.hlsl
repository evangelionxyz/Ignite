
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
    float3 position : POSITION;
};

struct PSInput
{
    float4 position : SV_POSITION;
};

PSInput main(VSInput input)
{
    PSInput output;
    output.position = mul(camera.viewProjection, float4(input.position.x, input.position.y, input.position.z, 1.0f));
    return output;
}
