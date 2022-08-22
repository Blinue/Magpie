// 移植自 https://github.com/bloc97/Anime4K/blob/master/glsl/Denoise/Anime4K_Denoise_Bilateral_Median.glsl


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
//!BLOCK_SIZE 8
//!NUM_THREADS 64

#define INTENSITY_SIGMA intensitySigma //Intensity window size, higher is stronger denoise, must be a positive real number
#define SPATIAL_SIGMA 1.0 //Spatial window size, higher is stronger denoise, must be a positive real number.
#define HISTOGRAM_REGULARIZATION 0.0 //Histogram regularization window size, higher values approximate a bilateral "closest-to-mean" filter.

#define INTENSITY_POWER_CURVE 1.0 //Intensity window power curve. Setting it to 0 will make the intensity window treat all intensities equally, while increasing it will make the window narrower in darker intensities and wider in brighter intensities.

#define KERNELSIZE uint(max(uint(SPATIAL_SIGMA), 1) * 2 + 1) //Kernel size, must be an positive odd integer.
#define KERNELHALFSIZE (int(KERNELSIZE/2)) //Half of the kernel size without remainder. Must be equal to trunc(KERNELSIZE/2).
#define KERNELLEN (KERNELSIZE * KERNELSIZE) //Total area of kernel. Must be equal to KERNELSIZE * KERNELSIZE.

#define GETOFFSET(i) int2(int(i % KERNELSIZE) - KERNELHALFSIZE, int(i / KERNELSIZE) - KERNELHALFSIZE)

float get_luma(float3 rgb) {
	return dot(float3(0.299, 0.587, 0.114), rgb);
}

float gaussian(float x, float s, float m) {
	float scaled = (x - m) / s;
	return exp(-0.5 * scaled * scaled);
}

float3 getMedian(float3 v[KERNELLEN], float w[KERNELLEN], float n) {
	float3 result = float3(0, 0, 0);

	[unroll]
	for (uint i = 0; i < KERNELLEN; i++) {
		float w_above = 0.0;
		float w_below = 0.0;
		[unroll]
		for (uint j = 0; j < KERNELLEN; j++) {
			if (v[j].x > v[i].x) {
				w_above += w[j];
			} else if (v[j].x < v[i].x) {
				w_below += w[j];
			}
		}

		if ((n - w_above) / n >= 0.5 && w_below / n <= 0.5) {
			result = v[i];
			break;
		}
	}

	return result;
}

void Pass1(uint2 blockStart, uint3 threadId) {
	uint2 gxy = Rmp8x8(threadId.x) + blockStart;
	if (!CheckViewport(gxy)) {
		return;
	}

	float2 inputPt = GetInputPt();
	float2 pos = (gxy + 0.5f) * inputPt;

	float3 histogram_v[KERNELLEN];
	float histogram_l[KERNELLEN];
	float histogram_w[KERNELLEN];
	float n = 0.0;

	float vc = get_luma(INPUT.SampleLevel(sam, pos, 0).rgb);

	float is = pow(vc + 0.0001, INTENSITY_POWER_CURVE) * INTENSITY_SIGMA;
	float ss = SPATIAL_SIGMA;

	uint i;

	[unroll]
	for (i = 0; i < KERNELLEN; i++) {
		int2 ipos = GETOFFSET(i);
		histogram_v[i] = INPUT.SampleLevel(sam, pos + ipos * inputPt, 0).rgb;
		histogram_l[i] = get_luma(histogram_v[i]);
		histogram_w[i] = gaussian(histogram_l[i], is, vc) * gaussian(length(ipos), ss, 0.0);
		n += histogram_w[i];
	}

	if (HISTOGRAM_REGULARIZATION > 0.0) {
		float histogram_wn[KERNELLEN];
		n = 0.0;

		[unroll]
		for (i = 0; i < KERNELLEN; i++) {
			histogram_wn[i] = 0.0;
		}

		[unroll]
		for (i = 0; i < KERNELLEN; i++) {
			histogram_wn[i] += gaussian(0.0, HISTOGRAM_REGULARIZATION, 0.0) * histogram_w[i];
			[unroll]
			for (uint j = (i + 1); j < KERNELLEN; j++) {
				float d = gaussian(histogram_l[j], HISTOGRAM_REGULARIZATION, histogram_l[i]);
				histogram_wn[j] += d * histogram_w[i];
				histogram_wn[i] += d * histogram_w[j];
			}
			n += histogram_wn[i];
		}

		WriteToOutput(gxy, getMedian(histogram_v, histogram_wn, n));
		return;
	}

	WriteToOutput(gxy, getMedian(histogram_v, histogram_w, n));
}
