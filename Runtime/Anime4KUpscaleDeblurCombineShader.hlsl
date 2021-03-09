// Deblur 版本的 combine 着色器
// 移植自 https://github.com/bloc97/Anime4K/blob/master/glsl/Upscale%2BDeblur/Anime4K_Upscale_CNN_M_x2_Deblur.glsl


cbuffer constants : register(b0) {
	int2 srcSize : packoffset(c0);
};


#define D2D_INPUT_COUNT 3
#define D2D_INPUT0_COMPLEX
#define D2D_INPUT1_COMPLEX
#define D2D_INPUT2_COMPLEX
#define MAGPIE_USE_YUV
#define MAGPIE_USE_SAMPLE_INPUT
#include "Anime4K.hlsli"


#define STRENGTH 1 //De-blur proportional strength, higher is sharper. However, it is better to tweak BLUR_CURVE instead to avoid ringing.
#define BLUR_CURVE 0.6 //De-blur power curve, lower is sharper. Good values are between 0.3 - 1. Values greater than 1 softens the image;
#define BLUR_THRESHOLD 0.1 //Value where curve kicks in, used to not de-blur already sharp edges. Only de-blur values that fall below this threshold.
#define NOISE_THRESHOLD 0.001 //Value where curve stops, used to not sharpen noise. Only de-blur values that fall above this threshold.



D2D_PS_ENTRY(main) {
	InitMagpieSampleInputWithScale(float2(2, 2));

	float2 f = frac(coord.xy / coord.zw);
	int2 i = int2(f * 2);
	float c0 = Uncompress(SampleInputRGBAOffNoCheck(1, (float2(0.5, 0.5) - f)))[i.y * 2 + i.x];

	float c = c0 * STRENGTH;

	float3 yuv = RGB2YUV(SampleInputCur(0));
	float2 mm = SampleInputRGBACur(2).xy;

	float c_t = abs(c);
	if (c_t > NOISE_THRESHOLD && c_t < BLUR_THRESHOLD) {
		float t_range = BLUR_THRESHOLD - NOISE_THRESHOLD;

		c_t = (c_t - NOISE_THRESHOLD) / t_range;
		c_t = pow(abs(c_t), BLUR_CURVE);
		c_t = c_t * t_range + NOISE_THRESHOLD;
		c_t = c_t * sign(c);

		yuv.x = clamp(c_t + yuv.x, mm.x, mm.y);
	} else {
		yuv.x += c;
	}
	yuv.x += c;
	return float4(YUV2RGB(yuv.x, yuv.y, yuv.z).xyz, 1);
}
