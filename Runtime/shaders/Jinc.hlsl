// Jinc2 插值算法
// 移植自 https://github.com/libretro/common-shaders/blob/master/windowed/shaders/jinc2.cg
//
// This is an approximation of Jinc(x)*Jinc(x*r1/r2) for x < 2.5,
// where r1 and r2 are the first two zeros of jinc function.
// For a jinc 2-lobe best approximation, use A=0.5 and B=0.825.

// A=0.5, B=0.825 is the best jinc approximation for x<2.5. if B=1.0, it's a lanczos filter.
// Increase A to get more blur. Decrease it to get a sharper picture. 
// B = 0.825 to get rid of dithering. Increase B to get a fine sharpness, though dithering returns.

//!MAGPIE EFFECT
//!VERSION 1


//!CONSTANT
//!VALUE INPUT_PT_X
float inputPtX;

//!CONSTANT
//!VALUE INPUT_PT_Y
float inputPtY;

//!CONSTANT
//!DEFAULT 0.5
//!MIN 1e-5
float windowSinc;

//!CONSTANT
//!DEFAULT 0.825
//!MIN 1e-5
float sinc;

//!CONSTANT
//!DEFAULT 0.5
//!MIN 0
//!MAX 1
float ARStrength;

//!TEXTURE
Texture2D INPUT;

//!SAMPLER
//!FILTER POINT
SamplerState sam;


//!PASS 1
//!BIND INPUT

#define PI 3.1415926535897932384626433832795

#define min4(a, b, c, d) min(min(a, b), min(c, d))
#define max4(a, b, c, d) max(max(a, b), max(c, d))


float d(float2 pt1, float2 pt2) {
	float2 v = pt2 - pt1;
	return sqrt(dot(v, v));
}

float4 resampler(float4 x, float wa, float wb) {
	return (x == float4(0.0, 0.0, 0.0, 0.0))
		? float4(wa * wb, wa * wb, wa * wb, wa * wb)
		: sin(x * wa) * sin(x * wb) / (x * x);
}

float4 Pass1(float2 pos) {
	float2 dx = float2(1.0, 0.0);
	float2 dy = float2(0.0, 1.0);

	float2 pc = pos / float2(inputPtX, inputPtY);
	float2 tc = floor(pc - float2(0.5, 0.5)) + float2(0.5, 0.5);

	float wa = windowSinc * PI;
	float wb = sinc * PI;
	float4x4 weights = {
		resampler(float4(d(pc, tc - dx - dy), d(pc, tc - dy), d(pc, tc + dx - dy), d(pc, tc + 2.0 * dx - dy)), wa, wb),
		resampler(float4(d(pc, tc - dx), d(pc, tc), d(pc, tc + dx), d(pc, tc + 2.0 * dx)), wa, wb),
		resampler(float4(d(pc, tc - dx + dy), d(pc, tc + dy), d(pc, tc + dx + dy), d(pc, tc + 2.0 * dx + dy)), wa, wb),
		resampler(float4(d(pc, tc - dx + 2.0 * dy), d(pc, tc + 2.0 * dy), d(pc, tc + dx + 2.0 * dy), d(pc, tc + 2.0 * dx + 2.0 * dy)), wa, wb)
	};

	dx *= float2(inputPtX, inputPtY);
	dy *= float2(inputPtX, inputPtY);
	tc *= float2(inputPtX, inputPtY);

	// reading the texels
	// [ c00, c10, c20, c30 ]
	// [ c01, c11, c21, c31 ]
	// [ c02, c12, c22, c32 ]
	// [ c03, c13, c23, c33 ]
	float3 c00 = INPUT.Sample(sam, tc - dx - dy).rgb;
	float3 c10 = INPUT.Sample(sam, tc - dy).rgb;
	float3 c20 = INPUT.Sample(sam, tc + dx - dy).rgb;
	float3 c30 = INPUT.Sample(sam, tc + 2.0 * dx - dy).rgb;
	float3 c01 = INPUT.Sample(sam, tc - dx).rgb;
	float3 c11 = INPUT.Sample(sam, tc).rgb;
	float3 c21 = INPUT.Sample(sam, tc + dx).rgb;
	float3 c31 = INPUT.Sample(sam, tc + 2.0 * dx).rgb;
	float3 c02 = INPUT.Sample(sam, tc - dx + dy).rgb;
	float3 c12 = INPUT.Sample(sam, tc + dy).rgb;
	float3 c22 = INPUT.Sample(sam, tc + dx + dy).rgb;
	float3 c32 = INPUT.Sample(sam, tc + 2.0 * dx + dy).rgb;
	float3 c03 = INPUT.Sample(sam, tc - dx + 2.0 * dy).rgb;
	float3 c13 = INPUT.Sample(sam, tc + 2.0 * dy).rgb;
	float3 c23 = INPUT.Sample(sam, tc + dx + 2.0 * dy).rgb;
	float3 c33 = INPUT.Sample(sam, tc + 2.0 * dx + 2.0 * dy).rgb;


	float3 color = mul(weights[0], float4x3(c00, c10, c20, c30));
	color += mul(weights[1], float4x3(c01, c11, c21, c31));
	color += mul(weights[2], float4x3(c02, c12, c22, c32));
	color += mul(weights[3], float4x3(c03, c13, c23, c33));
	color /= dot(mul(weights, float4(1, 1, 1, 1)), 1);

	// 抗振铃
	// Get min/max samples
	float3 min_sample = min4(c11, c21, c12, c22);
	float3 max_sample = max4(c11, c21, c12, c22);
	color = lerp(color, clamp(color, min_sample, max_sample), ARStrength);

	// final sum and weight normalization
	return float4(color.rgb, 1);
}
