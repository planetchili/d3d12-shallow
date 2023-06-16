struct CubeColors
{
	float4 colors[6];
};
ConstantBuffer<CubeColors> cubeColors : register(b0);

float4 main(uint primitiveID : SV_PrimitiveID) : SV_TARGET
{
	return cubeColors.colors[primitiveID >> 1];
}