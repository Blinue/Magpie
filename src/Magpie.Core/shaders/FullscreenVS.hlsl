struct PSInput {
	noperspective float2 uv : TEXCOORD;
	noperspective float4 position : SV_POSITION;
};

PSInput main(uint vID : SV_VertexID) {
	PSInput result;
	result.uv = float2((vID << 1) & 2, vID & 2);
	result.position = float4(result.uv * float2(2, -2) + float2(-1, 1), 0, 1);
	return result;
}
