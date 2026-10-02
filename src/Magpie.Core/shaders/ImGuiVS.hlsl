cbuffer vertexBuffer : register(b0) {
	float2 scale;
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
	// 从屏幕空间转换到裁剪空间，scale 的值为 (2 / displaySize.x, -2 / displaySize.y)
	output.position = float4(position * scale + float2(-1, 1), 0, 1);
	output.color = color;
	return output;
}
