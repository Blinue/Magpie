void main(uint id : SV_VERTEXID, out float4 pos : SV_POSITION, out float2 coord : TEXCOORD) {
	coord = float2(id & 1, id >> 1) * 2.0;
	pos = float4(coord * float2(2.0, -2.0) + float2(-1.0, 1.0), 0.0, 1.0);
}
