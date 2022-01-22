#include "../Libs/Common.hlsli"

#define VIEWPORT_WIDTH 1920.f
#define VIEWPORT_HEIGHT 1080.f
#define TILE_SIZE uint2(8, 8)
#define BIN_SIZE uint2(16, 16)
#define BIN_PIXEL_SIZE (BIN_SIZE * TILE_SIZE)
#define BINNING_DIMS uint2(ceil(VIEWPORT_WIDTH / BIN_PIXEL_SIZE.x), ceil(VIEWPORT_HEIGHT / BIN_PIXEL_SIZE.y))
#define BIN_COUNT (BINNING_DIMS.x * BINNING_DIMS.y)

#define GROUP_X 32
#define GROUP_Y 16
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

struct BinData
{
	uint2x4 coverage;
	uint triIdx;
};

ByteAddressBuffer G_BIN_BUFFER : register(t0);
StructuredBuffer<RasterData> G_RASTER_DATA : register(t1);

RWByteAddressBuffer G_BIN_COUNTER : register(u2);
RWStructuredBuffer<BinData> G_TILE_BUFFER : register(u3); // 3D array binCountX * binCountY * triangleCount : testing with 15 * 9 * 11k

uint2x4 GetCoverage(uint4 clampedAabb, uint2 binSize);

groupshared uint GroupBin[GROUP_Y]; // 16 * uint, store the the bin ID the wrap is processing

[numthreads(GROUP_DIMs)] // 32, 16, 1
void main(uint3 groupThreadId : SV_GroupThreadID, uint3 groupId : SV_GroupID) // process 1 bin per wrap, for 16 wraps, with enough dispatch
{
	const uint queueCount = 16;
	const uint batchSize = ceil(triangleCount / (float)queueCount);

	uint binIdx = (groupId.y * 15 + groupId.x) * GROUP_Y + groupThreadId.y /*GroupBin[groupThreadId.y]*/;
	if (binIdx >= BIN_COUNT) // BIN_COUNT = 15 * 9
		return;

	uint tileDataStart = binIdx * batchSize * queueCount;
	uint queueDataStart = binIdx * (batchSize + 1) * queueCount;
	uint triCount = G_BIN_BUFFER.Load(queueDataStart * 4);
	uint totalCount = 0;
	uint queueId = 0;

	uint4 binAabb;
	binAabb.xy = uint2(binIdx % BINNING_DIMS.x, binIdx / BINNING_DIMS.x) * BIN_PIXEL_SIZE; // BIN_PIXEL_SIZE uint2(128, 128)
	binAabb.zw = binAabb.xy + BIN_PIXEL_SIZE;
	uint dataIndex = groupThreadId.x;

	for (;;)
	{
		if (dataIndex < triCount)
		{
			const uint tri = G_BIN_BUFFER.Load((queueDataStart + 1 + dataIndex) * 4);
			const uint2 aabb_16 = G_RASTER_DATA[tri].aabb;
			uint4 triAabb = uint4(aabb_16.x >> 16, aabb_16.x & 0xffff, aabb_16.y >> 16, aabb_16.y & 0xffff);
			triAabb = clamp(triAabb, binAabb.xyxy, binAabb.zwzw) - binAabb.xyxy;
			triAabb.xy = triAabb.xy / TILE_SIZE;
			triAabb.zw = ceil(triAabb.zw / (float2)TILE_SIZE);
			BinData data = (BinData)0;
			data.coverage = GetCoverage(triAabb, BIN_SIZE);
			data.triIdx = tri;
			G_TILE_BUFFER[tileDataStart + totalCount + dataIndex] = data;

			dataIndex += GROUP_X;
		}
		else
		{
			++queueId;
			totalCount += triCount;
			if (queueId >= queueCount)
				break;

			queueDataStart += batchSize + 1;
			dataIndex -= triCount;
			triCount = G_BIN_BUFFER.Load(queueDataStart * 4);
		}
	}

	if (groupThreadId.x == 0)
		G_BIN_COUNTER.Store(binIdx * 4, totalCount);
}

uint2x4 GetCoverage(uint4 clampedAabb, uint2 binSize)
{
	uint coverageMask[8] = { 0, 0, 0, 0, 0, 0, 0, 0};
	for (uint y = clampedAabb.y; y < clampedAabb.w; ++y)
	{
		for (uint x = clampedAabb.x; x < clampedAabb.z; ++x)
		{
			const uint bitOffset = y * binSize.x + x;
			coverageMask[y / 2] |= 1 << (bitOffset % 32);
		}
	}

	return uint2x4( coverageMask[0], coverageMask[1], coverageMask[2], coverageMask[3], coverageMask[4], coverageMask[5], coverageMask[6], coverageMask[7]);
}