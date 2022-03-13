// ACNet
// 移植自 https://github.com/TianZerL/ACNetGLSL/blob/master/glsl/ACNet.glsl


//!MAGPIE EFFECT
//!VERSION 2
//!OUTPUT_WIDTH INPUT_WIDTH * 2
//!OUTPUT_HEIGHT INPUT_HEIGHT * 2

//!TEXTURE
Texture2D INPUT;

//!TEXTURE
//!WIDTH INPUT_WIDTH
//!HEIGHT INPUT_HEIGHT
//!FORMAT R16G16B16A16_FLOAT
Texture2D tex1;

//!TEXTURE
//!WIDTH INPUT_WIDTH
//!HEIGHT INPUT_HEIGHT
//!FORMAT R16G16B16A16_FLOAT
Texture2D tex2;

//!TEXTURE
//!WIDTH INPUT_WIDTH
//!HEIGHT INPUT_HEIGHT
//!FORMAT R16G16B16A16_FLOAT
Texture2D tex3;

//!TEXTURE
//!WIDTH INPUT_WIDTH
//!HEIGHT INPUT_HEIGHT
//!FORMAT R16G16B16A16_FLOAT
Texture2D tex4;


//!SAMPLER
//!FILTER POINT
SamplerState sam;


//!COMMON

#pragma warning(disable: 4714)	// X4714: sum of temp registers and indexable temp registers times 256 threads exceeds the recommended total 16384.  Performance may be reduced

#define RELU(x) max(x, 0)


//!PASS 1
//!IN INPUT
//!OUT tex1, tex2
//!BLOCK_SIZE 16
//!NUM_THREADS 64

float GetLuma(float3 color) {
	return dot(float3(0.299f, 0.587f, 0.114f), color);
}

const static float kernelsL1A[9 * 4] = {
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

const static float4 biasL1A = { -0.7577, -0.0210, 0.0292, -0.0189 };

const static float kernelsL1B[9 * 4] = {
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

const static float4 biasL1B = { 0.0223,  0.0340,  0.0150, -0.0044 };


void Pass1(uint2 blockStart, uint3 threadId) {
	uint2 gxy = (Rmp8x8(threadId.x) << 1) + blockStart;
	uint2 inputSize = GetInputSize();
	if (gxy.x >= inputSize.x || gxy.y >= inputSize.y) {
		return;
	}
	float2 inputPt = GetInputPt();

	uint i, j;

	float src[4][4];
	[unroll]
	for (i = 0; i <= 2; i += 2) {
		[unroll]
		for (j = 0; j <= 2; j += 2) {
			float2 tpos = (gxy + uint2(i, j)) * inputPt;
			const float4 sr = INPUT.GatherRed(sam, tpos, 0);
			const float4 sg = INPUT.GatherGreen(sam, tpos, 0);
			const float4 sb = INPUT.GatherBlue(sam, tpos, 0);

			// w z
			// x y
			src[i][j] = GetLuma(float3(sr.w, sg.w, sb.w));
			src[i][j + 1] = GetLuma(float3(sr.x, sg.x, sb.x));
			src[i + 1][j] = GetLuma(float3(sr.z, sg.z, sb.z));
			src[i + 1][j + 1] = GetLuma(float3(sr.y, sg.y, sb.y));
		}
	}

	[unroll]
	for (i = 1; i <= 2; ++i) {
		[unroll]
		for (j = 1; j <= 2; ++j) {
			uint2 destPos = gxy + uint2(i - 1, j - 1);

			if (i != 1 && j != 1) {
				if (destPos.x >= inputSize.x || destPos.y >= inputSize.y) {
					continue;
				}
			}

			float4 target1 = RELU(float4(
				src[i - 1][j - 1] * kernelsL1A[0 * 9 + 0] + src[i][j - 1] * kernelsL1A[0 * 9 + 1] + src[i + 1][j - 1] * kernelsL1A[0 * 9 + 2] +
				src[i - 1][j] * kernelsL1A[0 * 9 + 3] + src[i][j] * kernelsL1A[0 * 9 + 4] + src[i + 1][j] * kernelsL1A[0 * 9 + 5] +
				src[i - 1][j + 1] * kernelsL1A[0 * 9 + 6] + src[i][j + 1] * kernelsL1A[0 * 9 + 7] + src[i + 1][j + 1] * kernelsL1A[0 * 9 + 8] + biasL1A.x,

				src[i - 1][j - 1] * kernelsL1A[1 * 9 + 0] + src[i][j - 1] * kernelsL1A[1 * 9 + 1] + src[i + 1][j - 1] * kernelsL1A[1 * 9 + 2] +
				src[i - 1][j] * kernelsL1A[1 * 9 + 3] + src[i][j] * kernelsL1A[1 * 9 + 4] + src[i + 1][j] * kernelsL1A[1 * 9 + 5] +
				src[i - 1][j + 1] * kernelsL1A[1 * 9 + 6] + src[i][j + 1] * kernelsL1A[1 * 9 + 7] + src[i + 1][j + 1] * kernelsL1A[1 * 9 + 8] + biasL1A.y,

				src[i - 1][j - 1] * kernelsL1A[2 * 9 + 0] + src[i][j - 1] * kernelsL1A[2 * 9 + 1] + src[i + 1][j - 1] * kernelsL1A[2 * 9 + 2] +
				src[i - 1][j] * kernelsL1A[2 * 9 + 3] + src[i][j] * kernelsL1A[2 * 9 + 4] + src[i + 1][j] * kernelsL1A[2 * 9 + 5] +
				src[i - 1][j + 1] * kernelsL1A[2 * 9 + 6] + src[i][j + 1] * kernelsL1A[2 * 9 + 7] + src[i + 1][j + 1] * kernelsL1A[2 * 9 + 8] + biasL1A.z,

				src[i - 1][j - 1] * kernelsL1A[3 * 9 + 0] + src[i][j - 1] * kernelsL1A[3 * 9 + 1] + src[i + 1][j - 1] * kernelsL1A[3 * 9 + 2] +
				src[i - 1][j] * kernelsL1A[3 * 9 + 3] + src[i][j] * kernelsL1A[3 * 9 + 4] + src[i + 1][j] * kernelsL1A[3 * 9 + 5] +
				src[i + 1][j + 1] * kernelsL1A[3 * 9 + 6] + src[i][j + 1] * kernelsL1A[3 * 9 + 7] + src[i + 1][j + 1] * kernelsL1A[3 * 9 + 8] + biasL1A.w
				));

			float4 target2 = RELU(float4(
				src[i - 1][j - 1] * kernelsL1B[0 * 9 + 0] + src[i][j - 1] * kernelsL1B[0 * 9 + 1] + src[i + 1][j - 1] * kernelsL1B[0 * 9 + 2] +
				src[i - 1][j] * kernelsL1B[0 * 9 + 3] + src[i][j] * kernelsL1B[0 * 9 + 4] + src[i + 1][j] * kernelsL1B[0 * 9 + 5] +
				src[i - 1][j + 1] * kernelsL1B[0 * 9 + 6] + src[i][j + 1] * kernelsL1B[0 * 9 + 7] + src[i + 1][j + 1] * kernelsL1B[0 * 9 + 8] + biasL1B.x,

				src[i - 1][j - 1] * kernelsL1B[1 * 9 + 0] + src[i][j - 1] * kernelsL1B[1 * 9 + 1] + src[i + 1][j - 1] * kernelsL1B[1 * 9 + 2] +
				src[i - 1][j] * kernelsL1B[1 * 9 + 3] + src[i][j] * kernelsL1B[1 * 9 + 4] + src[i + 1][j] * kernelsL1B[1 * 9 + 5] +
				src[i - 1][j + 1] * kernelsL1B[1 * 9 + 6] + src[i][j + 1] * kernelsL1B[1 * 9 + 7] + src[i + 1][j + 1] * kernelsL1B[1 * 9 + 8] + biasL1B.y,

				src[i - 1][j - 1] * kernelsL1B[2 * 9 + 0] + src[i][j - 1] * kernelsL1B[2 * 9 + 1] + src[i + 1][j - 1] * kernelsL1B[2 * 9 + 2] +
				src[i - 1][j] * kernelsL1B[2 * 9 + 3] + src[i][j] * kernelsL1B[2 * 9 + 4] + src[i + 1][j] * kernelsL1B[2 * 9 + 5] +
				src[i - 1][j + 1] * kernelsL1B[2 * 9 + 6] + src[i][j + 1] * kernelsL1B[2 * 9 + 7] + src[i + 1][j + 1] * kernelsL1B[2 * 9 + 8] + biasL1B.z,

				src[i - 1][j - 1] * kernelsL1B[3 * 9 + 0] + src[i][j - 1] * kernelsL1B[3 * 9 + 1] + src[i + 1][j - 1] * kernelsL1B[3 * 9 + 2] +
				src[i - 1][j] * kernelsL1B[3 * 9 + 3] + src[i][j] * kernelsL1B[3 * 9 + 4] + src[i + 1][j] * kernelsL1B[3 * 9 + 5] +
				src[i - 1][j + 1] * kernelsL1B[3 * 9 + 6] + src[i][j + 1] * kernelsL1B[3 * 9 + 7] + src[i + 1][j + 1] * kernelsL1B[3 * 9 + 8] + biasL1B.w
				));

			tex1[destPos] = target1;
			tex2[destPos] = target2;
		}
	}
}


//!PASS 2
//!IN tex1, tex2
//!OUT tex3, tex4
//!BLOCK_SIZE 8
//!NUM_THREADS 64

const static float kernelsLA[9 * 8 * 4] = {
	 2.0611e-01,  6.6865e-02, -9.9123e-02,
	 8.5279e-02, -4.5549e-02, -2.9491e-02,
	-1.0358e-01, -2.4844e-02, -8.1539e-03,
	-1.1308e-01, -6.4228e-02, -8.8081e-02,
	 2.7810e-02, -1.6054e-01, -1.1985e-01,
	-2.8679e-01, -1.7785e-02,  1.1559e-01,
	 2.1614e-02, -6.8870e-02, -2.4707e-01,
	 9.6867e-02, -1.6561e-01,  2.8281e-02,
	-8.2469e-02, -9.8554e-02, -1.7147e-02,
	 3.3710e-01,  9.2126e-02,  3.6880e-02,
	 5.7004e-02,  4.0175e-02,  1.6116e-01,
	 2.5629e-01,  5.1154e-01,  2.4119e-02,
	 1.9495e-02,  2.6940e-01, -1.4050e-01,
	 5.0325e-02, -4.5920e-02, -1.3586e-01,
	 5.9458e-02,  1.3860e-01, -2.1065e-01,
	-1.0744e-01, -1.5915e-01, -1.1528e-02,
	-1.1470e-01,  6.3455e-02, -5.5558e-02,
	-6.9920e-02, -3.0142e-02, -4.9059e-02,
	 3.6421e-01,  3.0252e-01, -1.3562e-01,
	 1.5238e-01, -1.9868e-01, -3.2644e-02,
	-4.2849e-02,  1.3677e-02,  7.3854e-02,
	 7.6609e-02, -1.0121e-01,  3.6319e-02,
	 9.3536e-02,  6.0386e-02,  1.0086e-01,
	-2.6630e-01,  2.5875e-02, -1.9225e-01,
	 4.0687e-02,  1.1005e-01,  9.9578e-03,
	 1.6939e-01,  5.0872e-01,  8.9876e-02,
	 6.9561e-02,  1.1910e-01, -1.8091e-02,
	-3.5739e-02, -7.5300e-02, -1.6788e-02,
	 3.0316e-02,  1.5942e-01, -9.0878e-02,
	-6.3737e-02,  2.6141e-02,  8.8040e-03,
	 3.4954e-03, -6.6707e-02,  1.4551e-01,
	 7.6258e-02,  1.4893e-01, -1.5255e-01,
	 6.2442e-02,  2.2166e-01,  7.5327e-02,
	 5.4785e-02, -1.4503e-02, -1.5188e-03,
	 1.6748e-01, -5.2731e-03, -1.9900e-02,
	 4.4786e-02, -1.0669e-01,  1.3192e-01,
	 1.9961e-02, -8.1015e-02, -3.2264e-02,
	 1.0544e-01,  1.8844e-01,  7.4274e-03,
	 6.6729e-02, -7.8318e-02,  3.0775e-02,
	-8.6109e-03,  7.4977e-02,  9.4079e-02,
	-1.2726e-01, -2.9664e-01,  7.8153e-03,
	-4.8413e-02, -1.8450e-01, -7.1065e-02,
	-8.7609e-02, -7.7192e-02,  5.0919e-02,
	-1.4021e-01,  3.5696e-01,  1.2079e-02,
	-2.0318e-02, -1.8827e-02,  3.9084e-02,
	-2.8654e-02, -6.4166e-02,  5.4889e-02,
	 8.2689e-02,  8.4463e-02,  2.2339e-02,
	 1.0805e-01, -1.2566e-01,  1.7109e-01,
	-6.1338e-02, -3.4043e-02,  4.0473e-02,
	 6.3821e-02,  1.7626e-01, -5.8112e-02,
	-9.5002e-02,  1.3327e-02,  1.2242e-01,
	 4.9008e-02, -4.3678e-02,  2.2362e-02,
	-7.7903e-02, -3.8252e-02, -5.2271e-02,
	-1.8884e-02, -1.2859e-01,  4.1172e-02,
	-3.1181e-02,  3.2348e-02, -4.9081e-02,
	-6.7966e-02, -2.4896e-02, -6.5323e-02,
	 8.0742e-02,  2.6093e-01, -2.4638e-01,
	-8.0881e-02, -2.9643e-02, -7.9627e-02,
	 1.4020e-01,  2.1575e-01,  8.1244e-03,
	 2.1561e-01, -2.9305e-01, -2.5535e-02,
	-8.5538e-02, -1.4456e-01, -7.5664e-02,
	-3.9921e-02,  4.0659e-02,  1.7812e-01,
	 1.1580e-01,  5.6628e-02,  9.0008e-02,
	-2.2384e-02, -1.9788e-02, -4.0547e-02,
	 1.0070e-01,  2.9581e-01,  1.9936e-01,
	-1.1957e-01, -8.6508e-02, -8.2543e-04,
	-5.2879e-02,  1.5486e-01,  1.0829e-02,
	 1.4716e-01,  3.4257e-01, -3.2058e-03,
	-2.1687e-02,  5.8641e-02, -6.3806e-02,
	-3.2607e-02,  7.3328e-02, -6.4738e-03,
	-1.0031e-01, -1.7698e-01, -9.4201e-02,
	-3.3644e-02, -3.5860e-01, -9.3200e-02,
	-7.4142e-02,  5.5001e-02,  4.3741e-02,
	-2.2447e-03,  1.1941e-01, -1.6135e-02,
	-1.4764e-02, -1.0194e-02,  3.2540e-02,
	-1.0588e-01, -2.3000e-01, -1.1557e-02,
	-9.0254e-02,  2.3352e-01, -1.3622e-01,
	-1.9256e-03, -5.3372e-02,  1.0314e-01,
	-2.0100e-02,  1.0700e-01,  1.6108e-01,
	 2.8422e-02,  2.7909e-01,  3.8342e-01,
	 1.4025e-02,  9.0965e-02,  2.0218e-01,
	 3.3562e-03,  7.6652e-02,  4.5974e-02,
	-1.3617e-02, -1.4014e-01, -1.9253e-02,
	 1.1020e-01, -1.9678e-01,  6.7123e-02,
	-3.3294e-02, -1.3006e-01, -1.0111e-01,
	 5.5813e-02,  2.1127e-01,  2.0248e-02,
	-9.6386e-04, -5.2497e-03,  1.1134e-01,
	 2.8910e-02,  1.2229e-01,  1.8439e-01,
	 1.6413e-02,  1.5870e-01, -1.1616e-01,
	-1.6032e-03, -6.8258e-03, -2.1883e-02,
	 1.2052e-01, -2.1982e-02, -1.3088e-01,
	 2.8664e-02, -5.0670e-02,  2.2927e-01,
	 2.0461e-02,  7.7250e-03, -2.6630e-02,
	-9.0406e-02, -1.4174e-01,  9.8969e-02,
	-6.6573e-02, -2.4425e-01, -3.5126e-02,
	 9.3859e-02,  1.9058e-01, -1.6569e-01
};

const static float4 biasLA = { 0.0272, -0.5743, -0.0333, -0.0334 };

const static float kernelsLB[9 * 8 * 4] = {
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

const static float4 biasLB = { 0.0082, -0.0263, -0.0048, -0.0167 };

void Pass2(uint2 blockStart, uint3 threadId) {
	uint2 gxy = Rmp8x8(threadId.x) + blockStart;
	uint2 inputSize = GetInputSize();
	if (gxy.x >= inputSize.x || gxy.y >= inputSize.y) {
		return;
	}

	float2 inputPt = GetInputPt();
	float2 pos = (gxy + 0.5f) * inputPt;

	// [tl, tc, tr]
	// [ml, mc, mr]
	// [bl, bc, br]
	float4 tl1 = tex1.SampleLevel(sam, pos + float2(-inputPt.x, -inputPt.y), 0);
	float4 ml1 = tex1.SampleLevel(sam, pos + float2(-inputPt.x, 0), 0);
	float4 bl1 = tex1.SampleLevel(sam, pos + float2(-inputPt.x, inputPt.y), 0);
	float4 tc1 = tex1.SampleLevel(sam, pos + float2(0, -inputPt.y), 0);
	float4 mc1 = tex1.SampleLevel(sam, pos, 0);
	float4 bc1 = tex1.SampleLevel(sam, pos + float2(0, inputPt.y), 0);
	float4 tr1 = tex1.SampleLevel(sam, pos + float2(inputPt.x, -inputPt.y), 0);
	float4 mr1 = tex1.SampleLevel(sam, pos + float2(inputPt.x, 0), 0);
	float4 br1 = tex1.SampleLevel(sam, pos + float2(inputPt.x, inputPt.y), 0);

	float4 tl2 = tex2.SampleLevel(sam, pos + float2(-inputPt.x, -inputPt.y), 0);
	float4 ml2 = tex2.SampleLevel(sam, pos + float2(-inputPt.x, 0), 0);
	float4 bl2 = tex2.SampleLevel(sam, pos + float2(-inputPt.x, inputPt.y), 0);
	float4 tc2 = tex2.SampleLevel(sam, pos + float2(0, -inputPt.y), 0);
	float4 mc2 = tex2.SampleLevel(sam, pos, 0);
	float4 bc2 = tex2.SampleLevel(sam, pos + float2(0, inputPt.y), 0);
	float4 tr2 = tex2.SampleLevel(sam, pos + float2(inputPt.x, -inputPt.y), 0);
	float4 mr2 = tex2.SampleLevel(sam, pos + float2(inputPt.x, 0), 0);
	float4 br2 = tex2.SampleLevel(sam, pos + float2(inputPt.x, inputPt.y), 0);

	float4 target1 = RELU(float4(
		tl1.x * kernelsLA[0 * 72 + 0 * 9 + 0] + tc1.x * kernelsLA[0 * 72 + 0 * 9 + 1] + tr1.x * kernelsLA[0 * 72 + 0 * 9 + 2] +
		ml1.x * kernelsLA[0 * 72 + 0 * 9 + 3] + mc1.x * kernelsLA[0 * 72 + 0 * 9 + 4] + mr1.x * kernelsLA[0 * 72 + 0 * 9 + 5] +
		bl1.x * kernelsLA[0 * 72 + 0 * 9 + 6] + bc1.x * kernelsLA[0 * 72 + 0 * 9 + 7] + br1.x * kernelsLA[0 * 72 + 0 * 9 + 8] +

		tl1.y * kernelsLA[0 * 72 + 1 * 9 + 0] + tc1.y * kernelsLA[0 * 72 + 1 * 9 + 1] + tr1.y * kernelsLA[0 * 72 + 1 * 9 + 2] +
		ml1.y * kernelsLA[0 * 72 + 1 * 9 + 3] + mc1.y * kernelsLA[0 * 72 + 1 * 9 + 4] + mr1.y * kernelsLA[0 * 72 + 1 * 9 + 5] +
		bl1.y * kernelsLA[0 * 72 + 1 * 9 + 6] + bc1.y * kernelsLA[0 * 72 + 1 * 9 + 7] + br1.y * kernelsLA[0 * 72 + 1 * 9 + 8] +

		tl1.z * kernelsLA[0 * 72 + 2 * 9 + 0] + tc1.z * kernelsLA[0 * 72 + 2 * 9 + 1] + tr1.z * kernelsLA[0 * 72 + 2 * 9 + 2] +
		ml1.z * kernelsLA[0 * 72 + 2 * 9 + 3] + mc1.z * kernelsLA[0 * 72 + 2 * 9 + 4] + mr1.z * kernelsLA[0 * 72 + 2 * 9 + 5] +
		bl1.z * kernelsLA[0 * 72 + 2 * 9 + 6] + bc1.z * kernelsLA[0 * 72 + 2 * 9 + 7] + br1.z * kernelsLA[0 * 72 + 2 * 9 + 8] +

		tl1.w * kernelsLA[0 * 72 + 3 * 9 + 0] + tc1.w * kernelsLA[0 * 72 + 3 * 9 + 1] + tr1.w * kernelsLA[0 * 72 + 3 * 9 + 2] +
		ml1.w * kernelsLA[0 * 72 + 3 * 9 + 3] + mc1.w * kernelsLA[0 * 72 + 3 * 9 + 4] + mr1.w * kernelsLA[0 * 72 + 3 * 9 + 5] +
		bl1.w * kernelsLA[0 * 72 + 3 * 9 + 6] + bc1.w * kernelsLA[0 * 72 + 3 * 9 + 7] + br1.w * kernelsLA[0 * 72 + 3 * 9 + 8] +

		tl2.x * kernelsLA[0 * 72 + 4 * 9 + 0] + tc2.x * kernelsLA[0 * 72 + 4 * 9 + 1] + tr2.x * kernelsLA[0 * 72 + 4 * 9 + 2] +
		ml2.x * kernelsLA[0 * 72 + 4 * 9 + 3] + mc2.x * kernelsLA[0 * 72 + 4 * 9 + 4] + mr2.x * kernelsLA[0 * 72 + 4 * 9 + 5] +
		bl2.x * kernelsLA[0 * 72 + 4 * 9 + 6] + bc2.x * kernelsLA[0 * 72 + 4 * 9 + 7] + br2.x * kernelsLA[0 * 72 + 4 * 9 + 8] +

		tl2.y * kernelsLA[0 * 72 + 5 * 9 + 0] + tc2.y * kernelsLA[0 * 72 + 5 * 9 + 1] + tr2.y * kernelsLA[0 * 72 + 5 * 9 + 2] +
		ml2.y * kernelsLA[0 * 72 + 5 * 9 + 3] + mc2.y * kernelsLA[0 * 72 + 5 * 9 + 4] + mr2.y * kernelsLA[0 * 72 + 5 * 9 + 5] +
		bl2.y * kernelsLA[0 * 72 + 5 * 9 + 6] + bc2.y * kernelsLA[0 * 72 + 5 * 9 + 7] + br2.y * kernelsLA[0 * 72 + 5 * 9 + 8] +

		tl2.z * kernelsLA[0 * 72 + 6 * 9 + 0] + tc2.z * kernelsLA[0 * 72 + 6 * 9 + 1] + tr2.z * kernelsLA[0 * 72 + 6 * 9 + 2] +
		ml2.z * kernelsLA[0 * 72 + 6 * 9 + 3] + mc2.z * kernelsLA[0 * 72 + 6 * 9 + 4] + mr2.z * kernelsLA[0 * 72 + 6 * 9 + 5] +
		bl2.z * kernelsLA[0 * 72 + 6 * 9 + 6] + bc2.z * kernelsLA[0 * 72 + 6 * 9 + 7] + br2.z * kernelsLA[0 * 72 + 6 * 9 + 8] +

		tl2.w * kernelsLA[0 * 72 + 7 * 9 + 0] + tc2.w * kernelsLA[0 * 72 + 7 * 9 + 1] + tr2.w * kernelsLA[0 * 72 + 7 * 9 + 2] +
		ml2.w * kernelsLA[0 * 72 + 7 * 9 + 3] + mc2.w * kernelsLA[0 * 72 + 7 * 9 + 4] + mr2.w * kernelsLA[0 * 72 + 7 * 9 + 5] +
		bl2.w * kernelsLA[0 * 72 + 7 * 9 + 6] + bc2.w * kernelsLA[0 * 72 + 7 * 9 + 7] + br2.w * kernelsLA[0 * 72 + 7 * 9 + 8] + biasLA.x
		,
		tl1.x * kernelsLA[1 * 72 + 0 * 9 + 0] + tc1.x * kernelsLA[1 * 72 + 0 * 9 + 1] + tr1.x * kernelsLA[1 * 72 + 0 * 9 + 2] +
		ml1.x * kernelsLA[1 * 72 + 0 * 9 + 3] + mc1.x * kernelsLA[1 * 72 + 0 * 9 + 4] + mr1.x * kernelsLA[1 * 72 + 0 * 9 + 5] +
		bl1.x * kernelsLA[1 * 72 + 0 * 9 + 6] + bc1.x * kernelsLA[1 * 72 + 0 * 9 + 7] + br1.x * kernelsLA[1 * 72 + 0 * 9 + 8] +

		tl1.y * kernelsLA[1 * 72 + 1 * 9 + 0] + tc1.y * kernelsLA[1 * 72 + 1 * 9 + 1] + tr1.y * kernelsLA[1 * 72 + 1 * 9 + 2] +
		ml1.y * kernelsLA[1 * 72 + 1 * 9 + 3] + mc1.y * kernelsLA[1 * 72 + 1 * 9 + 4] + mr1.y * kernelsLA[1 * 72 + 1 * 9 + 5] +
		bl1.y * kernelsLA[1 * 72 + 1 * 9 + 6] + bc1.y * kernelsLA[1 * 72 + 1 * 9 + 7] + br1.y * kernelsLA[1 * 72 + 1 * 9 + 8] +

		tl1.z * kernelsLA[1 * 72 + 2 * 9 + 0] + tc1.z * kernelsLA[1 * 72 + 2 * 9 + 1] + tr1.z * kernelsLA[1 * 72 + 2 * 9 + 2] +
		ml1.z * kernelsLA[1 * 72 + 2 * 9 + 3] + mc1.z * kernelsLA[1 * 72 + 2 * 9 + 4] + mr1.z * kernelsLA[1 * 72 + 2 * 9 + 5] +
		bl1.z * kernelsLA[1 * 72 + 2 * 9 + 6] + bc1.z * kernelsLA[1 * 72 + 2 * 9 + 7] + br1.z * kernelsLA[1 * 72 + 2 * 9 + 8] +

		tl1.w * kernelsLA[1 * 72 + 3 * 9 + 0] + tc1.w * kernelsLA[1 * 72 + 3 * 9 + 1] + tr1.w * kernelsLA[1 * 72 + 3 * 9 + 2] +
		ml1.w * kernelsLA[1 * 72 + 3 * 9 + 3] + mc1.w * kernelsLA[1 * 72 + 3 * 9 + 4] + mr1.w * kernelsLA[1 * 72 + 3 * 9 + 5] +
		bl1.w * kernelsLA[1 * 72 + 3 * 9 + 6] + bc1.w * kernelsLA[1 * 72 + 3 * 9 + 7] + br1.w * kernelsLA[1 * 72 + 3 * 9 + 8] +

		tl2.x * kernelsLA[1 * 72 + 4 * 9 + 0] + tc2.x * kernelsLA[1 * 72 + 4 * 9 + 1] + tr2.x * kernelsLA[1 * 72 + 4 * 9 + 2] +
		ml2.x * kernelsLA[1 * 72 + 4 * 9 + 3] + mc2.x * kernelsLA[1 * 72 + 4 * 9 + 4] + mr2.x * kernelsLA[1 * 72 + 4 * 9 + 5] +
		bl2.x * kernelsLA[1 * 72 + 4 * 9 + 6] + bc2.x * kernelsLA[1 * 72 + 4 * 9 + 7] + br2.x * kernelsLA[1 * 72 + 4 * 9 + 8] +

		tl2.y * kernelsLA[1 * 72 + 5 * 9 + 0] + tc2.y * kernelsLA[1 * 72 + 5 * 9 + 1] + tr2.y * kernelsLA[1 * 72 + 5 * 9 + 2] +
		ml2.y * kernelsLA[1 * 72 + 5 * 9 + 3] + mc2.y * kernelsLA[1 * 72 + 5 * 9 + 4] + mr2.y * kernelsLA[1 * 72 + 5 * 9 + 5] +
		bl2.y * kernelsLA[1 * 72 + 5 * 9 + 6] + bc2.y * kernelsLA[1 * 72 + 5 * 9 + 7] + br2.y * kernelsLA[1 * 72 + 5 * 9 + 8] +

		tl2.z * kernelsLA[1 * 72 + 6 * 9 + 0] + tc2.z * kernelsLA[1 * 72 + 6 * 9 + 1] + tr2.z * kernelsLA[1 * 72 + 6 * 9 + 2] +
		ml2.z * kernelsLA[1 * 72 + 6 * 9 + 3] + mc2.z * kernelsLA[1 * 72 + 6 * 9 + 4] + mr2.z * kernelsLA[1 * 72 + 6 * 9 + 5] +
		bl2.z * kernelsLA[1 * 72 + 6 * 9 + 6] + bc2.z * kernelsLA[1 * 72 + 6 * 9 + 7] + br2.z * kernelsLA[1 * 72 + 6 * 9 + 8] +

		tl2.w * kernelsLA[1 * 72 + 7 * 9 + 0] + tc2.w * kernelsLA[1 * 72 + 7 * 9 + 1] + tr2.w * kernelsLA[1 * 72 + 7 * 9 + 2] +
		ml2.w * kernelsLA[1 * 72 + 7 * 9 + 3] + mc2.w * kernelsLA[1 * 72 + 7 * 9 + 4] + mr2.w * kernelsLA[1 * 72 + 7 * 9 + 5] +
		bl2.w * kernelsLA[1 * 72 + 7 * 9 + 6] + bc2.w * kernelsLA[1 * 72 + 7 * 9 + 7] + br2.w * kernelsLA[1 * 72 + 7 * 9 + 8] + biasLA.y
		,
		tl1.x * kernelsLA[2 * 72 + 0 * 9 + 0] + tc1.x * kernelsLA[2 * 72 + 0 * 9 + 1] + tr1.x * kernelsLA[2 * 72 + 0 * 9 + 2] +
		ml1.x * kernelsLA[2 * 72 + 0 * 9 + 3] + mc1.x * kernelsLA[2 * 72 + 0 * 9 + 4] + mr1.x * kernelsLA[2 * 72 + 0 * 9 + 5] +
		bl1.x * kernelsLA[2 * 72 + 0 * 9 + 6] + bc1.x * kernelsLA[2 * 72 + 0 * 9 + 7] + br1.x * kernelsLA[2 * 72 + 0 * 9 + 8] +

		tl1.y * kernelsLA[2 * 72 + 1 * 9 + 0] + tc1.y * kernelsLA[2 * 72 + 1 * 9 + 1] + tr1.y * kernelsLA[2 * 72 + 1 * 9 + 2] +
		ml1.y * kernelsLA[2 * 72 + 1 * 9 + 3] + mc1.y * kernelsLA[2 * 72 + 1 * 9 + 4] + mr1.y * kernelsLA[2 * 72 + 1 * 9 + 5] +
		bl1.y * kernelsLA[2 * 72 + 1 * 9 + 6] + bc1.y * kernelsLA[2 * 72 + 1 * 9 + 7] + br1.y * kernelsLA[2 * 72 + 1 * 9 + 8] +

		tl1.z * kernelsLA[2 * 72 + 2 * 9 + 0] + tc1.z * kernelsLA[2 * 72 + 2 * 9 + 1] + tr1.z * kernelsLA[2 * 72 + 2 * 9 + 2] +
		ml1.z * kernelsLA[2 * 72 + 2 * 9 + 3] + mc1.z * kernelsLA[2 * 72 + 2 * 9 + 4] + mr1.z * kernelsLA[2 * 72 + 2 * 9 + 5] +
		bl1.z * kernelsLA[2 * 72 + 2 * 9 + 6] + bc1.z * kernelsLA[2 * 72 + 2 * 9 + 7] + br1.z * kernelsLA[2 * 72 + 2 * 9 + 8] +

		tl1.w * kernelsLA[2 * 72 + 3 * 9 + 0] + tc1.w * kernelsLA[2 * 72 + 3 * 9 + 1] + tr1.w * kernelsLA[2 * 72 + 3 * 9 + 2] +
		ml1.w * kernelsLA[2 * 72 + 3 * 9 + 3] + mc1.w * kernelsLA[2 * 72 + 3 * 9 + 4] + mr1.w * kernelsLA[2 * 72 + 3 * 9 + 5] +
		bl1.w * kernelsLA[2 * 72 + 3 * 9 + 6] + bc1.w * kernelsLA[2 * 72 + 3 * 9 + 7] + br1.w * kernelsLA[2 * 72 + 3 * 9 + 8] +

		tl2.x * kernelsLA[2 * 72 + 4 * 9 + 0] + tc2.x * kernelsLA[2 * 72 + 4 * 9 + 1] + tr2.x * kernelsLA[2 * 72 + 4 * 9 + 2] +
		ml2.x * kernelsLA[2 * 72 + 4 * 9 + 3] + mc2.x * kernelsLA[2 * 72 + 4 * 9 + 4] + mr2.x * kernelsLA[2 * 72 + 4 * 9 + 5] +
		bl2.x * kernelsLA[2 * 72 + 4 * 9 + 6] + bc2.x * kernelsLA[2 * 72 + 4 * 9 + 7] + br2.x * kernelsLA[2 * 72 + 4 * 9 + 8] +

		tl2.y * kernelsLA[2 * 72 + 5 * 9 + 0] + tc2.y * kernelsLA[2 * 72 + 5 * 9 + 1] + tr2.y * kernelsLA[2 * 72 + 5 * 9 + 2] +
		ml2.y * kernelsLA[2 * 72 + 5 * 9 + 3] + mc2.y * kernelsLA[2 * 72 + 5 * 9 + 4] + mr2.y * kernelsLA[2 * 72 + 5 * 9 + 5] +
		bl2.y * kernelsLA[2 * 72 + 5 * 9 + 6] + bc2.y * kernelsLA[2 * 72 + 5 * 9 + 7] + br2.y * kernelsLA[2 * 72 + 5 * 9 + 8] +

		tl2.z * kernelsLA[2 * 72 + 6 * 9 + 0] + tc2.z * kernelsLA[2 * 72 + 6 * 9 + 1] + tr2.z * kernelsLA[2 * 72 + 6 * 9 + 2] +
		ml2.z * kernelsLA[2 * 72 + 6 * 9 + 3] + mc2.z * kernelsLA[2 * 72 + 6 * 9 + 4] + mr2.z * kernelsLA[2 * 72 + 6 * 9 + 5] +
		bl2.z * kernelsLA[2 * 72 + 6 * 9 + 6] + bc2.z * kernelsLA[2 * 72 + 6 * 9 + 7] + br2.z * kernelsLA[2 * 72 + 6 * 9 + 8] +

		tl2.w * kernelsLA[2 * 72 + 7 * 9 + 0] + tc2.w * kernelsLA[2 * 72 + 7 * 9 + 1] + tr2.w * kernelsLA[2 * 72 + 7 * 9 + 2] +
		ml2.w * kernelsLA[2 * 72 + 7 * 9 + 3] + mc2.w * kernelsLA[2 * 72 + 7 * 9 + 4] + mr2.w * kernelsLA[2 * 72 + 7 * 9 + 5] +
		bl2.w * kernelsLA[2 * 72 + 7 * 9 + 6] + bc2.w * kernelsLA[2 * 72 + 7 * 9 + 7] + br2.w * kernelsLA[2 * 72 + 7 * 9 + 8] + biasLA.z
		,
		tl1.x * kernelsLA[3 * 72 + 0 * 9 + 0] + tc1.x * kernelsLA[3 * 72 + 0 * 9 + 1] + tr1.x * kernelsLA[3 * 72 + 0 * 9 + 2] +
		ml1.x * kernelsLA[3 * 72 + 0 * 9 + 3] + mc1.x * kernelsLA[3 * 72 + 0 * 9 + 4] + mr1.x * kernelsLA[3 * 72 + 0 * 9 + 5] +
		bl1.x * kernelsLA[3 * 72 + 0 * 9 + 6] + bc1.x * kernelsLA[3 * 72 + 0 * 9 + 7] + br1.x * kernelsLA[3 * 72 + 0 * 9 + 8] +

		tl1.y * kernelsLA[3 * 72 + 1 * 9 + 0] + tc1.y * kernelsLA[3 * 72 + 1 * 9 + 1] + tr1.y * kernelsLA[3 * 72 + 1 * 9 + 2] +
		ml1.y * kernelsLA[3 * 72 + 1 * 9 + 3] + mc1.y * kernelsLA[3 * 72 + 1 * 9 + 4] + mr1.y * kernelsLA[3 * 72 + 1 * 9 + 5] +
		bl1.y * kernelsLA[3 * 72 + 1 * 9 + 6] + bc1.y * kernelsLA[3 * 72 + 1 * 9 + 7] + br1.y * kernelsLA[3 * 72 + 1 * 9 + 8] +

		tl1.z * kernelsLA[3 * 72 + 2 * 9 + 0] + tc1.z * kernelsLA[3 * 72 + 2 * 9 + 1] + tr1.z * kernelsLA[3 * 72 + 2 * 9 + 2] +
		ml1.z * kernelsLA[3 * 72 + 2 * 9 + 3] + mc1.z * kernelsLA[3 * 72 + 2 * 9 + 4] + mr1.z * kernelsLA[3 * 72 + 2 * 9 + 5] +
		bl1.z * kernelsLA[3 * 72 + 2 * 9 + 6] + bc1.z * kernelsLA[3 * 72 + 2 * 9 + 7] + br1.z * kernelsLA[3 * 72 + 2 * 9 + 8] +

		tl1.w * kernelsLA[3 * 72 + 3 * 9 + 0] + tc1.w * kernelsLA[3 * 72 + 3 * 9 + 1] + tr1.w * kernelsLA[3 * 72 + 3 * 9 + 2] +
		ml1.w * kernelsLA[3 * 72 + 3 * 9 + 3] + mc1.w * kernelsLA[3 * 72 + 3 * 9 + 4] + mr1.w * kernelsLA[3 * 72 + 3 * 9 + 5] +
		bl1.w * kernelsLA[3 * 72 + 3 * 9 + 6] + bc1.w * kernelsLA[3 * 72 + 3 * 9 + 7] + br1.w * kernelsLA[3 * 72 + 3 * 9 + 8] +

		tl2.x * kernelsLA[3 * 72 + 4 * 9 + 0] + tc2.x * kernelsLA[3 * 72 + 4 * 9 + 1] + tr2.x * kernelsLA[3 * 72 + 4 * 9 + 2] +
		ml2.x * kernelsLA[3 * 72 + 4 * 9 + 3] + mc2.x * kernelsLA[3 * 72 + 4 * 9 + 4] + mr2.x * kernelsLA[3 * 72 + 4 * 9 + 5] +
		bl2.x * kernelsLA[3 * 72 + 4 * 9 + 6] + bc2.x * kernelsLA[3 * 72 + 4 * 9 + 7] + br2.x * kernelsLA[3 * 72 + 4 * 9 + 8] +

		tl2.y * kernelsLA[3 * 72 + 5 * 9 + 0] + tc2.y * kernelsLA[3 * 72 + 5 * 9 + 1] + tr2.y * kernelsLA[3 * 72 + 5 * 9 + 2] +
		ml2.y * kernelsLA[3 * 72 + 5 * 9 + 3] + mc2.y * kernelsLA[3 * 72 + 5 * 9 + 4] + mr2.y * kernelsLA[3 * 72 + 5 * 9 + 5] +
		bl2.y * kernelsLA[3 * 72 + 5 * 9 + 6] + bc2.y * kernelsLA[3 * 72 + 5 * 9 + 7] + br2.y * kernelsLA[3 * 72 + 5 * 9 + 8] +

		tl2.z * kernelsLA[3 * 72 + 6 * 9 + 0] + tc2.z * kernelsLA[3 * 72 + 6 * 9 + 1] + tr2.z * kernelsLA[3 * 72 + 6 * 9 + 2] +
		ml2.z * kernelsLA[3 * 72 + 6 * 9 + 3] + mc2.z * kernelsLA[3 * 72 + 6 * 9 + 4] + mr2.z * kernelsLA[3 * 72 + 6 * 9 + 5] +
		bl2.z * kernelsLA[3 * 72 + 6 * 9 + 6] + bc2.z * kernelsLA[3 * 72 + 6 * 9 + 7] + br2.z * kernelsLA[3 * 72 + 6 * 9 + 8] +

		tl2.w * kernelsLA[3 * 72 + 7 * 9 + 0] + tc2.w * kernelsLA[3 * 72 + 7 * 9 + 1] + tr2.w * kernelsLA[3 * 72 + 7 * 9 + 2] +
		ml2.w * kernelsLA[3 * 72 + 7 * 9 + 3] + mc2.w * kernelsLA[3 * 72 + 7 * 9 + 4] + mr2.w * kernelsLA[3 * 72 + 7 * 9 + 5] +
		bl2.w * kernelsLA[3 * 72 + 7 * 9 + 6] + bc2.w * kernelsLA[3 * 72 + 7 * 9 + 7] + br2.w * kernelsLA[3 * 72 + 7 * 9 + 8] + biasLA.w
	));

	float4 target2 = RELU(float4(
		tl1.x * kernelsLB[0 * 72 + 0 * 9 + 0] + tc1.x * kernelsLB[0 * 72 + 0 * 9 + 1] + tr1.x * kernelsLB[0 * 72 + 0 * 9 + 2] +
		ml1.x * kernelsLB[0 * 72 + 0 * 9 + 3] + mc1.x * kernelsLB[0 * 72 + 0 * 9 + 4] + mr1.x * kernelsLB[0 * 72 + 0 * 9 + 5] +
		bl1.x * kernelsLB[0 * 72 + 0 * 9 + 6] + bc1.x * kernelsLB[0 * 72 + 0 * 9 + 7] + br1.x * kernelsLB[0 * 72 + 0 * 9 + 8] +

		tl1.y * kernelsLB[0 * 72 + 1 * 9 + 0] + tc1.y * kernelsLB[0 * 72 + 1 * 9 + 1] + tr1.y * kernelsLB[0 * 72 + 1 * 9 + 2] +
		ml1.y * kernelsLB[0 * 72 + 1 * 9 + 3] + mc1.y * kernelsLB[0 * 72 + 1 * 9 + 4] + mr1.y * kernelsLB[0 * 72 + 1 * 9 + 5] +
		bl1.y * kernelsLB[0 * 72 + 1 * 9 + 6] + bc1.y * kernelsLB[0 * 72 + 1 * 9 + 7] + br1.y * kernelsLB[0 * 72 + 1 * 9 + 8] +

		tl1.z * kernelsLB[0 * 72 + 2 * 9 + 0] + tc1.z * kernelsLB[0 * 72 + 2 * 9 + 1] + tr1.z * kernelsLB[0 * 72 + 2 * 9 + 2] +
		ml1.z * kernelsLB[0 * 72 + 2 * 9 + 3] + mc1.z * kernelsLB[0 * 72 + 2 * 9 + 4] + mr1.z * kernelsLB[0 * 72 + 2 * 9 + 5] +
		bl1.z * kernelsLB[0 * 72 + 2 * 9 + 6] + bc1.z * kernelsLB[0 * 72 + 2 * 9 + 7] + br1.z * kernelsLB[0 * 72 + 2 * 9 + 8] +

		tl1.w * kernelsLB[0 * 72 + 3 * 9 + 0] + tc1.w * kernelsLB[0 * 72 + 3 * 9 + 1] + tr1.w * kernelsLB[0 * 72 + 3 * 9 + 2] +
		ml1.w * kernelsLB[0 * 72 + 3 * 9 + 3] + mc1.w * kernelsLB[0 * 72 + 3 * 9 + 4] + mr1.w * kernelsLB[0 * 72 + 3 * 9 + 5] +
		bl1.w * kernelsLB[0 * 72 + 3 * 9 + 6] + bc1.w * kernelsLB[0 * 72 + 3 * 9 + 7] + br1.w * kernelsLB[0 * 72 + 3 * 9 + 8] +

		tl2.x * kernelsLB[0 * 72 + 4 * 9 + 0] + tc2.x * kernelsLB[0 * 72 + 4 * 9 + 1] + tr2.x * kernelsLB[0 * 72 + 4 * 9 + 2] +
		ml2.x * kernelsLB[0 * 72 + 4 * 9 + 3] + mc2.x * kernelsLB[0 * 72 + 4 * 9 + 4] + mr2.x * kernelsLB[0 * 72 + 4 * 9 + 5] +
		bl2.x * kernelsLB[0 * 72 + 4 * 9 + 6] + bc2.x * kernelsLB[0 * 72 + 4 * 9 + 7] + br2.x * kernelsLB[0 * 72 + 4 * 9 + 8] +

		tl2.y * kernelsLB[0 * 72 + 5 * 9 + 0] + tc2.y * kernelsLB[0 * 72 + 5 * 9 + 1] + tr2.y * kernelsLB[0 * 72 + 5 * 9 + 2] +
		ml2.y * kernelsLB[0 * 72 + 5 * 9 + 3] + mc2.y * kernelsLB[0 * 72 + 5 * 9 + 4] + mr2.y * kernelsLB[0 * 72 + 5 * 9 + 5] +
		bl2.y * kernelsLB[0 * 72 + 5 * 9 + 6] + bc2.y * kernelsLB[0 * 72 + 5 * 9 + 7] + br2.y * kernelsLB[0 * 72 + 5 * 9 + 8] +

		tl2.z * kernelsLB[0 * 72 + 6 * 9 + 0] + tc2.z * kernelsLB[0 * 72 + 6 * 9 + 1] + tr2.z * kernelsLB[0 * 72 + 6 * 9 + 2] +
		ml2.z * kernelsLB[0 * 72 + 6 * 9 + 3] + mc2.z * kernelsLB[0 * 72 + 6 * 9 + 4] + mr2.z * kernelsLB[0 * 72 + 6 * 9 + 5] +
		bl2.z * kernelsLB[0 * 72 + 6 * 9 + 6] + bc2.z * kernelsLB[0 * 72 + 6 * 9 + 7] + br2.z * kernelsLB[0 * 72 + 6 * 9 + 8] +

		tl2.w * kernelsLB[0 * 72 + 7 * 9 + 0] + tc2.w * kernelsLB[0 * 72 + 7 * 9 + 1] + tr2.w * kernelsLB[0 * 72 + 7 * 9 + 2] +
		ml2.w * kernelsLB[0 * 72 + 7 * 9 + 3] + mc2.w * kernelsLB[0 * 72 + 7 * 9 + 4] + mr2.w * kernelsLB[0 * 72 + 7 * 9 + 5] +
		bl2.w * kernelsLB[0 * 72 + 7 * 9 + 6] + bc2.w * kernelsLB[0 * 72 + 7 * 9 + 7] + br2.w * kernelsLB[0 * 72 + 7 * 9 + 8] + biasLB.x
		,
		tl1.x * kernelsLB[1 * 72 + 0 * 9 + 0] + tc1.x * kernelsLB[1 * 72 + 0 * 9 + 1] + tr1.x * kernelsLB[1 * 72 + 0 * 9 + 2] +
		ml1.x * kernelsLB[1 * 72 + 0 * 9 + 3] + mc1.x * kernelsLB[1 * 72 + 0 * 9 + 4] + mr1.x * kernelsLB[1 * 72 + 0 * 9 + 5] +
		bl1.x * kernelsLB[1 * 72 + 0 * 9 + 6] + bc1.x * kernelsLB[1 * 72 + 0 * 9 + 7] + br1.x * kernelsLB[1 * 72 + 0 * 9 + 8] +

		tl1.y * kernelsLB[1 * 72 + 1 * 9 + 0] + tc1.y * kernelsLB[1 * 72 + 1 * 9 + 1] + tr1.y * kernelsLB[1 * 72 + 1 * 9 + 2] +
		ml1.y * kernelsLB[1 * 72 + 1 * 9 + 3] + mc1.y * kernelsLB[1 * 72 + 1 * 9 + 4] + mr1.y * kernelsLB[1 * 72 + 1 * 9 + 5] +
		bl1.y * kernelsLB[1 * 72 + 1 * 9 + 6] + bc1.y * kernelsLB[1 * 72 + 1 * 9 + 7] + br1.y * kernelsLB[1 * 72 + 1 * 9 + 8] +

		tl1.z * kernelsLB[1 * 72 + 2 * 9 + 0] + tc1.z * kernelsLB[1 * 72 + 2 * 9 + 1] + tr1.z * kernelsLB[1 * 72 + 2 * 9 + 2] +
		ml1.z * kernelsLB[1 * 72 + 2 * 9 + 3] + mc1.z * kernelsLB[1 * 72 + 2 * 9 + 4] + mr1.z * kernelsLB[1 * 72 + 2 * 9 + 5] +
		bl1.z * kernelsLB[1 * 72 + 2 * 9 + 6] + bc1.z * kernelsLB[1 * 72 + 2 * 9 + 7] + br1.z * kernelsLB[1 * 72 + 2 * 9 + 8] +

		tl1.w * kernelsLB[1 * 72 + 3 * 9 + 0] + tc1.w * kernelsLB[1 * 72 + 3 * 9 + 1] + tr1.w * kernelsLB[1 * 72 + 3 * 9 + 2] +
		ml1.w * kernelsLB[1 * 72 + 3 * 9 + 3] + mc1.w * kernelsLB[1 * 72 + 3 * 9 + 4] + mr1.w * kernelsLB[1 * 72 + 3 * 9 + 5] +
		bl1.w * kernelsLB[1 * 72 + 3 * 9 + 6] + bc1.w * kernelsLB[1 * 72 + 3 * 9 + 7] + br1.w * kernelsLB[1 * 72 + 3 * 9 + 8] +

		tl2.x * kernelsLB[1 * 72 + 4 * 9 + 0] + tc2.x * kernelsLB[1 * 72 + 4 * 9 + 1] + tr2.x * kernelsLB[1 * 72 + 4 * 9 + 2] +
		ml2.x * kernelsLB[1 * 72 + 4 * 9 + 3] + mc2.x * kernelsLB[1 * 72 + 4 * 9 + 4] + mr2.x * kernelsLB[1 * 72 + 4 * 9 + 5] +
		bl2.x * kernelsLB[1 * 72 + 4 * 9 + 6] + bc2.x * kernelsLB[1 * 72 + 4 * 9 + 7] + br2.x * kernelsLB[1 * 72 + 4 * 9 + 8] +

		tl2.y * kernelsLB[1 * 72 + 5 * 9 + 0] + tc2.y * kernelsLB[1 * 72 + 5 * 9 + 1] + tr2.y * kernelsLB[1 * 72 + 5 * 9 + 2] +
		ml2.y * kernelsLB[1 * 72 + 5 * 9 + 3] + mc2.y * kernelsLB[1 * 72 + 5 * 9 + 4] + mr2.y * kernelsLB[1 * 72 + 5 * 9 + 5] +
		bl2.y * kernelsLB[1 * 72 + 5 * 9 + 6] + bc2.y * kernelsLB[1 * 72 + 5 * 9 + 7] + br2.y * kernelsLB[1 * 72 + 5 * 9 + 8] +

		tl2.z * kernelsLB[1 * 72 + 6 * 9 + 0] + tc2.z * kernelsLB[1 * 72 + 6 * 9 + 1] + tr2.z * kernelsLB[1 * 72 + 6 * 9 + 2] +
		ml2.z * kernelsLB[1 * 72 + 6 * 9 + 3] + mc2.z * kernelsLB[1 * 72 + 6 * 9 + 4] + mr2.z * kernelsLB[1 * 72 + 6 * 9 + 5] +
		bl2.z * kernelsLB[1 * 72 + 6 * 9 + 6] + bc2.z * kernelsLB[1 * 72 + 6 * 9 + 7] + br2.z * kernelsLB[1 * 72 + 6 * 9 + 8] +

		tl2.w * kernelsLB[1 * 72 + 7 * 9 + 0] + tc2.w * kernelsLB[1 * 72 + 7 * 9 + 1] + tr2.w * kernelsLB[1 * 72 + 7 * 9 + 2] +
		ml2.w * kernelsLB[1 * 72 + 7 * 9 + 3] + mc2.w * kernelsLB[1 * 72 + 7 * 9 + 4] + mr2.w * kernelsLB[1 * 72 + 7 * 9 + 5] +
		bl2.w * kernelsLB[1 * 72 + 7 * 9 + 6] + bc2.w * kernelsLB[1 * 72 + 7 * 9 + 7] + br2.w * kernelsLB[1 * 72 + 7 * 9 + 8] + biasLB.y
		,
		tl1.x * kernelsLB[2 * 72 + 0 * 9 + 0] + tc1.x * kernelsLB[2 * 72 + 0 * 9 + 1] + tr1.x * kernelsLB[2 * 72 + 0 * 9 + 2] +
		ml1.x * kernelsLB[2 * 72 + 0 * 9 + 3] + mc1.x * kernelsLB[2 * 72 + 0 * 9 + 4] + mr1.x * kernelsLB[2 * 72 + 0 * 9 + 5] +
		bl1.x * kernelsLB[2 * 72 + 0 * 9 + 6] + bc1.x * kernelsLB[2 * 72 + 0 * 9 + 7] + br1.x * kernelsLB[2 * 72 + 0 * 9 + 8] +

		tl1.y * kernelsLB[2 * 72 + 1 * 9 + 0] + tc1.y * kernelsLB[2 * 72 + 1 * 9 + 1] + tr1.y * kernelsLB[2 * 72 + 1 * 9 + 2] +
		ml1.y * kernelsLB[2 * 72 + 1 * 9 + 3] + mc1.y * kernelsLB[2 * 72 + 1 * 9 + 4] + mr1.y * kernelsLB[2 * 72 + 1 * 9 + 5] +
		bl1.y * kernelsLB[2 * 72 + 1 * 9 + 6] + bc1.y * kernelsLB[2 * 72 + 1 * 9 + 7] + br1.y * kernelsLB[2 * 72 + 1 * 9 + 8] +

		tl1.z * kernelsLB[2 * 72 + 2 * 9 + 0] + tc1.z * kernelsLB[2 * 72 + 2 * 9 + 1] + tr1.z * kernelsLB[2 * 72 + 2 * 9 + 2] +
		ml1.z * kernelsLB[2 * 72 + 2 * 9 + 3] + mc1.z * kernelsLB[2 * 72 + 2 * 9 + 4] + mr1.z * kernelsLB[2 * 72 + 2 * 9 + 5] +
		bl1.z * kernelsLB[2 * 72 + 2 * 9 + 6] + bc1.z * kernelsLB[2 * 72 + 2 * 9 + 7] + br1.z * kernelsLB[2 * 72 + 2 * 9 + 8] +

		tl1.w * kernelsLB[2 * 72 + 3 * 9 + 0] + tc1.w * kernelsLB[2 * 72 + 3 * 9 + 1] + tr1.w * kernelsLB[2 * 72 + 3 * 9 + 2] +
		ml1.w * kernelsLB[2 * 72 + 3 * 9 + 3] + mc1.w * kernelsLB[2 * 72 + 3 * 9 + 4] + mr1.w * kernelsLB[2 * 72 + 3 * 9 + 5] +
		bl1.w * kernelsLB[2 * 72 + 3 * 9 + 6] + bc1.w * kernelsLB[2 * 72 + 3 * 9 + 7] + br1.w * kernelsLB[2 * 72 + 3 * 9 + 8] +

		tl2.x * kernelsLB[2 * 72 + 4 * 9 + 0] + tc2.x * kernelsLB[2 * 72 + 4 * 9 + 1] + tr2.x * kernelsLB[2 * 72 + 4 * 9 + 2] +
		ml2.x * kernelsLB[2 * 72 + 4 * 9 + 3] + mc2.x * kernelsLB[2 * 72 + 4 * 9 + 4] + mr2.x * kernelsLB[2 * 72 + 4 * 9 + 5] +
		bl2.x * kernelsLB[2 * 72 + 4 * 9 + 6] + bc2.x * kernelsLB[2 * 72 + 4 * 9 + 7] + br2.x * kernelsLB[2 * 72 + 4 * 9 + 8] +

		tl2.y * kernelsLB[2 * 72 + 5 * 9 + 0] + tc2.y * kernelsLB[2 * 72 + 5 * 9 + 1] + tr2.y * kernelsLB[2 * 72 + 5 * 9 + 2] +
		ml2.y * kernelsLB[2 * 72 + 5 * 9 + 3] + mc2.y * kernelsLB[2 * 72 + 5 * 9 + 4] + mr2.y * kernelsLB[2 * 72 + 5 * 9 + 5] +
		bl2.y * kernelsLB[2 * 72 + 5 * 9 + 6] + bc2.y * kernelsLB[2 * 72 + 5 * 9 + 7] + br2.y * kernelsLB[2 * 72 + 5 * 9 + 8] +

		tl2.z * kernelsLB[2 * 72 + 6 * 9 + 0] + tc2.z * kernelsLB[2 * 72 + 6 * 9 + 1] + tr2.z * kernelsLB[2 * 72 + 6 * 9 + 2] +
		ml2.z * kernelsLB[2 * 72 + 6 * 9 + 3] + mc2.z * kernelsLB[2 * 72 + 6 * 9 + 4] + mr2.z * kernelsLB[2 * 72 + 6 * 9 + 5] +
		bl2.z * kernelsLB[2 * 72 + 6 * 9 + 6] + bc2.z * kernelsLB[2 * 72 + 6 * 9 + 7] + br2.z * kernelsLB[2 * 72 + 6 * 9 + 8] +

		tl2.w * kernelsLB[2 * 72 + 7 * 9 + 0] + tc2.w * kernelsLB[2 * 72 + 7 * 9 + 1] + tr2.w * kernelsLB[2 * 72 + 7 * 9 + 2] +
		ml2.w * kernelsLB[2 * 72 + 7 * 9 + 3] + mc2.w * kernelsLB[2 * 72 + 7 * 9 + 4] + mr2.w * kernelsLB[2 * 72 + 7 * 9 + 5] +
		bl2.w * kernelsLB[2 * 72 + 7 * 9 + 6] + bc2.w * kernelsLB[2 * 72 + 7 * 9 + 7] + br2.w * kernelsLB[2 * 72 + 7 * 9 + 8] + biasLB.z
		,
		tl1.x * kernelsLB[3 * 72 + 0 * 9 + 0] + tc1.x * kernelsLB[3 * 72 + 0 * 9 + 1] + tr1.x * kernelsLB[3 * 72 + 0 * 9 + 2] +
		ml1.x * kernelsLB[3 * 72 + 0 * 9 + 3] + mc1.x * kernelsLB[3 * 72 + 0 * 9 + 4] + mr1.x * kernelsLB[3 * 72 + 0 * 9 + 5] +
		bl1.x * kernelsLB[3 * 72 + 0 * 9 + 6] + bc1.x * kernelsLB[3 * 72 + 0 * 9 + 7] + br1.x * kernelsLB[3 * 72 + 0 * 9 + 8] +

		tl1.y * kernelsLB[3 * 72 + 1 * 9 + 0] + tc1.y * kernelsLB[3 * 72 + 1 * 9 + 1] + tr1.y * kernelsLB[3 * 72 + 1 * 9 + 2] +
		ml1.y * kernelsLB[3 * 72 + 1 * 9 + 3] + mc1.y * kernelsLB[3 * 72 + 1 * 9 + 4] + mr1.y * kernelsLB[3 * 72 + 1 * 9 + 5] +
		bl1.y * kernelsLB[3 * 72 + 1 * 9 + 6] + bc1.y * kernelsLB[3 * 72 + 1 * 9 + 7] + br1.y * kernelsLB[3 * 72 + 1 * 9 + 8] +

		tl1.z * kernelsLB[3 * 72 + 2 * 9 + 0] + tc1.z * kernelsLB[3 * 72 + 2 * 9 + 1] + tr1.z * kernelsLB[3 * 72 + 2 * 9 + 2] +
		ml1.z * kernelsLB[3 * 72 + 2 * 9 + 3] + mc1.z * kernelsLB[3 * 72 + 2 * 9 + 4] + mr1.z * kernelsLB[3 * 72 + 2 * 9 + 5] +
		bl1.z * kernelsLB[3 * 72 + 2 * 9 + 6] + bc1.z * kernelsLB[3 * 72 + 2 * 9 + 7] + br1.z * kernelsLB[3 * 72 + 2 * 9 + 8] +

		tl1.w * kernelsLB[3 * 72 + 3 * 9 + 0] + tc1.w * kernelsLB[3 * 72 + 3 * 9 + 1] + tr1.w * kernelsLB[3 * 72 + 3 * 9 + 2] +
		ml1.w * kernelsLB[3 * 72 + 3 * 9 + 3] + mc1.w * kernelsLB[3 * 72 + 3 * 9 + 4] + mr1.w * kernelsLB[3 * 72 + 3 * 9 + 5] +
		bl1.w * kernelsLB[3 * 72 + 3 * 9 + 6] + bc1.w * kernelsLB[3 * 72 + 3 * 9 + 7] + br1.w * kernelsLB[3 * 72 + 3 * 9 + 8] +

		tl2.x * kernelsLB[3 * 72 + 4 * 9 + 0] + tc2.x * kernelsLB[3 * 72 + 4 * 9 + 1] + tr2.x * kernelsLB[3 * 72 + 4 * 9 + 2] +
		ml2.x * kernelsLB[3 * 72 + 4 * 9 + 3] + mc2.x * kernelsLB[3 * 72 + 4 * 9 + 4] + mr2.x * kernelsLB[3 * 72 + 4 * 9 + 5] +
		bl2.x * kernelsLB[3 * 72 + 4 * 9 + 6] + bc2.x * kernelsLB[3 * 72 + 4 * 9 + 7] + br2.x * kernelsLB[3 * 72 + 4 * 9 + 8] +

		tl2.y * kernelsLB[3 * 72 + 5 * 9 + 0] + tc2.y * kernelsLB[3 * 72 + 5 * 9 + 1] + tr2.y * kernelsLB[3 * 72 + 5 * 9 + 2] +
		ml2.y * kernelsLB[3 * 72 + 5 * 9 + 3] + mc2.y * kernelsLB[3 * 72 + 5 * 9 + 4] + mr2.y * kernelsLB[3 * 72 + 5 * 9 + 5] +
		bl2.y * kernelsLB[3 * 72 + 5 * 9 + 6] + bc2.y * kernelsLB[3 * 72 + 5 * 9 + 7] + br2.y * kernelsLB[3 * 72 + 5 * 9 + 8] +

		tl2.z * kernelsLB[3 * 72 + 6 * 9 + 0] + tc2.z * kernelsLB[3 * 72 + 6 * 9 + 1] + tr2.z * kernelsLB[3 * 72 + 6 * 9 + 2] +
		ml2.z * kernelsLB[3 * 72 + 6 * 9 + 3] + mc2.z * kernelsLB[3 * 72 + 6 * 9 + 4] + mr2.z * kernelsLB[3 * 72 + 6 * 9 + 5] +
		bl2.z * kernelsLB[3 * 72 + 6 * 9 + 6] + bc2.z * kernelsLB[3 * 72 + 6 * 9 + 7] + br2.z * kernelsLB[3 * 72 + 6 * 9 + 8] +

		tl2.w * kernelsLB[3 * 72 + 7 * 9 + 0] + tc2.w * kernelsLB[3 * 72 + 7 * 9 + 1] + tr2.w * kernelsLB[3 * 72 + 7 * 9 + 2] +
		ml2.w * kernelsLB[3 * 72 + 7 * 9 + 3] + mc2.w * kernelsLB[3 * 72 + 7 * 9 + 4] + mr2.w * kernelsLB[3 * 72 + 7 * 9 + 5] +
		bl2.w * kernelsLB[3 * 72 + 7 * 9 + 6] + bc2.w * kernelsLB[3 * 72 + 7 * 9 + 7] + br2.w * kernelsLB[3 * 72 + 7 * 9 + 8] + biasLB.w
	));

	tex3[gxy] = target1;
	tex4[gxy] = target2;
}


//!PASS 3
//!IN tex3, tex4
//!OUT tex1, tex2
//!BLOCK_SIZE 8
//!NUM_THREADS 64

const static float kernelsLA[9 * 8 * 4] = {
	-4.2606e-02, -8.9001e-02, -6.4006e-02,
	 1.1132e-01,  7.6609e-02,  8.6417e-02,
	 7.6477e-03, -1.6416e-02, -8.2094e-02,
	 1.0779e-01,  2.1837e-01,  1.8094e-01,
	-2.6306e-02, -1.2452e-01,  1.2662e-02,
	 3.1633e-02,  1.8717e-02,  3.1043e-02,
	 4.0927e-02,  5.0311e-02,  1.1648e-01,
	 2.2429e-01,  2.0757e-01,  4.3662e-03,
	 3.6341e-02, -4.7637e-02,  8.3645e-02,
	-8.9260e-03,  1.8507e-02,  7.9069e-02,
	-1.9411e-01, -8.6847e-02, -3.6639e-03,
	 4.0328e-02, -3.6821e-02, -8.5387e-02,
	 5.8173e-02,  5.9991e-02, -3.1398e-02,
	 1.5818e-01,  3.0861e-01, -2.3818e-02,
	 1.2176e-01,  6.7520e-02,  8.9401e-02,
	-2.8859e-02, -1.2237e-01, -1.0625e-01,
	 3.1675e-02,  1.4172e-01, -1.4373e-01,
	 1.4653e-02,  1.0205e-01,  6.2557e-02,
	-8.7292e-02, -2.1255e-02,  3.6830e-02,
	-5.4417e-02,  3.0501e-01,  1.6897e-01,
	-2.2187e-02, -8.9609e-02, -2.2830e-02,
	 4.9846e-02,  3.3395e-01, -3.1561e-02,
	-1.3191e-02,  4.2663e-01, -6.9727e-02,
	 1.4570e-02, -4.0002e-02,  5.6394e-02,
	-8.2547e-02,  1.9249e-01,  1.5591e-01,
	 1.4536e-01, -1.0409e-01,  1.2382e-01,
	 1.8189e-01,  9.2917e-02, -1.4394e-01,
	-5.6260e-02, -2.7043e-01,  1.5392e-02,
	-1.4305e-02,  1.1131e-01, -8.5913e-02,
	 7.7914e-02, -6.5484e-03, -1.8375e-01,
	-1.4059e-01, -5.7339e-01, -3.9073e-02,
	-1.1701e-01, -3.1806e-02,  7.7726e-02,
	 2.1688e-02,  9.9297e-02,  3.8224e-02,
	 7.9884e-02,  5.2461e-02,  1.0318e-01,
	 4.0054e-02,  1.4695e-01,  1.2577e-01,
	-1.8790e-03, -4.9421e-02,  2.3235e-02,
	-8.9820e-02, -1.6994e-01, -1.5986e-01,
	 2.3436e-01, -1.5346e-01,  1.5014e-02,
	-3.9139e-02, -7.9388e-02, -4.9057e-02,
	-1.1193e-01, -2.5705e-01,  1.1995e-01,
	 5.7929e-02,  2.4988e-01, -4.9406e-03,
	-3.9363e-02, -1.1691e-02, -1.2236e-03,
	-2.0521e-01,  2.1901e-01,  1.5957e-01,
	 2.1062e-01, -1.4157e-01, -3.4340e-01,
	 3.8520e-02, -2.0820e-01,  2.4570e-03,
	 1.7211e-01,  2.0214e-01,  1.3821e-01,
	-7.1520e-02,  1.4847e-01, -1.3820e-01,
	-2.4712e-02, -1.5925e-02,  1.7403e-02,
	-3.7515e-02,  3.0461e-02, -2.7543e-02,
	 8.6148e-02, -6.1486e-02,  1.2610e-02,
	 2.9748e-03,  1.1778e-01,  2.9032e-02,
	-2.1706e-02, -2.2406e-02,  2.6769e-02,
	-3.6965e-02,  2.2180e-01, -4.0929e-02,
	-3.2629e-03,  8.3419e-02, -1.4587e-01,
	-1.3909e-02, -2.0166e-02, -1.0029e-01,
	 7.6360e-02,  8.0819e-02, -1.0933e-01,
	-5.8919e-02,  2.4745e-02,  3.7375e-02,
	-1.1333e-02,  1.4747e-02, -7.8958e-02,
	-3.1535e-02,  1.7403e-01,  1.3946e-02,
	-3.2038e-02,  5.1151e-02, -6.1063e-02,
	-8.6472e-03, -6.9689e-02,  5.6846e-03,
	 5.7914e-02, -1.9818e-01, -7.5321e-02,
	 8.7453e-02,  7.8354e-02,  2.1997e-02,
	-4.7606e-02,  1.3915e-01,  1.1653e-01,
	 9.6050e-02,  4.0099e-01,  1.5631e-01,
	 3.1492e-02,  2.4797e-01,  6.8716e-02,
	-6.2664e-03,  9.1754e-02, -5.7244e-03,
	 1.3538e-01,  1.5366e-01,  9.4916e-02,
	-4.2115e-02, -3.6585e-01, -1.4559e-01,
	 9.1550e-02, -5.4007e-02,  6.7482e-02,
	-1.8687e-01,  3.2120e-01,  5.1031e-03,
	-6.1205e-02, -5.1780e-02,  1.6442e-02,
	-1.2316e-02, -1.3907e-01, -1.4446e-01,
	-2.7899e-01, -8.5969e-02, -1.0870e-01,
	-2.6157e-01,  8.9532e-02,  3.0958e-02,
	-1.5393e-01, -4.2781e-02, -2.0951e-01,
	 2.0328e-01,  4.5317e-01, -3.0467e-02,
	-6.1346e-02,  1.0381e-01, -1.3719e-01,
	-9.8572e-02, -1.4035e-01, -1.9431e-02,
	 2.5542e-02,  3.2609e-01,  1.7983e-03,
	-1.0800e-01, -2.9022e-02,  6.2691e-03,
	 2.8937e-02, -1.3483e-01, -4.1655e-02,
	 2.0172e-01,  1.4283e-02,  9.6200e-02,
	 1.9027e-02,  3.1240e-01, -2.9553e-02,
	 6.2776e-02,  1.3845e-01,  4.5834e-02,
	-2.3854e-01, -4.0267e-02,  1.5634e-02,
	-1.9246e-01, -3.2332e-02,  3.2442e-03,
	-6.1880e-02, -8.8192e-02, -6.0172e-02,
	 2.5002e-01,  1.5148e-01,  6.4459e-02,
	-2.1022e-01, -8.3893e-02,  6.9554e-03,
	 7.0244e-02, -2.9551e-02,  1.6481e-02,
	-3.1036e-02, -2.0026e-01, -8.4748e-02,
	-1.3108e-01, -1.3784e-01,  9.4900e-02,
	-2.1256e-01, -4.1767e-02,  8.4665e-02,
	-4.0235e-01,  1.0604e-01, -3.1827e-02,
	-4.9825e-02, -9.1267e-04,  1.5527e-02
};

const static float4 biasLA = { -0.0239, -0.0385,  0.0026,  0.0288 };

const static float kernelsLB[9 * 8 * 4] = {
	-6.5729e-03, -1.8932e-02, -3.4591e-02,
	 1.1066e-01,  9.3979e-02,  2.6059e-02,
	-1.2395e-01, -2.4768e-01, -1.6304e-01,
	 8.8329e-03, -2.1606e-02, -4.0878e-02,
	-1.5581e-02, -1.4829e-02, -1.5959e-02,
	-1.0463e-04, -4.2903e-03, -4.6657e-02,
	 2.2995e-02,  1.7917e-02, -9.1404e-02,
	-1.2326e-01,  1.4582e-01, -7.0959e-02,
	-1.8058e-02, -8.5228e-02,  4.2799e-02,
	-2.2829e-03,  8.6577e-02, -1.1909e-01,
	-1.8061e-01,  1.1166e-01, -8.2255e-02,
	-1.3190e-01,  7.7123e-02,  2.3224e-02,
	 1.8661e-02,  2.4461e-02,  3.6060e-02,
	-4.5224e-02, -1.7672e-01,  1.6080e-01,
	-4.2175e-01, -2.2557e-01, -1.0719e-01,
	-2.9506e-02,  9.5020e-02, -6.6465e-02,
	-7.2627e-02,  3.1236e-01,  5.5764e-02,
	-2.8789e-01, -1.8915e-01,  9.0825e-02,
	-5.8618e-02,  6.4082e-02,  4.8461e-03,
	-5.9405e-02,  3.2644e-01, -7.1278e-02,
	-1.8084e-01,  2.0858e-02, -9.3690e-03,
	-7.6565e-03, -9.6854e-02,  7.6121e-03,
	 1.4791e-01,  4.5612e-01,  1.9889e-02,
	-5.5498e-02, -1.1266e-01,  2.2790e-02,
	-3.8821e-02, -1.5780e-02,  1.2549e-02,
	-3.8232e-02, -2.8870e-01,  2.6216e-02,
	 1.0375e-01, -2.9621e-02,  1.8479e-03,
	 5.0207e-02,  1.5189e-01,  1.2533e-01,
	 1.8298e-01, -1.2870e-01,  3.0681e-01,
	-1.9571e-02, -8.6302e-02,  9.1121e-02,
	 1.0113e-01, -1.8362e-01,  3.2642e-02,
	 1.7034e-01, -3.1077e-01, -4.8737e-02,
	 5.9144e-02,  5.6052e-03,  3.2360e-02,
	-9.0123e-02,  7.7996e-02,  3.6297e-02,
	-3.4389e-01,  1.1841e-01, -2.0900e-02,
	 9.4930e-02, -9.1504e-02, -4.5308e-02,
	 3.7723e-03, -3.7580e-02, -6.6410e-02,
	 5.2501e-02, -1.2530e-01,  3.5944e-02,
	 3.8378e-02,  9.5188e-02,  2.1952e-03,
	-2.4333e-02,  2.7977e-01,  5.6961e-02,
	-3.0605e-03,  8.3684e-02,  4.4848e-03,
	-7.8935e-02, -1.9544e-01, -5.3311e-02,
	-2.6595e-02,  1.2278e-01, -3.1659e-02,
	-1.0103e-02,  4.7763e-01,  2.5359e-02,
	 8.1397e-02,  3.0548e-01,  9.7097e-02,
	 3.6232e-02, -1.1091e-01,  1.2841e-01,
	 1.9277e-01,  2.9322e-01, -1.6740e-01,
	 1.2107e-01, -6.2883e-02,  4.0603e-02,
	-1.5750e-01, -8.6183e-02, -1.4194e-01,
	 1.1932e-01, -3.9175e-01, -5.4495e-02,
	-1.4001e-02, -2.0594e-01, -8.2683e-02,
	 8.6156e-02,  2.1499e-02,  2.2080e-01,
	 5.5703e-02, -3.6307e-01,  8.3129e-02,
	 8.9280e-02, -3.5897e-02,  1.6106e-01,
	 9.1171e-02, -3.1102e-01,  1.2425e-01,
	 1.0278e-01, -3.1014e-01, -6.9138e-02,
	 8.0839e-02, -3.6183e-02,  1.0341e-01,
	-1.8334e-01, -5.3700e-02,  2.3336e-01,
	-1.4464e-01, -5.0320e-01, -2.9836e-02,
	-1.7225e-01, -3.9499e-01, -1.7321e-01,
	 1.7510e-01,  1.7897e-01, -2.6518e-01,
	 2.3638e-01,  5.0270e-01, -4.9731e-03,
	 2.2603e-01,  2.5317e-01,  2.4079e-01,
	-1.3159e-01,  1.5638e-01,  1.2480e-01,
	-6.2164e-02,  7.9458e-02, -9.4804e-02,
	 8.5690e-03,  7.4971e-03,  8.6630e-02,
	-1.3148e-02,  6.8660e-02, -7.4230e-03,
	 2.9702e-02,  1.2036e-01,  9.5504e-02,
	-3.2694e-03,  8.6722e-02, -6.2433e-02,
	 3.2527e-01,  3.2087e-01, -9.4429e-05,
	 1.3556e-01, -7.0413e-02,  2.9383e-02,
	 2.0617e-02,  3.3218e-02,  4.4898e-02,
	-4.8260e-01, -2.1329e-01,  1.5890e-02,
	-2.6600e-01, -8.8519e-02, -4.3800e-02,
	-1.7299e-01, -2.0757e-01, -2.6658e-01,
	 6.9707e-02, -4.4700e-02,  6.5570e-02,
	 2.3992e-01,  1.5078e-01,  2.8713e-02,
	-9.1197e-02,  1.9765e-02, -1.8751e-02,
	-9.9277e-02, -3.1437e-01,  4.0730e-02,
	 2.4208e-02, -8.8322e-02, -1.6245e-01,
	 1.3037e-02, -3.4708e-02, -4.4285e-02,
	-1.3592e-01, -1.3575e-01, -7.4546e-02,
	 1.4670e-01, -1.3366e-01,  2.1553e-03,
	 8.1235e-03, -1.2068e-01, -5.7287e-02,
	 1.8015e-01,  2.1390e-01,  8.6923e-03,
	 2.8833e-01,  6.6345e-02,  1.4578e-01,
	 2.2338e-01,  2.6453e-01, -2.9112e-02,
	 1.4018e-01, -9.2824e-02, -2.2795e-02,
	 1.2360e-01,  2.2527e-01, -1.1817e-01,
	-3.8872e-02, -1.9982e-02, -7.7514e-02,
	 1.7744e-03,  3.1736e-02,  4.5882e-02,
	-2.5222e-02,  2.4298e-01, -3.8596e-02,
	 1.2545e-02,  3.1872e-02,  7.1925e-02,
	 7.9782e-02, -1.5533e-01, -1.4619e-02,
	-1.2223e-01, -1.8631e-03, -9.8832e-02,
	-1.6815e-02, -8.1440e-02,  6.8038e-02
};

const static float4 biasLB = { -0.0225,  0.0082, -0.0191, -0.0185 };


void Pass3(uint2 blockStart, uint3 threadId) {
	uint2 gxy = Rmp8x8(threadId.x) + blockStart;
	uint2 inputSize = GetInputSize();
	if (gxy.x >= inputSize.x || gxy.y >= inputSize.y) {
		return;
	}

	float2 inputPt = GetInputPt();
	float2 pos = (gxy + 0.5f) * inputPt;

	// [tl, tc, tr]
	// [ml, mc, mr]
	// [bl, bc, br]
	float4 tl1 = tex3.SampleLevel(sam, pos + float2(-inputPt.x, -inputPt.y), 0);
	float4 ml1 = tex3.SampleLevel(sam, pos + float2(-inputPt.x, 0), 0);
	float4 bl1 = tex3.SampleLevel(sam, pos + float2(-inputPt.x, inputPt.y), 0);
	float4 tc1 = tex3.SampleLevel(sam, pos + float2(0, -inputPt.y), 0);
	float4 mc1 = tex3.SampleLevel(sam, pos, 0);
	float4 bc1 = tex3.SampleLevel(sam, pos + float2(0, inputPt.y), 0);
	float4 tr1 = tex3.SampleLevel(sam, pos + float2(inputPt.x, -inputPt.y), 0);
	float4 mr1 = tex3.SampleLevel(sam, pos + float2(inputPt.x, 0), 0);
	float4 br1 = tex3.SampleLevel(sam, pos + float2(inputPt.x, inputPt.y), 0);

	float4 tl2 = tex4.SampleLevel(sam, pos + float2(-inputPt.x, -inputPt.y), 0);
	float4 ml2 = tex4.SampleLevel(sam, pos + float2(-inputPt.x, 0), 0);
	float4 bl2 = tex4.SampleLevel(sam, pos + float2(-inputPt.x, inputPt.y), 0);
	float4 tc2 = tex4.SampleLevel(sam, pos + float2(0, -inputPt.y), 0);
	float4 mc2 = tex4.SampleLevel(sam, pos, 0);
	float4 bc2 = tex4.SampleLevel(sam, pos + float2(0, inputPt.y), 0);
	float4 tr2 = tex4.SampleLevel(sam, pos + float2(inputPt.x, -inputPt.y), 0);
	float4 mr2 = tex4.SampleLevel(sam, pos + float2(inputPt.x, 0), 0);
	float4 br2 = tex4.SampleLevel(sam, pos + float2(inputPt.x, inputPt.y), 0);

	float4 target1 = RELU(float4(
		tl1.x * kernelsLA[0 * 72 + 0 * 9 + 0] + tc1.x * kernelsLA[0 * 72 + 0 * 9 + 1] + tr1.x * kernelsLA[0 * 72 + 0 * 9 + 2] +
		ml1.x * kernelsLA[0 * 72 + 0 * 9 + 3] + mc1.x * kernelsLA[0 * 72 + 0 * 9 + 4] + mr1.x * kernelsLA[0 * 72 + 0 * 9 + 5] +
		bl1.x * kernelsLA[0 * 72 + 0 * 9 + 6] + bc1.x * kernelsLA[0 * 72 + 0 * 9 + 7] + br1.x * kernelsLA[0 * 72 + 0 * 9 + 8] +

		tl1.y * kernelsLA[0 * 72 + 1 * 9 + 0] + tc1.y * kernelsLA[0 * 72 + 1 * 9 + 1] + tr1.y * kernelsLA[0 * 72 + 1 * 9 + 2] +
		ml1.y * kernelsLA[0 * 72 + 1 * 9 + 3] + mc1.y * kernelsLA[0 * 72 + 1 * 9 + 4] + mr1.y * kernelsLA[0 * 72 + 1 * 9 + 5] +
		bl1.y * kernelsLA[0 * 72 + 1 * 9 + 6] + bc1.y * kernelsLA[0 * 72 + 1 * 9 + 7] + br1.y * kernelsLA[0 * 72 + 1 * 9 + 8] +

		tl1.z * kernelsLA[0 * 72 + 2 * 9 + 0] + tc1.z * kernelsLA[0 * 72 + 2 * 9 + 1] + tr1.z * kernelsLA[0 * 72 + 2 * 9 + 2] +
		ml1.z * kernelsLA[0 * 72 + 2 * 9 + 3] + mc1.z * kernelsLA[0 * 72 + 2 * 9 + 4] + mr1.z * kernelsLA[0 * 72 + 2 * 9 + 5] +
		bl1.z * kernelsLA[0 * 72 + 2 * 9 + 6] + bc1.z * kernelsLA[0 * 72 + 2 * 9 + 7] + br1.z * kernelsLA[0 * 72 + 2 * 9 + 8] +

		tl1.w * kernelsLA[0 * 72 + 3 * 9 + 0] + tc1.w * kernelsLA[0 * 72 + 3 * 9 + 1] + tr1.w * kernelsLA[0 * 72 + 3 * 9 + 2] +
		ml1.w * kernelsLA[0 * 72 + 3 * 9 + 3] + mc1.w * kernelsLA[0 * 72 + 3 * 9 + 4] + mr1.w * kernelsLA[0 * 72 + 3 * 9 + 5] +
		bl1.w * kernelsLA[0 * 72 + 3 * 9 + 6] + bc1.w * kernelsLA[0 * 72 + 3 * 9 + 7] + br1.w * kernelsLA[0 * 72 + 3 * 9 + 8] +

		tl2.x * kernelsLA[0 * 72 + 4 * 9 + 0] + tc2.x * kernelsLA[0 * 72 + 4 * 9 + 1] + tr2.x * kernelsLA[0 * 72 + 4 * 9 + 2] +
		ml2.x * kernelsLA[0 * 72 + 4 * 9 + 3] + mc2.x * kernelsLA[0 * 72 + 4 * 9 + 4] + mr2.x * kernelsLA[0 * 72 + 4 * 9 + 5] +
		bl2.x * kernelsLA[0 * 72 + 4 * 9 + 6] + bc2.x * kernelsLA[0 * 72 + 4 * 9 + 7] + br2.x * kernelsLA[0 * 72 + 4 * 9 + 8] +

		tl2.y * kernelsLA[0 * 72 + 5 * 9 + 0] + tc2.y * kernelsLA[0 * 72 + 5 * 9 + 1] + tr2.y * kernelsLA[0 * 72 + 5 * 9 + 2] +
		ml2.y * kernelsLA[0 * 72 + 5 * 9 + 3] + mc2.y * kernelsLA[0 * 72 + 5 * 9 + 4] + mr2.y * kernelsLA[0 * 72 + 5 * 9 + 5] +
		bl2.y * kernelsLA[0 * 72 + 5 * 9 + 6] + bc2.y * kernelsLA[0 * 72 + 5 * 9 + 7] + br2.y * kernelsLA[0 * 72 + 5 * 9 + 8] +

		tl2.z * kernelsLA[0 * 72 + 6 * 9 + 0] + tc2.z * kernelsLA[0 * 72 + 6 * 9 + 1] + tr2.z * kernelsLA[0 * 72 + 6 * 9 + 2] +
		ml2.z * kernelsLA[0 * 72 + 6 * 9 + 3] + mc2.z * kernelsLA[0 * 72 + 6 * 9 + 4] + mr2.z * kernelsLA[0 * 72 + 6 * 9 + 5] +
		bl2.z * kernelsLA[0 * 72 + 6 * 9 + 6] + bc2.z * kernelsLA[0 * 72 + 6 * 9 + 7] + br2.z * kernelsLA[0 * 72 + 6 * 9 + 8] +

		tl2.w * kernelsLA[0 * 72 + 7 * 9 + 0] + tc2.w * kernelsLA[0 * 72 + 7 * 9 + 1] + tr2.w * kernelsLA[0 * 72 + 7 * 9 + 2] +
		ml2.w * kernelsLA[0 * 72 + 7 * 9 + 3] + mc2.w * kernelsLA[0 * 72 + 7 * 9 + 4] + mr2.w * kernelsLA[0 * 72 + 7 * 9 + 5] +
		bl2.w * kernelsLA[0 * 72 + 7 * 9 + 6] + bc2.w * kernelsLA[0 * 72 + 7 * 9 + 7] + br2.w * kernelsLA[0 * 72 + 7 * 9 + 8] + biasLA.x
		,
		tl1.x * kernelsLA[1 * 72 + 0 * 9 + 0] + tc1.x * kernelsLA[1 * 72 + 0 * 9 + 1] + tr1.x * kernelsLA[1 * 72 + 0 * 9 + 2] +
		ml1.x * kernelsLA[1 * 72 + 0 * 9 + 3] + mc1.x * kernelsLA[1 * 72 + 0 * 9 + 4] + mr1.x * kernelsLA[1 * 72 + 0 * 9 + 5] +
		bl1.x * kernelsLA[1 * 72 + 0 * 9 + 6] + bc1.x * kernelsLA[1 * 72 + 0 * 9 + 7] + br1.x * kernelsLA[1 * 72 + 0 * 9 + 8] +

		tl1.y * kernelsLA[1 * 72 + 1 * 9 + 0] + tc1.y * kernelsLA[1 * 72 + 1 * 9 + 1] + tr1.y * kernelsLA[1 * 72 + 1 * 9 + 2] +
		ml1.y * kernelsLA[1 * 72 + 1 * 9 + 3] + mc1.y * kernelsLA[1 * 72 + 1 * 9 + 4] + mr1.y * kernelsLA[1 * 72 + 1 * 9 + 5] +
		bl1.y * kernelsLA[1 * 72 + 1 * 9 + 6] + bc1.y * kernelsLA[1 * 72 + 1 * 9 + 7] + br1.y * kernelsLA[1 * 72 + 1 * 9 + 8] +

		tl1.z * kernelsLA[1 * 72 + 2 * 9 + 0] + tc1.z * kernelsLA[1 * 72 + 2 * 9 + 1] + tr1.z * kernelsLA[1 * 72 + 2 * 9 + 2] +
		ml1.z * kernelsLA[1 * 72 + 2 * 9 + 3] + mc1.z * kernelsLA[1 * 72 + 2 * 9 + 4] + mr1.z * kernelsLA[1 * 72 + 2 * 9 + 5] +
		bl1.z * kernelsLA[1 * 72 + 2 * 9 + 6] + bc1.z * kernelsLA[1 * 72 + 2 * 9 + 7] + br1.z * kernelsLA[1 * 72 + 2 * 9 + 8] +

		tl1.w * kernelsLA[1 * 72 + 3 * 9 + 0] + tc1.w * kernelsLA[1 * 72 + 3 * 9 + 1] + tr1.w * kernelsLA[1 * 72 + 3 * 9 + 2] +
		ml1.w * kernelsLA[1 * 72 + 3 * 9 + 3] + mc1.w * kernelsLA[1 * 72 + 3 * 9 + 4] + mr1.w * kernelsLA[1 * 72 + 3 * 9 + 5] +
		bl1.w * kernelsLA[1 * 72 + 3 * 9 + 6] + bc1.w * kernelsLA[1 * 72 + 3 * 9 + 7] + br1.w * kernelsLA[1 * 72 + 3 * 9 + 8] +

		tl2.x * kernelsLA[1 * 72 + 4 * 9 + 0] + tc2.x * kernelsLA[1 * 72 + 4 * 9 + 1] + tr2.x * kernelsLA[1 * 72 + 4 * 9 + 2] +
		ml2.x * kernelsLA[1 * 72 + 4 * 9 + 3] + mc2.x * kernelsLA[1 * 72 + 4 * 9 + 4] + mr2.x * kernelsLA[1 * 72 + 4 * 9 + 5] +
		bl2.x * kernelsLA[1 * 72 + 4 * 9 + 6] + bc2.x * kernelsLA[1 * 72 + 4 * 9 + 7] + br2.x * kernelsLA[1 * 72 + 4 * 9 + 8] +

		tl2.y * kernelsLA[1 * 72 + 5 * 9 + 0] + tc2.y * kernelsLA[1 * 72 + 5 * 9 + 1] + tr2.y * kernelsLA[1 * 72 + 5 * 9 + 2] +
		ml2.y * kernelsLA[1 * 72 + 5 * 9 + 3] + mc2.y * kernelsLA[1 * 72 + 5 * 9 + 4] + mr2.y * kernelsLA[1 * 72 + 5 * 9 + 5] +
		bl2.y * kernelsLA[1 * 72 + 5 * 9 + 6] + bc2.y * kernelsLA[1 * 72 + 5 * 9 + 7] + br2.y * kernelsLA[1 * 72 + 5 * 9 + 8] +

		tl2.z * kernelsLA[1 * 72 + 6 * 9 + 0] + tc2.z * kernelsLA[1 * 72 + 6 * 9 + 1] + tr2.z * kernelsLA[1 * 72 + 6 * 9 + 2] +
		ml2.z * kernelsLA[1 * 72 + 6 * 9 + 3] + mc2.z * kernelsLA[1 * 72 + 6 * 9 + 4] + mr2.z * kernelsLA[1 * 72 + 6 * 9 + 5] +
		bl2.z * kernelsLA[1 * 72 + 6 * 9 + 6] + bc2.z * kernelsLA[1 * 72 + 6 * 9 + 7] + br2.z * kernelsLA[1 * 72 + 6 * 9 + 8] +

		tl2.w * kernelsLA[1 * 72 + 7 * 9 + 0] + tc2.w * kernelsLA[1 * 72 + 7 * 9 + 1] + tr2.w * kernelsLA[1 * 72 + 7 * 9 + 2] +
		ml2.w * kernelsLA[1 * 72 + 7 * 9 + 3] + mc2.w * kernelsLA[1 * 72 + 7 * 9 + 4] + mr2.w * kernelsLA[1 * 72 + 7 * 9 + 5] +
		bl2.w * kernelsLA[1 * 72 + 7 * 9 + 6] + bc2.w * kernelsLA[1 * 72 + 7 * 9 + 7] + br2.w * kernelsLA[1 * 72 + 7 * 9 + 8] + biasLA.y
		,
		tl1.x * kernelsLA[2 * 72 + 0 * 9 + 0] + tc1.x * kernelsLA[2 * 72 + 0 * 9 + 1] + tr1.x * kernelsLA[2 * 72 + 0 * 9 + 2] +
		ml1.x * kernelsLA[2 * 72 + 0 * 9 + 3] + mc1.x * kernelsLA[2 * 72 + 0 * 9 + 4] + mr1.x * kernelsLA[2 * 72 + 0 * 9 + 5] +
		bl1.x * kernelsLA[2 * 72 + 0 * 9 + 6] + bc1.x * kernelsLA[2 * 72 + 0 * 9 + 7] + br1.x * kernelsLA[2 * 72 + 0 * 9 + 8] +

		tl1.y * kernelsLA[2 * 72 + 1 * 9 + 0] + tc1.y * kernelsLA[2 * 72 + 1 * 9 + 1] + tr1.y * kernelsLA[2 * 72 + 1 * 9 + 2] +
		ml1.y * kernelsLA[2 * 72 + 1 * 9 + 3] + mc1.y * kernelsLA[2 * 72 + 1 * 9 + 4] + mr1.y * kernelsLA[2 * 72 + 1 * 9 + 5] +
		bl1.y * kernelsLA[2 * 72 + 1 * 9 + 6] + bc1.y * kernelsLA[2 * 72 + 1 * 9 + 7] + br1.y * kernelsLA[2 * 72 + 1 * 9 + 8] +

		tl1.z * kernelsLA[2 * 72 + 2 * 9 + 0] + tc1.z * kernelsLA[2 * 72 + 2 * 9 + 1] + tr1.z * kernelsLA[2 * 72 + 2 * 9 + 2] +
		ml1.z * kernelsLA[2 * 72 + 2 * 9 + 3] + mc1.z * kernelsLA[2 * 72 + 2 * 9 + 4] + mr1.z * kernelsLA[2 * 72 + 2 * 9 + 5] +
		bl1.z * kernelsLA[2 * 72 + 2 * 9 + 6] + bc1.z * kernelsLA[2 * 72 + 2 * 9 + 7] + br1.z * kernelsLA[2 * 72 + 2 * 9 + 8] +

		tl1.w * kernelsLA[2 * 72 + 3 * 9 + 0] + tc1.w * kernelsLA[2 * 72 + 3 * 9 + 1] + tr1.w * kernelsLA[2 * 72 + 3 * 9 + 2] +
		ml1.w * kernelsLA[2 * 72 + 3 * 9 + 3] + mc1.w * kernelsLA[2 * 72 + 3 * 9 + 4] + mr1.w * kernelsLA[2 * 72 + 3 * 9 + 5] +
		bl1.w * kernelsLA[2 * 72 + 3 * 9 + 6] + bc1.w * kernelsLA[2 * 72 + 3 * 9 + 7] + br1.w * kernelsLA[2 * 72 + 3 * 9 + 8] +

		tl2.x * kernelsLA[2 * 72 + 4 * 9 + 0] + tc2.x * kernelsLA[2 * 72 + 4 * 9 + 1] + tr2.x * kernelsLA[2 * 72 + 4 * 9 + 2] +
		ml2.x * kernelsLA[2 * 72 + 4 * 9 + 3] + mc2.x * kernelsLA[2 * 72 + 4 * 9 + 4] + mr2.x * kernelsLA[2 * 72 + 4 * 9 + 5] +
		bl2.x * kernelsLA[2 * 72 + 4 * 9 + 6] + bc2.x * kernelsLA[2 * 72 + 4 * 9 + 7] + br2.x * kernelsLA[2 * 72 + 4 * 9 + 8] +

		tl2.y * kernelsLA[2 * 72 + 5 * 9 + 0] + tc2.y * kernelsLA[2 * 72 + 5 * 9 + 1] + tr2.y * kernelsLA[2 * 72 + 5 * 9 + 2] +
		ml2.y * kernelsLA[2 * 72 + 5 * 9 + 3] + mc2.y * kernelsLA[2 * 72 + 5 * 9 + 4] + mr2.y * kernelsLA[2 * 72 + 5 * 9 + 5] +
		bl2.y * kernelsLA[2 * 72 + 5 * 9 + 6] + bc2.y * kernelsLA[2 * 72 + 5 * 9 + 7] + br2.y * kernelsLA[2 * 72 + 5 * 9 + 8] +

		tl2.z * kernelsLA[2 * 72 + 6 * 9 + 0] + tc2.z * kernelsLA[2 * 72 + 6 * 9 + 1] + tr2.z * kernelsLA[2 * 72 + 6 * 9 + 2] +
		ml2.z * kernelsLA[2 * 72 + 6 * 9 + 3] + mc2.z * kernelsLA[2 * 72 + 6 * 9 + 4] + mr2.z * kernelsLA[2 * 72 + 6 * 9 + 5] +
		bl2.z * kernelsLA[2 * 72 + 6 * 9 + 6] + bc2.z * kernelsLA[2 * 72 + 6 * 9 + 7] + br2.z * kernelsLA[2 * 72 + 6 * 9 + 8] +

		tl2.w * kernelsLA[2 * 72 + 7 * 9 + 0] + tc2.w * kernelsLA[2 * 72 + 7 * 9 + 1] + tr2.w * kernelsLA[2 * 72 + 7 * 9 + 2] +
		ml2.w * kernelsLA[2 * 72 + 7 * 9 + 3] + mc2.w * kernelsLA[2 * 72 + 7 * 9 + 4] + mr2.w * kernelsLA[2 * 72 + 7 * 9 + 5] +
		bl2.w * kernelsLA[2 * 72 + 7 * 9 + 6] + bc2.w * kernelsLA[2 * 72 + 7 * 9 + 7] + br2.w * kernelsLA[2 * 72 + 7 * 9 + 8] + biasLA.z
		,
		tl1.x * kernelsLA[3 * 72 + 0 * 9 + 0] + tc1.x * kernelsLA[3 * 72 + 0 * 9 + 1] + tr1.x * kernelsLA[3 * 72 + 0 * 9 + 2] +
		ml1.x * kernelsLA[3 * 72 + 0 * 9 + 3] + mc1.x * kernelsLA[3 * 72 + 0 * 9 + 4] + mr1.x * kernelsLA[3 * 72 + 0 * 9 + 5] +
		bl1.x * kernelsLA[3 * 72 + 0 * 9 + 6] + bc1.x * kernelsLA[3 * 72 + 0 * 9 + 7] + br1.x * kernelsLA[3 * 72 + 0 * 9 + 8] +

		tl1.y * kernelsLA[3 * 72 + 1 * 9 + 0] + tc1.y * kernelsLA[3 * 72 + 1 * 9 + 1] + tr1.y * kernelsLA[3 * 72 + 1 * 9 + 2] +
		ml1.y * kernelsLA[3 * 72 + 1 * 9 + 3] + mc1.y * kernelsLA[3 * 72 + 1 * 9 + 4] + mr1.y * kernelsLA[3 * 72 + 1 * 9 + 5] +
		bl1.y * kernelsLA[3 * 72 + 1 * 9 + 6] + bc1.y * kernelsLA[3 * 72 + 1 * 9 + 7] + br1.y * kernelsLA[3 * 72 + 1 * 9 + 8] +

		tl1.z * kernelsLA[3 * 72 + 2 * 9 + 0] + tc1.z * kernelsLA[3 * 72 + 2 * 9 + 1] + tr1.z * kernelsLA[3 * 72 + 2 * 9 + 2] +
		ml1.z * kernelsLA[3 * 72 + 2 * 9 + 3] + mc1.z * kernelsLA[3 * 72 + 2 * 9 + 4] + mr1.z * kernelsLA[3 * 72 + 2 * 9 + 5] +
		bl1.z * kernelsLA[3 * 72 + 2 * 9 + 6] + bc1.z * kernelsLA[3 * 72 + 2 * 9 + 7] + br1.z * kernelsLA[3 * 72 + 2 * 9 + 8] +

		tl1.w * kernelsLA[3 * 72 + 3 * 9 + 0] + tc1.w * kernelsLA[3 * 72 + 3 * 9 + 1] + tr1.w * kernelsLA[3 * 72 + 3 * 9 + 2] +
		ml1.w * kernelsLA[3 * 72 + 3 * 9 + 3] + mc1.w * kernelsLA[3 * 72 + 3 * 9 + 4] + mr1.w * kernelsLA[3 * 72 + 3 * 9 + 5] +
		bl1.w * kernelsLA[3 * 72 + 3 * 9 + 6] + bc1.w * kernelsLA[3 * 72 + 3 * 9 + 7] + br1.w * kernelsLA[3 * 72 + 3 * 9 + 8] +

		tl2.x * kernelsLA[3 * 72 + 4 * 9 + 0] + tc2.x * kernelsLA[3 * 72 + 4 * 9 + 1] + tr2.x * kernelsLA[3 * 72 + 4 * 9 + 2] +
		ml2.x * kernelsLA[3 * 72 + 4 * 9 + 3] + mc2.x * kernelsLA[3 * 72 + 4 * 9 + 4] + mr2.x * kernelsLA[3 * 72 + 4 * 9 + 5] +
		bl2.x * kernelsLA[3 * 72 + 4 * 9 + 6] + bc2.x * kernelsLA[3 * 72 + 4 * 9 + 7] + br2.x * kernelsLA[3 * 72 + 4 * 9 + 8] +

		tl2.y * kernelsLA[3 * 72 + 5 * 9 + 0] + tc2.y * kernelsLA[3 * 72 + 5 * 9 + 1] + tr2.y * kernelsLA[3 * 72 + 5 * 9 + 2] +
		ml2.y * kernelsLA[3 * 72 + 5 * 9 + 3] + mc2.y * kernelsLA[3 * 72 + 5 * 9 + 4] + mr2.y * kernelsLA[3 * 72 + 5 * 9 + 5] +
		bl2.y * kernelsLA[3 * 72 + 5 * 9 + 6] + bc2.y * kernelsLA[3 * 72 + 5 * 9 + 7] + br2.y * kernelsLA[3 * 72 + 5 * 9 + 8] +

		tl2.z * kernelsLA[3 * 72 + 6 * 9 + 0] + tc2.z * kernelsLA[3 * 72 + 6 * 9 + 1] + tr2.z * kernelsLA[3 * 72 + 6 * 9 + 2] +
		ml2.z * kernelsLA[3 * 72 + 6 * 9 + 3] + mc2.z * kernelsLA[3 * 72 + 6 * 9 + 4] + mr2.z * kernelsLA[3 * 72 + 6 * 9 + 5] +
		bl2.z * kernelsLA[3 * 72 + 6 * 9 + 6] + bc2.z * kernelsLA[3 * 72 + 6 * 9 + 7] + br2.z * kernelsLA[3 * 72 + 6 * 9 + 8] +

		tl2.w * kernelsLA[3 * 72 + 7 * 9 + 0] + tc2.w * kernelsLA[3 * 72 + 7 * 9 + 1] + tr2.w * kernelsLA[3 * 72 + 7 * 9 + 2] +
		ml2.w * kernelsLA[3 * 72 + 7 * 9 + 3] + mc2.w * kernelsLA[3 * 72 + 7 * 9 + 4] + mr2.w * kernelsLA[3 * 72 + 7 * 9 + 5] +
		bl2.w * kernelsLA[3 * 72 + 7 * 9 + 6] + bc2.w * kernelsLA[3 * 72 + 7 * 9 + 7] + br2.w * kernelsLA[3 * 72 + 7 * 9 + 8] + biasLA.w
	));

	float4 target2 = RELU(float4(
		tl1.x * kernelsLB[0 * 72 + 0 * 9 + 0] + tc1.x * kernelsLB[0 * 72 + 0 * 9 + 1] + tr1.x * kernelsLB[0 * 72 + 0 * 9 + 2] +
		ml1.x * kernelsLB[0 * 72 + 0 * 9 + 3] + mc1.x * kernelsLB[0 * 72 + 0 * 9 + 4] + mr1.x * kernelsLB[0 * 72 + 0 * 9 + 5] +
		bl1.x * kernelsLB[0 * 72 + 0 * 9 + 6] + bc1.x * kernelsLB[0 * 72 + 0 * 9 + 7] + br1.x * kernelsLB[0 * 72 + 0 * 9 + 8] +

		tl1.y * kernelsLB[0 * 72 + 1 * 9 + 0] + tc1.y * kernelsLB[0 * 72 + 1 * 9 + 1] + tr1.y * kernelsLB[0 * 72 + 1 * 9 + 2] +
		ml1.y * kernelsLB[0 * 72 + 1 * 9 + 3] + mc1.y * kernelsLB[0 * 72 + 1 * 9 + 4] + mr1.y * kernelsLB[0 * 72 + 1 * 9 + 5] +
		bl1.y * kernelsLB[0 * 72 + 1 * 9 + 6] + bc1.y * kernelsLB[0 * 72 + 1 * 9 + 7] + br1.y * kernelsLB[0 * 72 + 1 * 9 + 8] +

		tl1.z * kernelsLB[0 * 72 + 2 * 9 + 0] + tc1.z * kernelsLB[0 * 72 + 2 * 9 + 1] + tr1.z * kernelsLB[0 * 72 + 2 * 9 + 2] +
		ml1.z * kernelsLB[0 * 72 + 2 * 9 + 3] + mc1.z * kernelsLB[0 * 72 + 2 * 9 + 4] + mr1.z * kernelsLB[0 * 72 + 2 * 9 + 5] +
		bl1.z * kernelsLB[0 * 72 + 2 * 9 + 6] + bc1.z * kernelsLB[0 * 72 + 2 * 9 + 7] + br1.z * kernelsLB[0 * 72 + 2 * 9 + 8] +

		tl1.w * kernelsLB[0 * 72 + 3 * 9 + 0] + tc1.w * kernelsLB[0 * 72 + 3 * 9 + 1] + tr1.w * kernelsLB[0 * 72 + 3 * 9 + 2] +
		ml1.w * kernelsLB[0 * 72 + 3 * 9 + 3] + mc1.w * kernelsLB[0 * 72 + 3 * 9 + 4] + mr1.w * kernelsLB[0 * 72 + 3 * 9 + 5] +
		bl1.w * kernelsLB[0 * 72 + 3 * 9 + 6] + bc1.w * kernelsLB[0 * 72 + 3 * 9 + 7] + br1.w * kernelsLB[0 * 72 + 3 * 9 + 8] +

		tl2.x * kernelsLB[0 * 72 + 4 * 9 + 0] + tc2.x * kernelsLB[0 * 72 + 4 * 9 + 1] + tr2.x * kernelsLB[0 * 72 + 4 * 9 + 2] +
		ml2.x * kernelsLB[0 * 72 + 4 * 9 + 3] + mc2.x * kernelsLB[0 * 72 + 4 * 9 + 4] + mr2.x * kernelsLB[0 * 72 + 4 * 9 + 5] +
		bl2.x * kernelsLB[0 * 72 + 4 * 9 + 6] + bc2.x * kernelsLB[0 * 72 + 4 * 9 + 7] + br2.x * kernelsLB[0 * 72 + 4 * 9 + 8] +

		tl2.y * kernelsLB[0 * 72 + 5 * 9 + 0] + tc2.y * kernelsLB[0 * 72 + 5 * 9 + 1] + tr2.y * kernelsLB[0 * 72 + 5 * 9 + 2] +
		ml2.y * kernelsLB[0 * 72 + 5 * 9 + 3] + mc2.y * kernelsLB[0 * 72 + 5 * 9 + 4] + mr2.y * kernelsLB[0 * 72 + 5 * 9 + 5] +
		bl2.y * kernelsLB[0 * 72 + 5 * 9 + 6] + bc2.y * kernelsLB[0 * 72 + 5 * 9 + 7] + br2.y * kernelsLB[0 * 72 + 5 * 9 + 8] +

		tl2.z * kernelsLB[0 * 72 + 6 * 9 + 0] + tc2.z * kernelsLB[0 * 72 + 6 * 9 + 1] + tr2.z * kernelsLB[0 * 72 + 6 * 9 + 2] +
		ml2.z * kernelsLB[0 * 72 + 6 * 9 + 3] + mc2.z * kernelsLB[0 * 72 + 6 * 9 + 4] + mr2.z * kernelsLB[0 * 72 + 6 * 9 + 5] +
		bl2.z * kernelsLB[0 * 72 + 6 * 9 + 6] + bc2.z * kernelsLB[0 * 72 + 6 * 9 + 7] + br2.z * kernelsLB[0 * 72 + 6 * 9 + 8] +

		tl2.w * kernelsLB[0 * 72 + 7 * 9 + 0] + tc2.w * kernelsLB[0 * 72 + 7 * 9 + 1] + tr2.w * kernelsLB[0 * 72 + 7 * 9 + 2] +
		ml2.w * kernelsLB[0 * 72 + 7 * 9 + 3] + mc2.w * kernelsLB[0 * 72 + 7 * 9 + 4] + mr2.w * kernelsLB[0 * 72 + 7 * 9 + 5] +
		bl2.w * kernelsLB[0 * 72 + 7 * 9 + 6] + bc2.w * kernelsLB[0 * 72 + 7 * 9 + 7] + br2.w * kernelsLB[0 * 72 + 7 * 9 + 8] + biasLB.x
		,
		tl1.x * kernelsLB[1 * 72 + 0 * 9 + 0] + tc1.x * kernelsLB[1 * 72 + 0 * 9 + 1] + tr1.x * kernelsLB[1 * 72 + 0 * 9 + 2] +
		ml1.x * kernelsLB[1 * 72 + 0 * 9 + 3] + mc1.x * kernelsLB[1 * 72 + 0 * 9 + 4] + mr1.x * kernelsLB[1 * 72 + 0 * 9 + 5] +
		bl1.x * kernelsLB[1 * 72 + 0 * 9 + 6] + bc1.x * kernelsLB[1 * 72 + 0 * 9 + 7] + br1.x * kernelsLB[1 * 72 + 0 * 9 + 8] +

		tl1.y * kernelsLB[1 * 72 + 1 * 9 + 0] + tc1.y * kernelsLB[1 * 72 + 1 * 9 + 1] + tr1.y * kernelsLB[1 * 72 + 1 * 9 + 2] +
		ml1.y * kernelsLB[1 * 72 + 1 * 9 + 3] + mc1.y * kernelsLB[1 * 72 + 1 * 9 + 4] + mr1.y * kernelsLB[1 * 72 + 1 * 9 + 5] +
		bl1.y * kernelsLB[1 * 72 + 1 * 9 + 6] + bc1.y * kernelsLB[1 * 72 + 1 * 9 + 7] + br1.y * kernelsLB[1 * 72 + 1 * 9 + 8] +

		tl1.z * kernelsLB[1 * 72 + 2 * 9 + 0] + tc1.z * kernelsLB[1 * 72 + 2 * 9 + 1] + tr1.z * kernelsLB[1 * 72 + 2 * 9 + 2] +
		ml1.z * kernelsLB[1 * 72 + 2 * 9 + 3] + mc1.z * kernelsLB[1 * 72 + 2 * 9 + 4] + mr1.z * kernelsLB[1 * 72 + 2 * 9 + 5] +
		bl1.z * kernelsLB[1 * 72 + 2 * 9 + 6] + bc1.z * kernelsLB[1 * 72 + 2 * 9 + 7] + br1.z * kernelsLB[1 * 72 + 2 * 9 + 8] +

		tl1.w * kernelsLB[1 * 72 + 3 * 9 + 0] + tc1.w * kernelsLB[1 * 72 + 3 * 9 + 1] + tr1.w * kernelsLB[1 * 72 + 3 * 9 + 2] +
		ml1.w * kernelsLB[1 * 72 + 3 * 9 + 3] + mc1.w * kernelsLB[1 * 72 + 3 * 9 + 4] + mr1.w * kernelsLB[1 * 72 + 3 * 9 + 5] +
		bl1.w * kernelsLB[1 * 72 + 3 * 9 + 6] + bc1.w * kernelsLB[1 * 72 + 3 * 9 + 7] + br1.w * kernelsLB[1 * 72 + 3 * 9 + 8] +

		tl2.x * kernelsLB[1 * 72 + 4 * 9 + 0] + tc2.x * kernelsLB[1 * 72 + 4 * 9 + 1] + tr2.x * kernelsLB[1 * 72 + 4 * 9 + 2] +
		ml2.x * kernelsLB[1 * 72 + 4 * 9 + 3] + mc2.x * kernelsLB[1 * 72 + 4 * 9 + 4] + mr2.x * kernelsLB[1 * 72 + 4 * 9 + 5] +
		bl2.x * kernelsLB[1 * 72 + 4 * 9 + 6] + bc2.x * kernelsLB[1 * 72 + 4 * 9 + 7] + br2.x * kernelsLB[1 * 72 + 4 * 9 + 8] +

		tl2.y * kernelsLB[1 * 72 + 5 * 9 + 0] + tc2.y * kernelsLB[1 * 72 + 5 * 9 + 1] + tr2.y * kernelsLB[1 * 72 + 5 * 9 + 2] +
		ml2.y * kernelsLB[1 * 72 + 5 * 9 + 3] + mc2.y * kernelsLB[1 * 72 + 5 * 9 + 4] + mr2.y * kernelsLB[1 * 72 + 5 * 9 + 5] +
		bl2.y * kernelsLB[1 * 72 + 5 * 9 + 6] + bc2.y * kernelsLB[1 * 72 + 5 * 9 + 7] + br2.y * kernelsLB[1 * 72 + 5 * 9 + 8] +

		tl2.z * kernelsLB[1 * 72 + 6 * 9 + 0] + tc2.z * kernelsLB[1 * 72 + 6 * 9 + 1] + tr2.z * kernelsLB[1 * 72 + 6 * 9 + 2] +
		ml2.z * kernelsLB[1 * 72 + 6 * 9 + 3] + mc2.z * kernelsLB[1 * 72 + 6 * 9 + 4] + mr2.z * kernelsLB[1 * 72 + 6 * 9 + 5] +
		bl2.z * kernelsLB[1 * 72 + 6 * 9 + 6] + bc2.z * kernelsLB[1 * 72 + 6 * 9 + 7] + br2.z * kernelsLB[1 * 72 + 6 * 9 + 8] +

		tl2.w * kernelsLB[1 * 72 + 7 * 9 + 0] + tc2.w * kernelsLB[1 * 72 + 7 * 9 + 1] + tr2.w * kernelsLB[1 * 72 + 7 * 9 + 2] +
		ml2.w * kernelsLB[1 * 72 + 7 * 9 + 3] + mc2.w * kernelsLB[1 * 72 + 7 * 9 + 4] + mr2.w * kernelsLB[1 * 72 + 7 * 9 + 5] +
		bl2.w * kernelsLB[1 * 72 + 7 * 9 + 6] + bc2.w * kernelsLB[1 * 72 + 7 * 9 + 7] + br2.w * kernelsLB[1 * 72 + 7 * 9 + 8] + biasLB.y
		,
		tl1.x * kernelsLB[2 * 72 + 0 * 9 + 0] + tc1.x * kernelsLB[2 * 72 + 0 * 9 + 1] + tr1.x * kernelsLB[2 * 72 + 0 * 9 + 2] +
		ml1.x * kernelsLB[2 * 72 + 0 * 9 + 3] + mc1.x * kernelsLB[2 * 72 + 0 * 9 + 4] + mr1.x * kernelsLB[2 * 72 + 0 * 9 + 5] +
		bl1.x * kernelsLB[2 * 72 + 0 * 9 + 6] + bc1.x * kernelsLB[2 * 72 + 0 * 9 + 7] + br1.x * kernelsLB[2 * 72 + 0 * 9 + 8] +

		tl1.y * kernelsLB[2 * 72 + 1 * 9 + 0] + tc1.y * kernelsLB[2 * 72 + 1 * 9 + 1] + tr1.y * kernelsLB[2 * 72 + 1 * 9 + 2] +
		ml1.y * kernelsLB[2 * 72 + 1 * 9 + 3] + mc1.y * kernelsLB[2 * 72 + 1 * 9 + 4] + mr1.y * kernelsLB[2 * 72 + 1 * 9 + 5] +
		bl1.y * kernelsLB[2 * 72 + 1 * 9 + 6] + bc1.y * kernelsLB[2 * 72 + 1 * 9 + 7] + br1.y * kernelsLB[2 * 72 + 1 * 9 + 8] +

		tl1.z * kernelsLB[2 * 72 + 2 * 9 + 0] + tc1.z * kernelsLB[2 * 72 + 2 * 9 + 1] + tr1.z * kernelsLB[2 * 72 + 2 * 9 + 2] +
		ml1.z * kernelsLB[2 * 72 + 2 * 9 + 3] + mc1.z * kernelsLB[2 * 72 + 2 * 9 + 4] + mr1.z * kernelsLB[2 * 72 + 2 * 9 + 5] +
		bl1.z * kernelsLB[2 * 72 + 2 * 9 + 6] + bc1.z * kernelsLB[2 * 72 + 2 * 9 + 7] + br1.z * kernelsLB[2 * 72 + 2 * 9 + 8] +

		tl1.w * kernelsLB[2 * 72 + 3 * 9 + 0] + tc1.w * kernelsLB[2 * 72 + 3 * 9 + 1] + tr1.w * kernelsLB[2 * 72 + 3 * 9 + 2] +
		ml1.w * kernelsLB[2 * 72 + 3 * 9 + 3] + mc1.w * kernelsLB[2 * 72 + 3 * 9 + 4] + mr1.w * kernelsLB[2 * 72 + 3 * 9 + 5] +
		bl1.w * kernelsLB[2 * 72 + 3 * 9 + 6] + bc1.w * kernelsLB[2 * 72 + 3 * 9 + 7] + br1.w * kernelsLB[2 * 72 + 3 * 9 + 8] +

		tl2.x * kernelsLB[2 * 72 + 4 * 9 + 0] + tc2.x * kernelsLB[2 * 72 + 4 * 9 + 1] + tr2.x * kernelsLB[2 * 72 + 4 * 9 + 2] +
		ml2.x * kernelsLB[2 * 72 + 4 * 9 + 3] + mc2.x * kernelsLB[2 * 72 + 4 * 9 + 4] + mr2.x * kernelsLB[2 * 72 + 4 * 9 + 5] +
		bl2.x * kernelsLB[2 * 72 + 4 * 9 + 6] + bc2.x * kernelsLB[2 * 72 + 4 * 9 + 7] + br2.x * kernelsLB[2 * 72 + 4 * 9 + 8] +

		tl2.y * kernelsLB[2 * 72 + 5 * 9 + 0] + tc2.y * kernelsLB[2 * 72 + 5 * 9 + 1] + tr2.y * kernelsLB[2 * 72 + 5 * 9 + 2] +
		ml2.y * kernelsLB[2 * 72 + 5 * 9 + 3] + mc2.y * kernelsLB[2 * 72 + 5 * 9 + 4] + mr2.y * kernelsLB[2 * 72 + 5 * 9 + 5] +
		bl2.y * kernelsLB[2 * 72 + 5 * 9 + 6] + bc2.y * kernelsLB[2 * 72 + 5 * 9 + 7] + br2.y * kernelsLB[2 * 72 + 5 * 9 + 8] +

		tl2.z * kernelsLB[2 * 72 + 6 * 9 + 0] + tc2.z * kernelsLB[2 * 72 + 6 * 9 + 1] + tr2.z * kernelsLB[2 * 72 + 6 * 9 + 2] +
		ml2.z * kernelsLB[2 * 72 + 6 * 9 + 3] + mc2.z * kernelsLB[2 * 72 + 6 * 9 + 4] + mr2.z * kernelsLB[2 * 72 + 6 * 9 + 5] +
		bl2.z * kernelsLB[2 * 72 + 6 * 9 + 6] + bc2.z * kernelsLB[2 * 72 + 6 * 9 + 7] + br2.z * kernelsLB[2 * 72 + 6 * 9 + 8] +

		tl2.w * kernelsLB[2 * 72 + 7 * 9 + 0] + tc2.w * kernelsLB[2 * 72 + 7 * 9 + 1] + tr2.w * kernelsLB[2 * 72 + 7 * 9 + 2] +
		ml2.w * kernelsLB[2 * 72 + 7 * 9 + 3] + mc2.w * kernelsLB[2 * 72 + 7 * 9 + 4] + mr2.w * kernelsLB[2 * 72 + 7 * 9 + 5] +
		bl2.w * kernelsLB[2 * 72 + 7 * 9 + 6] + bc2.w * kernelsLB[2 * 72 + 7 * 9 + 7] + br2.w * kernelsLB[2 * 72 + 7 * 9 + 8] + biasLB.z
		,
		tl1.x * kernelsLB[3 * 72 + 0 * 9 + 0] + tc1.x * kernelsLB[3 * 72 + 0 * 9 + 1] + tr1.x * kernelsLB[3 * 72 + 0 * 9 + 2] +
		ml1.x * kernelsLB[3 * 72 + 0 * 9 + 3] + mc1.x * kernelsLB[3 * 72 + 0 * 9 + 4] + mr1.x * kernelsLB[3 * 72 + 0 * 9 + 5] +
		bl1.x * kernelsLB[3 * 72 + 0 * 9 + 6] + bc1.x * kernelsLB[3 * 72 + 0 * 9 + 7] + br1.x * kernelsLB[3 * 72 + 0 * 9 + 8] +

		tl1.y * kernelsLB[3 * 72 + 1 * 9 + 0] + tc1.y * kernelsLB[3 * 72 + 1 * 9 + 1] + tr1.y * kernelsLB[3 * 72 + 1 * 9 + 2] +
		ml1.y * kernelsLB[3 * 72 + 1 * 9 + 3] + mc1.y * kernelsLB[3 * 72 + 1 * 9 + 4] + mr1.y * kernelsLB[3 * 72 + 1 * 9 + 5] +
		bl1.y * kernelsLB[3 * 72 + 1 * 9 + 6] + bc1.y * kernelsLB[3 * 72 + 1 * 9 + 7] + br1.y * kernelsLB[3 * 72 + 1 * 9 + 8] +

		tl1.z * kernelsLB[3 * 72 + 2 * 9 + 0] + tc1.z * kernelsLB[3 * 72 + 2 * 9 + 1] + tr1.z * kernelsLB[3 * 72 + 2 * 9 + 2] +
		ml1.z * kernelsLB[3 * 72 + 2 * 9 + 3] + mc1.z * kernelsLB[3 * 72 + 2 * 9 + 4] + mr1.z * kernelsLB[3 * 72 + 2 * 9 + 5] +
		bl1.z * kernelsLB[3 * 72 + 2 * 9 + 6] + bc1.z * kernelsLB[3 * 72 + 2 * 9 + 7] + br1.z * kernelsLB[3 * 72 + 2 * 9 + 8] +

		tl1.w * kernelsLB[3 * 72 + 3 * 9 + 0] + tc1.w * kernelsLB[3 * 72 + 3 * 9 + 1] + tr1.w * kernelsLB[3 * 72 + 3 * 9 + 2] +
		ml1.w * kernelsLB[3 * 72 + 3 * 9 + 3] + mc1.w * kernelsLB[3 * 72 + 3 * 9 + 4] + mr1.w * kernelsLB[3 * 72 + 3 * 9 + 5] +
		bl1.w * kernelsLB[3 * 72 + 3 * 9 + 6] + bc1.w * kernelsLB[3 * 72 + 3 * 9 + 7] + br1.w * kernelsLB[3 * 72 + 3 * 9 + 8] +

		tl2.x * kernelsLB[3 * 72 + 4 * 9 + 0] + tc2.x * kernelsLB[3 * 72 + 4 * 9 + 1] + tr2.x * kernelsLB[3 * 72 + 4 * 9 + 2] +
		ml2.x * kernelsLB[3 * 72 + 4 * 9 + 3] + mc2.x * kernelsLB[3 * 72 + 4 * 9 + 4] + mr2.x * kernelsLB[3 * 72 + 4 * 9 + 5] +
		bl2.x * kernelsLB[3 * 72 + 4 * 9 + 6] + bc2.x * kernelsLB[3 * 72 + 4 * 9 + 7] + br2.x * kernelsLB[3 * 72 + 4 * 9 + 8] +

		tl2.y * kernelsLB[3 * 72 + 5 * 9 + 0] + tc2.y * kernelsLB[3 * 72 + 5 * 9 + 1] + tr2.y * kernelsLB[3 * 72 + 5 * 9 + 2] +
		ml2.y * kernelsLB[3 * 72 + 5 * 9 + 3] + mc2.y * kernelsLB[3 * 72 + 5 * 9 + 4] + mr2.y * kernelsLB[3 * 72 + 5 * 9 + 5] +
		bl2.y * kernelsLB[3 * 72 + 5 * 9 + 6] + bc2.y * kernelsLB[3 * 72 + 5 * 9 + 7] + br2.y * kernelsLB[3 * 72 + 5 * 9 + 8] +

		tl2.z * kernelsLB[3 * 72 + 6 * 9 + 0] + tc2.z * kernelsLB[3 * 72 + 6 * 9 + 1] + tr2.z * kernelsLB[3 * 72 + 6 * 9 + 2] +
		ml2.z * kernelsLB[3 * 72 + 6 * 9 + 3] + mc2.z * kernelsLB[3 * 72 + 6 * 9 + 4] + mr2.z * kernelsLB[3 * 72 + 6 * 9 + 5] +
		bl2.z * kernelsLB[3 * 72 + 6 * 9 + 6] + bc2.z * kernelsLB[3 * 72 + 6 * 9 + 7] + br2.z * kernelsLB[3 * 72 + 6 * 9 + 8] +

		tl2.w * kernelsLB[3 * 72 + 7 * 9 + 0] + tc2.w * kernelsLB[3 * 72 + 7 * 9 + 1] + tr2.w * kernelsLB[3 * 72 + 7 * 9 + 2] +
		ml2.w * kernelsLB[3 * 72 + 7 * 9 + 3] + mc2.w * kernelsLB[3 * 72 + 7 * 9 + 4] + mr2.w * kernelsLB[3 * 72 + 7 * 9 + 5] +
		bl2.w * kernelsLB[3 * 72 + 7 * 9 + 6] + bc2.w * kernelsLB[3 * 72 + 7 * 9 + 7] + br2.w * kernelsLB[3 * 72 + 7 * 9 + 8] + biasLB.w
	));

	tex1[gxy] = target1;
	tex2[gxy] = target2;
}


//!PASS 4
//!IN tex1, tex2
//!OUT tex3, tex4
//!BLOCK_SIZE 8
//!NUM_THREADS 64

const static float kernelsLA[9 * 8 * 4] = {
	 2.3898e-02,  1.2411e-02, -3.2770e-02,
	-2.6029e-01,  3.2690e-01, -1.8246e-01,
	 1.1224e-02,  8.0193e-02, -5.0412e-02,
	-9.3849e-02,  2.0325e-02,  2.6309e-02,
	 1.2266e-02,  1.7698e-01,  2.7049e-01,
	 1.2918e-01,  2.0190e-01,  2.7352e-01,
	-7.2100e-02,  1.3357e-01, -1.3702e-01,
	 2.2527e-01,  1.5821e-01, -2.3104e-01,
	 1.0182e-02, -1.5499e-01,  7.1906e-02,
	 1.5865e-01,  7.0950e-02, -6.3336e-02,
	 2.2661e-01, -4.2997e-01, -4.2013e-01,
	 1.7549e-02, -1.3142e-01, -3.1663e-01,
	 1.3617e-01,  1.4229e-01, -1.0707e-02,
	-1.0986e-02,  2.8816e-01, -3.6239e-01,
	 2.2579e-02, -1.4332e-02,  7.1339e-03,
	-1.4357e-01, -9.7608e-02,  1.4646e-01,
	-5.3856e-02,  3.3898e-01, -2.4936e-01,
	-2.9500e-02,  2.1799e-02,  1.1901e-02,
	 3.6996e-02,  2.1291e-02,  3.2150e-02,
	 9.8375e-02,  2.4476e-01,  2.2896e-01,
	 1.8392e-01, -7.4510e-02, -1.0152e-01,
	 4.4757e-02, -4.8053e-03, -6.7254e-02,
	-4.8370e-02, -7.8975e-02, -3.6007e-01,
	-3.8160e-02,  8.7707e-02, -1.4986e-01,
	-8.7544e-03, -4.3522e-02,  7.3822e-02,
	-1.4523e-01,  1.1433e-01,  4.4109e-02,
	-1.6025e-03,  2.5459e-02, -9.3562e-02,
	-2.9192e-02, -1.0975e-01, -5.0943e-02,
	-1.1215e-01,  1.9907e-01,  7.9934e-02,
	 3.7066e-02,  3.0796e-01, -1.4034e-01,
	-8.2315e-02, -2.0182e-02, -1.2824e-02,
	-4.8007e-03,  1.2655e-01, -2.5157e-02,
	 2.7796e-02, -4.3032e-02,  2.5397e-02,
	 6.9377e-02,  2.3642e-01,  1.2713e-01,
	 2.7878e-02, -1.5325e-01, -1.4871e-01,
	 1.5800e-02, -4.5935e-02,  1.7370e-01,
	 4.8058e-02, -1.8725e-01, -6.7048e-03,
	-1.3932e-01, -6.0768e-02, -1.6976e-01,
	-2.1189e-02,  1.0311e-02, -2.2970e-02,
	-7.0546e-03,  7.9481e-02,  1.2146e-02,
	 4.2666e-02,  3.5383e-01,  1.4381e-01,
	 5.4384e-02, -9.3862e-02,  4.8870e-03,
	 2.1141e-02, -6.6826e-02, -1.8526e-01,
	 1.3309e-01,  3.3452e-01,  1.1058e-02,
	-1.6967e-02,  1.1094e-01,  5.3230e-02,
	 3.0409e-02, -4.7613e-02, -1.7737e-01,
	-1.6678e-02, -7.8644e-02,  1.1743e-01,
	 7.3322e-02, -1.1354e-01, -1.5737e-02,
	-1.2397e-03, -1.4685e-02, -1.0192e-02,
	 1.6045e-01,  3.6331e-02,  1.2219e-01,
	 1.3123e-01,  5.7578e-02,  1.0291e-01,
	 1.7424e-01,  1.0688e-01,  1.4263e-01,
	 8.9942e-02, -2.7141e-02,  3.1238e-02,
	-4.0240e-02, -1.0930e-01, -2.1276e-01,
	 1.0357e-01,  5.7673e-02,  1.0356e-02,
	-2.0864e-01, -1.9405e-01,  2.5094e-01,
	-4.8277e-03, -1.3758e-01,  1.1562e-01,
	-1.0358e-01,  2.0631e-01, -9.1445e-03,
	-1.7602e-01,  1.0200e-01,  3.0032e-02,
	-1.1495e-02, -4.5077e-02, -6.4748e-02,
	-2.3072e-02, -3.2342e-02,  1.4503e-02,
	-3.7052e-02, -1.2206e-01,  5.5395e-02,
	 2.8331e-02, -4.2812e-03,  6.9807e-02,
	 4.3593e-02, -6.7373e-03,  1.2760e-02,
	 3.2896e-03, -2.4007e-01, -5.2920e-02,
	 2.5193e-02, -2.1480e-01,  8.4654e-02,
	 2.2642e-02,  8.2132e-02, -2.3864e-02,
	-2.9726e-01,  8.0405e-02, -1.3190e-02,
	-1.1310e-01, -4.4342e-01, -6.3536e-02,
	-6.7090e-02,  1.1797e-01,  1.5315e-01,
	 7.7829e-02, -1.4494e-01,  1.0233e-01,
	 9.7059e-02,  1.2772e-01, -2.4394e-02,
	-2.6179e-02,  2.6721e-02,  1.1707e-02,
	-4.8024e-02, -2.3366e-01, -1.6978e-01,
	-2.4402e-01, -2.8572e-01, -2.4053e-02,
	-2.7451e-03,  7.1959e-02,  4.4706e-02,
	-1.9900e-01,  2.1353e-01,  1.0625e-01,
	 4.0246e-01,  4.2323e-01,  3.4046e-02,
	-1.6943e-01, -2.0221e-01, -1.6369e-01,
	 1.3882e-01,  2.1717e-01, -1.3581e-01,
	 1.3975e-01,  1.1980e-01,  1.8888e-02,
	-1.8110e-01, -2.6143e-01, -1.0109e-01,
	 5.5844e-02, -1.2175e-01,  3.4447e-02,
	 8.9688e-02,  2.4641e-01,  2.3287e-01,
	-5.8259e-02, -1.3656e-01, -1.3936e-02,
	-8.3429e-03,  2.3026e-01,  1.2302e-01,
	-2.2969e-02,  6.0932e-02,  3.4749e-02,
	 1.2910e-01,  2.4008e-01,  1.8908e-01,
	-5.8776e-02,  3.8121e-01,  8.1312e-02,
	 9.1175e-02, -1.8729e-02, -4.6156e-02,
	 3.7493e-02, -3.5877e-02, -9.9651e-03,
	 1.5864e-01,  1.3611e-01,  6.7880e-02,
	 2.2216e-01,  9.3697e-02,  7.4782e-02,
	-1.0861e-01, -2.5824e-01,  6.6455e-02,
	 9.2238e-02, -2.3448e-01, -3.4057e-01,
	-2.9658e-01,  9.4698e-03,  1.9315e-01
};

const static float4 biasLA = { -5.8305e-03, -8.6574e-02,  4.2228e-02, -4.3500e-02 };

const static float kernelsLB[9 * 8 * 4] = {
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

const static float4 biasLB = { -8.1892e-04, 3.3171e-03, -1.1582e-02, -4.1205e-40 };


void Pass4(uint2 blockStart, uint3 threadId) {
	uint2 gxy = Rmp8x8(threadId.x) + blockStart;
	uint2 inputSize = GetInputSize();
	if (gxy.x >= inputSize.x || gxy.y >= inputSize.y) {
		return;
	}

	float2 inputPt = GetInputPt();
	float2 pos = (gxy + 0.5f) * inputPt;

	// [tl, tc, tr]
	// [ml, mc, mr]
	// [bl, bc, br]
	float4 tl1 = tex1.SampleLevel(sam, pos + float2(-inputPt.x, -inputPt.y), 0);
	float4 ml1 = tex1.SampleLevel(sam, pos + float2(-inputPt.x, 0), 0);
	float4 bl1 = tex1.SampleLevel(sam, pos + float2(-inputPt.x, inputPt.y), 0);
	float4 tc1 = tex1.SampleLevel(sam, pos + float2(0, -inputPt.y), 0);
	float4 mc1 = tex1.SampleLevel(sam, pos, 0);
	float4 bc1 = tex1.SampleLevel(sam, pos + float2(0, inputPt.y), 0);
	float4 tr1 = tex1.SampleLevel(sam, pos + float2(inputPt.x, -inputPt.y), 0);
	float4 mr1 = tex1.SampleLevel(sam, pos + float2(inputPt.x, 0), 0);
	float4 br1 = tex1.SampleLevel(sam, pos + float2(inputPt.x, inputPt.y), 0);

	float4 tl2 = tex2.SampleLevel(sam, pos + float2(-inputPt.x, -inputPt.y), 0);
	float4 ml2 = tex2.SampleLevel(sam, pos + float2(-inputPt.x, 0), 0);
	float4 bl2 = tex2.SampleLevel(sam, pos + float2(-inputPt.x, inputPt.y), 0);
	float4 tc2 = tex2.SampleLevel(sam, pos + float2(0, -inputPt.y), 0);
	float4 mc2 = tex2.SampleLevel(sam, pos, 0);
	float4 bc2 = tex2.SampleLevel(sam, pos + float2(0, inputPt.y), 0);
	float4 tr2 = tex2.SampleLevel(sam, pos + float2(inputPt.x, -inputPt.y), 0);
	float4 mr2 = tex2.SampleLevel(sam, pos + float2(inputPt.x, 0), 0);
	float4 br2 = tex2.SampleLevel(sam, pos + float2(inputPt.x, inputPt.y), 0);

	float4 target1 = RELU(float4(
		tl1.x * kernelsLA[0 * 72 + 0 * 9 + 0] + tc1.x * kernelsLA[0 * 72 + 0 * 9 + 1] + tr1.x * kernelsLA[0 * 72 + 0 * 9 + 2] +
		ml1.x * kernelsLA[0 * 72 + 0 * 9 + 3] + mc1.x * kernelsLA[0 * 72 + 0 * 9 + 4] + mr1.x * kernelsLA[0 * 72 + 0 * 9 + 5] +
		bl1.x * kernelsLA[0 * 72 + 0 * 9 + 6] + bc1.x * kernelsLA[0 * 72 + 0 * 9 + 7] + br1.x * kernelsLA[0 * 72 + 0 * 9 + 8] +

		tl1.y * kernelsLA[0 * 72 + 1 * 9 + 0] + tc1.y * kernelsLA[0 * 72 + 1 * 9 + 1] + tr1.y * kernelsLA[0 * 72 + 1 * 9 + 2] +
		ml1.y * kernelsLA[0 * 72 + 1 * 9 + 3] + mc1.y * kernelsLA[0 * 72 + 1 * 9 + 4] + mr1.y * kernelsLA[0 * 72 + 1 * 9 + 5] +
		bl1.y * kernelsLA[0 * 72 + 1 * 9 + 6] + bc1.y * kernelsLA[0 * 72 + 1 * 9 + 7] + br1.y * kernelsLA[0 * 72 + 1 * 9 + 8] +

		tl1.z * kernelsLA[0 * 72 + 2 * 9 + 0] + tc1.z * kernelsLA[0 * 72 + 2 * 9 + 1] + tr1.z * kernelsLA[0 * 72 + 2 * 9 + 2] +
		ml1.z * kernelsLA[0 * 72 + 2 * 9 + 3] + mc1.z * kernelsLA[0 * 72 + 2 * 9 + 4] + mr1.z * kernelsLA[0 * 72 + 2 * 9 + 5] +
		bl1.z * kernelsLA[0 * 72 + 2 * 9 + 6] + bc1.z * kernelsLA[0 * 72 + 2 * 9 + 7] + br1.z * kernelsLA[0 * 72 + 2 * 9 + 8] +

		tl1.w * kernelsLA[0 * 72 + 3 * 9 + 0] + tc1.w * kernelsLA[0 * 72 + 3 * 9 + 1] + tr1.w * kernelsLA[0 * 72 + 3 * 9 + 2] +
		ml1.w * kernelsLA[0 * 72 + 3 * 9 + 3] + mc1.w * kernelsLA[0 * 72 + 3 * 9 + 4] + mr1.w * kernelsLA[0 * 72 + 3 * 9 + 5] +
		bl1.w * kernelsLA[0 * 72 + 3 * 9 + 6] + bc1.w * kernelsLA[0 * 72 + 3 * 9 + 7] + br1.w * kernelsLA[0 * 72 + 3 * 9 + 8] +

		tl2.x * kernelsLA[0 * 72 + 4 * 9 + 0] + tc2.x * kernelsLA[0 * 72 + 4 * 9 + 1] + tr2.x * kernelsLA[0 * 72 + 4 * 9 + 2] +
		ml2.x * kernelsLA[0 * 72 + 4 * 9 + 3] + mc2.x * kernelsLA[0 * 72 + 4 * 9 + 4] + mr2.x * kernelsLA[0 * 72 + 4 * 9 + 5] +
		bl2.x * kernelsLA[0 * 72 + 4 * 9 + 6] + bc2.x * kernelsLA[0 * 72 + 4 * 9 + 7] + br2.x * kernelsLA[0 * 72 + 4 * 9 + 8] +

		tl2.y * kernelsLA[0 * 72 + 5 * 9 + 0] + tc2.y * kernelsLA[0 * 72 + 5 * 9 + 1] + tr2.y * kernelsLA[0 * 72 + 5 * 9 + 2] +
		ml2.y * kernelsLA[0 * 72 + 5 * 9 + 3] + mc2.y * kernelsLA[0 * 72 + 5 * 9 + 4] + mr2.y * kernelsLA[0 * 72 + 5 * 9 + 5] +
		bl2.y * kernelsLA[0 * 72 + 5 * 9 + 6] + bc2.y * kernelsLA[0 * 72 + 5 * 9 + 7] + br2.y * kernelsLA[0 * 72 + 5 * 9 + 8] +

		tl2.z * kernelsLA[0 * 72 + 6 * 9 + 0] + tc2.z * kernelsLA[0 * 72 + 6 * 9 + 1] + tr2.z * kernelsLA[0 * 72 + 6 * 9 + 2] +
		ml2.z * kernelsLA[0 * 72 + 6 * 9 + 3] + mc2.z * kernelsLA[0 * 72 + 6 * 9 + 4] + mr2.z * kernelsLA[0 * 72 + 6 * 9 + 5] +
		bl2.z * kernelsLA[0 * 72 + 6 * 9 + 6] + bc2.z * kernelsLA[0 * 72 + 6 * 9 + 7] + br2.z * kernelsLA[0 * 72 + 6 * 9 + 8] +

		tl2.w * kernelsLA[0 * 72 + 7 * 9 + 0] + tc2.w * kernelsLA[0 * 72 + 7 * 9 + 1] + tr2.w * kernelsLA[0 * 72 + 7 * 9 + 2] +
		ml2.w * kernelsLA[0 * 72 + 7 * 9 + 3] + mc2.w * kernelsLA[0 * 72 + 7 * 9 + 4] + mr2.w * kernelsLA[0 * 72 + 7 * 9 + 5] +
		bl2.w * kernelsLA[0 * 72 + 7 * 9 + 6] + bc2.w * kernelsLA[0 * 72 + 7 * 9 + 7] + br2.w * kernelsLA[0 * 72 + 7 * 9 + 8] + biasLA.x
		,
		tl1.x * kernelsLA[1 * 72 + 0 * 9 + 0] + tc1.x * kernelsLA[1 * 72 + 0 * 9 + 1] + tr1.x * kernelsLA[1 * 72 + 0 * 9 + 2] +
		ml1.x * kernelsLA[1 * 72 + 0 * 9 + 3] + mc1.x * kernelsLA[1 * 72 + 0 * 9 + 4] + mr1.x * kernelsLA[1 * 72 + 0 * 9 + 5] +
		bl1.x * kernelsLA[1 * 72 + 0 * 9 + 6] + bc1.x * kernelsLA[1 * 72 + 0 * 9 + 7] + br1.x * kernelsLA[1 * 72 + 0 * 9 + 8] +

		tl1.y * kernelsLA[1 * 72 + 1 * 9 + 0] + tc1.y * kernelsLA[1 * 72 + 1 * 9 + 1] + tr1.y * kernelsLA[1 * 72 + 1 * 9 + 2] +
		ml1.y * kernelsLA[1 * 72 + 1 * 9 + 3] + mc1.y * kernelsLA[1 * 72 + 1 * 9 + 4] + mr1.y * kernelsLA[1 * 72 + 1 * 9 + 5] +
		bl1.y * kernelsLA[1 * 72 + 1 * 9 + 6] + bc1.y * kernelsLA[1 * 72 + 1 * 9 + 7] + br1.y * kernelsLA[1 * 72 + 1 * 9 + 8] +

		tl1.z * kernelsLA[1 * 72 + 2 * 9 + 0] + tc1.z * kernelsLA[1 * 72 + 2 * 9 + 1] + tr1.z * kernelsLA[1 * 72 + 2 * 9 + 2] +
		ml1.z * kernelsLA[1 * 72 + 2 * 9 + 3] + mc1.z * kernelsLA[1 * 72 + 2 * 9 + 4] + mr1.z * kernelsLA[1 * 72 + 2 * 9 + 5] +
		bl1.z * kernelsLA[1 * 72 + 2 * 9 + 6] + bc1.z * kernelsLA[1 * 72 + 2 * 9 + 7] + br1.z * kernelsLA[1 * 72 + 2 * 9 + 8] +

		tl1.w * kernelsLA[1 * 72 + 3 * 9 + 0] + tc1.w * kernelsLA[1 * 72 + 3 * 9 + 1] + tr1.w * kernelsLA[1 * 72 + 3 * 9 + 2] +
		ml1.w * kernelsLA[1 * 72 + 3 * 9 + 3] + mc1.w * kernelsLA[1 * 72 + 3 * 9 + 4] + mr1.w * kernelsLA[1 * 72 + 3 * 9 + 5] +
		bl1.w * kernelsLA[1 * 72 + 3 * 9 + 6] + bc1.w * kernelsLA[1 * 72 + 3 * 9 + 7] + br1.w * kernelsLA[1 * 72 + 3 * 9 + 8] +

		tl2.x * kernelsLA[1 * 72 + 4 * 9 + 0] + tc2.x * kernelsLA[1 * 72 + 4 * 9 + 1] + tr2.x * kernelsLA[1 * 72 + 4 * 9 + 2] +
		ml2.x * kernelsLA[1 * 72 + 4 * 9 + 3] + mc2.x * kernelsLA[1 * 72 + 4 * 9 + 4] + mr2.x * kernelsLA[1 * 72 + 4 * 9 + 5] +
		bl2.x * kernelsLA[1 * 72 + 4 * 9 + 6] + bc2.x * kernelsLA[1 * 72 + 4 * 9 + 7] + br2.x * kernelsLA[1 * 72 + 4 * 9 + 8] +

		tl2.y * kernelsLA[1 * 72 + 5 * 9 + 0] + tc2.y * kernelsLA[1 * 72 + 5 * 9 + 1] + tr2.y * kernelsLA[1 * 72 + 5 * 9 + 2] +
		ml2.y * kernelsLA[1 * 72 + 5 * 9 + 3] + mc2.y * kernelsLA[1 * 72 + 5 * 9 + 4] + mr2.y * kernelsLA[1 * 72 + 5 * 9 + 5] +
		bl2.y * kernelsLA[1 * 72 + 5 * 9 + 6] + bc2.y * kernelsLA[1 * 72 + 5 * 9 + 7] + br2.y * kernelsLA[1 * 72 + 5 * 9 + 8] +

		tl2.z * kernelsLA[1 * 72 + 6 * 9 + 0] + tc2.z * kernelsLA[1 * 72 + 6 * 9 + 1] + tr2.z * kernelsLA[1 * 72 + 6 * 9 + 2] +
		ml2.z * kernelsLA[1 * 72 + 6 * 9 + 3] + mc2.z * kernelsLA[1 * 72 + 6 * 9 + 4] + mr2.z * kernelsLA[1 * 72 + 6 * 9 + 5] +
		bl2.z * kernelsLA[1 * 72 + 6 * 9 + 6] + bc2.z * kernelsLA[1 * 72 + 6 * 9 + 7] + br2.z * kernelsLA[1 * 72 + 6 * 9 + 8] +

		tl2.w * kernelsLA[1 * 72 + 7 * 9 + 0] + tc2.w * kernelsLA[1 * 72 + 7 * 9 + 1] + tr2.w * kernelsLA[1 * 72 + 7 * 9 + 2] +
		ml2.w * kernelsLA[1 * 72 + 7 * 9 + 3] + mc2.w * kernelsLA[1 * 72 + 7 * 9 + 4] + mr2.w * kernelsLA[1 * 72 + 7 * 9 + 5] +
		bl2.w * kernelsLA[1 * 72 + 7 * 9 + 6] + bc2.w * kernelsLA[1 * 72 + 7 * 9 + 7] + br2.w * kernelsLA[1 * 72 + 7 * 9 + 8] + biasLA.y
		,
		tl1.x * kernelsLA[2 * 72 + 0 * 9 + 0] + tc1.x * kernelsLA[2 * 72 + 0 * 9 + 1] + tr1.x * kernelsLA[2 * 72 + 0 * 9 + 2] +
		ml1.x * kernelsLA[2 * 72 + 0 * 9 + 3] + mc1.x * kernelsLA[2 * 72 + 0 * 9 + 4] + mr1.x * kernelsLA[2 * 72 + 0 * 9 + 5] +
		bl1.x * kernelsLA[2 * 72 + 0 * 9 + 6] + bc1.x * kernelsLA[2 * 72 + 0 * 9 + 7] + br1.x * kernelsLA[2 * 72 + 0 * 9 + 8] +

		tl1.y * kernelsLA[2 * 72 + 1 * 9 + 0] + tc1.y * kernelsLA[2 * 72 + 1 * 9 + 1] + tr1.y * kernelsLA[2 * 72 + 1 * 9 + 2] +
		ml1.y * kernelsLA[2 * 72 + 1 * 9 + 3] + mc1.y * kernelsLA[2 * 72 + 1 * 9 + 4] + mr1.y * kernelsLA[2 * 72 + 1 * 9 + 5] +
		bl1.y * kernelsLA[2 * 72 + 1 * 9 + 6] + bc1.y * kernelsLA[2 * 72 + 1 * 9 + 7] + br1.y * kernelsLA[2 * 72 + 1 * 9 + 8] +

		tl1.z * kernelsLA[2 * 72 + 2 * 9 + 0] + tc1.z * kernelsLA[2 * 72 + 2 * 9 + 1] + tr1.z * kernelsLA[2 * 72 + 2 * 9 + 2] +
		ml1.z * kernelsLA[2 * 72 + 2 * 9 + 3] + mc1.z * kernelsLA[2 * 72 + 2 * 9 + 4] + mr1.z * kernelsLA[2 * 72 + 2 * 9 + 5] +
		bl1.z * kernelsLA[2 * 72 + 2 * 9 + 6] + bc1.z * kernelsLA[2 * 72 + 2 * 9 + 7] + br1.z * kernelsLA[2 * 72 + 2 * 9 + 8] +

		tl1.w * kernelsLA[2 * 72 + 3 * 9 + 0] + tc1.w * kernelsLA[2 * 72 + 3 * 9 + 1] + tr1.w * kernelsLA[2 * 72 + 3 * 9 + 2] +
		ml1.w * kernelsLA[2 * 72 + 3 * 9 + 3] + mc1.w * kernelsLA[2 * 72 + 3 * 9 + 4] + mr1.w * kernelsLA[2 * 72 + 3 * 9 + 5] +
		bl1.w * kernelsLA[2 * 72 + 3 * 9 + 6] + bc1.w * kernelsLA[2 * 72 + 3 * 9 + 7] + br1.w * kernelsLA[2 * 72 + 3 * 9 + 8] +

		tl2.x * kernelsLA[2 * 72 + 4 * 9 + 0] + tc2.x * kernelsLA[2 * 72 + 4 * 9 + 1] + tr2.x * kernelsLA[2 * 72 + 4 * 9 + 2] +
		ml2.x * kernelsLA[2 * 72 + 4 * 9 + 3] + mc2.x * kernelsLA[2 * 72 + 4 * 9 + 4] + mr2.x * kernelsLA[2 * 72 + 4 * 9 + 5] +
		bl2.x * kernelsLA[2 * 72 + 4 * 9 + 6] + bc2.x * kernelsLA[2 * 72 + 4 * 9 + 7] + br2.x * kernelsLA[2 * 72 + 4 * 9 + 8] +

		tl2.y * kernelsLA[2 * 72 + 5 * 9 + 0] + tc2.y * kernelsLA[2 * 72 + 5 * 9 + 1] + tr2.y * kernelsLA[2 * 72 + 5 * 9 + 2] +
		ml2.y * kernelsLA[2 * 72 + 5 * 9 + 3] + mc2.y * kernelsLA[2 * 72 + 5 * 9 + 4] + mr2.y * kernelsLA[2 * 72 + 5 * 9 + 5] +
		bl2.y * kernelsLA[2 * 72 + 5 * 9 + 6] + bc2.y * kernelsLA[2 * 72 + 5 * 9 + 7] + br2.y * kernelsLA[2 * 72 + 5 * 9 + 8] +

		tl2.z * kernelsLA[2 * 72 + 6 * 9 + 0] + tc2.z * kernelsLA[2 * 72 + 6 * 9 + 1] + tr2.z * kernelsLA[2 * 72 + 6 * 9 + 2] +
		ml2.z * kernelsLA[2 * 72 + 6 * 9 + 3] + mc2.z * kernelsLA[2 * 72 + 6 * 9 + 4] + mr2.z * kernelsLA[2 * 72 + 6 * 9 + 5] +
		bl2.z * kernelsLA[2 * 72 + 6 * 9 + 6] + bc2.z * kernelsLA[2 * 72 + 6 * 9 + 7] + br2.z * kernelsLA[2 * 72 + 6 * 9 + 8] +

		tl2.w * kernelsLA[2 * 72 + 7 * 9 + 0] + tc2.w * kernelsLA[2 * 72 + 7 * 9 + 1] + tr2.w * kernelsLA[2 * 72 + 7 * 9 + 2] +
		ml2.w * kernelsLA[2 * 72 + 7 * 9 + 3] + mc2.w * kernelsLA[2 * 72 + 7 * 9 + 4] + mr2.w * kernelsLA[2 * 72 + 7 * 9 + 5] +
		bl2.w * kernelsLA[2 * 72 + 7 * 9 + 6] + bc2.w * kernelsLA[2 * 72 + 7 * 9 + 7] + br2.w * kernelsLA[2 * 72 + 7 * 9 + 8] + biasLA.z
		,
		tl1.x * kernelsLA[3 * 72 + 0 * 9 + 0] + tc1.x * kernelsLA[3 * 72 + 0 * 9 + 1] + tr1.x * kernelsLA[3 * 72 + 0 * 9 + 2] +
		ml1.x * kernelsLA[3 * 72 + 0 * 9 + 3] + mc1.x * kernelsLA[3 * 72 + 0 * 9 + 4] + mr1.x * kernelsLA[3 * 72 + 0 * 9 + 5] +
		bl1.x * kernelsLA[3 * 72 + 0 * 9 + 6] + bc1.x * kernelsLA[3 * 72 + 0 * 9 + 7] + br1.x * kernelsLA[3 * 72 + 0 * 9 + 8] +

		tl1.y * kernelsLA[3 * 72 + 1 * 9 + 0] + tc1.y * kernelsLA[3 * 72 + 1 * 9 + 1] + tr1.y * kernelsLA[3 * 72 + 1 * 9 + 2] +
		ml1.y * kernelsLA[3 * 72 + 1 * 9 + 3] + mc1.y * kernelsLA[3 * 72 + 1 * 9 + 4] + mr1.y * kernelsLA[3 * 72 + 1 * 9 + 5] +
		bl1.y * kernelsLA[3 * 72 + 1 * 9 + 6] + bc1.y * kernelsLA[3 * 72 + 1 * 9 + 7] + br1.y * kernelsLA[3 * 72 + 1 * 9 + 8] +

		tl1.z * kernelsLA[3 * 72 + 2 * 9 + 0] + tc1.z * kernelsLA[3 * 72 + 2 * 9 + 1] + tr1.z * kernelsLA[3 * 72 + 2 * 9 + 2] +
		ml1.z * kernelsLA[3 * 72 + 2 * 9 + 3] + mc1.z * kernelsLA[3 * 72 + 2 * 9 + 4] + mr1.z * kernelsLA[3 * 72 + 2 * 9 + 5] +
		bl1.z * kernelsLA[3 * 72 + 2 * 9 + 6] + bc1.z * kernelsLA[3 * 72 + 2 * 9 + 7] + br1.z * kernelsLA[3 * 72 + 2 * 9 + 8] +

		tl1.w * kernelsLA[3 * 72 + 3 * 9 + 0] + tc1.w * kernelsLA[3 * 72 + 3 * 9 + 1] + tr1.w * kernelsLA[3 * 72 + 3 * 9 + 2] +
		ml1.w * kernelsLA[3 * 72 + 3 * 9 + 3] + mc1.w * kernelsLA[3 * 72 + 3 * 9 + 4] + mr1.w * kernelsLA[3 * 72 + 3 * 9 + 5] +
		bl1.w * kernelsLA[3 * 72 + 3 * 9 + 6] + bc1.w * kernelsLA[3 * 72 + 3 * 9 + 7] + br1.w * kernelsLA[3 * 72 + 3 * 9 + 8] +

		tl2.x * kernelsLA[3 * 72 + 4 * 9 + 0] + tc2.x * kernelsLA[3 * 72 + 4 * 9 + 1] + tr2.x * kernelsLA[3 * 72 + 4 * 9 + 2] +
		ml2.x * kernelsLA[3 * 72 + 4 * 9 + 3] + mc2.x * kernelsLA[3 * 72 + 4 * 9 + 4] + mr2.x * kernelsLA[3 * 72 + 4 * 9 + 5] +
		bl2.x * kernelsLA[3 * 72 + 4 * 9 + 6] + bc2.x * kernelsLA[3 * 72 + 4 * 9 + 7] + br2.x * kernelsLA[3 * 72 + 4 * 9 + 8] +

		tl2.y * kernelsLA[3 * 72 + 5 * 9 + 0] + tc2.y * kernelsLA[3 * 72 + 5 * 9 + 1] + tr2.y * kernelsLA[3 * 72 + 5 * 9 + 2] +
		ml2.y * kernelsLA[3 * 72 + 5 * 9 + 3] + mc2.y * kernelsLA[3 * 72 + 5 * 9 + 4] + mr2.y * kernelsLA[3 * 72 + 5 * 9 + 5] +
		bl2.y * kernelsLA[3 * 72 + 5 * 9 + 6] + bc2.y * kernelsLA[3 * 72 + 5 * 9 + 7] + br2.y * kernelsLA[3 * 72 + 5 * 9 + 8] +

		tl2.z * kernelsLA[3 * 72 + 6 * 9 + 0] + tc2.z * kernelsLA[3 * 72 + 6 * 9 + 1] + tr2.z * kernelsLA[3 * 72 + 6 * 9 + 2] +
		ml2.z * kernelsLA[3 * 72 + 6 * 9 + 3] + mc2.z * kernelsLA[3 * 72 + 6 * 9 + 4] + mr2.z * kernelsLA[3 * 72 + 6 * 9 + 5] +
		bl2.z * kernelsLA[3 * 72 + 6 * 9 + 6] + bc2.z * kernelsLA[3 * 72 + 6 * 9 + 7] + br2.z * kernelsLA[3 * 72 + 6 * 9 + 8] +

		tl2.w * kernelsLA[3 * 72 + 7 * 9 + 0] + tc2.w * kernelsLA[3 * 72 + 7 * 9 + 1] + tr2.w * kernelsLA[3 * 72 + 7 * 9 + 2] +
		ml2.w * kernelsLA[3 * 72 + 7 * 9 + 3] + mc2.w * kernelsLA[3 * 72 + 7 * 9 + 4] + mr2.w * kernelsLA[3 * 72 + 7 * 9 + 5] +
		bl2.w * kernelsLA[3 * 72 + 7 * 9 + 6] + bc2.w * kernelsLA[3 * 72 + 7 * 9 + 7] + br2.w * kernelsLA[3 * 72 + 7 * 9 + 8] + biasLA.w
	));

	float4 target2 = RELU(float4(
		tl1.x * kernelsLB[0 * 72 + 0 * 9 + 0] + tc1.x * kernelsLB[0 * 72 + 0 * 9 + 1] + tr1.x * kernelsLB[0 * 72 + 0 * 9 + 2] +
		ml1.x * kernelsLB[0 * 72 + 0 * 9 + 3] + mc1.x * kernelsLB[0 * 72 + 0 * 9 + 4] + mr1.x * kernelsLB[0 * 72 + 0 * 9 + 5] +
		bl1.x * kernelsLB[0 * 72 + 0 * 9 + 6] + bc1.x * kernelsLB[0 * 72 + 0 * 9 + 7] + br1.x * kernelsLB[0 * 72 + 0 * 9 + 8] +

		tl1.y * kernelsLB[0 * 72 + 1 * 9 + 0] + tc1.y * kernelsLB[0 * 72 + 1 * 9 + 1] + tr1.y * kernelsLB[0 * 72 + 1 * 9 + 2] +
		ml1.y * kernelsLB[0 * 72 + 1 * 9 + 3] + mc1.y * kernelsLB[0 * 72 + 1 * 9 + 4] + mr1.y * kernelsLB[0 * 72 + 1 * 9 + 5] +
		bl1.y * kernelsLB[0 * 72 + 1 * 9 + 6] + bc1.y * kernelsLB[0 * 72 + 1 * 9 + 7] + br1.y * kernelsLB[0 * 72 + 1 * 9 + 8] +

		tl1.z * kernelsLB[0 * 72 + 2 * 9 + 0] + tc1.z * kernelsLB[0 * 72 + 2 * 9 + 1] + tr1.z * kernelsLB[0 * 72 + 2 * 9 + 2] +
		ml1.z * kernelsLB[0 * 72 + 2 * 9 + 3] + mc1.z * kernelsLB[0 * 72 + 2 * 9 + 4] + mr1.z * kernelsLB[0 * 72 + 2 * 9 + 5] +
		bl1.z * kernelsLB[0 * 72 + 2 * 9 + 6] + bc1.z * kernelsLB[0 * 72 + 2 * 9 + 7] + br1.z * kernelsLB[0 * 72 + 2 * 9 + 8] +

		tl1.w * kernelsLB[0 * 72 + 3 * 9 + 0] + tc1.w * kernelsLB[0 * 72 + 3 * 9 + 1] + tr1.w * kernelsLB[0 * 72 + 3 * 9 + 2] +
		ml1.w * kernelsLB[0 * 72 + 3 * 9 + 3] + mc1.w * kernelsLB[0 * 72 + 3 * 9 + 4] + mr1.w * kernelsLB[0 * 72 + 3 * 9 + 5] +
		bl1.w * kernelsLB[0 * 72 + 3 * 9 + 6] + bc1.w * kernelsLB[0 * 72 + 3 * 9 + 7] + br1.w * kernelsLB[0 * 72 + 3 * 9 + 8] +

		tl2.x * kernelsLB[0 * 72 + 4 * 9 + 0] + tc2.x * kernelsLB[0 * 72 + 4 * 9 + 1] + tr2.x * kernelsLB[0 * 72 + 4 * 9 + 2] +
		ml2.x * kernelsLB[0 * 72 + 4 * 9 + 3] + mc2.x * kernelsLB[0 * 72 + 4 * 9 + 4] + mr2.x * kernelsLB[0 * 72 + 4 * 9 + 5] +
		bl2.x * kernelsLB[0 * 72 + 4 * 9 + 6] + bc2.x * kernelsLB[0 * 72 + 4 * 9 + 7] + br2.x * kernelsLB[0 * 72 + 4 * 9 + 8] +

		tl2.y * kernelsLB[0 * 72 + 5 * 9 + 0] + tc2.y * kernelsLB[0 * 72 + 5 * 9 + 1] + tr2.y * kernelsLB[0 * 72 + 5 * 9 + 2] +
		ml2.y * kernelsLB[0 * 72 + 5 * 9 + 3] + mc2.y * kernelsLB[0 * 72 + 5 * 9 + 4] + mr2.y * kernelsLB[0 * 72 + 5 * 9 + 5] +
		bl2.y * kernelsLB[0 * 72 + 5 * 9 + 6] + bc2.y * kernelsLB[0 * 72 + 5 * 9 + 7] + br2.y * kernelsLB[0 * 72 + 5 * 9 + 8] +

		tl2.z * kernelsLB[0 * 72 + 6 * 9 + 0] + tc2.z * kernelsLB[0 * 72 + 6 * 9 + 1] + tr2.z * kernelsLB[0 * 72 + 6 * 9 + 2] +
		ml2.z * kernelsLB[0 * 72 + 6 * 9 + 3] + mc2.z * kernelsLB[0 * 72 + 6 * 9 + 4] + mr2.z * kernelsLB[0 * 72 + 6 * 9 + 5] +
		bl2.z * kernelsLB[0 * 72 + 6 * 9 + 6] + bc2.z * kernelsLB[0 * 72 + 6 * 9 + 7] + br2.z * kernelsLB[0 * 72 + 6 * 9 + 8] +

		tl2.w * kernelsLB[0 * 72 + 7 * 9 + 0] + tc2.w * kernelsLB[0 * 72 + 7 * 9 + 1] + tr2.w * kernelsLB[0 * 72 + 7 * 9 + 2] +
		ml2.w * kernelsLB[0 * 72 + 7 * 9 + 3] + mc2.w * kernelsLB[0 * 72 + 7 * 9 + 4] + mr2.w * kernelsLB[0 * 72 + 7 * 9 + 5] +
		bl2.w * kernelsLB[0 * 72 + 7 * 9 + 6] + bc2.w * kernelsLB[0 * 72 + 7 * 9 + 7] + br2.w * kernelsLB[0 * 72 + 7 * 9 + 8] + biasLB.x
		,
		tl1.x * kernelsLB[1 * 72 + 0 * 9 + 0] + tc1.x * kernelsLB[1 * 72 + 0 * 9 + 1] + tr1.x * kernelsLB[1 * 72 + 0 * 9 + 2] +
		ml1.x * kernelsLB[1 * 72 + 0 * 9 + 3] + mc1.x * kernelsLB[1 * 72 + 0 * 9 + 4] + mr1.x * kernelsLB[1 * 72 + 0 * 9 + 5] +
		bl1.x * kernelsLB[1 * 72 + 0 * 9 + 6] + bc1.x * kernelsLB[1 * 72 + 0 * 9 + 7] + br1.x * kernelsLB[1 * 72 + 0 * 9 + 8] +

		tl1.y * kernelsLB[1 * 72 + 1 * 9 + 0] + tc1.y * kernelsLB[1 * 72 + 1 * 9 + 1] + tr1.y * kernelsLB[1 * 72 + 1 * 9 + 2] +
		ml1.y * kernelsLB[1 * 72 + 1 * 9 + 3] + mc1.y * kernelsLB[1 * 72 + 1 * 9 + 4] + mr1.y * kernelsLB[1 * 72 + 1 * 9 + 5] +
		bl1.y * kernelsLB[1 * 72 + 1 * 9 + 6] + bc1.y * kernelsLB[1 * 72 + 1 * 9 + 7] + br1.y * kernelsLB[1 * 72 + 1 * 9 + 8] +

		tl1.z * kernelsLB[1 * 72 + 2 * 9 + 0] + tc1.z * kernelsLB[1 * 72 + 2 * 9 + 1] + tr1.z * kernelsLB[1 * 72 + 2 * 9 + 2] +
		ml1.z * kernelsLB[1 * 72 + 2 * 9 + 3] + mc1.z * kernelsLB[1 * 72 + 2 * 9 + 4] + mr1.z * kernelsLB[1 * 72 + 2 * 9 + 5] +
		bl1.z * kernelsLB[1 * 72 + 2 * 9 + 6] + bc1.z * kernelsLB[1 * 72 + 2 * 9 + 7] + br1.z * kernelsLB[1 * 72 + 2 * 9 + 8] +

		tl1.w * kernelsLB[1 * 72 + 3 * 9 + 0] + tc1.w * kernelsLB[1 * 72 + 3 * 9 + 1] + tr1.w * kernelsLB[1 * 72 + 3 * 9 + 2] +
		ml1.w * kernelsLB[1 * 72 + 3 * 9 + 3] + mc1.w * kernelsLB[1 * 72 + 3 * 9 + 4] + mr1.w * kernelsLB[1 * 72 + 3 * 9 + 5] +
		bl1.w * kernelsLB[1 * 72 + 3 * 9 + 6] + bc1.w * kernelsLB[1 * 72 + 3 * 9 + 7] + br1.w * kernelsLB[1 * 72 + 3 * 9 + 8] +

		tl2.x * kernelsLB[1 * 72 + 4 * 9 + 0] + tc2.x * kernelsLB[1 * 72 + 4 * 9 + 1] + tr2.x * kernelsLB[1 * 72 + 4 * 9 + 2] +
		ml2.x * kernelsLB[1 * 72 + 4 * 9 + 3] + mc2.x * kernelsLB[1 * 72 + 4 * 9 + 4] + mr2.x * kernelsLB[1 * 72 + 4 * 9 + 5] +
		bl2.x * kernelsLB[1 * 72 + 4 * 9 + 6] + bc2.x * kernelsLB[1 * 72 + 4 * 9 + 7] + br2.x * kernelsLB[1 * 72 + 4 * 9 + 8] +

		tl2.y * kernelsLB[1 * 72 + 5 * 9 + 0] + tc2.y * kernelsLB[1 * 72 + 5 * 9 + 1] + tr2.y * kernelsLB[1 * 72 + 5 * 9 + 2] +
		ml2.y * kernelsLB[1 * 72 + 5 * 9 + 3] + mc2.y * kernelsLB[1 * 72 + 5 * 9 + 4] + mr2.y * kernelsLB[1 * 72 + 5 * 9 + 5] +
		bl2.y * kernelsLB[1 * 72 + 5 * 9 + 6] + bc2.y * kernelsLB[1 * 72 + 5 * 9 + 7] + br2.y * kernelsLB[1 * 72 + 5 * 9 + 8] +

		tl2.z * kernelsLB[1 * 72 + 6 * 9 + 0] + tc2.z * kernelsLB[1 * 72 + 6 * 9 + 1] + tr2.z * kernelsLB[1 * 72 + 6 * 9 + 2] +
		ml2.z * kernelsLB[1 * 72 + 6 * 9 + 3] + mc2.z * kernelsLB[1 * 72 + 6 * 9 + 4] + mr2.z * kernelsLB[1 * 72 + 6 * 9 + 5] +
		bl2.z * kernelsLB[1 * 72 + 6 * 9 + 6] + bc2.z * kernelsLB[1 * 72 + 6 * 9 + 7] + br2.z * kernelsLB[1 * 72 + 6 * 9 + 8] +

		tl2.w * kernelsLB[1 * 72 + 7 * 9 + 0] + tc2.w * kernelsLB[1 * 72 + 7 * 9 + 1] + tr2.w * kernelsLB[1 * 72 + 7 * 9 + 2] +
		ml2.w * kernelsLB[1 * 72 + 7 * 9 + 3] + mc2.w * kernelsLB[1 * 72 + 7 * 9 + 4] + mr2.w * kernelsLB[1 * 72 + 7 * 9 + 5] +
		bl2.w * kernelsLB[1 * 72 + 7 * 9 + 6] + bc2.w * kernelsLB[1 * 72 + 7 * 9 + 7] + br2.w * kernelsLB[1 * 72 + 7 * 9 + 8] + biasLB.y
		,
		tl1.x * kernelsLB[2 * 72 + 0 * 9 + 0] + tc1.x * kernelsLB[2 * 72 + 0 * 9 + 1] + tr1.x * kernelsLB[2 * 72 + 0 * 9 + 2] +
		ml1.x * kernelsLB[2 * 72 + 0 * 9 + 3] + mc1.x * kernelsLB[2 * 72 + 0 * 9 + 4] + mr1.x * kernelsLB[2 * 72 + 0 * 9 + 5] +
		bl1.x * kernelsLB[2 * 72 + 0 * 9 + 6] + bc1.x * kernelsLB[2 * 72 + 0 * 9 + 7] + br1.x * kernelsLB[2 * 72 + 0 * 9 + 8] +

		tl1.y * kernelsLB[2 * 72 + 1 * 9 + 0] + tc1.y * kernelsLB[2 * 72 + 1 * 9 + 1] + tr1.y * kernelsLB[2 * 72 + 1 * 9 + 2] +
		ml1.y * kernelsLB[2 * 72 + 1 * 9 + 3] + mc1.y * kernelsLB[2 * 72 + 1 * 9 + 4] + mr1.y * kernelsLB[2 * 72 + 1 * 9 + 5] +
		bl1.y * kernelsLB[2 * 72 + 1 * 9 + 6] + bc1.y * kernelsLB[2 * 72 + 1 * 9 + 7] + br1.y * kernelsLB[2 * 72 + 1 * 9 + 8] +

		tl1.z * kernelsLB[2 * 72 + 2 * 9 + 0] + tc1.z * kernelsLB[2 * 72 + 2 * 9 + 1] + tr1.z * kernelsLB[2 * 72 + 2 * 9 + 2] +
		ml1.z * kernelsLB[2 * 72 + 2 * 9 + 3] + mc1.z * kernelsLB[2 * 72 + 2 * 9 + 4] + mr1.z * kernelsLB[2 * 72 + 2 * 9 + 5] +
		bl1.z * kernelsLB[2 * 72 + 2 * 9 + 6] + bc1.z * kernelsLB[2 * 72 + 2 * 9 + 7] + br1.z * kernelsLB[2 * 72 + 2 * 9 + 8] +

		tl1.w * kernelsLB[2 * 72 + 3 * 9 + 0] + tc1.w * kernelsLB[2 * 72 + 3 * 9 + 1] + tr1.w * kernelsLB[2 * 72 + 3 * 9 + 2] +
		ml1.w * kernelsLB[2 * 72 + 3 * 9 + 3] + mc1.w * kernelsLB[2 * 72 + 3 * 9 + 4] + mr1.w * kernelsLB[2 * 72 + 3 * 9 + 5] +
		bl1.w * kernelsLB[2 * 72 + 3 * 9 + 6] + bc1.w * kernelsLB[2 * 72 + 3 * 9 + 7] + br1.w * kernelsLB[2 * 72 + 3 * 9 + 8] +

		tl2.x * kernelsLB[2 * 72 + 4 * 9 + 0] + tc2.x * kernelsLB[2 * 72 + 4 * 9 + 1] + tr2.x * kernelsLB[2 * 72 + 4 * 9 + 2] +
		ml2.x * kernelsLB[2 * 72 + 4 * 9 + 3] + mc2.x * kernelsLB[2 * 72 + 4 * 9 + 4] + mr2.x * kernelsLB[2 * 72 + 4 * 9 + 5] +
		bl2.x * kernelsLB[2 * 72 + 4 * 9 + 6] + bc2.x * kernelsLB[2 * 72 + 4 * 9 + 7] + br2.x * kernelsLB[2 * 72 + 4 * 9 + 8] +

		tl2.y * kernelsLB[2 * 72 + 5 * 9 + 0] + tc2.y * kernelsLB[2 * 72 + 5 * 9 + 1] + tr2.y * kernelsLB[2 * 72 + 5 * 9 + 2] +
		ml2.y * kernelsLB[2 * 72 + 5 * 9 + 3] + mc2.y * kernelsLB[2 * 72 + 5 * 9 + 4] + mr2.y * kernelsLB[2 * 72 + 5 * 9 + 5] +
		bl2.y * kernelsLB[2 * 72 + 5 * 9 + 6] + bc2.y * kernelsLB[2 * 72 + 5 * 9 + 7] + br2.y * kernelsLB[2 * 72 + 5 * 9 + 8] +

		tl2.z * kernelsLB[2 * 72 + 6 * 9 + 0] + tc2.z * kernelsLB[2 * 72 + 6 * 9 + 1] + tr2.z * kernelsLB[2 * 72 + 6 * 9 + 2] +
		ml2.z * kernelsLB[2 * 72 + 6 * 9 + 3] + mc2.z * kernelsLB[2 * 72 + 6 * 9 + 4] + mr2.z * kernelsLB[2 * 72 + 6 * 9 + 5] +
		bl2.z * kernelsLB[2 * 72 + 6 * 9 + 6] + bc2.z * kernelsLB[2 * 72 + 6 * 9 + 7] + br2.z * kernelsLB[2 * 72 + 6 * 9 + 8] +

		tl2.w * kernelsLB[2 * 72 + 7 * 9 + 0] + tc2.w * kernelsLB[2 * 72 + 7 * 9 + 1] + tr2.w * kernelsLB[2 * 72 + 7 * 9 + 2] +
		ml2.w * kernelsLB[2 * 72 + 7 * 9 + 3] + mc2.w * kernelsLB[2 * 72 + 7 * 9 + 4] + mr2.w * kernelsLB[2 * 72 + 7 * 9 + 5] +
		bl2.w * kernelsLB[2 * 72 + 7 * 9 + 6] + bc2.w * kernelsLB[2 * 72 + 7 * 9 + 7] + br2.w * kernelsLB[2 * 72 + 7 * 9 + 8] + biasLB.z
		,
		tl1.x * kernelsLB[3 * 72 + 0 * 9 + 0] + tc1.x * kernelsLB[3 * 72 + 0 * 9 + 1] + tr1.x * kernelsLB[3 * 72 + 0 * 9 + 2] +
		ml1.x * kernelsLB[3 * 72 + 0 * 9 + 3] + mc1.x * kernelsLB[3 * 72 + 0 * 9 + 4] + mr1.x * kernelsLB[3 * 72 + 0 * 9 + 5] +
		bl1.x * kernelsLB[3 * 72 + 0 * 9 + 6] + bc1.x * kernelsLB[3 * 72 + 0 * 9 + 7] + br1.x * kernelsLB[3 * 72 + 0 * 9 + 8] +

		tl1.y * kernelsLB[3 * 72 + 1 * 9 + 0] + tc1.y * kernelsLB[3 * 72 + 1 * 9 + 1] + tr1.y * kernelsLB[3 * 72 + 1 * 9 + 2] +
		ml1.y * kernelsLB[3 * 72 + 1 * 9 + 3] + mc1.y * kernelsLB[3 * 72 + 1 * 9 + 4] + mr1.y * kernelsLB[3 * 72 + 1 * 9 + 5] +
		bl1.y * kernelsLB[3 * 72 + 1 * 9 + 6] + bc1.y * kernelsLB[3 * 72 + 1 * 9 + 7] + br1.y * kernelsLB[3 * 72 + 1 * 9 + 8] +

		tl1.z * kernelsLB[3 * 72 + 2 * 9 + 0] + tc1.z * kernelsLB[3 * 72 + 2 * 9 + 1] + tr1.z * kernelsLB[3 * 72 + 2 * 9 + 2] +
		ml1.z * kernelsLB[3 * 72 + 2 * 9 + 3] + mc1.z * kernelsLB[3 * 72 + 2 * 9 + 4] + mr1.z * kernelsLB[3 * 72 + 2 * 9 + 5] +
		bl1.z * kernelsLB[3 * 72 + 2 * 9 + 6] + bc1.z * kernelsLB[3 * 72 + 2 * 9 + 7] + br1.z * kernelsLB[3 * 72 + 2 * 9 + 8] +

		tl1.w * kernelsLB[3 * 72 + 3 * 9 + 0] + tc1.w * kernelsLB[3 * 72 + 3 * 9 + 1] + tr1.w * kernelsLB[3 * 72 + 3 * 9 + 2] +
		ml1.w * kernelsLB[3 * 72 + 3 * 9 + 3] + mc1.w * kernelsLB[3 * 72 + 3 * 9 + 4] + mr1.w * kernelsLB[3 * 72 + 3 * 9 + 5] +
		bl1.w * kernelsLB[3 * 72 + 3 * 9 + 6] + bc1.w * kernelsLB[3 * 72 + 3 * 9 + 7] + br1.w * kernelsLB[3 * 72 + 3 * 9 + 8] +

		tl2.x * kernelsLB[3 * 72 + 4 * 9 + 0] + tc2.x * kernelsLB[3 * 72 + 4 * 9 + 1] + tr2.x * kernelsLB[3 * 72 + 4 * 9 + 2] +
		ml2.x * kernelsLB[3 * 72 + 4 * 9 + 3] + mc2.x * kernelsLB[3 * 72 + 4 * 9 + 4] + mr2.x * kernelsLB[3 * 72 + 4 * 9 + 5] +
		bl2.x * kernelsLB[3 * 72 + 4 * 9 + 6] + bc2.x * kernelsLB[3 * 72 + 4 * 9 + 7] + br2.x * kernelsLB[3 * 72 + 4 * 9 + 8] +

		tl2.y * kernelsLB[3 * 72 + 5 * 9 + 0] + tc2.y * kernelsLB[3 * 72 + 5 * 9 + 1] + tr2.y * kernelsLB[3 * 72 + 5 * 9 + 2] +
		ml2.y * kernelsLB[3 * 72 + 5 * 9 + 3] + mc2.y * kernelsLB[3 * 72 + 5 * 9 + 4] + mr2.y * kernelsLB[3 * 72 + 5 * 9 + 5] +
		bl2.y * kernelsLB[3 * 72 + 5 * 9 + 6] + bc2.y * kernelsLB[3 * 72 + 5 * 9 + 7] + br2.y * kernelsLB[3 * 72 + 5 * 9 + 8] +

		tl2.z * kernelsLB[3 * 72 + 6 * 9 + 0] + tc2.z * kernelsLB[3 * 72 + 6 * 9 + 1] + tr2.z * kernelsLB[3 * 72 + 6 * 9 + 2] +
		ml2.z * kernelsLB[3 * 72 + 6 * 9 + 3] + mc2.z * kernelsLB[3 * 72 + 6 * 9 + 4] + mr2.z * kernelsLB[3 * 72 + 6 * 9 + 5] +
		bl2.z * kernelsLB[3 * 72 + 6 * 9 + 6] + bc2.z * kernelsLB[3 * 72 + 6 * 9 + 7] + br2.z * kernelsLB[3 * 72 + 6 * 9 + 8] +

		tl2.w * kernelsLB[3 * 72 + 7 * 9 + 0] + tc2.w * kernelsLB[3 * 72 + 7 * 9 + 1] + tr2.w * kernelsLB[3 * 72 + 7 * 9 + 2] +
		ml2.w * kernelsLB[3 * 72 + 7 * 9 + 3] + mc2.w * kernelsLB[3 * 72 + 7 * 9 + 4] + mr2.w * kernelsLB[3 * 72 + 7 * 9 + 5] +
		bl2.w * kernelsLB[3 * 72 + 7 * 9 + 6] + bc2.w * kernelsLB[3 * 72 + 7 * 9 + 7] + br2.w * kernelsLB[3 * 72 + 7 * 9 + 8] + biasLB.w
	));

	tex3[gxy] = target1;
	tex4[gxy] = target2;
}


//!PASS 5
//!IN tex3, tex4
//!OUT tex1, tex2
//!BLOCK_SIZE 8
//!NUM_THREADS 64

const static float kernelsLA[9 * 8 * 4] = {
	 1.3625e-02, -8.5594e-02, -1.9901e-01,
	-6.4636e-02, -1.9030e-02,  4.1963e-02,
	-7.5507e-02, -2.4474e-01, -4.2621e-02,
	 2.8195e-02,  7.3102e-02, -9.3331e-02,
	 7.7093e-02,  1.7800e-01, -7.6451e-02,
	 2.8565e-02, -1.3540e-01, -1.9169e-01,
	-1.8583e-02,  3.0135e-02,  8.1094e-03,
	-1.2835e-01, -1.8041e-01, -8.9020e-02,
	-8.2731e-02,  3.7861e-02, -9.4014e-02,
	 4.6595e-02,  2.2052e-02, -1.5867e-01,
	-1.0937e-02,  1.0030e-01, -1.3018e-01,
	-9.1844e-02, -1.7508e-01,  2.2087e-01,
	-9.3080e-02,  9.8069e-02, -7.0154e-02,
	-6.6063e-02, -2.2142e-01,  4.1058e-01,
	-6.5947e-02, -5.4662e-02,  9.9412e-02,
	-5.1938e-02,  3.0932e-03,  1.8126e-01,
	 3.6701e-02, -3.0349e-01,  9.9839e-02,
	 2.5810e-02,  2.3644e-01, -2.4461e-01,
	 2.1054e-01,  1.5630e-01, -1.9587e-01,
	 5.0146e-02, -1.8844e-02,  3.6675e-01,
	-4.0389e-03,  3.1596e-01,  3.6771e-03,
	-2.2256e-40,  1.4272e-40, -2.0732e-40,
	 5.5913e-40, -6.0538e-40,  1.2791e-40,
	 4.5825e-41,  4.1080e-41, -1.8211e-40,
	 2.2687e-01, -5.8992e-02,  4.7796e-03,
	 6.0603e-01,  2.7961e-01,  1.5973e-02,
	 2.3035e-01,  1.3031e-01, -9.9280e-03,
	-4.7235e-02,  5.1773e-02, -4.8586e-02,
	-1.4510e-01, -1.7336e-01,  1.0981e-01,
	-2.0303e-01, -1.6008e-02, -1.8524e-03,
	-2.3440e-01, -3.2373e-02, -6.7911e-02,
	-1.6256e-01,  1.2316e-01,  2.7859e-02,
	 8.5089e-04, -3.7401e-02, -1.8672e-02,
	-1.0418e-01, -7.8407e-02, -1.8413e-02,
	 8.2834e-02,  2.3128e-01,  3.2983e-02,
	 3.1099e-02, -6.4485e-02, -8.1659e-02,
	 1.9152e-01, -1.9609e-02,  2.7364e-02,
	 1.0458e-02, -1.2507e-01,  4.1334e-02,
	-4.6215e-02,  5.6944e-02,  2.1477e-02,
	-1.4934e-01, -6.8383e-02,  2.7957e-02,
	-3.6846e-01,  4.8766e-01,  6.4000e-02,
	-3.9621e-02, -8.1667e-03,  4.5997e-02,
	-6.1391e-02,  1.2976e-02, -3.2152e-02,
	 7.5767e-02,  1.2931e-01, -2.3498e-02,
	 4.0320e-02,  1.3876e-02,  1.1022e-02,
	-6.2401e-41,  5.8564e-40,  3.9473e-40,
	-5.6890e-40, -2.6022e-40, -2.9841e-40,
	-4.2456e-40, -1.1546e-40,  4.4955e-40,
	-4.2969e-02, -1.0995e-01,  1.3021e-01,
	 1.0142e-01,  5.2225e-01, -5.5486e-02,
	-7.2349e-02,  8.5470e-02,  2.3438e-02,
	-1.0690e-01, -1.4370e-01, -1.2632e-01,
	 2.8754e-02,  1.1662e-01,  5.6515e-02,
	-1.5726e-01, -1.4945e-01, -4.4956e-02,
	 1.6574e-01, -5.6894e-02, -2.0851e-01,
	 8.1498e-03, -2.5441e-01, -1.4412e-01,
	-1.0959e-02, -2.5811e-02,  8.8934e-02,
	 6.3594e-02, -9.3314e-02,  7.8247e-02,
	 4.6795e-02, -2.2774e-01,  7.1041e-02,
	 1.4830e-01,  1.9911e-01,  5.1978e-02,
	 7.4936e-02,  2.3104e-02,  6.3928e-02,
	-1.3118e-02,  6.7544e-02,  7.9514e-02,
	 2.2335e-02, -9.9442e-02,  6.8070e-03,
	 2.4395e-02, -3.3576e-02,  5.5508e-02,
	-4.0872e-02,  5.4501e-02, -5.7051e-02,
	 8.6621e-03, -1.5361e-01,  1.2630e-01,
	-2.2344e-01,  1.3335e-01, -1.1688e-01,
	-2.4232e-01,  3.3319e-01, -1.2580e-01,
	-2.2169e-02,  2.0594e-01,  2.6521e-02,
	 4.1883e-40, -3.4540e-40,  4.9152e-40,
	-1.5711e-40,  3.3927e-40, -5.5069e-40,
	 5.5831e-40, -5.2011e-41,  1.0351e-40,
	 1.7989e-01,  2.3787e-02,  5.7447e-03,
	 4.8748e-01,  3.0152e-01,  3.5517e-02,
	 2.2155e-01,  1.8812e-01,  3.0994e-02,
	 7.8657e-02, -7.1135e-02, -5.8293e-02,
	-1.4220e-01,  1.6004e-02, -2.5180e-02,
	-1.6811e-01, -2.3441e-01,  1.4810e-02,
	 5.3140e-02, -1.2904e-01, -1.5105e-02,
	 5.4525e-02, -1.5418e-01,  6.6507e-02,
	 8.3947e-02, -1.1975e-01,  5.3902e-02,
	 8.0834e-02, -2.4321e-01, -1.0282e-03,
	 3.1276e-03,  3.2495e-01, -1.3238e-02,
	 4.5285e-02,  5.8777e-02, -1.3231e-01,
	-6.0928e-03,  8.7145e-02,  6.2031e-02,
	-5.3919e-01, -6.8810e-02, -1.0755e-01,
	-2.2571e-02,  2.6237e-02, -6.8731e-03,
	-6.6771e-02, -2.0586e-01,  4.7722e-02,
	-3.4968e-01,  3.0912e-01,  2.4487e-01,
	-4.9537e-02, -5.2779e-04,  6.7840e-02,
	 1.7583e-02,  3.3222e-02, -5.7070e-02,
	-2.3250e-01,  1.4470e-01, -4.9895e-02,
	 3.3147e-02,  8.6319e-02,  4.4719e-02,
	-6.9454e-41,  2.0308e-40, -1.1977e-40,
	 5.9045e-40, -2.6129e-40,  4.8298e-40,
	 4.7288e-40,  6.0736e-40,  2.2462e-40
};

const static float4 biasLA = { -0.0053,  0.0053, -0.0114, -0.0127 };

const static float kernelsLB[9 * 8 * 4] = {
	-4.0294e-02, -9.1437e-03, -2.4926e-02,
	-2.1269e-01,  1.1602e-01,  1.4383e-02,
	 5.1456e-02,  6.9047e-02,  1.6519e-02,
	 6.3737e-02, -9.0181e-02,  7.0716e-02,
	 7.0061e-02,  7.9046e-02, -4.3925e-02,
	 7.4396e-02, -5.2797e-02,  3.8125e-02,
	 7.5999e-02, -5.1307e-02,  2.4326e-03,
	-3.1716e-02, -1.2567e-01, -3.3898e-02,
	 8.4925e-02, -5.2404e-02,  2.8535e-02,
	 9.6844e-03,  4.6980e-02,  3.8552e-02,
	-5.7110e-02,  3.2163e-02,  1.5219e-02,
	 6.6905e-02, -2.7934e-02,  1.4184e-03,
	-2.4239e-02, -8.6317e-03, -2.3295e-03,
	-2.3065e-02,  1.0076e-01,  2.1562e-03,
	-1.3647e-02, -3.4262e-02,  2.5777e-02,
	 7.6601e-02,  1.3654e-01,  2.1458e-03,
	 1.4542e-01,  3.6310e-01,  1.6266e-01,
	-5.8465e-02,  4.3751e-02,  1.9227e-02,
	 9.1783e-03, -5.9547e-02, -1.8234e-02,
	-5.3399e-02,  1.9218e-01, -4.6238e-02,
	-1.9052e-01,  1.4635e-02,  2.9536e-02,
	 1.4621e-40, -5.5132e-40, -4.6215e-40,
	 4.3948e-40, -2.7285e-40, -5.5709e-40,
	 1.9428e-41, -4.0333e-40, -5.4469e-40,
	 9.3126e-02, -1.3236e-01,  9.9350e-02,
	-1.3308e-01,  3.5030e-01,  9.2221e-02,
	 1.1783e-01,  1.6648e-01, -7.9150e-02,
	 2.2654e-01, -1.2546e-01, -1.2354e-01,
	-1.6457e-01, -6.0740e-02, -3.1069e-02,
	-8.3203e-02, -1.8064e-01,  4.6900e-02,
	 1.2059e-01, -1.0569e-01, -7.1196e-02,
	-9.2991e-02, -1.7587e-01,  1.3100e-03,
	-1.5492e-01, -1.3849e-01,  1.2245e-01,
	-5.5276e-02, -9.7867e-02,  3.5550e-02,
	-6.0264e-02,  4.7760e-02,  6.0242e-02,
	-5.4096e-03,  2.4646e-01,  6.3592e-01,
	 5.8559e-02,  6.1117e-02,  8.0334e-02,
	-4.4582e-03, -1.2028e-01,  8.7394e-02,
	-2.5880e-02, -1.2206e-01,  1.2199e-01,
	 4.1990e-02, -1.3283e-01,  4.9047e-02,
	-4.9532e-02,  2.7688e-01, -4.6064e-03,
	-2.8812e-03, -2.4404e-01,  5.8614e-02,
	-1.4262e-01, -1.2810e-03, -1.2060e-01,
	-8.3595e-02,  5.6532e-02, -7.7556e-02,
	-1.3364e-01, -1.3883e-01, -1.2335e-01,
	-1.3273e-40,  6.5184e-41, -4.6946e-40,
	-4.0031e-40, -1.2807e-40, -3.1584e-40,
	 1.3009e-40,  2.4187e-40, -1.4202e-40,
	-8.8844e-03,  1.0101e-03, -6.0190e-02,
	-1.8851e-01, -7.6662e-02, -1.4562e-01,
	 2.9983e-02, -8.1533e-02,  1.1256e-02,
	 1.0205e-01,  6.7850e-02, -1.0911e-01,
	-1.2846e-01, -5.4605e-02,  6.2182e-02,
	-1.0797e-01, -5.1281e-02, -1.2036e-02,
	-8.1693e-02, -7.0432e-02,  1.6990e-01,
	-1.7329e-01, -2.2084e-01, -3.0977e-02,
	 8.2771e-02, -3.3089e-01, -1.4842e-01,
	 1.9576e-02, -1.5953e-01, -1.0348e-01,
	 6.6014e-02,  6.0094e-01, -6.9891e-04,
	 7.4969e-02, -1.4250e-01,  4.3221e-02,
	 1.6796e-02, -6.8125e-03,  4.7028e-02,
	-3.3421e-01, -2.2987e-01,  4.2936e-02,
	 9.3985e-04,  9.0827e-02,  2.4211e-01,
	-8.1571e-02, -1.0276e-01,  1.9092e-01,
	 2.1112e-01,  2.6837e-02, -2.5822e-01,
	-1.3290e-01,  1.6135e-01, -2.7672e-02,
	 3.4465e-01, -8.3286e-03, -6.1936e-02,
	 2.7406e-01, -6.8357e-02,  1.7426e-01,
	-9.0872e-02,  1.2999e-01,  7.2366e-02,
	 3.0944e-40, -1.2808e-40,  2.9336e-40,
	 5.5561e-42,  3.0978e-40,  1.0027e-40,
	-1.5881e-40, -2.9858e-40,  3.1599e-41,
	-9.1935e-02, -2.2666e-04, -6.2821e-02,
	-1.8605e-01,  3.0238e-01,  3.2759e-02,
	-5.0771e-02,  1.4585e-02, -1.0872e-01,
	 2.5511e-02, -9.3394e-02,  1.4810e-02,
	-6.2906e-02,  9.2472e-02,  1.2845e-02,
	-2.9041e-01, -9.6489e-03, -2.7277e-02,
	-6.9896e-02, -1.1645e-01, -5.9870e-02,
	-2.8037e-02, -2.2649e-01,  5.1781e-02,
	-1.4588e-02,  4.8753e-02, -2.8256e-02,
	-1.6462e-02,  8.0795e-02,  3.6222e-02,
	 8.0392e-02,  3.0118e-01,  2.0021e-01,
	 1.0394e-01,  6.4196e-01,  4.9545e-01,
	 2.1242e-02, -1.2514e-01,  1.0066e-01,
	-4.7676e-02, -2.0736e-02, -5.6951e-03,
	-8.3021e-02,  4.6763e-02,  1.7551e-01,
	 2.0038e-02,  1.8084e-01,  1.3244e-02,
	 1.0280e-02,  2.8740e-01,  8.9837e-03,
	-2.9437e-02, -3.7366e-01, -1.1861e-01,
	-4.8248e-03, -1.2970e-01, -1.8680e-02,
	 1.8458e-01,  5.6509e-02,  1.2734e-01,
	 1.9423e-01, -3.6960e-01, -2.5555e-02,
	 6.7959e-41, -3.2251e-40, -3.0631e-40,
	-4.0701e-40,  9.7399e-41,  2.2917e-40,
	 2.0169e-40,  5.7891e-40, -4.1286e-40
};

const static float4 biasLB = { -0.0039, -0.0426,  0.0053, -0.0017 };


void Pass5(uint2 blockStart, uint3 threadId) {
	uint2 gxy = Rmp8x8(threadId.x) + blockStart;
	uint2 inputSize = GetInputSize();
	if (gxy.x >= inputSize.x || gxy.y >= inputSize.y) {
		return;
	}

	float2 inputPt = GetInputPt();
	float2 pos = (gxy + 0.5f) * inputPt;

	// [tl, tc, tr]
	// [ml, mc, mr]
	// [bl, bc, br]
	float4 tl1 = tex3.SampleLevel(sam, pos + float2(-inputPt.x, -inputPt.y), 0);
	float4 ml1 = tex3.SampleLevel(sam, pos + float2(-inputPt.x, 0), 0);
	float4 bl1 = tex3.SampleLevel(sam, pos + float2(-inputPt.x, inputPt.y), 0);
	float4 tc1 = tex3.SampleLevel(sam, pos + float2(0, -inputPt.y), 0);
	float4 mc1 = tex3.SampleLevel(sam, pos, 0);
	float4 bc1 = tex3.SampleLevel(sam, pos + float2(0, inputPt.y), 0);
	float4 tr1 = tex3.SampleLevel(sam, pos + float2(inputPt.x, -inputPt.y), 0);
	float4 mr1 = tex3.SampleLevel(sam, pos + float2(inputPt.x, 0), 0);
	float4 br1 = tex3.SampleLevel(sam, pos + float2(inputPt.x, inputPt.y), 0);

	float4 tl2 = tex4.SampleLevel(sam, pos + float2(-inputPt.x, -inputPt.y), 0);
	float4 ml2 = tex4.SampleLevel(sam, pos + float2(-inputPt.x, 0), 0);
	float4 bl2 = tex4.SampleLevel(sam, pos + float2(-inputPt.x, inputPt.y), 0);
	float4 tc2 = tex4.SampleLevel(sam, pos + float2(0, -inputPt.y), 0);
	float4 mc2 = tex4.SampleLevel(sam, pos, 0);
	float4 bc2 = tex4.SampleLevel(sam, pos + float2(0, inputPt.y), 0);
	float4 tr2 = tex4.SampleLevel(sam, pos + float2(inputPt.x, -inputPt.y), 0);
	float4 mr2 = tex4.SampleLevel(sam, pos + float2(inputPt.x, 0), 0);
	float4 br2 = tex4.SampleLevel(sam, pos + float2(inputPt.x, inputPt.y), 0);


	float4 target1 = RELU(float4(
		tl1.x * kernelsLA[0 * 72 + 0 * 9 + 0] + tc1.x * kernelsLA[0 * 72 + 0 * 9 + 1] + tr1.x * kernelsLA[0 * 72 + 0 * 9 + 2] +
		ml1.x * kernelsLA[0 * 72 + 0 * 9 + 3] + mc1.x * kernelsLA[0 * 72 + 0 * 9 + 4] + mr1.x * kernelsLA[0 * 72 + 0 * 9 + 5] +
		bl1.x * kernelsLA[0 * 72 + 0 * 9 + 6] + bc1.x * kernelsLA[0 * 72 + 0 * 9 + 7] + br1.x * kernelsLA[0 * 72 + 0 * 9 + 8] +

		tl1.y * kernelsLA[0 * 72 + 1 * 9 + 0] + tc1.y * kernelsLA[0 * 72 + 1 * 9 + 1] + tr1.y * kernelsLA[0 * 72 + 1 * 9 + 2] +
		ml1.y * kernelsLA[0 * 72 + 1 * 9 + 3] + mc1.y * kernelsLA[0 * 72 + 1 * 9 + 4] + mr1.y * kernelsLA[0 * 72 + 1 * 9 + 5] +
		bl1.y * kernelsLA[0 * 72 + 1 * 9 + 6] + bc1.y * kernelsLA[0 * 72 + 1 * 9 + 7] + br1.y * kernelsLA[0 * 72 + 1 * 9 + 8] +

		tl1.z * kernelsLA[0 * 72 + 2 * 9 + 0] + tc1.z * kernelsLA[0 * 72 + 2 * 9 + 1] + tr1.z * kernelsLA[0 * 72 + 2 * 9 + 2] +
		ml1.z * kernelsLA[0 * 72 + 2 * 9 + 3] + mc1.z * kernelsLA[0 * 72 + 2 * 9 + 4] + mr1.z * kernelsLA[0 * 72 + 2 * 9 + 5] +
		bl1.z * kernelsLA[0 * 72 + 2 * 9 + 6] + bc1.z * kernelsLA[0 * 72 + 2 * 9 + 7] + br1.z * kernelsLA[0 * 72 + 2 * 9 + 8] +

		tl1.w * kernelsLA[0 * 72 + 3 * 9 + 0] + tc1.w * kernelsLA[0 * 72 + 3 * 9 + 1] + tr1.w * kernelsLA[0 * 72 + 3 * 9 + 2] +
		ml1.w * kernelsLA[0 * 72 + 3 * 9 + 3] + mc1.w * kernelsLA[0 * 72 + 3 * 9 + 4] + mr1.w * kernelsLA[0 * 72 + 3 * 9 + 5] +
		bl1.w * kernelsLA[0 * 72 + 3 * 9 + 6] + bc1.w * kernelsLA[0 * 72 + 3 * 9 + 7] + br1.w * kernelsLA[0 * 72 + 3 * 9 + 8] +

		tl2.x * kernelsLA[0 * 72 + 4 * 9 + 0] + tc2.x * kernelsLA[0 * 72 + 4 * 9 + 1] + tr2.x * kernelsLA[0 * 72 + 4 * 9 + 2] +
		ml2.x * kernelsLA[0 * 72 + 4 * 9 + 3] + mc2.x * kernelsLA[0 * 72 + 4 * 9 + 4] + mr2.x * kernelsLA[0 * 72 + 4 * 9 + 5] +
		bl2.x * kernelsLA[0 * 72 + 4 * 9 + 6] + bc2.x * kernelsLA[0 * 72 + 4 * 9 + 7] + br2.x * kernelsLA[0 * 72 + 4 * 9 + 8] +

		tl2.y * kernelsLA[0 * 72 + 5 * 9 + 0] + tc2.y * kernelsLA[0 * 72 + 5 * 9 + 1] + tr2.y * kernelsLA[0 * 72 + 5 * 9 + 2] +
		ml2.y * kernelsLA[0 * 72 + 5 * 9 + 3] + mc2.y * kernelsLA[0 * 72 + 5 * 9 + 4] + mr2.y * kernelsLA[0 * 72 + 5 * 9 + 5] +
		bl2.y * kernelsLA[0 * 72 + 5 * 9 + 6] + bc2.y * kernelsLA[0 * 72 + 5 * 9 + 7] + br2.y * kernelsLA[0 * 72 + 5 * 9 + 8] +

		tl2.z * kernelsLA[0 * 72 + 6 * 9 + 0] + tc2.z * kernelsLA[0 * 72 + 6 * 9 + 1] + tr2.z * kernelsLA[0 * 72 + 6 * 9 + 2] +
		ml2.z * kernelsLA[0 * 72 + 6 * 9 + 3] + mc2.z * kernelsLA[0 * 72 + 6 * 9 + 4] + mr2.z * kernelsLA[0 * 72 + 6 * 9 + 5] +
		bl2.z * kernelsLA[0 * 72 + 6 * 9 + 6] + bc2.z * kernelsLA[0 * 72 + 6 * 9 + 7] + br2.z * kernelsLA[0 * 72 + 6 * 9 + 8] +

		tl2.w * kernelsLA[0 * 72 + 7 * 9 + 0] + tc2.w * kernelsLA[0 * 72 + 7 * 9 + 1] + tr2.w * kernelsLA[0 * 72 + 7 * 9 + 2] +
		ml2.w * kernelsLA[0 * 72 + 7 * 9 + 3] + mc2.w * kernelsLA[0 * 72 + 7 * 9 + 4] + mr2.w * kernelsLA[0 * 72 + 7 * 9 + 5] +
		bl2.w * kernelsLA[0 * 72 + 7 * 9 + 6] + bc2.w * kernelsLA[0 * 72 + 7 * 9 + 7] + br2.w * kernelsLA[0 * 72 + 7 * 9 + 8] + biasLA.x
		,
		tl1.x * kernelsLA[1 * 72 + 0 * 9 + 0] + tc1.x * kernelsLA[1 * 72 + 0 * 9 + 1] + tr1.x * kernelsLA[1 * 72 + 0 * 9 + 2] +
		ml1.x * kernelsLA[1 * 72 + 0 * 9 + 3] + mc1.x * kernelsLA[1 * 72 + 0 * 9 + 4] + mr1.x * kernelsLA[1 * 72 + 0 * 9 + 5] +
		bl1.x * kernelsLA[1 * 72 + 0 * 9 + 6] + bc1.x * kernelsLA[1 * 72 + 0 * 9 + 7] + br1.x * kernelsLA[1 * 72 + 0 * 9 + 8] +

		tl1.y * kernelsLA[1 * 72 + 1 * 9 + 0] + tc1.y * kernelsLA[1 * 72 + 1 * 9 + 1] + tr1.y * kernelsLA[1 * 72 + 1 * 9 + 2] +
		ml1.y * kernelsLA[1 * 72 + 1 * 9 + 3] + mc1.y * kernelsLA[1 * 72 + 1 * 9 + 4] + mr1.y * kernelsLA[1 * 72 + 1 * 9 + 5] +
		bl1.y * kernelsLA[1 * 72 + 1 * 9 + 6] + bc1.y * kernelsLA[1 * 72 + 1 * 9 + 7] + br1.y * kernelsLA[1 * 72 + 1 * 9 + 8] +

		tl1.z * kernelsLA[1 * 72 + 2 * 9 + 0] + tc1.z * kernelsLA[1 * 72 + 2 * 9 + 1] + tr1.z * kernelsLA[1 * 72 + 2 * 9 + 2] +
		ml1.z * kernelsLA[1 * 72 + 2 * 9 + 3] + mc1.z * kernelsLA[1 * 72 + 2 * 9 + 4] + mr1.z * kernelsLA[1 * 72 + 2 * 9 + 5] +
		bl1.z * kernelsLA[1 * 72 + 2 * 9 + 6] + bc1.z * kernelsLA[1 * 72 + 2 * 9 + 7] + br1.z * kernelsLA[1 * 72 + 2 * 9 + 8] +

		tl1.w * kernelsLA[1 * 72 + 3 * 9 + 0] + tc1.w * kernelsLA[1 * 72 + 3 * 9 + 1] + tr1.w * kernelsLA[1 * 72 + 3 * 9 + 2] +
		ml1.w * kernelsLA[1 * 72 + 3 * 9 + 3] + mc1.w * kernelsLA[1 * 72 + 3 * 9 + 4] + mr1.w * kernelsLA[1 * 72 + 3 * 9 + 5] +
		bl1.w * kernelsLA[1 * 72 + 3 * 9 + 6] + bc1.w * kernelsLA[1 * 72 + 3 * 9 + 7] + br1.w * kernelsLA[1 * 72 + 3 * 9 + 8] +

		tl2.x * kernelsLA[1 * 72 + 4 * 9 + 0] + tc2.x * kernelsLA[1 * 72 + 4 * 9 + 1] + tr2.x * kernelsLA[1 * 72 + 4 * 9 + 2] +
		ml2.x * kernelsLA[1 * 72 + 4 * 9 + 3] + mc2.x * kernelsLA[1 * 72 + 4 * 9 + 4] + mr2.x * kernelsLA[1 * 72 + 4 * 9 + 5] +
		bl2.x * kernelsLA[1 * 72 + 4 * 9 + 6] + bc2.x * kernelsLA[1 * 72 + 4 * 9 + 7] + br2.x * kernelsLA[1 * 72 + 4 * 9 + 8] +

		tl2.y * kernelsLA[1 * 72 + 5 * 9 + 0] + tc2.y * kernelsLA[1 * 72 + 5 * 9 + 1] + tr2.y * kernelsLA[1 * 72 + 5 * 9 + 2] +
		ml2.y * kernelsLA[1 * 72 + 5 * 9 + 3] + mc2.y * kernelsLA[1 * 72 + 5 * 9 + 4] + mr2.y * kernelsLA[1 * 72 + 5 * 9 + 5] +
		bl2.y * kernelsLA[1 * 72 + 5 * 9 + 6] + bc2.y * kernelsLA[1 * 72 + 5 * 9 + 7] + br2.y * kernelsLA[1 * 72 + 5 * 9 + 8] +

		tl2.z * kernelsLA[1 * 72 + 6 * 9 + 0] + tc2.z * kernelsLA[1 * 72 + 6 * 9 + 1] + tr2.z * kernelsLA[1 * 72 + 6 * 9 + 2] +
		ml2.z * kernelsLA[1 * 72 + 6 * 9 + 3] + mc2.z * kernelsLA[1 * 72 + 6 * 9 + 4] + mr2.z * kernelsLA[1 * 72 + 6 * 9 + 5] +
		bl2.z * kernelsLA[1 * 72 + 6 * 9 + 6] + bc2.z * kernelsLA[1 * 72 + 6 * 9 + 7] + br2.z * kernelsLA[1 * 72 + 6 * 9 + 8] +

		tl2.w * kernelsLA[1 * 72 + 7 * 9 + 0] + tc2.w * kernelsLA[1 * 72 + 7 * 9 + 1] + tr2.w * kernelsLA[1 * 72 + 7 * 9 + 2] +
		ml2.w * kernelsLA[1 * 72 + 7 * 9 + 3] + mc2.w * kernelsLA[1 * 72 + 7 * 9 + 4] + mr2.w * kernelsLA[1 * 72 + 7 * 9 + 5] +
		bl2.w * kernelsLA[1 * 72 + 7 * 9 + 6] + bc2.w * kernelsLA[1 * 72 + 7 * 9 + 7] + br2.w * kernelsLA[1 * 72 + 7 * 9 + 8] + biasLA.y
		,
		tl1.x * kernelsLA[2 * 72 + 0 * 9 + 0] + tc1.x * kernelsLA[2 * 72 + 0 * 9 + 1] + tr1.x * kernelsLA[2 * 72 + 0 * 9 + 2] +
		ml1.x * kernelsLA[2 * 72 + 0 * 9 + 3] + mc1.x * kernelsLA[2 * 72 + 0 * 9 + 4] + mr1.x * kernelsLA[2 * 72 + 0 * 9 + 5] +
		bl1.x * kernelsLA[2 * 72 + 0 * 9 + 6] + bc1.x * kernelsLA[2 * 72 + 0 * 9 + 7] + br1.x * kernelsLA[2 * 72 + 0 * 9 + 8] +

		tl1.y * kernelsLA[2 * 72 + 1 * 9 + 0] + tc1.y * kernelsLA[2 * 72 + 1 * 9 + 1] + tr1.y * kernelsLA[2 * 72 + 1 * 9 + 2] +
		ml1.y * kernelsLA[2 * 72 + 1 * 9 + 3] + mc1.y * kernelsLA[2 * 72 + 1 * 9 + 4] + mr1.y * kernelsLA[2 * 72 + 1 * 9 + 5] +
		bl1.y * kernelsLA[2 * 72 + 1 * 9 + 6] + bc1.y * kernelsLA[2 * 72 + 1 * 9 + 7] + br1.y * kernelsLA[2 * 72 + 1 * 9 + 8] +

		tl1.z * kernelsLA[2 * 72 + 2 * 9 + 0] + tc1.z * kernelsLA[2 * 72 + 2 * 9 + 1] + tr1.z * kernelsLA[2 * 72 + 2 * 9 + 2] +
		ml1.z * kernelsLA[2 * 72 + 2 * 9 + 3] + mc1.z * kernelsLA[2 * 72 + 2 * 9 + 4] + mr1.z * kernelsLA[2 * 72 + 2 * 9 + 5] +
		bl1.z * kernelsLA[2 * 72 + 2 * 9 + 6] + bc1.z * kernelsLA[2 * 72 + 2 * 9 + 7] + br1.z * kernelsLA[2 * 72 + 2 * 9 + 8] +

		tl1.w * kernelsLA[2 * 72 + 3 * 9 + 0] + tc1.w * kernelsLA[2 * 72 + 3 * 9 + 1] + tr1.w * kernelsLA[2 * 72 + 3 * 9 + 2] +
		ml1.w * kernelsLA[2 * 72 + 3 * 9 + 3] + mc1.w * kernelsLA[2 * 72 + 3 * 9 + 4] + mr1.w * kernelsLA[2 * 72 + 3 * 9 + 5] +
		bl1.w * kernelsLA[2 * 72 + 3 * 9 + 6] + bc1.w * kernelsLA[2 * 72 + 3 * 9 + 7] + br1.w * kernelsLA[2 * 72 + 3 * 9 + 8] +

		tl2.x * kernelsLA[2 * 72 + 4 * 9 + 0] + tc2.x * kernelsLA[2 * 72 + 4 * 9 + 1] + tr2.x * kernelsLA[2 * 72 + 4 * 9 + 2] +
		ml2.x * kernelsLA[2 * 72 + 4 * 9 + 3] + mc2.x * kernelsLA[2 * 72 + 4 * 9 + 4] + mr2.x * kernelsLA[2 * 72 + 4 * 9 + 5] +
		bl2.x * kernelsLA[2 * 72 + 4 * 9 + 6] + bc2.x * kernelsLA[2 * 72 + 4 * 9 + 7] + br2.x * kernelsLA[2 * 72 + 4 * 9 + 8] +

		tl2.y * kernelsLA[2 * 72 + 5 * 9 + 0] + tc2.y * kernelsLA[2 * 72 + 5 * 9 + 1] + tr2.y * kernelsLA[2 * 72 + 5 * 9 + 2] +
		ml2.y * kernelsLA[2 * 72 + 5 * 9 + 3] + mc2.y * kernelsLA[2 * 72 + 5 * 9 + 4] + mr2.y * kernelsLA[2 * 72 + 5 * 9 + 5] +
		bl2.y * kernelsLA[2 * 72 + 5 * 9 + 6] + bc2.y * kernelsLA[2 * 72 + 5 * 9 + 7] + br2.y * kernelsLA[2 * 72 + 5 * 9 + 8] +

		tl2.z * kernelsLA[2 * 72 + 6 * 9 + 0] + tc2.z * kernelsLA[2 * 72 + 6 * 9 + 1] + tr2.z * kernelsLA[2 * 72 + 6 * 9 + 2] +
		ml2.z * kernelsLA[2 * 72 + 6 * 9 + 3] + mc2.z * kernelsLA[2 * 72 + 6 * 9 + 4] + mr2.z * kernelsLA[2 * 72 + 6 * 9 + 5] +
		bl2.z * kernelsLA[2 * 72 + 6 * 9 + 6] + bc2.z * kernelsLA[2 * 72 + 6 * 9 + 7] + br2.z * kernelsLA[2 * 72 + 6 * 9 + 8] +

		tl2.w * kernelsLA[2 * 72 + 7 * 9 + 0] + tc2.w * kernelsLA[2 * 72 + 7 * 9 + 1] + tr2.w * kernelsLA[2 * 72 + 7 * 9 + 2] +
		ml2.w * kernelsLA[2 * 72 + 7 * 9 + 3] + mc2.w * kernelsLA[2 * 72 + 7 * 9 + 4] + mr2.w * kernelsLA[2 * 72 + 7 * 9 + 5] +
		bl2.w * kernelsLA[2 * 72 + 7 * 9 + 6] + bc2.w * kernelsLA[2 * 72 + 7 * 9 + 7] + br2.w * kernelsLA[2 * 72 + 7 * 9 + 8] + biasLA.z
		,
		tl1.x * kernelsLA[3 * 72 + 0 * 9 + 0] + tc1.x * kernelsLA[3 * 72 + 0 * 9 + 1] + tr1.x * kernelsLA[3 * 72 + 0 * 9 + 2] +
		ml1.x * kernelsLA[3 * 72 + 0 * 9 + 3] + mc1.x * kernelsLA[3 * 72 + 0 * 9 + 4] + mr1.x * kernelsLA[3 * 72 + 0 * 9 + 5] +
		bl1.x * kernelsLA[3 * 72 + 0 * 9 + 6] + bc1.x * kernelsLA[3 * 72 + 0 * 9 + 7] + br1.x * kernelsLA[3 * 72 + 0 * 9 + 8] +

		tl1.y * kernelsLA[3 * 72 + 1 * 9 + 0] + tc1.y * kernelsLA[3 * 72 + 1 * 9 + 1] + tr1.y * kernelsLA[3 * 72 + 1 * 9 + 2] +
		ml1.y * kernelsLA[3 * 72 + 1 * 9 + 3] + mc1.y * kernelsLA[3 * 72 + 1 * 9 + 4] + mr1.y * kernelsLA[3 * 72 + 1 * 9 + 5] +
		bl1.y * kernelsLA[3 * 72 + 1 * 9 + 6] + bc1.y * kernelsLA[3 * 72 + 1 * 9 + 7] + br1.y * kernelsLA[3 * 72 + 1 * 9 + 8] +

		tl1.z * kernelsLA[3 * 72 + 2 * 9 + 0] + tc1.z * kernelsLA[3 * 72 + 2 * 9 + 1] + tr1.z * kernelsLA[3 * 72 + 2 * 9 + 2] +
		ml1.z * kernelsLA[3 * 72 + 2 * 9 + 3] + mc1.z * kernelsLA[3 * 72 + 2 * 9 + 4] + mr1.z * kernelsLA[3 * 72 + 2 * 9 + 5] +
		bl1.z * kernelsLA[3 * 72 + 2 * 9 + 6] + bc1.z * kernelsLA[3 * 72 + 2 * 9 + 7] + br1.z * kernelsLA[3 * 72 + 2 * 9 + 8] +

		tl1.w * kernelsLA[3 * 72 + 3 * 9 + 0] + tc1.w * kernelsLA[3 * 72 + 3 * 9 + 1] + tr1.w * kernelsLA[3 * 72 + 3 * 9 + 2] +
		ml1.w * kernelsLA[3 * 72 + 3 * 9 + 3] + mc1.w * kernelsLA[3 * 72 + 3 * 9 + 4] + mr1.w * kernelsLA[3 * 72 + 3 * 9 + 5] +
		bl1.w * kernelsLA[3 * 72 + 3 * 9 + 6] + bc1.w * kernelsLA[3 * 72 + 3 * 9 + 7] + br1.w * kernelsLA[3 * 72 + 3 * 9 + 8] +

		tl2.x * kernelsLA[3 * 72 + 4 * 9 + 0] + tc2.x * kernelsLA[3 * 72 + 4 * 9 + 1] + tr2.x * kernelsLA[3 * 72 + 4 * 9 + 2] +
		ml2.x * kernelsLA[3 * 72 + 4 * 9 + 3] + mc2.x * kernelsLA[3 * 72 + 4 * 9 + 4] + mr2.x * kernelsLA[3 * 72 + 4 * 9 + 5] +
		bl2.x * kernelsLA[3 * 72 + 4 * 9 + 6] + bc2.x * kernelsLA[3 * 72 + 4 * 9 + 7] + br2.x * kernelsLA[3 * 72 + 4 * 9 + 8] +

		tl2.y * kernelsLA[3 * 72 + 5 * 9 + 0] + tc2.y * kernelsLA[3 * 72 + 5 * 9 + 1] + tr2.y * kernelsLA[3 * 72 + 5 * 9 + 2] +
		ml2.y * kernelsLA[3 * 72 + 5 * 9 + 3] + mc2.y * kernelsLA[3 * 72 + 5 * 9 + 4] + mr2.y * kernelsLA[3 * 72 + 5 * 9 + 5] +
		bl2.y * kernelsLA[3 * 72 + 5 * 9 + 6] + bc2.y * kernelsLA[3 * 72 + 5 * 9 + 7] + br2.y * kernelsLA[3 * 72 + 5 * 9 + 8] +

		tl2.z * kernelsLA[3 * 72 + 6 * 9 + 0] + tc2.z * kernelsLA[3 * 72 + 6 * 9 + 1] + tr2.z * kernelsLA[3 * 72 + 6 * 9 + 2] +
		ml2.z * kernelsLA[3 * 72 + 6 * 9 + 3] + mc2.z * kernelsLA[3 * 72 + 6 * 9 + 4] + mr2.z * kernelsLA[3 * 72 + 6 * 9 + 5] +
		bl2.z * kernelsLA[3 * 72 + 6 * 9 + 6] + bc2.z * kernelsLA[3 * 72 + 6 * 9 + 7] + br2.z * kernelsLA[3 * 72 + 6 * 9 + 8] +

		tl2.w * kernelsLA[3 * 72 + 7 * 9 + 0] + tc2.w * kernelsLA[3 * 72 + 7 * 9 + 1] + tr2.w * kernelsLA[3 * 72 + 7 * 9 + 2] +
		ml2.w * kernelsLA[3 * 72 + 7 * 9 + 3] + mc2.w * kernelsLA[3 * 72 + 7 * 9 + 4] + mr2.w * kernelsLA[3 * 72 + 7 * 9 + 5] +
		bl2.w * kernelsLA[3 * 72 + 7 * 9 + 6] + bc2.w * kernelsLA[3 * 72 + 7 * 9 + 7] + br2.w * kernelsLA[3 * 72 + 7 * 9 + 8] + biasLA.w
	));

	float4 target2 = RELU(float4(
		tl1.x * kernelsLB[0 * 72 + 0 * 9 + 0] + tc1.x * kernelsLB[0 * 72 + 0 * 9 + 1] + tr1.x * kernelsLB[0 * 72 + 0 * 9 + 2] +
		ml1.x * kernelsLB[0 * 72 + 0 * 9 + 3] + mc1.x * kernelsLB[0 * 72 + 0 * 9 + 4] + mr1.x * kernelsLB[0 * 72 + 0 * 9 + 5] +
		bl1.x * kernelsLB[0 * 72 + 0 * 9 + 6] + bc1.x * kernelsLB[0 * 72 + 0 * 9 + 7] + br1.x * kernelsLB[0 * 72 + 0 * 9 + 8] +

		tl1.y * kernelsLB[0 * 72 + 1 * 9 + 0] + tc1.y * kernelsLB[0 * 72 + 1 * 9 + 1] + tr1.y * kernelsLB[0 * 72 + 1 * 9 + 2] +
		ml1.y * kernelsLB[0 * 72 + 1 * 9 + 3] + mc1.y * kernelsLB[0 * 72 + 1 * 9 + 4] + mr1.y * kernelsLB[0 * 72 + 1 * 9 + 5] +
		bl1.y * kernelsLB[0 * 72 + 1 * 9 + 6] + bc1.y * kernelsLB[0 * 72 + 1 * 9 + 7] + br1.y * kernelsLB[0 * 72 + 1 * 9 + 8] +

		tl1.z * kernelsLB[0 * 72 + 2 * 9 + 0] + tc1.z * kernelsLB[0 * 72 + 2 * 9 + 1] + tr1.z * kernelsLB[0 * 72 + 2 * 9 + 2] +
		ml1.z * kernelsLB[0 * 72 + 2 * 9 + 3] + mc1.z * kernelsLB[0 * 72 + 2 * 9 + 4] + mr1.z * kernelsLB[0 * 72 + 2 * 9 + 5] +
		bl1.z * kernelsLB[0 * 72 + 2 * 9 + 6] + bc1.z * kernelsLB[0 * 72 + 2 * 9 + 7] + br1.z * kernelsLB[0 * 72 + 2 * 9 + 8] +

		tl1.w * kernelsLB[0 * 72 + 3 * 9 + 0] + tc1.w * kernelsLB[0 * 72 + 3 * 9 + 1] + tr1.w * kernelsLB[0 * 72 + 3 * 9 + 2] +
		ml1.w * kernelsLB[0 * 72 + 3 * 9 + 3] + mc1.w * kernelsLB[0 * 72 + 3 * 9 + 4] + mr1.w * kernelsLB[0 * 72 + 3 * 9 + 5] +
		bl1.w * kernelsLB[0 * 72 + 3 * 9 + 6] + bc1.w * kernelsLB[0 * 72 + 3 * 9 + 7] + br1.w * kernelsLB[0 * 72 + 3 * 9 + 8] +

		tl2.x * kernelsLB[0 * 72 + 4 * 9 + 0] + tc2.x * kernelsLB[0 * 72 + 4 * 9 + 1] + tr2.x * kernelsLB[0 * 72 + 4 * 9 + 2] +
		ml2.x * kernelsLB[0 * 72 + 4 * 9 + 3] + mc2.x * kernelsLB[0 * 72 + 4 * 9 + 4] + mr2.x * kernelsLB[0 * 72 + 4 * 9 + 5] +
		bl2.x * kernelsLB[0 * 72 + 4 * 9 + 6] + bc2.x * kernelsLB[0 * 72 + 4 * 9 + 7] + br2.x * kernelsLB[0 * 72 + 4 * 9 + 8] +

		tl2.y * kernelsLB[0 * 72 + 5 * 9 + 0] + tc2.y * kernelsLB[0 * 72 + 5 * 9 + 1] + tr2.y * kernelsLB[0 * 72 + 5 * 9 + 2] +
		ml2.y * kernelsLB[0 * 72 + 5 * 9 + 3] + mc2.y * kernelsLB[0 * 72 + 5 * 9 + 4] + mr2.y * kernelsLB[0 * 72 + 5 * 9 + 5] +
		bl2.y * kernelsLB[0 * 72 + 5 * 9 + 6] + bc2.y * kernelsLB[0 * 72 + 5 * 9 + 7] + br2.y * kernelsLB[0 * 72 + 5 * 9 + 8] +

		tl2.z * kernelsLB[0 * 72 + 6 * 9 + 0] + tc2.z * kernelsLB[0 * 72 + 6 * 9 + 1] + tr2.z * kernelsLB[0 * 72 + 6 * 9 + 2] +
		ml2.z * kernelsLB[0 * 72 + 6 * 9 + 3] + mc2.z * kernelsLB[0 * 72 + 6 * 9 + 4] + mr2.z * kernelsLB[0 * 72 + 6 * 9 + 5] +
		bl2.z * kernelsLB[0 * 72 + 6 * 9 + 6] + bc2.z * kernelsLB[0 * 72 + 6 * 9 + 7] + br2.z * kernelsLB[0 * 72 + 6 * 9 + 8] +

		tl2.w * kernelsLB[0 * 72 + 7 * 9 + 0] + tc2.w * kernelsLB[0 * 72 + 7 * 9 + 1] + tr2.w * kernelsLB[0 * 72 + 7 * 9 + 2] +
		ml2.w * kernelsLB[0 * 72 + 7 * 9 + 3] + mc2.w * kernelsLB[0 * 72 + 7 * 9 + 4] + mr2.w * kernelsLB[0 * 72 + 7 * 9 + 5] +
		bl2.w * kernelsLB[0 * 72 + 7 * 9 + 6] + bc2.w * kernelsLB[0 * 72 + 7 * 9 + 7] + br2.w * kernelsLB[0 * 72 + 7 * 9 + 8] + biasLB.x
		,
		tl1.x * kernelsLB[1 * 72 + 0 * 9 + 0] + tc1.x * kernelsLB[1 * 72 + 0 * 9 + 1] + tr1.x * kernelsLB[1 * 72 + 0 * 9 + 2] +
		ml1.x * kernelsLB[1 * 72 + 0 * 9 + 3] + mc1.x * kernelsLB[1 * 72 + 0 * 9 + 4] + mr1.x * kernelsLB[1 * 72 + 0 * 9 + 5] +
		bl1.x * kernelsLB[1 * 72 + 0 * 9 + 6] + bc1.x * kernelsLB[1 * 72 + 0 * 9 + 7] + br1.x * kernelsLB[1 * 72 + 0 * 9 + 8] +

		tl1.y * kernelsLB[1 * 72 + 1 * 9 + 0] + tc1.y * kernelsLB[1 * 72 + 1 * 9 + 1] + tr1.y * kernelsLB[1 * 72 + 1 * 9 + 2] +
		ml1.y * kernelsLB[1 * 72 + 1 * 9 + 3] + mc1.y * kernelsLB[1 * 72 + 1 * 9 + 4] + mr1.y * kernelsLB[1 * 72 + 1 * 9 + 5] +
		bl1.y * kernelsLB[1 * 72 + 1 * 9 + 6] + bc1.y * kernelsLB[1 * 72 + 1 * 9 + 7] + br1.y * kernelsLB[1 * 72 + 1 * 9 + 8] +

		tl1.z * kernelsLB[1 * 72 + 2 * 9 + 0] + tc1.z * kernelsLB[1 * 72 + 2 * 9 + 1] + tr1.z * kernelsLB[1 * 72 + 2 * 9 + 2] +
		ml1.z * kernelsLB[1 * 72 + 2 * 9 + 3] + mc1.z * kernelsLB[1 * 72 + 2 * 9 + 4] + mr1.z * kernelsLB[1 * 72 + 2 * 9 + 5] +
		bl1.z * kernelsLB[1 * 72 + 2 * 9 + 6] + bc1.z * kernelsLB[1 * 72 + 2 * 9 + 7] + br1.z * kernelsLB[1 * 72 + 2 * 9 + 8] +

		tl1.w * kernelsLB[1 * 72 + 3 * 9 + 0] + tc1.w * kernelsLB[1 * 72 + 3 * 9 + 1] + tr1.w * kernelsLB[1 * 72 + 3 * 9 + 2] +
		ml1.w * kernelsLB[1 * 72 + 3 * 9 + 3] + mc1.w * kernelsLB[1 * 72 + 3 * 9 + 4] + mr1.w * kernelsLB[1 * 72 + 3 * 9 + 5] +
		bl1.w * kernelsLB[1 * 72 + 3 * 9 + 6] + bc1.w * kernelsLB[1 * 72 + 3 * 9 + 7] + br1.w * kernelsLB[1 * 72 + 3 * 9 + 8] +

		tl2.x * kernelsLB[1 * 72 + 4 * 9 + 0] + tc2.x * kernelsLB[1 * 72 + 4 * 9 + 1] + tr2.x * kernelsLB[1 * 72 + 4 * 9 + 2] +
		ml2.x * kernelsLB[1 * 72 + 4 * 9 + 3] + mc2.x * kernelsLB[1 * 72 + 4 * 9 + 4] + mr2.x * kernelsLB[1 * 72 + 4 * 9 + 5] +
		bl2.x * kernelsLB[1 * 72 + 4 * 9 + 6] + bc2.x * kernelsLB[1 * 72 + 4 * 9 + 7] + br2.x * kernelsLB[1 * 72 + 4 * 9 + 8] +

		tl2.y * kernelsLB[1 * 72 + 5 * 9 + 0] + tc2.y * kernelsLB[1 * 72 + 5 * 9 + 1] + tr2.y * kernelsLB[1 * 72 + 5 * 9 + 2] +
		ml2.y * kernelsLB[1 * 72 + 5 * 9 + 3] + mc2.y * kernelsLB[1 * 72 + 5 * 9 + 4] + mr2.y * kernelsLB[1 * 72 + 5 * 9 + 5] +
		bl2.y * kernelsLB[1 * 72 + 5 * 9 + 6] + bc2.y * kernelsLB[1 * 72 + 5 * 9 + 7] + br2.y * kernelsLB[1 * 72 + 5 * 9 + 8] +

		tl2.z * kernelsLB[1 * 72 + 6 * 9 + 0] + tc2.z * kernelsLB[1 * 72 + 6 * 9 + 1] + tr2.z * kernelsLB[1 * 72 + 6 * 9 + 2] +
		ml2.z * kernelsLB[1 * 72 + 6 * 9 + 3] + mc2.z * kernelsLB[1 * 72 + 6 * 9 + 4] + mr2.z * kernelsLB[1 * 72 + 6 * 9 + 5] +
		bl2.z * kernelsLB[1 * 72 + 6 * 9 + 6] + bc2.z * kernelsLB[1 * 72 + 6 * 9 + 7] + br2.z * kernelsLB[1 * 72 + 6 * 9 + 8] +

		tl2.w * kernelsLB[1 * 72 + 7 * 9 + 0] + tc2.w * kernelsLB[1 * 72 + 7 * 9 + 1] + tr2.w * kernelsLB[1 * 72 + 7 * 9 + 2] +
		ml2.w * kernelsLB[1 * 72 + 7 * 9 + 3] + mc2.w * kernelsLB[1 * 72 + 7 * 9 + 4] + mr2.w * kernelsLB[1 * 72 + 7 * 9 + 5] +
		bl2.w * kernelsLB[1 * 72 + 7 * 9 + 6] + bc2.w * kernelsLB[1 * 72 + 7 * 9 + 7] + br2.w * kernelsLB[1 * 72 + 7 * 9 + 8] + biasLB.y
		,
		tl1.x * kernelsLB[2 * 72 + 0 * 9 + 0] + tc1.x * kernelsLB[2 * 72 + 0 * 9 + 1] + tr1.x * kernelsLB[2 * 72 + 0 * 9 + 2] +
		ml1.x * kernelsLB[2 * 72 + 0 * 9 + 3] + mc1.x * kernelsLB[2 * 72 + 0 * 9 + 4] + mr1.x * kernelsLB[2 * 72 + 0 * 9 + 5] +
		bl1.x * kernelsLB[2 * 72 + 0 * 9 + 6] + bc1.x * kernelsLB[2 * 72 + 0 * 9 + 7] + br1.x * kernelsLB[2 * 72 + 0 * 9 + 8] +

		tl1.y * kernelsLB[2 * 72 + 1 * 9 + 0] + tc1.y * kernelsLB[2 * 72 + 1 * 9 + 1] + tr1.y * kernelsLB[2 * 72 + 1 * 9 + 2] +
		ml1.y * kernelsLB[2 * 72 + 1 * 9 + 3] + mc1.y * kernelsLB[2 * 72 + 1 * 9 + 4] + mr1.y * kernelsLB[2 * 72 + 1 * 9 + 5] +
		bl1.y * kernelsLB[2 * 72 + 1 * 9 + 6] + bc1.y * kernelsLB[2 * 72 + 1 * 9 + 7] + br1.y * kernelsLB[2 * 72 + 1 * 9 + 8] +

		tl1.z * kernelsLB[2 * 72 + 2 * 9 + 0] + tc1.z * kernelsLB[2 * 72 + 2 * 9 + 1] + tr1.z * kernelsLB[2 * 72 + 2 * 9 + 2] +
		ml1.z * kernelsLB[2 * 72 + 2 * 9 + 3] + mc1.z * kernelsLB[2 * 72 + 2 * 9 + 4] + mr1.z * kernelsLB[2 * 72 + 2 * 9 + 5] +
		bl1.z * kernelsLB[2 * 72 + 2 * 9 + 6] + bc1.z * kernelsLB[2 * 72 + 2 * 9 + 7] + br1.z * kernelsLB[2 * 72 + 2 * 9 + 8] +

		tl1.w * kernelsLB[2 * 72 + 3 * 9 + 0] + tc1.w * kernelsLB[2 * 72 + 3 * 9 + 1] + tr1.w * kernelsLB[2 * 72 + 3 * 9 + 2] +
		ml1.w * kernelsLB[2 * 72 + 3 * 9 + 3] + mc1.w * kernelsLB[2 * 72 + 3 * 9 + 4] + mr1.w * kernelsLB[2 * 72 + 3 * 9 + 5] +
		bl1.w * kernelsLB[2 * 72 + 3 * 9 + 6] + bc1.w * kernelsLB[2 * 72 + 3 * 9 + 7] + br1.w * kernelsLB[2 * 72 + 3 * 9 + 8] +

		tl2.x * kernelsLB[2 * 72 + 4 * 9 + 0] + tc2.x * kernelsLB[2 * 72 + 4 * 9 + 1] + tr2.x * kernelsLB[2 * 72 + 4 * 9 + 2] +
		ml2.x * kernelsLB[2 * 72 + 4 * 9 + 3] + mc2.x * kernelsLB[2 * 72 + 4 * 9 + 4] + mr2.x * kernelsLB[2 * 72 + 4 * 9 + 5] +
		bl2.x * kernelsLB[2 * 72 + 4 * 9 + 6] + bc2.x * kernelsLB[2 * 72 + 4 * 9 + 7] + br2.x * kernelsLB[2 * 72 + 4 * 9 + 8] +

		tl2.y * kernelsLB[2 * 72 + 5 * 9 + 0] + tc2.y * kernelsLB[2 * 72 + 5 * 9 + 1] + tr2.y * kernelsLB[2 * 72 + 5 * 9 + 2] +
		ml2.y * kernelsLB[2 * 72 + 5 * 9 + 3] + mc2.y * kernelsLB[2 * 72 + 5 * 9 + 4] + mr2.y * kernelsLB[2 * 72 + 5 * 9 + 5] +
		bl2.y * kernelsLB[2 * 72 + 5 * 9 + 6] + bc2.y * kernelsLB[2 * 72 + 5 * 9 + 7] + br2.y * kernelsLB[2 * 72 + 5 * 9 + 8] +

		tl2.z * kernelsLB[2 * 72 + 6 * 9 + 0] + tc2.z * kernelsLB[2 * 72 + 6 * 9 + 1] + tr2.z * kernelsLB[2 * 72 + 6 * 9 + 2] +
		ml2.z * kernelsLB[2 * 72 + 6 * 9 + 3] + mc2.z * kernelsLB[2 * 72 + 6 * 9 + 4] + mr2.z * kernelsLB[2 * 72 + 6 * 9 + 5] +
		bl2.z * kernelsLB[2 * 72 + 6 * 9 + 6] + bc2.z * kernelsLB[2 * 72 + 6 * 9 + 7] + br2.z * kernelsLB[2 * 72 + 6 * 9 + 8] +

		tl2.w * kernelsLB[2 * 72 + 7 * 9 + 0] + tc2.w * kernelsLB[2 * 72 + 7 * 9 + 1] + tr2.w * kernelsLB[2 * 72 + 7 * 9 + 2] +
		ml2.w * kernelsLB[2 * 72 + 7 * 9 + 3] + mc2.w * kernelsLB[2 * 72 + 7 * 9 + 4] + mr2.w * kernelsLB[2 * 72 + 7 * 9 + 5] +
		bl2.w * kernelsLB[2 * 72 + 7 * 9 + 6] + bc2.w * kernelsLB[2 * 72 + 7 * 9 + 7] + br2.w * kernelsLB[2 * 72 + 7 * 9 + 8] + biasLB.z
		,
		tl1.x * kernelsLB[3 * 72 + 0 * 9 + 0] + tc1.x * kernelsLB[3 * 72 + 0 * 9 + 1] + tr1.x * kernelsLB[3 * 72 + 0 * 9 + 2] +
		ml1.x * kernelsLB[3 * 72 + 0 * 9 + 3] + mc1.x * kernelsLB[3 * 72 + 0 * 9 + 4] + mr1.x * kernelsLB[3 * 72 + 0 * 9 + 5] +
		bl1.x * kernelsLB[3 * 72 + 0 * 9 + 6] + bc1.x * kernelsLB[3 * 72 + 0 * 9 + 7] + br1.x * kernelsLB[3 * 72 + 0 * 9 + 8] +

		tl1.y * kernelsLB[3 * 72 + 1 * 9 + 0] + tc1.y * kernelsLB[3 * 72 + 1 * 9 + 1] + tr1.y * kernelsLB[3 * 72 + 1 * 9 + 2] +
		ml1.y * kernelsLB[3 * 72 + 1 * 9 + 3] + mc1.y * kernelsLB[3 * 72 + 1 * 9 + 4] + mr1.y * kernelsLB[3 * 72 + 1 * 9 + 5] +
		bl1.y * kernelsLB[3 * 72 + 1 * 9 + 6] + bc1.y * kernelsLB[3 * 72 + 1 * 9 + 7] + br1.y * kernelsLB[3 * 72 + 1 * 9 + 8] +

		tl1.z * kernelsLB[3 * 72 + 2 * 9 + 0] + tc1.z * kernelsLB[3 * 72 + 2 * 9 + 1] + tr1.z * kernelsLB[3 * 72 + 2 * 9 + 2] +
		ml1.z * kernelsLB[3 * 72 + 2 * 9 + 3] + mc1.z * kernelsLB[3 * 72 + 2 * 9 + 4] + mr1.z * kernelsLB[3 * 72 + 2 * 9 + 5] +
		bl1.z * kernelsLB[3 * 72 + 2 * 9 + 6] + bc1.z * kernelsLB[3 * 72 + 2 * 9 + 7] + br1.z * kernelsLB[3 * 72 + 2 * 9 + 8] +

		tl1.w * kernelsLB[3 * 72 + 3 * 9 + 0] + tc1.w * kernelsLB[3 * 72 + 3 * 9 + 1] + tr1.w * kernelsLB[3 * 72 + 3 * 9 + 2] +
		ml1.w * kernelsLB[3 * 72 + 3 * 9 + 3] + mc1.w * kernelsLB[3 * 72 + 3 * 9 + 4] + mr1.w * kernelsLB[3 * 72 + 3 * 9 + 5] +
		bl1.w * kernelsLB[3 * 72 + 3 * 9 + 6] + bc1.w * kernelsLB[3 * 72 + 3 * 9 + 7] + br1.w * kernelsLB[3 * 72 + 3 * 9 + 8] +

		tl2.x * kernelsLB[3 * 72 + 4 * 9 + 0] + tc2.x * kernelsLB[3 * 72 + 4 * 9 + 1] + tr2.x * kernelsLB[3 * 72 + 4 * 9 + 2] +
		ml2.x * kernelsLB[3 * 72 + 4 * 9 + 3] + mc2.x * kernelsLB[3 * 72 + 4 * 9 + 4] + mr2.x * kernelsLB[3 * 72 + 4 * 9 + 5] +
		bl2.x * kernelsLB[3 * 72 + 4 * 9 + 6] + bc2.x * kernelsLB[3 * 72 + 4 * 9 + 7] + br2.x * kernelsLB[3 * 72 + 4 * 9 + 8] +

		tl2.y * kernelsLB[3 * 72 + 5 * 9 + 0] + tc2.y * kernelsLB[3 * 72 + 5 * 9 + 1] + tr2.y * kernelsLB[3 * 72 + 5 * 9 + 2] +
		ml2.y * kernelsLB[3 * 72 + 5 * 9 + 3] + mc2.y * kernelsLB[3 * 72 + 5 * 9 + 4] + mr2.y * kernelsLB[3 * 72 + 5 * 9 + 5] +
		bl2.y * kernelsLB[3 * 72 + 5 * 9 + 6] + bc2.y * kernelsLB[3 * 72 + 5 * 9 + 7] + br2.y * kernelsLB[3 * 72 + 5 * 9 + 8] +

		tl2.z * kernelsLB[3 * 72 + 6 * 9 + 0] + tc2.z * kernelsLB[3 * 72 + 6 * 9 + 1] + tr2.z * kernelsLB[3 * 72 + 6 * 9 + 2] +
		ml2.z * kernelsLB[3 * 72 + 6 * 9 + 3] + mc2.z * kernelsLB[3 * 72 + 6 * 9 + 4] + mr2.z * kernelsLB[3 * 72 + 6 * 9 + 5] +
		bl2.z * kernelsLB[3 * 72 + 6 * 9 + 6] + bc2.z * kernelsLB[3 * 72 + 6 * 9 + 7] + br2.z * kernelsLB[3 * 72 + 6 * 9 + 8] +

		tl2.w * kernelsLB[3 * 72 + 7 * 9 + 0] + tc2.w * kernelsLB[3 * 72 + 7 * 9 + 1] + tr2.w * kernelsLB[3 * 72 + 7 * 9 + 2] +
		ml2.w * kernelsLB[3 * 72 + 7 * 9 + 3] + mc2.w * kernelsLB[3 * 72 + 7 * 9 + 4] + mr2.w * kernelsLB[3 * 72 + 7 * 9 + 5] +
		bl2.w * kernelsLB[3 * 72 + 7 * 9 + 6] + bc2.w * kernelsLB[3 * 72 + 7 * 9 + 7] + br2.w * kernelsLB[3 * 72 + 7 * 9 + 8] + biasLB.w
	));

	tex1[gxy] = target1;
	tex2[gxy] = target2;
}


//!PASS 6
//!IN tex1, tex2
//!OUT tex3, tex4
//!BLOCK_SIZE 8
//!NUM_THREADS 64

const static float kernelsLA[9 * 8 * 4] = {
	 5.6253e-02,  1.0118e-02, -8.2749e-02,
	-6.4074e-02,  4.0723e-02,  1.1657e-02,
	-1.1560e-01, -3.5596e-03, -2.6713e-02,
	-7.9090e-02, -2.9223e-01,  1.5759e-01,
	 6.8756e-02,  1.5738e-01,  1.5413e-01,
	-6.1288e-02, -1.2536e-01, -1.5966e-01,
	 1.1165e-01,  5.0211e-02, -1.0338e-01,
	-5.2364e-04,  1.7660e-01, -2.2504e-03,
	-1.7697e-01,  1.8500e-02,  2.0693e-02,
	-2.5907e-02, -1.4201e-01,  8.4467e-02,
	 1.1138e-02,  2.1769e-01, -4.2422e-01,
	 6.5046e-02,  2.6834e-02,  2.9047e-03,
	-1.2130e-01, -5.1773e-01, -8.0393e-02,
	 3.0204e-02,  3.5952e-01,  1.6681e-01,
	-9.4720e-04,  7.7291e-02,  8.3039e-02,
	 3.4689e-01, -1.2389e-01, -2.0666e-01,
	-2.9650e-02,  1.1102e-01, -1.4782e-01,
	 3.2193e-02, -3.9862e-02,  1.6440e-02,
	-8.4264e-02,  1.0192e-01, -6.4256e-02,
	 2.2950e-02, -6.6511e-02, -6.3814e-02,
	 4.3744e-02, -1.0557e-01, -1.2045e-02,
	 1.6330e-01,  6.6130e-01,  1.5497e-01,
	 1.7103e-01,  1.5073e-01,  1.7400e-01,
	 9.0985e-04,  1.0917e-02, -1.3322e-02,
	-6.4273e-02, -6.2178e-02, -7.7223e-02,
	-1.0332e-01, -2.1072e-01, -2.2843e-03,
	 3.2717e-02, -6.3754e-02,  5.0359e-02,
	-5.2566e-02,  6.2090e-02, -1.5614e-02,
	 1.4570e-02, -1.0243e-01,  1.3091e-01,
	-2.9988e-02, -7.5897e-02, -9.4541e-04,
	-2.7999e-01, -4.7415e-03,  5.6419e-02,
	 7.0565e-02, -4.9273e-01, -1.2936e-01,
	 5.5685e-02, -5.8924e-03, -3.1967e-02,
	 8.8602e-02,  2.9337e-01,  1.3753e-01,
	 1.0063e-02,  1.6348e-02,  1.0063e-01,
	 3.6230e-02,  1.7968e-02, -1.1624e-01,
	-2.2488e-02,  1.3474e-01, -1.1419e-01,
	 2.8576e-02, -7.4794e-02, -7.7261e-02,
	 5.8874e-02, -2.9448e-03,  6.0207e-02,
	 1.4642e-01,  1.2321e-01, -2.4936e-01,
	 2.2609e-02, -2.8171e-01,  1.1510e-01,
	 2.6056e-02, -2.7532e-02, -4.7505e-02,
	-2.8762e-02, -1.2610e-02, -8.3766e-02,
	-5.0992e-02, -5.7269e-03, -7.0981e-02,
	-9.6191e-02, -9.2384e-02, -5.3328e-02,
	 2.3989e-01,  3.9819e-01,  1.8451e-01,
	 3.6888e-02,  1.1023e-01,  4.4804e-03,
	-4.4140e-03, -4.8275e-03,  2.0018e-02,
	-2.4346e-02, -6.5546e-02, -4.6065e-03,
	 2.2298e-01,  2.8810e-01,  1.4071e-02,
	-1.7315e-01, -5.7961e-02, -9.9136e-02,
	 3.6456e-02, -1.5518e-02,  6.4490e-02,
	 4.6983e-02,  5.2743e-02,  3.0802e-01,
	 6.7940e-02,  5.8777e-03,  3.1155e-01,
	 9.9510e-02,  2.7974e-02, -6.6716e-02,
	 3.7042e-01,  2.0813e-01, -3.1581e-02,
	 7.9064e-02, -1.3699e-01, -4.4722e-02,
	-8.4753e-03,  8.0676e-02,  1.5771e-01,
	-1.1467e-01,  5.6269e-02,  1.1369e-01,
	-1.4727e-02,  3.7263e-02, -2.0554e-01,
	 8.3383e-02,  4.5848e-02, -1.1732e-02,
	 4.5494e-02, -2.1406e-01,  6.0591e-02,
	 4.6503e-02, -1.0362e-01,  3.8794e-02,
	-4.6633e-01,  1.4504e-01,  1.4999e-01,
	 2.9642e-01, -4.8807e-01, -1.6012e-01,
	 1.6708e-01,  9.5313e-02, -7.5981e-02,
	-4.2655e-02,  9.2470e-02, -7.7242e-02,
	-2.1021e-01,  1.2423e-01,  1.4967e-02,
	-5.4129e-02,  7.4355e-02, -4.7068e-02,
	-1.6048e-01,  9.8742e-02,  4.4282e-02,
	-6.0187e-02,  1.9495e-01,  8.3291e-02,
	-7.5190e-02, -6.8429e-02,  3.7391e-02,
	 5.1413e-04,  1.5098e-01, -1.1549e-01,
	 1.6875e-01,  1.8040e-01, -1.3162e-01,
	 7.7101e-02,  2.0816e-01,  7.6289e-02,
	-1.7528e-02,  1.4408e-02,  3.7500e-02,
	 3.8647e-02,  1.6850e-01,  1.7535e-02,
	-2.8205e-02,  1.0273e-02,  1.6688e-01,
	 4.3676e-02,  6.9895e-02,  8.1063e-03,
	-2.6117e-01, -1.0920e-01,  5.2209e-02,
	-5.2749e-02, -1.7062e-02, -9.6808e-02,
	 2.7324e-02,  9.1342e-02, -5.0968e-02,
	 1.0689e-01,  5.0565e-01,  4.6004e-01,
	-6.6862e-03,  3.4162e-03,  3.3559e-01,
	 3.5084e-02,  1.9123e-02,  1.0073e-02,
	 1.6995e-01,  3.4099e-01, -4.0847e-01,
	-5.5317e-03,  4.0230e-02, -2.0305e-01,
	-8.9786e-02,  1.9667e-01,  3.8111e-02,
	 3.0607e-02, -1.9084e-02, -6.5114e-02,
	 8.5394e-02, -1.3992e-01,  1.4988e-02,
	-1.5926e-02, -9.1200e-03, -7.2328e-02,
	 1.3548e-01,  7.1040e-01, -9.4208e-02,
	 2.5411e-03, -7.2159e-02,  1.0848e-01,
	-8.9029e-02, -8.6339e-02, -2.7546e-02,
	 6.0378e-02,  2.8401e-01, -6.6550e-02,
	-3.0486e-02,  5.0307e-02, -1.1084e-02
};

const static float4 biasLA = { -0.0046, -0.0104, -0.0087, -0.0040 };

const static float kernelsLB[9 * 8 * 4] = {
	 2.9732e-02,  9.9960e-02, -7.7408e-02,
	 3.4940e-01, -5.6048e-01,  2.9053e-02,
	-2.6991e-02,  4.9637e-02, -3.9322e-02,
	-1.0418e-02,  1.0931e-01, -6.1609e-02,
	 3.6057e-02,  9.3866e-02, -1.0339e-01,
	-1.8572e-02, -2.0889e-02, -7.4531e-02,
	-7.3236e-02, -4.5908e-02,  2.2705e-02,
	-1.5148e-02,  2.1735e-01,  2.2477e-02,
	-3.4153e-02, -2.6939e-02, -5.0167e-03,
	 6.6774e-02,  2.0168e-01, -7.5083e-02,
	 5.6608e-02,  2.2799e-01, -3.7473e-01,
	-7.2336e-02,  4.4329e-02, -3.6747e-02,
	 3.5355e-02,  1.8671e-01, -4.0167e-02,
	 1.2871e-01,  3.5050e-01,  1.8090e-01,
	-6.2429e-02,  6.2184e-02,  6.8804e-02,
	-8.0164e-02, -2.4387e-02, -5.0309e-03,
	 1.0089e-01, -3.0008e-02,  1.7251e-02,
	-9.4662e-03, -1.4760e-02,  7.3434e-03,
	 7.3290e-02,  2.2546e-02, -2.9015e-02,
	 7.9944e-02, -2.6972e-01,  7.1349e-02,
	-1.7026e-02,  1.1461e-01, -4.1288e-02,
	-5.3732e-02, -2.4618e-01, -1.2890e-02,
	 8.6133e-02,  1.9503e-01,  8.2202e-02,
	-1.0060e-03, -4.5931e-04, -1.8789e-02,
	-4.0843e-02, -7.8149e-03, -6.1464e-02,
	-7.9364e-02, -5.9647e-02, -5.4059e-03,
	 1.9553e-01, -2.4079e-01, -7.9538e-03,
	 5.3620e-02,  1.4198e-01,  6.5651e-03,
	 2.3512e-02, -2.6609e-02, -4.6435e-02,
	 1.2499e-02,  5.1079e-02, -2.2713e-02,
	-7.1554e-02,  1.0608e-01,  5.8972e-02,
	 1.8638e-01, -2.1053e-01, -6.4009e-02,
	 1.0851e-01,  7.2187e-02,  8.9722e-02,
	-4.5365e-04,  1.0826e-01, -6.4141e-02,
	-2.3874e-02, -4.6307e-02, -2.7813e-02,
	 1.8385e-02,  9.4687e-02,  6.8374e-02,
	 9.4526e-02,  1.4432e-02,  1.5937e-01,
	 1.1292e-01, -3.4274e-01, -1.0813e-01,
	-7.4636e-03,  3.7101e-02,  3.7226e-02,
	 3.7079e-02, -3.9169e-02, -3.7752e-02,
	-7.9021e-02,  8.5978e-02,  1.0958e-02,
	-5.8576e-02,  5.5931e-02,  4.8301e-02,
	-1.3402e-01, -3.3809e-01, -4.4369e-02,
	 1.4262e-01,  6.5254e-02, -3.3366e-01,
	 1.2416e-02, -9.0492e-02, -5.8205e-02,
	-1.4886e-01,  4.0598e-02, -1.4219e-01,
	 2.0223e-03, -2.8673e-01, -3.3622e-01,
	 1.9191e-02, -2.2104e-02,  1.9048e-02,
	 6.0021e-02,  2.2520e-01, -5.3972e-02,
	 1.6226e-01, -2.1918e-01, -5.2117e-02,
	-6.2363e-03,  2.0266e-01, -7.3323e-03,
	 1.1137e-01, -1.9300e-02, -5.4983e-02,
	-1.8338e-01,  6.2511e-01, -1.7909e-01,
	 1.7003e-01,  1.7902e-01,  5.4462e-02,
	 5.6847e-02, -7.4696e-02, -1.1354e-02,
	 1.0544e-01, -1.4918e-01,  4.8208e-02,
	-5.6262e-02, -2.3303e-01, -2.9916e-02,
	-3.3261e-02,  1.3287e-01,  1.9831e-02,
	-1.3907e-01, -1.6180e-01, -7.2323e-03,
	-5.1689e-02,  6.3121e-02, -1.4480e-01,
	 1.1143e-01,  4.9625e-02, -5.4369e-02,
	-3.9247e-01,  2.3412e-01, -3.6726e-02,
	-1.1468e-02,  3.4045e-02,  6.6454e-02,
	-5.0103e-02,  6.1740e-02,  4.2922e-03,
	 1.7669e-01, -8.1250e-03,  6.3694e-03,
	-6.7723e-02,  7.4576e-02,  1.0113e-02,
	 1.1264e-01, -4.4691e-02, -5.3575e-02,
	 3.4691e-02, -1.2201e-02, -8.4221e-02,
	 2.3677e-01,  3.9073e-01,  2.4710e-02,
	-8.4580e-02, -1.0747e-01, -6.5695e-02,
	 1.5386e-01,  1.4041e-01,  6.9961e-03,
	 2.6138e-02,  2.3149e-02, -1.8820e-02,
	-3.3541e-02,  3.2089e-02, -1.8916e-02,
	 1.0564e-01, -7.5319e-02, -5.4282e-02,
	-6.9388e-03, -2.0873e-02,  5.6100e-02,
	 2.3524e-02, -6.4296e-02,  5.8950e-02,
	-3.1415e-03, -4.1203e-02,  1.0781e-01,
	 1.7848e-02, -2.9535e-02, -1.6412e-02,
	-4.6649e-02,  8.1277e-02, -5.9918e-02,
	 8.1522e-02, -9.2037e-02,  8.1039e-03,
	-6.5541e-02,  5.1811e-02, -1.4380e-03,
	 5.0419e-02,  9.3091e-03, -2.8054e-02,
	-3.0979e-02, -2.5366e-02,  3.5265e-02,
	-3.7730e-02,  5.7574e-02,  3.4683e-02,
	 4.8819e-03, -2.9519e-02,  3.7740e-02,
	 6.4546e-02, -3.7272e-01, -8.5393e-02,
	-3.0223e-02, -7.7899e-02,  2.7365e-03,
	 2.2282e-02, -3.3440e-02,  1.9048e-02,
	 2.3275e-02, -2.1153e-02, -2.0385e-02,
	-4.6245e-02,  2.2443e-02, -3.0206e-02,
	-2.5302e-02, -1.1418e-02,  4.8228e-02,
	 5.8367e-02, -4.3062e-02,  2.2814e-02,
	-4.6279e-02,  5.0052e-02,  2.2961e-02,
	-5.4984e-02,  1.4773e-01, -2.5546e-02,
	 3.3025e-02, -1.0138e-01,  6.3886e-02,
	 1.2403e-02,  1.6215e-02,  1.0783e-02
};

const static float4 biasLB = { 0.1077,  0.0347, -0.0165,  0.7296 };


void Pass6(uint2 blockStart, uint3 threadId) {
	uint2 gxy = Rmp8x8(threadId.x) + blockStart;
	uint2 inputSize = GetInputSize();
	if (gxy.x >= inputSize.x || gxy.y >= inputSize.y) {
		return;
	}

	float2 inputPt = GetInputPt();
	float2 pos = (gxy + 0.5f) * inputPt;

	// [tl, tc, tr]
	// [ml, mc, mr]
	// [bl, bc, br]
	float4 tl1 = tex1.SampleLevel(sam, pos + float2(-inputPt.x, -inputPt.y), 0);
	float4 ml1 = tex1.SampleLevel(sam, pos + float2(-inputPt.x, 0), 0);
	float4 bl1 = tex1.SampleLevel(sam, pos + float2(-inputPt.x, inputPt.y), 0);
	float4 tc1 = tex1.SampleLevel(sam, pos + float2(0, -inputPt.y), 0);
	float4 mc1 = tex1.SampleLevel(sam, pos, 0);
	float4 bc1 = tex1.SampleLevel(sam, pos + float2(0, inputPt.y), 0);
	float4 tr1 = tex1.SampleLevel(sam, pos + float2(inputPt.x, -inputPt.y), 0);
	float4 mr1 = tex1.SampleLevel(sam, pos + float2(inputPt.x, 0), 0);
	float4 br1 = tex1.SampleLevel(sam, pos + float2(inputPt.x, inputPt.y), 0);

	float4 tl2 = tex2.SampleLevel(sam, pos + float2(-inputPt.x, -inputPt.y), 0);
	float4 ml2 = tex2.SampleLevel(sam, pos + float2(-inputPt.x, 0), 0);
	float4 bl2 = tex2.SampleLevel(sam, pos + float2(-inputPt.x, inputPt.y), 0);
	float4 tc2 = tex2.SampleLevel(sam, pos + float2(0, -inputPt.y), 0);
	float4 mc2 = tex2.SampleLevel(sam, pos, 0);
	float4 bc2 = tex2.SampleLevel(sam, pos + float2(0, inputPt.y), 0);
	float4 tr2 = tex2.SampleLevel(sam, pos + float2(inputPt.x, -inputPt.y), 0);
	float4 mr2 = tex2.SampleLevel(sam, pos + float2(inputPt.x, 0), 0);
	float4 br2 = tex2.SampleLevel(sam, pos + float2(inputPt.x, inputPt.y), 0);

	float4 target1 = RELU(float4(
		tl1.x * kernelsLA[0 * 72 + 0 * 9 + 0] + tc1.x * kernelsLA[0 * 72 + 0 * 9 + 1] + tr1.x * kernelsLA[0 * 72 + 0 * 9 + 2] +
		ml1.x * kernelsLA[0 * 72 + 0 * 9 + 3] + mc1.x * kernelsLA[0 * 72 + 0 * 9 + 4] + mr1.x * kernelsLA[0 * 72 + 0 * 9 + 5] +
		bl1.x * kernelsLA[0 * 72 + 0 * 9 + 6] + bc1.x * kernelsLA[0 * 72 + 0 * 9 + 7] + br1.x * kernelsLA[0 * 72 + 0 * 9 + 8] +

		tl1.y * kernelsLA[0 * 72 + 1 * 9 + 0] + tc1.y * kernelsLA[0 * 72 + 1 * 9 + 1] + tr1.y * kernelsLA[0 * 72 + 1 * 9 + 2] +
		ml1.y * kernelsLA[0 * 72 + 1 * 9 + 3] + mc1.y * kernelsLA[0 * 72 + 1 * 9 + 4] + mr1.y * kernelsLA[0 * 72 + 1 * 9 + 5] +
		bl1.y * kernelsLA[0 * 72 + 1 * 9 + 6] + bc1.y * kernelsLA[0 * 72 + 1 * 9 + 7] + br1.y * kernelsLA[0 * 72 + 1 * 9 + 8] +

		tl1.z * kernelsLA[0 * 72 + 2 * 9 + 0] + tc1.z * kernelsLA[0 * 72 + 2 * 9 + 1] + tr1.z * kernelsLA[0 * 72 + 2 * 9 + 2] +
		ml1.z * kernelsLA[0 * 72 + 2 * 9 + 3] + mc1.z * kernelsLA[0 * 72 + 2 * 9 + 4] + mr1.z * kernelsLA[0 * 72 + 2 * 9 + 5] +
		bl1.z * kernelsLA[0 * 72 + 2 * 9 + 6] + bc1.z * kernelsLA[0 * 72 + 2 * 9 + 7] + br1.z * kernelsLA[0 * 72 + 2 * 9 + 8] +

		tl1.w * kernelsLA[0 * 72 + 3 * 9 + 0] + tc1.w * kernelsLA[0 * 72 + 3 * 9 + 1] + tr1.w * kernelsLA[0 * 72 + 3 * 9 + 2] +
		ml1.w * kernelsLA[0 * 72 + 3 * 9 + 3] + mc1.w * kernelsLA[0 * 72 + 3 * 9 + 4] + mr1.w * kernelsLA[0 * 72 + 3 * 9 + 5] +
		bl1.w * kernelsLA[0 * 72 + 3 * 9 + 6] + bc1.w * kernelsLA[0 * 72 + 3 * 9 + 7] + br1.w * kernelsLA[0 * 72 + 3 * 9 + 8] +

		tl2.x * kernelsLA[0 * 72 + 4 * 9 + 0] + tc2.x * kernelsLA[0 * 72 + 4 * 9 + 1] + tr2.x * kernelsLA[0 * 72 + 4 * 9 + 2] +
		ml2.x * kernelsLA[0 * 72 + 4 * 9 + 3] + mc2.x * kernelsLA[0 * 72 + 4 * 9 + 4] + mr2.x * kernelsLA[0 * 72 + 4 * 9 + 5] +
		bl2.x * kernelsLA[0 * 72 + 4 * 9 + 6] + bc2.x * kernelsLA[0 * 72 + 4 * 9 + 7] + br2.x * kernelsLA[0 * 72 + 4 * 9 + 8] +

		tl2.y * kernelsLA[0 * 72 + 5 * 9 + 0] + tc2.y * kernelsLA[0 * 72 + 5 * 9 + 1] + tr2.y * kernelsLA[0 * 72 + 5 * 9 + 2] +
		ml2.y * kernelsLA[0 * 72 + 5 * 9 + 3] + mc2.y * kernelsLA[0 * 72 + 5 * 9 + 4] + mr2.y * kernelsLA[0 * 72 + 5 * 9 + 5] +
		bl2.y * kernelsLA[0 * 72 + 5 * 9 + 6] + bc2.y * kernelsLA[0 * 72 + 5 * 9 + 7] + br2.y * kernelsLA[0 * 72 + 5 * 9 + 8] +

		tl2.z * kernelsLA[0 * 72 + 6 * 9 + 0] + tc2.z * kernelsLA[0 * 72 + 6 * 9 + 1] + tr2.z * kernelsLA[0 * 72 + 6 * 9 + 2] +
		ml2.z * kernelsLA[0 * 72 + 6 * 9 + 3] + mc2.z * kernelsLA[0 * 72 + 6 * 9 + 4] + mr2.z * kernelsLA[0 * 72 + 6 * 9 + 5] +
		bl2.z * kernelsLA[0 * 72 + 6 * 9 + 6] + bc2.z * kernelsLA[0 * 72 + 6 * 9 + 7] + br2.z * kernelsLA[0 * 72 + 6 * 9 + 8] +

		tl2.w * kernelsLA[0 * 72 + 7 * 9 + 0] + tc2.w * kernelsLA[0 * 72 + 7 * 9 + 1] + tr2.w * kernelsLA[0 * 72 + 7 * 9 + 2] +
		ml2.w * kernelsLA[0 * 72 + 7 * 9 + 3] + mc2.w * kernelsLA[0 * 72 + 7 * 9 + 4] + mr2.w * kernelsLA[0 * 72 + 7 * 9 + 5] +
		bl2.w * kernelsLA[0 * 72 + 7 * 9 + 6] + bc2.w * kernelsLA[0 * 72 + 7 * 9 + 7] + br2.w * kernelsLA[0 * 72 + 7 * 9 + 8] + biasLA.x
		,
		tl1.x * kernelsLA[1 * 72 + 0 * 9 + 0] + tc1.x * kernelsLA[1 * 72 + 0 * 9 + 1] + tr1.x * kernelsLA[1 * 72 + 0 * 9 + 2] +
		ml1.x * kernelsLA[1 * 72 + 0 * 9 + 3] + mc1.x * kernelsLA[1 * 72 + 0 * 9 + 4] + mr1.x * kernelsLA[1 * 72 + 0 * 9 + 5] +
		bl1.x * kernelsLA[1 * 72 + 0 * 9 + 6] + bc1.x * kernelsLA[1 * 72 + 0 * 9 + 7] + br1.x * kernelsLA[1 * 72 + 0 * 9 + 8] +

		tl1.y * kernelsLA[1 * 72 + 1 * 9 + 0] + tc1.y * kernelsLA[1 * 72 + 1 * 9 + 1] + tr1.y * kernelsLA[1 * 72 + 1 * 9 + 2] +
		ml1.y * kernelsLA[1 * 72 + 1 * 9 + 3] + mc1.y * kernelsLA[1 * 72 + 1 * 9 + 4] + mr1.y * kernelsLA[1 * 72 + 1 * 9 + 5] +
		bl1.y * kernelsLA[1 * 72 + 1 * 9 + 6] + bc1.y * kernelsLA[1 * 72 + 1 * 9 + 7] + br1.y * kernelsLA[1 * 72 + 1 * 9 + 8] +

		tl1.z * kernelsLA[1 * 72 + 2 * 9 + 0] + tc1.z * kernelsLA[1 * 72 + 2 * 9 + 1] + tr1.z * kernelsLA[1 * 72 + 2 * 9 + 2] +
		ml1.z * kernelsLA[1 * 72 + 2 * 9 + 3] + mc1.z * kernelsLA[1 * 72 + 2 * 9 + 4] + mr1.z * kernelsLA[1 * 72 + 2 * 9 + 5] +
		bl1.z * kernelsLA[1 * 72 + 2 * 9 + 6] + bc1.z * kernelsLA[1 * 72 + 2 * 9 + 7] + br1.z * kernelsLA[1 * 72 + 2 * 9 + 8] +

		tl1.w * kernelsLA[1 * 72 + 3 * 9 + 0] + tc1.w * kernelsLA[1 * 72 + 3 * 9 + 1] + tr1.w * kernelsLA[1 * 72 + 3 * 9 + 2] +
		ml1.w * kernelsLA[1 * 72 + 3 * 9 + 3] + mc1.w * kernelsLA[1 * 72 + 3 * 9 + 4] + mr1.w * kernelsLA[1 * 72 + 3 * 9 + 5] +
		bl1.w * kernelsLA[1 * 72 + 3 * 9 + 6] + bc1.w * kernelsLA[1 * 72 + 3 * 9 + 7] + br1.w * kernelsLA[1 * 72 + 3 * 9 + 8] +

		tl2.x * kernelsLA[1 * 72 + 4 * 9 + 0] + tc2.x * kernelsLA[1 * 72 + 4 * 9 + 1] + tr2.x * kernelsLA[1 * 72 + 4 * 9 + 2] +
		ml2.x * kernelsLA[1 * 72 + 4 * 9 + 3] + mc2.x * kernelsLA[1 * 72 + 4 * 9 + 4] + mr2.x * kernelsLA[1 * 72 + 4 * 9 + 5] +
		bl2.x * kernelsLA[1 * 72 + 4 * 9 + 6] + bc2.x * kernelsLA[1 * 72 + 4 * 9 + 7] + br2.x * kernelsLA[1 * 72 + 4 * 9 + 8] +

		tl2.y * kernelsLA[1 * 72 + 5 * 9 + 0] + tc2.y * kernelsLA[1 * 72 + 5 * 9 + 1] + tr2.y * kernelsLA[1 * 72 + 5 * 9 + 2] +
		ml2.y * kernelsLA[1 * 72 + 5 * 9 + 3] + mc2.y * kernelsLA[1 * 72 + 5 * 9 + 4] + mr2.y * kernelsLA[1 * 72 + 5 * 9 + 5] +
		bl2.y * kernelsLA[1 * 72 + 5 * 9 + 6] + bc2.y * kernelsLA[1 * 72 + 5 * 9 + 7] + br2.y * kernelsLA[1 * 72 + 5 * 9 + 8] +

		tl2.z * kernelsLA[1 * 72 + 6 * 9 + 0] + tc2.z * kernelsLA[1 * 72 + 6 * 9 + 1] + tr2.z * kernelsLA[1 * 72 + 6 * 9 + 2] +
		ml2.z * kernelsLA[1 * 72 + 6 * 9 + 3] + mc2.z * kernelsLA[1 * 72 + 6 * 9 + 4] + mr2.z * kernelsLA[1 * 72 + 6 * 9 + 5] +
		bl2.z * kernelsLA[1 * 72 + 6 * 9 + 6] + bc2.z * kernelsLA[1 * 72 + 6 * 9 + 7] + br2.z * kernelsLA[1 * 72 + 6 * 9 + 8] +

		tl2.w * kernelsLA[1 * 72 + 7 * 9 + 0] + tc2.w * kernelsLA[1 * 72 + 7 * 9 + 1] + tr2.w * kernelsLA[1 * 72 + 7 * 9 + 2] +
		ml2.w * kernelsLA[1 * 72 + 7 * 9 + 3] + mc2.w * kernelsLA[1 * 72 + 7 * 9 + 4] + mr2.w * kernelsLA[1 * 72 + 7 * 9 + 5] +
		bl2.w * kernelsLA[1 * 72 + 7 * 9 + 6] + bc2.w * kernelsLA[1 * 72 + 7 * 9 + 7] + br2.w * kernelsLA[1 * 72 + 7 * 9 + 8] + biasLA.y
		,
		tl1.x * kernelsLA[2 * 72 + 0 * 9 + 0] + tc1.x * kernelsLA[2 * 72 + 0 * 9 + 1] + tr1.x * kernelsLA[2 * 72 + 0 * 9 + 2] +
		ml1.x * kernelsLA[2 * 72 + 0 * 9 + 3] + mc1.x * kernelsLA[2 * 72 + 0 * 9 + 4] + mr1.x * kernelsLA[2 * 72 + 0 * 9 + 5] +
		bl1.x * kernelsLA[2 * 72 + 0 * 9 + 6] + bc1.x * kernelsLA[2 * 72 + 0 * 9 + 7] + br1.x * kernelsLA[2 * 72 + 0 * 9 + 8] +

		tl1.y * kernelsLA[2 * 72 + 1 * 9 + 0] + tc1.y * kernelsLA[2 * 72 + 1 * 9 + 1] + tr1.y * kernelsLA[2 * 72 + 1 * 9 + 2] +
		ml1.y * kernelsLA[2 * 72 + 1 * 9 + 3] + mc1.y * kernelsLA[2 * 72 + 1 * 9 + 4] + mr1.y * kernelsLA[2 * 72 + 1 * 9 + 5] +
		bl1.y * kernelsLA[2 * 72 + 1 * 9 + 6] + bc1.y * kernelsLA[2 * 72 + 1 * 9 + 7] + br1.y * kernelsLA[2 * 72 + 1 * 9 + 8] +

		tl1.z * kernelsLA[2 * 72 + 2 * 9 + 0] + tc1.z * kernelsLA[2 * 72 + 2 * 9 + 1] + tr1.z * kernelsLA[2 * 72 + 2 * 9 + 2] +
		ml1.z * kernelsLA[2 * 72 + 2 * 9 + 3] + mc1.z * kernelsLA[2 * 72 + 2 * 9 + 4] + mr1.z * kernelsLA[2 * 72 + 2 * 9 + 5] +
		bl1.z * kernelsLA[2 * 72 + 2 * 9 + 6] + bc1.z * kernelsLA[2 * 72 + 2 * 9 + 7] + br1.z * kernelsLA[2 * 72 + 2 * 9 + 8] +

		tl1.w * kernelsLA[2 * 72 + 3 * 9 + 0] + tc1.w * kernelsLA[2 * 72 + 3 * 9 + 1] + tr1.w * kernelsLA[2 * 72 + 3 * 9 + 2] +
		ml1.w * kernelsLA[2 * 72 + 3 * 9 + 3] + mc1.w * kernelsLA[2 * 72 + 3 * 9 + 4] + mr1.w * kernelsLA[2 * 72 + 3 * 9 + 5] +
		bl1.w * kernelsLA[2 * 72 + 3 * 9 + 6] + bc1.w * kernelsLA[2 * 72 + 3 * 9 + 7] + br1.w * kernelsLA[2 * 72 + 3 * 9 + 8] +

		tl2.x * kernelsLA[2 * 72 + 4 * 9 + 0] + tc2.x * kernelsLA[2 * 72 + 4 * 9 + 1] + tr2.x * kernelsLA[2 * 72 + 4 * 9 + 2] +
		ml2.x * kernelsLA[2 * 72 + 4 * 9 + 3] + mc2.x * kernelsLA[2 * 72 + 4 * 9 + 4] + mr2.x * kernelsLA[2 * 72 + 4 * 9 + 5] +
		bl2.x * kernelsLA[2 * 72 + 4 * 9 + 6] + bc2.x * kernelsLA[2 * 72 + 4 * 9 + 7] + br2.x * kernelsLA[2 * 72 + 4 * 9 + 8] +

		tl2.y * kernelsLA[2 * 72 + 5 * 9 + 0] + tc2.y * kernelsLA[2 * 72 + 5 * 9 + 1] + tr2.y * kernelsLA[2 * 72 + 5 * 9 + 2] +
		ml2.y * kernelsLA[2 * 72 + 5 * 9 + 3] + mc2.y * kernelsLA[2 * 72 + 5 * 9 + 4] + mr2.y * kernelsLA[2 * 72 + 5 * 9 + 5] +
		bl2.y * kernelsLA[2 * 72 + 5 * 9 + 6] + bc2.y * kernelsLA[2 * 72 + 5 * 9 + 7] + br2.y * kernelsLA[2 * 72 + 5 * 9 + 8] +

		tl2.z * kernelsLA[2 * 72 + 6 * 9 + 0] + tc2.z * kernelsLA[2 * 72 + 6 * 9 + 1] + tr2.z * kernelsLA[2 * 72 + 6 * 9 + 2] +
		ml2.z * kernelsLA[2 * 72 + 6 * 9 + 3] + mc2.z * kernelsLA[2 * 72 + 6 * 9 + 4] + mr2.z * kernelsLA[2 * 72 + 6 * 9 + 5] +
		bl2.z * kernelsLA[2 * 72 + 6 * 9 + 6] + bc2.z * kernelsLA[2 * 72 + 6 * 9 + 7] + br2.z * kernelsLA[2 * 72 + 6 * 9 + 8] +

		tl2.w * kernelsLA[2 * 72 + 7 * 9 + 0] + tc2.w * kernelsLA[2 * 72 + 7 * 9 + 1] + tr2.w * kernelsLA[2 * 72 + 7 * 9 + 2] +
		ml2.w * kernelsLA[2 * 72 + 7 * 9 + 3] + mc2.w * kernelsLA[2 * 72 + 7 * 9 + 4] + mr2.w * kernelsLA[2 * 72 + 7 * 9 + 5] +
		bl2.w * kernelsLA[2 * 72 + 7 * 9 + 6] + bc2.w * kernelsLA[2 * 72 + 7 * 9 + 7] + br2.w * kernelsLA[2 * 72 + 7 * 9 + 8] + biasLA.z
		,
		tl1.x * kernelsLA[3 * 72 + 0 * 9 + 0] + tc1.x * kernelsLA[3 * 72 + 0 * 9 + 1] + tr1.x * kernelsLA[3 * 72 + 0 * 9 + 2] +
		ml1.x * kernelsLA[3 * 72 + 0 * 9 + 3] + mc1.x * kernelsLA[3 * 72 + 0 * 9 + 4] + mr1.x * kernelsLA[3 * 72 + 0 * 9 + 5] +
		bl1.x * kernelsLA[3 * 72 + 0 * 9 + 6] + bc1.x * kernelsLA[3 * 72 + 0 * 9 + 7] + br1.x * kernelsLA[3 * 72 + 0 * 9 + 8] +

		tl1.y * kernelsLA[3 * 72 + 1 * 9 + 0] + tc1.y * kernelsLA[3 * 72 + 1 * 9 + 1] + tr1.y * kernelsLA[3 * 72 + 1 * 9 + 2] +
		ml1.y * kernelsLA[3 * 72 + 1 * 9 + 3] + mc1.y * kernelsLA[3 * 72 + 1 * 9 + 4] + mr1.y * kernelsLA[3 * 72 + 1 * 9 + 5] +
		bl1.y * kernelsLA[3 * 72 + 1 * 9 + 6] + bc1.y * kernelsLA[3 * 72 + 1 * 9 + 7] + br1.y * kernelsLA[3 * 72 + 1 * 9 + 8] +

		tl1.z * kernelsLA[3 * 72 + 2 * 9 + 0] + tc1.z * kernelsLA[3 * 72 + 2 * 9 + 1] + tr1.z * kernelsLA[3 * 72 + 2 * 9 + 2] +
		ml1.z * kernelsLA[3 * 72 + 2 * 9 + 3] + mc1.z * kernelsLA[3 * 72 + 2 * 9 + 4] + mr1.z * kernelsLA[3 * 72 + 2 * 9 + 5] +
		bl1.z * kernelsLA[3 * 72 + 2 * 9 + 6] + bc1.z * kernelsLA[3 * 72 + 2 * 9 + 7] + br1.z * kernelsLA[3 * 72 + 2 * 9 + 8] +

		tl1.w * kernelsLA[3 * 72 + 3 * 9 + 0] + tc1.w * kernelsLA[3 * 72 + 3 * 9 + 1] + tr1.w * kernelsLA[3 * 72 + 3 * 9 + 2] +
		ml1.w * kernelsLA[3 * 72 + 3 * 9 + 3] + mc1.w * kernelsLA[3 * 72 + 3 * 9 + 4] + mr1.w * kernelsLA[3 * 72 + 3 * 9 + 5] +
		bl1.w * kernelsLA[3 * 72 + 3 * 9 + 6] + bc1.w * kernelsLA[3 * 72 + 3 * 9 + 7] + br1.w * kernelsLA[3 * 72 + 3 * 9 + 8] +

		tl2.x * kernelsLA[3 * 72 + 4 * 9 + 0] + tc2.x * kernelsLA[3 * 72 + 4 * 9 + 1] + tr2.x * kernelsLA[3 * 72 + 4 * 9 + 2] +
		ml2.x * kernelsLA[3 * 72 + 4 * 9 + 3] + mc2.x * kernelsLA[3 * 72 + 4 * 9 + 4] + mr2.x * kernelsLA[3 * 72 + 4 * 9 + 5] +
		bl2.x * kernelsLA[3 * 72 + 4 * 9 + 6] + bc2.x * kernelsLA[3 * 72 + 4 * 9 + 7] + br2.x * kernelsLA[3 * 72 + 4 * 9 + 8] +

		tl2.y * kernelsLA[3 * 72 + 5 * 9 + 0] + tc2.y * kernelsLA[3 * 72 + 5 * 9 + 1] + tr2.y * kernelsLA[3 * 72 + 5 * 9 + 2] +
		ml2.y * kernelsLA[3 * 72 + 5 * 9 + 3] + mc2.y * kernelsLA[3 * 72 + 5 * 9 + 4] + mr2.y * kernelsLA[3 * 72 + 5 * 9 + 5] +
		bl2.y * kernelsLA[3 * 72 + 5 * 9 + 6] + bc2.y * kernelsLA[3 * 72 + 5 * 9 + 7] + br2.y * kernelsLA[3 * 72 + 5 * 9 + 8] +

		tl2.z * kernelsLA[3 * 72 + 6 * 9 + 0] + tc2.z * kernelsLA[3 * 72 + 6 * 9 + 1] + tr2.z * kernelsLA[3 * 72 + 6 * 9 + 2] +
		ml2.z * kernelsLA[3 * 72 + 6 * 9 + 3] + mc2.z * kernelsLA[3 * 72 + 6 * 9 + 4] + mr2.z * kernelsLA[3 * 72 + 6 * 9 + 5] +
		bl2.z * kernelsLA[3 * 72 + 6 * 9 + 6] + bc2.z * kernelsLA[3 * 72 + 6 * 9 + 7] + br2.z * kernelsLA[3 * 72 + 6 * 9 + 8] +

		tl2.w * kernelsLA[3 * 72 + 7 * 9 + 0] + tc2.w * kernelsLA[3 * 72 + 7 * 9 + 1] + tr2.w * kernelsLA[3 * 72 + 7 * 9 + 2] +
		ml2.w * kernelsLA[3 * 72 + 7 * 9 + 3] + mc2.w * kernelsLA[3 * 72 + 7 * 9 + 4] + mr2.w * kernelsLA[3 * 72 + 7 * 9 + 5] +
		bl2.w * kernelsLA[3 * 72 + 7 * 9 + 6] + bc2.w * kernelsLA[3 * 72 + 7 * 9 + 7] + br2.w * kernelsLA[3 * 72 + 7 * 9 + 8] + biasLA.w
	));

	float4 target2 = RELU(float4(
		tl1.x * kernelsLB[0 * 72 + 0 * 9 + 0] + tc1.x * kernelsLB[0 * 72 + 0 * 9 + 1] + tr1.x * kernelsLB[0 * 72 + 0 * 9 + 2] +
		ml1.x * kernelsLB[0 * 72 + 0 * 9 + 3] + mc1.x * kernelsLB[0 * 72 + 0 * 9 + 4] + mr1.x * kernelsLB[0 * 72 + 0 * 9 + 5] +
		bl1.x * kernelsLB[0 * 72 + 0 * 9 + 6] + bc1.x * kernelsLB[0 * 72 + 0 * 9 + 7] + br1.x * kernelsLB[0 * 72 + 0 * 9 + 8] +

		tl1.y * kernelsLB[0 * 72 + 1 * 9 + 0] + tc1.y * kernelsLB[0 * 72 + 1 * 9 + 1] + tr1.y * kernelsLB[0 * 72 + 1 * 9 + 2] +
		ml1.y * kernelsLB[0 * 72 + 1 * 9 + 3] + mc1.y * kernelsLB[0 * 72 + 1 * 9 + 4] + mr1.y * kernelsLB[0 * 72 + 1 * 9 + 5] +
		bl1.y * kernelsLB[0 * 72 + 1 * 9 + 6] + bc1.y * kernelsLB[0 * 72 + 1 * 9 + 7] + br1.y * kernelsLB[0 * 72 + 1 * 9 + 8] +

		tl1.z * kernelsLB[0 * 72 + 2 * 9 + 0] + tc1.z * kernelsLB[0 * 72 + 2 * 9 + 1] + tr1.z * kernelsLB[0 * 72 + 2 * 9 + 2] +
		ml1.z * kernelsLB[0 * 72 + 2 * 9 + 3] + mc1.z * kernelsLB[0 * 72 + 2 * 9 + 4] + mr1.z * kernelsLB[0 * 72 + 2 * 9 + 5] +
		bl1.z * kernelsLB[0 * 72 + 2 * 9 + 6] + bc1.z * kernelsLB[0 * 72 + 2 * 9 + 7] + br1.z * kernelsLB[0 * 72 + 2 * 9 + 8] +

		tl1.w * kernelsLB[0 * 72 + 3 * 9 + 0] + tc1.w * kernelsLB[0 * 72 + 3 * 9 + 1] + tr1.w * kernelsLB[0 * 72 + 3 * 9 + 2] +
		ml1.w * kernelsLB[0 * 72 + 3 * 9 + 3] + mc1.w * kernelsLB[0 * 72 + 3 * 9 + 4] + mr1.w * kernelsLB[0 * 72 + 3 * 9 + 5] +
		bl1.w * kernelsLB[0 * 72 + 3 * 9 + 6] + bc1.w * kernelsLB[0 * 72 + 3 * 9 + 7] + br1.w * kernelsLB[0 * 72 + 3 * 9 + 8] +

		tl2.x * kernelsLB[0 * 72 + 4 * 9 + 0] + tc2.x * kernelsLB[0 * 72 + 4 * 9 + 1] + tr2.x * kernelsLB[0 * 72 + 4 * 9 + 2] +
		ml2.x * kernelsLB[0 * 72 + 4 * 9 + 3] + mc2.x * kernelsLB[0 * 72 + 4 * 9 + 4] + mr2.x * kernelsLB[0 * 72 + 4 * 9 + 5] +
		bl2.x * kernelsLB[0 * 72 + 4 * 9 + 6] + bc2.x * kernelsLB[0 * 72 + 4 * 9 + 7] + br2.x * kernelsLB[0 * 72 + 4 * 9 + 8] +

		tl2.y * kernelsLB[0 * 72 + 5 * 9 + 0] + tc2.y * kernelsLB[0 * 72 + 5 * 9 + 1] + tr2.y * kernelsLB[0 * 72 + 5 * 9 + 2] +
		ml2.y * kernelsLB[0 * 72 + 5 * 9 + 3] + mc2.y * kernelsLB[0 * 72 + 5 * 9 + 4] + mr2.y * kernelsLB[0 * 72 + 5 * 9 + 5] +
		bl2.y * kernelsLB[0 * 72 + 5 * 9 + 6] + bc2.y * kernelsLB[0 * 72 + 5 * 9 + 7] + br2.y * kernelsLB[0 * 72 + 5 * 9 + 8] +

		tl2.z * kernelsLB[0 * 72 + 6 * 9 + 0] + tc2.z * kernelsLB[0 * 72 + 6 * 9 + 1] + tr2.z * kernelsLB[0 * 72 + 6 * 9 + 2] +
		ml2.z * kernelsLB[0 * 72 + 6 * 9 + 3] + mc2.z * kernelsLB[0 * 72 + 6 * 9 + 4] + mr2.z * kernelsLB[0 * 72 + 6 * 9 + 5] +
		bl2.z * kernelsLB[0 * 72 + 6 * 9 + 6] + bc2.z * kernelsLB[0 * 72 + 6 * 9 + 7] + br2.z * kernelsLB[0 * 72 + 6 * 9 + 8] +

		tl2.w * kernelsLB[0 * 72 + 7 * 9 + 0] + tc2.w * kernelsLB[0 * 72 + 7 * 9 + 1] + tr2.w * kernelsLB[0 * 72 + 7 * 9 + 2] +
		ml2.w * kernelsLB[0 * 72 + 7 * 9 + 3] + mc2.w * kernelsLB[0 * 72 + 7 * 9 + 4] + mr2.w * kernelsLB[0 * 72 + 7 * 9 + 5] +
		bl2.w * kernelsLB[0 * 72 + 7 * 9 + 6] + bc2.w * kernelsLB[0 * 72 + 7 * 9 + 7] + br2.w * kernelsLB[0 * 72 + 7 * 9 + 8] + biasLB.x
		,
		tl1.x * kernelsLB[1 * 72 + 0 * 9 + 0] + tc1.x * kernelsLB[1 * 72 + 0 * 9 + 1] + tr1.x * kernelsLB[1 * 72 + 0 * 9 + 2] +
		ml1.x * kernelsLB[1 * 72 + 0 * 9 + 3] + mc1.x * kernelsLB[1 * 72 + 0 * 9 + 4] + mr1.x * kernelsLB[1 * 72 + 0 * 9 + 5] +
		bl1.x * kernelsLB[1 * 72 + 0 * 9 + 6] + bc1.x * kernelsLB[1 * 72 + 0 * 9 + 7] + br1.x * kernelsLB[1 * 72 + 0 * 9 + 8] +

		tl1.y * kernelsLB[1 * 72 + 1 * 9 + 0] + tc1.y * kernelsLB[1 * 72 + 1 * 9 + 1] + tr1.y * kernelsLB[1 * 72 + 1 * 9 + 2] +
		ml1.y * kernelsLB[1 * 72 + 1 * 9 + 3] + mc1.y * kernelsLB[1 * 72 + 1 * 9 + 4] + mr1.y * kernelsLB[1 * 72 + 1 * 9 + 5] +
		bl1.y * kernelsLB[1 * 72 + 1 * 9 + 6] + bc1.y * kernelsLB[1 * 72 + 1 * 9 + 7] + br1.y * kernelsLB[1 * 72 + 1 * 9 + 8] +

		tl1.z * kernelsLB[1 * 72 + 2 * 9 + 0] + tc1.z * kernelsLB[1 * 72 + 2 * 9 + 1] + tr1.z * kernelsLB[1 * 72 + 2 * 9 + 2] +
		ml1.z * kernelsLB[1 * 72 + 2 * 9 + 3] + mc1.z * kernelsLB[1 * 72 + 2 * 9 + 4] + mr1.z * kernelsLB[1 * 72 + 2 * 9 + 5] +
		bl1.z * kernelsLB[1 * 72 + 2 * 9 + 6] + bc1.z * kernelsLB[1 * 72 + 2 * 9 + 7] + br1.z * kernelsLB[1 * 72 + 2 * 9 + 8] +

		tl1.w * kernelsLB[1 * 72 + 3 * 9 + 0] + tc1.w * kernelsLB[1 * 72 + 3 * 9 + 1] + tr1.w * kernelsLB[1 * 72 + 3 * 9 + 2] +
		ml1.w * kernelsLB[1 * 72 + 3 * 9 + 3] + mc1.w * kernelsLB[1 * 72 + 3 * 9 + 4] + mr1.w * kernelsLB[1 * 72 + 3 * 9 + 5] +
		bl1.w * kernelsLB[1 * 72 + 3 * 9 + 6] + bc1.w * kernelsLB[1 * 72 + 3 * 9 + 7] + br1.w * kernelsLB[1 * 72 + 3 * 9 + 8] +

		tl2.x * kernelsLB[1 * 72 + 4 * 9 + 0] + tc2.x * kernelsLB[1 * 72 + 4 * 9 + 1] + tr2.x * kernelsLB[1 * 72 + 4 * 9 + 2] +
		ml2.x * kernelsLB[1 * 72 + 4 * 9 + 3] + mc2.x * kernelsLB[1 * 72 + 4 * 9 + 4] + mr2.x * kernelsLB[1 * 72 + 4 * 9 + 5] +
		bl2.x * kernelsLB[1 * 72 + 4 * 9 + 6] + bc2.x * kernelsLB[1 * 72 + 4 * 9 + 7] + br2.x * kernelsLB[1 * 72 + 4 * 9 + 8] +

		tl2.y * kernelsLB[1 * 72 + 5 * 9 + 0] + tc2.y * kernelsLB[1 * 72 + 5 * 9 + 1] + tr2.y * kernelsLB[1 * 72 + 5 * 9 + 2] +
		ml2.y * kernelsLB[1 * 72 + 5 * 9 + 3] + mc2.y * kernelsLB[1 * 72 + 5 * 9 + 4] + mr2.y * kernelsLB[1 * 72 + 5 * 9 + 5] +
		bl2.y * kernelsLB[1 * 72 + 5 * 9 + 6] + bc2.y * kernelsLB[1 * 72 + 5 * 9 + 7] + br2.y * kernelsLB[1 * 72 + 5 * 9 + 8] +

		tl2.z * kernelsLB[1 * 72 + 6 * 9 + 0] + tc2.z * kernelsLB[1 * 72 + 6 * 9 + 1] + tr2.z * kernelsLB[1 * 72 + 6 * 9 + 2] +
		ml2.z * kernelsLB[1 * 72 + 6 * 9 + 3] + mc2.z * kernelsLB[1 * 72 + 6 * 9 + 4] + mr2.z * kernelsLB[1 * 72 + 6 * 9 + 5] +
		bl2.z * kernelsLB[1 * 72 + 6 * 9 + 6] + bc2.z * kernelsLB[1 * 72 + 6 * 9 + 7] + br2.z * kernelsLB[1 * 72 + 6 * 9 + 8] +

		tl2.w * kernelsLB[1 * 72 + 7 * 9 + 0] + tc2.w * kernelsLB[1 * 72 + 7 * 9 + 1] + tr2.w * kernelsLB[1 * 72 + 7 * 9 + 2] +
		ml2.w * kernelsLB[1 * 72 + 7 * 9 + 3] + mc2.w * kernelsLB[1 * 72 + 7 * 9 + 4] + mr2.w * kernelsLB[1 * 72 + 7 * 9 + 5] +
		bl2.w * kernelsLB[1 * 72 + 7 * 9 + 6] + bc2.w * kernelsLB[1 * 72 + 7 * 9 + 7] + br2.w * kernelsLB[1 * 72 + 7 * 9 + 8] + biasLB.y
		,
		tl1.x * kernelsLB[2 * 72 + 0 * 9 + 0] + tc1.x * kernelsLB[2 * 72 + 0 * 9 + 1] + tr1.x * kernelsLB[2 * 72 + 0 * 9 + 2] +
		ml1.x * kernelsLB[2 * 72 + 0 * 9 + 3] + mc1.x * kernelsLB[2 * 72 + 0 * 9 + 4] + mr1.x * kernelsLB[2 * 72 + 0 * 9 + 5] +
		bl1.x * kernelsLB[2 * 72 + 0 * 9 + 6] + bc1.x * kernelsLB[2 * 72 + 0 * 9 + 7] + br1.x * kernelsLB[2 * 72 + 0 * 9 + 8] +

		tl1.y * kernelsLB[2 * 72 + 1 * 9 + 0] + tc1.y * kernelsLB[2 * 72 + 1 * 9 + 1] + tr1.y * kernelsLB[2 * 72 + 1 * 9 + 2] +
		ml1.y * kernelsLB[2 * 72 + 1 * 9 + 3] + mc1.y * kernelsLB[2 * 72 + 1 * 9 + 4] + mr1.y * kernelsLB[2 * 72 + 1 * 9 + 5] +
		bl1.y * kernelsLB[2 * 72 + 1 * 9 + 6] + bc1.y * kernelsLB[2 * 72 + 1 * 9 + 7] + br1.y * kernelsLB[2 * 72 + 1 * 9 + 8] +

		tl1.z * kernelsLB[2 * 72 + 2 * 9 + 0] + tc1.z * kernelsLB[2 * 72 + 2 * 9 + 1] + tr1.z * kernelsLB[2 * 72 + 2 * 9 + 2] +
		ml1.z * kernelsLB[2 * 72 + 2 * 9 + 3] + mc1.z * kernelsLB[2 * 72 + 2 * 9 + 4] + mr1.z * kernelsLB[2 * 72 + 2 * 9 + 5] +
		bl1.z * kernelsLB[2 * 72 + 2 * 9 + 6] + bc1.z * kernelsLB[2 * 72 + 2 * 9 + 7] + br1.z * kernelsLB[2 * 72 + 2 * 9 + 8] +

		tl1.w * kernelsLB[2 * 72 + 3 * 9 + 0] + tc1.w * kernelsLB[2 * 72 + 3 * 9 + 1] + tr1.w * kernelsLB[2 * 72 + 3 * 9 + 2] +
		ml1.w * kernelsLB[2 * 72 + 3 * 9 + 3] + mc1.w * kernelsLB[2 * 72 + 3 * 9 + 4] + mr1.w * kernelsLB[2 * 72 + 3 * 9 + 5] +
		bl1.w * kernelsLB[2 * 72 + 3 * 9 + 6] + bc1.w * kernelsLB[2 * 72 + 3 * 9 + 7] + br1.w * kernelsLB[2 * 72 + 3 * 9 + 8] +

		tl2.x * kernelsLB[2 * 72 + 4 * 9 + 0] + tc2.x * kernelsLB[2 * 72 + 4 * 9 + 1] + tr2.x * kernelsLB[2 * 72 + 4 * 9 + 2] +
		ml2.x * kernelsLB[2 * 72 + 4 * 9 + 3] + mc2.x * kernelsLB[2 * 72 + 4 * 9 + 4] + mr2.x * kernelsLB[2 * 72 + 4 * 9 + 5] +
		bl2.x * kernelsLB[2 * 72 + 4 * 9 + 6] + bc2.x * kernelsLB[2 * 72 + 4 * 9 + 7] + br2.x * kernelsLB[2 * 72 + 4 * 9 + 8] +

		tl2.y * kernelsLB[2 * 72 + 5 * 9 + 0] + tc2.y * kernelsLB[2 * 72 + 5 * 9 + 1] + tr2.y * kernelsLB[2 * 72 + 5 * 9 + 2] +
		ml2.y * kernelsLB[2 * 72 + 5 * 9 + 3] + mc2.y * kernelsLB[2 * 72 + 5 * 9 + 4] + mr2.y * kernelsLB[2 * 72 + 5 * 9 + 5] +
		bl2.y * kernelsLB[2 * 72 + 5 * 9 + 6] + bc2.y * kernelsLB[2 * 72 + 5 * 9 + 7] + br2.y * kernelsLB[2 * 72 + 5 * 9 + 8] +

		tl2.z * kernelsLB[2 * 72 + 6 * 9 + 0] + tc2.z * kernelsLB[2 * 72 + 6 * 9 + 1] + tr2.z * kernelsLB[2 * 72 + 6 * 9 + 2] +
		ml2.z * kernelsLB[2 * 72 + 6 * 9 + 3] + mc2.z * kernelsLB[2 * 72 + 6 * 9 + 4] + mr2.z * kernelsLB[2 * 72 + 6 * 9 + 5] +
		bl2.z * kernelsLB[2 * 72 + 6 * 9 + 6] + bc2.z * kernelsLB[2 * 72 + 6 * 9 + 7] + br2.z * kernelsLB[2 * 72 + 6 * 9 + 8] +

		tl2.w * kernelsLB[2 * 72 + 7 * 9 + 0] + tc2.w * kernelsLB[2 * 72 + 7 * 9 + 1] + tr2.w * kernelsLB[2 * 72 + 7 * 9 + 2] +
		ml2.w * kernelsLB[2 * 72 + 7 * 9 + 3] + mc2.w * kernelsLB[2 * 72 + 7 * 9 + 4] + mr2.w * kernelsLB[2 * 72 + 7 * 9 + 5] +
		bl2.w * kernelsLB[2 * 72 + 7 * 9 + 6] + bc2.w * kernelsLB[2 * 72 + 7 * 9 + 7] + br2.w * kernelsLB[2 * 72 + 7 * 9 + 8] + biasLB.z
		,
		tl1.x * kernelsLB[3 * 72 + 0 * 9 + 0] + tc1.x * kernelsLB[3 * 72 + 0 * 9 + 1] + tr1.x * kernelsLB[3 * 72 + 0 * 9 + 2] +
		ml1.x * kernelsLB[3 * 72 + 0 * 9 + 3] + mc1.x * kernelsLB[3 * 72 + 0 * 9 + 4] + mr1.x * kernelsLB[3 * 72 + 0 * 9 + 5] +
		bl1.x * kernelsLB[3 * 72 + 0 * 9 + 6] + bc1.x * kernelsLB[3 * 72 + 0 * 9 + 7] + br1.x * kernelsLB[3 * 72 + 0 * 9 + 8] +

		tl1.y * kernelsLB[3 * 72 + 1 * 9 + 0] + tc1.y * kernelsLB[3 * 72 + 1 * 9 + 1] + tr1.y * kernelsLB[3 * 72 + 1 * 9 + 2] +
		ml1.y * kernelsLB[3 * 72 + 1 * 9 + 3] + mc1.y * kernelsLB[3 * 72 + 1 * 9 + 4] + mr1.y * kernelsLB[3 * 72 + 1 * 9 + 5] +
		bl1.y * kernelsLB[3 * 72 + 1 * 9 + 6] + bc1.y * kernelsLB[3 * 72 + 1 * 9 + 7] + br1.y * kernelsLB[3 * 72 + 1 * 9 + 8] +

		tl1.z * kernelsLB[3 * 72 + 2 * 9 + 0] + tc1.z * kernelsLB[3 * 72 + 2 * 9 + 1] + tr1.z * kernelsLB[3 * 72 + 2 * 9 + 2] +
		ml1.z * kernelsLB[3 * 72 + 2 * 9 + 3] + mc1.z * kernelsLB[3 * 72 + 2 * 9 + 4] + mr1.z * kernelsLB[3 * 72 + 2 * 9 + 5] +
		bl1.z * kernelsLB[3 * 72 + 2 * 9 + 6] + bc1.z * kernelsLB[3 * 72 + 2 * 9 + 7] + br1.z * kernelsLB[3 * 72 + 2 * 9 + 8] +

		tl1.w * kernelsLB[3 * 72 + 3 * 9 + 0] + tc1.w * kernelsLB[3 * 72 + 3 * 9 + 1] + tr1.w * kernelsLB[3 * 72 + 3 * 9 + 2] +
		ml1.w * kernelsLB[3 * 72 + 3 * 9 + 3] + mc1.w * kernelsLB[3 * 72 + 3 * 9 + 4] + mr1.w * kernelsLB[3 * 72 + 3 * 9 + 5] +
		bl1.w * kernelsLB[3 * 72 + 3 * 9 + 6] + bc1.w * kernelsLB[3 * 72 + 3 * 9 + 7] + br1.w * kernelsLB[3 * 72 + 3 * 9 + 8] +

		tl2.x * kernelsLB[3 * 72 + 4 * 9 + 0] + tc2.x * kernelsLB[3 * 72 + 4 * 9 + 1] + tr2.x * kernelsLB[3 * 72 + 4 * 9 + 2] +
		ml2.x * kernelsLB[3 * 72 + 4 * 9 + 3] + mc2.x * kernelsLB[3 * 72 + 4 * 9 + 4] + mr2.x * kernelsLB[3 * 72 + 4 * 9 + 5] +
		bl2.x * kernelsLB[3 * 72 + 4 * 9 + 6] + bc2.x * kernelsLB[3 * 72 + 4 * 9 + 7] + br2.x * kernelsLB[3 * 72 + 4 * 9 + 8] +

		tl2.y * kernelsLB[3 * 72 + 5 * 9 + 0] + tc2.y * kernelsLB[3 * 72 + 5 * 9 + 1] + tr2.y * kernelsLB[3 * 72 + 5 * 9 + 2] +
		ml2.y * kernelsLB[3 * 72 + 5 * 9 + 3] + mc2.y * kernelsLB[3 * 72 + 5 * 9 + 4] + mr2.y * kernelsLB[3 * 72 + 5 * 9 + 5] +
		bl2.y * kernelsLB[3 * 72 + 5 * 9 + 6] + bc2.y * kernelsLB[3 * 72 + 5 * 9 + 7] + br2.y * kernelsLB[3 * 72 + 5 * 9 + 8] +

		tl2.z * kernelsLB[3 * 72 + 6 * 9 + 0] + tc2.z * kernelsLB[3 * 72 + 6 * 9 + 1] + tr2.z * kernelsLB[3 * 72 + 6 * 9 + 2] +
		ml2.z * kernelsLB[3 * 72 + 6 * 9 + 3] + mc2.z * kernelsLB[3 * 72 + 6 * 9 + 4] + mr2.z * kernelsLB[3 * 72 + 6 * 9 + 5] +
		bl2.z * kernelsLB[3 * 72 + 6 * 9 + 6] + bc2.z * kernelsLB[3 * 72 + 6 * 9 + 7] + br2.z * kernelsLB[3 * 72 + 6 * 9 + 8] +

		tl2.w * kernelsLB[3 * 72 + 7 * 9 + 0] + tc2.w * kernelsLB[3 * 72 + 7 * 9 + 1] + tr2.w * kernelsLB[3 * 72 + 7 * 9 + 2] +
		ml2.w * kernelsLB[3 * 72 + 7 * 9 + 3] + mc2.w * kernelsLB[3 * 72 + 7 * 9 + 4] + mr2.w * kernelsLB[3 * 72 + 7 * 9 + 5] +
		bl2.w * kernelsLB[3 * 72 + 7 * 9 + 6] + bc2.w * kernelsLB[3 * 72 + 7 * 9 + 7] + br2.w * kernelsLB[3 * 72 + 7 * 9 + 8] + biasLB.w
	));

	tex3[gxy] = target1;
	tex4[gxy] = target2;
}


//!PASS 7
//!IN tex3, tex4
//!OUT tex1, tex2
//!BLOCK_SIZE 8
//!NUM_THREADS 64

const static float kernelsLA[9 * 8 * 4] = {
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

const static float4 biasLA = { 8.7612e-02,  5.9126e-01,  4.6709e-03, -1.1559e-39 };

const static float kernelsLB[9 * 8 * 4] = {
	 2.5152e-02, -3.2878e-02,  2.1626e-02,
	 1.9879e-01,  2.9080e-02, -3.0331e-03,
	-2.3380e-01, -2.3578e-02,  1.1871e-01,
	-3.1824e-02, -5.5095e-02,  3.1338e-02,
	-3.2199e-02, -4.3820e-01,  4.1391e-02,
	-4.1207e-02,  3.7475e-01, -1.8548e-01,
	-1.4460e-02, -8.7834e-02, -3.2343e-02,
	 2.4023e-01,  7.1916e-01, -1.8559e-01,
	-6.7635e-03, -9.4409e-02, -1.7890e-02,
	-5.8334e-02,  1.8886e-01,  6.1547e-02,
	-2.6152e-01,  6.6722e-01, -1.2486e-01,
	-4.8128e-02,  1.0510e-01, -4.2619e-02,
	 3.0101e-03,  9.6380e-02,  6.6140e-02,
	 1.0201e-01, -2.3240e-01, -1.8356e-01,
	 4.0019e-02,  2.2985e-01, -1.2980e-01,
	-1.1400e-01, -1.9221e-01, -3.4158e-02,
	 2.2871e-02, -6.8684e-01, -1.0856e-02,
	 2.6311e-02,  2.5422e-02, -1.5190e-02,
	 3.2182e-02, -5.6346e-02,  3.2655e-02,
	-1.6912e-02,  8.4264e-02, -7.9521e-02,
	 1.2788e-03, -7.1110e-02,  8.6585e-02,
	-4.2829e-02,  1.0778e-01, -6.8129e-02,
	 5.8156e-03, -2.3998e-01,  1.9052e-01,
	-4.1855e-02,  1.0140e-01, -1.7139e-02,
	 5.2301e-40, -2.9923e-40,  3.8688e-41,
	 3.1575e-40,  1.1504e-40,  5.5655e-40,
	-3.4499e-40,  2.3050e-40, -6.3766e-41,
	 1.3282e-40,  4.5849e-40,  3.5308e-40,
	-2.6657e-41,  5.9829e-40,  3.2791e-40,
	-2.8348e-40,  2.5810e-40,  5.5791e-40,
	 4.2613e-40,  3.2607e-40, -2.0789e-40,
	-3.9054e-40, -2.5608e-40, -2.7638e-40,
	 4.5027e-40,  2.7065e-40, -4.5593e-40,
	 1.6336e-40, -2.0391e-40, -5.9017e-41,
	-7.9899e-41, -2.9870e-40,  5.6390e-40,
	-2.5560e-41, -1.9786e-40,  9.4700e-41,
	-7.4049e-41, -2.3902e-40, -2.8497e-40,
	-1.8912e-40, -1.5589e-40,  5.5463e-40,
	-2.1782e-40, -1.9532e-40, -2.3785e-40,
	 2.7539e-40,  4.0214e-40,  2.0732e-40,
	 7.0120e-41, -4.4200e-40,  7.3787e-41,
	 2.6452e-40,  1.1970e-40,  2.8298e-40,
	 5.2721e-40,  1.9304e-40, -3.8489e-40,
	-3.9759e-40,  2.6184e-40,  1.2594e-40,
	 1.5831e-40,  3.7179e-40, -3.4915e-40,
	-1.7681e-40, -6.9657e-41, -4.0746e-40,
	 8.0894e-41,  1.6950e-40, -1.0574e-40,
	-1.0590e-40,  2.8466e-41, -2.7558e-40,
	-5.4027e-40,  4.4355e-41, -3.2144e-40,
	-4.8838e-41, -3.8595e-40,  2.5064e-40,
	 4.0365e-40, -1.0195e-40,  4.8356e-40,
	 4.4499e-40, -4.4871e-40, -2.4561e-40,
	 4.1687e-40,  5.2239e-40, -5.7603e-41,
	-1.5211e-40, -3.5768e-40,  3.6385e-40,
	 1.6089e-40,  4.1624e-40,  4.5114e-40,
	 1.6438e-40, -3.6331e-40,  6.4961e-41,
	 5.0899e-40,  6.1036e-40,  2.4828e-40,
	 5.8681e-40, -5.7259e-40, -1.5371e-40,
	 5.2654e-40,  4.7412e-40, -2.0265e-40,
	-4.8621e-41,  4.9497e-40,  3.0176e-40,
	 4.2235e-40,  4.5381e-40,  4.6501e-40,
	-1.6124e-40, -1.9449e-40,  5.1497e-40,
	-1.2891e-40, -1.6549e-40,  4.8348e-40,
	-2.0735e-40,  1.3423e-41, -4.4109e-40,
	-5.4218e-40, -1.1537e-40, -1.1664e-40,
	 5.6006e-40,  3.4109e-40, -3.1434e-40,
	 3.4969e-40, -5.3459e-40,  3.9245e-41,
	 2.4028e-40,  5.7774e-40, -6.2973e-40,
	 1.8802e-40, -4.6258e-41, -5.0716e-40,
	 3.4962e-40, -6.2313e-41, -2.7290e-40,
	-5.2709e-40, -3.2225e-40,  2.4245e-40,
	-3.6300e-40, -2.0794e-40,  4.0541e-40,
	-3.5157e-02,  6.8337e-02,  1.6149e-02,
	-5.8650e-03,  6.0605e-01,  3.1738e-02,
	 9.3306e-02,  2.1499e-01,  1.3609e-01,
	 6.4043e-02, -1.0253e-02, -6.2813e-04,
	 4.6828e-02, -3.9619e-01, -9.2633e-03,
	-8.1752e-02,  9.9083e-02,  4.4296e-03,
	 7.1594e-02,  3.9860e-02,  8.1088e-02,
	 1.7750e-01, -1.2381e-01,  1.4476e-01,
	 2.3416e-02,  1.2819e-01,  1.0816e-02,
	 5.5296e-02,  5.5199e-02, -2.1253e-02,
	 1.7214e-01,  2.0542e-01, -3.7859e-03,
	 1.2831e-01,  3.2087e-02, -5.1851e-02,
	-2.3686e-02,  1.2271e-01, -1.6009e-02,
	-2.0176e-01,  7.4757e-01, -3.4526e-02,
	-4.7055e-02, -3.7099e-01, -1.9216e-01,
	-8.8030e-02, -2.5853e-02, -1.7087e-02,
	-2.0533e-01,  1.5214e-01, -1.8639e-03,
	-1.1236e-01, -2.4612e-01,  6.3094e-02,
	 2.3829e-02, -5.0078e-03,  5.3854e-02,
	-9.6934e-03,  3.7047e-02,  4.7325e-01,
	 5.6975e-03, -8.6108e-02,  6.5569e-02,
	-3.9768e-03,  2.0580e-02, -4.1931e-02,
	 6.9577e-02, -1.0416e-01, -2.5037e-03,
	-1.9198e-02,  6.2027e-02, -1.0833e-02
};

const static float4 biasLB = { 2.3381e-02, -1.2136e-40, -5.6040e-39,  3.7100e-02 };


void Pass7(uint2 blockStart, uint3 threadId) {
	uint2 gxy = Rmp8x8(threadId.x) + blockStart;
	uint2 inputSize = GetInputSize();
	if (gxy.x >= inputSize.x || gxy.y >= inputSize.y) {
		return;
	}

	float2 inputPt = GetInputPt();
	float2 pos = (gxy + 0.5f) * inputPt;

	// [tl, tc, tr]
	// [ml, mc, mr]
	// [bl, bc, br]
	float4 tl1 = tex3.SampleLevel(sam, pos + float2(-inputPt.x, -inputPt.y), 0);
	float4 ml1 = tex3.SampleLevel(sam, pos + float2(-inputPt.x, 0), 0);
	float4 bl1 = tex3.SampleLevel(sam, pos + float2(-inputPt.x, inputPt.y), 0);
	float4 tc1 = tex3.SampleLevel(sam, pos + float2(0, -inputPt.y), 0);
	float4 mc1 = tex3.SampleLevel(sam, pos, 0);
	float4 bc1 = tex3.SampleLevel(sam, pos + float2(0, inputPt.y), 0);
	float4 tr1 = tex3.SampleLevel(sam, pos + float2(inputPt.x, -inputPt.y), 0);
	float4 mr1 = tex3.SampleLevel(sam, pos + float2(inputPt.x, 0), 0);
	float4 br1 = tex3.SampleLevel(sam, pos + float2(inputPt.x, inputPt.y), 0);

	float4 tl2 = tex4.SampleLevel(sam, pos + float2(-inputPt.x, -inputPt.y), 0);
	float4 ml2 = tex4.SampleLevel(sam, pos + float2(-inputPt.x, 0), 0);
	float4 bl2 = tex4.SampleLevel(sam, pos + float2(-inputPt.x, inputPt.y), 0);
	float4 tc2 = tex4.SampleLevel(sam, pos + float2(0, -inputPt.y), 0);
	float4 mc2 = tex4.SampleLevel(sam, pos, 0);
	float4 bc2 = tex4.SampleLevel(sam, pos + float2(0, inputPt.y), 0);
	float4 tr2 = tex4.SampleLevel(sam, pos + float2(inputPt.x, -inputPt.y), 0);
	float4 mr2 = tex4.SampleLevel(sam, pos + float2(inputPt.x, 0), 0);
	float4 br2 = tex4.SampleLevel(sam, pos + float2(inputPt.x, inputPt.y), 0);

	float4 target1 = RELU(float4(
		tl1.x * kernelsLA[0 * 72 + 0 * 9 + 0] + tc1.x * kernelsLA[0 * 72 + 0 * 9 + 1] + tr1.x * kernelsLA[0 * 72 + 0 * 9 + 2] +
		ml1.x * kernelsLA[0 * 72 + 0 * 9 + 3] + mc1.x * kernelsLA[0 * 72 + 0 * 9 + 4] + mr1.x * kernelsLA[0 * 72 + 0 * 9 + 5] +
		bl1.x * kernelsLA[0 * 72 + 0 * 9 + 6] + bc1.x * kernelsLA[0 * 72 + 0 * 9 + 7] + br1.x * kernelsLA[0 * 72 + 0 * 9 + 8] +

		tl1.y * kernelsLA[0 * 72 + 1 * 9 + 0] + tc1.y * kernelsLA[0 * 72 + 1 * 9 + 1] + tr1.y * kernelsLA[0 * 72 + 1 * 9 + 2] +
		ml1.y * kernelsLA[0 * 72 + 1 * 9 + 3] + mc1.y * kernelsLA[0 * 72 + 1 * 9 + 4] + mr1.y * kernelsLA[0 * 72 + 1 * 9 + 5] +
		bl1.y * kernelsLA[0 * 72 + 1 * 9 + 6] + bc1.y * kernelsLA[0 * 72 + 1 * 9 + 7] + br1.y * kernelsLA[0 * 72 + 1 * 9 + 8] +

		tl1.z * kernelsLA[0 * 72 + 2 * 9 + 0] + tc1.z * kernelsLA[0 * 72 + 2 * 9 + 1] + tr1.z * kernelsLA[0 * 72 + 2 * 9 + 2] +
		ml1.z * kernelsLA[0 * 72 + 2 * 9 + 3] + mc1.z * kernelsLA[0 * 72 + 2 * 9 + 4] + mr1.z * kernelsLA[0 * 72 + 2 * 9 + 5] +
		bl1.z * kernelsLA[0 * 72 + 2 * 9 + 6] + bc1.z * kernelsLA[0 * 72 + 2 * 9 + 7] + br1.z * kernelsLA[0 * 72 + 2 * 9 + 8] +

		tl1.w * kernelsLA[0 * 72 + 3 * 9 + 0] + tc1.w * kernelsLA[0 * 72 + 3 * 9 + 1] + tr1.w * kernelsLA[0 * 72 + 3 * 9 + 2] +
		ml1.w * kernelsLA[0 * 72 + 3 * 9 + 3] + mc1.w * kernelsLA[0 * 72 + 3 * 9 + 4] + mr1.w * kernelsLA[0 * 72 + 3 * 9 + 5] +
		bl1.w * kernelsLA[0 * 72 + 3 * 9 + 6] + bc1.w * kernelsLA[0 * 72 + 3 * 9 + 7] + br1.w * kernelsLA[0 * 72 + 3 * 9 + 8] +

		tl2.x * kernelsLA[0 * 72 + 4 * 9 + 0] + tc2.x * kernelsLA[0 * 72 + 4 * 9 + 1] + tr2.x * kernelsLA[0 * 72 + 4 * 9 + 2] +
		ml2.x * kernelsLA[0 * 72 + 4 * 9 + 3] + mc2.x * kernelsLA[0 * 72 + 4 * 9 + 4] + mr2.x * kernelsLA[0 * 72 + 4 * 9 + 5] +
		bl2.x * kernelsLA[0 * 72 + 4 * 9 + 6] + bc2.x * kernelsLA[0 * 72 + 4 * 9 + 7] + br2.x * kernelsLA[0 * 72 + 4 * 9 + 8] +

		tl2.y * kernelsLA[0 * 72 + 5 * 9 + 0] + tc2.y * kernelsLA[0 * 72 + 5 * 9 + 1] + tr2.y * kernelsLA[0 * 72 + 5 * 9 + 2] +
		ml2.y * kernelsLA[0 * 72 + 5 * 9 + 3] + mc2.y * kernelsLA[0 * 72 + 5 * 9 + 4] + mr2.y * kernelsLA[0 * 72 + 5 * 9 + 5] +
		bl2.y * kernelsLA[0 * 72 + 5 * 9 + 6] + bc2.y * kernelsLA[0 * 72 + 5 * 9 + 7] + br2.y * kernelsLA[0 * 72 + 5 * 9 + 8] +

		tl2.z * kernelsLA[0 * 72 + 6 * 9 + 0] + tc2.z * kernelsLA[0 * 72 + 6 * 9 + 1] + tr2.z * kernelsLA[0 * 72 + 6 * 9 + 2] +
		ml2.z * kernelsLA[0 * 72 + 6 * 9 + 3] + mc2.z * kernelsLA[0 * 72 + 6 * 9 + 4] + mr2.z * kernelsLA[0 * 72 + 6 * 9 + 5] +
		bl2.z * kernelsLA[0 * 72 + 6 * 9 + 6] + bc2.z * kernelsLA[0 * 72 + 6 * 9 + 7] + br2.z * kernelsLA[0 * 72 + 6 * 9 + 8] +

		tl2.w * kernelsLA[0 * 72 + 7 * 9 + 0] + tc2.w * kernelsLA[0 * 72 + 7 * 9 + 1] + tr2.w * kernelsLA[0 * 72 + 7 * 9 + 2] +
		ml2.w * kernelsLA[0 * 72 + 7 * 9 + 3] + mc2.w * kernelsLA[0 * 72 + 7 * 9 + 4] + mr2.w * kernelsLA[0 * 72 + 7 * 9 + 5] +
		bl2.w * kernelsLA[0 * 72 + 7 * 9 + 6] + bc2.w * kernelsLA[0 * 72 + 7 * 9 + 7] + br2.w * kernelsLA[0 * 72 + 7 * 9 + 8] + biasLA.x
		,
		tl1.x * kernelsLA[1 * 72 + 0 * 9 + 0] + tc1.x * kernelsLA[1 * 72 + 0 * 9 + 1] + tr1.x * kernelsLA[1 * 72 + 0 * 9 + 2] +
		ml1.x * kernelsLA[1 * 72 + 0 * 9 + 3] + mc1.x * kernelsLA[1 * 72 + 0 * 9 + 4] + mr1.x * kernelsLA[1 * 72 + 0 * 9 + 5] +
		bl1.x * kernelsLA[1 * 72 + 0 * 9 + 6] + bc1.x * kernelsLA[1 * 72 + 0 * 9 + 7] + br1.x * kernelsLA[1 * 72 + 0 * 9 + 8] +

		tl1.y * kernelsLA[1 * 72 + 1 * 9 + 0] + tc1.y * kernelsLA[1 * 72 + 1 * 9 + 1] + tr1.y * kernelsLA[1 * 72 + 1 * 9 + 2] +
		ml1.y * kernelsLA[1 * 72 + 1 * 9 + 3] + mc1.y * kernelsLA[1 * 72 + 1 * 9 + 4] + mr1.y * kernelsLA[1 * 72 + 1 * 9 + 5] +
		bl1.y * kernelsLA[1 * 72 + 1 * 9 + 6] + bc1.y * kernelsLA[1 * 72 + 1 * 9 + 7] + br1.y * kernelsLA[1 * 72 + 1 * 9 + 8] +

		tl1.z * kernelsLA[1 * 72 + 2 * 9 + 0] + tc1.z * kernelsLA[1 * 72 + 2 * 9 + 1] + tr1.z * kernelsLA[1 * 72 + 2 * 9 + 2] +
		ml1.z * kernelsLA[1 * 72 + 2 * 9 + 3] + mc1.z * kernelsLA[1 * 72 + 2 * 9 + 4] + mr1.z * kernelsLA[1 * 72 + 2 * 9 + 5] +
		bl1.z * kernelsLA[1 * 72 + 2 * 9 + 6] + bc1.z * kernelsLA[1 * 72 + 2 * 9 + 7] + br1.z * kernelsLA[1 * 72 + 2 * 9 + 8] +

		tl1.w * kernelsLA[1 * 72 + 3 * 9 + 0] + tc1.w * kernelsLA[1 * 72 + 3 * 9 + 1] + tr1.w * kernelsLA[1 * 72 + 3 * 9 + 2] +
		ml1.w * kernelsLA[1 * 72 + 3 * 9 + 3] + mc1.w * kernelsLA[1 * 72 + 3 * 9 + 4] + mr1.w * kernelsLA[1 * 72 + 3 * 9 + 5] +
		bl1.w * kernelsLA[1 * 72 + 3 * 9 + 6] + bc1.w * kernelsLA[1 * 72 + 3 * 9 + 7] + br1.w * kernelsLA[1 * 72 + 3 * 9 + 8] +

		tl2.x * kernelsLA[1 * 72 + 4 * 9 + 0] + tc2.x * kernelsLA[1 * 72 + 4 * 9 + 1] + tr2.x * kernelsLA[1 * 72 + 4 * 9 + 2] +
		ml2.x * kernelsLA[1 * 72 + 4 * 9 + 3] + mc2.x * kernelsLA[1 * 72 + 4 * 9 + 4] + mr2.x * kernelsLA[1 * 72 + 4 * 9 + 5] +
		bl2.x * kernelsLA[1 * 72 + 4 * 9 + 6] + bc2.x * kernelsLA[1 * 72 + 4 * 9 + 7] + br2.x * kernelsLA[1 * 72 + 4 * 9 + 8] +

		tl2.y * kernelsLA[1 * 72 + 5 * 9 + 0] + tc2.y * kernelsLA[1 * 72 + 5 * 9 + 1] + tr2.y * kernelsLA[1 * 72 + 5 * 9 + 2] +
		ml2.y * kernelsLA[1 * 72 + 5 * 9 + 3] + mc2.y * kernelsLA[1 * 72 + 5 * 9 + 4] + mr2.y * kernelsLA[1 * 72 + 5 * 9 + 5] +
		bl2.y * kernelsLA[1 * 72 + 5 * 9 + 6] + bc2.y * kernelsLA[1 * 72 + 5 * 9 + 7] + br2.y * kernelsLA[1 * 72 + 5 * 9 + 8] +

		tl2.z * kernelsLA[1 * 72 + 6 * 9 + 0] + tc2.z * kernelsLA[1 * 72 + 6 * 9 + 1] + tr2.z * kernelsLA[1 * 72 + 6 * 9 + 2] +
		ml2.z * kernelsLA[1 * 72 + 6 * 9 + 3] + mc2.z * kernelsLA[1 * 72 + 6 * 9 + 4] + mr2.z * kernelsLA[1 * 72 + 6 * 9 + 5] +
		bl2.z * kernelsLA[1 * 72 + 6 * 9 + 6] + bc2.z * kernelsLA[1 * 72 + 6 * 9 + 7] + br2.z * kernelsLA[1 * 72 + 6 * 9 + 8] +

		tl2.w * kernelsLA[1 * 72 + 7 * 9 + 0] + tc2.w * kernelsLA[1 * 72 + 7 * 9 + 1] + tr2.w * kernelsLA[1 * 72 + 7 * 9 + 2] +
		ml2.w * kernelsLA[1 * 72 + 7 * 9 + 3] + mc2.w * kernelsLA[1 * 72 + 7 * 9 + 4] + mr2.w * kernelsLA[1 * 72 + 7 * 9 + 5] +
		bl2.w * kernelsLA[1 * 72 + 7 * 9 + 6] + bc2.w * kernelsLA[1 * 72 + 7 * 9 + 7] + br2.w * kernelsLA[1 * 72 + 7 * 9 + 8] + biasLA.y
		,
		tl1.x * kernelsLA[2 * 72 + 0 * 9 + 0] + tc1.x * kernelsLA[2 * 72 + 0 * 9 + 1] + tr1.x * kernelsLA[2 * 72 + 0 * 9 + 2] +
		ml1.x * kernelsLA[2 * 72 + 0 * 9 + 3] + mc1.x * kernelsLA[2 * 72 + 0 * 9 + 4] + mr1.x * kernelsLA[2 * 72 + 0 * 9 + 5] +
		bl1.x * kernelsLA[2 * 72 + 0 * 9 + 6] + bc1.x * kernelsLA[2 * 72 + 0 * 9 + 7] + br1.x * kernelsLA[2 * 72 + 0 * 9 + 8] +

		tl1.y * kernelsLA[2 * 72 + 1 * 9 + 0] + tc1.y * kernelsLA[2 * 72 + 1 * 9 + 1] + tr1.y * kernelsLA[2 * 72 + 1 * 9 + 2] +
		ml1.y * kernelsLA[2 * 72 + 1 * 9 + 3] + mc1.y * kernelsLA[2 * 72 + 1 * 9 + 4] + mr1.y * kernelsLA[2 * 72 + 1 * 9 + 5] +
		bl1.y * kernelsLA[2 * 72 + 1 * 9 + 6] + bc1.y * kernelsLA[2 * 72 + 1 * 9 + 7] + br1.y * kernelsLA[2 * 72 + 1 * 9 + 8] +

		tl1.z * kernelsLA[2 * 72 + 2 * 9 + 0] + tc1.z * kernelsLA[2 * 72 + 2 * 9 + 1] + tr1.z * kernelsLA[2 * 72 + 2 * 9 + 2] +
		ml1.z * kernelsLA[2 * 72 + 2 * 9 + 3] + mc1.z * kernelsLA[2 * 72 + 2 * 9 + 4] + mr1.z * kernelsLA[2 * 72 + 2 * 9 + 5] +
		bl1.z * kernelsLA[2 * 72 + 2 * 9 + 6] + bc1.z * kernelsLA[2 * 72 + 2 * 9 + 7] + br1.z * kernelsLA[2 * 72 + 2 * 9 + 8] +

		tl1.w * kernelsLA[2 * 72 + 3 * 9 + 0] + tc1.w * kernelsLA[2 * 72 + 3 * 9 + 1] + tr1.w * kernelsLA[2 * 72 + 3 * 9 + 2] +
		ml1.w * kernelsLA[2 * 72 + 3 * 9 + 3] + mc1.w * kernelsLA[2 * 72 + 3 * 9 + 4] + mr1.w * kernelsLA[2 * 72 + 3 * 9 + 5] +
		bl1.w * kernelsLA[2 * 72 + 3 * 9 + 6] + bc1.w * kernelsLA[2 * 72 + 3 * 9 + 7] + br1.w * kernelsLA[2 * 72 + 3 * 9 + 8] +

		tl2.x * kernelsLA[2 * 72 + 4 * 9 + 0] + tc2.x * kernelsLA[2 * 72 + 4 * 9 + 1] + tr2.x * kernelsLA[2 * 72 + 4 * 9 + 2] +
		ml2.x * kernelsLA[2 * 72 + 4 * 9 + 3] + mc2.x * kernelsLA[2 * 72 + 4 * 9 + 4] + mr2.x * kernelsLA[2 * 72 + 4 * 9 + 5] +
		bl2.x * kernelsLA[2 * 72 + 4 * 9 + 6] + bc2.x * kernelsLA[2 * 72 + 4 * 9 + 7] + br2.x * kernelsLA[2 * 72 + 4 * 9 + 8] +

		tl2.y * kernelsLA[2 * 72 + 5 * 9 + 0] + tc2.y * kernelsLA[2 * 72 + 5 * 9 + 1] + tr2.y * kernelsLA[2 * 72 + 5 * 9 + 2] +
		ml2.y * kernelsLA[2 * 72 + 5 * 9 + 3] + mc2.y * kernelsLA[2 * 72 + 5 * 9 + 4] + mr2.y * kernelsLA[2 * 72 + 5 * 9 + 5] +
		bl2.y * kernelsLA[2 * 72 + 5 * 9 + 6] + bc2.y * kernelsLA[2 * 72 + 5 * 9 + 7] + br2.y * kernelsLA[2 * 72 + 5 * 9 + 8] +

		tl2.z * kernelsLA[2 * 72 + 6 * 9 + 0] + tc2.z * kernelsLA[2 * 72 + 6 * 9 + 1] + tr2.z * kernelsLA[2 * 72 + 6 * 9 + 2] +
		ml2.z * kernelsLA[2 * 72 + 6 * 9 + 3] + mc2.z * kernelsLA[2 * 72 + 6 * 9 + 4] + mr2.z * kernelsLA[2 * 72 + 6 * 9 + 5] +
		bl2.z * kernelsLA[2 * 72 + 6 * 9 + 6] + bc2.z * kernelsLA[2 * 72 + 6 * 9 + 7] + br2.z * kernelsLA[2 * 72 + 6 * 9 + 8] +

		tl2.w * kernelsLA[2 * 72 + 7 * 9 + 0] + tc2.w * kernelsLA[2 * 72 + 7 * 9 + 1] + tr2.w * kernelsLA[2 * 72 + 7 * 9 + 2] +
		ml2.w * kernelsLA[2 * 72 + 7 * 9 + 3] + mc2.w * kernelsLA[2 * 72 + 7 * 9 + 4] + mr2.w * kernelsLA[2 * 72 + 7 * 9 + 5] +
		bl2.w * kernelsLA[2 * 72 + 7 * 9 + 6] + bc2.w * kernelsLA[2 * 72 + 7 * 9 + 7] + br2.w * kernelsLA[2 * 72 + 7 * 9 + 8] + biasLA.z
		,
		tl1.x * kernelsLA[3 * 72 + 0 * 9 + 0] + tc1.x * kernelsLA[3 * 72 + 0 * 9 + 1] + tr1.x * kernelsLA[3 * 72 + 0 * 9 + 2] +
		ml1.x * kernelsLA[3 * 72 + 0 * 9 + 3] + mc1.x * kernelsLA[3 * 72 + 0 * 9 + 4] + mr1.x * kernelsLA[3 * 72 + 0 * 9 + 5] +
		bl1.x * kernelsLA[3 * 72 + 0 * 9 + 6] + bc1.x * kernelsLA[3 * 72 + 0 * 9 + 7] + br1.x * kernelsLA[3 * 72 + 0 * 9 + 8] +

		tl1.y * kernelsLA[3 * 72 + 1 * 9 + 0] + tc1.y * kernelsLA[3 * 72 + 1 * 9 + 1] + tr1.y * kernelsLA[3 * 72 + 1 * 9 + 2] +
		ml1.y * kernelsLA[3 * 72 + 1 * 9 + 3] + mc1.y * kernelsLA[3 * 72 + 1 * 9 + 4] + mr1.y * kernelsLA[3 * 72 + 1 * 9 + 5] +
		bl1.y * kernelsLA[3 * 72 + 1 * 9 + 6] + bc1.y * kernelsLA[3 * 72 + 1 * 9 + 7] + br1.y * kernelsLA[3 * 72 + 1 * 9 + 8] +

		tl1.z * kernelsLA[3 * 72 + 2 * 9 + 0] + tc1.z * kernelsLA[3 * 72 + 2 * 9 + 1] + tr1.z * kernelsLA[3 * 72 + 2 * 9 + 2] +
		ml1.z * kernelsLA[3 * 72 + 2 * 9 + 3] + mc1.z * kernelsLA[3 * 72 + 2 * 9 + 4] + mr1.z * kernelsLA[3 * 72 + 2 * 9 + 5] +
		bl1.z * kernelsLA[3 * 72 + 2 * 9 + 6] + bc1.z * kernelsLA[3 * 72 + 2 * 9 + 7] + br1.z * kernelsLA[3 * 72 + 2 * 9 + 8] +

		tl1.w * kernelsLA[3 * 72 + 3 * 9 + 0] + tc1.w * kernelsLA[3 * 72 + 3 * 9 + 1] + tr1.w * kernelsLA[3 * 72 + 3 * 9 + 2] +
		ml1.w * kernelsLA[3 * 72 + 3 * 9 + 3] + mc1.w * kernelsLA[3 * 72 + 3 * 9 + 4] + mr1.w * kernelsLA[3 * 72 + 3 * 9 + 5] +
		bl1.w * kernelsLA[3 * 72 + 3 * 9 + 6] + bc1.w * kernelsLA[3 * 72 + 3 * 9 + 7] + br1.w * kernelsLA[3 * 72 + 3 * 9 + 8] +

		tl2.x * kernelsLA[3 * 72 + 4 * 9 + 0] + tc2.x * kernelsLA[3 * 72 + 4 * 9 + 1] + tr2.x * kernelsLA[3 * 72 + 4 * 9 + 2] +
		ml2.x * kernelsLA[3 * 72 + 4 * 9 + 3] + mc2.x * kernelsLA[3 * 72 + 4 * 9 + 4] + mr2.x * kernelsLA[3 * 72 + 4 * 9 + 5] +
		bl2.x * kernelsLA[3 * 72 + 4 * 9 + 6] + bc2.x * kernelsLA[3 * 72 + 4 * 9 + 7] + br2.x * kernelsLA[3 * 72 + 4 * 9 + 8] +

		tl2.y * kernelsLA[3 * 72 + 5 * 9 + 0] + tc2.y * kernelsLA[3 * 72 + 5 * 9 + 1] + tr2.y * kernelsLA[3 * 72 + 5 * 9 + 2] +
		ml2.y * kernelsLA[3 * 72 + 5 * 9 + 3] + mc2.y * kernelsLA[3 * 72 + 5 * 9 + 4] + mr2.y * kernelsLA[3 * 72 + 5 * 9 + 5] +
		bl2.y * kernelsLA[3 * 72 + 5 * 9 + 6] + bc2.y * kernelsLA[3 * 72 + 5 * 9 + 7] + br2.y * kernelsLA[3 * 72 + 5 * 9 + 8] +

		tl2.z * kernelsLA[3 * 72 + 6 * 9 + 0] + tc2.z * kernelsLA[3 * 72 + 6 * 9 + 1] + tr2.z * kernelsLA[3 * 72 + 6 * 9 + 2] +
		ml2.z * kernelsLA[3 * 72 + 6 * 9 + 3] + mc2.z * kernelsLA[3 * 72 + 6 * 9 + 4] + mr2.z * kernelsLA[3 * 72 + 6 * 9 + 5] +
		bl2.z * kernelsLA[3 * 72 + 6 * 9 + 6] + bc2.z * kernelsLA[3 * 72 + 6 * 9 + 7] + br2.z * kernelsLA[3 * 72 + 6 * 9 + 8] +

		tl2.w * kernelsLA[3 * 72 + 7 * 9 + 0] + tc2.w * kernelsLA[3 * 72 + 7 * 9 + 1] + tr2.w * kernelsLA[3 * 72 + 7 * 9 + 2] +
		ml2.w * kernelsLA[3 * 72 + 7 * 9 + 3] + mc2.w * kernelsLA[3 * 72 + 7 * 9 + 4] + mr2.w * kernelsLA[3 * 72 + 7 * 9 + 5] +
		bl2.w * kernelsLA[3 * 72 + 7 * 9 + 6] + bc2.w * kernelsLA[3 * 72 + 7 * 9 + 7] + br2.w * kernelsLA[3 * 72 + 7 * 9 + 8] + biasLA.w
	));

	float4 target2 = RELU(float4(
		tl1.x * kernelsLB[0 * 72 + 0 * 9 + 0] + tc1.x * kernelsLB[0 * 72 + 0 * 9 + 1] + tr1.x * kernelsLB[0 * 72 + 0 * 9 + 2] +
		ml1.x * kernelsLB[0 * 72 + 0 * 9 + 3] + mc1.x * kernelsLB[0 * 72 + 0 * 9 + 4] + mr1.x * kernelsLB[0 * 72 + 0 * 9 + 5] +
		bl1.x * kernelsLB[0 * 72 + 0 * 9 + 6] + bc1.x * kernelsLB[0 * 72 + 0 * 9 + 7] + br1.x * kernelsLB[0 * 72 + 0 * 9 + 8] +

		tl1.y * kernelsLB[0 * 72 + 1 * 9 + 0] + tc1.y * kernelsLB[0 * 72 + 1 * 9 + 1] + tr1.y * kernelsLB[0 * 72 + 1 * 9 + 2] +
		ml1.y * kernelsLB[0 * 72 + 1 * 9 + 3] + mc1.y * kernelsLB[0 * 72 + 1 * 9 + 4] + mr1.y * kernelsLB[0 * 72 + 1 * 9 + 5] +
		bl1.y * kernelsLB[0 * 72 + 1 * 9 + 6] + bc1.y * kernelsLB[0 * 72 + 1 * 9 + 7] + br1.y * kernelsLB[0 * 72 + 1 * 9 + 8] +

		tl1.z * kernelsLB[0 * 72 + 2 * 9 + 0] + tc1.z * kernelsLB[0 * 72 + 2 * 9 + 1] + tr1.z * kernelsLB[0 * 72 + 2 * 9 + 2] +
		ml1.z * kernelsLB[0 * 72 + 2 * 9 + 3] + mc1.z * kernelsLB[0 * 72 + 2 * 9 + 4] + mr1.z * kernelsLB[0 * 72 + 2 * 9 + 5] +
		bl1.z * kernelsLB[0 * 72 + 2 * 9 + 6] + bc1.z * kernelsLB[0 * 72 + 2 * 9 + 7] + br1.z * kernelsLB[0 * 72 + 2 * 9 + 8] +

		tl1.w * kernelsLB[0 * 72 + 3 * 9 + 0] + tc1.w * kernelsLB[0 * 72 + 3 * 9 + 1] + tr1.w * kernelsLB[0 * 72 + 3 * 9 + 2] +
		ml1.w * kernelsLB[0 * 72 + 3 * 9 + 3] + mc1.w * kernelsLB[0 * 72 + 3 * 9 + 4] + mr1.w * kernelsLB[0 * 72 + 3 * 9 + 5] +
		bl1.w * kernelsLB[0 * 72 + 3 * 9 + 6] + bc1.w * kernelsLB[0 * 72 + 3 * 9 + 7] + br1.w * kernelsLB[0 * 72 + 3 * 9 + 8] +

		tl2.x * kernelsLB[0 * 72 + 4 * 9 + 0] + tc2.x * kernelsLB[0 * 72 + 4 * 9 + 1] + tr2.x * kernelsLB[0 * 72 + 4 * 9 + 2] +
		ml2.x * kernelsLB[0 * 72 + 4 * 9 + 3] + mc2.x * kernelsLB[0 * 72 + 4 * 9 + 4] + mr2.x * kernelsLB[0 * 72 + 4 * 9 + 5] +
		bl2.x * kernelsLB[0 * 72 + 4 * 9 + 6] + bc2.x * kernelsLB[0 * 72 + 4 * 9 + 7] + br2.x * kernelsLB[0 * 72 + 4 * 9 + 8] +

		tl2.y * kernelsLB[0 * 72 + 5 * 9 + 0] + tc2.y * kernelsLB[0 * 72 + 5 * 9 + 1] + tr2.y * kernelsLB[0 * 72 + 5 * 9 + 2] +
		ml2.y * kernelsLB[0 * 72 + 5 * 9 + 3] + mc2.y * kernelsLB[0 * 72 + 5 * 9 + 4] + mr2.y * kernelsLB[0 * 72 + 5 * 9 + 5] +
		bl2.y * kernelsLB[0 * 72 + 5 * 9 + 6] + bc2.y * kernelsLB[0 * 72 + 5 * 9 + 7] + br2.y * kernelsLB[0 * 72 + 5 * 9 + 8] +

		tl2.z * kernelsLB[0 * 72 + 6 * 9 + 0] + tc2.z * kernelsLB[0 * 72 + 6 * 9 + 1] + tr2.z * kernelsLB[0 * 72 + 6 * 9 + 2] +
		ml2.z * kernelsLB[0 * 72 + 6 * 9 + 3] + mc2.z * kernelsLB[0 * 72 + 6 * 9 + 4] + mr2.z * kernelsLB[0 * 72 + 6 * 9 + 5] +
		bl2.z * kernelsLB[0 * 72 + 6 * 9 + 6] + bc2.z * kernelsLB[0 * 72 + 6 * 9 + 7] + br2.z * kernelsLB[0 * 72 + 6 * 9 + 8] +

		tl2.w * kernelsLB[0 * 72 + 7 * 9 + 0] + tc2.w * kernelsLB[0 * 72 + 7 * 9 + 1] + tr2.w * kernelsLB[0 * 72 + 7 * 9 + 2] +
		ml2.w * kernelsLB[0 * 72 + 7 * 9 + 3] + mc2.w * kernelsLB[0 * 72 + 7 * 9 + 4] + mr2.w * kernelsLB[0 * 72 + 7 * 9 + 5] +
		bl2.w * kernelsLB[0 * 72 + 7 * 9 + 6] + bc2.w * kernelsLB[0 * 72 + 7 * 9 + 7] + br2.w * kernelsLB[0 * 72 + 7 * 9 + 8] + biasLB.x
		,
		tl1.x * kernelsLB[1 * 72 + 0 * 9 + 0] + tc1.x * kernelsLB[1 * 72 + 0 * 9 + 1] + tr1.x * kernelsLB[1 * 72 + 0 * 9 + 2] +
		ml1.x * kernelsLB[1 * 72 + 0 * 9 + 3] + mc1.x * kernelsLB[1 * 72 + 0 * 9 + 4] + mr1.x * kernelsLB[1 * 72 + 0 * 9 + 5] +
		bl1.x * kernelsLB[1 * 72 + 0 * 9 + 6] + bc1.x * kernelsLB[1 * 72 + 0 * 9 + 7] + br1.x * kernelsLB[1 * 72 + 0 * 9 + 8] +

		tl1.y * kernelsLB[1 * 72 + 1 * 9 + 0] + tc1.y * kernelsLB[1 * 72 + 1 * 9 + 1] + tr1.y * kernelsLB[1 * 72 + 1 * 9 + 2] +
		ml1.y * kernelsLB[1 * 72 + 1 * 9 + 3] + mc1.y * kernelsLB[1 * 72 + 1 * 9 + 4] + mr1.y * kernelsLB[1 * 72 + 1 * 9 + 5] +
		bl1.y * kernelsLB[1 * 72 + 1 * 9 + 6] + bc1.y * kernelsLB[1 * 72 + 1 * 9 + 7] + br1.y * kernelsLB[1 * 72 + 1 * 9 + 8] +

		tl1.z * kernelsLB[1 * 72 + 2 * 9 + 0] + tc1.z * kernelsLB[1 * 72 + 2 * 9 + 1] + tr1.z * kernelsLB[1 * 72 + 2 * 9 + 2] +
		ml1.z * kernelsLB[1 * 72 + 2 * 9 + 3] + mc1.z * kernelsLB[1 * 72 + 2 * 9 + 4] + mr1.z * kernelsLB[1 * 72 + 2 * 9 + 5] +
		bl1.z * kernelsLB[1 * 72 + 2 * 9 + 6] + bc1.z * kernelsLB[1 * 72 + 2 * 9 + 7] + br1.z * kernelsLB[1 * 72 + 2 * 9 + 8] +

		tl1.w * kernelsLB[1 * 72 + 3 * 9 + 0] + tc1.w * kernelsLB[1 * 72 + 3 * 9 + 1] + tr1.w * kernelsLB[1 * 72 + 3 * 9 + 2] +
		ml1.w * kernelsLB[1 * 72 + 3 * 9 + 3] + mc1.w * kernelsLB[1 * 72 + 3 * 9 + 4] + mr1.w * kernelsLB[1 * 72 + 3 * 9 + 5] +
		bl1.w * kernelsLB[1 * 72 + 3 * 9 + 6] + bc1.w * kernelsLB[1 * 72 + 3 * 9 + 7] + br1.w * kernelsLB[1 * 72 + 3 * 9 + 8] +

		tl2.x * kernelsLB[1 * 72 + 4 * 9 + 0] + tc2.x * kernelsLB[1 * 72 + 4 * 9 + 1] + tr2.x * kernelsLB[1 * 72 + 4 * 9 + 2] +
		ml2.x * kernelsLB[1 * 72 + 4 * 9 + 3] + mc2.x * kernelsLB[1 * 72 + 4 * 9 + 4] + mr2.x * kernelsLB[1 * 72 + 4 * 9 + 5] +
		bl2.x * kernelsLB[1 * 72 + 4 * 9 + 6] + bc2.x * kernelsLB[1 * 72 + 4 * 9 + 7] + br2.x * kernelsLB[1 * 72 + 4 * 9 + 8] +

		tl2.y * kernelsLB[1 * 72 + 5 * 9 + 0] + tc2.y * kernelsLB[1 * 72 + 5 * 9 + 1] + tr2.y * kernelsLB[1 * 72 + 5 * 9 + 2] +
		ml2.y * kernelsLB[1 * 72 + 5 * 9 + 3] + mc2.y * kernelsLB[1 * 72 + 5 * 9 + 4] + mr2.y * kernelsLB[1 * 72 + 5 * 9 + 5] +
		bl2.y * kernelsLB[1 * 72 + 5 * 9 + 6] + bc2.y * kernelsLB[1 * 72 + 5 * 9 + 7] + br2.y * kernelsLB[1 * 72 + 5 * 9 + 8] +

		tl2.z * kernelsLB[1 * 72 + 6 * 9 + 0] + tc2.z * kernelsLB[1 * 72 + 6 * 9 + 1] + tr2.z * kernelsLB[1 * 72 + 6 * 9 + 2] +
		ml2.z * kernelsLB[1 * 72 + 6 * 9 + 3] + mc2.z * kernelsLB[1 * 72 + 6 * 9 + 4] + mr2.z * kernelsLB[1 * 72 + 6 * 9 + 5] +
		bl2.z * kernelsLB[1 * 72 + 6 * 9 + 6] + bc2.z * kernelsLB[1 * 72 + 6 * 9 + 7] + br2.z * kernelsLB[1 * 72 + 6 * 9 + 8] +

		tl2.w * kernelsLB[1 * 72 + 7 * 9 + 0] + tc2.w * kernelsLB[1 * 72 + 7 * 9 + 1] + tr2.w * kernelsLB[1 * 72 + 7 * 9 + 2] +
		ml2.w * kernelsLB[1 * 72 + 7 * 9 + 3] + mc2.w * kernelsLB[1 * 72 + 7 * 9 + 4] + mr2.w * kernelsLB[1 * 72 + 7 * 9 + 5] +
		bl2.w * kernelsLB[1 * 72 + 7 * 9 + 6] + bc2.w * kernelsLB[1 * 72 + 7 * 9 + 7] + br2.w * kernelsLB[1 * 72 + 7 * 9 + 8] + biasLB.y
		,
		tl1.x * kernelsLB[2 * 72 + 0 * 9 + 0] + tc1.x * kernelsLB[2 * 72 + 0 * 9 + 1] + tr1.x * kernelsLB[2 * 72 + 0 * 9 + 2] +
		ml1.x * kernelsLB[2 * 72 + 0 * 9 + 3] + mc1.x * kernelsLB[2 * 72 + 0 * 9 + 4] + mr1.x * kernelsLB[2 * 72 + 0 * 9 + 5] +
		bl1.x * kernelsLB[2 * 72 + 0 * 9 + 6] + bc1.x * kernelsLB[2 * 72 + 0 * 9 + 7] + br1.x * kernelsLB[2 * 72 + 0 * 9 + 8] +

		tl1.y * kernelsLB[2 * 72 + 1 * 9 + 0] + tc1.y * kernelsLB[2 * 72 + 1 * 9 + 1] + tr1.y * kernelsLB[2 * 72 + 1 * 9 + 2] +
		ml1.y * kernelsLB[2 * 72 + 1 * 9 + 3] + mc1.y * kernelsLB[2 * 72 + 1 * 9 + 4] + mr1.y * kernelsLB[2 * 72 + 1 * 9 + 5] +
		bl1.y * kernelsLB[2 * 72 + 1 * 9 + 6] + bc1.y * kernelsLB[2 * 72 + 1 * 9 + 7] + br1.y * kernelsLB[2 * 72 + 1 * 9 + 8] +

		tl1.z * kernelsLB[2 * 72 + 2 * 9 + 0] + tc1.z * kernelsLB[2 * 72 + 2 * 9 + 1] + tr1.z * kernelsLB[2 * 72 + 2 * 9 + 2] +
		ml1.z * kernelsLB[2 * 72 + 2 * 9 + 3] + mc1.z * kernelsLB[2 * 72 + 2 * 9 + 4] + mr1.z * kernelsLB[2 * 72 + 2 * 9 + 5] +
		bl1.z * kernelsLB[2 * 72 + 2 * 9 + 6] + bc1.z * kernelsLB[2 * 72 + 2 * 9 + 7] + br1.z * kernelsLB[2 * 72 + 2 * 9 + 8] +

		tl1.w * kernelsLB[2 * 72 + 3 * 9 + 0] + tc1.w * kernelsLB[2 * 72 + 3 * 9 + 1] + tr1.w * kernelsLB[2 * 72 + 3 * 9 + 2] +
		ml1.w * kernelsLB[2 * 72 + 3 * 9 + 3] + mc1.w * kernelsLB[2 * 72 + 3 * 9 + 4] + mr1.w * kernelsLB[2 * 72 + 3 * 9 + 5] +
		bl1.w * kernelsLB[2 * 72 + 3 * 9 + 6] + bc1.w * kernelsLB[2 * 72 + 3 * 9 + 7] + br1.w * kernelsLB[2 * 72 + 3 * 9 + 8] +

		tl2.x * kernelsLB[2 * 72 + 4 * 9 + 0] + tc2.x * kernelsLB[2 * 72 + 4 * 9 + 1] + tr2.x * kernelsLB[2 * 72 + 4 * 9 + 2] +
		ml2.x * kernelsLB[2 * 72 + 4 * 9 + 3] + mc2.x * kernelsLB[2 * 72 + 4 * 9 + 4] + mr2.x * kernelsLB[2 * 72 + 4 * 9 + 5] +
		bl2.x * kernelsLB[2 * 72 + 4 * 9 + 6] + bc2.x * kernelsLB[2 * 72 + 4 * 9 + 7] + br2.x * kernelsLB[2 * 72 + 4 * 9 + 8] +

		tl2.y * kernelsLB[2 * 72 + 5 * 9 + 0] + tc2.y * kernelsLB[2 * 72 + 5 * 9 + 1] + tr2.y * kernelsLB[2 * 72 + 5 * 9 + 2] +
		ml2.y * kernelsLB[2 * 72 + 5 * 9 + 3] + mc2.y * kernelsLB[2 * 72 + 5 * 9 + 4] + mr2.y * kernelsLB[2 * 72 + 5 * 9 + 5] +
		bl2.y * kernelsLB[2 * 72 + 5 * 9 + 6] + bc2.y * kernelsLB[2 * 72 + 5 * 9 + 7] + br2.y * kernelsLB[2 * 72 + 5 * 9 + 8] +

		tl2.z * kernelsLB[2 * 72 + 6 * 9 + 0] + tc2.z * kernelsLB[2 * 72 + 6 * 9 + 1] + tr2.z * kernelsLB[2 * 72 + 6 * 9 + 2] +
		ml2.z * kernelsLB[2 * 72 + 6 * 9 + 3] + mc2.z * kernelsLB[2 * 72 + 6 * 9 + 4] + mr2.z * kernelsLB[2 * 72 + 6 * 9 + 5] +
		bl2.z * kernelsLB[2 * 72 + 6 * 9 + 6] + bc2.z * kernelsLB[2 * 72 + 6 * 9 + 7] + br2.z * kernelsLB[2 * 72 + 6 * 9 + 8] +

		tl2.w * kernelsLB[2 * 72 + 7 * 9 + 0] + tc2.w * kernelsLB[2 * 72 + 7 * 9 + 1] + tr2.w * kernelsLB[2 * 72 + 7 * 9 + 2] +
		ml2.w * kernelsLB[2 * 72 + 7 * 9 + 3] + mc2.w * kernelsLB[2 * 72 + 7 * 9 + 4] + mr2.w * kernelsLB[2 * 72 + 7 * 9 + 5] +
		bl2.w * kernelsLB[2 * 72 + 7 * 9 + 6] + bc2.w * kernelsLB[2 * 72 + 7 * 9 + 7] + br2.w * kernelsLB[2 * 72 + 7 * 9 + 8] + biasLB.z
		,
		tl1.x * kernelsLB[3 * 72 + 0 * 9 + 0] + tc1.x * kernelsLB[3 * 72 + 0 * 9 + 1] + tr1.x * kernelsLB[3 * 72 + 0 * 9 + 2] +
		ml1.x * kernelsLB[3 * 72 + 0 * 9 + 3] + mc1.x * kernelsLB[3 * 72 + 0 * 9 + 4] + mr1.x * kernelsLB[3 * 72 + 0 * 9 + 5] +
		bl1.x * kernelsLB[3 * 72 + 0 * 9 + 6] + bc1.x * kernelsLB[3 * 72 + 0 * 9 + 7] + br1.x * kernelsLB[3 * 72 + 0 * 9 + 8] +

		tl1.y * kernelsLB[3 * 72 + 1 * 9 + 0] + tc1.y * kernelsLB[3 * 72 + 1 * 9 + 1] + tr1.y * kernelsLB[3 * 72 + 1 * 9 + 2] +
		ml1.y * kernelsLB[3 * 72 + 1 * 9 + 3] + mc1.y * kernelsLB[3 * 72 + 1 * 9 + 4] + mr1.y * kernelsLB[3 * 72 + 1 * 9 + 5] +
		bl1.y * kernelsLB[3 * 72 + 1 * 9 + 6] + bc1.y * kernelsLB[3 * 72 + 1 * 9 + 7] + br1.y * kernelsLB[3 * 72 + 1 * 9 + 8] +

		tl1.z * kernelsLB[3 * 72 + 2 * 9 + 0] + tc1.z * kernelsLB[3 * 72 + 2 * 9 + 1] + tr1.z * kernelsLB[3 * 72 + 2 * 9 + 2] +
		ml1.z * kernelsLB[3 * 72 + 2 * 9 + 3] + mc1.z * kernelsLB[3 * 72 + 2 * 9 + 4] + mr1.z * kernelsLB[3 * 72 + 2 * 9 + 5] +
		bl1.z * kernelsLB[3 * 72 + 2 * 9 + 6] + bc1.z * kernelsLB[3 * 72 + 2 * 9 + 7] + br1.z * kernelsLB[3 * 72 + 2 * 9 + 8] +

		tl1.w * kernelsLB[3 * 72 + 3 * 9 + 0] + tc1.w * kernelsLB[3 * 72 + 3 * 9 + 1] + tr1.w * kernelsLB[3 * 72 + 3 * 9 + 2] +
		ml1.w * kernelsLB[3 * 72 + 3 * 9 + 3] + mc1.w * kernelsLB[3 * 72 + 3 * 9 + 4] + mr1.w * kernelsLB[3 * 72 + 3 * 9 + 5] +
		bl1.w * kernelsLB[3 * 72 + 3 * 9 + 6] + bc1.w * kernelsLB[3 * 72 + 3 * 9 + 7] + br1.w * kernelsLB[3 * 72 + 3 * 9 + 8] +

		tl2.x * kernelsLB[3 * 72 + 4 * 9 + 0] + tc2.x * kernelsLB[3 * 72 + 4 * 9 + 1] + tr2.x * kernelsLB[3 * 72 + 4 * 9 + 2] +
		ml2.x * kernelsLB[3 * 72 + 4 * 9 + 3] + mc2.x * kernelsLB[3 * 72 + 4 * 9 + 4] + mr2.x * kernelsLB[3 * 72 + 4 * 9 + 5] +
		bl2.x * kernelsLB[3 * 72 + 4 * 9 + 6] + bc2.x * kernelsLB[3 * 72 + 4 * 9 + 7] + br2.x * kernelsLB[3 * 72 + 4 * 9 + 8] +

		tl2.y * kernelsLB[3 * 72 + 5 * 9 + 0] + tc2.y * kernelsLB[3 * 72 + 5 * 9 + 1] + tr2.y * kernelsLB[3 * 72 + 5 * 9 + 2] +
		ml2.y * kernelsLB[3 * 72 + 5 * 9 + 3] + mc2.y * kernelsLB[3 * 72 + 5 * 9 + 4] + mr2.y * kernelsLB[3 * 72 + 5 * 9 + 5] +
		bl2.y * kernelsLB[3 * 72 + 5 * 9 + 6] + bc2.y * kernelsLB[3 * 72 + 5 * 9 + 7] + br2.y * kernelsLB[3 * 72 + 5 * 9 + 8] +

		tl2.z * kernelsLB[3 * 72 + 6 * 9 + 0] + tc2.z * kernelsLB[3 * 72 + 6 * 9 + 1] + tr2.z * kernelsLB[3 * 72 + 6 * 9 + 2] +
		ml2.z * kernelsLB[3 * 72 + 6 * 9 + 3] + mc2.z * kernelsLB[3 * 72 + 6 * 9 + 4] + mr2.z * kernelsLB[3 * 72 + 6 * 9 + 5] +
		bl2.z * kernelsLB[3 * 72 + 6 * 9 + 6] + bc2.z * kernelsLB[3 * 72 + 6 * 9 + 7] + br2.z * kernelsLB[3 * 72 + 6 * 9 + 8] +

		tl2.w * kernelsLB[3 * 72 + 7 * 9 + 0] + tc2.w * kernelsLB[3 * 72 + 7 * 9 + 1] + tr2.w * kernelsLB[3 * 72 + 7 * 9 + 2] +
		ml2.w * kernelsLB[3 * 72 + 7 * 9 + 3] + mc2.w * kernelsLB[3 * 72 + 7 * 9 + 4] + mr2.w * kernelsLB[3 * 72 + 7 * 9 + 5] +
		bl2.w * kernelsLB[3 * 72 + 7 * 9 + 6] + bc2.w * kernelsLB[3 * 72 + 7 * 9 + 7] + br2.w * kernelsLB[3 * 72 + 7 * 9 + 8] + biasLB.w
	));

	tex1[gxy] = target1;
	tex2[gxy] = target2;
}


//!PASS 8
//!IN tex1, tex2
//!OUT tex3, tex4
//!BLOCK_SIZE 8
//!NUM_THREADS 64

const static float kernelsLA[9 * 8 * 4] = {
	-5.3430e-40,  2.5717e-41,  5.7504e-40,
	 7.1679e-41,  6.2076e-40, -8.4201e-41,
	-4.2111e-40,  3.4851e-40,  1.3009e-40,
	 3.3016e-40, -7.6473e-41, -1.8392e-40,
	 2.2773e-41,  1.2087e-40,  1.1565e-40,
	 6.5190e-41,  2.0075e-40,  2.5796e-40,
	 5.0575e-40, -2.6261e-40, -2.5486e-40,
	-3.9886e-40, -6.0644e-40,  2.9264e-40,
	 8.9627e-41, -3.0550e-40, -2.3456e-40,
	-4.8855e-40, -4.8867e-40, -5.0492e-40,
	-1.0706e-40,  5.3827e-40, -1.6413e-40,
	 1.4714e-40, -3.4024e-40, -4.4881e-40,
	 3.2361e-40,  2.0858e-40,  3.8836e-40,
	 2.0949e-40,  5.9633e-40, -1.7878e-41,
	-4.1980e-40, -4.4383e-40,  2.7859e-40,
	 7.0317e-42, -8.9973e-41,  5.8700e-41,
	 1.8411e-40, -3.6097e-42,  2.7362e-40,
	 5.4341e-40,  6.0305e-40,  5.9004e-40,
	 5.2692e-40, -6.3449e-41,  1.2075e-40,
	 7.5297e-41,  8.9267e-41,  4.9139e-40,
	-1.4609e-40,  3.1821e-41,  2.3288e-40,
	 3.1748e-41, -3.8052e-40, -2.4322e-40,
	-5.7959e-40,  6.1966e-40,  3.4964e-40,
	-5.6776e-40, -6.8327e-41, -3.3777e-41,
	-5.9108e-02,  3.5468e-02, -2.8772e-02,
	 6.8602e-01,  1.4232e-01,  1.1954e-02,
	-3.8234e-02,  7.1837e-02, -1.8832e-02,
	 4.7972e-02,  1.1623e-02, -2.1687e-03,
	-4.9744e-01,  2.7751e-01,  1.7862e-02,
	 7.4286e-02,  3.1309e-03,  1.1030e-03,
	-6.1084e-01, -8.5679e-03,  9.4956e-03,
	-4.5246e-01, -1.2126e-01, -3.7368e-02,
	 2.5624e-02,  1.2087e-02, -1.5431e-02,
	 6.0313e-40,  1.8404e-40, -7.2006e-41,
	 6.0697e-40, -9.1199e-41,  5.8965e-40,
	 5.4830e-40,  1.3014e-40,  1.5585e-41,
	-3.6027e-02, -6.3004e-03,  1.5237e-02,
	 6.0743e-01,  9.2523e-02, -4.7370e-03,
	 3.4407e-02, -8.3823e-02,  1.6898e-02,
	 5.7527e-40, -5.0621e-40, -2.9035e-42,
	 3.8199e-40, -2.2913e-40, -5.0895e-40,
	 4.0079e-40,  5.1744e-40, -3.3006e-40,
	 6.1448e-40,  1.2347e-40, -3.1673e-40,
	 7.3214e-41,  5.2143e-40, -2.6071e-40,
	 1.6109e-40, -2.0298e-40,  9.5817e-41,
	 6.9876e-02, -2.9290e-02,  3.2294e-03,
	-4.2632e-01,  1.5789e-01,  3.6809e-02,
	 2.1220e-02,  1.6531e-04,  6.8502e-03,
	-6.5221e-02,  8.8059e-02,  5.7934e-03,
	-1.7280e-01,  1.5303e-01,  1.7663e-01,
	-1.2908e-01, -1.1749e-01,  5.7887e-02,
	 1.0685e-01,  2.2763e-01,  3.3796e-02,
	 1.7629e-01,  3.8882e-01,  6.3540e-02,
	 6.4707e-02,  1.0046e-01, -8.1911e-02,
	-3.9718e-03,  4.6416e-02,  4.7357e-02,
	 7.3694e-02, -1.6444e-01,  2.4784e-02,
	-3.0808e-03,  2.7399e-02, -2.9216e-04,
	 2.4428e-40, -3.0160e-40,  2.3184e-40,
	-4.9114e-40,  5.6685e-40, -3.6020e-40,
	 2.2618e-40, -2.8145e-40,  2.1149e-40,
	 2.3559e-02, -8.6949e-02, -3.8350e-02,
	-2.9547e-01,  7.0187e-01, -8.3979e-02,
	-2.8576e-02, -1.6538e-01, -5.2465e-02,
	-1.6016e-40, -1.4760e-40, -2.1977e-40,
	 4.3180e-40,  4.1724e-40, -1.2969e-40,
	-1.3023e-40, -1.0095e-40, -1.5965e-40,
	-4.0721e-40, -4.1747e-40, -4.3706e-40,
	-4.2838e-40, -4.5507e-40, -4.6023e-40,
	-3.7435e-40, -3.9889e-40, -4.2249e-40,
	-1.2429e-01, -3.5062e-01, -1.1418e-01,
	-4.0787e-02,  6.1690e-01, -1.0085e-01,
	 1.6098e-02,  8.5100e-02, -1.1621e-02,
	 3.0709e-40, -4.4880e-40, -2.7530e-41,
	-1.2649e-40, -5.3936e-40,  5.0995e-41,
	 4.4003e-40, -2.1211e-40, -6.6422e-43,
	-1.8989e-40, -3.6631e-40,  4.1392e-40,
	-3.9057e-40, -5.5599e-40,  6.9979e-41,
	 3.8983e-40,  5.6737e-41,  2.3997e-40,
	-9.4862e-41,  2.4256e-40, -3.7040e-40,
	 1.6374e-40,  3.5439e-42, -1.0385e-40,
	 3.6145e-40, -2.4342e-41, -3.0115e-40,
	-6.0009e-40, -5.2386e-41, -1.2504e-40,
	 2.9237e-40, -1.2290e-40, -1.1502e-40,
	-3.5887e-40, -6.1810e-40, -1.6289e-41,
	 2.5438e-41,  5.1229e-40, -2.4915e-40,
	 1.3516e-40,  3.3553e-40,  8.5831e-41,
	-8.5122e-41,  3.7625e-41,  2.5507e-40,
	-1.5828e-40,  2.1991e-40, -1.5628e-40,
	-5.3110e-40,  5.1395e-40, -5.8162e-40,
	-3.1571e-40, -5.5139e-40,  1.2299e-40,
	 4.8855e-40, -9.3940e-41, -6.2534e-40,
	-3.3275e-40, -2.4982e-40, -1.2956e-40,
	-6.0047e-40, -1.8712e-41, -7.3274e-42,
	-2.8519e-40,  3.5541e-40,  2.4485e-40,
	-8.1435e-41, -2.7091e-40,  7.1206e-41,
	-5.9519e-41, -2.5552e-40, -3.6189e-40
};

const static float4 biasLA = { -3.3246e-39, -1.4536e-02, -6.3362e-02,  8.5347e-41 };

const static float kernelsLB[9 * 8 * 4] = {
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

const static float4 biasLB = { 7.9956e-02, 3.0679e-04, -1.0257e-02, -1.2037e-02 };


void Pass8(uint2 blockStart, uint3 threadId) {
	uint2 gxy = Rmp8x8(threadId.x) + blockStart;
	uint2 inputSize = GetInputSize();
	if (gxy.x >= inputSize.x || gxy.y >= inputSize.y) {
		return;
	}

	float2 inputPt = GetInputPt();
	float2 pos = (gxy + 0.5f) * inputPt;

	// [tl, tc, tr]
	// [ml, mc, mr]
	// [bl, bc, br]
	float4 tl1 = tex1.SampleLevel(sam, pos + float2(-inputPt.x, -inputPt.y), 0);
	float4 ml1 = tex1.SampleLevel(sam, pos + float2(-inputPt.x, 0), 0);
	float4 bl1 = tex1.SampleLevel(sam, pos + float2(-inputPt.x, inputPt.y), 0);
	float4 tc1 = tex1.SampleLevel(sam, pos + float2(0, -inputPt.y), 0);
	float4 mc1 = tex1.SampleLevel(sam, pos, 0);
	float4 bc1 = tex1.SampleLevel(sam, pos + float2(0, inputPt.y), 0);
	float4 tr1 = tex1.SampleLevel(sam, pos + float2(inputPt.x, -inputPt.y), 0);
	float4 mr1 = tex1.SampleLevel(sam, pos + float2(inputPt.x, 0), 0);
	float4 br1 = tex1.SampleLevel(sam, pos + float2(inputPt.x, inputPt.y), 0);

	float4 tl2 = tex2.SampleLevel(sam, pos + float2(-inputPt.x, -inputPt.y), 0);
	float4 ml2 = tex2.SampleLevel(sam, pos + float2(-inputPt.x, 0), 0);
	float4 bl2 = tex2.SampleLevel(sam, pos + float2(-inputPt.x, inputPt.y), 0);
	float4 tc2 = tex2.SampleLevel(sam, pos + float2(0, -inputPt.y), 0);
	float4 mc2 = tex2.SampleLevel(sam, pos, 0);
	float4 bc2 = tex2.SampleLevel(sam, pos + float2(0, inputPt.y), 0);
	float4 tr2 = tex2.SampleLevel(sam, pos + float2(inputPt.x, -inputPt.y), 0);
	float4 mr2 = tex2.SampleLevel(sam, pos + float2(inputPt.x, 0), 0);
	float4 br2 = tex2.SampleLevel(sam, pos + float2(inputPt.x, inputPt.y), 0);

	float4 target1 = RELU(float4(
		tl1.x * kernelsLA[0 * 72 + 0 * 9 + 0] + tc1.x * kernelsLA[0 * 72 + 0 * 9 + 1] + tr1.x * kernelsLA[0 * 72 + 0 * 9 + 2] +
		ml1.x * kernelsLA[0 * 72 + 0 * 9 + 3] + mc1.x * kernelsLA[0 * 72 + 0 * 9 + 4] + mr1.x * kernelsLA[0 * 72 + 0 * 9 + 5] +
		bl1.x * kernelsLA[0 * 72 + 0 * 9 + 6] + bc1.x * kernelsLA[0 * 72 + 0 * 9 + 7] + br1.x * kernelsLA[0 * 72 + 0 * 9 + 8] +

		tl1.y * kernelsLA[0 * 72 + 1 * 9 + 0] + tc1.y * kernelsLA[0 * 72 + 1 * 9 + 1] + tr1.y * kernelsLA[0 * 72 + 1 * 9 + 2] +
		ml1.y * kernelsLA[0 * 72 + 1 * 9 + 3] + mc1.y * kernelsLA[0 * 72 + 1 * 9 + 4] + mr1.y * kernelsLA[0 * 72 + 1 * 9 + 5] +
		bl1.y * kernelsLA[0 * 72 + 1 * 9 + 6] + bc1.y * kernelsLA[0 * 72 + 1 * 9 + 7] + br1.y * kernelsLA[0 * 72 + 1 * 9 + 8] +

		tl1.z * kernelsLA[0 * 72 + 2 * 9 + 0] + tc1.z * kernelsLA[0 * 72 + 2 * 9 + 1] + tr1.z * kernelsLA[0 * 72 + 2 * 9 + 2] +
		ml1.z * kernelsLA[0 * 72 + 2 * 9 + 3] + mc1.z * kernelsLA[0 * 72 + 2 * 9 + 4] + mr1.z * kernelsLA[0 * 72 + 2 * 9 + 5] +
		bl1.z * kernelsLA[0 * 72 + 2 * 9 + 6] + bc1.z * kernelsLA[0 * 72 + 2 * 9 + 7] + br1.z * kernelsLA[0 * 72 + 2 * 9 + 8] +

		tl1.w * kernelsLA[0 * 72 + 3 * 9 + 0] + tc1.w * kernelsLA[0 * 72 + 3 * 9 + 1] + tr1.w * kernelsLA[0 * 72 + 3 * 9 + 2] +
		ml1.w * kernelsLA[0 * 72 + 3 * 9 + 3] + mc1.w * kernelsLA[0 * 72 + 3 * 9 + 4] + mr1.w * kernelsLA[0 * 72 + 3 * 9 + 5] +
		bl1.w * kernelsLA[0 * 72 + 3 * 9 + 6] + bc1.w * kernelsLA[0 * 72 + 3 * 9 + 7] + br1.w * kernelsLA[0 * 72 + 3 * 9 + 8] +

		tl2.x * kernelsLA[0 * 72 + 4 * 9 + 0] + tc2.x * kernelsLA[0 * 72 + 4 * 9 + 1] + tr2.x * kernelsLA[0 * 72 + 4 * 9 + 2] +
		ml2.x * kernelsLA[0 * 72 + 4 * 9 + 3] + mc2.x * kernelsLA[0 * 72 + 4 * 9 + 4] + mr2.x * kernelsLA[0 * 72 + 4 * 9 + 5] +
		bl2.x * kernelsLA[0 * 72 + 4 * 9 + 6] + bc2.x * kernelsLA[0 * 72 + 4 * 9 + 7] + br2.x * kernelsLA[0 * 72 + 4 * 9 + 8] +

		tl2.y * kernelsLA[0 * 72 + 5 * 9 + 0] + tc2.y * kernelsLA[0 * 72 + 5 * 9 + 1] + tr2.y * kernelsLA[0 * 72 + 5 * 9 + 2] +
		ml2.y * kernelsLA[0 * 72 + 5 * 9 + 3] + mc2.y * kernelsLA[0 * 72 + 5 * 9 + 4] + mr2.y * kernelsLA[0 * 72 + 5 * 9 + 5] +
		bl2.y * kernelsLA[0 * 72 + 5 * 9 + 6] + bc2.y * kernelsLA[0 * 72 + 5 * 9 + 7] + br2.y * kernelsLA[0 * 72 + 5 * 9 + 8] +

		tl2.z * kernelsLA[0 * 72 + 6 * 9 + 0] + tc2.z * kernelsLA[0 * 72 + 6 * 9 + 1] + tr2.z * kernelsLA[0 * 72 + 6 * 9 + 2] +
		ml2.z * kernelsLA[0 * 72 + 6 * 9 + 3] + mc2.z * kernelsLA[0 * 72 + 6 * 9 + 4] + mr2.z * kernelsLA[0 * 72 + 6 * 9 + 5] +
		bl2.z * kernelsLA[0 * 72 + 6 * 9 + 6] + bc2.z * kernelsLA[0 * 72 + 6 * 9 + 7] + br2.z * kernelsLA[0 * 72 + 6 * 9 + 8] +

		tl2.w * kernelsLA[0 * 72 + 7 * 9 + 0] + tc2.w * kernelsLA[0 * 72 + 7 * 9 + 1] + tr2.w * kernelsLA[0 * 72 + 7 * 9 + 2] +
		ml2.w * kernelsLA[0 * 72 + 7 * 9 + 3] + mc2.w * kernelsLA[0 * 72 + 7 * 9 + 4] + mr2.w * kernelsLA[0 * 72 + 7 * 9 + 5] +
		bl2.w * kernelsLA[0 * 72 + 7 * 9 + 6] + bc2.w * kernelsLA[0 * 72 + 7 * 9 + 7] + br2.w * kernelsLA[0 * 72 + 7 * 9 + 8] + biasLA.x
		,
		tl1.x * kernelsLA[1 * 72 + 0 * 9 + 0] + tc1.x * kernelsLA[1 * 72 + 0 * 9 + 1] + tr1.x * kernelsLA[1 * 72 + 0 * 9 + 2] +
		ml1.x * kernelsLA[1 * 72 + 0 * 9 + 3] + mc1.x * kernelsLA[1 * 72 + 0 * 9 + 4] + mr1.x * kernelsLA[1 * 72 + 0 * 9 + 5] +
		bl1.x * kernelsLA[1 * 72 + 0 * 9 + 6] + bc1.x * kernelsLA[1 * 72 + 0 * 9 + 7] + br1.x * kernelsLA[1 * 72 + 0 * 9 + 8] +

		tl1.y * kernelsLA[1 * 72 + 1 * 9 + 0] + tc1.y * kernelsLA[1 * 72 + 1 * 9 + 1] + tr1.y * kernelsLA[1 * 72 + 1 * 9 + 2] +
		ml1.y * kernelsLA[1 * 72 + 1 * 9 + 3] + mc1.y * kernelsLA[1 * 72 + 1 * 9 + 4] + mr1.y * kernelsLA[1 * 72 + 1 * 9 + 5] +
		bl1.y * kernelsLA[1 * 72 + 1 * 9 + 6] + bc1.y * kernelsLA[1 * 72 + 1 * 9 + 7] + br1.y * kernelsLA[1 * 72 + 1 * 9 + 8] +

		tl1.z * kernelsLA[1 * 72 + 2 * 9 + 0] + tc1.z * kernelsLA[1 * 72 + 2 * 9 + 1] + tr1.z * kernelsLA[1 * 72 + 2 * 9 + 2] +
		ml1.z * kernelsLA[1 * 72 + 2 * 9 + 3] + mc1.z * kernelsLA[1 * 72 + 2 * 9 + 4] + mr1.z * kernelsLA[1 * 72 + 2 * 9 + 5] +
		bl1.z * kernelsLA[1 * 72 + 2 * 9 + 6] + bc1.z * kernelsLA[1 * 72 + 2 * 9 + 7] + br1.z * kernelsLA[1 * 72 + 2 * 9 + 8] +

		tl1.w * kernelsLA[1 * 72 + 3 * 9 + 0] + tc1.w * kernelsLA[1 * 72 + 3 * 9 + 1] + tr1.w * kernelsLA[1 * 72 + 3 * 9 + 2] +
		ml1.w * kernelsLA[1 * 72 + 3 * 9 + 3] + mc1.w * kernelsLA[1 * 72 + 3 * 9 + 4] + mr1.w * kernelsLA[1 * 72 + 3 * 9 + 5] +
		bl1.w * kernelsLA[1 * 72 + 3 * 9 + 6] + bc1.w * kernelsLA[1 * 72 + 3 * 9 + 7] + br1.w * kernelsLA[1 * 72 + 3 * 9 + 8] +

		tl2.x * kernelsLA[1 * 72 + 4 * 9 + 0] + tc2.x * kernelsLA[1 * 72 + 4 * 9 + 1] + tr2.x * kernelsLA[1 * 72 + 4 * 9 + 2] +
		ml2.x * kernelsLA[1 * 72 + 4 * 9 + 3] + mc2.x * kernelsLA[1 * 72 + 4 * 9 + 4] + mr2.x * kernelsLA[1 * 72 + 4 * 9 + 5] +
		bl2.x * kernelsLA[1 * 72 + 4 * 9 + 6] + bc2.x * kernelsLA[1 * 72 + 4 * 9 + 7] + br2.x * kernelsLA[1 * 72 + 4 * 9 + 8] +

		tl2.y * kernelsLA[1 * 72 + 5 * 9 + 0] + tc2.y * kernelsLA[1 * 72 + 5 * 9 + 1] + tr2.y * kernelsLA[1 * 72 + 5 * 9 + 2] +
		ml2.y * kernelsLA[1 * 72 + 5 * 9 + 3] + mc2.y * kernelsLA[1 * 72 + 5 * 9 + 4] + mr2.y * kernelsLA[1 * 72 + 5 * 9 + 5] +
		bl2.y * kernelsLA[1 * 72 + 5 * 9 + 6] + bc2.y * kernelsLA[1 * 72 + 5 * 9 + 7] + br2.y * kernelsLA[1 * 72 + 5 * 9 + 8] +

		tl2.z * kernelsLA[1 * 72 + 6 * 9 + 0] + tc2.z * kernelsLA[1 * 72 + 6 * 9 + 1] + tr2.z * kernelsLA[1 * 72 + 6 * 9 + 2] +
		ml2.z * kernelsLA[1 * 72 + 6 * 9 + 3] + mc2.z * kernelsLA[1 * 72 + 6 * 9 + 4] + mr2.z * kernelsLA[1 * 72 + 6 * 9 + 5] +
		bl2.z * kernelsLA[1 * 72 + 6 * 9 + 6] + bc2.z * kernelsLA[1 * 72 + 6 * 9 + 7] + br2.z * kernelsLA[1 * 72 + 6 * 9 + 8] +

		tl2.w * kernelsLA[1 * 72 + 7 * 9 + 0] + tc2.w * kernelsLA[1 * 72 + 7 * 9 + 1] + tr2.w * kernelsLA[1 * 72 + 7 * 9 + 2] +
		ml2.w * kernelsLA[1 * 72 + 7 * 9 + 3] + mc2.w * kernelsLA[1 * 72 + 7 * 9 + 4] + mr2.w * kernelsLA[1 * 72 + 7 * 9 + 5] +
		bl2.w * kernelsLA[1 * 72 + 7 * 9 + 6] + bc2.w * kernelsLA[1 * 72 + 7 * 9 + 7] + br2.w * kernelsLA[1 * 72 + 7 * 9 + 8] + biasLA.y
		,
		tl1.x * kernelsLA[2 * 72 + 0 * 9 + 0] + tc1.x * kernelsLA[2 * 72 + 0 * 9 + 1] + tr1.x * kernelsLA[2 * 72 + 0 * 9 + 2] +
		ml1.x * kernelsLA[2 * 72 + 0 * 9 + 3] + mc1.x * kernelsLA[2 * 72 + 0 * 9 + 4] + mr1.x * kernelsLA[2 * 72 + 0 * 9 + 5] +
		bl1.x * kernelsLA[2 * 72 + 0 * 9 + 6] + bc1.x * kernelsLA[2 * 72 + 0 * 9 + 7] + br1.x * kernelsLA[2 * 72 + 0 * 9 + 8] +

		tl1.y * kernelsLA[2 * 72 + 1 * 9 + 0] + tc1.y * kernelsLA[2 * 72 + 1 * 9 + 1] + tr1.y * kernelsLA[2 * 72 + 1 * 9 + 2] +
		ml1.y * kernelsLA[2 * 72 + 1 * 9 + 3] + mc1.y * kernelsLA[2 * 72 + 1 * 9 + 4] + mr1.y * kernelsLA[2 * 72 + 1 * 9 + 5] +
		bl1.y * kernelsLA[2 * 72 + 1 * 9 + 6] + bc1.y * kernelsLA[2 * 72 + 1 * 9 + 7] + br1.y * kernelsLA[2 * 72 + 1 * 9 + 8] +

		tl1.z * kernelsLA[2 * 72 + 2 * 9 + 0] + tc1.z * kernelsLA[2 * 72 + 2 * 9 + 1] + tr1.z * kernelsLA[2 * 72 + 2 * 9 + 2] +
		ml1.z * kernelsLA[2 * 72 + 2 * 9 + 3] + mc1.z * kernelsLA[2 * 72 + 2 * 9 + 4] + mr1.z * kernelsLA[2 * 72 + 2 * 9 + 5] +
		bl1.z * kernelsLA[2 * 72 + 2 * 9 + 6] + bc1.z * kernelsLA[2 * 72 + 2 * 9 + 7] + br1.z * kernelsLA[2 * 72 + 2 * 9 + 8] +

		tl1.w * kernelsLA[2 * 72 + 3 * 9 + 0] + tc1.w * kernelsLA[2 * 72 + 3 * 9 + 1] + tr1.w * kernelsLA[2 * 72 + 3 * 9 + 2] +
		ml1.w * kernelsLA[2 * 72 + 3 * 9 + 3] + mc1.w * kernelsLA[2 * 72 + 3 * 9 + 4] + mr1.w * kernelsLA[2 * 72 + 3 * 9 + 5] +
		bl1.w * kernelsLA[2 * 72 + 3 * 9 + 6] + bc1.w * kernelsLA[2 * 72 + 3 * 9 + 7] + br1.w * kernelsLA[2 * 72 + 3 * 9 + 8] +

		tl2.x * kernelsLA[2 * 72 + 4 * 9 + 0] + tc2.x * kernelsLA[2 * 72 + 4 * 9 + 1] + tr2.x * kernelsLA[2 * 72 + 4 * 9 + 2] +
		ml2.x * kernelsLA[2 * 72 + 4 * 9 + 3] + mc2.x * kernelsLA[2 * 72 + 4 * 9 + 4] + mr2.x * kernelsLA[2 * 72 + 4 * 9 + 5] +
		bl2.x * kernelsLA[2 * 72 + 4 * 9 + 6] + bc2.x * kernelsLA[2 * 72 + 4 * 9 + 7] + br2.x * kernelsLA[2 * 72 + 4 * 9 + 8] +

		tl2.y * kernelsLA[2 * 72 + 5 * 9 + 0] + tc2.y * kernelsLA[2 * 72 + 5 * 9 + 1] + tr2.y * kernelsLA[2 * 72 + 5 * 9 + 2] +
		ml2.y * kernelsLA[2 * 72 + 5 * 9 + 3] + mc2.y * kernelsLA[2 * 72 + 5 * 9 + 4] + mr2.y * kernelsLA[2 * 72 + 5 * 9 + 5] +
		bl2.y * kernelsLA[2 * 72 + 5 * 9 + 6] + bc2.y * kernelsLA[2 * 72 + 5 * 9 + 7] + br2.y * kernelsLA[2 * 72 + 5 * 9 + 8] +

		tl2.z * kernelsLA[2 * 72 + 6 * 9 + 0] + tc2.z * kernelsLA[2 * 72 + 6 * 9 + 1] + tr2.z * kernelsLA[2 * 72 + 6 * 9 + 2] +
		ml2.z * kernelsLA[2 * 72 + 6 * 9 + 3] + mc2.z * kernelsLA[2 * 72 + 6 * 9 + 4] + mr2.z * kernelsLA[2 * 72 + 6 * 9 + 5] +
		bl2.z * kernelsLA[2 * 72 + 6 * 9 + 6] + bc2.z * kernelsLA[2 * 72 + 6 * 9 + 7] + br2.z * kernelsLA[2 * 72 + 6 * 9 + 8] +

		tl2.w * kernelsLA[2 * 72 + 7 * 9 + 0] + tc2.w * kernelsLA[2 * 72 + 7 * 9 + 1] + tr2.w * kernelsLA[2 * 72 + 7 * 9 + 2] +
		ml2.w * kernelsLA[2 * 72 + 7 * 9 + 3] + mc2.w * kernelsLA[2 * 72 + 7 * 9 + 4] + mr2.w * kernelsLA[2 * 72 + 7 * 9 + 5] +
		bl2.w * kernelsLA[2 * 72 + 7 * 9 + 6] + bc2.w * kernelsLA[2 * 72 + 7 * 9 + 7] + br2.w * kernelsLA[2 * 72 + 7 * 9 + 8] + biasLA.z
		,
		tl1.x * kernelsLA[3 * 72 + 0 * 9 + 0] + tc1.x * kernelsLA[3 * 72 + 0 * 9 + 1] + tr1.x * kernelsLA[3 * 72 + 0 * 9 + 2] +
		ml1.x * kernelsLA[3 * 72 + 0 * 9 + 3] + mc1.x * kernelsLA[3 * 72 + 0 * 9 + 4] + mr1.x * kernelsLA[3 * 72 + 0 * 9 + 5] +
		bl1.x * kernelsLA[3 * 72 + 0 * 9 + 6] + bc1.x * kernelsLA[3 * 72 + 0 * 9 + 7] + br1.x * kernelsLA[3 * 72 + 0 * 9 + 8] +

		tl1.y * kernelsLA[3 * 72 + 1 * 9 + 0] + tc1.y * kernelsLA[3 * 72 + 1 * 9 + 1] + tr1.y * kernelsLA[3 * 72 + 1 * 9 + 2] +
		ml1.y * kernelsLA[3 * 72 + 1 * 9 + 3] + mc1.y * kernelsLA[3 * 72 + 1 * 9 + 4] + mr1.y * kernelsLA[3 * 72 + 1 * 9 + 5] +
		bl1.y * kernelsLA[3 * 72 + 1 * 9 + 6] + bc1.y * kernelsLA[3 * 72 + 1 * 9 + 7] + br1.y * kernelsLA[3 * 72 + 1 * 9 + 8] +

		tl1.z * kernelsLA[3 * 72 + 2 * 9 + 0] + tc1.z * kernelsLA[3 * 72 + 2 * 9 + 1] + tr1.z * kernelsLA[3 * 72 + 2 * 9 + 2] +
		ml1.z * kernelsLA[3 * 72 + 2 * 9 + 3] + mc1.z * kernelsLA[3 * 72 + 2 * 9 + 4] + mr1.z * kernelsLA[3 * 72 + 2 * 9 + 5] +
		bl1.z * kernelsLA[3 * 72 + 2 * 9 + 6] + bc1.z * kernelsLA[3 * 72 + 2 * 9 + 7] + br1.z * kernelsLA[3 * 72 + 2 * 9 + 8] +

		tl1.w * kernelsLA[3 * 72 + 3 * 9 + 0] + tc1.w * kernelsLA[3 * 72 + 3 * 9 + 1] + tr1.w * kernelsLA[3 * 72 + 3 * 9 + 2] +
		ml1.w * kernelsLA[3 * 72 + 3 * 9 + 3] + mc1.w * kernelsLA[3 * 72 + 3 * 9 + 4] + mr1.w * kernelsLA[3 * 72 + 3 * 9 + 5] +
		bl1.w * kernelsLA[3 * 72 + 3 * 9 + 6] + bc1.w * kernelsLA[3 * 72 + 3 * 9 + 7] + br1.w * kernelsLA[3 * 72 + 3 * 9 + 8] +

		tl2.x * kernelsLA[3 * 72 + 4 * 9 + 0] + tc2.x * kernelsLA[3 * 72 + 4 * 9 + 1] + tr2.x * kernelsLA[3 * 72 + 4 * 9 + 2] +
		ml2.x * kernelsLA[3 * 72 + 4 * 9 + 3] + mc2.x * kernelsLA[3 * 72 + 4 * 9 + 4] + mr2.x * kernelsLA[3 * 72 + 4 * 9 + 5] +
		bl2.x * kernelsLA[3 * 72 + 4 * 9 + 6] + bc2.x * kernelsLA[3 * 72 + 4 * 9 + 7] + br2.x * kernelsLA[3 * 72 + 4 * 9 + 8] +

		tl2.y * kernelsLA[3 * 72 + 5 * 9 + 0] + tc2.y * kernelsLA[3 * 72 + 5 * 9 + 1] + tr2.y * kernelsLA[3 * 72 + 5 * 9 + 2] +
		ml2.y * kernelsLA[3 * 72 + 5 * 9 + 3] + mc2.y * kernelsLA[3 * 72 + 5 * 9 + 4] + mr2.y * kernelsLA[3 * 72 + 5 * 9 + 5] +
		bl2.y * kernelsLA[3 * 72 + 5 * 9 + 6] + bc2.y * kernelsLA[3 * 72 + 5 * 9 + 7] + br2.y * kernelsLA[3 * 72 + 5 * 9 + 8] +

		tl2.z * kernelsLA[3 * 72 + 6 * 9 + 0] + tc2.z * kernelsLA[3 * 72 + 6 * 9 + 1] + tr2.z * kernelsLA[3 * 72 + 6 * 9 + 2] +
		ml2.z * kernelsLA[3 * 72 + 6 * 9 + 3] + mc2.z * kernelsLA[3 * 72 + 6 * 9 + 4] + mr2.z * kernelsLA[3 * 72 + 6 * 9 + 5] +
		bl2.z * kernelsLA[3 * 72 + 6 * 9 + 6] + bc2.z * kernelsLA[3 * 72 + 6 * 9 + 7] + br2.z * kernelsLA[3 * 72 + 6 * 9 + 8] +

		tl2.w * kernelsLA[3 * 72 + 7 * 9 + 0] + tc2.w * kernelsLA[3 * 72 + 7 * 9 + 1] + tr2.w * kernelsLA[3 * 72 + 7 * 9 + 2] +
		ml2.w * kernelsLA[3 * 72 + 7 * 9 + 3] + mc2.w * kernelsLA[3 * 72 + 7 * 9 + 4] + mr2.w * kernelsLA[3 * 72 + 7 * 9 + 5] +
		bl2.w * kernelsLA[3 * 72 + 7 * 9 + 6] + bc2.w * kernelsLA[3 * 72 + 7 * 9 + 7] + br2.w * kernelsLA[3 * 72 + 7 * 9 + 8] + biasLA.w
		));

	float4 target2 = RELU(float4(
		tl1.x * kernelsLB[0 * 72 + 0 * 9 + 0] + tc1.x * kernelsLB[0 * 72 + 0 * 9 + 1] + tr1.x * kernelsLB[0 * 72 + 0 * 9 + 2] +
		ml1.x * kernelsLB[0 * 72 + 0 * 9 + 3] + mc1.x * kernelsLB[0 * 72 + 0 * 9 + 4] + mr1.x * kernelsLB[0 * 72 + 0 * 9 + 5] +
		bl1.x * kernelsLB[0 * 72 + 0 * 9 + 6] + bc1.x * kernelsLB[0 * 72 + 0 * 9 + 7] + br1.x * kernelsLB[0 * 72 + 0 * 9 + 8] +

		tl1.y * kernelsLB[0 * 72 + 1 * 9 + 0] + tc1.y * kernelsLB[0 * 72 + 1 * 9 + 1] + tr1.y * kernelsLB[0 * 72 + 1 * 9 + 2] +
		ml1.y * kernelsLB[0 * 72 + 1 * 9 + 3] + mc1.y * kernelsLB[0 * 72 + 1 * 9 + 4] + mr1.y * kernelsLB[0 * 72 + 1 * 9 + 5] +
		bl1.y * kernelsLB[0 * 72 + 1 * 9 + 6] + bc1.y * kernelsLB[0 * 72 + 1 * 9 + 7] + br1.y * kernelsLB[0 * 72 + 1 * 9 + 8] +

		tl1.z * kernelsLB[0 * 72 + 2 * 9 + 0] + tc1.z * kernelsLB[0 * 72 + 2 * 9 + 1] + tr1.z * kernelsLB[0 * 72 + 2 * 9 + 2] +
		ml1.z * kernelsLB[0 * 72 + 2 * 9 + 3] + mc1.z * kernelsLB[0 * 72 + 2 * 9 + 4] + mr1.z * kernelsLB[0 * 72 + 2 * 9 + 5] +
		bl1.z * kernelsLB[0 * 72 + 2 * 9 + 6] + bc1.z * kernelsLB[0 * 72 + 2 * 9 + 7] + br1.z * kernelsLB[0 * 72 + 2 * 9 + 8] +

		tl1.w * kernelsLB[0 * 72 + 3 * 9 + 0] + tc1.w * kernelsLB[0 * 72 + 3 * 9 + 1] + tr1.w * kernelsLB[0 * 72 + 3 * 9 + 2] +
		ml1.w * kernelsLB[0 * 72 + 3 * 9 + 3] + mc1.w * kernelsLB[0 * 72 + 3 * 9 + 4] + mr1.w * kernelsLB[0 * 72 + 3 * 9 + 5] +
		bl1.w * kernelsLB[0 * 72 + 3 * 9 + 6] + bc1.w * kernelsLB[0 * 72 + 3 * 9 + 7] + br1.w * kernelsLB[0 * 72 + 3 * 9 + 8] +

		tl2.x * kernelsLB[0 * 72 + 4 * 9 + 0] + tc2.x * kernelsLB[0 * 72 + 4 * 9 + 1] + tr2.x * kernelsLB[0 * 72 + 4 * 9 + 2] +
		ml2.x * kernelsLB[0 * 72 + 4 * 9 + 3] + mc2.x * kernelsLB[0 * 72 + 4 * 9 + 4] + mr2.x * kernelsLB[0 * 72 + 4 * 9 + 5] +
		bl2.x * kernelsLB[0 * 72 + 4 * 9 + 6] + bc2.x * kernelsLB[0 * 72 + 4 * 9 + 7] + br2.x * kernelsLB[0 * 72 + 4 * 9 + 8] +

		tl2.y * kernelsLB[0 * 72 + 5 * 9 + 0] + tc2.y * kernelsLB[0 * 72 + 5 * 9 + 1] + tr2.y * kernelsLB[0 * 72 + 5 * 9 + 2] +
		ml2.y * kernelsLB[0 * 72 + 5 * 9 + 3] + mc2.y * kernelsLB[0 * 72 + 5 * 9 + 4] + mr2.y * kernelsLB[0 * 72 + 5 * 9 + 5] +
		bl2.y * kernelsLB[0 * 72 + 5 * 9 + 6] + bc2.y * kernelsLB[0 * 72 + 5 * 9 + 7] + br2.y * kernelsLB[0 * 72 + 5 * 9 + 8] +

		tl2.z * kernelsLB[0 * 72 + 6 * 9 + 0] + tc2.z * kernelsLB[0 * 72 + 6 * 9 + 1] + tr2.z * kernelsLB[0 * 72 + 6 * 9 + 2] +
		ml2.z * kernelsLB[0 * 72 + 6 * 9 + 3] + mc2.z * kernelsLB[0 * 72 + 6 * 9 + 4] + mr2.z * kernelsLB[0 * 72 + 6 * 9 + 5] +
		bl2.z * kernelsLB[0 * 72 + 6 * 9 + 6] + bc2.z * kernelsLB[0 * 72 + 6 * 9 + 7] + br2.z * kernelsLB[0 * 72 + 6 * 9 + 8] +

		tl2.w * kernelsLB[0 * 72 + 7 * 9 + 0] + tc2.w * kernelsLB[0 * 72 + 7 * 9 + 1] + tr2.w * kernelsLB[0 * 72 + 7 * 9 + 2] +
		ml2.w * kernelsLB[0 * 72 + 7 * 9 + 3] + mc2.w * kernelsLB[0 * 72 + 7 * 9 + 4] + mr2.w * kernelsLB[0 * 72 + 7 * 9 + 5] +
		bl2.w * kernelsLB[0 * 72 + 7 * 9 + 6] + bc2.w * kernelsLB[0 * 72 + 7 * 9 + 7] + br2.w * kernelsLB[0 * 72 + 7 * 9 + 8] + biasLB.x
		,
		tl1.x * kernelsLB[1 * 72 + 0 * 9 + 0] + tc1.x * kernelsLB[1 * 72 + 0 * 9 + 1] + tr1.x * kernelsLB[1 * 72 + 0 * 9 + 2] +
		ml1.x * kernelsLB[1 * 72 + 0 * 9 + 3] + mc1.x * kernelsLB[1 * 72 + 0 * 9 + 4] + mr1.x * kernelsLB[1 * 72 + 0 * 9 + 5] +
		bl1.x * kernelsLB[1 * 72 + 0 * 9 + 6] + bc1.x * kernelsLB[1 * 72 + 0 * 9 + 7] + br1.x * kernelsLB[1 * 72 + 0 * 9 + 8] +

		tl1.y * kernelsLB[1 * 72 + 1 * 9 + 0] + tc1.y * kernelsLB[1 * 72 + 1 * 9 + 1] + tr1.y * kernelsLB[1 * 72 + 1 * 9 + 2] +
		ml1.y * kernelsLB[1 * 72 + 1 * 9 + 3] + mc1.y * kernelsLB[1 * 72 + 1 * 9 + 4] + mr1.y * kernelsLB[1 * 72 + 1 * 9 + 5] +
		bl1.y * kernelsLB[1 * 72 + 1 * 9 + 6] + bc1.y * kernelsLB[1 * 72 + 1 * 9 + 7] + br1.y * kernelsLB[1 * 72 + 1 * 9 + 8] +

		tl1.z * kernelsLB[1 * 72 + 2 * 9 + 0] + tc1.z * kernelsLB[1 * 72 + 2 * 9 + 1] + tr1.z * kernelsLB[1 * 72 + 2 * 9 + 2] +
		ml1.z * kernelsLB[1 * 72 + 2 * 9 + 3] + mc1.z * kernelsLB[1 * 72 + 2 * 9 + 4] + mr1.z * kernelsLB[1 * 72 + 2 * 9 + 5] +
		bl1.z * kernelsLB[1 * 72 + 2 * 9 + 6] + bc1.z * kernelsLB[1 * 72 + 2 * 9 + 7] + br1.z * kernelsLB[1 * 72 + 2 * 9 + 8] +

		tl1.w * kernelsLB[1 * 72 + 3 * 9 + 0] + tc1.w * kernelsLB[1 * 72 + 3 * 9 + 1] + tr1.w * kernelsLB[1 * 72 + 3 * 9 + 2] +
		ml1.w * kernelsLB[1 * 72 + 3 * 9 + 3] + mc1.w * kernelsLB[1 * 72 + 3 * 9 + 4] + mr1.w * kernelsLB[1 * 72 + 3 * 9 + 5] +
		bl1.w * kernelsLB[1 * 72 + 3 * 9 + 6] + bc1.w * kernelsLB[1 * 72 + 3 * 9 + 7] + br1.w * kernelsLB[1 * 72 + 3 * 9 + 8] +

		tl2.x * kernelsLB[1 * 72 + 4 * 9 + 0] + tc2.x * kernelsLB[1 * 72 + 4 * 9 + 1] + tr2.x * kernelsLB[1 * 72 + 4 * 9 + 2] +
		ml2.x * kernelsLB[1 * 72 + 4 * 9 + 3] + mc2.x * kernelsLB[1 * 72 + 4 * 9 + 4] + mr2.x * kernelsLB[1 * 72 + 4 * 9 + 5] +
		bl2.x * kernelsLB[1 * 72 + 4 * 9 + 6] + bc2.x * kernelsLB[1 * 72 + 4 * 9 + 7] + br2.x * kernelsLB[1 * 72 + 4 * 9 + 8] +

		tl2.y * kernelsLB[1 * 72 + 5 * 9 + 0] + tc2.y * kernelsLB[1 * 72 + 5 * 9 + 1] + tr2.y * kernelsLB[1 * 72 + 5 * 9 + 2] +
		ml2.y * kernelsLB[1 * 72 + 5 * 9 + 3] + mc2.y * kernelsLB[1 * 72 + 5 * 9 + 4] + mr2.y * kernelsLB[1 * 72 + 5 * 9 + 5] +
		bl2.y * kernelsLB[1 * 72 + 5 * 9 + 6] + bc2.y * kernelsLB[1 * 72 + 5 * 9 + 7] + br2.y * kernelsLB[1 * 72 + 5 * 9 + 8] +

		tl2.z * kernelsLB[1 * 72 + 6 * 9 + 0] + tc2.z * kernelsLB[1 * 72 + 6 * 9 + 1] + tr2.z * kernelsLB[1 * 72 + 6 * 9 + 2] +
		ml2.z * kernelsLB[1 * 72 + 6 * 9 + 3] + mc2.z * kernelsLB[1 * 72 + 6 * 9 + 4] + mr2.z * kernelsLB[1 * 72 + 6 * 9 + 5] +
		bl2.z * kernelsLB[1 * 72 + 6 * 9 + 6] + bc2.z * kernelsLB[1 * 72 + 6 * 9 + 7] + br2.z * kernelsLB[1 * 72 + 6 * 9 + 8] +

		tl2.w * kernelsLB[1 * 72 + 7 * 9 + 0] + tc2.w * kernelsLB[1 * 72 + 7 * 9 + 1] + tr2.w * kernelsLB[1 * 72 + 7 * 9 + 2] +
		ml2.w * kernelsLB[1 * 72 + 7 * 9 + 3] + mc2.w * kernelsLB[1 * 72 + 7 * 9 + 4] + mr2.w * kernelsLB[1 * 72 + 7 * 9 + 5] +
		bl2.w * kernelsLB[1 * 72 + 7 * 9 + 6] + bc2.w * kernelsLB[1 * 72 + 7 * 9 + 7] + br2.w * kernelsLB[1 * 72 + 7 * 9 + 8] + biasLB.y
		,
		tl1.x * kernelsLB[2 * 72 + 0 * 9 + 0] + tc1.x * kernelsLB[2 * 72 + 0 * 9 + 1] + tr1.x * kernelsLB[2 * 72 + 0 * 9 + 2] +
		ml1.x * kernelsLB[2 * 72 + 0 * 9 + 3] + mc1.x * kernelsLB[2 * 72 + 0 * 9 + 4] + mr1.x * kernelsLB[2 * 72 + 0 * 9 + 5] +
		bl1.x * kernelsLB[2 * 72 + 0 * 9 + 6] + bc1.x * kernelsLB[2 * 72 + 0 * 9 + 7] + br1.x * kernelsLB[2 * 72 + 0 * 9 + 8] +

		tl1.y * kernelsLB[2 * 72 + 1 * 9 + 0] + tc1.y * kernelsLB[2 * 72 + 1 * 9 + 1] + tr1.y * kernelsLB[2 * 72 + 1 * 9 + 2] +
		ml1.y * kernelsLB[2 * 72 + 1 * 9 + 3] + mc1.y * kernelsLB[2 * 72 + 1 * 9 + 4] + mr1.y * kernelsLB[2 * 72 + 1 * 9 + 5] +
		bl1.y * kernelsLB[2 * 72 + 1 * 9 + 6] + bc1.y * kernelsLB[2 * 72 + 1 * 9 + 7] + br1.y * kernelsLB[2 * 72 + 1 * 9 + 8] +

		tl1.z * kernelsLB[2 * 72 + 2 * 9 + 0] + tc1.z * kernelsLB[2 * 72 + 2 * 9 + 1] + tr1.z * kernelsLB[2 * 72 + 2 * 9 + 2] +
		ml1.z * kernelsLB[2 * 72 + 2 * 9 + 3] + mc1.z * kernelsLB[2 * 72 + 2 * 9 + 4] + mr1.z * kernelsLB[2 * 72 + 2 * 9 + 5] +
		bl1.z * kernelsLB[2 * 72 + 2 * 9 + 6] + bc1.z * kernelsLB[2 * 72 + 2 * 9 + 7] + br1.z * kernelsLB[2 * 72 + 2 * 9 + 8] +

		tl1.w * kernelsLB[2 * 72 + 3 * 9 + 0] + tc1.w * kernelsLB[2 * 72 + 3 * 9 + 1] + tr1.w * kernelsLB[2 * 72 + 3 * 9 + 2] +
		ml1.w * kernelsLB[2 * 72 + 3 * 9 + 3] + mc1.w * kernelsLB[2 * 72 + 3 * 9 + 4] + mr1.w * kernelsLB[2 * 72 + 3 * 9 + 5] +
		bl1.w * kernelsLB[2 * 72 + 3 * 9 + 6] + bc1.w * kernelsLB[2 * 72 + 3 * 9 + 7] + br1.w * kernelsLB[2 * 72 + 3 * 9 + 8] +

		tl2.x * kernelsLB[2 * 72 + 4 * 9 + 0] + tc2.x * kernelsLB[2 * 72 + 4 * 9 + 1] + tr2.x * kernelsLB[2 * 72 + 4 * 9 + 2] +
		ml2.x * kernelsLB[2 * 72 + 4 * 9 + 3] + mc2.x * kernelsLB[2 * 72 + 4 * 9 + 4] + mr2.x * kernelsLB[2 * 72 + 4 * 9 + 5] +
		bl2.x * kernelsLB[2 * 72 + 4 * 9 + 6] + bc2.x * kernelsLB[2 * 72 + 4 * 9 + 7] + br2.x * kernelsLB[2 * 72 + 4 * 9 + 8] +

		tl2.y * kernelsLB[2 * 72 + 5 * 9 + 0] + tc2.y * kernelsLB[2 * 72 + 5 * 9 + 1] + tr2.y * kernelsLB[2 * 72 + 5 * 9 + 2] +
		ml2.y * kernelsLB[2 * 72 + 5 * 9 + 3] + mc2.y * kernelsLB[2 * 72 + 5 * 9 + 4] + mr2.y * kernelsLB[2 * 72 + 5 * 9 + 5] +
		bl2.y * kernelsLB[2 * 72 + 5 * 9 + 6] + bc2.y * kernelsLB[2 * 72 + 5 * 9 + 7] + br2.y * kernelsLB[2 * 72 + 5 * 9 + 8] +

		tl2.z * kernelsLB[2 * 72 + 6 * 9 + 0] + tc2.z * kernelsLB[2 * 72 + 6 * 9 + 1] + tr2.z * kernelsLB[2 * 72 + 6 * 9 + 2] +
		ml2.z * kernelsLB[2 * 72 + 6 * 9 + 3] + mc2.z * kernelsLB[2 * 72 + 6 * 9 + 4] + mr2.z * kernelsLB[2 * 72 + 6 * 9 + 5] +
		bl2.z * kernelsLB[2 * 72 + 6 * 9 + 6] + bc2.z * kernelsLB[2 * 72 + 6 * 9 + 7] + br2.z * kernelsLB[2 * 72 + 6 * 9 + 8] +

		tl2.w * kernelsLB[2 * 72 + 7 * 9 + 0] + tc2.w * kernelsLB[2 * 72 + 7 * 9 + 1] + tr2.w * kernelsLB[2 * 72 + 7 * 9 + 2] +
		ml2.w * kernelsLB[2 * 72 + 7 * 9 + 3] + mc2.w * kernelsLB[2 * 72 + 7 * 9 + 4] + mr2.w * kernelsLB[2 * 72 + 7 * 9 + 5] +
		bl2.w * kernelsLB[2 * 72 + 7 * 9 + 6] + bc2.w * kernelsLB[2 * 72 + 7 * 9 + 7] + br2.w * kernelsLB[2 * 72 + 7 * 9 + 8] + biasLB.z
		,
		tl1.x * kernelsLB[3 * 72 + 0 * 9 + 0] + tc1.x * kernelsLB[3 * 72 + 0 * 9 + 1] + tr1.x * kernelsLB[3 * 72 + 0 * 9 + 2] +
		ml1.x * kernelsLB[3 * 72 + 0 * 9 + 3] + mc1.x * kernelsLB[3 * 72 + 0 * 9 + 4] + mr1.x * kernelsLB[3 * 72 + 0 * 9 + 5] +
		bl1.x * kernelsLB[3 * 72 + 0 * 9 + 6] + bc1.x * kernelsLB[3 * 72 + 0 * 9 + 7] + br1.x * kernelsLB[3 * 72 + 0 * 9 + 8] +

		tl1.y * kernelsLB[3 * 72 + 1 * 9 + 0] + tc1.y * kernelsLB[3 * 72 + 1 * 9 + 1] + tr1.y * kernelsLB[3 * 72 + 1 * 9 + 2] +
		ml1.y * kernelsLB[3 * 72 + 1 * 9 + 3] + mc1.y * kernelsLB[3 * 72 + 1 * 9 + 4] + mr1.y * kernelsLB[3 * 72 + 1 * 9 + 5] +
		bl1.y * kernelsLB[3 * 72 + 1 * 9 + 6] + bc1.y * kernelsLB[3 * 72 + 1 * 9 + 7] + br1.y * kernelsLB[3 * 72 + 1 * 9 + 8] +

		tl1.z * kernelsLB[3 * 72 + 2 * 9 + 0] + tc1.z * kernelsLB[3 * 72 + 2 * 9 + 1] + tr1.z * kernelsLB[3 * 72 + 2 * 9 + 2] +
		ml1.z * kernelsLB[3 * 72 + 2 * 9 + 3] + mc1.z * kernelsLB[3 * 72 + 2 * 9 + 4] + mr1.z * kernelsLB[3 * 72 + 2 * 9 + 5] +
		bl1.z * kernelsLB[3 * 72 + 2 * 9 + 6] + bc1.z * kernelsLB[3 * 72 + 2 * 9 + 7] + br1.z * kernelsLB[3 * 72 + 2 * 9 + 8] +

		tl1.w * kernelsLB[3 * 72 + 3 * 9 + 0] + tc1.w * kernelsLB[3 * 72 + 3 * 9 + 1] + tr1.w * kernelsLB[3 * 72 + 3 * 9 + 2] +
		ml1.w * kernelsLB[3 * 72 + 3 * 9 + 3] + mc1.w * kernelsLB[3 * 72 + 3 * 9 + 4] + mr1.w * kernelsLB[3 * 72 + 3 * 9 + 5] +
		bl1.w * kernelsLB[3 * 72 + 3 * 9 + 6] + bc1.w * kernelsLB[3 * 72 + 3 * 9 + 7] + br1.w * kernelsLB[3 * 72 + 3 * 9 + 8] +

		tl2.x * kernelsLB[3 * 72 + 4 * 9 + 0] + tc2.x * kernelsLB[3 * 72 + 4 * 9 + 1] + tr2.x * kernelsLB[3 * 72 + 4 * 9 + 2] +
		ml2.x * kernelsLB[3 * 72 + 4 * 9 + 3] + mc2.x * kernelsLB[3 * 72 + 4 * 9 + 4] + mr2.x * kernelsLB[3 * 72 + 4 * 9 + 5] +
		bl2.x * kernelsLB[3 * 72 + 4 * 9 + 6] + bc2.x * kernelsLB[3 * 72 + 4 * 9 + 7] + br2.x * kernelsLB[3 * 72 + 4 * 9 + 8] +

		tl2.y * kernelsLB[3 * 72 + 5 * 9 + 0] + tc2.y * kernelsLB[3 * 72 + 5 * 9 + 1] + tr2.y * kernelsLB[3 * 72 + 5 * 9 + 2] +
		ml2.y * kernelsLB[3 * 72 + 5 * 9 + 3] + mc2.y * kernelsLB[3 * 72 + 5 * 9 + 4] + mr2.y * kernelsLB[3 * 72 + 5 * 9 + 5] +
		bl2.y * kernelsLB[3 * 72 + 5 * 9 + 6] + bc2.y * kernelsLB[3 * 72 + 5 * 9 + 7] + br2.y * kernelsLB[3 * 72 + 5 * 9 + 8] +

		tl2.z * kernelsLB[3 * 72 + 6 * 9 + 0] + tc2.z * kernelsLB[3 * 72 + 6 * 9 + 1] + tr2.z * kernelsLB[3 * 72 + 6 * 9 + 2] +
		ml2.z * kernelsLB[3 * 72 + 6 * 9 + 3] + mc2.z * kernelsLB[3 * 72 + 6 * 9 + 4] + mr2.z * kernelsLB[3 * 72 + 6 * 9 + 5] +
		bl2.z * kernelsLB[3 * 72 + 6 * 9 + 6] + bc2.z * kernelsLB[3 * 72 + 6 * 9 + 7] + br2.z * kernelsLB[3 * 72 + 6 * 9 + 8] +

		tl2.w * kernelsLB[3 * 72 + 7 * 9 + 0] + tc2.w * kernelsLB[3 * 72 + 7 * 9 + 1] + tr2.w * kernelsLB[3 * 72 + 7 * 9 + 2] +
		ml2.w * kernelsLB[3 * 72 + 7 * 9 + 3] + mc2.w * kernelsLB[3 * 72 + 7 * 9 + 4] + mr2.w * kernelsLB[3 * 72 + 7 * 9 + 5] +
		bl2.w * kernelsLB[3 * 72 + 7 * 9 + 6] + bc2.w * kernelsLB[3 * 72 + 7 * 9 + 7] + br2.w * kernelsLB[3 * 72 + 7 * 9 + 8] + biasLB.w
	));

	tex3[gxy] = target1;
	tex4[gxy] = target2;
}


//!PASS 9
//!IN INPUT, tex3, tex4
//!BLOCK_SIZE 16
//!NUM_THREADS 64

const static float kernelsLA[9 * 8 * 4] = {
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

const static float4 biasLA = { -0.0006,  0.0117,  0.0083,  0.0686 };

const static float kernelsLB[9 * 8 * 4] = {
	-8.8977e-41,  5.9533e-40, -3.1413e-40,
	-3.1892e-40,  5.5204e-40, -5.0634e-40,
	-2.4932e-41,  4.3474e-41,  6.2961e-40,
	 4.7864e-03,  5.7125e-02, -1.5468e-02,
	-3.9614e-03, -2.9042e-02,  2.8347e-01,
	-1.0133e-02,  8.2745e-02, -1.0450e-01,
	 5.9537e-03,  1.4050e-02,  1.9802e-04,
	 2.4964e-02,  1.3077e-01, -4.7314e-02,
	 6.2744e-03, -1.9068e-01,  5.2593e-02,
	-2.0550e-40, -2.4231e-40,  3.3927e-40,
	-3.9609e-41,  2.2262e-40,  1.8866e-40,
	 2.0788e-40, -1.8012e-40, -1.9375e-40,
	-4.7530e-03, -1.2315e-01,  8.2373e-03,
	-9.2412e-02,  1.7156e-01,  1.1176e-02,
	-1.4081e-02,  1.4694e-02, -1.9475e-02,
	-1.5269e-02, -3.8430e-02, -7.4717e-02,
	 3.3361e-02, -1.1956e-01,  4.2304e-01,
	-2.9924e-03, -3.3035e-02, -3.6560e-02,
	-1.2386e-02,  6.3762e-03, -3.7047e-02,
	 1.3839e-02, -3.6358e-02,  4.3609e-02,
	-8.3692e-03,  4.5794e-01, -3.0761e-01,
	 2.2287e-02,  2.5360e-02, -6.1253e-03,
	-1.8992e-02, -4.0078e-01,  7.3821e-02,
	 5.6517e-03,  4.2348e-02, -2.5642e-02,
	 5.5659e-40, -6.1219e-40,  4.1493e-40,
	 5.7719e-42, -3.7181e-40, -3.3260e-40,
	-4.8241e-41,  5.2207e-40, -1.2199e-40,
	-1.2074e-02,  1.7647e-01,  1.1882e-02,
	 6.4764e-03, -2.3742e-01, -1.8033e-01,
	 2.5866e-02,  6.5985e-02,  3.7191e-02,
	 5.1047e-02, -3.0457e-02,  1.2531e-02,
	-1.3252e-01,  1.2593e-01, -6.3717e-02,
	 4.0794e-02, -1.4786e-02,  1.7139e-02,
	 2.4343e-40, -1.7451e-40,  2.0169e-40,
	-5.5166e-40,  2.4201e-40, -2.5701e-40,
	 2.9947e-40,  2.9321e-40, -1.6015e-40,
	-3.6598e-02, -1.8520e-03, -1.6999e-01,
	-8.6806e-02, -7.7266e-02, -9.6042e-02,
	-2.1342e-02,  2.5793e-02, -7.2541e-03,
	 3.0667e-02, -2.6287e-01,  3.0592e-02,
	-4.5559e-02, -1.4716e-01,  2.0932e-01,
	-5.8472e-03, -1.0023e-02,  1.2134e-02,
	-1.3284e-02,  2.0538e-02, -5.4476e-04,
	 5.8096e-02, -1.4790e-02, -2.0158e-02,
	-3.9654e-02, -2.2069e-01, -1.5089e-01,
	-1.8966e-01, -1.6834e-01,  9.8934e-02,
	 8.2326e-02,  7.5585e-02, -1.7188e-02,
	-1.4985e-02,  2.1823e-02, -7.7015e-03,
	 1.8353e-40,  4.8298e-40, -2.0568e-40,
	-3.7196e-40, -5.7237e-40,  1.0648e-40,
	 9.4960e-41,  3.0411e-40,  1.3294e-40,
	-1.4884e-02,  4.9767e-02, -3.0288e-02,
	 8.9874e-03, -1.0290e-01,  3.1344e-01,
	 5.9735e-03, -2.0813e-01, -6.6145e-03,
	 1.6592e-02,  3.0529e-05, -1.0180e-02,
	-4.8683e-02,  1.4025e-01,  2.9237e-02,
	-2.3334e-02, -9.6638e-02, -1.0268e-02,
	-4.9497e-41, -5.6377e-40, -2.0142e-40,
	 2.1230e-40,  1.6067e-40,  3.4830e-40,
	-4.9031e-40, -3.0290e-40, -2.9060e-40,
	 3.4053e-02, -8.9560e-02, -4.4479e-02,
	 4.2128e-02,  6.9253e-02, -7.1096e-03,
	 4.2358e-02, -1.7215e-02,  9.0389e-03,
	 1.8129e-02, -1.4785e-01,  1.1267e-01,
	-7.1637e-02,  5.5595e-01, -1.0569e-02,
	 1.8481e-02, -4.7556e-02, -1.1185e-02,
	-1.1766e-02, -8.5959e-03, -3.0046e-02,
	-2.1081e-03,  1.1518e-01, -8.4419e-02,
	-7.5829e-02,  1.8199e-01, -9.7726e-03,
	 3.6473e-02,  1.8761e-01,  4.9495e-03,
	-6.9640e-02, -2.8775e-01,  3.6149e-02,
	 9.6345e-04,  1.3967e-02, -6.0015e-03,
	 2.9861e-40,  3.9190e-40,  5.3741e-40,
	 3.8059e-40,  4.7113e-40,  5.9498e-40,
	-5.0640e-40, -4.1610e-40,  6.2009e-40,
	-2.3464e-03, -7.3888e-02,  3.4701e-02,
	-5.2257e-04,  3.8444e-02, -5.3735e-01,
	-1.7970e-03,  9.0298e-02,  5.3151e-02,
	-2.6033e-02,  1.2973e-02,  4.9147e-03,
	 2.3005e-02,  1.7045e-01,  2.4715e-02,
	 2.7981e-02, -8.4662e-02, -9.4778e-03,
	 5.3019e-40, -2.1800e-40,  1.5281e-40,
	-1.0282e-40,  1.8040e-41,  1.3929e-40,
	-5.9679e-40, -5.2958e-40,  1.4429e-40,
	 3.4325e-02, -1.7240e-01, -4.9645e-02,
	-2.4341e-02,  5.2652e-02, -1.1188e-02,
	-3.6336e-03,  4.2148e-04,  3.3086e-03,
	 5.5059e-03,  1.7744e-01, -2.8681e-02,
	-3.4868e-03, -1.4569e-01,  1.6508e-02,
	 4.6766e-03, -1.7963e-02, -2.6397e-03,
	 4.3618e-03, -4.2793e-03, -4.7820e-04,
	-4.2795e-02,  2.0070e-01,  3.8402e-02,
	 5.0586e-02,  2.1910e-01, -3.4381e-02,
	 5.7625e-02,  4.2314e-01, -1.9732e-02,
	 3.4811e-02, -2.3033e-01,  1.1477e-02,
	-7.3744e-03,  1.9112e-02,  4.2251e-03
};

const static float4 biasLB = { -0.0046,  0.0015, -0.0076,  0.0079 };

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

const static float2x3 rgb2uv = {
	-0.169, -0.331, 0.5,
	0.5, -0.419, -0.081
};

const static float3x3 yuv2rgb = {
	1, -0.00093, 1.401687,
	1, -0.3437, -0.71417,
	1, 1.77216, 0.00099
};

void Pass9(uint2 blockStart, uint3 threadId) {
	uint2 gxy = (Rmp8x8(threadId.x) << 1) + blockStart;

	if (!CheckViewport(gxy)) {
		return;
	}

	float2 inputPt = GetInputPt();
	float2 outputPt = GetOutputPt();

	float2 pos = ((gxy >> 1) + 0.5f) * inputPt;

	// [tl, tc, tr]
	// [ml, mc, mr]
	// [bl, bc, br]
	float4 tl1 = tex3.SampleLevel(sam, pos + float2(-inputPt.x, -inputPt.y), 0);
	float4 ml1 = tex3.SampleLevel(sam, pos + float2(-inputPt.x, 0), 0);
	float4 bl1 = tex3.SampleLevel(sam, pos + float2(-inputPt.x, inputPt.y), 0);
	float4 tc1 = tex3.SampleLevel(sam, pos + float2(0, -inputPt.y), 0);
	float4 mc1 = tex3.SampleLevel(sam, pos, 0);
	float4 bc1 = tex3.SampleLevel(sam, pos + float2(0, inputPt.y), 0);
	float4 tr1 = tex3.SampleLevel(sam, pos + float2(inputPt.x, -inputPt.y), 0);
	float4 mr1 = tex3.SampleLevel(sam, pos + float2(inputPt.x, 0), 0);
	float4 br1 = tex3.SampleLevel(sam, pos + float2(inputPt.x, inputPt.y), 0);

	float4 tl2 = tex4.SampleLevel(sam, pos + float2(-inputPt.x, -inputPt.y), 0);
	float4 ml2 = tex4.SampleLevel(sam, pos + float2(-inputPt.x, 0), 0);
	float4 bl2 = tex4.SampleLevel(sam, pos + float2(-inputPt.x, inputPt.y), 0);
	float4 tc2 = tex4.SampleLevel(sam, pos + float2(0, -inputPt.y), 0);
	float4 mc2 = tex4.SampleLevel(sam, pos, 0);
	float4 bc2 = tex4.SampleLevel(sam, pos + float2(0, inputPt.y), 0);
	float4 tr2 = tex4.SampleLevel(sam, pos + float2(inputPt.x, -inputPt.y), 0);
	float4 mr2 = tex4.SampleLevel(sam, pos + float2(inputPt.x, 0), 0);
	float4 br2 = tex4.SampleLevel(sam, pos + float2(inputPt.x, inputPt.y), 0);

	float4 target1 = RELU(float4(
		tl1.x * kernelsLA[0 * 72 + 0 * 9 + 0] + tc1.x * kernelsLA[0 * 72 + 0 * 9 + 1] + tr1.x * kernelsLA[0 * 72 + 0 * 9 + 2] +
		ml1.x * kernelsLA[0 * 72 + 0 * 9 + 3] + mc1.x * kernelsLA[0 * 72 + 0 * 9 + 4] + mr1.x * kernelsLA[0 * 72 + 0 * 9 + 5] +
		bl1.x * kernelsLA[0 * 72 + 0 * 9 + 6] + bc1.x * kernelsLA[0 * 72 + 0 * 9 + 7] + br1.x * kernelsLA[0 * 72 + 0 * 9 + 8] +

		tl1.y * kernelsLA[0 * 72 + 1 * 9 + 0] + tc1.y * kernelsLA[0 * 72 + 1 * 9 + 1] + tr1.y * kernelsLA[0 * 72 + 1 * 9 + 2] +
		ml1.y * kernelsLA[0 * 72 + 1 * 9 + 3] + mc1.y * kernelsLA[0 * 72 + 1 * 9 + 4] + mr1.y * kernelsLA[0 * 72 + 1 * 9 + 5] +
		bl1.y * kernelsLA[0 * 72 + 1 * 9 + 6] + bc1.y * kernelsLA[0 * 72 + 1 * 9 + 7] + br1.y * kernelsLA[0 * 72 + 1 * 9 + 8] +

		tl1.z * kernelsLA[0 * 72 + 2 * 9 + 0] + tc1.z * kernelsLA[0 * 72 + 2 * 9 + 1] + tr1.z * kernelsLA[0 * 72 + 2 * 9 + 2] +
		ml1.z * kernelsLA[0 * 72 + 2 * 9 + 3] + mc1.z * kernelsLA[0 * 72 + 2 * 9 + 4] + mr1.z * kernelsLA[0 * 72 + 2 * 9 + 5] +
		bl1.z * kernelsLA[0 * 72 + 2 * 9 + 6] + bc1.z * kernelsLA[0 * 72 + 2 * 9 + 7] + br1.z * kernelsLA[0 * 72 + 2 * 9 + 8] +

		tl1.w * kernelsLA[0 * 72 + 3 * 9 + 0] + tc1.w * kernelsLA[0 * 72 + 3 * 9 + 1] + tr1.w * kernelsLA[0 * 72 + 3 * 9 + 2] +
		ml1.w * kernelsLA[0 * 72 + 3 * 9 + 3] + mc1.w * kernelsLA[0 * 72 + 3 * 9 + 4] + mr1.w * kernelsLA[0 * 72 + 3 * 9 + 5] +
		bl1.w * kernelsLA[0 * 72 + 3 * 9 + 6] + bc1.w * kernelsLA[0 * 72 + 3 * 9 + 7] + br1.w * kernelsLA[0 * 72 + 3 * 9 + 8] +

		tl2.x * kernelsLA[0 * 72 + 4 * 9 + 0] + tc2.x * kernelsLA[0 * 72 + 4 * 9 + 1] + tr2.x * kernelsLA[0 * 72 + 4 * 9 + 2] +
		ml2.x * kernelsLA[0 * 72 + 4 * 9 + 3] + mc2.x * kernelsLA[0 * 72 + 4 * 9 + 4] + mr2.x * kernelsLA[0 * 72 + 4 * 9 + 5] +
		bl2.x * kernelsLA[0 * 72 + 4 * 9 + 6] + bc2.x * kernelsLA[0 * 72 + 4 * 9 + 7] + br2.x * kernelsLA[0 * 72 + 4 * 9 + 8] +

		tl2.y * kernelsLA[0 * 72 + 5 * 9 + 0] + tc2.y * kernelsLA[0 * 72 + 5 * 9 + 1] + tr2.y * kernelsLA[0 * 72 + 5 * 9 + 2] +
		ml2.y * kernelsLA[0 * 72 + 5 * 9 + 3] + mc2.y * kernelsLA[0 * 72 + 5 * 9 + 4] + mr2.y * kernelsLA[0 * 72 + 5 * 9 + 5] +
		bl2.y * kernelsLA[0 * 72 + 5 * 9 + 6] + bc2.y * kernelsLA[0 * 72 + 5 * 9 + 7] + br2.y * kernelsLA[0 * 72 + 5 * 9 + 8] +

		tl2.z * kernelsLA[0 * 72 + 6 * 9 + 0] + tc2.z * kernelsLA[0 * 72 + 6 * 9 + 1] + tr2.z * kernelsLA[0 * 72 + 6 * 9 + 2] +
		ml2.z * kernelsLA[0 * 72 + 6 * 9 + 3] + mc2.z * kernelsLA[0 * 72 + 6 * 9 + 4] + mr2.z * kernelsLA[0 * 72 + 6 * 9 + 5] +
		bl2.z * kernelsLA[0 * 72 + 6 * 9 + 6] + bc2.z * kernelsLA[0 * 72 + 6 * 9 + 7] + br2.z * kernelsLA[0 * 72 + 6 * 9 + 8] +

		tl2.w * kernelsLA[0 * 72 + 7 * 9 + 0] + tc2.w * kernelsLA[0 * 72 + 7 * 9 + 1] + tr2.w * kernelsLA[0 * 72 + 7 * 9 + 2] +
		ml2.w * kernelsLA[0 * 72 + 7 * 9 + 3] + mc2.w * kernelsLA[0 * 72 + 7 * 9 + 4] + mr2.w * kernelsLA[0 * 72 + 7 * 9 + 5] +
		bl2.w * kernelsLA[0 * 72 + 7 * 9 + 6] + bc2.w * kernelsLA[0 * 72 + 7 * 9 + 7] + br2.w * kernelsLA[0 * 72 + 7 * 9 + 8] + biasLA.x
		,
		tl1.x * kernelsLA[1 * 72 + 0 * 9 + 0] + tc1.x * kernelsLA[1 * 72 + 0 * 9 + 1] + tr1.x * kernelsLA[1 * 72 + 0 * 9 + 2] +
		ml1.x * kernelsLA[1 * 72 + 0 * 9 + 3] + mc1.x * kernelsLA[1 * 72 + 0 * 9 + 4] + mr1.x * kernelsLA[1 * 72 + 0 * 9 + 5] +
		bl1.x * kernelsLA[1 * 72 + 0 * 9 + 6] + bc1.x * kernelsLA[1 * 72 + 0 * 9 + 7] + br1.x * kernelsLA[1 * 72 + 0 * 9 + 8] +

		tl1.y * kernelsLA[1 * 72 + 1 * 9 + 0] + tc1.y * kernelsLA[1 * 72 + 1 * 9 + 1] + tr1.y * kernelsLA[1 * 72 + 1 * 9 + 2] +
		ml1.y * kernelsLA[1 * 72 + 1 * 9 + 3] + mc1.y * kernelsLA[1 * 72 + 1 * 9 + 4] + mr1.y * kernelsLA[1 * 72 + 1 * 9 + 5] +
		bl1.y * kernelsLA[1 * 72 + 1 * 9 + 6] + bc1.y * kernelsLA[1 * 72 + 1 * 9 + 7] + br1.y * kernelsLA[1 * 72 + 1 * 9 + 8] +

		tl1.z * kernelsLA[1 * 72 + 2 * 9 + 0] + tc1.z * kernelsLA[1 * 72 + 2 * 9 + 1] + tr1.z * kernelsLA[1 * 72 + 2 * 9 + 2] +
		ml1.z * kernelsLA[1 * 72 + 2 * 9 + 3] + mc1.z * kernelsLA[1 * 72 + 2 * 9 + 4] + mr1.z * kernelsLA[1 * 72 + 2 * 9 + 5] +
		bl1.z * kernelsLA[1 * 72 + 2 * 9 + 6] + bc1.z * kernelsLA[1 * 72 + 2 * 9 + 7] + br1.z * kernelsLA[1 * 72 + 2 * 9 + 8] +

		tl1.w * kernelsLA[1 * 72 + 3 * 9 + 0] + tc1.w * kernelsLA[1 * 72 + 3 * 9 + 1] + tr1.w * kernelsLA[1 * 72 + 3 * 9 + 2] +
		ml1.w * kernelsLA[1 * 72 + 3 * 9 + 3] + mc1.w * kernelsLA[1 * 72 + 3 * 9 + 4] + mr1.w * kernelsLA[1 * 72 + 3 * 9 + 5] +
		bl1.w * kernelsLA[1 * 72 + 3 * 9 + 6] + bc1.w * kernelsLA[1 * 72 + 3 * 9 + 7] + br1.w * kernelsLA[1 * 72 + 3 * 9 + 8] +

		tl2.x * kernelsLA[1 * 72 + 4 * 9 + 0] + tc2.x * kernelsLA[1 * 72 + 4 * 9 + 1] + tr2.x * kernelsLA[1 * 72 + 4 * 9 + 2] +
		ml2.x * kernelsLA[1 * 72 + 4 * 9 + 3] + mc2.x * kernelsLA[1 * 72 + 4 * 9 + 4] + mr2.x * kernelsLA[1 * 72 + 4 * 9 + 5] +
		bl2.x * kernelsLA[1 * 72 + 4 * 9 + 6] + bc2.x * kernelsLA[1 * 72 + 4 * 9 + 7] + br2.x * kernelsLA[1 * 72 + 4 * 9 + 8] +

		tl2.y * kernelsLA[1 * 72 + 5 * 9 + 0] + tc2.y * kernelsLA[1 * 72 + 5 * 9 + 1] + tr2.y * kernelsLA[1 * 72 + 5 * 9 + 2] +
		ml2.y * kernelsLA[1 * 72 + 5 * 9 + 3] + mc2.y * kernelsLA[1 * 72 + 5 * 9 + 4] + mr2.y * kernelsLA[1 * 72 + 5 * 9 + 5] +
		bl2.y * kernelsLA[1 * 72 + 5 * 9 + 6] + bc2.y * kernelsLA[1 * 72 + 5 * 9 + 7] + br2.y * kernelsLA[1 * 72 + 5 * 9 + 8] +

		tl2.z * kernelsLA[1 * 72 + 6 * 9 + 0] + tc2.z * kernelsLA[1 * 72 + 6 * 9 + 1] + tr2.z * kernelsLA[1 * 72 + 6 * 9 + 2] +
		ml2.z * kernelsLA[1 * 72 + 6 * 9 + 3] + mc2.z * kernelsLA[1 * 72 + 6 * 9 + 4] + mr2.z * kernelsLA[1 * 72 + 6 * 9 + 5] +
		bl2.z * kernelsLA[1 * 72 + 6 * 9 + 6] + bc2.z * kernelsLA[1 * 72 + 6 * 9 + 7] + br2.z * kernelsLA[1 * 72 + 6 * 9 + 8] +

		tl2.w * kernelsLA[1 * 72 + 7 * 9 + 0] + tc2.w * kernelsLA[1 * 72 + 7 * 9 + 1] + tr2.w * kernelsLA[1 * 72 + 7 * 9 + 2] +
		ml2.w * kernelsLA[1 * 72 + 7 * 9 + 3] + mc2.w * kernelsLA[1 * 72 + 7 * 9 + 4] + mr2.w * kernelsLA[1 * 72 + 7 * 9 + 5] +
		bl2.w * kernelsLA[1 * 72 + 7 * 9 + 6] + bc2.w * kernelsLA[1 * 72 + 7 * 9 + 7] + br2.w * kernelsLA[1 * 72 + 7 * 9 + 8] + biasLA.y
		,
		tl1.x * kernelsLA[2 * 72 + 0 * 9 + 0] + tc1.x * kernelsLA[2 * 72 + 0 * 9 + 1] + tr1.x * kernelsLA[2 * 72 + 0 * 9 + 2] +
		ml1.x * kernelsLA[2 * 72 + 0 * 9 + 3] + mc1.x * kernelsLA[2 * 72 + 0 * 9 + 4] + mr1.x * kernelsLA[2 * 72 + 0 * 9 + 5] +
		bl1.x * kernelsLA[2 * 72 + 0 * 9 + 6] + bc1.x * kernelsLA[2 * 72 + 0 * 9 + 7] + br1.x * kernelsLA[2 * 72 + 0 * 9 + 8] +

		tl1.y * kernelsLA[2 * 72 + 1 * 9 + 0] + tc1.y * kernelsLA[2 * 72 + 1 * 9 + 1] + tr1.y * kernelsLA[2 * 72 + 1 * 9 + 2] +
		ml1.y * kernelsLA[2 * 72 + 1 * 9 + 3] + mc1.y * kernelsLA[2 * 72 + 1 * 9 + 4] + mr1.y * kernelsLA[2 * 72 + 1 * 9 + 5] +
		bl1.y * kernelsLA[2 * 72 + 1 * 9 + 6] + bc1.y * kernelsLA[2 * 72 + 1 * 9 + 7] + br1.y * kernelsLA[2 * 72 + 1 * 9 + 8] +

		tl1.z * kernelsLA[2 * 72 + 2 * 9 + 0] + tc1.z * kernelsLA[2 * 72 + 2 * 9 + 1] + tr1.z * kernelsLA[2 * 72 + 2 * 9 + 2] +
		ml1.z * kernelsLA[2 * 72 + 2 * 9 + 3] + mc1.z * kernelsLA[2 * 72 + 2 * 9 + 4] + mr1.z * kernelsLA[2 * 72 + 2 * 9 + 5] +
		bl1.z * kernelsLA[2 * 72 + 2 * 9 + 6] + bc1.z * kernelsLA[2 * 72 + 2 * 9 + 7] + br1.z * kernelsLA[2 * 72 + 2 * 9 + 8] +

		tl1.w * kernelsLA[2 * 72 + 3 * 9 + 0] + tc1.w * kernelsLA[2 * 72 + 3 * 9 + 1] + tr1.w * kernelsLA[2 * 72 + 3 * 9 + 2] +
		ml1.w * kernelsLA[2 * 72 + 3 * 9 + 3] + mc1.w * kernelsLA[2 * 72 + 3 * 9 + 4] + mr1.w * kernelsLA[2 * 72 + 3 * 9 + 5] +
		bl1.w * kernelsLA[2 * 72 + 3 * 9 + 6] + bc1.w * kernelsLA[2 * 72 + 3 * 9 + 7] + br1.w * kernelsLA[2 * 72 + 3 * 9 + 8] +

		tl2.x * kernelsLA[2 * 72 + 4 * 9 + 0] + tc2.x * kernelsLA[2 * 72 + 4 * 9 + 1] + tr2.x * kernelsLA[2 * 72 + 4 * 9 + 2] +
		ml2.x * kernelsLA[2 * 72 + 4 * 9 + 3] + mc2.x * kernelsLA[2 * 72 + 4 * 9 + 4] + mr2.x * kernelsLA[2 * 72 + 4 * 9 + 5] +
		bl2.x * kernelsLA[2 * 72 + 4 * 9 + 6] + bc2.x * kernelsLA[2 * 72 + 4 * 9 + 7] + br2.x * kernelsLA[2 * 72 + 4 * 9 + 8] +

		tl2.y * kernelsLA[2 * 72 + 5 * 9 + 0] + tc2.y * kernelsLA[2 * 72 + 5 * 9 + 1] + tr2.y * kernelsLA[2 * 72 + 5 * 9 + 2] +
		ml2.y * kernelsLA[2 * 72 + 5 * 9 + 3] + mc2.y * kernelsLA[2 * 72 + 5 * 9 + 4] + mr2.y * kernelsLA[2 * 72 + 5 * 9 + 5] +
		bl2.y * kernelsLA[2 * 72 + 5 * 9 + 6] + bc2.y * kernelsLA[2 * 72 + 5 * 9 + 7] + br2.y * kernelsLA[2 * 72 + 5 * 9 + 8] +

		tl2.z * kernelsLA[2 * 72 + 6 * 9 + 0] + tc2.z * kernelsLA[2 * 72 + 6 * 9 + 1] + tr2.z * kernelsLA[2 * 72 + 6 * 9 + 2] +
		ml2.z * kernelsLA[2 * 72 + 6 * 9 + 3] + mc2.z * kernelsLA[2 * 72 + 6 * 9 + 4] + mr2.z * kernelsLA[2 * 72 + 6 * 9 + 5] +
		bl2.z * kernelsLA[2 * 72 + 6 * 9 + 6] + bc2.z * kernelsLA[2 * 72 + 6 * 9 + 7] + br2.z * kernelsLA[2 * 72 + 6 * 9 + 8] +

		tl2.w * kernelsLA[2 * 72 + 7 * 9 + 0] + tc2.w * kernelsLA[2 * 72 + 7 * 9 + 1] + tr2.w * kernelsLA[2 * 72 + 7 * 9 + 2] +
		ml2.w * kernelsLA[2 * 72 + 7 * 9 + 3] + mc2.w * kernelsLA[2 * 72 + 7 * 9 + 4] + mr2.w * kernelsLA[2 * 72 + 7 * 9 + 5] +
		bl2.w * kernelsLA[2 * 72 + 7 * 9 + 6] + bc2.w * kernelsLA[2 * 72 + 7 * 9 + 7] + br2.w * kernelsLA[2 * 72 + 7 * 9 + 8] + biasLA.z
		,
		tl1.x * kernelsLA[3 * 72 + 0 * 9 + 0] + tc1.x * kernelsLA[3 * 72 + 0 * 9 + 1] + tr1.x * kernelsLA[3 * 72 + 0 * 9 + 2] +
		ml1.x * kernelsLA[3 * 72 + 0 * 9 + 3] + mc1.x * kernelsLA[3 * 72 + 0 * 9 + 4] + mr1.x * kernelsLA[3 * 72 + 0 * 9 + 5] +
		bl1.x * kernelsLA[3 * 72 + 0 * 9 + 6] + bc1.x * kernelsLA[3 * 72 + 0 * 9 + 7] + br1.x * kernelsLA[3 * 72 + 0 * 9 + 8] +

		tl1.y * kernelsLA[3 * 72 + 1 * 9 + 0] + tc1.y * kernelsLA[3 * 72 + 1 * 9 + 1] + tr1.y * kernelsLA[3 * 72 + 1 * 9 + 2] +
		ml1.y * kernelsLA[3 * 72 + 1 * 9 + 3] + mc1.y * kernelsLA[3 * 72 + 1 * 9 + 4] + mr1.y * kernelsLA[3 * 72 + 1 * 9 + 5] +
		bl1.y * kernelsLA[3 * 72 + 1 * 9 + 6] + bc1.y * kernelsLA[3 * 72 + 1 * 9 + 7] + br1.y * kernelsLA[3 * 72 + 1 * 9 + 8] +

		tl1.z * kernelsLA[3 * 72 + 2 * 9 + 0] + tc1.z * kernelsLA[3 * 72 + 2 * 9 + 1] + tr1.z * kernelsLA[3 * 72 + 2 * 9 + 2] +
		ml1.z * kernelsLA[3 * 72 + 2 * 9 + 3] + mc1.z * kernelsLA[3 * 72 + 2 * 9 + 4] + mr1.z * kernelsLA[3 * 72 + 2 * 9 + 5] +
		bl1.z * kernelsLA[3 * 72 + 2 * 9 + 6] + bc1.z * kernelsLA[3 * 72 + 2 * 9 + 7] + br1.z * kernelsLA[3 * 72 + 2 * 9 + 8] +

		tl1.w * kernelsLA[3 * 72 + 3 * 9 + 0] + tc1.w * kernelsLA[3 * 72 + 3 * 9 + 1] + tr1.w * kernelsLA[3 * 72 + 3 * 9 + 2] +
		ml1.w * kernelsLA[3 * 72 + 3 * 9 + 3] + mc1.w * kernelsLA[3 * 72 + 3 * 9 + 4] + mr1.w * kernelsLA[3 * 72 + 3 * 9 + 5] +
		bl1.w * kernelsLA[3 * 72 + 3 * 9 + 6] + bc1.w * kernelsLA[3 * 72 + 3 * 9 + 7] + br1.w * kernelsLA[3 * 72 + 3 * 9 + 8] +

		tl2.x * kernelsLA[3 * 72 + 4 * 9 + 0] + tc2.x * kernelsLA[3 * 72 + 4 * 9 + 1] + tr2.x * kernelsLA[3 * 72 + 4 * 9 + 2] +
		ml2.x * kernelsLA[3 * 72 + 4 * 9 + 3] + mc2.x * kernelsLA[3 * 72 + 4 * 9 + 4] + mr2.x * kernelsLA[3 * 72 + 4 * 9 + 5] +
		bl2.x * kernelsLA[3 * 72 + 4 * 9 + 6] + bc2.x * kernelsLA[3 * 72 + 4 * 9 + 7] + br2.x * kernelsLA[3 * 72 + 4 * 9 + 8] +

		tl2.y * kernelsLA[3 * 72 + 5 * 9 + 0] + tc2.y * kernelsLA[3 * 72 + 5 * 9 + 1] + tr2.y * kernelsLA[3 * 72 + 5 * 9 + 2] +
		ml2.y * kernelsLA[3 * 72 + 5 * 9 + 3] + mc2.y * kernelsLA[3 * 72 + 5 * 9 + 4] + mr2.y * kernelsLA[3 * 72 + 5 * 9 + 5] +
		bl2.y * kernelsLA[3 * 72 + 5 * 9 + 6] + bc2.y * kernelsLA[3 * 72 + 5 * 9 + 7] + br2.y * kernelsLA[3 * 72 + 5 * 9 + 8] +

		tl2.z * kernelsLA[3 * 72 + 6 * 9 + 0] + tc2.z * kernelsLA[3 * 72 + 6 * 9 + 1] + tr2.z * kernelsLA[3 * 72 + 6 * 9 + 2] +
		ml2.z * kernelsLA[3 * 72 + 6 * 9 + 3] + mc2.z * kernelsLA[3 * 72 + 6 * 9 + 4] + mr2.z * kernelsLA[3 * 72 + 6 * 9 + 5] +
		bl2.z * kernelsLA[3 * 72 + 6 * 9 + 6] + bc2.z * kernelsLA[3 * 72 + 6 * 9 + 7] + br2.z * kernelsLA[3 * 72 + 6 * 9 + 8] +

		tl2.w * kernelsLA[3 * 72 + 7 * 9 + 0] + tc2.w * kernelsLA[3 * 72 + 7 * 9 + 1] + tr2.w * kernelsLA[3 * 72 + 7 * 9 + 2] +
		ml2.w * kernelsLA[3 * 72 + 7 * 9 + 3] + mc2.w * kernelsLA[3 * 72 + 7 * 9 + 4] + mr2.w * kernelsLA[3 * 72 + 7 * 9 + 5] +
		bl2.w * kernelsLA[3 * 72 + 7 * 9 + 6] + bc2.w * kernelsLA[3 * 72 + 7 * 9 + 7] + br2.w * kernelsLA[3 * 72 + 7 * 9 + 8] + biasLA.w
	));

	float4 target2 = RELU(float4(
		tl1.x * kernelsLB[0 * 72 + 0 * 9 + 0] + tc1.x * kernelsLB[0 * 72 + 0 * 9 + 1] + tr1.x * kernelsLB[0 * 72 + 0 * 9 + 2] +
		ml1.x * kernelsLB[0 * 72 + 0 * 9 + 3] + mc1.x * kernelsLB[0 * 72 + 0 * 9 + 4] + mr1.x * kernelsLB[0 * 72 + 0 * 9 + 5] +
		bl1.x * kernelsLB[0 * 72 + 0 * 9 + 6] + bc1.x * kernelsLB[0 * 72 + 0 * 9 + 7] + br1.x * kernelsLB[0 * 72 + 0 * 9 + 8] +

		tl1.y * kernelsLB[0 * 72 + 1 * 9 + 0] + tc1.y * kernelsLB[0 * 72 + 1 * 9 + 1] + tr1.y * kernelsLB[0 * 72 + 1 * 9 + 2] +
		ml1.y * kernelsLB[0 * 72 + 1 * 9 + 3] + mc1.y * kernelsLB[0 * 72 + 1 * 9 + 4] + mr1.y * kernelsLB[0 * 72 + 1 * 9 + 5] +
		bl1.y * kernelsLB[0 * 72 + 1 * 9 + 6] + bc1.y * kernelsLB[0 * 72 + 1 * 9 + 7] + br1.y * kernelsLB[0 * 72 + 1 * 9 + 8] +

		tl1.z * kernelsLB[0 * 72 + 2 * 9 + 0] + tc1.z * kernelsLB[0 * 72 + 2 * 9 + 1] + tr1.z * kernelsLB[0 * 72 + 2 * 9 + 2] +
		ml1.z * kernelsLB[0 * 72 + 2 * 9 + 3] + mc1.z * kernelsLB[0 * 72 + 2 * 9 + 4] + mr1.z * kernelsLB[0 * 72 + 2 * 9 + 5] +
		bl1.z * kernelsLB[0 * 72 + 2 * 9 + 6] + bc1.z * kernelsLB[0 * 72 + 2 * 9 + 7] + br1.z * kernelsLB[0 * 72 + 2 * 9 + 8] +

		tl1.w * kernelsLB[0 * 72 + 3 * 9 + 0] + tc1.w * kernelsLB[0 * 72 + 3 * 9 + 1] + tr1.w * kernelsLB[0 * 72 + 3 * 9 + 2] +
		ml1.w * kernelsLB[0 * 72 + 3 * 9 + 3] + mc1.w * kernelsLB[0 * 72 + 3 * 9 + 4] + mr1.w * kernelsLB[0 * 72 + 3 * 9 + 5] +
		bl1.w * kernelsLB[0 * 72 + 3 * 9 + 6] + bc1.w * kernelsLB[0 * 72 + 3 * 9 + 7] + br1.w * kernelsLB[0 * 72 + 3 * 9 + 8] +

		tl2.x * kernelsLB[0 * 72 + 4 * 9 + 0] + tc2.x * kernelsLB[0 * 72 + 4 * 9 + 1] + tr2.x * kernelsLB[0 * 72 + 4 * 9 + 2] +
		ml2.x * kernelsLB[0 * 72 + 4 * 9 + 3] + mc2.x * kernelsLB[0 * 72 + 4 * 9 + 4] + mr2.x * kernelsLB[0 * 72 + 4 * 9 + 5] +
		bl2.x * kernelsLB[0 * 72 + 4 * 9 + 6] + bc2.x * kernelsLB[0 * 72 + 4 * 9 + 7] + br2.x * kernelsLB[0 * 72 + 4 * 9 + 8] +

		tl2.y * kernelsLB[0 * 72 + 5 * 9 + 0] + tc2.y * kernelsLB[0 * 72 + 5 * 9 + 1] + tr2.y * kernelsLB[0 * 72 + 5 * 9 + 2] +
		ml2.y * kernelsLB[0 * 72 + 5 * 9 + 3] + mc2.y * kernelsLB[0 * 72 + 5 * 9 + 4] + mr2.y * kernelsLB[0 * 72 + 5 * 9 + 5] +
		bl2.y * kernelsLB[0 * 72 + 5 * 9 + 6] + bc2.y * kernelsLB[0 * 72 + 5 * 9 + 7] + br2.y * kernelsLB[0 * 72 + 5 * 9 + 8] +

		tl2.z * kernelsLB[0 * 72 + 6 * 9 + 0] + tc2.z * kernelsLB[0 * 72 + 6 * 9 + 1] + tr2.z * kernelsLB[0 * 72 + 6 * 9 + 2] +
		ml2.z * kernelsLB[0 * 72 + 6 * 9 + 3] + mc2.z * kernelsLB[0 * 72 + 6 * 9 + 4] + mr2.z * kernelsLB[0 * 72 + 6 * 9 + 5] +
		bl2.z * kernelsLB[0 * 72 + 6 * 9 + 6] + bc2.z * kernelsLB[0 * 72 + 6 * 9 + 7] + br2.z * kernelsLB[0 * 72 + 6 * 9 + 8] +

		tl2.w * kernelsLB[0 * 72 + 7 * 9 + 0] + tc2.w * kernelsLB[0 * 72 + 7 * 9 + 1] + tr2.w * kernelsLB[0 * 72 + 7 * 9 + 2] +
		ml2.w * kernelsLB[0 * 72 + 7 * 9 + 3] + mc2.w * kernelsLB[0 * 72 + 7 * 9 + 4] + mr2.w * kernelsLB[0 * 72 + 7 * 9 + 5] +
		bl2.w * kernelsLB[0 * 72 + 7 * 9 + 6] + bc2.w * kernelsLB[0 * 72 + 7 * 9 + 7] + br2.w * kernelsLB[0 * 72 + 7 * 9 + 8] + biasLB.x
		,
		tl1.x * kernelsLB[1 * 72 + 0 * 9 + 0] + tc1.x * kernelsLB[1 * 72 + 0 * 9 + 1] + tr1.x * kernelsLB[1 * 72 + 0 * 9 + 2] +
		ml1.x * kernelsLB[1 * 72 + 0 * 9 + 3] + mc1.x * kernelsLB[1 * 72 + 0 * 9 + 4] + mr1.x * kernelsLB[1 * 72 + 0 * 9 + 5] +
		bl1.x * kernelsLB[1 * 72 + 0 * 9 + 6] + bc1.x * kernelsLB[1 * 72 + 0 * 9 + 7] + br1.x * kernelsLB[1 * 72 + 0 * 9 + 8] +

		tl1.y * kernelsLB[1 * 72 + 1 * 9 + 0] + tc1.y * kernelsLB[1 * 72 + 1 * 9 + 1] + tr1.y * kernelsLB[1 * 72 + 1 * 9 + 2] +
		ml1.y * kernelsLB[1 * 72 + 1 * 9 + 3] + mc1.y * kernelsLB[1 * 72 + 1 * 9 + 4] + mr1.y * kernelsLB[1 * 72 + 1 * 9 + 5] +
		bl1.y * kernelsLB[1 * 72 + 1 * 9 + 6] + bc1.y * kernelsLB[1 * 72 + 1 * 9 + 7] + br1.y * kernelsLB[1 * 72 + 1 * 9 + 8] +

		tl1.z * kernelsLB[1 * 72 + 2 * 9 + 0] + tc1.z * kernelsLB[1 * 72 + 2 * 9 + 1] + tr1.z * kernelsLB[1 * 72 + 2 * 9 + 2] +
		ml1.z * kernelsLB[1 * 72 + 2 * 9 + 3] + mc1.z * kernelsLB[1 * 72 + 2 * 9 + 4] + mr1.z * kernelsLB[1 * 72 + 2 * 9 + 5] +
		bl1.z * kernelsLB[1 * 72 + 2 * 9 + 6] + bc1.z * kernelsLB[1 * 72 + 2 * 9 + 7] + br1.z * kernelsLB[1 * 72 + 2 * 9 + 8] +

		tl1.w * kernelsLB[1 * 72 + 3 * 9 + 0] + tc1.w * kernelsLB[1 * 72 + 3 * 9 + 1] + tr1.w * kernelsLB[1 * 72 + 3 * 9 + 2] +
		ml1.w * kernelsLB[1 * 72 + 3 * 9 + 3] + mc1.w * kernelsLB[1 * 72 + 3 * 9 + 4] + mr1.w * kernelsLB[1 * 72 + 3 * 9 + 5] +
		bl1.w * kernelsLB[1 * 72 + 3 * 9 + 6] + bc1.w * kernelsLB[1 * 72 + 3 * 9 + 7] + br1.w * kernelsLB[1 * 72 + 3 * 9 + 8] +

		tl2.x * kernelsLB[1 * 72 + 4 * 9 + 0] + tc2.x * kernelsLB[1 * 72 + 4 * 9 + 1] + tr2.x * kernelsLB[1 * 72 + 4 * 9 + 2] +
		ml2.x * kernelsLB[1 * 72 + 4 * 9 + 3] + mc2.x * kernelsLB[1 * 72 + 4 * 9 + 4] + mr2.x * kernelsLB[1 * 72 + 4 * 9 + 5] +
		bl2.x * kernelsLB[1 * 72 + 4 * 9 + 6] + bc2.x * kernelsLB[1 * 72 + 4 * 9 + 7] + br2.x * kernelsLB[1 * 72 + 4 * 9 + 8] +

		tl2.y * kernelsLB[1 * 72 + 5 * 9 + 0] + tc2.y * kernelsLB[1 * 72 + 5 * 9 + 1] + tr2.y * kernelsLB[1 * 72 + 5 * 9 + 2] +
		ml2.y * kernelsLB[1 * 72 + 5 * 9 + 3] + mc2.y * kernelsLB[1 * 72 + 5 * 9 + 4] + mr2.y * kernelsLB[1 * 72 + 5 * 9 + 5] +
		bl2.y * kernelsLB[1 * 72 + 5 * 9 + 6] + bc2.y * kernelsLB[1 * 72 + 5 * 9 + 7] + br2.y * kernelsLB[1 * 72 + 5 * 9 + 8] +

		tl2.z * kernelsLB[1 * 72 + 6 * 9 + 0] + tc2.z * kernelsLB[1 * 72 + 6 * 9 + 1] + tr2.z * kernelsLB[1 * 72 + 6 * 9 + 2] +
		ml2.z * kernelsLB[1 * 72 + 6 * 9 + 3] + mc2.z * kernelsLB[1 * 72 + 6 * 9 + 4] + mr2.z * kernelsLB[1 * 72 + 6 * 9 + 5] +
		bl2.z * kernelsLB[1 * 72 + 6 * 9 + 6] + bc2.z * kernelsLB[1 * 72 + 6 * 9 + 7] + br2.z * kernelsLB[1 * 72 + 6 * 9 + 8] +

		tl2.w * kernelsLB[1 * 72 + 7 * 9 + 0] + tc2.w * kernelsLB[1 * 72 + 7 * 9 + 1] + tr2.w * kernelsLB[1 * 72 + 7 * 9 + 2] +
		ml2.w * kernelsLB[1 * 72 + 7 * 9 + 3] + mc2.w * kernelsLB[1 * 72 + 7 * 9 + 4] + mr2.w * kernelsLB[1 * 72 + 7 * 9 + 5] +
		bl2.w * kernelsLB[1 * 72 + 7 * 9 + 6] + bc2.w * kernelsLB[1 * 72 + 7 * 9 + 7] + br2.w * kernelsLB[1 * 72 + 7 * 9 + 8] + biasLB.y
		,
		tl1.x * kernelsLB[2 * 72 + 0 * 9 + 0] + tc1.x * kernelsLB[2 * 72 + 0 * 9 + 1] + tr1.x * kernelsLB[2 * 72 + 0 * 9 + 2] +
		ml1.x * kernelsLB[2 * 72 + 0 * 9 + 3] + mc1.x * kernelsLB[2 * 72 + 0 * 9 + 4] + mr1.x * kernelsLB[2 * 72 + 0 * 9 + 5] +
		bl1.x * kernelsLB[2 * 72 + 0 * 9 + 6] + bc1.x * kernelsLB[2 * 72 + 0 * 9 + 7] + br1.x * kernelsLB[2 * 72 + 0 * 9 + 8] +

		tl1.y * kernelsLB[2 * 72 + 1 * 9 + 0] + tc1.y * kernelsLB[2 * 72 + 1 * 9 + 1] + tr1.y * kernelsLB[2 * 72 + 1 * 9 + 2] +
		ml1.y * kernelsLB[2 * 72 + 1 * 9 + 3] + mc1.y * kernelsLB[2 * 72 + 1 * 9 + 4] + mr1.y * kernelsLB[2 * 72 + 1 * 9 + 5] +
		bl1.y * kernelsLB[2 * 72 + 1 * 9 + 6] + bc1.y * kernelsLB[2 * 72 + 1 * 9 + 7] + br1.y * kernelsLB[2 * 72 + 1 * 9 + 8] +

		tl1.z * kernelsLB[2 * 72 + 2 * 9 + 0] + tc1.z * kernelsLB[2 * 72 + 2 * 9 + 1] + tr1.z * kernelsLB[2 * 72 + 2 * 9 + 2] +
		ml1.z * kernelsLB[2 * 72 + 2 * 9 + 3] + mc1.z * kernelsLB[2 * 72 + 2 * 9 + 4] + mr1.z * kernelsLB[2 * 72 + 2 * 9 + 5] +
		bl1.z * kernelsLB[2 * 72 + 2 * 9 + 6] + bc1.z * kernelsLB[2 * 72 + 2 * 9 + 7] + br1.z * kernelsLB[2 * 72 + 2 * 9 + 8] +

		tl1.w * kernelsLB[2 * 72 + 3 * 9 + 0] + tc1.w * kernelsLB[2 * 72 + 3 * 9 + 1] + tr1.w * kernelsLB[2 * 72 + 3 * 9 + 2] +
		ml1.w * kernelsLB[2 * 72 + 3 * 9 + 3] + mc1.w * kernelsLB[2 * 72 + 3 * 9 + 4] + mr1.w * kernelsLB[2 * 72 + 3 * 9 + 5] +
		bl1.w * kernelsLB[2 * 72 + 3 * 9 + 6] + bc1.w * kernelsLB[2 * 72 + 3 * 9 + 7] + br1.w * kernelsLB[2 * 72 + 3 * 9 + 8] +

		tl2.x * kernelsLB[2 * 72 + 4 * 9 + 0] + tc2.x * kernelsLB[2 * 72 + 4 * 9 + 1] + tr2.x * kernelsLB[2 * 72 + 4 * 9 + 2] +
		ml2.x * kernelsLB[2 * 72 + 4 * 9 + 3] + mc2.x * kernelsLB[2 * 72 + 4 * 9 + 4] + mr2.x * kernelsLB[2 * 72 + 4 * 9 + 5] +
		bl2.x * kernelsLB[2 * 72 + 4 * 9 + 6] + bc2.x * kernelsLB[2 * 72 + 4 * 9 + 7] + br2.x * kernelsLB[2 * 72 + 4 * 9 + 8] +

		tl2.y * kernelsLB[2 * 72 + 5 * 9 + 0] + tc2.y * kernelsLB[2 * 72 + 5 * 9 + 1] + tr2.y * kernelsLB[2 * 72 + 5 * 9 + 2] +
		ml2.y * kernelsLB[2 * 72 + 5 * 9 + 3] + mc2.y * kernelsLB[2 * 72 + 5 * 9 + 4] + mr2.y * kernelsLB[2 * 72 + 5 * 9 + 5] +
		bl2.y * kernelsLB[2 * 72 + 5 * 9 + 6] + bc2.y * kernelsLB[2 * 72 + 5 * 9 + 7] + br2.y * kernelsLB[2 * 72 + 5 * 9 + 8] +

		tl2.z * kernelsLB[2 * 72 + 6 * 9 + 0] + tc2.z * kernelsLB[2 * 72 + 6 * 9 + 1] + tr2.z * kernelsLB[2 * 72 + 6 * 9 + 2] +
		ml2.z * kernelsLB[2 * 72 + 6 * 9 + 3] + mc2.z * kernelsLB[2 * 72 + 6 * 9 + 4] + mr2.z * kernelsLB[2 * 72 + 6 * 9 + 5] +
		bl2.z * kernelsLB[2 * 72 + 6 * 9 + 6] + bc2.z * kernelsLB[2 * 72 + 6 * 9 + 7] + br2.z * kernelsLB[2 * 72 + 6 * 9 + 8] +

		tl2.w * kernelsLB[2 * 72 + 7 * 9 + 0] + tc2.w * kernelsLB[2 * 72 + 7 * 9 + 1] + tr2.w * kernelsLB[2 * 72 + 7 * 9 + 2] +
		ml2.w * kernelsLB[2 * 72 + 7 * 9 + 3] + mc2.w * kernelsLB[2 * 72 + 7 * 9 + 4] + mr2.w * kernelsLB[2 * 72 + 7 * 9 + 5] +
		bl2.w * kernelsLB[2 * 72 + 7 * 9 + 6] + bc2.w * kernelsLB[2 * 72 + 7 * 9 + 7] + br2.w * kernelsLB[2 * 72 + 7 * 9 + 8] + biasLB.z
		,
		tl1.x * kernelsLB[3 * 72 + 0 * 9 + 0] + tc1.x * kernelsLB[3 * 72 + 0 * 9 + 1] + tr1.x * kernelsLB[3 * 72 + 0 * 9 + 2] +
		ml1.x * kernelsLB[3 * 72 + 0 * 9 + 3] + mc1.x * kernelsLB[3 * 72 + 0 * 9 + 4] + mr1.x * kernelsLB[3 * 72 + 0 * 9 + 5] +
		bl1.x * kernelsLB[3 * 72 + 0 * 9 + 6] + bc1.x * kernelsLB[3 * 72 + 0 * 9 + 7] + br1.x * kernelsLB[3 * 72 + 0 * 9 + 8] +

		tl1.y * kernelsLB[3 * 72 + 1 * 9 + 0] + tc1.y * kernelsLB[3 * 72 + 1 * 9 + 1] + tr1.y * kernelsLB[3 * 72 + 1 * 9 + 2] +
		ml1.y * kernelsLB[3 * 72 + 1 * 9 + 3] + mc1.y * kernelsLB[3 * 72 + 1 * 9 + 4] + mr1.y * kernelsLB[3 * 72 + 1 * 9 + 5] +
		bl1.y * kernelsLB[3 * 72 + 1 * 9 + 6] + bc1.y * kernelsLB[3 * 72 + 1 * 9 + 7] + br1.y * kernelsLB[3 * 72 + 1 * 9 + 8] +

		tl1.z * kernelsLB[3 * 72 + 2 * 9 + 0] + tc1.z * kernelsLB[3 * 72 + 2 * 9 + 1] + tr1.z * kernelsLB[3 * 72 + 2 * 9 + 2] +
		ml1.z * kernelsLB[3 * 72 + 2 * 9 + 3] + mc1.z * kernelsLB[3 * 72 + 2 * 9 + 4] + mr1.z * kernelsLB[3 * 72 + 2 * 9 + 5] +
		bl1.z * kernelsLB[3 * 72 + 2 * 9 + 6] + bc1.z * kernelsLB[3 * 72 + 2 * 9 + 7] + br1.z * kernelsLB[3 * 72 + 2 * 9 + 8] +

		tl1.w * kernelsLB[3 * 72 + 3 * 9 + 0] + tc1.w * kernelsLB[3 * 72 + 3 * 9 + 1] + tr1.w * kernelsLB[3 * 72 + 3 * 9 + 2] +
		ml1.w * kernelsLB[3 * 72 + 3 * 9 + 3] + mc1.w * kernelsLB[3 * 72 + 3 * 9 + 4] + mr1.w * kernelsLB[3 * 72 + 3 * 9 + 5] +
		bl1.w * kernelsLB[3 * 72 + 3 * 9 + 6] + bc1.w * kernelsLB[3 * 72 + 3 * 9 + 7] + br1.w * kernelsLB[3 * 72 + 3 * 9 + 8] +

		tl2.x * kernelsLB[3 * 72 + 4 * 9 + 0] + tc2.x * kernelsLB[3 * 72 + 4 * 9 + 1] + tr2.x * kernelsLB[3 * 72 + 4 * 9 + 2] +
		ml2.x * kernelsLB[3 * 72 + 4 * 9 + 3] + mc2.x * kernelsLB[3 * 72 + 4 * 9 + 4] + mr2.x * kernelsLB[3 * 72 + 4 * 9 + 5] +
		bl2.x * kernelsLB[3 * 72 + 4 * 9 + 6] + bc2.x * kernelsLB[3 * 72 + 4 * 9 + 7] + br2.x * kernelsLB[3 * 72 + 4 * 9 + 8] +

		tl2.y * kernelsLB[3 * 72 + 5 * 9 + 0] + tc2.y * kernelsLB[3 * 72 + 5 * 9 + 1] + tr2.y * kernelsLB[3 * 72 + 5 * 9 + 2] +
		ml2.y * kernelsLB[3 * 72 + 5 * 9 + 3] + mc2.y * kernelsLB[3 * 72 + 5 * 9 + 4] + mr2.y * kernelsLB[3 * 72 + 5 * 9 + 5] +
		bl2.y * kernelsLB[3 * 72 + 5 * 9 + 6] + bc2.y * kernelsLB[3 * 72 + 5 * 9 + 7] + br2.y * kernelsLB[3 * 72 + 5 * 9 + 8] +

		tl2.z * kernelsLB[3 * 72 + 6 * 9 + 0] + tc2.z * kernelsLB[3 * 72 + 6 * 9 + 1] + tr2.z * kernelsLB[3 * 72 + 6 * 9 + 2] +
		ml2.z * kernelsLB[3 * 72 + 6 * 9 + 3] + mc2.z * kernelsLB[3 * 72 + 6 * 9 + 4] + mr2.z * kernelsLB[3 * 72 + 6 * 9 + 5] +
		bl2.z * kernelsLB[3 * 72 + 6 * 9 + 6] + bc2.z * kernelsLB[3 * 72 + 6 * 9 + 7] + br2.z * kernelsLB[3 * 72 + 6 * 9 + 8] +

		tl2.w * kernelsLB[3 * 72 + 7 * 9 + 0] + tc2.w * kernelsLB[3 * 72 + 7 * 9 + 1] + tr2.w * kernelsLB[3 * 72 + 7 * 9 + 2] +
		ml2.w * kernelsLB[3 * 72 + 7 * 9 + 3] + mc2.w * kernelsLB[3 * 72 + 7 * 9 + 4] + mr2.w * kernelsLB[3 * 72 + 7 * 9 + 5] +
		bl2.w * kernelsLB[3 * 72 + 7 * 9 + 6] + bc2.w * kernelsLB[3 * 72 + 7 * 9 + 7] + br2.w * kernelsLB[3 * 72 + 7 * 9 + 8] + biasLB.w
	));

	float2 originUV = mul(rgb2uv, INPUT.SampleLevel(sam, pos, 0).rgb);

	[unroll]
	for (uint i = 0; i <= 1; ++i) {
		[unroll]
		for (uint j = 0; j <= 1; ++j) {
			uint2 destPos = gxy + uint2(i, j);

			if (i != 0 && j != 0) {
				if (!CheckViewport(destPos)) {
					continue;
				}
			}

			uint index = j * 2 + i;
			float luma = clamp(
				target1.x * kernelsL10[0 + index] +
				target1.y * kernelsL10[4 + index] +
				target1.z * kernelsL10[8 + index] +
				target1.w * kernelsL10[12 + index] +
				target2.x * kernelsL10[16 + index] +
				target2.y * kernelsL10[20 + index] +
				target2.z * kernelsL10[24 + index] +
				target2.w * kernelsL10[28 + index], 0.0f, 1.0f);

			WriteToOutput(destPos, mul(yuv2rgb, float3(luma, originUV)));
		}
	}
}
