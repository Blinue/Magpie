// Conv-4x3x3x1 (0)
// ÒÆÖ²×Ô https://github.com/bloc97/Anime4K/blob/master/glsl/Upscale/Anime4K_Upscale_CNN_M_x2.glsl
//
// Anime4K-v3.1-Upscale(x2)-CNN(M)-Conv-4x3x3x1


cbuffer constants : register(b0) {
	int2 srcSize : packoffset(c0);
};


#define D2D_INPUT_COUNT 1
#define D2D_INPUT0_COMPLEX
#define MAGPIE_USE_SAMPLE_INPUT
#include "Anime4K.hlsli"


D2D_PS_ENTRY(main) {
	InitMagpieSampleInput();

	float left1X = GetCheckedLeft(1);
	float right1X = GetCheckedRight(1);
	float top1Y = GetCheckedTop(1);
	float bottom1Y = GetCheckedBottom(1);

	// [ a, d, g ]
	// [ b, e, h ]
	// [ c, f, i ]
	float a = GetLuma(SampleInputNoCheck(0, float2(left1X, top1Y)));
	float b = GetLuma(SampleInputNoCheck(0, float2(left1X, coord.y)));
	float c = GetLuma(SampleInputNoCheck(0, float2(left1X, bottom1Y)));
	float d = GetLuma(SampleInputNoCheck(0, float2(coord.x, top1Y)));
	float e = GetLuma(SampleInputCur(0));
	float f = GetLuma(SampleInputNoCheck(0, float2(coord.x, bottom1Y)));
	float g = GetLuma(SampleInputNoCheck(0, float2(right1X, top1Y)));
	float h = GetLuma(SampleInputNoCheck(0, float2(right1X, coord.y)));
	float i = GetLuma(SampleInputNoCheck(0, float2(right1X, bottom1Y)));

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
