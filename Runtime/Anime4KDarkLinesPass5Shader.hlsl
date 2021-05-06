// Anime4K-v3.1-DarkLines-Kernel(Y)
// ÒÆÖ²×Ô https://github.com/bloc97/Anime4K/blob/master/glsl/Experimental-Effects/Anime4K_DarkLines_HQ.glsl



cbuffer constants : register(b0) {
	int2 srcSize : packoffset(c0);
};


#define MAGPIE_INPUT_COUNT 2
#define MAGPIE_USE_YUV
#include "Anime4K.hlsli"

#define STRENGTH 0.5 //Line darken proportional strength, higher is darker.


D2D_PS_ENTRY(main) {
	InitMagpieSampleInput();


	float c = -SampleInputCur(1).x * STRENGTH;
	if (c > -0.01) {
		c = 0;
	}

	float3 yuv = SampleInputCur(0).xyz;
	return float4(YUV2RGB(max(0, c + yuv.x), yuv.y, yuv.z), 1);
}
