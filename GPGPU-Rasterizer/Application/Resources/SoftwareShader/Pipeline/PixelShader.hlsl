#include "../Libs/Common.hlsli"

#define GROUP_DIMs 32, 32, 1
#define UINT3_GROUP_DIMs uint3(GROUP_DIMs)
#define PI 3.14159265358979323846f

cbuffer DispatchInfo
{
	uint3 G_DISPATCH_DIMS;
}

cbuffer LightInfo
{
	float3 lightDir;
	float lightIntensity;
}

struct Fragment
{
	float4 position;
	float3 normal;
	float pad;
};

StructuredBuffer<Fragment> G_FRAGMENT_BUFFER;

RWTexture2D<float4> G_RENDER_TARGET;

[numthreads(GROUP_DIMs)]
void main(uint groupIndex : SV_GroupIndex, uint3 dispatchID : SV_GroupId)
{
	const uint globalThreadId = FlattenedGlobalThreadId(groupIndex, dispatchID, UINT3_GROUP_DIMs, uint3(64, 1, 1) /*G_DISPATCH_DIMS*/);

	Fragment f = G_FRAGMENT_BUFFER[globalThreadId];
	float diffuseStrength = saturate(dot(f.normal, -lightDir)) * lightIntensity;
	diffuseStrength /= PI;

	G_RENDER_TARGET[uint2(asuint(f.position.x), asuint(f.position.y))] = float4(float3(0.5f, 0.5f, 0.5f) * diffuseStrength, 1.f);
}