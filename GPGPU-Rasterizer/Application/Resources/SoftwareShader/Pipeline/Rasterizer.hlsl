#include "../Libs/Common.hlsli"

#define VIEWPORT_WIDTH 1920.f
#define VIEWPORT_HEIGHT 1080.f
#define BIN_SIZE uint2(64, 64)
#define BIN_PixelCount BIN_SIZE.x * BIN_SIZE.y
#define BINNING_DIMS uint2(ceil(VIEWPORT_WIDTH / BIN_SIZE.x), ceil(VIEWPORT_HEIGHT / BIN_SIZE.y))
#define BIN_COUNT BINNING_DIMS.x * BINNING_DIMS.y
#define BIN_UINT_COUNT (uint)ceil(BIN_COUNT / 32.f)

#define GROUP_X 32
#define GROUP_Y 16
#define GROUP_DIMs GROUP_X, GROUP_Y, 1
#define UINT3_GROUP_DIMs uint3(GROUP_DIMs)

cbuffer ObjectInfo : register(b0)
{
	float4x4 worldViewProj;
	float4x4 world;
	uint vertexCount;
	uint triangleCount;
	uint indexCount;
}

struct RasterData
{ 
	float edgeEq[9];
	float3 pad;
	uint2 aabb;
	float invArea;
	uint isClipped;
};

StructuredBuffer<RasterData> G_RASTER_DATA;

RWByteAddressBuffer G_TRIANGLE_BIN_BUFFER : register(u2);

groupshared uint groupCache[GROUP_X * GROUP_Y][BIN_UINT_COUNT];

[numthreads(GROUP_DIMs)]
void main(uint groupIndex : SV_GroupIndex, uint3 dispatchID : SV_GroupId)
{
	const uint globalThreadId = FlattenedGlobalThreadId(groupIndex, dispatchID, UINT3_GROUP_DIMs, uint3(64, 1, 1) /*G_DISPATCH_DIMS*/);
	const uint threadCount = GROUP_X * GROUP_Y * 1;
	uint loopCount = 0;

	uint triIndex = 0;
	while ((triIndex = loopCount * threadCount + globalThreadId) < triangleCount)
	{
		++loopCount;

		const RasterData triData = G_RASTER_DATA[triIndex];
		if (triData.isClipped)
			continue;

		uint4 binAabb = uint4(triData.aabb.x >> 16, triData.aabb.x & 0xffff, triData.aabb.y >> 16, triData.aabb.y & 0xffff) / BIN_SIZE.xyxy;
		for (uint x = binAabb.x; x <= binAabb.z; ++x)
		{
			for (uint y = binAabb.y; y <= binAabb.w; ++y)
			{
				uint binIdx = y * BINNING_DIMS.x + x;
				groupCache[groupIndex][binIdx / 32] |= 1 << (32 - binIdx % 32);
			}
		}

		for (uint idx = 0; idx < BIN_UINT_COUNT; ++idx)
		{
			G_TRIANGLE_BIN_BUFFER.Store((triIndex * BIN_UINT_COUNT + idx) * 4, groupCache[groupIndex][idx]);
		}

		//G_TRIANGLE_BIN_BUFFER.Store((triIndex * BIN_UINT_COUNT) * 4, groupCache[groupIndex][0]);

		//const uint maxStore4 = floor(BIN_UINT_COUNT / 4);
		//for (uint idx = 0; idx < maxStore4; ++idx)
		//	G_TRIANGLE_BIN_BUFFER.Store4((triIndex * BIN_UINT_COUNT + idx) * 4, uint4(groupCache[groupIndex][idx * 4], groupCache[groupIndex][idx * 4 + 1], groupCache[groupIndex][idx * 4 + 2], groupCache[groupIndex][idx * 4 + 3]));

		//const uint remainingStore = BIN_UINT_COUNT - maxStore4 * 4;
		//const uint startShared = maxStore4 * 4;
		//const uint startStore = triIndex * BIN_UINT_COUNT + maxStore4;
		//for (uint idx2 = 0; idx2 < remainingStore; ++idx2)
		//	G_TRIANGLE_BIN_BUFFER.Store((startStore + idx2) * 4, groupCache[groupIndex][startShared + idx2]);
	}
}