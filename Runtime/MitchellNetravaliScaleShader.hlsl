// Mitchell-Netravali 插值算法
// 移植自 https://github.com/libretro/common-shaders/blob/master/bicubic/shaders/bicubic-normal.cg


cbuffer constants : register(b0) {
	int2 srcSize : packoffset(c0.x);
	int2 destSize : packoffset(c0.z);
	int useSharperVersion : packoffset(c1.x);
};


#define MAGPIE_INPUT_COUNT 1
#include "common.hlsli"


float weight(float x, float B, float C) {
	float ax = abs(x);

	if (ax < 1.0) {
		return (pow(x, 2.0) * ((12.0 - 9.0 * B - 6.0 * C) * ax + (-18.0 + 12.0 * B + 6.0 * C)) + (6.0 - 2.0 * B)) / 6.0;
	} else if (ax >= 1.0 && ax < 2.0) {
		return (pow(x, 2.0) * ((-B - 6.0 * C) * ax + (6.0 * B + 30.0 * C)) + (-12.0 * B - 48.0 * C) * ax + (8.0 * B + 24.0 * C)) / 6.0;
	} else {
		return 0.0;
	}
}

float4 weight4(float x) {
	float B = 0.0;
	float C = 0.0;
	if (useSharperVersion == 0) {
		// Mitchel-Netravali coefficients.
		// Best psychovisual result.
		B = 1.0 / 3.0;
		C = 1.0 / 3.0;
	} else {
		// Sharper version.
		// May look better in some cases.
		B = 0.0;
		C = 0.75;
	}

	return float4(
		weight(x - 2.0, B, C),
		weight(x - 1.0, B, C),
		weight(x, B, C),
		weight(x + 1.0, B, C)
	);
}


float3 line_run(float ypos, float4 xpos, float4 linetaps) {
	return SampleInput(0, float2(xpos.r, ypos)).rgb * linetaps.r
		+ SampleInput(0, float2(xpos.g, ypos)).rgb * linetaps.g
		+ SampleInput(0, float2(xpos.b, ypos)).rgb * linetaps.b
		+ SampleInput(0, float2(xpos.a, ypos)).rgb * linetaps.a;
}


D2D_PS_ENTRY(main) {
	InitMagpieSampleInputWithScale(float2(destSize) / srcSize);

	float2 f = frac(Coord(0).xy / Coord(0).zw + 0.5);
	
	float4 linetaps = weight4(1.0 - f.x);
	float4 columntaps = weight4(1.0 - f.y);

	//make sure all taps added together is exactly 1.0, otherwise some (very small) distortion can occur
	linetaps /= linetaps.r + linetaps.g + linetaps.b + linetaps.a;
	columntaps /= columntaps.r + columntaps.g + columntaps.b + columntaps.a;

	// !!!改变当前坐标
	Coord(0).xy -= (f + 1) * Coord(0).zw;

	float4 xpos = float4(Coord(0).x, min(Coord(0).x + Coord(0).z, maxCoord0.x), min(Coord(0).x + 2 * Coord(0).z, maxCoord0.x), min(Coord(0).x + 3 * Coord(0).z, maxCoord0.x));

	// final sum and weight normalization
	return float4(line_run(Coord(0).y, xpos, linetaps) * columntaps.r
		+ line_run(min(Coord(0).y + Coord(0).w, maxCoord0.y), xpos, linetaps) * columntaps.g
		+ line_run(min(Coord(0).y + 2 * Coord(0).w, maxCoord0.y), xpos, linetaps) * columntaps.b
		+ line_run(min(Coord(0).y + 3 * Coord(0).w, maxCoord0.y), xpos, linetaps) * columntaps.a,
		1);
}
