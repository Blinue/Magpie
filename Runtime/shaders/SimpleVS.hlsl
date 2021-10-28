void main(
	float4 pos : SV_POSITION,
	float2 coord : TEXCOORD,
	out float4 posOut : SV_POSITION,
	out float2 coordOut : TEXCOORD
) {
	posOut = pos;
	coordOut = coord;
}
