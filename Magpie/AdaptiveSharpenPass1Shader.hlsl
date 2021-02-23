// 自适应锐化算法 Pass1
// 移植自 https://github.com/libretro/common-shaders/blob/master/sharpen/shaders/adaptive-sharpen-pass1.cg
// 
// Adaptive sharpen
// Tuned for use post resize, EXPECTS FULL RANGE GAMMA LIGHT


cbuffer constants : register(b0) {
	int2 srcSize : packoffset(c0.x);
};


#define D2D_INPUT_COUNT 1
#define D2D_INPUT0_COMPLEX
#define MAGPIE_USE_SAMPLE_INPUT
#include "AdaptiveSharpen.hlsli"


// Compute diff
#define b_diff(z) (abs(blur-c[z]))


D2D_PS_ENTRY(main) {
	InitMagpieSampleInput();

	// Get points and saturate out of range values (BTB & WTW)
	// [                c22               ]
	// [           c24, c9,  c23          ]
	// [      c21, c1,  c2,  c3, c18      ]
	// [ c19, c10, c4,  c0,  c5, c11, c16 ]
	// [      c20, c6,  c7,  c8, c17      ]
	// [           c15, c12, c14          ]
	// [                c13               ]
	float left1X = GetCheckedLeft(1);
	float left2X = GetCheckedLeft(2);
	float left3X = GetCheckedLeft(3);
	float right1X = GetCheckedRight(1);
	float right2X = GetCheckedRight(2);
	float right3X = GetCheckedRight(3);
	float top1Y = GetCheckedTop(1);
	float top2Y = GetCheckedTop(2);
	float top3Y = GetCheckedTop(3);
	float bottom1Y = GetCheckedBottom(1);
	float bottom2Y = GetCheckedBottom(2);
	float bottom3Y = GetCheckedBottom(3);
	
	float3 c[25] = {
		SampleInputCur(0),									// c0
		SampleInputNoCheck(0, float2(left1X, top1Y)),		// c1
		SampleInputNoCheck(0, float2(coord.x, top1Y)),		// c2
		SampleInputNoCheck(0, float2(right1X, top1Y)),		// c3
		SampleInputNoCheck(0, float2(left1X, coord.y)),		// c4
		SampleInputNoCheck(0, float2(right1X, coord.y)),	// c5
		SampleInputNoCheck(0, float2(left1X, bottom1Y)),	// c6
		SampleInputNoCheck(0, float2(coord.x, bottom1Y)),	// c7
		SampleInputNoCheck(0, float2(right1X, bottom1Y)),	// c8
		SampleInputNoCheck(0, float2(coord.x, top2Y)),		// c9
		SampleInputNoCheck(0, float2(left2X, coord.y)),		// c10
		SampleInputNoCheck(0, float2(right2X, coord.y)),	// c11
		SampleInputNoCheck(0, float2(coord.x, bottom2Y)),	// c12
		SampleInputNoCheck(0, float2(coord.x, bottom3Y)),	// c13
		SampleInputNoCheck(0, float2(right1X, bottom2Y)),	// c14
		SampleInputNoCheck(0, float2(left1X, bottom2Y)),	// c15
		SampleInputNoCheck(0, float2(right3X, coord.y)),	// c16
		SampleInputNoCheck(0, float2(right2X, bottom1Y)),	// c17
		SampleInputNoCheck(0, float2(right2X, top1Y)),		// c18
		SampleInputNoCheck(0, float2(left3X, coord.y)),		// c19
		SampleInputNoCheck(0, float2(left2X, bottom1Y)),	// c20
		SampleInputNoCheck(0, float2(left2X, top1Y)),		// c21
		SampleInputNoCheck(0, float2(coord.x, top3Y)),		// c22
		SampleInputNoCheck(0, float2(right1X, top2Y)),		// c23
		SampleInputNoCheck(0, float2(left1X, top2Y))		// c24
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
