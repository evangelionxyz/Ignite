Texture2D inputTexture : register(t0);
SamplerState linearSampler : register(s0);

cbuffer BlurConstants : register(b0)
{
    float2 texelSize; // 1.0 / textureResolution
    float blurRadius; // Blur strength
    float _padding;
};
struct PSInput
{
    float4 position : SV_POSITION;
    float2 texCoord : TEXCOORD;
};

float4 main(PSInput input) : SV_TARGET
{
    float4 color = float4(0, 0, 0, 0);
    float weightSum = 0.0f;
    
    // 9-tap Gaussian blur
    const int kernelSize = 4;
    const float weights[5] = { 0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216 };
    
    // Horizontal blur (you'd do vertical in a separate pass for better performance)
    for (int i = -kernelSize; i <= kernelSize; ++i)
    {
        float2 offset = float2(i * texelSize.x * blurRadius, 0.0f);
        float weight = weights[abs(i)];
        
        color += inputTexture.Sample(linearSampler, input.texCoord + offset) * weight;
        weightSum += weight;
    }
    
    return color / weightSum;
}