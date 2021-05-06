// Anime4K-v3.1-ThinLines
// ÒÆÖ²×Ô https://github.com/bloc97/Anime4K/blob/master/glsl/Experimental-Effects/Anime4K_ThinLines_HQ.glsl


cbuffer constants : register(b0) {
	int2 srcSize : packoffset(c0);
};


#define MAGPIE_INPUT_COUNT 2
#include "Anime4K.hlsli"

#define STRENGTH 0.3 //Strength of warping for each iteration
#define ITERATIONS 3 //Number of iterations for the forwards solver, decreasing strength and increasing iterations improves quality at the cost of speed.


D2D_PS_ENTRY(main) {
	InitMagpieSampleInput();

	float relstr = srcSize.y / 1080.0 * STRENGTH;

	float2 pos = Coord(0).xy;

#if ITERATIONS != 1
	for (int i = 0; i < ITERATIONS; i++) {
#endif

	float2 dn = Uncompress2(SampleInput(1, pos / Coord(0).zw * Coord(1).zw).xy);
	float2 dd = (dn / (length(dn) + 0.01)) * Coord(0).zw * relstr; //Quasi-normalization for large vectors, avoids divide by zero
	pos = GetCheckedPos(0, pos);

#if ITERATIONS != 1
	}
#endif

	return SampleInput(0, pos);
}
