#define D2D_INPUT_COUNT 1
#define D2D_INPUT0_COMPLEX
#include "d2d1effecthelpers.hlsli"
#include "Anime4K.hlsli"

/*
* Anime4K-v3.1-Upscale(x2)-CNN(M)-Conv-4x3x3x1
*/

cbuffer constants : register(b0) {
    int2 srcSize : packoffset(c0);
};


D2D_PS_ENTRY(main) {
	float4 coord = D2DGetInputCoordinate(0);

	float leftPos = max(0, coord.x - coord.z);
	float topPos = max(0, coord.y - coord.w);
	float rightPos = min((srcSize.x - 1) * coord.z, coord.x + coord.z);
	float bottomPos = min((srcSize.y - 1) * coord.w, coord.y + coord.w);

	float a = GetYOfYUV(D2DSampleInput(0, float2(leftPos, topPos)).rgb);
	float b = GetYOfYUV(D2DSampleInput(0, float2(leftPos, coord.y)).rgb);
	float c = GetYOfYUV(D2DSampleInput(0, float2(leftPos, bottomPos)).rgb);
	float d = GetYOfYUV(D2DSampleInput(0, float2(coord.x, topPos)).rgb);
	float e = GetYOfYUV(D2DSampleInput(0, coord.xy).rgb);
	float f = GetYOfYUV(D2DSampleInput(0, float2(coord.x, bottomPos)).rgb);
	float g = GetYOfYUV(D2DSampleInput(0, float2(rightPos, topPos)).rgb);
	float h = GetYOfYUV(D2DSampleInput(0, float2(rightPos, coord.y)).rgb);
	float i = GetYOfYUV(D2DSampleInput(0, float2(rightPos, bottomPos)).rgb);

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
