#define FLT_EPSILON 1.19209290E-07F
#define VIEWPORT_WIDTH 1920.f
#define VIEWPORT_HEIGHT 1080.f
#define PI 3.14159265358979323846f
#define GROUP_DIMs 32, 32, 1
#define UINT3_GROUP_DIMs uint3(GROUP_DIMs)

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
	float pad;
	float3 normal;
	float pad1;
};

struct Vertex_Out
{
	float4 position;
	float3 normal;
	float pad;
};

RWTexture2D<float4> g_RenderTarget : register(u0);
RWTexture2D<uint> g_DepthBuffer : register(u1);

StructuredBuffer<Vertex_In> g_Vertices;
StructuredBuffer<uint> g_Indices;

inline float cross2d(float2 a, float2 b)
{
	return a.x * b.y - a.y * b.x;
}

inline uint FlatenGlobalThreadId(uint groupIndex, uint3 groupId, uint3 groupDimensions, uint3 dispatchDimensions)
{
	return groupIndex + (groupId.z * dispatchDimensions.x * dispatchDimensions.y + groupId.y * dispatchDimensions.x + groupId.x) * groupDimensions.x * groupDimensions.y * groupDimensions.z;
}

Vertex_Out TransformVertexToNDC(Vertex_In vertex);
bool IsClipped(float4 vertex);
float4 NdcVertexToScreen(float4 ndcVertex, float viewportWidth, float viewportHeight);
uint4 GetAabb(float2 v0, float2 v1, float2 v2, float viewportWidth, float viewportHeight);
 
[numthreads(GROUP_DIMs)]
void main(uint groupIndex : SV_GroupIndex, uint3 dispatchID : SV_GroupId )
{
	const uint globalThreadId = FlatenGlobalThreadId(groupIndex, dispatchID, uint3(GROUP_DIMs), uint3(64, 1, 1) /*DI_Dimensions*/);
	Vertex_Out v0 = TransformVertexToNDC(g_Vertices[g_Indices[globalThreadId * 3]]);
	Vertex_Out v1 = TransformVertexToNDC(g_Vertices[g_Indices[globalThreadId * 3 + 1]]);
	Vertex_Out v2 = TransformVertexToNDC(g_Vertices[g_Indices[globalThreadId * 3 + 2]]);

	if (IsClipped(v0.position) || IsClipped(v1.position) || IsClipped(v2.position))
		return;

	v0.position = NdcVertexToScreen(v0.position, VIEWPORT_WIDTH, VIEWPORT_HEIGHT);
	v1.position = NdcVertexToScreen(v1.position, VIEWPORT_WIDTH, VIEWPORT_HEIGHT);
	v2.position = NdcVertexToScreen(v2.position, VIEWPORT_WIDTH, VIEWPORT_HEIGHT);

	uint4 aabb = GetAabb(v0.position.xy, v1.position.xy, v2.position.xy, VIEWPORT_WIDTH, VIEWPORT_HEIGHT);

	uint2 aabb2 = uint2((aabb.x << 16) | (0x0000ffff & aabb.y), (aabb.z << 16) | (0x0000ffff & aabb.w));
	aabb = uint4(aabb2.x >> 16, aabb2.x & 0x0000ffff, aabb2.y >> 16, aabb2.y & 0x0000ffff);

	const float invTriArea = 1 / cross(v0.position.xyz - v2.position.xyz, v1.position.xyz - v2.position.xyz).z;
	for (uint x = aabb.x; x <= aabb.z; ++x)
	{
		for (uint y = aabb.y; y <= aabb.w; ++y)
		{
			const uint2 pixel = uint2(x, y);
			float3 weights = float3(cross2d(v2.position.xy - v1.position.xy, pixel - v1.position.xy)
				, cross2d(v0.position.xy - v2.position.xy, pixel - v2.position.xy)
				, cross2d(v1.position.xy - v0.position.xy, pixel - v0.position.xy)) * invTriArea;
			if (weights.x >= 0.f && weights.y >= 0.f && weights.z >= 0.f)
			{
				const float z = 1.f / dot(1.f / float3(v0.position.z, v1.position.z, v2.position.z), weights);
				const float w = 1 / dot(float3(v0.position.w, v1.position.w, v2.position.w), weights);

				const uint uintZ = asuint(z);
				uint prevZ;
				InterlockedMin(g_DepthBuffer[uint2(x, y)], asuint(z), prevZ);
				if (uintZ < prevZ)
				{
					float3 normal = (v0.normal * v0.position.w * weights.x + v1.normal * v1.position.w * weights.y + v2.normal * v2.position.w * weights.z) * w;
					normal = normalize(normal);
					float diffuseStrength = saturate(dot(normal, -lightDir)) * lightIntensity;
					diffuseStrength /= PI;
					diffuseStrength = 1.f;

					g_RenderTarget[uint2(x, y)] = float4(float3(0.5f, 0.5f, 0.5f) * diffuseStrength, 1.f);
				}
			}
		}
	}
}

Vertex_Out TransformVertexToNDC(Vertex_In vertex)
{
	Vertex_Out transformedVert = (Vertex_Out)0;
	transformedVert.position = mul(worldViewProj, float4(vertex.position, 1.f));
	const float invW = 1.f / transformedVert.position.w;
	transformedVert.position *= invW * float2(1.f, invW).xxxy;
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
	return (ndcVertex + float3(1.f, -1.f, 0.f).xyzz) * float3(viewportWidth, -viewportHeight, 1.f).xyzz * float2(0.5f, 1.f).xxyy;
}

uint4 GetAabb(float2 v0, float2 v1, float2 v2, float viewportWidth, float viewportHeight)
{
	float4 aabb;
	aabb.xy = min(v0.xy, min(v1.xy, v2.xy));
	aabb.zw = ceil(max(v0.xy, max(v1.xy, v2.xy)));
	return aabb;
}