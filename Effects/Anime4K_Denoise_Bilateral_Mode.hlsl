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
//!STYLE PS
//!IN INPUT


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

float4 Pass1(float2 pos) {
	float3 histogram_v[KERNELLEN];
	float histogram_l[KERNELLEN];
	float histogram_w[KERNELLEN];
	float histogram_wn[KERNELLEN];

	float vc = get_luma(INPUT.SampleLevel(sam, pos, 0).rgb);

	float is = pow(vc + 0.0001, INTENSITY_POWER_CURVE) * INTENSITY_SIGMA;
	float ss = SPATIAL_SIGMA;

	uint i;
	float2 inputPt = GetInputPt();

	[unroll]
	for (i = 0; i < KERNELLEN; i++) {
		float2 ipos = GETOFFSET(i);
		histogram_v[i] = INPUT.SampleLevel(sam, pos + ipos * inputPt, 0).rgb;
		histogram_l[i] = get_luma(histogram_v[i]);
		histogram_w[i] = gaussian(histogram_l[i], is, vc) * gaussian(length(ipos), ss, 0.0);
		histogram_wn[i] = 0.0;
	}

	[unroll]
	for (i = 0; i < KERNELLEN; i++) {
		histogram_wn[i] += gaussian(0.0, HISTOGRAM_REGULARIZATION, 0.0) * histogram_w[i];
		for (uint j = (i + 1); j < KERNELLEN; j++) {
			float d = gaussian(histogram_l[j], HISTOGRAM_REGULARIZATION, histogram_l[i]);
			histogram_wn[j] += d * histogram_w[i];
			histogram_wn[i] += d * histogram_w[j];
		}
	}

	float3 maxv = 0;
	float maxw = 0;

	[unroll]
	for (i = 0; i < KERNELLEN; ++i) {
		if (histogram_wn[i] >= maxw) {
			maxw = histogram_wn[i];
			maxv = histogram_v[i];
		}
	}

	return float4(maxv, 1);
}
