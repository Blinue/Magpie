// ACNet_L2_2
// ÒÆÖ²×Ô https://github.com/TianZerL/ACNetGLSL/blob/master/glsl/ACNet.glsl


cbuffer constants : register(b0) {
	int2 srcSize : packoffset(c0);
};


#define MAGPIE_INPUT_COUNT 2
#include "ACNet.hlsli"


const static float kernelsL[9 * 8 * 4] = {
	-4.9163e-03,  7.4149e-02,  6.3345e-02,
	-1.7888e-02, -9.1876e-02,  1.3728e-01,
	-9.6098e-02, -3.4814e-02, -1.0862e-02,
	 4.8031e-03,  2.5206e-01,  8.0316e-02,
	 1.5102e-01,  4.1236e-02,  2.2339e-01,
	 2.8500e-01,  1.5106e-01,  9.6321e-04,
	-6.0741e-02,  3.5759e-02, -1.8829e-01,
	-1.1295e-03, -6.2322e-02,  8.4974e-01,
	-3.9817e-02, -2.0666e-01,  2.2961e-01,
	 3.6857e-02, -2.0211e-02, -9.3342e-02,
	 2.0827e-02,  6.8874e-02, -6.0287e-02,
	-6.9724e-02,  1.4423e-01, -7.6017e-02,
	 1.4718e-02,  1.8990e-01,  1.1789e-01,
	-1.5018e-01, -2.3071e-01,  1.7511e-01,
	-7.7605e-02,  5.0621e-02, -1.0381e-01,
	 8.6845e-02, -1.2410e-01, -4.4669e-01,
	 2.7930e-02, -5.4713e-02, -7.7923e-02,
	 8.6000e-02, -2.6371e-02, -8.6541e-02,
	-1.1521e-01,  1.4389e-01,  5.0507e-02,
	-1.6618e-02, -2.5150e-01, -4.9759e-02,
	 7.7166e-02,  4.5033e-03, -5.4649e-02,
	 2.8548e-03, -2.8078e-03,  8.1129e-02,
	-4.5973e-02,  3.6740e-03,  2.0746e-01,
	-9.8191e-02,  1.2807e-01,  8.1950e-03,
	 1.4240e-01,  1.5104e-01,  6.9624e-02,
	 2.2309e-01,  2.5688e-01,  9.4766e-02,
	 6.2560e-02,  7.1347e-02,  4.1432e-02,
	-3.1829e-02,  1.5207e-01,  2.0575e-02,
	-1.2506e-01,  2.9274e-01,  9.4712e-02,
	-2.0520e-01,  4.9894e-04,  5.6171e-02,
	-4.1567e-03,  6.6753e-02, -1.5767e-01,
	 6.3768e-02,  8.3008e-02, -3.5639e-01,
	 4.4660e-02,  2.6996e-01, -6.4014e-02,
	 8.5475e-02,  1.7854e-02, -6.4079e-02,
	 1.8760e-01,  1.5285e-01, -3.5614e-02,
	 1.0747e-02, -3.1330e-01, -4.8664e-02,
	 7.2150e-02,  1.7570e-01,  1.6716e-01,
	 6.2431e-02,  2.3755e-01,  2.8554e-01,
	 3.5791e-02,  2.8185e-01,  1.5810e-01,
	-4.0886e-02,  1.8833e-02, -8.2903e-03,
	 1.3994e-02, -1.0846e-01,  3.5315e-02,
	-6.2674e-02,  6.2806e-02,  2.2168e-02,
	-3.6236e-01, -2.5326e-01,  5.6331e-02,
	 9.8762e-02,  3.8049e-01,  5.9885e-02,
	-3.0541e-02,  7.9855e-02, -5.8639e-02,
	 1.1104e-03,  1.7147e-02,  3.3115e-02,
	-3.3663e-02,  7.4615e-02,  6.4211e-02,
	-7.3441e-02, -1.5568e-01,  7.6546e-02,
	 6.1802e-02, -1.5300e-01, -1.8209e-02,
	-9.2786e-03,  1.6622e-01,  1.1354e-01,
	 9.5865e-03, -2.4226e-02, -1.4750e-03,
	-5.5294e-02, -1.1839e-01,  3.8867e-03,
	 1.7262e-01,  4.2743e-01,  6.8970e-02,
	-2.0232e-01, -1.4564e-01,  2.3025e-02,
	-2.6139e-03, -1.6907e-02,  1.1693e-01,
	-9.4871e-03,  3.8488e-02, -4.8351e-02,
	-9.2171e-02,  4.8227e-02,  9.7378e-02,
	-1.0292e-01, -1.2084e-01, -9.6676e-02,
	 1.8103e-02,  3.0658e-01, -7.7755e-02,
	-2.4362e-02, -1.9862e-01, -6.9665e-02,
	 8.2944e-03, -1.4680e-01, -1.7371e-02,
	-1.6534e-01,  2.5752e-01,  1.1129e-01,
	-9.4151e-02, -1.3225e-01,  1.5933e-01,
	 9.0723e-02,  5.5469e-02, -1.4091e-01,
	 8.3404e-02,  1.3741e-01, -3.5438e-02,
	 3.2681e-02,  2.8491e-02,  1.4278e-02,
	 2.3789e-01, -2.3687e-03, -5.3264e-03,
	-1.1161e-01,  1.9351e-02,  5.0832e-02,
	 8.2246e-03,  2.9892e-02, -3.7197e-02,
	 4.8236e-02,  1.6945e-01,  1.3673e-01,
	 1.1236e-01,  7.2318e-01, -4.1618e-02,
	 2.7494e-01,  1.0081e-01, -8.5399e-03,
	-5.6151e-02,  8.1212e-02, -7.5770e-02,
	 2.7872e-02,  9.4644e-02,  1.1175e-02,
	-6.1539e-02,  7.7395e-02, -3.2495e-02,
	-5.1640e-02,  2.1028e-03,  1.5825e-02,
	-1.1004e-01,  2.3153e-01, -6.1653e-02,
	-2.6497e-02,  5.9461e-01,  4.0865e-02,
	-1.9956e-02,  7.9328e-02, -1.7002e-02,
	-5.5930e-03,  5.2015e-02,  7.7945e-04,
	 1.0136e-02, -9.0111e-02, -1.1175e-01,
	-3.1781e-02,  1.4686e-01, -7.5718e-03,
	 1.1036e-02,  2.4618e-01,  8.5951e-02,
	 3.4775e-02, -1.2184e-01,  1.8010e-01,
	-3.6781e-02, -1.3912e-01, -4.9172e-02,
	 3.3064e-02,  5.0582e-01,  1.0713e-02,
	-1.2934e-02, -1.7697e-01, -1.4954e-01,
	 2.2229e-02, -5.8568e-03, -5.0186e-02,
	 1.9648e-02, -1.1302e-01,  1.5629e-02,
	-3.5015e-02,  9.5032e-02, -2.9677e-02,
	 9.5173e-02, -3.0330e-02, -3.7652e-02,
	-2.6097e-03,  7.4723e-01, -7.6234e-03,
	-3.8826e-02,  1.0191e-01,  3.6589e-03,
	-2.6503e-02, -1.1133e-01, -2.2029e-02,
	-1.9101e-01, -2.1108e-01, -7.4371e-02,
	-7.9349e-02, -1.0405e-01,  5.0315e-02
};

const static float4 biasL = { 0.0082, -0.0263, -0.0048, -0.0167 };


D2D_PS_ENTRY(main) {
	InitMagpieSampleInput();

	float2 leftTop1 = max(0, Coord(0).xy - Coord(0).zw);
	float2 rightBottom1 = min(maxCoord0.xy, Coord(0).xy + Coord(0).zw);

	// [tl, tc, tr]
	// [ml, mc, mr]
	// [bl, bc, br]
	float4 tl1 = uncompressLinear(SampleInput(0, leftTop1), 0, 2);
	float4 ml1 = uncompressLinear(SampleInput(0, float2(leftTop1.x, Coord(0).y)), 0, 2);
	float4 bl1 = uncompressLinear(SampleInput(0, float2(leftTop1.x, rightBottom1.y)), 0, 2);
	float4 tc1 = uncompressLinear(SampleInput(0, float2(Coord(0).x, leftTop1.y)), 0, 2);
	float4 mc1 = uncompressLinear(SampleInputCur(0), 0, 2);
	float4 bc1 = uncompressLinear(SampleInput(0, float2(Coord(0).x, rightBottom1.y)), 0, 2);
	float4 tr1 = uncompressLinear(SampleInput(0, float2(rightBottom1.x, leftTop1.y)), 0, 2);
	float4 mr1 = uncompressLinear(SampleInput(0, float2(rightBottom1.x, Coord(0).y)), 0, 2);
	float4 br1 = uncompressLinear(SampleInput(0, rightBottom1), 0, 2);

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
