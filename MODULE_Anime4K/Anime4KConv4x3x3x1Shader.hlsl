// Conv-4x3x3x1 (0)
// ÒÆÖ²×Ô https://github.com/bloc97/Anime4K/blob/master/glsl/Upscale/Anime4K_Upscale_CNN_M_x2.glsl
//
// Anime4K-v3.1-Upscale(x2)-CNN(M)-Conv-4x3x3x1


cbuffer constants : register(b0) {
	int2 srcSize : packoffset(c0);
};


#define MAGPIE_INPUT_COUNT 1
#include "Anime4K.hlsli"


D2D_PS_ENTRY(main) {
	InitMagpieSampleInput();

	float2 leftTop = max(0, Coord(0).xy - Coord(0).zw);
	float2 rightBottom = min(maxCoord0.xy, Coord(0).xy + Coord(0).zw);

	// [ a, d, g ]
	// [ b, e, h ]
	// [ c, f, i ]
	float a = SampleInput(0, leftTop).x;
	float b = SampleInput(0, float2(leftTop.x, Coord(0).y)).x;
	float c = SampleInput(0, float2(leftTop.x, rightBottom.y)).x;
	float d = SampleInput(0, float2(Coord(0).x, leftTop.y)).x;
	float e = SampleInputCur(0).x;
	float f = SampleInput(0, float2(Coord(0).x, rightBottom.y)).x;
	float g = SampleInput(0, float2(rightBottom.x, leftTop.y)).x;
	float h = SampleInput(0, float2(rightBottom.x, Coord(0).y)).x;
	float i = SampleInput(0, rightBottom).x;

	float s = -0.09440448 * a + 0.49120164 * b + -0.022703001 * c + -0.016553257 * d + 0.6272513 * e + -0.97632706 * f + 0.10815585 * g + -0.21898738 * h + 0.09604159 * i;
	float o = s + 0.00028890301;
	s = 0.061990097 * a + -0.87003845 * b + -0.037461795 * c + 0.13172528 * d + 0.87585527 * e + -0.13609451 * f + -0.070119604 * g + -0.051131595 * h + 0.09209152 * i;
	float p = s + -0.017290013;
	s = 0.45264956 * a + -1.1240269 * b + 0.07975403 * c + 0.6734861 * d + -0.05388544 * e + 0.007570164 * f + -0.06987841 * g + 0.012247365 * h + 0.034949988 * i;
	float q = s + -0.0145500265;
	s = -0.035659406 * a + 0.043313805 * b + -0.056556296 * c + 0.08745333 * d + 0.6312519 * e + -0.24501355 * f + -0.13407958 * g + -0.18634492 * h + -0.08149098 * i;
	float r = s + -0.009025143;
	
	return Compress(float4(o,p,q,r));
}
