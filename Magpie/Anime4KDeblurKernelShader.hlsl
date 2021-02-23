/*
* Anime4K-v3.1-Upscale(x2)+Deblur-CNN(M)-Kernel(X/Y)
*/

cbuffer constants : register(b0) {
	int2 srcSize : packoffset(c0);
};

#define D2D_INPUT_COUNT 1
#define D2D_INPUT0_COMPLEX
#define MAGPIE_USE_SAMPLE_INPUT
#include "Anime4K.hlsli"

#define max9(a, b, c, d, e, f, g, h, i) max3(max4(a, b, c, d), max4(e, f, g, h), i)
#define min9(a, b, c, d, e, f, g, h, i) min3(min4(a, b, c, d), min4(e, f, g, h), i)


D2D_PS_ENTRY(main) {
	InitMagpieSampleInput();

	float left1X = GetCheckedLeft(1);
	float right1X = GetCheckedRight(1);
	float top1Y = GetCheckedTop(1);
	float bottom1Y = GetCheckedBottom(1);

	// [ a, b, c ]
	// [ d, e, f ]
	// [ g, h, i ]
	float a = GetYOfYUV(SampleInputNoCheck(0, float2(left1X, top1Y)));
	float b = GetYOfYUV(SampleInputNoCheck(0, float2(coord.x, top1Y)));
	float c = GetYOfYUV(SampleInputNoCheck(0, float2(right1X, top1Y)));
	float d = GetYOfYUV(SampleInputNoCheck(0, float2(left1X, coord.y)));
	float e = GetYOfYUV(SampleInputCur(0));
	float f = GetYOfYUV(SampleInputNoCheck(0, float2(right1X, coord.y)));
	float g = GetYOfYUV(SampleInputNoCheck(0, float2(left1X, bottom1Y)));
	float h = GetYOfYUV(SampleInputNoCheck(0, float2(coord.x, bottom1Y)));
	float i = GetYOfYUV(SampleInputNoCheck(0, float2(right1X, bottom1Y)));

	return float4(min9(a, b, c, d, e, f, g, h, i), max9(a, b, c, d, e, f, g, h, i), 0, 0);
}
