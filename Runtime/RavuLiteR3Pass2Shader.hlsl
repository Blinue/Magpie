// ravu-lite-r3 Pass2
// ÒÆÖ²×Ô https://github.com/bjin/mpv-prescalers/blob/master/ravu-lite-r3.hook


cbuffer constants : register(b0) {
	int2 srcSize : packoffset(c0);
};


#define MAGPIE_INPUT_COUNT 2
#define MAGPIE_NO_CHECK
#define MAGPIE_USE_YUV
#include "common.hlsli"


D2D_PS_ENTRY(main) {
	Coord(0).xy /= 2;
	Coord(1).xy /= 2;

	float2 dir = frac(Coord(1).xy / Coord(1).zw) - 0.5;
	int idx = int(dir.x > 0.0) * 2 + int(dir.y > 0.0);

	float l = SampleInputOff(1, -dir)[idx];
	float3 yuv = SampleInputCur(0).xyz;
	return float4(YUV2RGB(float3(l, yuv.yz)), 1);
}
