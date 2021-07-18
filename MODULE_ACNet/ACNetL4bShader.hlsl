// ACNet_L4_2
// ÒÆÖ²×Ô https://github.com/TianZerL/ACNetGLSL/blob/master/glsl/ACNet.glsl


cbuffer constants : register(b0) {
	int2 srcSize : packoffset(c0);
};


#define MAGPIE_INPUT_COUNT 2
#include "ACNet.hlsli"


const static float kernelsL[9 * 8 * 4] = {
	-5.2396e-02,  1.2310e-01, -5.2917e-02,
	-4.3708e-03,  1.9560e-01, -2.4309e-02,
	-6.7388e-02, -8.8839e-02, -2.0907e-02,
	 4.6550e-02,  3.4119e-02,  6.0977e-02,
	-1.0054e-02,  1.4411e-01,  1.5622e-01,
	 1.7401e-02,  2.5685e-01, -9.1853e-03,
	-4.4530e-02, -1.8623e-01, -8.4557e-02,
	 9.5962e-02,  2.6491e-01,  1.7854e-01,
	-2.0547e-02, -1.2023e-01, -7.6897e-02,
	-1.3418e-01, -1.4960e-01,  1.6292e-01,
	-1.7275e-01, -6.0181e-02, -2.7034e-02,
	-7.4189e-02, -3.5566e-02,  1.3995e-01,
	 3.0758e-02,  3.3476e-02,  6.9837e-03,
	-6.1089e-02, -9.6021e-02,  7.1716e-03,
	 1.0389e-01,  4.7963e-02,  9.5921e-02,
	 4.4569e-02,  1.2230e-01, -1.4417e-01,
	-1.2825e-02,  3.1980e-01, -3.5905e-01,
	-1.2557e-01, -7.5283e-02, -1.2343e-01,
	 1.9791e-01,  7.9003e-02,  3.1163e-02,
	 1.0969e-01,  1.6839e-01, -2.5816e-01,
	-1.2617e-01,  1.3686e-01, -2.1078e-01,
	-2.1870e-02, -1.8378e-01, -2.8893e-01,
	-8.2523e-02, -3.0475e-02,  9.6007e-02,
	 1.0669e-01, -1.4581e-03,  3.2441e-01,
	-8.1872e-03,  1.1690e-02, -4.0179e-02,
	-1.0835e-01,  3.6112e-01, -4.5990e-02,
	-1.2355e-01, -1.3372e-01,  3.8136e-02,
	-9.1530e-03,  3.5432e-02,  4.3950e-02,
	-8.6859e-02,  1.5887e-01,  1.2796e-02,
	 1.3554e-02, -1.5669e-01, -1.4371e-02,
	-4.6609e-02,  1.7114e-01, -7.8284e-02,
	 1.7611e-01,  4.1204e-01,  9.3281e-02,
	 1.1420e-01,  1.2951e-01, -7.6025e-02,
	-5.4831e-02,  9.7574e-02,  3.2839e-02,
	 3.8475e-02, -6.0247e-02, -2.9627e-02,
	-2.4367e-02,  1.3143e-02,  4.7017e-02,
	 2.3800e-02, -2.4046e-02, -5.7044e-02,
	 2.7280e-02,  7.8573e-01,  1.0079e-02,
	 6.4100e-02,  5.1584e-02,  7.9653e-03,
	-8.9480e-02, -1.6207e-01, -8.9418e-02,
	-3.5589e-02,  3.5903e-01, -1.8381e-01,
	 9.2356e-02,  8.8046e-02, -5.0229e-02,
	 1.8609e-02,  1.1243e-01,  5.2599e-02,
	-1.3374e-02, -3.3097e-01,  6.5346e-02,
	 2.6760e-01, -1.0281e-01,  1.1607e-02,
	 7.6576e-03, -3.5957e-02,  3.1924e-02,
	-7.0088e-02,  9.1241e-02,  1.2827e-02,
	 3.7165e-02,  7.0273e-03, -7.3945e-04,
	-6.5406e-03,  7.2666e-02, -5.7348e-02,
	-1.9100e-01, -7.4449e-02, -1.2496e-01,
	 1.5299e-01, -8.8047e-02, -2.1810e-02,
	-3.0241e-02, -7.4310e-03, -8.7682e-02,
	-2.2479e-02,  9.6008e-02, -8.4539e-02,
	-2.8915e-02,  1.7538e-01, -3.7735e-02,
	-9.8463e-03, -6.9618e-02, -2.6095e-01,
	 9.9950e-02,  5.0534e-01, -1.8812e-01,
	-1.1986e-01,  7.1166e-02, -2.4769e-02,
	 8.8529e-02,  9.8348e-02,  2.1136e-02,
	-9.0337e-03,  1.3679e-01, -1.2115e-01,
	-6.2478e-03,  1.1436e-01, -3.4610e-02,
	-2.7350e-02,  1.0702e-01,  1.6220e-02,
	 1.0912e-02,  1.0953e-01,  8.6762e-02,
	 2.9348e-03, -2.2035e-02,  1.2376e-01,
	 7.0102e-02, -1.0945e-01, -1.6640e-01,
	-3.9916e-03, -2.6658e-02, -9.7031e-02,
	-3.0047e-02,  1.6631e-03, -5.5031e-02,
	-7.9624e-02,  1.9976e-01,  1.9582e-01,
	 2.1377e-01,  3.5835e-01,  1.7012e-01,
	-9.7751e-02,  4.9143e-01,  1.0988e-01,
	 8.4055e-02, -7.3187e-03, -9.8808e-02,
	 5.0590e-02, -8.9291e-02, -6.6857e-02,
	 9.6737e-02, -3.0699e-01,  2.2889e-01,
	 2.6727e-40, -5.2704e-40, -4.5038e-40,
	-3.3108e-40,  5.2330e-40, -1.2724e-40,
	-3.2957e-40, -5.8613e-40,  2.1618e-40,
	-4.3882e-40, -3.3950e-40,  5.9372e-40,
	 2.7277e-40, -1.3741e-40, -3.3597e-40,
	 5.0687e-40,  4.7873e-40, -3.2116e-40,
	-6.1388e-40, -6.0790e-40, -5.2667e-40,
	-5.6524e-40, -6.1696e-40, -5.9796e-40,
	 1.5824e-40, -5.2002e-40, -5.8960e-40,
	-5.9860e-40,  3.6419e-40,  2.9975e-40,
	-5.8988e-40,  3.3994e-40, -5.0611e-40,
	 3.6410e-40,  2.9550e-40,  4.7468e-40,
	 2.7503e-40, -3.4103e-40,  6.0339e-40,
	-1.7691e-40,  6.7170e-41,  1.7101e-40,
	 2.7166e-40,  4.3023e-40,  2.7735e-40,
	-3.1937e-40, -4.9247e-40, -6.2495e-40,
	 5.2938e-40, -3.3702e-40,  1.4976e-41,
	 1.4031e-40, -4.6995e-40, -5.2409e-40,
	 2.5460e-40,  2.6670e-40, -4.5339e-40,
	 4.2896e-40, -5.7141e-40, -1.7003e-40,
	 2.3597e-40,  1.3748e-40,  4.6163e-40,
	 4.0680e-41, -6.1642e-40,  2.7304e-41,
	 5.2250e-40, -3.9481e-40, -6.1808e-40,
	 1.9462e-40,  2.6005e-40, -2.7281e-40
};

const static float4 biasL = { -8.1892e-04, 3.3171e-03, -1.1582e-02, -4.1205e-40 };


D2D_PS_ENTRY(main) {
	InitMagpieSampleInput();

	float2 leftTop1 = max(0, Coord(0).xy - Coord(0).zw);
	float2 rightBottom1 = min(maxCoord0.xy, Coord(0).xy + Coord(0).zw);

	// [tl, tc, tr]
	// [ml, mc, mr]
	// [bl, bc, br]
	float4 tl1 = uncompressLinear(SampleInput(0, leftTop1), 0, 3.5);
	float4 ml1 = uncompressLinear(SampleInput(0, float2(leftTop1.x, Coord(0).y)), 0, 3.5);
	float4 bl1 = uncompressLinear(SampleInput(0, float2(leftTop1.x, rightBottom1.y)), 0, 3.5);
	float4 tc1 = uncompressLinear(SampleInput(0, float2(Coord(0).x, leftTop1.y)), 0, 3.5);
	float4 mc1 = uncompressLinear(SampleInputCur(0), 0, 3.5);
	float4 bc1 = uncompressLinear(SampleInput(0, float2(Coord(0).x, rightBottom1.y)), 0, 3.5);
	float4 tr1 = uncompressLinear(SampleInput(0, float2(rightBottom1.x, leftTop1.y)), 0, 3.5);
	float4 mr1 = uncompressLinear(SampleInput(0, float2(rightBottom1.x, Coord(0).y)), 0, 3.5);
	float4 br1 = uncompressLinear(SampleInput(0, rightBottom1), 0, 3.5);

	float2 leftTop2 = max(0, Coord(1).xy - Coord(1).zw);
	float2 rightBottom2 = min(maxCoord1.xy, Coord(1).xy + Coord(1).zw);

	float4 tl2 = uncompressLinear(SampleInput(1, leftTop2), 0, 2);
	float4 ml2 = uncompressLinear(SampleInput(1, float2(leftTop2.x, Coord(1).y)), 0, 2);
	float4 bl2 = uncompressLinear(SampleInput(1, float2(leftTop2.x, rightBottom2.y)), 0, 2);
	float4 tc2 = uncompressLinear(SampleInput(1, float2(Coord(1).x, leftTop2.y)), 0, 2);
	float4 mc2 = uncompressLinear(SampleInputCur(1), 0, 2);
	float4 bc2 = uncompressLinear(SampleInput(1, float2(Coord(1).x, rightBottom2.y)), 0, 2);
	float4 tr2 = uncompressLinear(SampleInput(1, float2(rightBottom2.x, leftTop2.y)), 0, 2);
	float4 mr2 = uncompressLinear(SampleInput(1, float2(rightBottom2.x, Coord(1).y)), 0, 2);
	float4 br2 = uncompressLinear(SampleInput(1, rightBottom2), 0, 2);

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

	return compressLinear(c5678, 0, 3);
}
