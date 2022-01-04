#define gPi 3.14159265358979323846f;

cbuffer LightInfo
{
	float3 lightDir;
	float lightIntensity;
}

struct PIXEL_IN
{
	float4 position : SV_POSITION;
	float3 normal : NORMAL;
};

float4 main(PIXEL_IN input) : SV_TARGET
{
	float diffuseStrength = saturate(dot(input.normal, -lightDir)) * lightIntensity;
	diffuseStrength /= gPi;

	return float4(0.5f, 0.5f, 0.5f, 1.0f);
}