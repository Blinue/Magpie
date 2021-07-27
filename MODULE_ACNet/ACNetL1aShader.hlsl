// ACNet_L1_1
// 移植自 https://github.com/TianZerL/ACNetGLSL/blob/master/glsl/ACNet.glsl


cbuffer constants : register(b0) {
	int2 srcSize : packoffset(c0);
};


#define MAGPIE_INPUT_COUNT 1
#include "ACNet.hlsli"


const static float kernelsL1[9 * 4] = {
	 0.0609,  0.1027, -0.0447,
	-0.1423,  0.7196,  0.1803,
	 0.0842,  0.0696,  0.0082,
	 0.0089,  0.1540, -0.8589,
	 0.0448,  0.8659, -0.2420,
	-0.0364,  0.0585,  0.0125,
	-0.1937,  0.7259,  0.0119,
	-0.8266,  0.4147,  0.0088,
	-0.0453, -0.0451, -0.0182,
	 0.0264, -0.9422,  0.1258,
	-0.0543,  0.1282,  0.7102,
	-0.0106,  0.0386, -0.0141
};

const static float4 biasL1 = {-0.7577, -0.0210, 0.0292, -0.0189};


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

	float4 c1234 = RELU(float4(
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

	return compressLinear(c1234, 0, 2);
}
