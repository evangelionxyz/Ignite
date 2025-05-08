cbuffer PushConstants : register(b0)
{
    float4x4 viewProjection;
    float4 cameraPosition;
};

cbuffer ModelPushConstants : register(b1)
{
    float4x4 transformMatrix;
    float4x4 normalMatrix;
}

cbuffer MaterialPushConstants : register(b2)
{
    float4 baseColor;
    float4 diffuseColor;
    float emissive;
}

struct PSInput
{
    float4 position     : SV_POSITION;
    float3 normal       : NORMAL;
    float3 worldPos     : WORLDPOS;
    float2 texCoord     : TEXCOORD;
    float2 tilingFactor : TILINGFACTOR;
    float4 color        : COLOR;
};

Texture2D texture0 : register(t0);
sampler sampler0 : register(s0);

float4 main(PSInput input) : SV_TARGET
{
    float3 normal = normalize(input.normal);
    float3 viewDirection = normalize(cameraPosition.xzy - input.worldPos);
    float3 lightDirection = normalize(float3(-0.5f, -1.0f, -3.0f));

    // lambert diffuse
    float diff = max(dot(normal, -lightDirection), 0.0f);

    // blin-phong specular
    float3 halfway = normalize(viewDirection - lightDirection);
    float spec = pow(max(dot(normal, halfway), 0.0f), 32.0f); // 32 shininess

    float ambient = 0.2f;
    float lighting = saturate(ambient + diff + spec);

    float4 texColor = texture0.Sample(sampler0, input.texCoord) * baseColor;
    float4 finalColor = input.color * texColor * lighting;

    return finalColor;
}