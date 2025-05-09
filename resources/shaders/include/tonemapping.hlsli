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

float4 tonemap(float4 color, float exposure, float gamma)
{
	float4 mappedColor = Uncharted2Tonemap(color * exposure);

	// normalize by the tonemapped white point
	float4 whiteScale = 1.0f / Uncharted2Tonemap(float4(11.2f, 11.2f, 11.2f, 11.2f));
	mappedColor *= whiteScale;

	// apply gamma correction
	float3 gammaCorrected = pow(mappedColor.rgb, 1.0f / gamma);
	return float4(gammaCorrected, color.a);
}
