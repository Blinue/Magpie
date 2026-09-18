cbuffer vertexBuffer : register(b0) {
	float4x4 projectionMatrix;
};

struct PSInput {
	noperspective float2 uv : TEXCOORD;
	float4 color : COLOR;
	noperspective float4 position : SV_POSITION;
};

PSInput main(
	float2 position : POSITION,
	float2 uv : TEXCOORD,
	float4 color : COLOR
) {
	PSInput output;
	output.uv = uv;
	// TODO
	output.position = mul(projectionMatrix, float4(position, 0, 1));;
	output.color = color;
	return output;
}
