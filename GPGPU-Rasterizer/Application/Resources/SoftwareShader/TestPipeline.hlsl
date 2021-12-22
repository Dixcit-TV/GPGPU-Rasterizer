#define FLT_EPSILON 1.19209290E-07F
#define VIEWPORT_WIDTH 1920.f
#define VIEWPORT_HEIGHT 1080.f
#define PI 3.14159265358979323846f
#define GROUP_DIMs 32, 32, 1

cbuffer ViewportInfo
{
	float VI_Width;
	float VI_Height;
}

cbuffer DispatchInfo
{
	uint3 DI_Dimensions;
}

cbuffer ObjectMatrices
{
	float4x4 worldViewProj;
	float4x4 world;
}

cbuffer LightInfo
{
	float3 lightDir;
	float lightIntensity;
}

struct Vertex_In
{
	float3 position;
	float3 normal;
};

struct Vertex_Out
{
	float4 position;
	float3 normal;
};

StructuredBuffer<Vertex_In> g_Vertices;
StructuredBuffer<uint> g_Indices;

RWTexture2D<float4> g_RenderTarget;
RWTexture2D<float> g_DepthBuffer;

inline uint FlatenGlobalThreadId(uint groupIndex, uint3 groupId, uint3 groupDimensions, uint3 dispatchDimensions)
{
	return groupIndex + (groupId.z * dispatchDimensions.x * dispatchDimensions.y + groupId.y * dispatchDimensions.x + groupId.x) * groupDimensions.x * groupDimensions.y * groupDimensions.z;
}

Vertex_Out TransformVertexToNDC(Vertex_In vertex);
bool IsClipped(float4 vertex);
float4 NdcVertexToScreen(float4 ndcVertex, float viewportWidth, float viewportHeight);
uint4 GetAabb(float2 v0, float2 v1, float2 v2, float viewportWidth, float viewportHeight);
bool RasterizeTriangle(float2 v0, float2 v1, float2 v2, uint2 pixel, out float3 weights);


float cross2d(float2 a, float2 b);
 
[numthreads(GROUP_DIMs)]
void main(uint groupIndex : SV_GroupIndex, uint3 dispatchID : SV_GroupId )
{
	const uint globalThreadId = FlatenGlobalThreadId(groupIndex, dispatchID, uint3(GROUP_DIMs), uint3(48, 48, 1) /*DI_Dimensions*/);
	Vertex_Out v0 = TransformVertexToNDC(g_Vertices[g_Indices[globalThreadId * 3]]);
	Vertex_Out v1 = TransformVertexToNDC(g_Vertices[g_Indices[globalThreadId * 3 + 1]]);
	Vertex_Out v2 = TransformVertexToNDC(g_Vertices[g_Indices[globalThreadId * 3 + 2]]);

	if (IsClipped(v0.position) || IsClipped(v1.position) || IsClipped(v2.position))
		return;

	v0.position = NdcVertexToScreen(v0.position, VIEWPORT_WIDTH, VIEWPORT_HEIGHT);
	v1.position = NdcVertexToScreen(v1.position, VIEWPORT_WIDTH, VIEWPORT_HEIGHT);
	v2.position = NdcVertexToScreen(v2.position, VIEWPORT_WIDTH, VIEWPORT_HEIGHT);

	uint4 aabb = GetAabb(v0.position.xy, v1.position.xy, v2.position.xy, VIEWPORT_WIDTH, VIEWPORT_HEIGHT);

	for (uint x = aabb.x; x <= aabb.z; ++x)
	{
		for (uint y = aabb.y; y <= aabb.w; ++y)
		{
			float3 weights;
			if (RasterizeTriangle(v0.position.xy, v1.position.xy, v2.position.xy, uint2(x, y), weights))
			{
				const float z = 1.f / (1.f / v0.position.z * weights.x + 1.f / v1.position.z * weights.y + 1.f / v2.position.z * weights.z);
				const float w = 1 / (v0.position.w * weights.x + v1.position.w * weights.y + v2.position.w * weights.z);
				//SO BAD
				if (g_DepthBuffer[uint2(x, y)] > z)
				{
					g_DepthBuffer[uint2(x, y)] = z;

					float3 normal = (v0.normal * v0.position.w * weights.x + v1.normal * v1.position.w * weights.y + v2.normal * v2.position.w * weights.z) * w;
					normal = normalize(normal);
					float diffuseStrength = saturate(dot(normal, -lightDir)) * lightIntensity;
					diffuseStrength /= PI;
					g_RenderTarget[uint2(x, y)] = float4(0.5f, 0.5f, 0.5f, 1.0f) * diffuseStrength;
				}
			}
		}
	}
}

Vertex_Out TransformVertexToNDC(Vertex_In vertex)
{
	Vertex_Out transformedVert;
	transformedVert.position = mul(worldViewProj, float4(vertex.position, 1.f));
	transformedVert.position.x /= transformedVert.position.w;
	transformedVert.position.y /= transformedVert.position.w;
	transformedVert.position.z /= transformedVert.position.w;
	transformedVert.position.w = 1.f / transformedVert.position.w;

	transformedVert.normal = mul((float3x3)world, vertex.normal);
	return transformedVert;
}

bool IsClipped(float4 vertex)
{
	return vertex.x < -1.f || vertex.y < -1.f
		|| vertex.x > 1.f || vertex.y > 1.f
		|| vertex.z < 0.f || vertex.z > 1.f;
}

float4 NdcVertexToScreen(float4 ndcVertex, float viewportWidth, float viewportHeight)
{
	ndcVertex.x = viewportWidth * (ndcVertex.x + 1.f) / 2.f;
	ndcVertex.y = viewportHeight * (1.f - ndcVertex.y) / 2.f;

	return ndcVertex;
}

uint4 GetAabb(float2 v0, float2 v1, float2 v2, float viewportWidth, float viewportHeight)
{
	uint4 aabb;
	aabb.x = (uint)max(min(v0.x, min(v1.x, v2.x)) - 1.f, 0.f);
	aabb.y = (uint)max(min(v0.y, min(v1.y, v2.y)) - 1.f, 0.f);
	aabb.z = (uint)min(max(v0.x, max(v1.x, v2.x)) + 1.f, viewportWidth);
	aabb.w = (uint)min(max(v0.y, max(v1.y, v2.y)) + 1.f, viewportHeight);

	return aabb;
}

bool RasterizeTriangle(float2 v0, float2 v1, float2 v2, uint2 pixel, out float3 weights)
{
	bool ret = false;
	weights = 0;
	const float2 edge_v1v2 = v2 - v1;
	const float2 edge_v2v0 = v0 - v2;

	float tempW0 = cross2d(edge_v1v2, pixel - v1);
	float tempW1 = cross2d(edge_v2v0, pixel - v2);

	if (tempW0 >= 0.f && tempW1 >= 0.f)
	{
		const float invTriArea = 1 / cross2d(edge_v2v0, -edge_v1v2);

		tempW0 = tempW0 * invTriArea;
		if (tempW0 >= 0.f && tempW0 <= 1.f)
		{
			tempW1 = tempW1 * invTriArea;
			if (tempW1 >= 0.f && tempW0 + tempW1 <= 1.f)
			{
				weights.x = tempW0;
				weights.y = tempW1;
				weights.z = 1 - (tempW0 + tempW1);
				ret = true;
			}
		}
	}

	return ret;
}

float cross2d(float2 a, float2 b)
{
	return a.x * b.y - a.y * b.x;
}