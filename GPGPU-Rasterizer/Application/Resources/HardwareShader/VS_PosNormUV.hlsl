cbuffer ObjectMatrices
{
	float4x4 worldViewProj;
	float4x4 world;
}

struct Vertex_IN
{
	float3 position : POSITION;
	float3 normal : NORMAL;
};

struct Vertex_OUT
{
	float4 position : SV_POSITION;
	float3 normal : NORMAL;
};

Vertex_OUT main(Vertex_IN input)
{
	Vertex_OUT output = (Vertex_OUT)0;

	output.position = mul(worldViewProj, float4(input.position, 1.f));
	output.normal = mul((float3x3)world, input.normal);
	return output;
}