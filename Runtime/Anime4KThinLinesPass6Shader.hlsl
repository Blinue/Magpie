// Anime4K-v3.1-ThinLines-Kernel(Y)
// ÒÆÖ²×Ô https://github.com/bloc97/Anime4K/blob/master/glsl/Experimental-Effects/Anime4K_ThinLines_HQ.glsl



cbuffer constants : register(b0) {
	int2 srcSize : packoffset(c0);
};


#define MAGPIE_INPUT_COUNT 1
#include "Anime4K.hlsli"


D2D_PS_ENTRY(main) {
	InitMagpieSampleInput();

	float2 t1 = uncompressTan(SampleInputOffChecked(0, float2(0, -1)).xy);
	float2 t2 = uncompressTan(SampleInputOffChecked(0, float2(0, 1)).xy);

	//[tl  t tr]
	//[ l cc  r]
	//[bl  b br]
	float tx = t1.x;
	float cx = uncompressTan(SampleInputCur(0).x);
	float bx = t2.x;


	float ty = t1.y;
	//float cy = LUMAD_tex(HOOKED_pos).y;
	float by = t2.y;


	//Horizontal Gradient
	//[-1  0  1]
	//[-2  0  2]
	//[-1  0  1]
	float xgrad = (tx + cx + cx + bx) / 8.0;

	//Vertical Gradient
	//[-1 -2 -1]
	//[ 0  0  0]
	//[ 1  2  1]
	float ygrad = (-ty + by) / 8.0;

	float2 dn = { xgrad, ygrad };
	//Computes the luminance's gradient
	//Quasi-normalization for large vectors, avoids divide by zero
	return float4(compressLinear(dn / (length(dn) + 0.01), -5, 5), 0, 1);
}
