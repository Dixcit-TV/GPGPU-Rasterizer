#include "../Libs/Common.hlsli"

#define VIEWPORT_WIDTH 1920.f
#define VIEWPORT_HEIGHT 1080.f
#define BIN_SIZE uint2(64, 64)
#define BIN_PixelCount BIN_SIZE.x * BIN_SIZE.y
#define BINNING_DIMS uint2(ceil(VIEWPORT_WIDTH / BIN_SIZE.x), ceil(VIEWPORT_HEIGHT / BIN_SIZE.y))

#define GROUP_X 32
#define GROUP_Y 32
#define GROUP_DIMs GROUP_X, GROUP_Y, 1
#define UINT3_GROUP_DIMs uint3(GROUP_DIMs)

#define LIGHT_DIR float3(0.f, -1.f, 0.f)
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

RWTexture2D<float4> G_RENDER_TARGET: register(u0);
RWTexture2D<uint> G_DEPTH_BUFFER : register(u1);

StructuredBuffer<RasterData> G_RASTER_DATA;
ByteAddressBuffer G_TRIANGLE_BIN_BUFFER;
StructuredBuffer<Vertex_Out> G_TRANS_VERTEX_BUFFER;
ByteAddressBuffer G_INDEX_BUFFER;

[numthreads(GROUP_DIMs)]
void main(uint groupIndex : SV_GroupIndex, uint3 dispatchID : SV_GroupId)
{
	const uint threadCount = GROUP_X * GROUP_Y;
	//G_TRIANGLE_BIN_BUFFER.GetDimensions(0, binBufferSize.x, binBufferSize.y, levelCount);
	//uint binIdx = dispatchID.y * BINNING_DIMS.x + dispatchID.x;
	uint counter = 0;
	uint triIndex = 0;
	while((triIndex = threadCount * counter + groupIndex) < triangleCount)
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
			uint3 tri = G_INDEX_BUFFER.Load3(triIndex * 3 * 4);
			const Vertex_Out v0 = G_TRANS_VERTEX_BUFFER[tri.x];
			const Vertex_Out v1 = G_TRANS_VERTEX_BUFFER[tri.y];
			const Vertex_Out v2 = G_TRANS_VERTEX_BUFFER[tri.z];

			uint4 aabb4 = uint4(triData.aabb.x >> 16, triData.aabb.x & 0xffff, triData.aabb.y >> 16, triData.aabb.y & 0xffff);
			float3 cy = float3(triData.edgeEq[6], triData.edgeEq[7], triData.edgeEq[8]);

			for (uint y = aabb4.y; y <= aabb4.w; ++y)
			{
				float3 cx = cy;
				for (uint x = aabb4.x; x <= aabb4.z; ++x)
				{
					if (cx.x >= 0 && cx.y >= 0 && cx.z >= 0)
					{
						const float3 weights = cx * triData.invArea;
						const float z = 1.f / dot(1.f / float3(v0.position.z, v1.position.z, v2.position.z), weights);
						const float w = 1 / dot(float3(v0.position.w, v1.position.w, v2.position.w), weights);

						if (asuint(z) < G_DEPTH_BUFFER[uint2(x, y)])
						{
							float3 normal = (v0.normal * v0.position.w * weights.x + v1.normal * v1.position.w * weights.y + v2.normal * v2.position.w * weights.z) * w;
							normal = normalize(normal);
							float diffuseStrength = saturate(dot(normal, -LIGHT_DIR)) * LIGHT_INTENSITY;
							diffuseStrength /= PI;

							G_RENDER_TARGET[uint2(x, y)] = float4(float3(0.5f, 0.5f, 0.5f) * diffuseStrength, 1.f);
						}
					}
					cx += float3(triData.edgeEq[0], triData.edgeEq[2], triData.edgeEq[4]);
				}
				cy += float3(triData.edgeEq[1], triData.edgeEq[3], triData.edgeEq[5]);
			}
		}
		//}
		++counter;
	}
}