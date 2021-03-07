// 普通和 Denoise 版本的 combine 着色器
// 移植自 https://github.com/bloc97/Anime4K/blob/master/glsl/Upscale/Anime4K_Upscale_CNN_M_x2.glsl


cbuffer constants : register(b0) {
	int2 srcSize : packoffset(c0);
};


#define D2D_INPUT_COUNT 2
#define D2D_INPUT0_COMPLEX
#define D2D_INPUT1_COMPLEX
#define MAGPIE_USE_SAMPLE_INPUT
#define MAGPIE_USE_YUV
#include "Anime4K.hlsli"


D2D_PS_ENTRY(main) {
	InitMagpieSampleInputWithScale(float2(2, 2));

	float2 f = frac(coord.xy / coord.zw);
	int2 i = round(f * 2);
	float l = Uncompress(SampleInputRGBAOffNoCheck(1, (float2(0.5, 0.5) - f)))[i.y * 2 + i.x];

	float3 yuv = RGB2YUV(SampleInputCur(0));
	float3 color = YUV2RGB(yuv.x + l, yuv.y, yuv.z);

	
	float left1X = GetCheckedLeft(1);
	float right1X = GetCheckedRight(1);
	float top1Y = GetCheckedTop(1);
	float bottom1Y = GetCheckedBottom(1);

	float3 c0 = SampleInputNoCheck(0, float2(left1X, top1Y));
	float3 c1 = SampleInputNoCheck(0, float2(left1X, coord.y));
	float3 c2 = SampleInputNoCheck(0, float2(left1X, bottom1Y));
	float3 c3 = SampleInputNoCheck(0, float2(coord.x, top1Y));
	float3 c4 = SampleInputCur(0);
	float3 c5 = SampleInputNoCheck(0, float2(coord.x, bottom1Y));
	float3 c6 = SampleInputNoCheck(0, float2(right1X, top1Y));
	float3 c7 = SampleInputNoCheck(0, float2(right1X, coord.y));
	float3 c8 = SampleInputNoCheck(0, float2(right1X, bottom1Y));

	float3 min_sample = min9(c0, c1, c2, c3, c4, c5, c6, c7, c8);
	float3 max_sample = max9(c0, c1, c2, c3, c4, c5, c6, c7, c8);
	color = lerp(color, clamp(color, min_sample, max_sample), 0.5);

	return float4(color, 1);
}
