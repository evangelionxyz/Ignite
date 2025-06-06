cbuffer EdgeConstants : register(b0)
{
    float2 texelSize;
    float edgeThreshold;
    float _padding2;
};

Texture2D inputTexture : register(t0);
SamplerState linearSampler : register(s0);

struct PSInput
{
    float4 position : SV_POSITION;
    float2 texCoord : TEXCOORD;
};

float4 main(PSInput input) : SV_TARGET
{
    float2 uv = input.texCoord;
    
    // Sample 3x3 neighborhood
    float tl = inputTexture.Sample(linearSampler, uv + float2(-texelSize.x, -texelSize.y)).r; // top-left
    float tm = inputTexture.Sample(linearSampler, uv + float2(0.0f, -texelSize.y)).r; // top-middle
    float tr = inputTexture.Sample(linearSampler, uv + float2(texelSize.x, -texelSize.y)).r; // top-right
    float ml = inputTexture.Sample(linearSampler, uv + float2(-texelSize.x, 0.0f)).r; // middle-left
    float mm = inputTexture.Sample(linearSampler, uv).r; // center
    float mr = inputTexture.Sample(linearSampler, uv + float2(texelSize.x, 0.0f)).r; // middle-right
    float bl = inputTexture.Sample(linearSampler, uv + float2(-texelSize.x, texelSize.y)).r; // bottom-left
    float bm = inputTexture.Sample(linearSampler, uv + float2(0.0f, texelSize.y)).r; // bottom-middle
    float br = inputTexture.Sample(linearSampler, uv + float2(texelSize.x, texelSize.y)).r; // bottom-right
    
    // Sobel X kernel
    float sobelX = (tr + 2.0f * mr + br) - (tl + 2.0f * ml + bl);
    
    // Sobel Y kernel  
    float sobelY = (tl + 2.0f * tm + tr) - (bl + 2.0f * bm + br);
    
    // Calculate gradient magnitude
    float edge = sqrt(sobelX * sobelX + sobelY * sobelY);
    
    // Threshold the edge
    edge = edge > edgeThreshold ? 1.0f : 0.0f;
    
    return float4(edge, edge, edge, edge);
}