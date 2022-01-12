#include "../Libs/Common.hlsli"

#define VIEWPORT_WIDTH 1920.f
#define VIEWPORT_HEIGHT 1080.f
#define BIN_SIZE uint2(64, 64)
#define BIN_PixelCount BIN_SIZE.x * BIN_SIZE.y
#define BINNING_DIMS uint2(ceil(VIEWPORT_WIDTH / BIN_SIZE.x), ceil(VIEWPORT_HEIGHT / BIN_SIZE.y))
#define BIN_COUNT BINNING_DIMS.x * BINNING_DIMS.y
#define BIN_UINT_COUNT ceil(BIN_COUNT / 32.f)

#define GROUP_X 32
#define GROUP_Y 16
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

struct FragData
{
	float edgeEq[9];
	uint2 aabb;
	float invArea;
};

RWTexture2D<unorm float4> G_RENDER_TARGET: register(u0);
RWTexture2D<float> G_DEPTH_BUFFER : register(u1);

StructuredBuffer<RasterData> G_RASTER_DATA;
ByteAddressBuffer G_TRIANGLE_BIN_BUFFER;
StructuredBuffer<Vertex_Out> G_TRANS_VERTEX_BUFFER;
ByteAddressBuffer G_INDEX_BUFFER;

groupshared uint tri[GROUP_Y][GROUP_X];			// 16 * 32 * 4 = 2k
groupshared uint2 coverage[GROUP_Y][GROUP_X];	// 16 * 32 * 8 = 4k
groupshared uint groupCounter[GROUP_Y];			// 16 * 4 = 64 bytes
groupshared uint groupMask[GROUP_Y];			// 16 * 4 = 64 bytes
groupshared uint groupTriIndex[GROUP_Y];		// 16 * 4 = 64 bytes
groupshared uint warpTileIdx[GROUP_Y];	// 16 * 4 = 64 bytes
												// ~6k bytes

RWByteAddressBuffer TileCounter;

float Remap(float val, float min, float max)
{
	return (val - min) / (max - min);
}
//
//[numthreads(GROUP_DIMs)]
//void main(uint3 groupThread : SV_GroupThreadID, uint3 dispatchID : SV_GroupId)
//{
//	warpTileIdx[groupThread.y] = 0;
//	GroupMemoryBarrierWithGroupSync();
//
//	const uint triLoopLimit = ceil(triangleCount / GROUP_X) * GROUP_X;
//	uint tileId = 0;
//	for(;;)
//	{
//		if (groupThread.x == 0)
//		{
//			TileCounter.InterlockedAdd(0, 1, warpTileIdx[groupThread.y]);
//		}
//		tileId = warpTileIdx[groupThread.y];
//
//		if (tileId >= BIN_COUNT * 64)
//			break;
//
//		uint binId = tileId / 64;
//		uint tileid = tileId % 64;
//		uint2 tileCoord = uint2(binId % BINNING_DIMS.x, binId / BINNING_DIMS.x) * BIN_SIZE + uint2(tileid % 8, tileid / 8) * uint2(8, 8);
//
//		uint2 pixelIds = (groupThread.x * 2).xx + uint2(0, 1);
//		uint2 pixel0 = tileCoord + uint2(pixelIds.x % 8, pixelIds.x / 8);
//		uint2 pixel1 = tileCoord + uint2(pixelIds.y % 8, pixelIds.y / 8);
//		//unorm float4 color0 = G_RENDER_TARGET[pixel0];
//		//unorm float4 color1 = G_RENDER_TARGET[pixel1];
//
//		uint triIndex = groupThread.x;
//		while (triIndex < triLoopLimit)
//		{
//			if (triIndex < triangleCount)
//			{
//				uint count = 0;
//				while (count < 32 && triIndex < triangleCount)
//				{
//					if (groupThread.x == 0)
//						groupMask[groupThread.y] = 0;
//
//					bool binMask = false;
//					if (triIndex < triangleCount)
//						binMask = G_TRIANGLE_BIN_BUFFER.Load((triIndex * BIN_UINT_COUNT + binId / 32u) * 4) & (1 << (32u - binId % 32u));
//
//					InterlockedOr(groupMask[groupThread.y], (binMask * 1) << groupThread.x);
//
//					uint mask = groupMask[groupThread.y];
//					if (mask & (1 << groupThread.x))
//					{
//						uint cacheId = count + countbits(mask << (31 - groupThread.x));
//						if (cacheId < 32)
//						{
//							tri[groupThread.y][cacheId] = triIndex;
//						}
//					}
//
//					count += countbits(mask);
//					triIndex += GROUP_X + min(32 - count, 0);
//				}
//				count = min(32, count);
//
//				if (groupThread.x < count)
//				{
//					uint2 covMask = (0).xx;
//					uint dataIdx = tri[groupThread.y][groupThread.x];
//					RasterData rData = G_RASTER_DATA[dataIdx];
//
//					if (!rData.isClipped)
//					{
//						uint4 aabb4 = uint4(rData.aabb.x >> 16, rData.aabb.x & 0xffff, rData.aabb.y >> 16, rData.aabb.y & 0xffff);
//						uint4 clampAabb;
//						clampAabb.xy = max(aabb4.xy, tileCoord);
//						clampAabb.zw = min(aabb4.zw, tileCoord + uint2(8, 8));
//
//						float3 baseCy = float3(rData.edgeEq[6], rData.edgeEq[7], rData.edgeEq[8]);
//						float3 cy = baseCy + float3(rData.edgeEq[1], rData.edgeEq[3], rData.edgeEq[5]) * (clampAabb.y - aabb4.y);
//						for (uint y = clampAabb.y; y <= clampAabb.w; ++y)
//						{
//							float3 cx = cy + float3(rData.edgeEq[0], rData.edgeEq[2], rData.edgeEq[4]) * (clampAabb.x - aabb4.x);
//							for (uint x = clampAabb.x; x <= clampAabb.z; ++x)
//							{
//								if (cx.x >= 0 && cx.y >= 0 && cx.z >= 0)
//								{
//									uint pixelId = y * 8 + x;
//									if ((pixelId / 32) == 0)
//										covMask[1] |= 1 << (pixelId % 32);
//									else
//										covMask[0] |= 1 << (pixelId % 32);
//
//									G_RENDER_TARGET[uint2(x, y)] = float4(float3(0.5f, 0.5f, 0.5f), 1.f);
//								}
//								cx += float3(rData.edgeEq[0], rData.edgeEq[2], rData.edgeEq[4]);
//							}
//							cy += float3(rData.edgeEq[1], rData.edgeEq[3], rData.edgeEq[5]);
//						}
//					}
//					coverage[groupThread.y][groupThread.x] = covMask;
//
//					for (uint cacheIdx = 0; cacheIdx < count; ++cacheIdx)
//					{
//						//uint triIndex = tri[groupThread.y][cacheIdx];
//						bool pixel1Cover = coverage[groupThread.y][cacheIdx][1 - (pixelIds.x / 32)] & 1 << (pixelIds.x % 32);
//						bool pixel2Cover = coverage[groupThread.y][cacheIdx][1 - (pixelIds.y / 32)] & 1 << (pixelIds.y % 32);
//
//						if (pixel1Cover)
//							G_RENDER_TARGET[pixel0] = float4(float3(0.5f, 0.5f, 0.5f), 1.f);
//						if (pixel2Cover)
//							G_RENDER_TARGET[pixel1] = float4(float3(0.5f, 0.5f, 0.5f), 1.f);
//					}
//				}
//			}
//
//			//G_RENDER_TARGET[pixel0] = color0;
//			//G_RENDER_TARGET[pixel1] = color1;
//		}
//	}
//}

[numthreads(GROUP_DIMs)]
void main(uint3 groupThread : SV_GroupThreadID, uint3 dispatchID : SV_GroupId)
{
	warpTileIdx[groupThread.y] = 0;
	GroupMemoryBarrierWithGroupSync();

	const uint triLoopLimit = ceil(triangleCount / GROUP_X) * GROUP_X;
	uint tileId = 0;
	for (;;)
	{
		if (groupThread.x == 0)
		{
			TileCounter.InterlockedAdd(0, 1, warpTileIdx[groupThread.y]);
		}
		tileId = warpTileIdx[groupThread.y];

		if (tileId >= BIN_COUNT * 64)
			break;

		uint binId = tileId / 64;
		uint tileid = tileId % 64;
		uint2 tileCoord = uint2(binId % BINNING_DIMS.x, binId / BINNING_DIMS.x) * BIN_SIZE + uint2(tileid % 8, tileid / 8) * uint2(8, 8);

		uint2 pixelIds = (groupThread.x * 2).xx + uint2(0, 1);
		uint2 pixel0 = tileCoord + uint2(pixelIds.x % 8, pixelIds.x / 8);
		uint2 pixel1 = tileCoord + uint2(pixelIds.y % 8, pixelIds.y / 8);
		unorm float4 color0 = G_RENDER_TARGET[pixel0];
		unorm float4 color1 = G_RENDER_TARGET[pixel1];

		uint triIndex = groupThread.x;
		while (triIndex < triLoopLimit)
		{
			if (triIndex < triangleCount)
			{
				uint2 covMask = (0).xx;
				bool binMask = G_TRIANGLE_BIN_BUFFER.Load((triIndex * BIN_UINT_COUNT + binId / 32u) * 4) & (1 << (binId % 32u));

				if (binMask)
				{
					RasterData rData = G_RASTER_DATA[triIndex];

					if (!rData.isClipped)
					{
						uint4 aabb4 = uint4(rData.aabb.x >> 16, rData.aabb.x & 0xffff, rData.aabb.y >> 16, rData.aabb.y & 0xffff);
						uint4 clampAabb;
						clampAabb.xy = max(aabb4.xy, tileCoord);
						clampAabb.zw = min(aabb4.zw, tileCoord + uint2(8, 8));

						float3 baseCy = float3(rData.edgeEq[6], rData.edgeEq[7], rData.edgeEq[8]);
						float3 cy = baseCy + float3(rData.edgeEq[1], rData.edgeEq[3], rData.edgeEq[5]) * (clampAabb.y - aabb4.y);
						for (uint y = clampAabb.y; y <= clampAabb.w; ++y)
						{
							float3 cx = cy + float3(rData.edgeEq[0], rData.edgeEq[2], rData.edgeEq[4]) * (clampAabb.x - aabb4.x);
							for (uint x = clampAabb.x; x <= clampAabb.z; ++x)
							{
								if (cx.x >= 0 && cx.y >= 0 && cx.z >= 0)
								{
									uint pixelId = y * 8 + x;
									if ((pixelId / 32) == 0)
										covMask[1] |= 1 << (pixelId % 32);
									else
										covMask[0] |= 1 << (pixelId % 32);

									//G_RENDER_TARGET[uint2(x, y)] = float4(float3(0.5f, 0.5f, 0.5f), 1.f);
								}
								cx += float3(rData.edgeEq[0], rData.edgeEq[2], rData.edgeEq[4]);
							}
							cy += float3(rData.edgeEq[1], rData.edgeEq[3], rData.edgeEq[5]);
						}
					}
				}
				tri[groupThread.y][groupThread.x] = triIndex;
				coverage[groupThread.y][groupThread.x] = covMask;
				triIndex += GROUP_X;

				for (uint cacheIdx = 0; cacheIdx < GROUP_X; ++cacheIdx)
				{
					//uint triIndex = tri[groupThread.y][cacheIdx];
					bool pixel1Cover = coverage[groupThread.y][cacheIdx][1 - (pixelIds.x / 32)] & 1 << (pixelIds.x % 32);
					bool pixel2Cover = coverage[groupThread.y][cacheIdx][1 - (pixelIds.y / 32)] & 1 << (pixelIds.y % 32);

					color0 = pixel1Cover * float4(float3(0.5f, 0.5f, 0.5f), 1.f) + !pixel1Cover * color0;
					color1 = pixel2Cover * float4(float3(0.5f, 0.5f, 0.5f), 1.f) + !pixel2Cover * color1;
				}
			}
		}

		G_RENDER_TARGET[pixel0] = color0;
		G_RENDER_TARGET[pixel1] = color1;
	}
}