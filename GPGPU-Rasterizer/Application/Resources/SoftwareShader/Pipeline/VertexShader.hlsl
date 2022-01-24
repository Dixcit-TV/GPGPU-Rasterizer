#include "../Libs/Common.hlsli"

#define VIEWPORT_WIDTH 1920.f
#define VIEWPORT_HEIGHT 1080.f

#define GROUP_X 32
#define GROUP_Y 16
#define GROUP_DIMs GROUP_X, GROUP_Y, 1
#define UINT3_GROUP_DIMs uint3(GROUP_DIMs)
#define THREAD_COUNT (GROUP_X * GROUP_Y)

struct Vertex_In
{
	float3 position;
	float pad1;
	float3 normal;
	float pad2;
};

struct Vertex_Out
{
	float4 position;
	float3 normal;
	float pad;
};

cbuffer ObjectInfo : register(b0)
{
	float4x4 worldViewProj;
	float4x4 world;
	uint vertexCount;
	uint triangleCount;
	uint indexCount;
}

StructuredBuffer<Vertex_In> G_VERTEX_BUFFER;
RWStructuredBuffer<Vertex_Out> G_TRANS_VERTEX_BUFFER : register(u2);

Vertex_Out Transform(Vertex_In v)
{
	Vertex_Out vOut = (Vertex_Out)0;
	vOut.position = mul(worldViewProj, float4(v.position, 1.f));
	vOut.normal = mul((float3x3)world, v.normal);
	return vOut;
}

void ProjectionToNDC(inout float4 vertex);
void NDCToScreen(inout float4 vertex, float viewportWidth, float viewportHeight);

[numthreads(GROUP_DIMs)]
void main(uint groupIndex : SV_GroupIndex, uint3 dispatchID : SV_GroupId)
{
	const uint numGroup = ceil(vertexCount / (float)THREAD_COUNT);
	const uint globalThreadId = FlattenedGlobalThreadId(groupIndex, dispatchID, UINT3_GROUP_DIMs, uint3(numGroup, 1, 1) /*G_DISPATCH_DIMS*/);

	if (globalThreadId >= vertexCount)
		return;

	Vertex_In v = G_VERTEX_BUFFER[globalThreadId];

	Vertex_Out vOut = Transform(v);

	ProjectionToNDC(vOut.position);
	NDCToScreen(vOut.position, VIEWPORT_WIDTH, VIEWPORT_HEIGHT);

	G_TRANS_VERTEX_BUFFER[globalThreadId] = vOut;
}

void ProjectionToNDC(inout float4 vPos)
{
	const float invW = 1.f / vPos.w;
	vPos *= invW * float2(1.f, invW).xxxy;
}

void NDCToScreen(inout float4 vPos, float viewportWidth, float viewportHeight)
{
	vPos = (vPos + float3(1.f, -1.f, 0.f).xyzz) * float3(viewportWidth, -viewportHeight, 1.f).xyzz * float2(0.5f, 1.f).xxyy;
}