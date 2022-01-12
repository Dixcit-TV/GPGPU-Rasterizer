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
	uint triIdx;
};

RWTexture2D<unorm float4> G_RENDER_TARGET: register(u0);
RWTexture2D<float> G_DEPTH_BUFFER : register(u1);

StructuredBuffer<RasterData> G_RASTER_DATA;
ByteAddressBuffer G_TRIANGLE_BIN_BUFFER;
StructuredBuffer<Vertex_Out> G_TRANS_VERTEX_BUFFER;
ByteAddressBuffer G_INDEX_BUFFER;

groupshared FragData groupCache[GROUP_X * GROUP_Y]; // 32 * 16 * 13 * 4 = 26k bytes
groupshared uint groupCounter = 0;						// 4 = 4 bytes
groupshared uint groupTriIndex = 0;						// 4 = 4 bytes
													// ~26k bytes

float Remap(float val, float min, float max)
{
	return (val - min) / (max - min);
}

[numthreads(GROUP_DIMs)]
void main(uint groupIndex : SV_GroupIndex, uint3 dispatchID : SV_GroupId)
{
	const uint binIdx = dispatchID.y * BINNING_DIMS.x + dispatchID.x;
	const uint2 binCoord = dispatchID.xy * BIN_SIZE;
	const uint threadCount = GROUP_X * GROUP_Y;
	const uint pixelPerThread = (uint)ceil(BIN_PixelCount / (float)threadCount);

	for(;;)
	{
		if (groupIndex == 0)
			groupCounter = 0;

		GroupMemoryBarrierWithGroupSync();

		uint triIndex = 0;
		InterlockedAdd(groupTriIndex, 1, triIndex);
		//groupCache[groupIndex] = (FragData)0;
		while (triIndex < triangleCount)
		{
			bool binMask = G_TRIANGLE_BIN_BUFFER.Load((triIndex * BIN_UINT_COUNT + binIdx / 32u) * 4) & (1 << (binIdx % 32u));

			if (binMask)
			{
				RasterData rData = G_RASTER_DATA[triIndex];

				if (!rData.isClipped)
				{
					uint counter;
					InterlockedAdd(groupCounter, 1, counter);
					groupCache[counter].edgeEq = rData.edgeEq;
					groupCache[counter].aabb = rData.aabb;
					groupCache[counter].invArea = rData.invArea;
					groupCache[counter].triIdx = triIndex;

					break;
				}
			}

			InterlockedAdd(groupTriIndex, 1, triIndex);
		}
		GroupMemoryBarrierWithGroupSync();

		if (groupCounter == 0)
			break;

		const uint startPixel = groupIndex * pixelPerThread;
		const uint endPixel = startPixel + pixelPerThread;
		for (uint triId = 0; triId < groupCounter; ++triId)
		{
			FragData triData = groupCache[triId];
			uint4 aabb4 = uint4(triData.aabb.x >> 16, triData.aabb.x & 0xffff, triData.aabb.y >> 16, triData.aabb.y & 0xffff);
			float3 baseCy = float3(triData.edgeEq[6], triData.edgeEq[7], triData.edgeEq[8]);

			uint3 tri = G_INDEX_BUFFER.Load3(triData.triIdx * 3 * 4);
			const Vertex_Out v0 = G_TRANS_VERTEX_BUFFER[tri.x];
			const Vertex_Out v1 = G_TRANS_VERTEX_BUFFER[tri.y];
			const Vertex_Out v2 = G_TRANS_VERTEX_BUFFER[tri.z];

			for (uint pixel = startPixel; pixel < endPixel; ++pixel)
			{
				float2 screenPixelCoord = binCoord + float2(pixel % BIN_SIZE.x, pixel / BIN_SIZE.x);

				float2 pixelOffset = float2(screenPixelCoord - aabb4.xy);
				float3 cy = baseCy + float3(triData.edgeEq[1], triData.edgeEq[3], triData.edgeEq[5]) * pixelOffset.y;
				float3 cx = cy + float3(triData.edgeEq[0], triData.edgeEq[2], triData.edgeEq[4]) * pixelOffset.x;
				if (all(cx >= 0.f))
				{
					const float3 weights = cx * triData.invArea;
					const float z = 1.f / dot(1.f / float3(v0.position.z, v1.position.z, v2.position.z), weights);
					if (z < G_DEPTH_BUFFER[screenPixelCoord])
					{
						G_DEPTH_BUFFER[screenPixelCoord] = z;
						G_RENDER_TARGET[screenPixelCoord] = float4(float3(0.5f, 0.5f, 0.5f), 1.f);
					}
				}
			}
		}

		//uint pixelId = 0, pixelCounter = 0;
		//while ((pixelId = threadCount * pixelCounter + groupIndex) < BIN_PixelCount)
		//{
		//	float2 screenPixelCoord = binCoord + float2(pixelId % BIN_SIZE.x, pixelId / BIN_SIZE.x) + (0.5f).xx;

		//	unorm float4 color = G_RENDER_TARGET.Load(screenPixelCoord);
		//	float depthValue = G_DEPTH_BUFFER.Load(screenPixelCoord);
		//	++pixelCounter;
		//	for (uint triId = 0; triId < groupCounter; ++triId)
		//	{
		//		FragData triData = groupCache[triId];

		//		uint4 aabb4 = uint4(triData.aabb.x >> 16, triData.aabb.x & 0xffff, triData.aabb.y >> 16, triData.aabb.y & 0xffff);

		//		float2 pixelOffset = float2(screenPixelCoord - aabb4.xy);
		//		float3 cy = float3(triData.edgeEq[6], triData.edgeEq[7], triData.edgeEq[8]) + float3(triData.edgeEq[1], triData.edgeEq[3], triData.edgeEq[5]) * pixelOffset.y;
		//		float3 cx = cy + float3(triData.edgeEq[0], triData.edgeEq[2], triData.edgeEq[4]) * pixelOffset.x;
		//		if (cx.x >= 0 && cx.y >= 0 && cx.z >= 0)
		//		{
		//			uint3 tri = G_INDEX_BUFFER.Load3(triData.triIdx * 3 * 4);
		//			const Vertex_Out v0 = G_TRANS_VERTEX_BUFFER[tri.x];
		//			const Vertex_Out v1 = G_TRANS_VERTEX_BUFFER[tri.y];
		//			const Vertex_Out v2 = G_TRANS_VERTEX_BUFFER[tri.z];

		//			const float3 weights = cx * triData.invArea;
		//			const float z = 1.f / dot(1.f / float3(v0.position.z, v1.position.z, v2.position.z), weights);

		//			if (z < depthValue)
		//			{
		//				depthValue = z;
		//				float remapZ = Remap(z, 0.99f, 1.f);

		//				const float w = 1 / dot(float3(v0.position.w, v1.position.w, v2.position.w), weights);

		//				float3 normal = (v0.normal * v0.position.w * weights.x + v1.normal * v1.position.w * weights.y + v2.normal * v2.position.w * weights.z) * w;
		//				normal = normalize(normal);
		//				float diffuseStrength = saturate(dot(normal, -LIGHT_DIR)) * LIGHT_INTENSITY;
		//				diffuseStrength /= PI;
		//				diffuseStrength = 1.f;

		//				color = float4((0.5f).xxx * diffuseStrength, 1.f);
		//				//G_RENDER_TARGET[screenPixelCoord] = color;
		//			}
		//		}
		//	}

		//	G_RENDER_TARGET[screenPixelCoord] = color;
		//	G_DEPTH_BUFFER[screenPixelCoord] = depthValue;
		//}

		GroupMemoryBarrierWithGroupSync();
	}
}