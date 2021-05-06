// Jinc2 插值算法
// 移植自 https://github.com/libretro/common-shaders/blob/master/windowed/shaders/jinc2.cg
//
// This is an approximation of Jinc(x)*Jinc(x*r1/r2) for x < 2.5,
// where r1 and r2 are the first two zeros of jinc function.
// For a jinc 2-lobe best approximation, use A=0.5 and B=0.825.


// A=0.5, B=0.825 is the best jinc approximation for x<2.5. if B=1.0, it's a lanczos filter.
// Increase A to get more blur. Decrease it to get a sharper picture. 
// B = 0.825 to get rid of dithering. Increase B to get a fine sharpness, though dithering returns.

cbuffer constants : register(b0) {
	int2 srcSize : packoffset(c0.x);
	int2 destSize : packoffset(c0.z);
	float windowSinc : packoffset(c1.x);	// A。必须大于0，值越小图像越清晰，但会有锯齿
	float sinc : packoffset(c1.y);			// B。必须大于0，值越大线条越锐利，但会有抖动
											// 等于 1 时退化成 lanczos 插值
	float ARStrength : packoffset(c1.z);	// 抗振铃强度。取值范围 0~1，越大抗振铃效果越好，但图像越模糊
};


#define MAGPIE_INPUT_COUNT 1
#include "common.hlsli"


float d(float2 pt1, float2 pt2) {
	float2 v = pt2 - pt1;
	return sqrt(dot(v, v));
}

float4 resampler(float4 x, float wa, float wb) {
	return (x == float4(0.0, 0.0, 0.0, 0.0)) 
		? float4(wa * wb, wa * wb, wa * wb, wa * wb)
		: sin(x * wa) * sin(x * wb) / (x * x);
}


D2D_PS_ENTRY(main) {
	InitMagpieSampleInputWithScale(float2(destSize) / srcSize);

	float2 dx = float2(1.0, 0.0);
	float2 dy = float2(0.0, 1.0);

	float2 pc = Coord(0).xy / Coord(0).zw;
	float2 tc = floor(pc - float2(0.5, 0.5)) + float2(0.5, 0.5);

	float wa = windowSinc * PI;
	float wb = sinc * PI;
	float4x4 weights = {
		resampler(float4(d(pc, tc - dx - dy), d(pc, tc - dy), d(pc, tc + dx - dy), d(pc, tc + 2.0 * dx - dy)), wa, wb),
		resampler(float4(d(pc, tc - dx), d(pc, tc), d(pc, tc + dx), d(pc, tc + 2.0 * dx)), wa, wb),
		resampler(float4(d(pc, tc - dx + dy), d(pc, tc + dy), d(pc, tc + dx + dy), d(pc, tc + 2.0 * dx + dy)), wa, wb),
		resampler(float4(d(pc, tc - dx + 2.0 * dy), d(pc, tc + 2.0 * dy), d(pc, tc + dx + 2.0 * dy), d(pc, tc + 2.0 * dx + 2.0 * dy)), wa, wb)
	};

	// !!!改变当前坐标
	Coord(0).xy = tc * Coord(0).zw;

	float2 leftTop1 = GetCheckedOffPos(0, float2(-1, -1));
	float2 rightBottom1 = GetCheckedOffPos(0, float2(1, 1));
	float2 rightBottom2 = GetCheckedOffPos(0, float2(2, 2));

	// reading the texels
	// [ c00, c10, c20, c30 ]
	// [ c01, c11, c21, c31 ]
	// [ c02, c12, c22, c32 ]
	// [ c03, c13, c23, c33 ]
	float3 c00 = SampleInput(0, leftTop1).rgb;
	float3 c10 = SampleInput(0, float2(Coord(0).x, leftTop1.y)).rgb;
	float3 c20 = SampleInput(0, float2(rightBottom1.x, leftTop1.y)).rgb;
	float3 c30 = SampleInput(0, float2(rightBottom2.x, leftTop1.y)).rgb;
	float3 c01 = SampleInput(0, float2(leftTop1.x, Coord(0).y)).rgb;
	float3 c11 = SampleInputCur(0).rgb;
	float3 c21 = SampleInput(0, float2(rightBottom1.x, Coord(0).y)).rgb;
	float3 c31 = SampleInput(0, float2(rightBottom2.x, Coord(0).y)).rgb;
	float3 c02 = SampleInput(0, float2(leftTop1.x, rightBottom1.y)).rgb;
	float3 c12 = SampleInput(0, float2(Coord(0).x, rightBottom1.y)).rgb;
	float3 c22 = SampleInput(0, rightBottom1).rgb;
	float3 c32 = SampleInput(0, float2(rightBottom2.x, rightBottom1.y)).rgb;
	float3 c03 = SampleInput(0, float2(leftTop1.x, rightBottom2.y)).rgb;
	float3 c13 = SampleInput(0, float2(Coord(0).x, rightBottom2.y)).rgb;
	float3 c23 = SampleInput(0, float2(rightBottom1.x, rightBottom2.y)).rgb;
	float3 c33 = SampleInput(0, rightBottom2).rgb;
	

	float3 color = mul(weights[0], float4x3(c00, c10, c20, c30));
	color += mul(weights[1], float4x3(c01, c11, c21, c31));
	color += mul(weights[2], float4x3(c02, c12, c22, c32));
	color += mul(weights[3], float4x3(c03, c13, c23, c33));
	color = color / (dot(mul(weights, float4(1, 1, 1, 1)), 1));

	// 抗振铃
	// Get min/max samples
	float3 min_sample = min4(c11, c21, c12, c22);
	float3 max_sample = max4(c11, c21, c12, c22);
	color = lerp(color, clamp(color, min_sample, max_sample), ARStrength);

	// final sum and weight normalization
	return float4(color.rgb, 1);
}
