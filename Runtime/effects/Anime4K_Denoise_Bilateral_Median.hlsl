// 移植自 https://github.com/bloc97/Anime4K/blob/master/glsl/Denoise/Anime4K_Denoise_Bilateral_Median.glsl


//!MAGPIE EFFECT
//!VERSION 1
//!OUTPUT_WIDTH INPUT_WIDTH
//!OUTPUT_HEIGHT INPUT_HEIGHT

//!CONSTANT
//!VALUE INPUT_PT_X
float inputPtX;

//!CONSTANT
//!VALUE INPUT_PT_Y
float inputPtY;

//!CONSTANT
//!MIN 1e-5
//!DEFAULT 0.1
float intensitySigma;

//!TEXTURE
Texture2D INPUT;

//!TEXTURE
//!WIDTH INPUT_WIDTH
//!HEIGHT INPUT_HEIGHT
//!FORMAT R16_FLOAT
Texture2D lumaTex;

//!SAMPLER
//!FILTER POINT
SamplerState sam;


//!PASS 1
//!BIND INPUT
//!SAVE lumaTex

float get_luma(float3 rgba) {
	return dot(float3(0.299, 0.587, 0.114), rgba);
}

float4 Pass1(float2 pos) {
	return float4(get_luma(INPUT.Sample(sam, pos).rgb), 0.0, 0.0, 0.0);
}


//!PASS 2
//!BIND INPUT, lumaTex

#pragma warning(disable: 3557)

#define INTENSITY_SIGMA intensitySigma //Intensity window size, higher is stronger denoise, must be a positive real number
#define SPATIAL_SIGMA 1.0 //Spatial window size, higher is stronger denoise, must be a positive real number.
#define HISTOGRAM_REGULARIZATION 0.0 //Histogram regularization window size, higher values approximate a bilateral "closest-to-mean" filter.

#define INTENSITY_POWER_CURVE 1.0 //Intensity window power curve. Setting it to 0 will make the intensity window treat all intensities equally, while increasing it will make the window narrower in darker intensities and wider in brighter intensities.

#define KERNELSIZE uint(max(uint(SPATIAL_SIGMA), 1) * 2 + 1) //Kernel size, must be an positive odd integer.
#define KERNELHALFSIZE (int(KERNELSIZE/2)) //Half of the kernel size without remainder. Must be equal to trunc(KERNELSIZE/2).
#define KERNELLEN (KERNELSIZE * KERNELSIZE) //Total area of kernel. Must be equal to KERNELSIZE * KERNELSIZE.

#define GETOFFSET(i) int2(int(i % KERNELSIZE) - KERNELHALFSIZE, int(i / KERNELSIZE) - KERNELHALFSIZE)

float gaussian(float x, float s, float m) {
	float scaled = (x - m) / s;
	return exp(-0.5 * scaled * scaled);
}

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

	return float3(0, 0, 0);
}

float4 Pass2(float2 pos) {
	float3 histogram_v[KERNELLEN];
	float histogram_l[KERNELLEN];
	float histogram_w[KERNELLEN];
	float n = 0.0;

	float vc = lumaTex.Sample(sam, pos).x;

	float is = pow(vc + 0.0001, INTENSITY_POWER_CURVE) * INTENSITY_SIGMA;
	float ss = SPATIAL_SIGMA;

	uint i;

	for (i = 0; i < KERNELLEN; i++) {
		float2 ipos = pos + GETOFFSET(i) * float2(inputPtX, inputPtY);
		histogram_v[i] = INPUT.Sample(sam, ipos).rgb;
		histogram_l[i] = lumaTex.Sample(sam, ipos).x;
		histogram_w[i] = gaussian(histogram_l[i], is, vc) * gaussian(length(ipos), ss, 0.0);
		n += histogram_w[i];
	}

	if (HISTOGRAM_REGULARIZATION > 0.0) {
		float histogram_wn[KERNELLEN];
		n = 0.0;

		for (i = 0; i < KERNELLEN; i++) {
			histogram_wn[i] = 0.0;
		}

		for (i = 0; i < KERNELLEN; i++) {
			histogram_wn[i] += gaussian(0.0, HISTOGRAM_REGULARIZATION, 0.0) * histogram_w[i];
			for (uint j = (i + 1); j < KERNELLEN; j++) {
				float d = gaussian(histogram_l[j], HISTOGRAM_REGULARIZATION, histogram_l[i]);
				histogram_wn[j] += d * histogram_w[i];
				histogram_wn[i] += d * histogram_w[j];
			}
			n += histogram_wn[i];
		}

		return float4(getMedian(histogram_v, histogram_wn, n), 1);
	}

	return float4(getMedian(histogram_v, histogram_w, n), 1);
}
