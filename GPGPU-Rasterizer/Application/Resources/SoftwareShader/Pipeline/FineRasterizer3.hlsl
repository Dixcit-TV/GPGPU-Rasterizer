#include "../Libs/Common.hlsli"

#define VIEWPORT_WIDTH 1920.f
#define VIEWPORT_HEIGHT 1080.f
#define TILE_SIZE uint2(8, 8)
#define BIN_SIZE uint2(16, 16)
#define BIN_TILE_COUNT (BIN_SIZE.x * BIN_SIZE.y)
#define BIN_PIXEL_SIZE (BIN_SIZE * TILE_SIZE)
#define BINNING_DIMS uint2(ceil(VIEWPORT_WIDTH / BIN_PIXEL_SIZE.x), ceil(VIEWPORT_HEIGHT / BIN_PIXEL_SIZE.y))
#define TILING_DIMS (BINNING_DIMS * BIN_SIZE)
#define BIN_COUNT (BINNING_DIMS.x * BINNING_DIMS.y)
#define TILE_COUNT (TILING_DIMS.x * TILING_DIMS.y)

#define GROUP_X 32
#define GROUP_Y 2
#define THREAD_COUNT (GROUP_X * GROUP_Y)
#define GROUP_DIMs GROUP_X, GROUP_Y, 1
#define UINT3_GROUP_DIMs uint3(GROUP_DIMs)

#define LIGHT_DIR float3(0.577f, -0.577f, 0.577f)
#define LIGHT_INTENSITY 2.f
#define PI 3.14159265358979323846f

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

struct RasterData
{
	float edgeEq[9];
	float3 pad;
	uint2 aabb;
	float invArea;
	uint isClipped;
};

struct BinData
{
	uint2x4 coverage;
	uint triIdx;
};

StructuredBuffer<RasterData> G_RASTER_DATA : register(t0);
StructuredBuffer<BinData> G_TILE_BUFFER : register(t1);
ByteAddressBuffer G_BIN_COUNTER: register(t2);
//StructuredBuffer<Vertex_Out> G_TRANS_VERTEX_BUFFER;
//ByteAddressBuffer G_INDEX_BUFFER;

RWTexture2D<unorm float4> G_RENDER_TARGET: register(u0);
RWTexture2D<float> G_DEPTH_BUFFER : register(u1);
RWByteAddressBuffer G_TILE_COUNTER: register(u2);

//groupshared uint GroupBatch[GROUP_Y][GROUP_X];
//groupshared uint2 GroupCoverage[GROUP_Y][GROUP_X];
//groupshared uint GroupMask[GROUP_Y];
//groupshared uint GroupTile[GROUP_Y];

groupshared uint GroupBatch[THREAD_COUNT];
groupshared uint GroupCoverage[THREAD_COUNT][2];
groupshared uint GroupTile;
groupshared uint GroupMask[2];

[numthreads(GROUP_DIMs)]
void main(int threadId : SV_GroupIndex, int3 groupThreadId : SV_GroupThreadID, int3 groupId : SV_GroupID)
{
	const uint queueCount = 16;
	const uint batchSize = ceil(triangleCount / (float)queueCount);

	//if (threadId == 0)
	//{
	//	G_TILE_COUNTER.InterlockedAdd(0, 1, GroupTile);
	//	GroupMask[0] = GroupMask[1] = 0;
	//}

	//GroupMemoryBarrierWithGroupSync();

	uint tileIdx = groupId.x + groupId.y * 240;//GroupTile;
	if (tileIdx >= TILE_COUNT)
		return;

	const uint binIdx = tileIdx / BIN_TILE_COUNT;
	const uint binDataStart = binIdx * batchSize * queueCount;
	const uint triCount = G_BIN_COUNTER.Load(binIdx * 4);
	const uint loopCount = ceil(triCount / (float)THREAD_COUNT);

	const uint2 binCoord = uint2(binIdx % BINNING_DIMS.x, binIdx / BINNING_DIMS.x) * BIN_PIXEL_SIZE;

	const uint binTileId = tileIdx % BIN_TILE_COUNT;
	uint4 tileAabb;
	tileAabb.xy = binCoord + uint2(binTileId % BIN_SIZE.x, binTileId / BIN_SIZE.x) * TILE_SIZE;
	tileAabb.zw = tileAabb.xy + TILE_SIZE;

	uint2 pixel = tileAabb.xy + uint2(threadId % TILE_SIZE.x, threadId / TILE_SIZE.x);
	unorm float4 color = G_RENDER_TARGET[pixel];

	uint triIndex = threadId;
	uint loop = 0;
	while (loop < loopCount)
	{
		++loop;
		int batchCount = 0;
		uint count = 0;
		do {
			count = 0;
			if (threadId == 0)
			{
				GroupMask[0] = GroupMask[1] = 0;
			}
			GroupMemoryBarrierWithGroupSync();

			uint triMask = 0;
			BinData triBinData = (BinData)0;
			if (triIndex < triCount)
			{
				triBinData = G_TILE_BUFFER[binDataStart + triIndex];
				const uint bitIdx = (binTileId % 128);
				triMask = (triBinData.coverage[binTileId / 128][bitIdx / 32] & (1 << (bitIdx % 32))) != 0;
				//triMask = 1;
			}
			InterlockedOr(GroupMask[groupThreadId.y], triMask << groupThreadId.x);
			GroupMemoryBarrierWithGroupSync();

			if (triMask)
			{
				uint cacheId = batchCount;
				for (int idx = 0; idx < groupThreadId.y; ++idx)
					cacheId += countbits(GroupMask[idx]);

				cacheId += countbits(GroupMask[groupThreadId.y] << (31 - groupThreadId.x)) - 1;

				if (cacheId < THREAD_COUNT)
					GroupBatch[cacheId] = triBinData.triIdx;
			}

			count = countbits(GroupMask[0]) + countbits(GroupMask[1]);
			batchCount += count;
			triIndex += THREAD_COUNT;
			GroupMemoryBarrierWithGroupSync();
		} while (batchCount < THREAD_COUNT && count != 0);

		triIndex -= max(batchCount - THREAD_COUNT, 0);
		batchCount = min(batchCount, THREAD_COUNT);

		uint covMask[2] = { 0, 0 };
		if (threadId < batchCount)
		{
			const uint processId = GroupBatch[threadId];
			const RasterData triData = G_RASTER_DATA[processId];
			uint4 triAabb = uint4(triData.aabb.x >> 16, triData.aabb.x & 0xffff, triData.aabb.y >> 16, triData.aabb.y & 0xffff);
			uint4 clampedAabb = clamp(triAabb, tileAabb.xyxy, tileAabb.zwzw);
			//clampedAabb.xy = max(triAabb.xy, tileAabb.xy);
			//clampedAabb.zw = min(triAabb.zw, tileAabb.zw);
			//uint4 clampedAabb = tileAabb;
			uint2 startPixels = clampedAabb.xy;

			clampedAabb -= tileAabb.xyxy;
			float3 cy = float3(triData.edgeEq[6], triData.edgeEq[7], triData.edgeEq[8]) + float3(triData.edgeEq[1], triData.edgeEq[3], triData.edgeEq[5]) * (startPixels.y - triAabb.y);
			for (uint y = clampedAabb.y; y < clampedAabb.w; ++y)
			{
				float3 cx = cy + float3(triData.edgeEq[0], triData.edgeEq[2], triData.edgeEq[4]) * (startPixels.x - triAabb.x);
				for (uint x = clampedAabb.x; x < clampedAabb.z; ++x)
				{
					if (cx.x >= 0 && cx.y >= 0 && cx.z >= 0)
					{
						uint pixelId = y * TILE_SIZE.x + x;
						covMask[(pixelId / 32)] |= 1 << (pixelId % 32);
					}
					cx += float3(triData.edgeEq[0], triData.edgeEq[2], triData.edgeEq[4]);
				}
				cy += float3(triData.edgeEq[1], triData.edgeEq[3], triData.edgeEq[5]);
			}
		}
		GroupCoverage[threadId] = covMask;
		GroupMemoryBarrierWithGroupSync();

		for (int cacheIdx = 0; cacheIdx < batchCount; ++cacheIdx)
		{
			//uint triIndex = tri[groupThread.y][cacheIdx];
			if (GroupCoverage[cacheIdx][groupThreadId.y] & 1 << groupThreadId.x)
				color = float4(float3(0.5f, 0.5f, 0.5f), 1.f);
		}
	}

	G_RENDER_TARGET[pixel] = color;
	//G_RENDER_TARGET[pixel1] = color1;
}

//while (triIndex < triangleCount)
//{
//	uint tileDataIdx = BIN_COUNT * triIndex * 8 + (tileIdx / 32);
//	if (G_TILE_BUFFER.Load(tileDataIdx * 4) & (1 << (tileIdx % 32)))
//	{
//		const RasterData triData = G_RASTER_DATA[triIndex];
//		uint4 triAabb = uint4(triData.aabb.x >> 16, triData.aabb.x & 0xffff, triData.aabb.y >> 16, triData.aabb.y & 0xffff);
//		uint4 clampedAabb;
//		clampedAabb.xy = max(triAabb.xy, tileAabb.xy);
//		clampedAabb.zw = min(triAabb.zw, tileAabb.zw);
//
//		float3 cy = float3(triData.edgeEq[6], triData.edgeEq[7], triData.edgeEq[8]) + float3(triData.edgeEq[1], triData.edgeEq[3], triData.edgeEq[5]) * (clampedAabb.y - triAabb.y);
//		for (uint y = clampedAabb.y; y <= clampedAabb.w; ++y)
//		{
//			float3 cx = cy + float3(triData.edgeEq[0], triData.edgeEq[2], triData.edgeEq[4]) * (clampedAabb.x - triAabb.x);
//			for (uint x = clampedAabb.x; x <= clampedAabb.z; ++x)
//			{
//				if (cx.x >= 0 && cx.y >= 0 && cx.z >= 0)
//				{
//					G_RENDER_TARGET[uint2(x, y)] = float4(float3(0.5f, 0.5f, 0.5f), 1.f);
//				}
//				cx += float3(triData.edgeEq[0], triData.edgeEq[2], triData.edgeEq[4]);
//			}
//			cy += float3(triData.edgeEq[1], triData.edgeEq[3], triData.edgeEq[5]);
//		}
//	}
//	triIndex += THREAD_COUNT;
//}