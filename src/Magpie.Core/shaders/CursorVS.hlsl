cbuffer RootConstants : register(b0) {
	float2 pos;
	float2 size;
};

struct PSInput {
	noperspective float2 uv : TEXCOORD;
	noperspective float4 position : SV_POSITION;
};

PSInput main(uint vID : SV_VertexID) {
	PSInput result;
	// (0,0)->(1,0)->(0,1)->(1,1)
	result.uv = float2(vID & 1, (vID >> 1) & 1);
	result.position = float4(result.uv * size + pos, 0, 1);
	return result;
}
