#define D2D_INPUT_COUNT 2
#define D2D_INPUT0_COMPLEX
#define D2D_INPUT1_COMPLEX
#include "d2d1effecthelpers.hlsli"
#include "Anime4K.hlsli"


cbuffer constants : register(b0) {
	int2 srcSize : packoffset(c0);
};


D2D_PS_ENTRY(main) {
	float4 coord = D2DGetInputCoordinate(0);
	float2 srcPos = coord.xy / 2;

	float2 f = frac(round(coord.xy / coord.zw) / 2);
	int2 i = f * 2;
	float c = Uncompress(D2DSampleInput(1, srcPos + (float2(0.5, 0.5) - f) * coord.zw))[i.y * 2 + i.x];

	float3 yuv = RGB2YUV(D2DSampleInput(0, srcPos).rgb);

	yuv += c;

	return float4(YUV2RGB(yuv).xyz, 1);
}
