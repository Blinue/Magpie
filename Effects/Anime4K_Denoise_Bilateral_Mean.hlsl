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
//!BLOCK_SIZE 8,8
//!NUM_THREADS 64,1,1

#define INTENSITY_SIGMA intensitySigma //Intensity window size, higher is stronger denoise, must be a positive real number
#define SPATIAL_SIGMA 1.0 //Spatial window size, higher is stronger denoise, must be a positive real number.

#define INTENSITY_POWER_CURVE 1.0 //Intensity window power curve. Setting it to 0 will make the intensity window treat all intensities equally, while increasing it will make the window narrower in darker intensities and wider in brighter intensities.

#define KERNELSIZE (max(uint(ceil(SPATIAL_SIGMA * 2.0)), 1) * 2 + 1) //Kernel size, must be an positive odd integer.
#define KERNELHALFSIZE (int(KERNELSIZE/2)) //Half of the kernel size without remainder. Must be equal to trunc(KERNELSIZE/2).
#define KERNELLEN (KERNELSIZE * KERNELSIZE) //Total area of kernel. Must be equal to KERNELSIZE * KERNELSIZE.

#define GETOFFSET(i) int2(int(i % KERNELSIZE) - KERNELHALFSIZE, int(i / KERNELSIZE) - KERNELHALFSIZE)

float3 gaussian_vec(float3 x, float3 s, float3 m) {
	float3 scaled = (x - m) * s;
	return exp(-0.5 * scaled * scaled);
}

float gaussian(float x, float s, float m) {
	float scaled = (x - m) * s;
	return exp(-0.5 * scaled * scaled);
}


void Pass1(uint2 blockStart, uint3 threadId) {
	uint2 gxy = Rmp8x8(threadId.x) + blockStart;
	if (!CheckViewport(gxy)) {
		return;
	}

	float2 inputPt = GetInputPt();
	float2 pos = (gxy + 0.5f) * inputPt;

	float3 sum = 0;
	float3 n = 0;

	int i, j;

	float4 src[KERNELSIZE][KERNELSIZE];
	[unroll]
	for (i = -KERNELHALFSIZE; i <= KERNELHALFSIZE; ++i) {
		[unroll]
		for (j = -KERNELHALFSIZE; j <= KERNELHALFSIZE; ++j) {
			src[i + KERNELHALFSIZE][j + KERNELHALFSIZE] = 
				float4(INPUT.SampleLevel(sam, pos + float2(i, j) * inputPt, 0).rgb, length(float2(i, j)));
		}
	}

	float3 vc = src[KERNELHALFSIZE][KERNELHALFSIZE].rgb;

	float3 rcpIs = rcp(pow(vc + 0.0001, INTENSITY_POWER_CURVE) * INTENSITY_SIGMA);
	float rcpSs = rcp(SPATIAL_SIGMA);
	
	[unroll]
	for (i = 0; i < (int)KERNELSIZE; ++i) {
		[unroll]
		for (j = 0; j < (int)KERNELSIZE; ++j) {
			float3 v = src[i][j].rgb;
			float3 d = gaussian_vec(v, rcpIs, vc) * gaussian(src[i][j].a, rcpSs, 0);
			sum += d * v;
			n += d;
		}
	}

	WriteToOutput(gxy, sum / n);
}
