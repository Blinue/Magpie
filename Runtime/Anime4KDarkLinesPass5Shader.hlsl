// Anime4K-v3.1-DarkLines-Kernel(Y)
// ÒÆÖ²×Ô https://github.com/bloc97/Anime4K/blob/master/glsl/Experimental-Effects/Anime4K_DarkLines_HQ.glsl



cbuffer constants : register(b0) {
	int2 srcSize : packoffset(c0);
};


#define D2D_INPUT_COUNT 2
#define D2D_INPUT0_COMPLEX
#define D2D_INPUT1_COMPLEX
#define MAGPIE_USE_YUV
#include "Anime4K.hlsli"

#define STRENGTH 1 //Line darken proportional strength, higher is darker.


D2D_PS_ENTRY(main) {
	float4 coord0 = D2DGetInputCoordinate(0);
	float4 coord1 = D2DGetInputCoordinate(1);

	float c = -D2DSampleInput(1, coord1.xy).x * STRENGTH;

	float3 yuv = D2DSampleInput(0, coord0.xy).xyz;
	if (abs(c) < 0.005) {
		c = 0;
	}

	return float4(YUV2RGB(clamp(c + yuv.x, 0.0, yuv.x), yuv.y, yuv.z), 1);
}
