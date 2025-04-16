cbuffer PushConstants : register(b0)
{
    float4x4 mvp;
    float4x4 modelMatrix;
    float4x4 normalMatrix;
    float4 cameraPos;
};

struct PSInput
{
    float4 position     : SV_POSITION;
    float3 normal       : NORMAL;
    float3 worldPos     : WORLDPOS;
    float2 texCoord     : TEXCOORD;
    float2 tilingFactor : TILINGFACTOR;
    float4 color        : COLOR;
};

float4 main(PSInput input) : SV_TARGET
{
    float3 normal = normalize(input.normal);
    float3 viewDirection = normalize(cameraPos.xzy - input.worldPos);
    float3 lightDirection = normalize(float3(-0.5f, -1.0f, -3.0f));

    // lambert diffuse
    float diff = max(dot(normal, -lightDirection), 0.0f);

    // blin-phong specular
    float3 halfway = normalize(viewDirection - lightDirection);
    float spec = pow(max(dot(normal, halfway), 0.0f), 32.0f); // 32 shininess

    float ambient = 0.2f;
    float lighting = saturate(ambient + diff + spec);

    float4 finalColor = input.color * lighting;

    return finalColor;
}