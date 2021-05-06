// Denoise °æ±¾µÄ Conv-4x3x3x1 (0)
// ÒÆÖ²×Ô https://github.com/bloc97/Anime4K/blob/master/glsl/Upscale%2BDenoise/Anime4K_Upscale_CNN_M_x2_Denoise.glsl
//
// Anime4K-v3.1-Upscale(x2)+Denoise-CNN(M)-Conv-4x3x3x1


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
	float a = SampleInputNoCheck(0, float2(left1X, top1Y)).x;
	float b = SampleInputNoCheck(0, float2(left1X, coord.y)).x;
	float c = SampleInputNoCheck(0, float2(left1X, bottom1Y)).x;
	float d = SampleInputNoCheck(0, float2(coord.x, top1Y)).x;
	float e = SampleInputCur(0).x;
	float f = SampleInputNoCheck(0, float2(coord.x, bottom1Y)).x;
	float g = SampleInputNoCheck(0, float2(right1X, top1Y)).x;
	float h = SampleInputNoCheck(0, float2(right1X, coord.y)).x;
	float i = SampleInputNoCheck(0, float2(right1X, bottom1Y)).x;

	float s = -0.096334584 * a + 0.4884673 * b + -0.03821645 * c + 0.025493674 * d + 0.6071791 * e + -0.97234 * f + 0.099867396 * g + -0.20816612 * h + 0.08195157 * i;
	float o = s + -0.008229233;
	s = 0.0356221 * a + -0.8889586 * b + -0.041130573 * c + 0.15342546 * d + 0.8915476 * e + -0.12391313 * f + -0.069834635 * g + -0.04377315 * h + 0.080039404 * i;
	float p = s + -0.023339543;
	s = 0.47696692 * a + -1.1180642 * b + 0.09315012 * c + 0.6659025 * d + -0.06723025 * e + 0.020558799 * f + -0.03196199 * g + -0.003144155 * h + -0.005075667 * i;
	float q = s + -0.045330495;
	s = -0.035183977 * a + 0.00825989 * b + -0.07498109 * c + 0.109649256 * d + 0.5719336 * e + -0.1938904 * f + -0.137681 * g + -0.1617649 * h + -0.032986585 * i;
	float r = s + -0.019567212;

	return Compress(float4(o, p, q, r));
}
