// Anime4K-v3.1-ThinLines-Kernel(X)
// ÒÆÖ²×Ô https://github.com/bloc97/Anime4K/blob/master/glsl/Experimental-Effects/Anime4K_ThinLines_HQ.glsl



cbuffer constants : register(b0) {
	int2 srcSize : packoffset(c0);
};


#define D2D_INPUT_COUNT 1
#define D2D_INPUT0_COMPLEX
#define MAGPIE_USE_SAMPLE_INPUT
#include "Anime4K.hlsli"


D2D_PS_ENTRY(main) {
	InitMagpieSampleInput();

	//[tl  t tr]
	//[ l  c  r]
	//[bl  b br]
	float l = SampleInputOffCheckLeft(0, -1, 0).x;
	float c = SampleInputCur(0).x;
	float r = SampleInputOffCheckRight(0, 1, 0).x;


	//Horizontal Gradient
	//[-1  0  1]
	//[-2  0  2]
	//[-1  0  1]
	float xgrad = (-l + r);

	//Vertical Gradient
	//[-1 -2 -1]
	//[ 0  0  0]
	//[ 1  2  1]
	float ygrad = (l + c + c + r);

	//Computes the luminance's gradient
	return float4(Compress2(xgrad), Compress2(ygrad), 0, 1);
}
