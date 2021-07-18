// ACNet_L7_1
// ÒÆÖ²×Ô https://github.com/TianZerL/ACNetGLSL/blob/master/glsl/ACNet.glsl


cbuffer constants : register(b0) {
	int2 srcSize : packoffset(c0);
};


#define MAGPIE_INPUT_COUNT 2
#include "ACNet.hlsli"


const static float kernelsL[9 * 8 * 4] = {
	 2.5042e-02, -5.3266e-02,  3.8484e-02,
	 3.7189e-03,  1.0493e-01,  1.4459e-01,
	-3.7442e-02, -1.5744e-01,  1.9957e-01,
	-1.9203e-02,  1.6256e-02,  4.2906e-03,
	-3.1637e-02,  5.0287e-01, -6.9504e-02,
	 1.4677e-03, -8.9984e-02, -9.0376e-02,
	 4.0578e-02,  2.4004e-02,  3.4044e-03,
	 7.5916e-02, -1.3564e-01, -9.0296e-02,
	 3.4156e-02,  7.2494e-02, -2.0037e-02,
	-6.4614e-02, -1.7301e-03, -3.3444e-02,
	-2.7950e-01,  7.1351e-01,  4.2825e-02,
	 2.4797e-02,  5.4162e-04, -8.9676e-02,
	 3.8002e-02, -2.7692e-02, -1.7757e-02,
	 1.9356e-01,  1.9598e-02, -1.0862e-01,
	 2.5734e-02,  1.1703e-02, -7.3912e-02,
	-6.0213e-04,  1.6024e-01, -6.4591e-03,
	 3.1779e-02, -3.1049e-01,  1.2684e-02,
	-1.0098e-01, -1.8839e-01,  5.1387e-02,
	 5.2004e-02,  3.1489e-01,  5.9716e-01,
	-7.2238e-02,  3.4332e-01, -2.0655e-01,
	 1.1013e-03, -5.0328e-02, -4.6118e-02,
	 9.4442e-04,  2.7964e-02,  1.7672e-02,
	-8.6022e-02, -3.8280e-02,  2.8017e-04,
	 3.3824e-02, -6.7883e-02,  1.0529e-02,
	-6.5982e-02,  1.1385e-01,  3.0091e-03,
	 1.2330e-01,  6.1876e-01,  5.7145e-02,
	-4.3835e-02, -6.8186e-01, -1.0917e-01,
	 3.2006e-02, -2.0627e-03, -6.9043e-02,
	 7.2219e-02, -3.2393e-01, -2.6657e-02,
	 1.3523e-02,  1.8099e-01,  4.9168e-02,
	 7.1367e-02,  9.8283e-02,  1.0425e-01,
	 2.2286e-01, -5.9374e-01,  1.0014e-01,
	 6.5700e-02,  1.3618e-02, -7.4045e-02,
	 1.0481e-01,  3.0734e-02,  1.0431e-02,
	-2.1314e-01, -7.2817e-02,  1.2036e-01,
	-5.4180e-02,  1.0500e-01,  2.7821e-02,
	-5.0657e-02,  8.7702e-02,  7.0234e-02,
	 9.0349e-02,  1.4905e-01,  1.1612e-01,
	 5.9924e-02,  2.4928e-01,  1.7078e-01,
	-5.9110e-02, -7.4252e-02,  9.8241e-03,
	-1.2006e-01,  1.3879e-01, -1.4322e-02,
	-7.5463e-02,  1.4407e-02, -6.9202e-03,
	 7.0279e-02,  1.7065e-01, -2.5150e-01,
	-2.6289e-02,  3.8421e-01, -2.2051e-01,
	-2.8918e-02,  4.0074e-02, -7.1296e-02,
	 1.0357e-01, -1.8885e-01,  2.3780e-02,
	-1.8884e-01, -4.3326e-01, -1.1465e-01,
	 3.3497e-02, -1.3462e-01, -3.4127e-02,
	-1.2731e-02,  5.4326e-02, -2.6581e-02,
	 5.1753e-02,  6.8200e-03,  4.3246e-03,
	-6.9963e-02, -1.5618e-01,  2.5192e-01,
	 2.2890e-02,  6.1421e-02,  5.2832e-02,
	-9.8369e-02, -1.1452e-01,  1.7420e-01,
	 2.0392e-01, -1.1322e-01,  9.8462e-02,
	-3.3547e-02, -2.8993e-01,  7.0080e-02,
	 8.2478e-02, -1.9881e-01,  1.2849e-01,
	-2.7802e-01, -1.5621e-01,  6.2712e-02,
	 1.3028e-02,  1.4716e-01,  2.0434e-02,
	-4.4071e-01,  3.8359e-01, -1.6655e-03,
	-2.0297e-01,  1.5631e-01,  7.7086e-02,
	 9.6714e-03, -5.5842e-03,  7.9155e-03,
	 1.4525e-01, -3.2228e-01,  1.1454e-01,
	 1.4527e-01, -3.0399e-02, -6.7043e-02,
	 9.4233e-03, -1.1296e-02, -1.0927e-01,
	 7.9300e-02,  5.5286e-02, -1.1558e-01,
	 3.8173e-01, -5.4351e-02, -1.7890e-01,
	 5.4882e-02,  1.5119e-01,  1.8363e-01,
	-8.8223e-02, -9.0083e-02,  4.8221e-01,
	 4.0890e-02,  5.6429e-02, -2.8538e-01,
	 1.2102e-02, -1.8177e-02, -3.1643e-03,
	-6.9064e-02,  3.1853e-04, -7.0113e-02,
	 9.7308e-02,  1.0691e-01, -6.5919e-02,
	-1.4536e-40, -1.7049e-40, -2.6781e-40,
	 4.5792e-40,  1.4489e-40,  1.3645e-40,
	-5.8774e-40, -2.2505e-40, -4.7571e-40,
	 3.3670e-40,  1.5398e-40, -3.3819e-40,
	 2.6303e-40, -1.9434e-40, -5.5555e-40,
	-4.3830e-40, -2.8750e-40, -3.0788e-41,
	 5.6364e-40,  3.1307e-40, -2.3064e-41,
	 2.8909e-40, -5.8115e-40,  2.9852e-41,
	-1.9273e-40, -7.5503e-41, -6.0335e-40,
	 5.8073e-40,  2.9252e-40, -1.3038e-40,
	 5.2260e-40,  3.8172e-40, -2.0389e-40,
	-2.1905e-41,  1.8473e-40, -2.9226e-40,
	 2.9957e-41,  2.6068e-40,  6.1324e-40,
	-4.3013e-41,  5.1421e-40, -4.1157e-40,
	 2.1416e-41, -1.6614e-40, -3.0843e-42,
	-4.3402e-40,  2.8507e-40,  1.1560e-40,
	 3.8826e-40, -3.0797e-40, -6.0685e-40,
	 5.4170e-40, -6.1858e-40,  9.3049e-41,
	-1.9491e-40, -1.9211e-40, -6.2723e-40,
	 3.9906e-40,  1.2356e-40,  3.8682e-40,
	 2.8630e-40,  6.2303e-40,  5.3034e-40,
	-4.1904e-40,  4.8916e-40, -3.6125e-40,
	-5.5393e-40, -2.4980e-40, -6.1877e-40,
	 2.7289e-40, -1.8348e-40, -5.6663e-40
};

const static float4 biasL = { 8.7612e-02,  5.9126e-01,  4.6709e-03, -1.1559e-39 };


D2D_PS_ENTRY(main) {
	InitMagpieSampleInput();

	float2 leftTop1 = max(0, Coord(0).xy - Coord(0).zw);
	float2 rightBottom1 = min(maxCoord0.xy, Coord(0).xy + Coord(0).zw);

	// [tl, tc, tr]
	// [ml, mc, mr]
	// [bl, bc, br]
	float4 tl1 = uncompressLinear(SampleInput(0, leftTop1), 0, 2.5);
	float4 ml1 = uncompressLinear(SampleInput(0, float2(leftTop1.x, Coord(0).y)), 0, 2.5);
	float4 bl1 = uncompressLinear(SampleInput(0, float2(leftTop1.x, rightBottom1.y)), 0, 2.5);
	float4 tc1 = uncompressLinear(SampleInput(0, float2(Coord(0).x, leftTop1.y)), 0, 2.5);
	float4 mc1 = uncompressLinear(SampleInputCur(0), 0, 2.5);
	float4 bc1 = uncompressLinear(SampleInput(0, float2(Coord(0).x, rightBottom1.y)), 0, 2.5);
	float4 tr1 = uncompressLinear(SampleInput(0, float2(rightBottom1.x, leftTop1.y)), 0, 2.5);
	float4 mr1 = uncompressLinear(SampleInput(0, float2(rightBottom1.x, Coord(0).y)), 0, 2.5);
	float4 br1 = uncompressLinear(SampleInput(0, rightBottom1), 0, 2.5);

	float2 leftTop2 = max(0, Coord(1).xy - Coord(1).zw);
	float2 rightBottom2 = min(maxCoord1.xy, Coord(1).xy + Coord(1).zw);

	float4 tl2 = uncompressLinear(SampleInput(1, leftTop2), 0, 3);
	float4 ml2 = uncompressLinear(SampleInput(1, float2(leftTop2.x, Coord(1).y)), 0, 3);
	float4 bl2 = uncompressLinear(SampleInput(1, float2(leftTop2.x, rightBottom2.y)), 0, 3);
	float4 tc2 = uncompressLinear(SampleInput(1, float2(Coord(1).x, leftTop2.y)), 0, 3);
	float4 mc2 = uncompressLinear(SampleInputCur(1), 0, 3);
	float4 bc2 = uncompressLinear(SampleInput(1, float2(Coord(1).x, rightBottom2.y)), 0, 3);
	float4 tr2 = uncompressLinear(SampleInput(1, float2(rightBottom2.x, leftTop2.y)), 0, 3);
	float4 mr2 = uncompressLinear(SampleInput(1, float2(rightBottom2.x, Coord(1).y)), 0, 3);
	float4 br2 = uncompressLinear(SampleInput(1, rightBottom2), 0, 3);

	float4 c1234 = RELU(float4(
		tl1.x * kernelsL[0 * 72 + 0 * 9 + 0] + tc1.x * kernelsL[0 * 72 + 0 * 9 + 1] + tr1.x * kernelsL[0 * 72 + 0 * 9 + 2] +
		ml1.x * kernelsL[0 * 72 + 0 * 9 + 3] + mc1.x * kernelsL[0 * 72 + 0 * 9 + 4] + mr1.x * kernelsL[0 * 72 + 0 * 9 + 5] +
		bl1.x * kernelsL[0 * 72 + 0 * 9 + 6] + bc1.x * kernelsL[0 * 72 + 0 * 9 + 7] + br1.x * kernelsL[0 * 72 + 0 * 9 + 8] +

		tl1.y * kernelsL[0 * 72 + 1 * 9 + 0] + tc1.y * kernelsL[0 * 72 + 1 * 9 + 1] + tr1.y * kernelsL[0 * 72 + 1 * 9 + 2] +
		ml1.y * kernelsL[0 * 72 + 1 * 9 + 3] + mc1.y * kernelsL[0 * 72 + 1 * 9 + 4] + mr1.y * kernelsL[0 * 72 + 1 * 9 + 5] +
		bl1.y * kernelsL[0 * 72 + 1 * 9 + 6] + bc1.y * kernelsL[0 * 72 + 1 * 9 + 7] + br1.y * kernelsL[0 * 72 + 1 * 9 + 8] +

		tl1.z * kernelsL[0 * 72 + 2 * 9 + 0] + tc1.z * kernelsL[0 * 72 + 2 * 9 + 1] + tr1.z * kernelsL[0 * 72 + 2 * 9 + 2] +
		ml1.z * kernelsL[0 * 72 + 2 * 9 + 3] + mc1.z * kernelsL[0 * 72 + 2 * 9 + 4] + mr1.z * kernelsL[0 * 72 + 2 * 9 + 5] +
		bl1.z * kernelsL[0 * 72 + 2 * 9 + 6] + bc1.z * kernelsL[0 * 72 + 2 * 9 + 7] + br1.z * kernelsL[0 * 72 + 2 * 9 + 8] +

		tl1.w * kernelsL[0 * 72 + 3 * 9 + 0] + tc1.w * kernelsL[0 * 72 + 3 * 9 + 1] + tr1.w * kernelsL[0 * 72 + 3 * 9 + 2] +
		ml1.w * kernelsL[0 * 72 + 3 * 9 + 3] + mc1.w * kernelsL[0 * 72 + 3 * 9 + 4] + mr1.w * kernelsL[0 * 72 + 3 * 9 + 5] +
		bl1.w * kernelsL[0 * 72 + 3 * 9 + 6] + bc1.w * kernelsL[0 * 72 + 3 * 9 + 7] + br1.w * kernelsL[0 * 72 + 3 * 9 + 8] +

		tl2.x * kernelsL[0 * 72 + 4 * 9 + 0] + tc2.x * kernelsL[0 * 72 + 4 * 9 + 1] + tr2.x * kernelsL[0 * 72 + 4 * 9 + 2] +
		ml2.x * kernelsL[0 * 72 + 4 * 9 + 3] + mc2.x * kernelsL[0 * 72 + 4 * 9 + 4] + mr2.x * kernelsL[0 * 72 + 4 * 9 + 5] +
		bl2.x * kernelsL[0 * 72 + 4 * 9 + 6] + bc2.x * kernelsL[0 * 72 + 4 * 9 + 7] + br2.x * kernelsL[0 * 72 + 4 * 9 + 8] +

		tl2.y * kernelsL[0 * 72 + 5 * 9 + 0] + tc2.y * kernelsL[0 * 72 + 5 * 9 + 1] + tr2.y * kernelsL[0 * 72 + 5 * 9 + 2] +
		ml2.y * kernelsL[0 * 72 + 5 * 9 + 3] + mc2.y * kernelsL[0 * 72 + 5 * 9 + 4] + mr2.y * kernelsL[0 * 72 + 5 * 9 + 5] +
		bl2.y * kernelsL[0 * 72 + 5 * 9 + 6] + bc2.y * kernelsL[0 * 72 + 5 * 9 + 7] + br2.y * kernelsL[0 * 72 + 5 * 9 + 8] +

		tl2.z * kernelsL[0 * 72 + 6 * 9 + 0] + tc2.z * kernelsL[0 * 72 + 6 * 9 + 1] + tr2.z * kernelsL[0 * 72 + 6 * 9 + 2] +
		ml2.z * kernelsL[0 * 72 + 6 * 9 + 3] + mc2.z * kernelsL[0 * 72 + 6 * 9 + 4] + mr2.z * kernelsL[0 * 72 + 6 * 9 + 5] +
		bl2.z * kernelsL[0 * 72 + 6 * 9 + 6] + bc2.z * kernelsL[0 * 72 + 6 * 9 + 7] + br2.z * kernelsL[0 * 72 + 6 * 9 + 8] +

		tl2.w * kernelsL[0 * 72 + 7 * 9 + 0] + tc2.w * kernelsL[0 * 72 + 7 * 9 + 1] + tr2.w * kernelsL[0 * 72 + 7 * 9 + 2] +
		ml2.w * kernelsL[0 * 72 + 7 * 9 + 3] + mc2.w * kernelsL[0 * 72 + 7 * 9 + 4] + mr2.w * kernelsL[0 * 72 + 7 * 9 + 5] +
		bl2.w * kernelsL[0 * 72 + 7 * 9 + 6] + bc2.w * kernelsL[0 * 72 + 7 * 9 + 7] + br2.w * kernelsL[0 * 72 + 7 * 9 + 8] + biasL.x
		,
		tl1.x * kernelsL[1 * 72 + 0 * 9 + 0] + tc1.x * kernelsL[1 * 72 + 0 * 9 + 1] + tr1.x * kernelsL[1 * 72 + 0 * 9 + 2] +
		ml1.x * kernelsL[1 * 72 + 0 * 9 + 3] + mc1.x * kernelsL[1 * 72 + 0 * 9 + 4] + mr1.x * kernelsL[1 * 72 + 0 * 9 + 5] +
		bl1.x * kernelsL[1 * 72 + 0 * 9 + 6] + bc1.x * kernelsL[1 * 72 + 0 * 9 + 7] + br1.x * kernelsL[1 * 72 + 0 * 9 + 8] +

		tl1.y * kernelsL[1 * 72 + 1 * 9 + 0] + tc1.y * kernelsL[1 * 72 + 1 * 9 + 1] + tr1.y * kernelsL[1 * 72 + 1 * 9 + 2] +
		ml1.y * kernelsL[1 * 72 + 1 * 9 + 3] + mc1.y * kernelsL[1 * 72 + 1 * 9 + 4] + mr1.y * kernelsL[1 * 72 + 1 * 9 + 5] +
		bl1.y * kernelsL[1 * 72 + 1 * 9 + 6] + bc1.y * kernelsL[1 * 72 + 1 * 9 + 7] + br1.y * kernelsL[1 * 72 + 1 * 9 + 8] +

		tl1.z * kernelsL[1 * 72 + 2 * 9 + 0] + tc1.z * kernelsL[1 * 72 + 2 * 9 + 1] + tr1.z * kernelsL[1 * 72 + 2 * 9 + 2] +
		ml1.z * kernelsL[1 * 72 + 2 * 9 + 3] + mc1.z * kernelsL[1 * 72 + 2 * 9 + 4] + mr1.z * kernelsL[1 * 72 + 2 * 9 + 5] +
		bl1.z * kernelsL[1 * 72 + 2 * 9 + 6] + bc1.z * kernelsL[1 * 72 + 2 * 9 + 7] + br1.z * kernelsL[1 * 72 + 2 * 9 + 8] +

		tl1.w * kernelsL[1 * 72 + 3 * 9 + 0] + tc1.w * kernelsL[1 * 72 + 3 * 9 + 1] + tr1.w * kernelsL[1 * 72 + 3 * 9 + 2] +
		ml1.w * kernelsL[1 * 72 + 3 * 9 + 3] + mc1.w * kernelsL[1 * 72 + 3 * 9 + 4] + mr1.w * kernelsL[1 * 72 + 3 * 9 + 5] +
		bl1.w * kernelsL[1 * 72 + 3 * 9 + 6] + bc1.w * kernelsL[1 * 72 + 3 * 9 + 7] + br1.w * kernelsL[1 * 72 + 3 * 9 + 8] +

		tl2.x * kernelsL[1 * 72 + 4 * 9 + 0] + tc2.x * kernelsL[1 * 72 + 4 * 9 + 1] + tr2.x * kernelsL[1 * 72 + 4 * 9 + 2] +
		ml2.x * kernelsL[1 * 72 + 4 * 9 + 3] + mc2.x * kernelsL[1 * 72 + 4 * 9 + 4] + mr2.x * kernelsL[1 * 72 + 4 * 9 + 5] +
		bl2.x * kernelsL[1 * 72 + 4 * 9 + 6] + bc2.x * kernelsL[1 * 72 + 4 * 9 + 7] + br2.x * kernelsL[1 * 72 + 4 * 9 + 8] +

		tl2.y * kernelsL[1 * 72 + 5 * 9 + 0] + tc2.y * kernelsL[1 * 72 + 5 * 9 + 1] + tr2.y * kernelsL[1 * 72 + 5 * 9 + 2] +
		ml2.y * kernelsL[1 * 72 + 5 * 9 + 3] + mc2.y * kernelsL[1 * 72 + 5 * 9 + 4] + mr2.y * kernelsL[1 * 72 + 5 * 9 + 5] +
		bl2.y * kernelsL[1 * 72 + 5 * 9 + 6] + bc2.y * kernelsL[1 * 72 + 5 * 9 + 7] + br2.y * kernelsL[1 * 72 + 5 * 9 + 8] +

		tl2.z * kernelsL[1 * 72 + 6 * 9 + 0] + tc2.z * kernelsL[1 * 72 + 6 * 9 + 1] + tr2.z * kernelsL[1 * 72 + 6 * 9 + 2] +
		ml2.z * kernelsL[1 * 72 + 6 * 9 + 3] + mc2.z * kernelsL[1 * 72 + 6 * 9 + 4] + mr2.z * kernelsL[1 * 72 + 6 * 9 + 5] +
		bl2.z * kernelsL[1 * 72 + 6 * 9 + 6] + bc2.z * kernelsL[1 * 72 + 6 * 9 + 7] + br2.z * kernelsL[1 * 72 + 6 * 9 + 8] +

		tl2.w * kernelsL[1 * 72 + 7 * 9 + 0] + tc2.w * kernelsL[1 * 72 + 7 * 9 + 1] + tr2.w * kernelsL[1 * 72 + 7 * 9 + 2] +
		ml2.w * kernelsL[1 * 72 + 7 * 9 + 3] + mc2.w * kernelsL[1 * 72 + 7 * 9 + 4] + mr2.w * kernelsL[1 * 72 + 7 * 9 + 5] +
		bl2.w * kernelsL[1 * 72 + 7 * 9 + 6] + bc2.w * kernelsL[1 * 72 + 7 * 9 + 7] + br2.w * kernelsL[1 * 72 + 7 * 9 + 8] + biasL.y
		,
		tl1.x * kernelsL[2 * 72 + 0 * 9 + 0] + tc1.x * kernelsL[2 * 72 + 0 * 9 + 1] + tr1.x * kernelsL[2 * 72 + 0 * 9 + 2] +
		ml1.x * kernelsL[2 * 72 + 0 * 9 + 3] + mc1.x * kernelsL[2 * 72 + 0 * 9 + 4] + mr1.x * kernelsL[2 * 72 + 0 * 9 + 5] +
		bl1.x * kernelsL[2 * 72 + 0 * 9 + 6] + bc1.x * kernelsL[2 * 72 + 0 * 9 + 7] + br1.x * kernelsL[2 * 72 + 0 * 9 + 8] +

		tl1.y * kernelsL[2 * 72 + 1 * 9 + 0] + tc1.y * kernelsL[2 * 72 + 1 * 9 + 1] + tr1.y * kernelsL[2 * 72 + 1 * 9 + 2] +
		ml1.y * kernelsL[2 * 72 + 1 * 9 + 3] + mc1.y * kernelsL[2 * 72 + 1 * 9 + 4] + mr1.y * kernelsL[2 * 72 + 1 * 9 + 5] +
		bl1.y * kernelsL[2 * 72 + 1 * 9 + 6] + bc1.y * kernelsL[2 * 72 + 1 * 9 + 7] + br1.y * kernelsL[2 * 72 + 1 * 9 + 8] +

		tl1.z * kernelsL[2 * 72 + 2 * 9 + 0] + tc1.z * kernelsL[2 * 72 + 2 * 9 + 1] + tr1.z * kernelsL[2 * 72 + 2 * 9 + 2] +
		ml1.z * kernelsL[2 * 72 + 2 * 9 + 3] + mc1.z * kernelsL[2 * 72 + 2 * 9 + 4] + mr1.z * kernelsL[2 * 72 + 2 * 9 + 5] +
		bl1.z * kernelsL[2 * 72 + 2 * 9 + 6] + bc1.z * kernelsL[2 * 72 + 2 * 9 + 7] + br1.z * kernelsL[2 * 72 + 2 * 9 + 8] +

		tl1.w * kernelsL[2 * 72 + 3 * 9 + 0] + tc1.w * kernelsL[2 * 72 + 3 * 9 + 1] + tr1.w * kernelsL[2 * 72 + 3 * 9 + 2] +
		ml1.w * kernelsL[2 * 72 + 3 * 9 + 3] + mc1.w * kernelsL[2 * 72 + 3 * 9 + 4] + mr1.w * kernelsL[2 * 72 + 3 * 9 + 5] +
		bl1.w * kernelsL[2 * 72 + 3 * 9 + 6] + bc1.w * kernelsL[2 * 72 + 3 * 9 + 7] + br1.w * kernelsL[2 * 72 + 3 * 9 + 8] +

		tl2.x * kernelsL[2 * 72 + 4 * 9 + 0] + tc2.x * kernelsL[2 * 72 + 4 * 9 + 1] + tr2.x * kernelsL[2 * 72 + 4 * 9 + 2] +
		ml2.x * kernelsL[2 * 72 + 4 * 9 + 3] + mc2.x * kernelsL[2 * 72 + 4 * 9 + 4] + mr2.x * kernelsL[2 * 72 + 4 * 9 + 5] +
		bl2.x * kernelsL[2 * 72 + 4 * 9 + 6] + bc2.x * kernelsL[2 * 72 + 4 * 9 + 7] + br2.x * kernelsL[2 * 72 + 4 * 9 + 8] +

		tl2.y * kernelsL[2 * 72 + 5 * 9 + 0] + tc2.y * kernelsL[2 * 72 + 5 * 9 + 1] + tr2.y * kernelsL[2 * 72 + 5 * 9 + 2] +
		ml2.y * kernelsL[2 * 72 + 5 * 9 + 3] + mc2.y * kernelsL[2 * 72 + 5 * 9 + 4] + mr2.y * kernelsL[2 * 72 + 5 * 9 + 5] +
		bl2.y * kernelsL[2 * 72 + 5 * 9 + 6] + bc2.y * kernelsL[2 * 72 + 5 * 9 + 7] + br2.y * kernelsL[2 * 72 + 5 * 9 + 8] +

		tl2.z * kernelsL[2 * 72 + 6 * 9 + 0] + tc2.z * kernelsL[2 * 72 + 6 * 9 + 1] + tr2.z * kernelsL[2 * 72 + 6 * 9 + 2] +
		ml2.z * kernelsL[2 * 72 + 6 * 9 + 3] + mc2.z * kernelsL[2 * 72 + 6 * 9 + 4] + mr2.z * kernelsL[2 * 72 + 6 * 9 + 5] +
		bl2.z * kernelsL[2 * 72 + 6 * 9 + 6] + bc2.z * kernelsL[2 * 72 + 6 * 9 + 7] + br2.z * kernelsL[2 * 72 + 6 * 9 + 8] +

		tl2.w * kernelsL[2 * 72 + 7 * 9 + 0] + tc2.w * kernelsL[2 * 72 + 7 * 9 + 1] + tr2.w * kernelsL[2 * 72 + 7 * 9 + 2] +
		ml2.w * kernelsL[2 * 72 + 7 * 9 + 3] + mc2.w * kernelsL[2 * 72 + 7 * 9 + 4] + mr2.w * kernelsL[2 * 72 + 7 * 9 + 5] +
		bl2.w * kernelsL[2 * 72 + 7 * 9 + 6] + bc2.w * kernelsL[2 * 72 + 7 * 9 + 7] + br2.w * kernelsL[2 * 72 + 7 * 9 + 8] + biasL.z
		,
		tl1.x * kernelsL[3 * 72 + 0 * 9 + 0] + tc1.x * kernelsL[3 * 72 + 0 * 9 + 1] + tr1.x * kernelsL[3 * 72 + 0 * 9 + 2] +
		ml1.x * kernelsL[3 * 72 + 0 * 9 + 3] + mc1.x * kernelsL[3 * 72 + 0 * 9 + 4] + mr1.x * kernelsL[3 * 72 + 0 * 9 + 5] +
		bl1.x * kernelsL[3 * 72 + 0 * 9 + 6] + bc1.x * kernelsL[3 * 72 + 0 * 9 + 7] + br1.x * kernelsL[3 * 72 + 0 * 9 + 8] +

		tl1.y * kernelsL[3 * 72 + 1 * 9 + 0] + tc1.y * kernelsL[3 * 72 + 1 * 9 + 1] + tr1.y * kernelsL[3 * 72 + 1 * 9 + 2] +
		ml1.y * kernelsL[3 * 72 + 1 * 9 + 3] + mc1.y * kernelsL[3 * 72 + 1 * 9 + 4] + mr1.y * kernelsL[3 * 72 + 1 * 9 + 5] +
		bl1.y * kernelsL[3 * 72 + 1 * 9 + 6] + bc1.y * kernelsL[3 * 72 + 1 * 9 + 7] + br1.y * kernelsL[3 * 72 + 1 * 9 + 8] +

		tl1.z * kernelsL[3 * 72 + 2 * 9 + 0] + tc1.z * kernelsL[3 * 72 + 2 * 9 + 1] + tr1.z * kernelsL[3 * 72 + 2 * 9 + 2] +
		ml1.z * kernelsL[3 * 72 + 2 * 9 + 3] + mc1.z * kernelsL[3 * 72 + 2 * 9 + 4] + mr1.z * kernelsL[3 * 72 + 2 * 9 + 5] +
		bl1.z * kernelsL[3 * 72 + 2 * 9 + 6] + bc1.z * kernelsL[3 * 72 + 2 * 9 + 7] + br1.z * kernelsL[3 * 72 + 2 * 9 + 8] +

		tl1.w * kernelsL[3 * 72 + 3 * 9 + 0] + tc1.w * kernelsL[3 * 72 + 3 * 9 + 1] + tr1.w * kernelsL[3 * 72 + 3 * 9 + 2] +
		ml1.w * kernelsL[3 * 72 + 3 * 9 + 3] + mc1.w * kernelsL[3 * 72 + 3 * 9 + 4] + mr1.w * kernelsL[3 * 72 + 3 * 9 + 5] +
		bl1.w * kernelsL[3 * 72 + 3 * 9 + 6] + bc1.w * kernelsL[3 * 72 + 3 * 9 + 7] + br1.w * kernelsL[3 * 72 + 3 * 9 + 8] +

		tl2.x * kernelsL[3 * 72 + 4 * 9 + 0] + tc2.x * kernelsL[3 * 72 + 4 * 9 + 1] + tr2.x * kernelsL[3 * 72 + 4 * 9 + 2] +
		ml2.x * kernelsL[3 * 72 + 4 * 9 + 3] + mc2.x * kernelsL[3 * 72 + 4 * 9 + 4] + mr2.x * kernelsL[3 * 72 + 4 * 9 + 5] +
		bl2.x * kernelsL[3 * 72 + 4 * 9 + 6] + bc2.x * kernelsL[3 * 72 + 4 * 9 + 7] + br2.x * kernelsL[3 * 72 + 4 * 9 + 8] +

		tl2.y * kernelsL[3 * 72 + 5 * 9 + 0] + tc2.y * kernelsL[3 * 72 + 5 * 9 + 1] + tr2.y * kernelsL[3 * 72 + 5 * 9 + 2] +
		ml2.y * kernelsL[3 * 72 + 5 * 9 + 3] + mc2.y * kernelsL[3 * 72 + 5 * 9 + 4] + mr2.y * kernelsL[3 * 72 + 5 * 9 + 5] +
		bl2.y * kernelsL[3 * 72 + 5 * 9 + 6] + bc2.y * kernelsL[3 * 72 + 5 * 9 + 7] + br2.y * kernelsL[3 * 72 + 5 * 9 + 8] +

		tl2.z * kernelsL[3 * 72 + 6 * 9 + 0] + tc2.z * kernelsL[3 * 72 + 6 * 9 + 1] + tr2.z * kernelsL[3 * 72 + 6 * 9 + 2] +
		ml2.z * kernelsL[3 * 72 + 6 * 9 + 3] + mc2.z * kernelsL[3 * 72 + 6 * 9 + 4] + mr2.z * kernelsL[3 * 72 + 6 * 9 + 5] +
		bl2.z * kernelsL[3 * 72 + 6 * 9 + 6] + bc2.z * kernelsL[3 * 72 + 6 * 9 + 7] + br2.z * kernelsL[3 * 72 + 6 * 9 + 8] +

		tl2.w * kernelsL[3 * 72 + 7 * 9 + 0] + tc2.w * kernelsL[3 * 72 + 7 * 9 + 1] + tr2.w * kernelsL[3 * 72 + 7 * 9 + 2] +
		ml2.w * kernelsL[3 * 72 + 7 * 9 + 3] + mc2.w * kernelsL[3 * 72 + 7 * 9 + 4] + mr2.w * kernelsL[3 * 72 + 7 * 9 + 5] +
		bl2.w * kernelsL[3 * 72 + 7 * 9 + 6] + bc2.w * kernelsL[3 * 72 + 7 * 9 + 7] + br2.w * kernelsL[3 * 72 + 7 * 9 + 8] + biasL.w
	));

	return compressLinear(c1234, 0, 4);
}
