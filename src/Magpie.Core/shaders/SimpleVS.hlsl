void main(
	float4 pos : SV_POSITION,
	float2 coord : TEXCOORD,
	out float2 outCoord : TEXCOORD,
	out float4 outPos : SV_POSITION
) {
	outPos = pos;
	outCoord = coord;
}
