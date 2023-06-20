struct Output
{
	float2 uv : TEXCOORD;
	float4 Position : SV_Position;
};

struct Rotation
{
	matrix transform;
};
ConstantBuffer<Rotation> rot : register(b0);

Output main(float3 pos : POSITION, float2 uv : TEXCOORD)
{
	Output vertexOut;

	vertexOut.Position = mul(float4(pos, 1.0f), rot.transform);
	vertexOut.uv = uv;

	return vertexOut;
}