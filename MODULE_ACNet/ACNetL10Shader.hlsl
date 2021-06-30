// ACNet_L10
// 移植自 https://github.com/TianZerL/ACNetGLSL/blob/master/glsl/ACNet.glsl


#define MAGPIE_INPUT_COUNT 3
#define MAGPIE_USE_YUV
#define MAGPIE_NO_CHECK
#include "ACNet.hlsli"

#define noise_threshold 0.02


const static float kernelsL10[4 * 8] = {
	 0.4908, -0.0457,
	-0.1716, -0.2115,
	-0.0015, -0.3152,
	 0.3045,  0.0330,
	-0.2981,  0.0912,
	 0.0122,  0.2281,
	 0.3331,  0.2853,
	 0.2210,  0.2611,
	 0.2364,  0.0792,
	 0.2885, -0.7122,
	-0.3715,  0.1404,
	-0.0260,  0.2144,
	 0.2378,  0.1570,
	-0.5734,  0.2077,
	-0.0851,  0.2771,
	 0.0415, -0.1858
};

const static float4 biasL = { -0.0006,  0.0117,  0.0083,  0.0686 };


D2D_PS_ENTRY(main) {
	InitMagpieSampleInput();
	Coord(0).xy /= 2;
	Coord(1).xy /= 2;
	Coord(2).xy /= 2;

	float2 f = frac(Coord(1).xy / Coord(1).zw);
	int2 i = int2(f * 2);
	int index = i.y * 2 + i.x;
	float2 pos = Coord(1).xy + (float2(0.5, 0.5) - f) * Coord(1).zw;

	float4 mc1 = uncompressLinear(SampleInput(1, pos), 0, 4.5);
	float4 mc2 = uncompressLinear(SampleInput(2, pos), 0, 3);

	float luma = clamp(
		mc1.x * kernelsL10[0 + index] +
		mc1.y * kernelsL10[4 + index] +
		mc1.z * kernelsL10[8 + index] +
		mc1.w * kernelsL10[12 + index] +
		mc2.x * kernelsL10[16 + index] +
		mc2.y * kernelsL10[20 + index] +
		mc2.z * kernelsL10[24 + index] +
		mc2.w * kernelsL10[28 + index], 0.0f, 1.0f);

	float3 yuv = SampleInputCur(0).xyz;
	// 消除因压缩产生的噪声
	if (abs(luma - yuv.x) < noise_threshold) {
		luma = yuv.x;
	}

	return float4(YUV2RGB(float3(luma, yuv.yz)), 1);
}
