

cbuffer constants : register(b0) {
	int2 srcSize : packoffset(c0);
};


#define D2D_INPUT_COUNT 3
#define D2D_INPUT0_COMPLEX
#define D2D_INPUT1_COMPLEX
#define D2D_INPUT2_COMPLEX
#define MAGPIE_USE_YUV
#include "Anime4K.hlsli"

#define STRENGTH 1 //De-blur proportional strength, higher is sharper. However, it is better to tweak BLUR_CURVE instead to avoid ringing.
#define BLUR_CURVE 0.6 //De-blur power curve, lower is sharper. Good values are between 0.3 - 1. Values greater than 1 softens the image;
#define BLUR_THRESHOLD 0.1 //Value where curve kicks in, used to not de-blur already sharp edges. Only de-blur values that fall below this threshold.
#define NOISE_THRESHOLD 0.001 //Value where curve stops, used to not sharpen noise. Only de-blur values that fall above this threshold.


D2D_PS_ENTRY(main) {
	float4 coord = D2DGetInputCoordinate(0);
	float2 srcPos = coord.xy / 2;

	float2 f = frac(round(coord.xy / coord.zw) / 2);
	int2 i = f * 2;
	float c0 = Uncompress(D2DSampleInput(1, srcPos + (float2(0.5, 0.5) - f) * coord.zw))[i.y * 2 + i.x];

	float c = c0 * STRENGTH;

	float3 yuv = RGB2YUV(D2DSampleInput(0, srcPos).rgb);
	float2 mm = D2DSampleInput(2, srcPos).xy;

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
	
	return float4(YUV2RGB(yuv.x, yuv.y, yuv.z).xyz, 1);
}
