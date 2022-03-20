// Anime4K_Denoise_Bilateral_Mean
// 移植自 https://github.com/bloc97/Anime4K/blob/master/glsl/Denoise/Anime4K_Denoise_Bilateral_Mean.glsl


//!MAGPIE EFFECT
//!VERSION 2
//!OUTPUT_WIDTH INPUT_WIDTH
//!OUTPUT_HEIGHT INPUT_HEIGHT


//!PARAMETER
//!MIN 1e-5
//!DEFAULT 0.1
float intensitySigma;

//!TEXTURE
Texture2D INPUT;

//!SAMPLER
//!FILTER POINT
SamplerState sam;


//!PASS 1
//!IN INPUT
//!BLOCK_SIZE 16
//!NUM_THREADS 64

#define INTENSITY_SIGMA intensitySigma //Intensity window size, higher is stronger denoise, must be a positive real number
#define SPATIAL_SIGMA 1.0 //Spatial window size, higher is stronger denoise, must be a positive real number.

#define INTENSITY_POWER_CURVE 1.0 //Intensity window power curve. Setting it to 0 will make the intensity window treat all intensities equally, while increasing it will make the window narrower in darker intensities and wider in brighter intensities.

#define KERNELSIZE (max(uint(ceil(SPATIAL_SIGMA * 2.0)), 1) * 2 + 1) //Kernel size, must be an positive odd integer.
#define KERNELHALFSIZE (uint(KERNELSIZE/2)) //Half of the kernel size without remainder. Must be equal to trunc(KERNELSIZE/2).
#define KERNELLEN (KERNELSIZE * KERNELSIZE) //Total area of kernel. Must be equal to KERNELSIZE * KERNELSIZE.


float3 gaussian_vec(float3 x, float3 rcpS, float3 m) {
	float3 scaled = (x - m) * rcpS;
	return exp(-0.5 * scaled * scaled);
}

float gaussian(float x, float rcpS, float m) {
	float scaled = (x - m) * rcpS;
	return exp(-0.5 * scaled * scaled);
}


void Pass1(uint2 blockStart, uint3 threadId) {
	uint2 gxy = (Rmp8x8(threadId.x) << 1) + blockStart;
	if (!CheckViewport(gxy)) {
		return;
	}

	float2 inputPt = GetInputPt();
	uint i, j;

	float3 src[KERNELSIZE + 1][KERNELSIZE + 1];
	[unroll]
	for (i = 0; i <= KERNELSIZE - 1; i += 2) {
		[unroll]
		for (j = 0; j <= KERNELSIZE - 1; j += 2) {
			float2 tpos = (gxy + int2(i, j) - KERNELHALFSIZE + 1) * inputPt;
			const float4 sr = INPUT.GatherRed(sam, tpos);
			const float4 sg = INPUT.GatherGreen(sam, tpos);
			const float4 sb = INPUT.GatherBlue(sam, tpos);

			// w z
			// x y
			src[i][j] = float3(sr.w, sg.w, sb.w);
			src[i][j + 1] = float3(sr.x, sg.x, sb.x);
			src[i + 1][j] = float3(sr.z, sg.z, sb.z);
			src[i + 1][j + 1] = float3(sr.y, sg.y, sb.y);
		}
	}

	float len[KERNELSIZE][KERNELSIZE];
	[unroll]
	for (i = 0; i < KERNELSIZE; ++i) {
		[unroll]
		for (j = 0; j < KERNELSIZE; ++j) {
			len[i][j] = length(float2((int)i - KERNELHALFSIZE, (int)j - KERNELHALFSIZE));
		}
	}

	[unroll]
	for (i = 0; i <= 1; ++i) {
		[unroll]
		for (j = 0; j <= 1; ++j) {
			uint2 destPos = gxy + uint2(i, j);

			if (i != 0 && j != 0) {
				if (!CheckViewport(gxy)) {
					continue;
				}
			}

			float3 sum = 0;
			float3 n = 0;

			float3 vc = src[KERNELHALFSIZE + i][KERNELHALFSIZE + j].rgb;

			float3 rcpIs = rcp(pow(vc + 0.0001, INTENSITY_POWER_CURVE) * INTENSITY_SIGMA);
			float rcpSs = rcp(SPATIAL_SIGMA);

			[unroll]
			for (uint k = 0; k < KERNELSIZE; ++k) {
				[unroll]
				for (uint m = 0; m < KERNELSIZE; ++m) {
					float3 v = src[k + i][m + j];
					float3 d = gaussian_vec(v, rcpIs, vc) * gaussian(len[k][m], rcpSs, 0);
					sum += d * v;
					n += d;
				}
			}

			WriteToOutput(destPos, sum / n);
		}
	}
}
