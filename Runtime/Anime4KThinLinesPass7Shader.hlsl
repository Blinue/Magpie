// Anime4K-v3.1-ThinLines
// ÒÆÖ²×Ô https://github.com/bloc97/Anime4K/blob/master/glsl/Experimental-Effects/Anime4K_ThinLines_HQ.glsl


cbuffer constants : register(b0) {
	int2 srcSize : packoffset(c0.x);
	float strength : packoffset(c0.z);
};


#define MAGPIE_INPUT_COUNT 2
#include "Anime4K.hlsli"


#define ITERATIONS 1 //Number of iterations for the forwards solver, decreasing strength and increasing iterations improves quality at the cost of speed.


D2D_PS_ENTRY(main) {
	InitMagpieSampleInput();

	float relstr = srcSize.y / 1080.0 * strength;

	float2 pos = Coord(0).xy;

#if ITERATIONS > 1
	for (int i = 0; i < ITERATIONS; i++) {
#endif

	float2 dn = uncompressLinear(SampleInput(1, pos / Coord(0).zw * Coord(1).zw).xy, -5, 5);
	float2 dd = dn * Coord(0).zw * relstr; 
	pos = GetCheckedPos(0, pos - dd);

#if ITERATIONS > 1
	}
#endif

	return SampleInput(0, pos);
}
