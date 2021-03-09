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
	// 截取整数部分，如果使用 round 会有 bug，因为在特殊情况下 f 可能等于 0.75
	int2 i = int2(f * 2);
	float l = Uncompress(SampleInputRGBAOffNoCheck(1, (float2(0.5, 0.5) - f)))[i.y * 2 + i.x];

	float3 yuv = RGB2YUV(SampleInputCur(0));
	return float4(YUV2RGB(yuv.x+l, yuv.y, yuv.z), 1);
}
