#define D2D_INPUT_COUNT 1
#define D2D_INPUT0_COMPLEX
#include "Anime4K.hlsli"

/*
* Anime4K-v3.1-Upscale(x2)+Deblur-CNN(M)-Kernel(X)
*/


cbuffer constants : register(b0) {
	int2 srcSize : packoffset(c0);
};


D2D_PS_ENTRY(main) {
	float4 coord = D2DGetInputCoordinate(0);

	float a = GetYOfYUV(
		D2DSampleInput(0, float2(max(0, coord.x - coord.z), coord.y)).rgb
	);
	float b = GetYOfYUV(D2DSampleInput(0, coord.xy).rgb);
	float c = GetYOfYUV(
		D2DSampleInput(0, float2(min((srcSize.x - 1) * coord.z, coord.x + coord.z), coord.y)).rgb
	);

	return float4(min3(a, b, c), max3(a, b, c), 0, 0);
}
