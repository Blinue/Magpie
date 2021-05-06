// Anime4K-v3.1-DarkLines-Kernel(Y)
// ÒÆÖ²×Ô https://github.com/bloc97/Anime4K/blob/master/glsl/Experimental-Effects/Anime4K_DarkLines_HQ.glsl



cbuffer constants : register(b0) {
	int2 srcSize : packoffset(c0);
};


#define D2D_INPUT_COUNT 2
#define D2D_INPUT0_COMPLEX
#define D2D_INPUT1_COMPLEX
#include "Anime4K.hlsli"


#define SIGMA 1.0

static float4 coord0;
static float4 coord1;
static float2 maxCoord1;

float get(float2 pos) {
	pos = clamp(pos, float2(0, 0), maxCoord1);
	return Uncompress2(D2DSampleInput(1, pos).x);
}

float gaussian(float x, float s, float m) {
	return (1.0 / (s * sqrt(2.0 * 3.14159))) * exp(-0.5 * pow(abs(x - m) / s, 2.0));
}

float lumGaussian(float2 pos, float2 d) {
	float s = SIGMA * srcSize.y / 1080.0;
	float kernel_size = s * 2.0 + 1.0;

	float g = get(pos) * gaussian(0.0, s, 0.0);
	float gn = gaussian(0.0, s, 0.0);

	g += (get(max(pos - d, float2(0, 0))) + get(min(pos + d, maxCoord1))) * gaussian(1.0, s, 0.0);
	gn += gaussian(1.0, s, 0.0) * 2.0;

	for (int i = 2; float(i) < kernel_size; i++) {
		g += (get(max(pos - d * i, float2(0,0))) + get(min(pos + d * i, maxCoord1))) * gaussian(float(i), s, 0.0);
		gn += gaussian(float(i), s, 0.0) * 2.0;
	}

	return g / gn;
}


D2D_PS_ENTRY(main) {
	coord0 = D2DGetInputCoordinate(0);
	coord1 = D2DGetInputCoordinate(1);
	maxCoord1 = float2((srcSize.x - 1) * coord1.z, (srcSize.y - 1) * coord1.w);

	return float4(Compress2(min(D2DSampleInput(0, coord0.xy).x - lumGaussian(coord1.xy, float2(0, coord1.w)), 0.0)), 0, 0, 1);
}
