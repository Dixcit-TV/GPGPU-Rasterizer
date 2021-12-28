#include "../Libs/Common.hlsli"

#define GROUP_X 32
#define GROUP_Y 32
#define GROUP_DIMs GROUP_X, GROUP_Y, 1
#define UINT3_GROUP_DIMs uint3(GROUP_DIMs)
#define GROUP_SIZE GROUP_X * GROUP_Y;

#define VIEWPORT_WIDTH 1920.f
#define VIEWPORT_HEIGHT 1080.f

#define TRIANGLE_COUNT 11638

ByteAddressBuffer G_TRANS_VERTEX_BUFFER;
ByteAddressBuffer G_INDEX_BUFFER;

struct RasterData
{
	float edgeEq[12];
	uint2 aabb;
	float invArea;
	uint isClipped;
};
RWStructuredBuffer<RasterData> G_RASTER_DATA : register(u2);

bool IsClipped(float4 vertex, float viewportWidth, float viewportHeight);
uint4 GetAabb(float2 v0, float2 v1, float2 v2, float viewportWidth, float viewportHeight);

[numthreads(GROUP_DIMs)]
void main(uint groupIndex : SV_GroupIndex, uint3 dispatchID : SV_GroupId)
{
	const uint globalThreadId = FlattenedGlobalThreadId(groupIndex, dispatchID, UINT3_GROUP_DIMs, uint3(64, 1, 1) /*G_DISPATCH_DIMS*/);
	if (globalThreadId >= TRIANGLE_COUNT)
		return;

	RasterData data = (RasterData)0;
	uint3 tri = G_INDEX_BUFFER.Load3(globalThreadId * 4);

	const uint stride = 2;
	const float4 v0 = asfloat(G_TRANS_VERTEX_BUFFER.Load4(tri.x * stride * 4));
	const float4 v1 = asfloat(G_TRANS_VERTEX_BUFFER.Load4(tri.y * stride * 4));
	const float4 v2 = asfloat(G_TRANS_VERTEX_BUFFER.Load4(tri.z * stride * 4));

	data.isClipped = IsClipped(v0, VIEWPORT_WIDTH, VIEWPORT_HEIGHT) || IsClipped(v1, VIEWPORT_WIDTH, VIEWPORT_HEIGHT) || IsClipped(v2, VIEWPORT_WIDTH, VIEWPORT_HEIGHT);
	if (!data.isClipped)
	{
		uint4 aabb = GetAabb(v0.xy, v1.xy, v2.xy, VIEWPORT_WIDTH, VIEWPORT_HEIGHT);

		data.edgeEq[0] = v1.y - v2.y;
		data.edgeEq[1] = v2.x - v1.x;
		data.edgeEq[2] = cross2d(v1.xy, v2.xy);

		data.edgeEq[3] = v2.y - v0.y;
		data.edgeEq[4] = v0.x - v2.x;
		data.edgeEq[5] = cross2d(v2.xy, v0.xy);

		data.edgeEq[6] = v0.y - v1.y;
		data.edgeEq[7] = v1.x - v0.x;
		data.edgeEq[8] = cross2d(v0.xy, v1.xy);

		data.edgeEq[9] = data.edgeEq[0] * aabb.x + data.edgeEq[1] * aabb.y + data.edgeEq[2];
		data.edgeEq[10] = data.edgeEq[3] * aabb.x + data.edgeEq[4] * aabb.y + data.edgeEq[5];
		data.edgeEq[11] = data.edgeEq[6] * aabb.x + data.edgeEq[7] * aabb.y + data.edgeEq[8];

		data.aabb = uint2((aabb.x << 16) | (0x0000ffff & aabb.y), (aabb.z << 16) | (0x0000ffff & aabb.w));
		data.invArea = 1 / cross2d(v0.xy - v2.xy, v1.xy - v2.xy);
	}

	G_RASTER_DATA[globalThreadId] = data;
}

bool IsClipped(float4 vertex, float viewportWidth, float viewportHeight)
{
	return vertex.x < 0.f || vertex.y < 0.f
		|| vertex.x > viewportWidth || vertex.y > viewportHeight
		|| vertex.z < 0.f || vertex.z > 1.f;
}

uint4 GetAabb(float2 v0, float2 v1, float2 v2, float viewportWidth, float viewportHeight)
{
	float4 aabb;
	aabb.xy = min(v0.xy, min(v1.xy, v2.xy));
	aabb.zw = ceil(max(v0.xy, max(v1.xy, v2.xy)));
	return asuint(aabb);
}