#include "../Libs/Common.hlsli"

#define VIEWPORT_WIDTH 1920.f
#define VIEWPORT_HEIGHT 1080.f
#define TILE_SIZE uint2(8, 8)
#define BIN_SIZE uint2(16, 16)
#define BIN_PIXEL_SIZE (BIN_SIZE * TILE_SIZE)
#define BINNING_DIMS uint2(ceil(VIEWPORT_WIDTH / BIN_PIXEL_SIZE.x), ceil(VIEWPORT_HEIGHT / BIN_PIXEL_SIZE.y))
#define BIN_COUNT (BINNING_DIMS.x * BINNING_DIMS.y)

#define GROUP_X 32
#define GROUP_Y 32
#define THREAD_COUNT (GROUP_X * GROUP_Y)
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
	float3 invZ;
	uint2 aabb;
	float invArea;
	uint isClipped;
};

StructuredBuffer<RasterData> G_RASTER_DATA;
RWByteAddressBuffer G_BIN_BUFFER : register(u2);

groupshared uint GroupBatchTri[THREAD_COUNT];
groupshared uint4 GroupBatchAabb[THREAD_COUNT];

[numthreads(GROUP_DIMs)]
void main(uint groupIndex : SV_GroupIndex, uint3 dispatchID : SV_GroupId)
{
	const uint queueCount = 16;
	const uint batchSize = ceil(triangleCount / (float)queueCount);
	const uint loopCount = ceil(batchSize / (float)THREAD_COUNT);
	const uint batchStart = batchSize * dispatchID.x;
	const uint2 binDim = uint2(groupIndex % BINNING_DIMS.x, groupIndex / BINNING_DIMS.x);
	const uint binDataIdx = (batchSize + 1) * dispatchID.x + groupIndex * (batchSize + 1) * queueCount;

	uint binTriCount = 0;
	uint triIdx = batchStart + groupIndex;
	for (uint loop = 0; loop < loopCount; ++loop)
	{
		if (triIdx < triangleCount && (triIdx - batchStart) < batchSize)
		{
			const RasterData triData = G_RASTER_DATA[triIdx];
			if (!triData.isClipped)
			{
				uint4 binAabb = uint4(triData.aabb.x >> 16, triData.aabb.x & 0xffff, triData.aabb.y >> 16, triData.aabb.y & 0xffff);
				binAabb.xy /= BIN_PIXEL_SIZE;
				binAabb.zw = ceil(binAabb.zw / (float2)BIN_PIXEL_SIZE);
				GroupBatchTri[groupIndex] = triIdx;
				GroupBatchAabb[groupIndex] = binAabb;
			}
			else
				GroupBatchTri[groupIndex] = -1;
		}
		else
			GroupBatchTri[groupIndex] = -1;

		GroupMemoryBarrierWithGroupSync();
		triIdx += THREAD_COUNT;

		if (groupIndex < BIN_COUNT)
		{
			for (uint idx = 0; idx < batchSize; ++idx)
			{
				uint triId = GroupBatchTri[idx];
				uint4 aabb = GroupBatchAabb[idx];
				if (triId != -1 
					&& aabb.x <= binDim.x && aabb.z >= binDim.x
					&& aabb.y <= binDim.y && aabb.w >= binDim.y)
				{
					G_BIN_BUFFER.Store((binDataIdx + 1 + binTriCount) * 4, triId);
					++binTriCount;
				}
			}
		}
		GroupMemoryBarrierWithGroupSync();
	}

	G_BIN_BUFFER.Store(binDataIdx * 4, binTriCount);
}