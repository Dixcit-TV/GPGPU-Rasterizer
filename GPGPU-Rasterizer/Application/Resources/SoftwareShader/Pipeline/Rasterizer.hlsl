#include "../Libs/Common.hlsli"

#define VIEWPORT_WIDTH 1920.f
#define VIEWPORT_HEIGHT 1080.f
#define BIN_SIZE uint2(512, 512)
#define BIN_PixelCount BIN_SIZE.x * BIN_SIZE.y
#define BINNING_DIMS uint2(ceil(VIEWPORT_WIDTH / BIN_SIZE.x), ceil(VIEWPORT_HEIGHT / BIN_SIZE.y))

#define GROUP_X 32
#define GROUP_Y 32
#define GROUP_DIMs GROUP_X, GROUP_Y, 1
#define UINT3_GROUP_DIMs uint3(GROUP_DIMs)

#define TRIANGLE_COUNT 11638

struct RasterData
{
	float edgeEq[12];
	uint2 aabb;
	float invArea;
	uint isClipped;
};

StructuredBuffer<RasterData> G_RASTER_DATA;

RWByteAddressBuffer G_TRIANGLE_BIN_BUFFER : register(u2);

//groupshared uint g_Cache[GROUP_X][GROUP_Y];

[numthreads(GROUP_DIMs)]
void main(uint groupIndex : SV_GroupIndex, uint3 dispatchID : SV_GroupId)
{
	const uint globalThreadId = FlattenedGlobalThreadId(groupIndex, dispatchID, UINT3_GROUP_DIMs, uint3(64, 1, 1) /*G_DISPATCH_DIMS*/);
	const uint threadCount = GROUP_X * GROUP_Y * 1;
	uint loopCount = 0;

	uint triIndex = 0;
	while ((triIndex = loopCount * threadCount + globalThreadId) < TRIANGLE_COUNT)
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
				//uint2 maskIdx = uint2(binIdx, triIndex / 32);
				//InterlockedOr(G_TRIANGLE_BIN_BUFFER[maskIdx], 1 << (32 - triIndex % 32));
				G_TRIANGLE_BIN_BUFFER.Store((binIdx * TRIANGLE_COUNT + triIndex) * 4, 1);
			}
		}
	}
}