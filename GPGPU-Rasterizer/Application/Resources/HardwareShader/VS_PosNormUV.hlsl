struct Vertex_IN
{
	float3 position : POSITION;
	//float3 normal : NORMAL;
	//float3 uv : TEXCOORD;
};

float4 main(Vertex_IN input ) : SV_POSITION
{
	return float4(input.position, 1.f);
}