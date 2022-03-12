// Anime4K_Thin_HQ
// 移植自 https://github.com/bloc97/Anime4K/blob/master/glsl/Experimental-Effects/Anime4K_Thin_HQ.glsl

//!MAGPIE EFFECT
//!VERSION 1
//!OUTPUT_WIDTH INPUT_WIDTH
//!OUTPUT_HEIGHT INPUT_HEIGHT


//!CONSTANT
//!VALUE INPUT_HEIGHT
int inputHeight;

//!CONSTANT
//!VALUE INPUT_PT_X
float inputPtX;

//!CONSTANT
//!VALUE INPUT_PT_Y
float inputPtY;

//!CONSTANT
//!DEFAULT 0.6
//!MIN 1e-5

// Strength of warping for each iteration
float strength;

//!CONSTANT
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

//!PASS 1
//!BIND INPUT
//!SAVE tex1

float get_luma(float3 rgb) {
	return dot(float3(0.299, 0.587, 0.114), rgb);
}

float4 Pass1(float2 pos) {
	float l = get_luma(INPUT.Sample(sam, float2(pos.x - inputPtX, pos.y)).rgb);
	float c = get_luma(INPUT.Sample(sam, pos).rgb);
	float r = get_luma(INPUT.Sample(sam, float2(pos.x + inputPtX, pos.y)).rgb);

	float xgrad = (-l + r);
	float ygrad = (l + c + c + r);

	return float4(xgrad, ygrad, 0.0, 0.0);
}

//!PASS 2
//!BIND tex1
//!SAVE tex2

float4 Pass2(float2 pos) {
	float2 t = tex1.Sample(sam, float2(pos.x, pos.y - inputPtY)).xy;
	float cx = tex1.Sample(sam, pos).x;
	float2 b = tex1.Sample(sam, float2(pos.x, pos.y + inputPtY)).xy;

	float xgrad = (t.x + cx + cx + b.x) / 8.0;

	float ygrad = (-t.y + b.y) / 8.0;

	//Computes the luminance's gradient
	float norm = sqrt(xgrad * xgrad + ygrad * ygrad);
	return float4(pow(norm, 0.7), 0, 0, 0);
}

//!PASS 3
//!BIND tex2
//!SAVE tex1

#define SPATIAL_SIGMA (2.0 * inputHeight / 1080.0) //Spatial window size, must be a positive real number.

#define KERNELSIZE (max(int(ceil(SPATIAL_SIGMA * 2.0)), 1) * 2 + 1) //Kernel size, must be an positive odd integer.
#define KERNELHALFSIZE (int(KERNELSIZE/2)) //Half of the kernel size without remainder. Must be equal to trunc(KERNELSIZE/2).
#define KERNELLEN (KERNELSIZE * KERNELSIZE) //Total area of kernel. Must be equal to KERNELSIZE * KERNELSIZE.

float gaussian(float x, float s, float m) {
	float scaled = (x - m) / s;
	return exp(-0.5 * scaled * scaled);
}

float4 Pass3(float2 pos) {
	float g = 0.0;
	float gn = 0.0;

	for (int i = 0; i < KERNELSIZE; i++) {
		int di = i - KERNELHALFSIZE;
		float gf = gaussian(di, SPATIAL_SIGMA, 0.0);

		g = g + tex2.SampleLevel(sam, pos + float2(di * inputPtX, 0.0), 0).x * gf;
		gn = gn + gf;
	}

	return float4(g / gn, 0, 0, 0);
}

//!PASS 4
//!BIND tex1
//!SAVE tex2

#define SPATIAL_SIGMA (2.0 * inputHeight / 1080.0) //Spatial window size, must be a positive real number.

#define KERNELSIZE (max(int(ceil(SPATIAL_SIGMA * 2.0)), 1) * 2 + 1) //Kernel size, must be an positive odd integer.
#define KERNELHALFSIZE (int(KERNELSIZE/2)) //Half of the kernel size without remainder. Must be equal to trunc(KERNELSIZE/2).
#define KERNELLEN (KERNELSIZE * KERNELSIZE) //Total area of kernel. Must be equal to KERNELSIZE * KERNELSIZE.

float gaussian(float x, float s, float m) {
	float scaled = (x - m) / s;
	return exp(-0.5 * scaled * scaled);
}

float4 Pass4(float2 pos) {
	float g = 0.0;
	float gn = 0.0;

	for (int i = 0; i < KERNELSIZE; i++) {
		int di = i - KERNELHALFSIZE;
		float gf = gaussian(di, SPATIAL_SIGMA, 0.0);

		g = g + tex1.SampleLevel(sam, pos + float2(0, di * inputPtY), 0).x * gf;
		gn = gn + gf;
	}

	return float4(g / gn, 0, 0, 0);
}

//!PASS 5
//!BIND tex2
//!SAVE tex1

float4 Pass5(float2 pos) {
	float l = tex2.Sample(sam, float2(pos.x - inputPtX, pos.y)).x;
	float c = tex2.Sample(sam, pos).x;
	float r = tex2.Sample(sam, float2(pos.x + inputPtX, pos.y)).x;

	float xgrad = (-l + r);
	float ygrad = (l + c + c + r);

	return float4(xgrad, ygrad, 0.0, 0.0);
}

//!PASS 6
//!BIND tex1
//!SAVE tex2

float4 Pass6(float2 pos) {
	float2 t = tex1.Sample(sam, float2(pos.x, pos.y - inputPtY)).xy;
	float cx = tex1.Sample(sam, pos).x;
	float2 b = tex1.Sample(sam, float2(pos.x, pos.y + inputPtY)).xy;

	float xgrad = (t.x + cx + cx + b.x) / 8.0;

	float ygrad = (-t.y + b.y) / 8.0;

	//Computes the luminance's gradient
	return float4(xgrad, ygrad, 0.0, 0.0);
}

//!PASS 7
//!BIND tex2, INPUT

#define STRENGTH strength 
#define ITERATIONS iterations 

float4 Pass7(float2 pos) {
	float2 d = { inputPtX, inputPtY };

	float relstr = inputHeight / 1080.0 * STRENGTH;

	for (int i = 0; i < ITERATIONS; i++) {
		float2 dn = tex2.SampleLevel(sam1, pos, 0).xy;
		float2 dd = (dn / (length(dn) + 0.01)) * d * relstr; //Quasi-normalization for large vectors, avoids divide by zero
		pos -= dd;
	}

	return INPUT.Sample(sam1, pos);
}
