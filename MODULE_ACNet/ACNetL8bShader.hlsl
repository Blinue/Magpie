// ACNet_L8_2
// ÒÆÖ²×Ô https://github.com/TianZerL/ACNetGLSL/blob/master/glsl/ACNet.glsl


cbuffer constants : register(b0) {
	int2 srcSize : packoffset(c0);
};


#define MAGPIE_INPUT_COUNT 2
#include "ACNet.hlsli"


const static float kernelsL[9 * 8 * 4] = {
	 7.7038e-02, -1.6317e-02, -2.4118e-02,
	-4.3086e-02, -2.1512e-01,  1.2288e-01,
	 1.8237e-01, -1.5438e-01, -1.1346e-01,
	-4.6141e-02, -4.0750e-02, -5.6414e-04,
	-1.5640e-01, -3.4506e-01, -1.4441e-02,
	-2.0278e-01, -3.1403e-01, -6.2542e-02,
	-1.9622e-02,  1.6348e-02,  6.9859e-03,
	-9.3142e-02,  1.0368e-02, -5.6585e-02,
	 8.4213e-02,  1.0776e-01, -1.0315e-01,
	 8.7873e-41, -5.3947e-40,  1.1714e-40,
	 7.5534e-41, -1.1871e-40, -5.4012e-40,
	 3.8269e-41, -1.4913e-40, -3.1802e-40,
	-3.4707e-02,  1.2518e-02,  9.4679e-03,
	 1.2254e-01,  1.9394e-01,  2.6530e-02,
	 2.2413e-01, -1.6298e-01, -6.1446e-02,
	-1.1042e-42, -2.7255e-40, -5.5067e-40,
	 3.8272e-40,  4.9956e-40, -3.2074e-41,
	 2.8351e-40,  4.2501e-40,  3.9389e-41,
	 6.1941e-40, -4.8790e-40, -3.4137e-40,
	 2.2577e-40, -5.7183e-40, -8.6861e-41,
	 5.7021e-40, -3.2349e-40,  1.9655e-40,
	 9.1180e-02,  5.6665e-02, -6.5437e-04,
	 1.1759e-01,  2.7517e-01,  1.9143e-01,
	 9.7905e-02,  6.6707e-02,  8.6535e-02,
	 8.8717e-03,  3.0913e-02,  6.6909e-03,
	-8.1791e-02, -4.7883e-01,  7.4920e-02,
	 4.5843e-01, -1.0410e-01,  1.6655e-01,
	-4.7094e-03,  3.4769e-02, -1.3291e-02,
	-8.5570e-03, -4.0038e-01,  1.8418e-01,
	-1.4696e-01,  3.2279e-01,  2.5712e-02,
	-2.6207e-01, -4.6150e-02, -6.4099e-02,
	-3.2623e-01, -1.8984e-01, -5.7891e-02,
	-2.2088e-01, -4.2042e-02, -2.5307e-02,
	 1.0260e-40,  5.0443e-40,  7.5150e-41,
	 1.4402e-40, -5.1952e-40, -5.3810e-40,
	 6.2240e-40,  1.8661e-40, -8.2983e-41,
	 7.1850e-02,  4.8770e-02, -1.5081e-02,
	 4.8072e-01,  2.5477e-01,  3.8197e-02,
	 2.6011e-01,  2.4610e-01, -3.6167e-02,
	 3.8901e-40,  1.6760e-41,  2.8471e-40,
	 3.1983e-40,  1.2460e-40, -4.3961e-40,
	 3.9187e-40,  2.7818e-40, -9.1501e-41,
	-2.3320e-40, -1.9998e-40, -2.8132e-40,
	-2.9552e-40, -3.9643e-40, -5.1375e-40,
	-1.6686e-40, -5.3138e-40, -2.6988e-40,
	 2.5623e-02,  2.6942e-02,  2.4342e-02,
	-9.9084e-02,  5.2974e-01, -6.7983e-02,
	-2.2454e-01,  1.1507e-01,  2.0364e-02,
	 3.4852e-01, -3.1091e-01,  8.1154e-02,
	-3.2205e-01,  1.7103e-01,  2.4162e-01,
	-2.6892e-03,  2.4142e-02,  5.5540e-02,
	-4.5753e-02, -5.0097e-01,  1.7503e-01,
	 1.4058e-01,  1.1311e-01,  1.5945e-01,
	-5.3975e-02,  5.2326e-02, -6.2382e-02,
	 9.4114e-02, -5.6812e-01, -1.2081e-01,
	-8.5809e-02, -9.8661e-03, -2.3064e-02,
	-1.6453e-03, -1.8328e-02,  2.4282e-03,
	 1.5943e-40,  4.6894e-40, -6.2730e-40,
	 3.8054e-40, -3.7914e-41, -1.4429e-40,
	 1.6925e-40,  5.1566e-41, -1.7909e-40,
	-3.7920e-02,  2.4698e-01,  5.0019e-02,
	-1.4246e-02,  2.8739e-01, -5.4704e-02,
	 7.9436e-02, -2.7838e-02, -3.4191e-02,
	-3.3565e-40,  2.1368e-40,  6.7346e-42,
	 5.6681e-40, -5.5776e-40, -2.7705e-40,
	-2.2966e-40,  1.1692e-40, -2.5187e-40,
	 4.4806e-40, -4.8424e-40, -9.1436e-41,
	-4.3250e-40, -2.0721e-40, -2.0050e-40,
	-5.1061e-40,  2.6405e-40, -3.0913e-40,
	-1.2078e-01,  3.1948e-01,  1.0082e-02,
	-1.0781e-02,  8.0720e-02, -4.6330e-02,
	-1.8084e-02, -2.2846e-02, -5.5861e-03,
	-3.2400e-02, -1.7329e-01, -2.7995e-02,
	-5.3680e-02,  4.1310e-01, -9.4691e-02,
	 7.6938e-02, -4.9596e-02,  1.9649e-01,
	 3.2594e-02,  1.1544e-01, -1.8501e-02,
	 7.0248e-02, -6.9838e-02, -5.4278e-02,
	-2.9317e-02, -1.4890e-01,  7.8661e-02,
	 3.7685e-02,  5.9594e-02,  8.9527e-02,
	 2.2957e-01, -2.9681e-01, -1.6329e-01,
	-1.3206e-01, -4.3808e-02,  3.8854e-02,
	 1.7529e-40, -3.8429e-41,  1.4443e-40,
	-4.0829e-40, -2.5643e-40, -5.4821e-40,
	 1.6827e-40, -1.1628e-40,  2.2441e-40,
	 5.2451e-02,  1.0179e-01,  4.8487e-02,
	-2.1020e-01, -4.4345e-01, -8.7642e-02,
	 7.0958e-02,  1.9934e-01, -2.1090e-02,
	-3.0795e-41,  2.7921e-40,  2.8491e-40,
	-2.1154e-40,  9.8876e-41, -8.8824e-41,
	 2.6552e-40,  2.5767e-40, -3.8369e-40,
	 6.1348e-40, -3.4170e-40, -1.7109e-40,
	-3.3080e-40,  5.4199e-41, -1.7512e-40,
	 1.8363e-40, -4.4080e-40, -2.5508e-40,
	-4.0716e-02, -2.8531e-01,  3.9981e-02,
	 2.2278e-02,  5.6661e-01, -8.3890e-02,
	-7.7331e-02, -9.3843e-02,  1.5584e-02
};

const static float4 biasL = { 7.9956e-02, 3.0679e-04, -1.0257e-02, -1.2037e-02 };


D2D_PS_ENTRY(main) {
	InitMagpieSampleInput();

	float2 leftTop1 = max(0, Coord(0).xy - Coord(0).zw);
	float2 rightBottom1 = min(maxCoord0.xy, Coord(0).xy + Coord(0).zw);

	// [tl, tc, tr]
	// [ml, mc, mr]
	// [bl, bc, br]
	float4 tl1 = uncompressLinear(SampleInput(0, leftTop1), 0, 4);
	float4 ml1 = uncompressLinear(SampleInput(0, float2(leftTop1.x, Coord(0).y)), 0, 4);
	float4 bl1 = uncompressLinear(SampleInput(0, float2(leftTop1.x, rightBottom1.y)), 0, 4);
	float4 tc1 = uncompressLinear(SampleInput(0, float2(Coord(0).x, leftTop1.y)), 0, 4);
	float4 mc1 = uncompressLinear(SampleInputCur(0), 0, 4);
	float4 bc1 = uncompressLinear(SampleInput(0, float2(Coord(0).x, rightBottom1.y)), 0, 4);
	float4 tr1 = uncompressLinear(SampleInput(0, float2(rightBottom1.x, leftTop1.y)), 0, 4);
	float4 mr1 = uncompressLinear(SampleInput(0, float2(rightBottom1.x, Coord(0).y)), 0, 4);
	float4 br1 = uncompressLinear(SampleInput(0, rightBottom1), 0, 4);

	float2 leftTop2 = max(0, Coord(1).xy - Coord(1).zw);
	float2 rightBottom2 = min(maxCoord1.xy, Coord(1).xy + Coord(1).zw);

	float4 tl2 = uncompressLinear(SampleInput(1, leftTop2), 0, 4);
	float4 ml2 = uncompressLinear(SampleInput(1, float2(leftTop2.x, Coord(1).y)), 0, 4);
	float4 bl2 = uncompressLinear(SampleInput(1, float2(leftTop2.x, rightBottom2.y)), 0, 4);
	float4 tc2 = uncompressLinear(SampleInput(1, float2(Coord(1).x, leftTop2.y)), 0, 4);
	float4 mc2 = uncompressLinear(SampleInputCur(1), 0, 4);
	float4 bc2 = uncompressLinear(SampleInput(1, float2(Coord(1).x, rightBottom2.y)), 0, 4);
	float4 tr2 = uncompressLinear(SampleInput(1, float2(rightBottom2.x, leftTop2.y)), 0, 4);
	float4 mr2 = uncompressLinear(SampleInput(1, float2(rightBottom2.x, Coord(1).y)), 0, 4);
	float4 br2 = uncompressLinear(SampleInput(1, rightBottom2), 0, 4);

	float4 c5678 = RELU(float4(
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

	return compressLinear(c5678, 0, 4);
}
