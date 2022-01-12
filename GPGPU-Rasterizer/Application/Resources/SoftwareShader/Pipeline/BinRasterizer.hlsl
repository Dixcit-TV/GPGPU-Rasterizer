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
RWByteAddressBuffer G_BIN_BUFFER : register(u2);

[numthreads(GROUP_DIMs)]
void main(uint groupIndex : SV_GroupIndex, uint3 dispatchID : SV_GroupId)
{
	const uint batchSize = min(ceil(triangleCount / 24.f), GROUP_X * GROUP_Y);
	const uint batchOffset = batchSize * 24;
	const uint batchStart = batchOffset * dispatchID.x;
	const uint loopCount = ceil((triangleCount / batchSize) / 24.f);
	const uint2 binPixelSize = BIN_SIZE * TILE_SIZE;

	for (uint loop = 0; loop < loopCount; ++loop)
	{
		uint triIdx = batchStart + loop * batchOffset + groupIndex;

		if (triIdx < triangleCount)
		{
			const RasterData triData = G_RASTER_DATA[triIdx];
			if (!triData.isClipped)
			{
				uint4 binAabb = uint4(triData.aabb.x >> 16, triData.aabb.x & 0xffff, triData.aabb.y >> 16, triData.aabb.y & 0xffff) / binPixelSize.xyxy;
				for (uint x = binAabb.x; x <= binAabb.z; ++x)
				{
					for (uint y = binAabb.y; y <= binAabb.w; ++y)
					{
						uint binIdx = x + BINNING_DIMS.x * y + BINNING_DIMS.x * BINNING_DIMS.y * triIdx;
						G_BIN_BUFFER.Store(binIdx * 4, 1);
					}
				}
			}
		}
	}
}