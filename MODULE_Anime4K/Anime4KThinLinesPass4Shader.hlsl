// Anime4K-v3.1-ThinLines-Kernel(Y)
// ÒÆÖ²×Ô https://github.com/bloc97/Anime4K/blob/master/glsl/Experimental-Effects/Anime4K_ThinLines_HQ.glsl



cbuffer constants : register(b0) {
	int2 srcSize : packoffset(c0);
};


#define MAGPIE_INPUT_COUNT 1
#include "common.hlsli"


#define KERNELSIZE (sigma * 2.0 + 1.0)

#define get(pos) (uncompressLinear(SampleInputChecked(0, pos).x, 0, 3))


float gaussian(float x, float s) {
	float t = x / s;
	return exp(-0.5 * t * t) / s / 2.506628274631;
}

float lumGaussian(float2 pos, float2 d) {
	float sigma = (srcSize.y / 1080.0) * 2.0;
	float kernelSize = sigma * 2.0 + 1.0;

	float g = get(pos) / (sigma * 2.506628274631);
	g += (get(pos - d) + get(pos + d)) * gaussian(1.0, sigma);
	for (int i = 2; float(i) < kernelSize; i++) {
		g += (get(pos - d * i) + get(pos + d * i)) * gaussian(i, sigma);
	}

	return g;
}


D2D_PS_ENTRY(main) {
	InitMagpieSampleInput();

	float g = lumGaussian(Coord(0).xy, float2(0, Coord(0).w));
	return float4(compressLinear(g, 0, 3), 0, 0, 1);
}
