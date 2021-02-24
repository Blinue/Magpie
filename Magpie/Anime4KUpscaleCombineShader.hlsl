// 非 Deblur 版本的 combine 着色器
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
	InitMagpieSampleInput();
	coord.xy /= 2;	// 将 dest 坐标映射为 src 坐标

	float2 f = frac(coord.xy / coord.zw);
	int2 i = round(f * 2);
	float c = Uncompress(SampleInputRGBAOffNoCheck(1, (float2(0.5, 0.5) - f)))[i.y * 2 + i.x];

	float3 yuv = RGB2YUV(SampleInputCur(0));
	return float4(YUV2RGB(yuv.x + c, yuv.y, yuv.z).rgb, 1);
}
