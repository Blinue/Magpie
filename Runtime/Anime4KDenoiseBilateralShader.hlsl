// Anime4K-v3.1-Denoise-Bilateral-Median
// ÒÆÖ²×Ô https://github.com/bloc97/Anime4K/blob/master/glsl/Denoise/Anime4K_Denoise_Bilateral_Median.glsl


#pragma warning(disable: 3557)

cbuffer constants : register(b0) {
	int2 srcSize : packoffset(c0.x);
	int variant : packoffset(c0.z);	// 0: mode, 1: median, 2: mean
	float intensitySigma : packoffset(c0.w);	//Intensity window size, higher is stronger denoise, must be a positive real number
};


#define MAGPIE_INPUT_COUNT 1
#define MAGPIE_USE_YUV
#include "Anime4K.hlsli"


#define SPATIAL_SIGMA 1.0 //Spatial window size, higher is stronger denoise, must be a positive real number.

#define HISTOGRAM_REGULARIZATION_MEDIAN 0.0 //Histogram regularization window size, higher values approximate a bilateral "closest-to-mean" filter.
#define HISTOGRAM_REGULARIZATION_MODE 0.2

#define INTENSITY_POWER_CURVE 1.0 //Intensity window power curve. Setting it to 0 will make the intensity window treat all intensities equally, while increasing it will make the window narrower in darker intensities and wider in brighter intensities.

#define KERNELSIZE uint(max(uint(SPATIAL_SIGMA), 1) * 2 + 1) //Kernel size, must be an positive odd integer.
#define KERNELHALFSIZE (int(KERNELSIZE/2)) //Half of the kernel size without remainder. Must be equal to trunc(KERNELSIZE/2).
#define KERNELLEN (KERNELSIZE * KERNELSIZE) //Total area of kernel. Must be equal to KERNELSIZE * KERNELSIZE.

#define GETOFFSET(i) float2(int(i % KERNELSIZE) - KERNELHALFSIZE, int(i / KERNELSIZE) - KERNELHALFSIZE)


float gaussian(float x, float s, float m) {
	float scaled = (x - m) / s;
	return exp(-0.5 * scaled * scaled);
}

/*
* MEAN
*/

float3 mean() {
	float3 sum = ZEROS3;
	float n = 0.0;

	float vc = SampleInputCur(0).x;

	float is = pow(vc + 0.0001, INTENSITY_POWER_CURVE) * intensitySigma;
	float ss = SPATIAL_SIGMA;

	for (uint i = 0; i < KERNELLEN; i++) {
		float2 ipos = GETOFFSET(i);
		float3 v = SampleInputOffChecked(0, ipos).xyz;
		float d = gaussian(v.x, is, vc) * gaussian(length(ipos), ss, 0.0);
		sum += d * v;
		n += d;
	}

	return sum / n;
}

/*
* MEDIAN
*/

float3 getMedian(float3 v[KERNELLEN], float w[KERNELLEN], float n) {
	for (uint i = 0; i < KERNELLEN; i++) {
		float w_above = 0.0;
		float w_below = 0.0;
		for (uint j = 0; j < KERNELLEN; j++) {
			if (v[j].x > v[i].x) {
				w_above += w[j];
			} else if (v[j].x < v[i].x) {
				w_below += w[j];
			}
		}

		if ((n - w_above) / n >= 0.5 && w_below / n <= 0.5) {
			return v[i];
		}
	}

	return ZEROS3;
}

float3 median() {
	uint i;
	float3 histogram_v[KERNELLEN];
	float histogram_w[KERNELLEN];
	float n = 0.0;

	float vc = SampleInputCur(0).x;

	float is = pow(vc + 0.0001, INTENSITY_POWER_CURVE) * intensitySigma;
	float ss = SPATIAL_SIGMA;

	for (i = 0; i < KERNELLEN; i++) {
		float2 ipos = GETOFFSET(i);
		histogram_v[i] = SampleInputOffChecked(0, ipos).xyz;
		histogram_w[i] = gaussian(histogram_v[i].x, is, vc) * gaussian(length(ipos), ss, 0.0);
		n += histogram_w[i];
	}

	if (HISTOGRAM_REGULARIZATION_MEDIAN > 0.0) {
		float histogram_wn[KERNELLEN];
		n = 0.0;

		for (i = 0; i < KERNELLEN; i++) {
			histogram_wn[i] = 0.0;
		}

		for (i = 0; i < KERNELLEN; i++) {
			histogram_wn[i] += gaussian(0.0, HISTOGRAM_REGULARIZATION_MEDIAN, 0.0) * histogram_w[i];
			for (uint j = (i + 1); j < KERNELLEN; j++) {
				float d = gaussian(histogram_v[j].x, HISTOGRAM_REGULARIZATION_MEDIAN, histogram_v[i].x);
				histogram_wn[j] += d * histogram_w[i];
				histogram_wn[i] += d * histogram_w[j];
			}
			n += histogram_wn[i];
		}

		return getMedian(histogram_v, histogram_wn, n);
	}

	return getMedian(histogram_v, histogram_w, n);
}


/*
* MODE
*/

float3 mode() {
	uint i;

	float3 histogram_v[KERNELLEN];
	float histogram_w[KERNELLEN];
	float histogram_wn[KERNELLEN];

	float vc = SampleInputCur(0).x;

	float is = pow(vc + 0.0001, INTENSITY_POWER_CURVE) * intensitySigma;
	float ss = SPATIAL_SIGMA;

	for (i = 0; i < KERNELLEN; i++) {
		float2 ipos = GETOFFSET(i);
		histogram_v[i] = SampleInputOffChecked(0, ipos).xyz;
		histogram_w[i] = gaussian(histogram_v[i].x, is, vc) * gaussian(length(ipos), ss, 0.0);
		histogram_wn[i] = 0.0;
	}

	for (i = 0; i < KERNELLEN; i++) {
		histogram_wn[i] += gaussian(0.0, HISTOGRAM_REGULARIZATION_MODE, 0.0) * histogram_w[i];
		for (uint j = (i + 1); j < KERNELLEN; j++) {
			float d = gaussian(histogram_v[j].x, HISTOGRAM_REGULARIZATION_MODE, histogram_v[i].x);
			histogram_wn[j] += d * histogram_w[i];
			histogram_wn[i] += d * histogram_w[j];
		}
	}

	float3 maxv = ZEROS3;
	float maxw = 0.0f;

	for (i = 0; i < KERNELLEN; i++) {
		if (histogram_wn[i] >= maxw) {
			maxw = histogram_wn[i];
			maxv = histogram_v[i];
		}
	}

	return maxv;
}


D2D_PS_ENTRY(main) {
	InitMagpieSampleInput();

	float3 r = ZEROS3;
	if (variant == 0) {
		r = mode();
	} else if (variant == 1) {
		r = median();
	} else {
		r = mean();
	}

	return float4(YUV2RGB(r), 1);
}
