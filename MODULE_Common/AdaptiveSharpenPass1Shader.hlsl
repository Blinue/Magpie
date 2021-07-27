// 自适应锐化算法 Pass1
// 移植自 https://github.com/libretro/common-shaders/blob/master/sharpen/shaders/adaptive-sharpen-pass1.cg
// 
// Adaptive sharpen
// Tuned for use post resize, EXPECTS FULL RANGE GAMMA LIGHT


cbuffer constants : register(b0) {
	int2 srcSize : packoffset(c0.x);
};


#define MAGPIE_INPUT_COUNT 1
#include "AdaptiveSharpen.hlsli"


// Compute diff
#define b_diff(z) (abs(blur-c[z]))


D2D_PS_ENTRY(main) {
	InitMagpieSampleInput();

	float left1X = max(0, Coord(0).x - Coord(0).z);
	float left2X = max(0, left1X - Coord(0).z);
	float left3X = max(0, left2X - Coord(0).z);
	float right1X = min(maxCoord0.x, Coord(0).x + Coord(0).z);
	float right2X = min(maxCoord0.x, right1X + Coord(0).z);
	float right3X = min(maxCoord0.x, right2X + Coord(0).z);
	float top1Y = max(0, Coord(0).y - Coord(0).w);
	float top2Y = max(0, top1Y - Coord(0).w);
	float top3Y = max(0, top2Y - Coord(0).w);
	float bottom1Y = min(maxCoord0.y, Coord(0).y + Coord(0).w);
	float bottom2Y = min(maxCoord0.y, bottom1Y + Coord(0).w);
	float bottom3Y = min(maxCoord0.y, bottom2Y + Coord(0).w);
	
	// Get points and saturate out of range values (BTB & WTW)
	// [                c22               ]
	// [           c24, c9,  c23          ]
	// [      c21, c1,  c2,  c3, c18      ]
	// [ c19, c10, c4,  c0,  c5, c11, c16 ]
	// [      c20, c6,  c7,  c8, c17      ]
	// [           c15, c12, c14          ]
	// [                c13               ]
	float3 c[25] = {
		SampleInputCur(0).rgb,									// c0
		SampleInput(0, float2(left1X, top1Y)).rgb,		// c1
		SampleInput(0, float2(Coord(0).x, top1Y)).rgb,		// c2
		SampleInput(0, float2(right1X, top1Y)).rgb,		// c3
		SampleInput(0, float2(left1X, Coord(0).y)).rgb,		// c4
		SampleInput(0, float2(right1X, Coord(0).y)).rgb,	// c5
		SampleInput(0, float2(left1X, bottom1Y)).rgb,	// c6
		SampleInput(0, float2(Coord(0).x, bottom1Y)).rgb,	// c7
		SampleInput(0, float2(right1X, bottom1Y)).rgb,	// c8
		SampleInput(0, float2(Coord(0).x, top2Y)).rgb,		// c9
		SampleInput(0, float2(left2X, Coord(0).y)).rgb,		// c10
		SampleInput(0, float2(right2X, Coord(0).y)).rgb,	// c11
		SampleInput(0, float2(Coord(0).x, bottom2Y)).rgb,	// c12
		SampleInput(0, float2(Coord(0).x, bottom3Y)).rgb,	// c13
		SampleInput(0, float2(right1X, bottom2Y)).rgb,	// c14
		SampleInput(0, float2(left1X, bottom2Y)).rgb,	// c15
		SampleInput(0, float2(right3X, Coord(0).y)).rgb,	// c16
		SampleInput(0, float2(right2X, bottom1Y)).rgb,	// c17
		SampleInput(0, float2(right2X, top1Y)).rgb,		// c18
		SampleInput(0, float2(left3X, Coord(0).y)).rgb,		// c19
		SampleInput(0, float2(left2X, bottom1Y)).rgb,	// c20
		SampleInput(0, float2(left2X, top1Y)).rgb,		// c21
		SampleInput(0, float2(Coord(0).x, top3Y)).rgb,		// c22
		SampleInput(0, float2(right1X, top2Y)).rgb,		// c23
		SampleInput(0, float2(left1X, top2Y)).rgb		// c24
	};

	
	// Blur, gauss 3x3
	float3 blur = (2 * (c[2] + c[4] + c[5] + c[7]) + (c[1] + c[3] + c[6] + c[8]) + 4 * c[0]) / 16;
	float  blur_Y = (blur.r / 3 + blur.g / 3 + blur.b / 3);

	// Contrast compression, center = 0.5, scaled to 1/3
	float c_comp = saturate(0.266666681f + 0.9 * pow(2, (-7.4 * blur_Y)));

	// Edge detection
	// Matrix weights
	// [         1/4,        ]
	// [      4,  1,  4      ]
	// [ 1/4, 4,  1,  4, 1/4 ]
	// [      4,  1,  4      ]
	// [         1/4         ]
	float edge = length(b_diff(0) + b_diff(1) + b_diff(2) + b_diff(3)
		+ b_diff(4) + b_diff(5) + b_diff(6) + b_diff(7) + b_diff(8)
		+ 0.25 * (b_diff(9) + b_diff(10) + b_diff(11) + b_diff(12)));

	return float4(c[0].rgb, Compress(edge * c_comp));
}
