// (FSRCNNX_x2_8-0-4-1) aggregation


#define MAGPIE_INPUT_COUNT 2
#define MAGPIE_USE_YUV
#define MAGPIE_NO_CHECK
#include "common.hlsli"

#define noise_threshold 0.02


D2D_PS_ENTRY(main) {
	InitMagpieSampleInput();
	Coord(0).xy /= 2;
	Coord(1).xy /= 2;

	float2 f = frac(Coord(1).xy / Coord(1).zw);
	int2 i = int2(f * 2);
	int index = i.x * 2 + i.y;
	float2 pos = Coord(1).xy + (float2(0.5, 0.5) - f) * Coord(1).zw;

	float luma = SampleInput(1, pos)[index];
	float3 yuv = SampleInputCur(0).xyz;

	// 消除因压缩产生的噪声
	if (abs(luma - yuv.x) < noise_threshold) {
		luma = yuv.x;
	}

	return float4(YUV2RGB(float3(luma, yuv.yz)), 1);
}
