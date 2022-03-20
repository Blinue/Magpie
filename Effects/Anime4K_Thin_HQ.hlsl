// Anime4K_Thin_HQ
// 移植自 https://github.com/bloc97/Anime4K/blob/master/glsl/Experimental-Effects/Anime4K_Thin_HQ.glsl

//!MAGPIE EFFECT
//!VERSION 2
//!OUTPUT_WIDTH INPUT_WIDTH
//!OUTPUT_HEIGHT INPUT_HEIGHT


//!PARAMETER
//!DEFAULT 0.6
//!MIN 1e-5

// Strength of warping for each iteration
float strength;

//!PARAMETER
//!DEFAULT 1
//!MIN 1

// Number of iterations for the forwards solver, decreasing strength and increasing iterations improves quality at the cost of speed.
int iterations;

//!TEXTURE
Texture2D INPUT;

//!TEXTURE
//!WIDTH INPUT_WIDTH
//!HEIGHT INPUT_HEIGHT
//!FORMAT R16G16_FLOAT
Texture2D tex1;

//!TEXTURE
//!WIDTH INPUT_WIDTH
//!HEIGHT INPUT_HEIGHT
//!FORMAT R16G16_FLOAT
Texture2D tex2;

//!SAMPLER
//!FILTER POINT
SamplerState sam;

//!SAMPLER
//!FILTER LINEAR
SamplerState sam1;

//!COMMON

#ifdef MP_INLINE_PARAMS
#pragma warning(disable: 3557) // X3557: loop only executes for 1 iteration(s), forcing loop to unroll
#endif

//!PASS 1
//!IN INPUT
//!OUT tex2
//!BLOCK_SIZE 16
//!NUM_THREADS 64

float GetLuma(float3 rgb) {
	return dot(float3(0.299, 0.587, 0.114), rgb);
}

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
			const float4 sr = INPUT.GatherRed(sam, tpos);
			const float4 sg = INPUT.GatherGreen(sam, tpos);
			const float4 sb = INPUT.GatherBlue(sam, tpos);

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

			float xgrad = (-src[i - 1][j - 1] + src[i + 1][j - 1] - src[i - 1][j] + src[i + 1][j] - src[i - 1][j] + src[i + 1][j] - src[i - 1][j + 1] + src[i + 1][j + 1]) / 8.0f;
			float ygrad = (-src[i - 1][j - 1] - src[i][j - 1] - src[i][j - 1] - src[i + 1][j - 1] + src[i - 1][j + 1] + src[i][j + 1] + src[i][j + 1] + src[i + 1][j + 1]) / 8.0f;

			// Computes the luminance's gradient
			float norm = sqrt(xgrad * xgrad + ygrad * ygrad);
			tex2[destPos] = float2(pow(norm, 0.7), 0);
		}
	}
}


//!PASS 2
//!IN tex2
//!OUT tex1
//!BLOCK_SIZE 16
//!NUM_THREADS 64

float gaussian(float x, float s, float m) {
	float scaled = (x - m) / s;
	return exp(-0.5 * scaled * scaled);
}

void Pass2(uint2 blockStart, uint3 threadId) {
	const uint2 gxy = (Rmp8x8(threadId.x) << 1) + blockStart;
	const uint2 inputSize = GetInputSize();
	if (gxy.x >= inputSize.x || gxy.y >= inputSize.y) {
		return;
	}
	const float2 inputPt = GetInputPt();

	// Spatial window size, must be a positive real number.
	const float SPATIAL_SIGMA = 2.0f * GetInputSize().y / 1080.0f;
	// Kernel size, must be an positive odd integer.
	const uint KERNELSIZE = max(uint(ceil(SPATIAL_SIGMA * 2.0)), 1) * 2 + 1;
	//Half of the kernel size without remainder. Must be equal to trunc(KERNELSIZE/2).
	const uint KERNELHALFSIZE = KERNELSIZE / 2;

	[unroll]
	for (uint i = 0; i <= 1; ++i) {
		[unroll]
		for (uint j = 0; j <= 1; ++j) {
			const uint2 destPos = gxy + uint2(i, j);

			if (i != 0 && j != 0) {
				if (destPos.x >= inputSize.x || destPos.y >= inputSize.y) {
					continue;
				}
			}

			const float2 pos = (destPos + 0.5f) * inputPt;

			float g = 0.0;
			float gn = 0.0;

			for (uint k = 0; k < KERNELSIZE; ++k) {
				int di = (int)k - (int)KERNELHALFSIZE;
				float gf = gaussian(di, SPATIAL_SIGMA, 0.0);
				g = g + tex2.SampleLevel(sam, pos + float2(di * inputPt.x, 0.0), 0).x * gf;
				gn = gn + gf;
			}

			tex1[destPos] = float2(g / gn, 0);
		}
	}
}


//!PASS 3
//!IN tex1
//!OUT tex2
//!BLOCK_SIZE 16
//!NUM_THREADS 64

float gaussian(float x, float s, float m) {
	float scaled = (x - m) / s;
	return exp(-0.5 * scaled * scaled);
}

void Pass3(uint2 blockStart, uint3 threadId) {
	const uint2 gxy = (Rmp8x8(threadId.x) << 1) + blockStart;
	const uint2 inputSize = GetInputSize();
	if (gxy.x >= inputSize.x || gxy.y >= inputSize.y) {
		return;
	}
	const float2 inputPt = GetInputPt();

	// Spatial window size, must be a positive real number.
	const float SPATIAL_SIGMA = 2.0f * GetInputSize().y / 1080.0f;
	// Kernel size, must be an positive odd integer.
	const uint KERNELSIZE = max(uint(ceil(SPATIAL_SIGMA * 2.0)), 1) * 2 + 1;
	//Half of the kernel size without remainder. Must be equal to trunc(KERNELSIZE/2).
	const uint KERNELHALFSIZE = KERNELSIZE / 2;

	[unroll]
	for (uint i = 0; i <= 1; ++i) {
		[unroll]
		for (uint j = 0; j <= 1; ++j) {
			const uint2 destPos = gxy + uint2(i, j);

			if (i != 0 && j != 0) {
				if (destPos.x >= inputSize.x || destPos.y >= inputSize.y) {
					continue;
				}
			}

			const float2 pos = (destPos + 0.5f) * inputPt;

			float g = 0.0;
			float gn = 0.0;

			for (uint k = 0; k < KERNELSIZE; ++k) {
				int di = (int)k - (int)KERNELHALFSIZE;
				float gf = gaussian(di, SPATIAL_SIGMA, 0.0);

				g = g + tex1.SampleLevel(sam, pos + float2(0, di * inputPt.y), 0).x * gf;
				gn = gn + gf;
			}

			tex2[destPos] = float2(g / gn, 0);
		}
	}
}

//!PASS 4
//!IN tex2
//!OUT tex1
//!BLOCK_SIZE 16
//!NUM_THREADS 64

void Pass4(uint2 blockStart, uint3 threadId) {
	const uint2 gxy = (Rmp8x8(threadId.x) << 1) + blockStart;
	const uint2 inputSize = GetInputSize();
	if (gxy.x >= inputSize.x || gxy.y >= inputSize.y) {
		return;
	}
	const float2 inputPt = GetInputPt();

	uint i, j;

	float src[4][4];
	[unroll]
	for (i = 0; i <= 2; i += 2) {
		[unroll]
		for (j = 0; j <= 2; j += 2) {
			float2 tpos = (gxy + uint2(i, j)) * inputPt;
			const float4 sr = tex2.GatherRed(sam, tpos);

			// w z
			// x y
			src[i][j] = sr.w;
			src[i][j + 1] = sr.x;
			src[i + 1][j] = sr.z;
			src[i + 1][j + 1] = sr.y;
		}
	}

	[unroll]
	for (i = 1; i <= 2; ++i) {
		[unroll]
		for (j = 1; j <= 2; ++j) {
			uint2 destPos = gxy + uint2(i - 1, j - 1);

			float xgrad = -src[i - 1][j - 1] + src[i + 1][j - 1] - src[i - 1][j] + src[i + 1][j] - src[i - 1][j] + src[i + 1][j] - src[i - 1][j + 1] + src[i + 1][j + 1];
			float ygrad = -src[i - 1][j - 1] - src[i][j - 1] - src[i][j - 1] - src[i + 1][j - 1] + src[i - 1][j + 1] + src[i][j + 1] + src[i][j + 1] + src[i + 1][j + 1];

			// Computes the luminance's gradient
			tex1[destPos] = float2(xgrad, ygrad) / 8.0f;
		}
	}
}


//!PASS 5
//!IN tex1, INPUT 
//!BLOCK_SIZE 16
//!NUM_THREADS 64

void Pass5(uint2 blockStart, uint3 threadId) {
	const uint2 gxy = (Rmp8x8(threadId.x) << 1) + blockStart;
	const uint2 inputSize = GetInputSize();
	if (!CheckViewport(gxy)) {
		return;
	}

	const float2 inputPt = GetInputPt();
	const float relstr = GetInputSize().y / 1080.0f * strength;

	[unroll]
	for (uint i = 0; i <= 1; ++i) {
		[unroll]
		for (uint j = 0; j <= 1; ++j) {
			const uint2 destPos = gxy + uint2(i, j);

			if (i != 0 && j != 0) {
				if (!CheckViewport(destPos)) {
					continue;
				}
			}

			float2 pos = (destPos + 0.5f) * inputPt;

			for (int i = 0; i < iterations; ++i) {
				float2 dn = tex1.SampleLevel(sam1, pos, 0).xy;
				float2 dd = (dn / (length(dn) + 0.01f)) * inputPt * relstr; //Quasi-normalization for large vectors, avoids divide by zero
				pos -= dd;
			}

			WriteToOutput(destPos, INPUT.SampleLevel(sam1, pos, 0).rgb);
		}
	}
}
