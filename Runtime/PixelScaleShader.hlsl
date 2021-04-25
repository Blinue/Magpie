// Pixel 插值算法


cbuffer constants : register(b0) {
	int2 srcSize : packoffset(c0.x);
	int scale : packoffset(c0.z);
};


#define D2D_INPUT_COUNT 1
#define D2D_INPUT0_COMPLEX
#define MAGPIE_USE_SAMPLE_INPUT
#include "common.hlsli"


D2D_PS_ENTRY(main) {
	InitMagpieSampleInputWithScale(float2(scale, scale));

	// 不加上 0.5 会使画面模糊
	return SampleInputRGBANoCheck(0, (int2(coord.xy / coord.zw) + 0.5) * coord.zw);
}
