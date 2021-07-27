// 对比度自适应锐化

cbuffer constants : register(b0) {
	int2 srcSize : packoffset(c0.x);
	float sharpness : packoffset(c0.z); // 锐化强度，必须在0~1之间
};


#define MAGPIE_INPUT_COUNT 1
#include "common.hlsli"



D2D_PS_ENTRY(main) {
	InitMagpieSampleInput();

	float2 leftTop = max(0, Coord(0).xy - Coord(0).zw);
	float2 rightBottom = min(maxCoord0.xy, Coord(0).xy + Coord(0).zw);

	// fetch a 3x3 neighborhood around the pixel 'e',
	//	a b c
	//	d(e)f
	//	g h i
	float3 a = SampleInput(0, leftTop).rgb;
	float3 b = SampleInput(0, float2(Coord(0).x, leftTop.y)).rgb;
	float3 c = SampleInput(0, float2(rightBottom.x, leftTop.y)).rgb;
	float3 d = SampleInput(0, float2(leftTop.x, Coord(0).y)).rgb;
	float3 e = SampleInputCur(0).rgb;
	float3 f = SampleInput(0, float2(rightBottom.x, Coord(0).y)).rgb;
	float3 g = SampleInput(0, float2(leftTop.x, rightBottom.y)).rgb;
	float3 h = SampleInput(0, float2(Coord(0).x, rightBottom.y)).rgb;
	float3 i = SampleInput(0, rightBottom).rgb;

	// Soft min and max.
	//	a b c			  b
	//	d e f * 0.5	 +	d e f * 0.5
	//	g h i			  h
	// These are 2.0x bigger (factored out the extra multiply).
	float3 mnRGB = min(min(min(d, e), min(f, b)), h);
	float3 mnRGB2 = min(mnRGB, min(min(a, c), min(g, i)));
	mnRGB += mnRGB2;

	float3 mxRGB = max(max(max(d, e), max(f, b)), h);
	float3 mxRGB2 = max(mxRGB, max(max(a, c), max(g, i)));
	mxRGB += mxRGB2;

	// Smooth minimum distance to signal limit divided by smooth max.
	float3 ampRGB = saturate(min(mnRGB, 2.0 - mxRGB) / mxRGB);

	float peak = -1.0 / lerp(8.0, 5.0, sharpness);
	// Shaping amount of sharpening.
	float3 wRGB = sqrt(ampRGB) * peak;

	// Filter shape.
	//  0 w 0
	//  w 1 w
	//  0 w 0  
	float3 weightRGB = 1.0 + 4.0 * wRGB;
	float3 window = (b + d) + (f + h);
	return float4(saturate((window * wRGB + e) / weightRGB).rgb, 1);
}
