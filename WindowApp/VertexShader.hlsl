struct Output
{
	float4 Color    : COLOR;
	float4 Position : SV_Position;
};

struct Rotation
{
	matrix transform;
};

ConstantBuffer<Rotation> rot : register(b0);

Output main(float3 pos : POSITION, float3 color : COLOR)
{
	Output vertexOut;

	vertexOut.Position = mul(float4(pos, 1.0f), rot.transform);
	vertexOut.Color = float4(color, 1.0f);

	return vertexOut;
}