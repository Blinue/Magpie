// Anime4K-v3.1-DarkLines-Kernel(X)
// ÒÆÖ²×Ô https://github.com/bloc97/Anime4K/blob/master/glsl/Experimental-Effects/Anime4K_DarkLines_HQ.glsl



cbuffer constants : register(b0) {
	int2 srcSize : packoffset(c0);
};


#define MAGPIE_INPUT_COUNT 1
#include "Anime4K.hlsli"

#define SIGMA 1.0

#define get(pos) (uncompressLinear(SampleInputChecked(0, pos).x, -2, 0))


float gaussian(float x, float s) {
	float t = x / s;
	return exp(-0.5 * t * t) / s / 2.506628274631;
}

float lumGaussian(float2 pos, float2 d) {
	float sigma = SIGMA * srcSize.y / 1080.0;
	float kernelSize = sigma * 2.0 + 1.0;

	float gn = 1 / (sigma * 2.506628274631);
	float g = get(pos) * gn;

	float t = gaussian(1.0, sigma);
	g += (get(pos - d) + get(pos + d)) * t;
	gn += t * 2.0;

	for (int i = 2; float(i) < kernelSize; i++) {
		t = gaussian(float(i), sigma);
		g += (get(pos - d * i) + get(pos + d * i))* t;
		gn += t * 2.0;
	}

	return g / gn;
}


D2D_PS_ENTRY(main) {
	InitMagpieSampleInput();

	float g = lumGaussian(Coord(0).xy, float2(Coord(0).z, 0));
	return float4(compressLinear(g, -2, 0), 0, 0, 1);
}
