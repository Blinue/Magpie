// Anime4K_Denoise_Bilateral_Mode
// 移植自 https://github.com/bloc97/Anime4K/blob/master/glsl/Denoise/Anime4K_Denoise_Bilateral_Mode.glsl


//!MAGPIE EFFECT
//!VERSION 2
//!OUTPUT_WIDTH INPUT_WIDTH
//!OUTPUT_HEIGHT INPUT_HEIGHT


//!TEXTURE
Texture2D INPUT;

//!PARAMETER
//!MIN 1e-5
//!DEFAULT 0.1
float intensitySigma;

//!SAMPLER
//!FILTER POINT
SamplerState sam;


//!PASS 1
//!IN INPUT
//!BLOCK_SIZE 16
//!NUM_THREADS 64

#define INTENSITY_SIGMA intensitySigma //Intensity window size, higher is stronger denoise, must be a positive real number
#define SPATIAL_SIGMA 1.0 //Spatial window size, higher is stronger denoise, must be a positive real number.
#define HISTOGRAM_REGULARIZATION 0.2 //Histogram regularization window size, higher values approximate a bilateral "closest-to-mean" filter.

#define INTENSITY_POWER_CURVE 1.0 //Intensity window power curve. Setting it to 0 will make the intensity window treat all intensities equally, while increasing it will make the window narrower in darker intensities and wider in brighter intensities.

#define KERNELSIZE uint(max(uint(SPATIAL_SIGMA), 1) * 2 + 1) //Kernel size, must be an positive odd integer.
#define KERNELHALFSIZE (int(KERNELSIZE/2)) //Half of the kernel size without remainder. Must be equal to trunc(KERNELSIZE/2).
#define KERNELLEN (KERNELSIZE * KERNELSIZE) //Total area of kernel. Must be equal to KERNELSIZE * KERNELSIZE.

#define GETOFFSET(i) int2(int(i % KERNELSIZE) - KERNELHALFSIZE, int(i / KERNELSIZE) - KERNELHALFSIZE)

float get_luma(float3 rgba) {
	return dot(float3(0.299, 0.587, 0.114), rgba);
}

float gaussian(float x, float s, float m) {
	float scaled = (x - m) / s;
	return exp(-0.5 * scaled * scaled);
}

void Pass1(uint2 blockStart, uint3 threadId) {
	uint2 gxy = (Rmp8x8(threadId.x) << 1) + blockStart;
	if (!CheckViewport(gxy)) {
		return;
	}

	float2 inputPt = GetInputPt();
	uint i, j, k, m;

	float4 src[KERNELSIZE + 1][KERNELSIZE + 1];
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
			src[i][j] = float4(sr.w, sg.w, sb.w, get_luma(float3(sr.w, sg.w, sb.w)));
			src[i][j + 1] = float4(sr.x, sg.x, sb.x, get_luma(float3(sr.x, sg.x, sb.x)));
			src[i + 1][j] = float4(sr.z, sg.z, sb.z, get_luma(float3(sr.z, sg.z, sb.z)));
			src[i + 1][j + 1] = float4(sr.y, sg.y, sb.y, get_luma(float3(sr.y, sg.y, sb.y)));
		}
	}

	[unroll]
	for (i = 0; i <= 1; ++i) {
		[unroll]
		for (j = 0; j <= 1; ++j) {
			const uint2 destPos = gxy + uint2(i, j);

			if (i != 0 && j != 0) {
				if (!CheckViewport(gxy)) {
					continue;
				}
			}

			float3 histogram_v[KERNELLEN];
			float histogram_l[KERNELLEN];
			float histogram_w[KERNELLEN];
			float histogram_wn[KERNELLEN];

			float vc = src[KERNELHALFSIZE + i][KERNELHALFSIZE + j].a;

			float is = pow(vc + 0.0001, INTENSITY_POWER_CURVE) * INTENSITY_SIGMA;
			float ss = SPATIAL_SIGMA;

			[unroll]
			for (k = 0; k < KERNELLEN; k++) {
				const int2 ipos = GETOFFSET(k);
				const uint2 idx = uint2(i, j) + ipos.yx + KERNELHALFSIZE;
				histogram_v[k] = src[idx.x][idx.y].rgb;
				histogram_l[k] = src[idx.x][idx.y].a;
				histogram_w[k] = gaussian(histogram_l[k], is, vc) * gaussian(length(ipos), ss, 0.0);
				histogram_wn[k] = 0.0;
			}

			[unroll]
			for (k = 0; k < KERNELLEN; k++) {
				histogram_wn[k] += gaussian(0.0, HISTOGRAM_REGULARIZATION, 0.0) * histogram_w[k];
				[unroll]
				for (uint m = (k + 1); m < KERNELLEN; m++) {
					float d = gaussian(histogram_l[m], HISTOGRAM_REGULARIZATION, histogram_l[k]);
					histogram_wn[m] += d * histogram_w[k];
					histogram_wn[k] += d * histogram_w[m];
				}
			}

			float3 maxv = 0;
			float maxw = 0;

			[unroll]
			for (k = 0; k < KERNELLEN; ++k) {
				if (histogram_wn[k] >= maxw) {
					maxw = histogram_wn[k];
					maxv = histogram_v[k];
				}
			}

			WriteToOutput(destPos, maxv);
		}
	}
}
