#include "../Libs/Common.hlsli"

#define VIEWPORT_WIDTH 1920.f
#define VIEWPORT_HEIGHT 1080.f
#define TILE_SIZE uint2(8, 8)
#define BIN_SIZE uint2(8, 8)
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
#define LIGHT_INTENSITY 4.f
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
	float3 invZ;
	uint2 aabb;
	float invArea;
	uint isClipped;
};

struct BinData
{
	uint4 coverage;
	uint triIdx;
};

struct CacheData
{
	float edgeEq[9];
	float3 invZ;
	uint2 startPixel;
	float invArea;
	uint triIdx;
};

StructuredBuffer<RasterData> G_RASTER_DATA : register(t0);
StructuredBuffer<BinData> G_TILE_BUFFER : register(t1);
ByteAddressBuffer G_BIN_TRI_COUNTER : register(t2);
StructuredBuffer<Vertex_Out> G_TRANS_VERTEX_BUFFER : register(t3);
ByteAddressBuffer G_INDEX_BUFFER : register(t4);

RWTexture2D<unorm float4> G_RENDER_TARGET: register(u0);
RWTexture2D<float> G_DEPTH_BUFFER : register(u1);
RWByteAddressBuffer G_TILE_COUNTER: register(u2);

groupshared CacheData GroupBatchData[THREAD_COUNT];
groupshared uint GroupTile;
groupshared uint GroupTriCount;
groupshared uint GroupMask[2];

float Remap(float val, float min, float max)
{
	return (val - min) / (max - min);
}

[numthreads(GROUP_DIMs)]
void main(int threadId : SV_GroupIndex, int3 groupThreadId : SV_GroupThreadID)
{
	const uint queueCount = 16;
	const uint batchSize = ceil(triangleCount / (float)queueCount);

	for (;;)
	{
		if (threadId == 0)
		{
			G_TILE_COUNTER.InterlockedAdd(0, 1, GroupTile);
			GroupMask[0] = GroupMask[1] = 0;

			const uint binIdx = GroupTile / BIN_TILE_COUNT;
			GroupTriCount = G_BIN_TRI_COUNTER.Load(binIdx * 4);
		}

		GroupMemoryBarrierWithGroupSync();

		uint tileIdx = GroupTile;
		if (tileIdx >= TILE_COUNT)
			break;

		const uint binIdx = tileIdx / BIN_TILE_COUNT;
		const uint binDataStart = binIdx * batchSize * queueCount;
		const uint triCount = GroupTriCount;
		GroupMemoryBarrierWithGroupSync();
		if (triCount == 0)
			continue;

		const uint loopCount = ceil(triCount / (float)THREAD_COUNT);

		const uint2 binCoord = uint2(binIdx % BINNING_DIMS.x, binIdx / BINNING_DIMS.x) * BIN_PIXEL_SIZE;

		const uint binTileId = tileIdx % BIN_TILE_COUNT;
		uint4 tileAabb;
		tileAabb.xy = binCoord + uint2(binTileId % BIN_SIZE.x, binTileId / BIN_SIZE.x) * TILE_SIZE;
		tileAabb.zw = tileAabb.xy + TILE_SIZE;

		uint2 pixel = tileAabb.xy + uint2(threadId % TILE_SIZE.x, threadId / TILE_SIZE.x);
		unorm float4 color = G_RENDER_TARGET[pixel];
		float depth = G_DEPTH_BUFFER[pixel];
		float depth2 = G_DEPTH_BUFFER[pixel];

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
					triMask = (triBinData.coverage[binTileId / 32] & (1 << (binTileId % 32))) != 0;
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
					{
						RasterData rData = G_RASTER_DATA[triBinData.triIdx];
						uint4 triAabb = uint4(rData.aabb.x >> 16, rData.aabb.x & 0xffff, rData.aabb.y >> 16, rData.aabb.y & 0xffff);

						CacheData data = (CacheData)0;
						data.startPixel = triAabb.xy;
						data.edgeEq = rData.edgeEq;
						data.invArea = rData.invArea;
						data.triIdx = triBinData.triIdx;
						data.invZ = rData.invZ;
						GroupBatchData[cacheId] = data;
					}
				}

				count = countbits(GroupMask[0]) + countbits(GroupMask[1]);
				batchCount += count;
				triIndex += THREAD_COUNT;
				GroupMemoryBarrierWithGroupSync();
			} while (batchCount < THREAD_COUNT && count != 0);

			triIndex -= max(batchCount - THREAD_COUNT, 0);
			batchCount = min(batchCount, THREAD_COUNT);

			for (int cacheIdx = 0; cacheIdx < batchCount; ++cacheIdx)
			{
				CacheData process = GroupBatchData[cacheIdx];
				float3 cy = float3(process.edgeEq[6], process.edgeEq[7], process.edgeEq[8]) + float3(process.edgeEq[1], process.edgeEq[3], process.edgeEq[5]) * ((int)pixel.y - (int)process.startPixel.y);
				float3 cx = cy + float3(process.edgeEq[0], process.edgeEq[2], process.edgeEq[4]) * ((int)pixel.x - (int)process.startPixel.x);
				if (all(cx > 0))
				{
					uint3 tri = G_INDEX_BUFFER.Load3(process.triIdx * 3 * 4);
					const Vertex_Out v0 = G_TRANS_VERTEX_BUFFER[tri.x];
					const Vertex_Out v1 = G_TRANS_VERTEX_BUFFER[tri.y];
					const Vertex_Out v2 = G_TRANS_VERTEX_BUFFER[tri.z];

					float3 weights = cx * process.invArea;
					weights.z = 1 - weights.x - weights.y;
					const float z = 1.f / dot(process.invZ, weights);

					if (z < depth)
					{
						depth = z;

						const float w = 1 / dot(float3(v0.position.w, v1.position.w, v2.position.w), weights);
						float3 n = (v0.normal * weights.x + v1.normal * weights.y + v2.normal * weights.z) * w;
						n = normalize(n);
						float diffuseStrength = saturate(dot(n, -LIGHT_DIR)) * LIGHT_INTENSITY;
						diffuseStrength /= PI;

						color = float4(float3(0.5f, 0.5f, 0.5f) * diffuseStrength, 1.f);
					}
				}
			}
		}

		G_RENDER_TARGET[pixel] = color;
		G_DEPTH_BUFFER[pixel] = depth;

		GroupMemoryBarrierWithGroupSync();
	}
}