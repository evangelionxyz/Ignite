float3 SRGBToLinear(float3 srgb)
{
    float3 lt = step(float3(0.04045, 0.04045, 0.04045), srgb);
    return lerp(srgb / 12.92, pow((srgb + 0.055) / 1.055, 2.4), lt);
}
