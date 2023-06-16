struct Output
{
	float4 Position : SV_Position;
};

struct Rotation
{
	matrix transform;
};
ConstantBuffer<Rotation> rot : register(b0);

Output main(float3 pos : POSITION)
{
	Output vertexOut;

	vertexOut.Position = mul(float4(pos, 1.0f), rot.transform);

	return vertexOut;
}