#include "../Libs/Common.hlsli"

#define GROUP_X 32
#define GROUP_Y 32
#define GROUP_DIMs GROUP_X, GROUP_Y, 1
#define UINT3_GROUP_DIMs uint3(GROUP_DIMs)
#define GROUP_SIZE GROUP_X * GROUP_Y;

#define VIEWPORT_WIDTH 1920.f
#define VIEWPORT_HEIGHT 1080.f

cbuffer ObjectInfo : register(b0)
{
	float4x4 worldViewProj;
	float4x4 world;
	uint vertexCount;
	uint triangleCount;
	uint indexCount;
}

struct Vertex_Out
{
	float4 position;
	float3 normal;
	float pad;
};

StructuredBuffer<Vertex_Out> G_TRANS_VERTEX_BUFFER;
ByteAddressBuffer G_INDEX_BUFFER;

struct RasterData
{
	float edgeEq[9];
	float3 pad;
	uint2 aabb;
	float invArea;
	uint isClipped;
};
RWStructuredBuffer<RasterData> G_RASTER_DATA : register(u2);

bool IsClipped(float4 vertex, float viewportWidth, float viewportHeight);
uint4 GetAabb(float2 v0, float2 v1, float2 v2);

[numthreads(GROUP_DIMs)]
void main(uint groupIndex : SV_GroupIndex, uint3 dispatchID : SV_GroupId)
{
	const uint globalThreadId = FlattenedGlobalThreadId(groupIndex, dispatchID, UINT3_GROUP_DIMs, uint3(64, 1, 1) /*G_DISPATCH_DIMS*/);
	if (globalThreadId >= triangleCount)
		return;

	RasterData data = (RasterData)0;
	uint3 tri = G_INDEX_BUFFER.Load3(globalThreadId * 3 * 4);

	const float4 v0 = G_TRANS_VERTEX_BUFFER[tri.x].position;
	const float4 v1 = G_TRANS_VERTEX_BUFFER[tri.y].position;
	const float4 v2 = G_TRANS_VERTEX_BUFFER[tri.z].position;

	data.isClipped = IsClipped(v0, VIEWPORT_WIDTH, VIEWPORT_HEIGHT) || IsClipped(v1, VIEWPORT_WIDTH, VIEWPORT_HEIGHT) || IsClipped(v2, VIEWPORT_WIDTH, VIEWPORT_HEIGHT);
	if (!data.isClipped)
	{
		uint4 aabb = GetAabb(v0.xy, v1.xy, v2.xy);

		data.edgeEq[0] = v1.y - v2.y;
		data.edgeEq[1] = v2.x - v1.x;

		data.edgeEq[2] = v2.y - v0.y;
		data.edgeEq[3] = v0.x - v2.x;

		data.edgeEq[4] = v0.y - v1.y;
		data.edgeEq[5] = v1.x - v0.x;

		data.edgeEq[6] = data.edgeEq[0] * aabb.x + data.edgeEq[1] * aabb.y + cross2d(v1.xy, v2.xy);
		data.edgeEq[7] = data.edgeEq[2] * aabb.x + data.edgeEq[3] * aabb.y + cross2d(v2.xy, v0.xy);
		data.edgeEq[8] = data.edgeEq[4] * aabb.x + data.edgeEq[5] * aabb.y + cross2d(v0.xy, v1.xy);

		data.aabb = uint2((aabb.x << 16) | aabb.y, (aabb.z << 16) | aabb.w);
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

uint4 GetAabb(float2 v0, float2 v1, float2 v2)
{
	float4 aabb;
	aabb.xy = min(v0.xy, min(v1.xy, v2.xy));
	aabb.zw = ceil(max(v0.xy, max(v1.xy, v2.xy)));
	return aabb;
}