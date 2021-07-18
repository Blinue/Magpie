// ACNet_L1_2
// ÒÆÖ²×Ô https://github.com/TianZerL/ACNetGLSL/blob/master/glsl/ACNet.glsl


cbuffer constants : register(b0) {
	int2 srcSize : packoffset(c0);
};


#define MAGPIE_INPUT_COUNT 1
#include "ACNet.hlsli"


const static float kernelsL1[9 * 4] = {
	 0.2054, -0.0393,  0.1494,
	 0.3106,  0.5722,  0.2640,
	 0.1708, -0.1640, -0.0212,
	 0.0558, -0.2887, -0.1666,
	 0.3123, -0.3097, -0.2281,
	 0.2880,  0.3001,  0.0526,
	-0.0320,  0.0584, -0.0193,
	-0.0135,  1.0649, -0.1246,
	 0.0283, -0.3030, -0.6378,
	-0.0040, -0.9122,  0.0181,
	 0.0365,  0.8947, -0.0420,
	-0.0199,  0.0217,  0.0060
};

const static float4 biasL1 = { 0.0223,  0.0340,  0.0150, -0.0044 };


D2D_PS_ENTRY(main) {
	InitMagpieSampleInput();

	float2 leftTop = max(0, Coord(0).xy - Coord(0).zw);
	float2 rightBottom = min(maxCoord0.xy, Coord(0).xy + Coord(0).zw);

	// [tl, tc, tr]
	// [ml, mc, mr]
	// [bl, bc, br]
	float tl = SampleInput(0, leftTop).x;
	float ml = SampleInput(0, float2(leftTop.x, Coord(0).y)).x;
	float bl = SampleInput(0, float2(leftTop.x, rightBottom.y)).x;
	float tc = SampleInput(0, float2(Coord(0).x, leftTop.y)).x;
	float mc = SampleInputCur(0).x;
	float bc = SampleInput(0, float2(Coord(0).x, rightBottom.y)).x;
	float tr = SampleInput(0, float2(rightBottom.x, leftTop.y)).x;
	float mr = SampleInput(0, float2(rightBottom.x, Coord(0).y)).x;
	float br = SampleInput(0, rightBottom).x;

	float4 c5678 = RELU(float4(
		tl * kernelsL1[0 * 9 + 0] + tc * kernelsL1[0 * 9 + 1] + tr * kernelsL1[0 * 9 + 2] +
		ml * kernelsL1[0 * 9 + 3] + mc * kernelsL1[0 * 9 + 4] + mr * kernelsL1[0 * 9 + 5] +
		bl * kernelsL1[0 * 9 + 6] + bc * kernelsL1[0 * 9 + 7] + br * kernelsL1[0 * 9 + 8] + biasL1.x,

		tl * kernelsL1[1 * 9 + 0] + tc * kernelsL1[1 * 9 + 1] + tr * kernelsL1[1 * 9 + 2] +
		ml * kernelsL1[1 * 9 + 3] + mc * kernelsL1[1 * 9 + 4] + mr * kernelsL1[1 * 9 + 5] +
		bl * kernelsL1[1 * 9 + 6] + bc * kernelsL1[1 * 9 + 7] + br * kernelsL1[1 * 9 + 8] + biasL1.y,

		tl * kernelsL1[2 * 9 + 0] + tc * kernelsL1[2 * 9 + 1] + tr * kernelsL1[2 * 9 + 2] +
		ml * kernelsL1[2 * 9 + 3] + mc * kernelsL1[2 * 9 + 4] + mr * kernelsL1[2 * 9 + 5] +
		bl * kernelsL1[2 * 9 + 6] + bc * kernelsL1[2 * 9 + 7] + br * kernelsL1[2 * 9 + 8] + biasL1.z,

		tl * kernelsL1[3 * 9 + 0] + tc * kernelsL1[3 * 9 + 1] + tr * kernelsL1[3 * 9 + 2] +
		ml * kernelsL1[3 * 9 + 3] + mc * kernelsL1[3 * 9 + 4] + mr * kernelsL1[3 * 9 + 5] +
		bl * kernelsL1[3 * 9 + 6] + bc * kernelsL1[3 * 9 + 7] + br * kernelsL1[3 * 9 + 8] + biasL1.w
		));

	return compressLinear(c5678, 0, 2);
}
