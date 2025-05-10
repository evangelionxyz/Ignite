float4 Uncharted2Tonemap(float4 color)
{
    float A = 0.15;
    float B = 0.50;
    float C = 0.10;
    float D = 0.20;
    float E = 0.02;
    float F = 0.30;
    float W = 11.2;

    return ((color * (A * color + C * B) + D * E) / (color * (A * color + B)+ D * F))-E/F;
}

float3 Uncharted2Tonemap(float3 color, float exposure, float gamma)
{
    float4 mappedColor = Uncharted2Tonemap(float4(color, 1.0f) * exposure);

    // normalize by the tonemapped white point
    float4 whiteScale = 1.0f / Uncharted2Tonemap(float4(11.2f, 11.2f, 11.2f, 11.2f));
    mappedColor *= whiteScale;

    // Gamma correction
    float3 gammaCorrected = pow(mappedColor.rgb, 1.0f / gamma);
    return gammaCorrected.rgb;
}

float3 Reinhard2Tonemap(float3 color)
{
  const float L_white = 4.0;
  return (color * (1.0 + color / (L_white * L_white))) / (1.0 + color);
}

float3 Reinhard2Tonemap(float3 color, float exposure, float gamma) {
    float3 mappedColor = Reinhard2Tonemap(color * exposure);
    
    float3 whiteScale = 1.0f / Reinhard2Tonemap(float3(11.2f, 11.2f, 11.2f));
    mappedColor *= mappedColor;
    
    // Gamma correction
    float3 gammaCorrected = pow(mappedColor.rgb, 1.0f / gamma);
    return gammaCorrected.rgb;
}

float3 FilmicTonemap(float3 color)
{
    float3 X = max(float3(0.0f, 0.0f, 0.0f), color - 0.004);
    float3 result = (X * (6.2 * X + 0.5)) / (X * (6.2 * X + 1.7) + 0.06);
    return pow(result, float3(2.2f, 2.2f, 2.2f));
}

float3 FilmicTonemap(float3 color, float exposure, float gamma)
{
    float3 mappedColor = FilmicTonemap(color * exposure);
    
    float3 whiteScale = 1.0f / FilmicTonemap(float3(11.2f, 11.2f, 11.2f));
    mappedColor *= mappedColor;
    
    // Gamma correction
    float3 gammaCorrected = pow(mappedColor.rgb, 1.0f / gamma);
    return gammaCorrected.rgb;
}

float3 SRGBToLinear(float3 srgb)
{
    float3 lt = step(float3(0.04045, 0.04045, 0.04045), srgb);
    return lerp(srgb / 12.92, pow((srgb + 0.055) / 1.055, 2.4), lt);
}

float3 SampleSphericalMap(Texture2D tex, SamplerState samp, float3 dir)
{
    dir = normalize(dir);
    float2 uv;
    uv.x = atan2(dir.z, dir.x) / (2.0f * 3.14159265f) + 0.5f;
    uv.y = asin(clamp(dir.y, -1.0f, 1.0f)) / 3.14159265f + 0.5f;
    return tex.Sample(samp, uv).rgb;
}
