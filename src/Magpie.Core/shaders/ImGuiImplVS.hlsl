cbuffer vertexBuffer : register(b0) {
	float4x4 projectionMatrix;
};

void main(
	float4 pos : SV_POSITION,
	float2 coord : TEXCOORD,
	float4 color : COLOR,
	out float2 outCoord : TEXCOORD,
	out float4 outColor : COLOR,
	out float4 outPos : SV_POSITION
) {
	outPos = mul(projectionMatrix, float4(pos.xy, 0.f, 1.f));
	outCoord = coord;
	outColor = color;
}
