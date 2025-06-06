Texture2D inputTexture : register(t0);
SamplerState linearSampler : register(s0);

Texture2D sceneTexture : register(t0);
Texture2D outlineTexture : register(t1);

cbuffer CompositeConstants : register(b0)
{
    float3 outlineColor;
    float outlineIntensity;
};

struct PSInput
{
    float4 position : SV_POSITION;
    float2 texCoord : TEXCOORD;
};

float4 main(PSInput input) : SV_TARGET
{
    float4 sceneColor = sceneTexture.Sample(linearSampler, input.texCoord);
    float outlineMask = outlineTexture.Sample(linearSampler, input.texCoord).r;
    
    // Blend outline with scene
    float3 finalColor = lerp(sceneColor.rgb, outlineColor, outlineMask * outlineIntensity);
    
    return float4(finalColor, sceneColor.a);
}