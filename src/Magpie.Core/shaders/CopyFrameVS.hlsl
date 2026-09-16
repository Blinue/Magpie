cbuffer RootConstants : register(b0) {
	float2 scale;
	float2 offset;
};

struct PSInput {
	noperspective float2 uv : TEXCOORD;
	noperspective float4 position : SV_POSITION;
};

PSInput main(uint vID : SV_VertexID) {
	PSInput result;
	// 全屏三角形 (0,0), (2,0), (0,2)
	float2 rawUV = float2((vID << 1) & 2, vID & 2);
	// 将输出区域 uv 变换为 0~1
	result.uv = rawUV * scale + offset;
	// 裁剪空间 (-1,1), (3,1), (-1,-3)
	result.position = float4(rawUV * float2(2, -2) + float2(-1, 1), 0, 1);
	return result;
}
