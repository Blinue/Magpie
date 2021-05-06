// Anime4K-v3.1-DarkLines-Kernel(Y)
// ÒÆÖ²×Ô https://github.com/bloc97/Anime4K/blob/master/glsl/Experimental-Effects/Anime4K_DarkLines_HQ.glsl



cbuffer constants : register(b0) {
	int2 srcSize : packoffset(c0);
};


#define MAGPIE_INPUT_COUNT 1
#include "Anime4K.hlsli"


#define SIGMA 1.0

#define get(pos) (Uncompress2(SampleInputChecked(0, pos).x))

float gaussian(float x, float s, float m) {
	return (1.0 / (s * sqrt(2.0 * 3.14159))) * exp(-0.5 * pow(abs(x - m) / s, 2.0));
}

float lumGaussian(float2 pos, float2 d) {
	float s = SIGMA * srcSize.y / 1080.0;
	float kernel_size = s * 2.0 + 1.0;

	float g = get(pos) * gaussian(0.0, s, 0.0);
	float gn = gaussian(0.0, s, 0.0);

	g += (get(pos - d) + get(pos + d)) * gaussian(1.0, s, 0.0);
	gn += gaussian(1.0, s, 0.0) * 2.0;

	for (int i = 2; float(i) < kernel_size; i++) {
		g += (get(pos - (d * float(i))) + get(pos + (d * float(i)))) * gaussian(float(i), s, 0.0);
		gn += gaussian(float(i), s, 0.0) * 2.0;
	}

	return g / gn;
}


D2D_PS_ENTRY(main) {
	InitMagpieSampleInput();

	return float4(-lumGaussian(Coord(0).xy, float2(0, Coord(0).w)), 0, 0, 1);
}
