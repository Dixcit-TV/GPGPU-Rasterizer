#include "../Libs/Common.hlsli"

#define VIEWPORT_WIDTH 1920.f
#define VIEWPORT_HEIGHT 1080.f
#define BIN_SIZE uint2(16, 16)
#define BIN_PixelCount BIN_SIZE.x * BIN_SIZE.y
#define BINNING_DIMS uint2(ceil(BIN_SIZE.x / VIEWPORT_WIDTH), ceil(BIN_SIZE.y / VIEWPORT_HEIGHT))

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

RWTexture2D<uint> G_TRIANGLE_BIN_BUFFER : register(u2);

//groupshared uint g_Cache[GROUP_X][GROUP_Y];

uint4 GetAabb(float2 v0, float2 v1, float2 v2, float viewportWidth, float viewportHeight);

[numthreads(GROUP_DIMs)]
void main(uint groupIndex : SV_GroupIndex, uint3 threadId : SV_GroupThreadID, uint3 dispatchID : SV_GroupId)
{
	const uint globalThreadId = FlattenedGlobalThreadId(groupIndex, dispatchID, UINT3_GROUP_DIMs, uint3(64, 1, 1) /*G_DISPATCH_DIMS*/);
	const uint threadCount = dot(UINT3_GROUP_DIMs, (1).xxx) * BINNING_DIMS.x * BINNING_DIMS.y;
	uint loopCount = 0;

	uint triIndex = 0;
	while ((triIndex = loopCount * threadCount + globalThreadId) < TRIANGLE_COUNT)
	{
		++loopCount;

		const RasterData triData = G_RASTER_DATA[triIndex];
		if (triData.isClipped)
			continue;

		uint4 binAabb = uint4(triData.aabb.x >> 16, triData.aabb.x & 0x0000ffff, triData.aabb.y >> 16, triData.aabb.y & 0x0000ffff) / BIN_SIZE.xyxy;
		for (uint x = binAabb.x; x <= binAabb.z; ++x)
		{
			for (uint y = binAabb.y; y <= binAabb.w; ++y)
			{
				uint binIdx = y * BINNING_DIMS.x + x;
				uint2 maskIdx = uint2(binIdx, triIndex / 32);
				InterlockedOr(G_TRIANGLE_BIN_BUFFER[maskIdx], 1 << (32 - triIndex % 32));
			}
		}

		//g_Cache[threadId.x][threadId.y] = uint2((aabb.x << 16) | (0x0000ffff & aabb.y), (aabb.z << 16) | (0x0000ffff & aabb.w));
	}
}

uint4 GetAabb(float2 v0, float2 v1, float2 v2, float viewportWidth, float viewportHeight)
{
	float4 aabb;
	aabb.xy = min(v0.xy, min(v1.xy, v2.xy));
	aabb.zw = ceil(max(v0.xy, max(v1.xy, v2.xy)));
	return asuint(aabb);
}