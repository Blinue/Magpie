// ACNet_L9_1
// 移植自 https://github.com/TianZerL/ACNetGLSL/blob/master/glsl/ACNet.glsl


cbuffer constants : register(b0) {
	int2 srcSize : packoffset(c0);
};


#define MAGPIE_INPUT_COUNT 2
#include "ACNet.hlsli"


const static float kernelsL[9 * 8 * 4] = {
	-3.6751e-40, -5.4562e-41,  6.1860e-40,
	 8.9003e-41,  5.5262e-40,  3.9537e-40,
	-2.1258e-42, -3.1069e-40, -7.6225e-41,
	-1.2220e-02, -8.6886e-02,  1.0714e-02,
	 1.1656e-02, -7.3635e-02,  5.9427e-02,
	 4.8518e-03,  1.3543e-01,  1.4668e-02,
	-1.7505e-02, -2.0691e-02, -1.4507e-02,
	 2.6157e-02,  7.4109e-02,  1.2822e-02,
	-1.9737e-02, -4.9281e-02,  8.5962e-03,
	 5.6236e-40,  2.4616e-40,  1.6384e-40,
	-3.9469e-40, -1.7094e-40,  1.9285e-40,
	-1.3634e-40, -1.5785e-40,  6.4184e-41,
	-1.2752e-02,  2.3150e-02, -5.3355e-03,
	-5.9667e-02, -3.9580e-01, -7.0033e-02,
	-2.2612e-02,  1.9176e-02,  1.0588e-02,
	 8.0027e-04,  3.2242e-01, -2.2566e-02,
	 8.7850e-03, -2.4025e-01,  4.6123e-02,
	-1.9038e-02, -8.5750e-03, -4.8153e-03,
	-1.3049e-03, -5.7771e-03,  9.6437e-03,
	 3.2477e-02,  2.4482e-01,  4.0580e-02,
	 1.3194e-02, -4.6602e-01, -6.6163e-02,
	-1.0647e-01,  7.3328e-02,  2.5871e-02,
	-7.0883e-02, -9.2725e-02, -1.5185e-02,
	 1.1804e-02,  1.7784e-03, -4.4099e-03,
	-4.9226e-40, -1.3081e-40, -3.5969e-40,
	 4.3539e-40, -2.9631e-40,  2.3531e-41,
	 5.6191e-40,  6.1545e-41, -1.1112e-40,
	-1.1880e-02, -3.1884e-02, -2.0850e-02,
	-6.8633e-03,  1.6422e-01,  1.0281e+00,
	 3.5887e-03,  2.1180e-01, -1.0094e-01,
	-1.5103e-02, -4.9074e-02, -1.7702e-02,
	 7.2119e-02,  3.3199e-02, -9.7082e-04,
	 5.5383e-02,  1.0343e-01,  2.5156e-02,
	 2.9049e-40, -1.6397e-40, -8.8848e-41,
	-6.2827e-40,  8.1281e-41,  5.2909e-40,
	-4.1132e-40,  1.5751e-40,  1.5400e-40,
	-7.3765e-02, -4.9723e-02,  4.9357e-02,
	-2.4207e-02, -1.0291e-01, -1.4001e-03,
	-1.2751e-02,  4.2805e-03,  1.8934e-03,
	 2.6862e-02,  1.1634e-01,  4.5666e-02,
	-4.7351e-03, -4.1593e-01,  3.6082e-02,
	 1.1446e-02, -5.2026e-03,  1.8672e-02,
	-7.0960e-04, -6.7877e-03,  9.6674e-03,
	-4.9952e-03,  8.8664e-02, -2.7707e-02,
	 8.5309e-02,  5.5513e-02, -7.6230e-02,
	 3.6354e-02,  9.7794e-02,  1.1687e-02,
	 2.6847e-02,  3.2565e-01, -8.7710e-03,
	-2.0372e-02, -1.9090e-02, -3.2566e-03,
	-5.5592e-40,  7.4408e-41,  3.5576e-40,
	 2.7758e-40,  4.5458e-41, -6.2347e-40,
	 9.9739e-41, -1.6078e-40, -5.2900e-40,
	 1.1500e-02, -3.0675e-01, -3.0079e-02,
	 1.5080e-02, -2.4292e-01,  1.2736e-01,
	-1.9513e-02, -1.9376e-02, -8.5960e-02,
	-1.0241e-01, -2.1312e-02, -3.1999e-02,
	-6.3598e-02,  1.5187e-01,  1.2279e-01,
	 1.5695e-03,  1.1376e-01,  5.2648e-03,
	 2.6415e-40,  3.0508e-40,  3.6407e-41,
	-1.4403e-40,  2.8942e-40, -1.0089e-40,
	 2.2362e-41,  1.9843e-40, -1.5509e-40,
	 1.3269e-01, -3.1031e-01, -4.4091e-02,
	 4.6385e-03,  2.1411e-02,  5.7141e-02,
	 2.0724e-02, -3.5406e-02,  2.5717e-03,
	-5.5922e-02,  7.1404e-01, -2.9852e-02,
	 1.3041e-02,  3.9373e-02, -2.4515e-01,
	 4.4278e-03,  2.1557e-02, -8.4940e-03,
	 1.3677e-02, -3.5183e-02,  1.2391e-02,
	-9.2405e-02,  2.9650e-01,  6.9695e-02,
	-3.3125e-02,  3.4700e-01,  1.4552e-01,
	 2.7357e-02,  5.2133e-01, -5.7571e-02,
	 2.7580e-02,  1.0381e-01,  1.3678e-02,
	 4.9260e-03, -4.4419e-02,  7.0651e-04,
	 2.9472e-40, -5.2892e-40, -3.6567e-40,
	 4.9403e-40, -6.2132e-40, -6.2920e-40,
	-1.5156e-40, -3.6134e-40,  5.2432e-40,
	-5.0427e-03, -2.8247e-03, -5.3734e-02,
	-1.5918e-02,  1.8325e-01, -1.7834e-01,
	-5.1774e-03,  8.0009e-02,  5.6296e-03,
	 3.1480e-02,  2.0665e-02,  2.7806e-04,
	 7.3085e-02,  7.7660e-01,  1.1979e-01,
	 1.9979e-02,  1.6629e-01,  2.3216e-02,
	-5.9701e-40,  9.5583e-41,  1.8231e-40,
	-3.3216e-40, -4.1253e-40, -3.3326e-40,
	 1.7131e-40,  2.9588e-40, -2.2520e-40,
	-1.3337e-01, -4.2777e-01, -1.3569e-01,
	 2.9915e-02, -2.7016e-01, -3.7454e-03,
	-1.3574e-02, -3.6298e-02, -1.6571e-02,
	 4.2530e-02, -4.2299e-02,  1.4320e-01,
	 1.4371e-02, -1.1289e-01, -3.8829e-02,
	 5.1689e-03,  1.5804e-02,  1.6125e-03,
	-3.4601e-03, -7.2087e-03, -5.5514e-04,
	 4.4568e-02,  1.3621e-01, -4.3811e-02,
	 1.1350e-02, -2.8417e-01,  3.1553e-02,
	-7.8854e-02, -2.0316e-01,  7.7746e-03,
	-1.1437e-02,  2.1557e-01, -1.9479e-02,
	-1.3511e-02, -2.0339e-02, -1.0276e-02
};

const static float4 biasL = { -0.0006,  0.0117,  0.0083,  0.0686 };


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

	return compressLinear(c1234, 0, 4.5);
}
