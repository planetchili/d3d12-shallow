struct Output
{
	float4 Color    : COLOR;
	float4 Position : SV_Position;
};

Output main(float3 pos : POSITION, float3 color : COLOR)
{
	Output vertexOut;

	vertexOut.Position = float4(pos, 1.0f);
	vertexOut.Color = float4(color, 1.0f);

	return vertexOut;
}