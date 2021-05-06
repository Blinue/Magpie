// Anime4K-v3.1-ThinLines
// ÒÆÖ²×Ô https://github.com/bloc97/Anime4K/blob/master/glsl/Experimental-Effects/Anime4K_ThinLines_HQ.glsl


cbuffer constants : register(b0) {
	int2 srcSize : packoffset(c0);
};


#define D2D_INPUT_COUNT 2
#define D2D_INPUT0_COMPLEX
#define D2D_INPUT1_COMPLEX
#include "Anime4K.hlsli"

#define STRENGTH 0.3 //Strength of warping for each iteration
#define ITERATIONS 1 //Number of iterations for the forwards solver, decreasing strength and increasing iterations improves quality at the cost of speed.


D2D_PS_ENTRY(main) {
	float4 coord0 = D2DGetInputCoordinate(0);
	float4 coord1 = D2DGetInputCoordinate(1);
	float2 maxCoord0 = float2((srcSize.x - 1) * coord0.z, (srcSize.y - 1) * coord0.w);

	float relstr = srcSize.y / 1080.0 * STRENGTH;

	float2 pos = coord0.xy;

#if ITERATIONS != 1
	for (int i = 0; i < ITERATIONS; i++) {
#endif

	float2 dn = Uncompress2(D2DSampleInput(1, pos / coord0.zw * coord1.zw).xy);
	float2 dd = (dn / (length(dn) + 0.01)) * coord0.zw * relstr; //Quasi-normalization for large vectors, avoids divide by zero
	pos = clamp(pos - dd, float2(0, 0), maxCoord0);

#if ITERATIONS != 1
	}
#endif

	return D2DSampleInput(0, pos);
}
