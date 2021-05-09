// Anime4K-v3.1-ThinLines-Kernel(X)
// ÒÆÖ²×Ô https://github.com/bloc97/Anime4K/blob/master/glsl/Experimental-Effects/Anime4K_ThinLines_HQ.glsl



cbuffer constants : register(b0) {
	int2 srcSize : packoffset(c0);
};


#define MAGPIE_INPUT_COUNT 1
#include "Anime4K.hlsli"

#define KERNELSIZE (sigma * 2.0 + 1.0)

#define get(pos) (SampleInputChecked(0, pos).x * 2)


float gaussian(float x, float s) {
	float a = 1.0 / (s * 2.506628274631);
	float t = x / s;

	return a * exp(-0.5 * t * t);
}

float lumGaussian(float2 pos, float2 d, float sigma) {
	float g = get(pos) / (sigma * 2.506628274631);
	g += (get(pos - d) + get(pos + d)) * gaussian(1.0, sigma);
	for (int i = 2; float(i) < KERNELSIZE; i++) {
		g += (get(pos - (d * float(i))) + get(pos + (d * float(i)))) * gaussian(float(i), sigma);
	}

	return g;
}


D2D_PS_ENTRY(main) {
	InitMagpieSampleInput();

	float sigma = (srcSize.y / 1080.0) * 2.0;
	return float4(Compress2(lumGaussian(Coord(0).xy, float2(Coord(0).z, 0), sigma)), 0, 0, 1);
}
