#define D2D_INPUT_COUNT 1
#define D2D_INPUT0_COMPLEX
#include "d2d1effecthelpers.hlsli"
#include "Anime4K.hlsli"

/*
* Anime4K-v3.1-Upscale(x2)+Deblur-CNN(M)-Kernel(X)
*/


cbuffer constants : register(b0) {
	int2 srcSize : packoffset(c0);
};


D2D_PS_ENTRY(main) {
	float4 coord = D2DGetInputCoordinate(0);

	float2 a = D2DSampleInput(0, float2(coord.x, max(0, coord.y - coord.w))).xy;
	float2 b = D2DSampleInput(0, coord.xy).xy;
	float2 c = D2DSampleInput(0, float2(coord.x, min((srcSize.y - 1) * coord.w, coord.y + coord.w))).xy;

	return float4(min3(a.x, b.x, c.x), max3(a.y, b.y, c.y), 0, 0);
}
