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

struct Vertex_Out
{
	float4 position;
	float3 normal;
	float pad;
};

struct RasterData
{
	float edgeEq[12];
	uint2 aabb;
	float invArea;
	uint isClipped;
};

RWTexture2D<float4> g_RENDER_TARGET: register(u0);
RWTexture2D<uint> G_DEPTH_BUFFER : register(u1);

StructuredBuffer<RasterData> G_RASTER_DATA;
ByteAddressBuffer G_TRIANGLE_BIN_BUFFER;
StructuredBuffer<Vertex_Out> G_TRANS_VERTEX_BUFFER;
ByteAddressBuffer G_INDEX_BUFFER;

uint4 GetAabb(float2 v0, float2 v1, float2 v2);

[numthreads(GROUP_DIMs)]
void main(uint groupIndex : SV_GroupIndex, uint3 dispatchID : SV_GroupId)
{
	uint levelCount;
	const uint threadCount = GROUP_X * GROUP_Y;
	//G_TRIANGLE_BIN_BUFFER.GetDimensions(0, binBufferSize.x, binBufferSize.y, levelCount);
	uint binIdx = dispatchID.y * BINNING_DIMS.x + dispatchID.x;
	uint counter = 0;
	uint triIndex = 0;
	while((triIndex = threadCount * counter + groupIndex) < TRIANGLE_COUNT)
	{
		//uint2 maskIdx = uint2(binIdx, counter / 32);
		//uint2 maskIdx = uint2(binIdx, counter);

		//int mask = G_TRIANGLE_BIN_BUFFER.Load((binIdx * TRIANGLE_COUNT + triIndex) * 4);
		////if (mask & (1 << (32 - counter % 32)))
		//if (mask == 1)
		//{
		const RasterData triData = G_RASTER_DATA[triIndex];
		if (!triData.isClipped)
		{
			uint4 aabb4 = uint4(triData.aabb.x >> 16, triData.aabb.x & 0xffff, triData.aabb.y >> 16, triData.aabb.y & 0xffff);
			float3 cy = float3(triData.edgeEq[9], triData.edgeEq[10], triData.edgeEq[11]);

			for (uint y = aabb4.y; y <= aabb4.w; ++y)
			{
				float3 cx = cy;
				for (uint x = aabb4.x; x <= aabb4.z; ++x)
				{
					if (cx.x >= 0 && cx.y >= 0 && cx.z >= 0)
					{
						//float3 weights = cx * triData.invArea;
						g_RENDER_TARGET[uint2(x, y)] = float4(float3(0.5f, 0.5f, 0.5f), 1.f);
					}
					cx += float3(triData.edgeEq[0], triData.edgeEq[3], triData.edgeEq[6]);
				}
				cy += float3(triData.edgeEq[1], triData.edgeEq[4], triData.edgeEq[7]);
			}
		}
		//}
		++counter;
	}
}

uint4 GetAabb(float2 v0, float2 v1, float2 v2)
{
	float4 aabb;
	aabb.xy = min(v0.xy, min(v1.xy, v2.xy));
	aabb.zw = ceil(max(v0.xy, max(v1.xy, v2.xy)));
	return aabb;
}