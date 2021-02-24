// Jinc2 ²åÖµËã·¨
// ÒÆÖ²×Ô https://github.com/libretro/common-shaders/blob/master/windowed/shaders/jinc2.cg
//
// This is an approximation of Jinc(x)*Jinc(x*r1/r2) for x < 2.5,
// where r1 and r2 are the first two zeros of jinc function.
// For a jinc 2-lobe best approximation, use A=0.5 and B=0.825.


cbuffer constants : register(b0) {
	int2 srcSize : packoffset(c0.x);
	int2 destSize : packoffset(c0.z);
};


#define D2D_INPUT_COUNT 1
#define D2D_INPUT0_COMPLEX
#include "common.hlsli"


// A=0.5, B=0.825 is the best jinc approximation for x<2.5. if B=1.0, it's a lanczos filter.
// Increase A to get more blur. Decrease it to get a sharper picture. 
// B = 0.825 to get rid of dithering. Increase B to get a fine sharpness, though dithering returns.

#define JINC2_WINDOW_SINC 0.44
#define JINC2_SINC 0.825		// B
#define JINC2_AR_STRENGTH 0.5	// A
#define wa    (JINC2_WINDOW_SINC * PI)
#define wb    (JINC2_SINC * PI)


float d(float2 pt1, float2 pt2) {
	float2 v = pt2 - pt1;
	return sqrt(dot(v, v));
}

float4 resampler(float4 x) {
	return (x == float4(0.0, 0.0, 0.0, 0.0)) 
		? float4(wa * wb, wa * wb, wa * wb, wa * wb)
		: sin(x * wa) * sin(x * wb) / (x * x);
}

float3 sample0(float2 pos) {
	return D2DSampleInput(0, pos).rgb;
}


D2D_PS_ENTRY(main) {
	float4 coord = D2DGetInputCoordinate(0);

	float2 texCoord = coord.xy / coord.zw / destSize;

	float2 dx = float2(1.0, 0.0);
	float2 dy = float2(0.0, 1.0);

	float2 pc = texCoord * srcSize;

	float2 tc = (floor(pc - float2(0.5, 0.5)) + float2(0.5, 0.5));

	float4x4 weights = {
		resampler(float4(d(pc, tc - dx - dy), d(pc, tc - dy), d(pc, tc + dx - dy), d(pc, tc + 2.0 * dx - dy))),
		resampler(float4(d(pc, tc - dx), d(pc, tc), d(pc, tc + dx), d(pc, tc + 2.0 * dx))),
		resampler(float4(d(pc, tc - dx + dy), d(pc, tc + dy), d(pc, tc + dx + dy), d(pc, tc + 2.0 * dx + dy))),
		resampler(float4(d(pc, tc - dx + 2.0 * dy), d(pc, tc + 2.0 * dy), d(pc, tc + dx + 2.0 * dy), d(pc, tc + 2.0 * dx + 2.0 * dy)))
	};

	dx = float2(coord.z, 0);
	dy = float2(0, coord.w);
	tc = tc * coord.zw;

	// reading the texels

	float3 c00 = sample0(tc - dx - dy);
	float3 c10 = sample0(tc - dy);
	float3 c20 = sample0(tc + dx - dy);
	float3 c30 = sample0(tc + 2.0 * dx - dy);
	float3 c01 = sample0(tc - dx);
	float3 c11 = sample0(tc);
	float3 c21 = sample0(tc + dx);
	float3 c31 = sample0(tc + 2.0 * dx);
	float3 c02 = sample0(tc - dx + dy);
	float3 c12 = sample0(tc + dy);
	float3 c22 = sample0(tc + dx + dy);
	float3 c32 = sample0(tc + 2.0 * dx + dy);
	float3 c03 = sample0(tc - dx + 2.0 * dy);
	float3 c13 = sample0(tc + 2.0 * dy);
	float3 c23 = sample0(tc + dx + 2.0 * dy);
	float3 c33 = sample0(tc + 2.0 * dx + 2.0 * dy);

	//  Get min/max samples
	float3 min_sample = min4(c11, c21, c12, c22);
	float3 max_sample = max4(c11, c21, c12, c22);

	float3 color = mul(weights[0], float4x3(c00, c10, c20, c30));
	color += mul(weights[1], float4x3(c01, c11, c21, c31));
	color += mul(weights[2], float4x3(c02, c12, c22, c32));
	color += mul(weights[3], float4x3(c03, c13, c23, c33));
	color = color / (dot(mul(weights, float4(1, 1, 1, 1)), 1));

	// Anti-ringing
	float3 aux = color;
	color = clamp(color, min_sample, max_sample);

	color = lerp(aux, color, JINC2_AR_STRENGTH);

	// final sum and weight normalization
	return float4(color.rgb, 1);
}
