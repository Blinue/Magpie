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
	float3 c[25] = { 
		SampleInputCur(0), SampleInputOffCheckLeftTop(0, -1, -1),  SampleInputOffCheckTop(0, 0, -1), SampleInputOffCheckRightTop(0, 1, -1), SampleInputOffCheckLeft(0, -1, 0), 
		SampleInputOffCheckRight(0, 1, 0), SampleInputOffCheckLeftBottom(0, -1, 1), SampleInputOffCheckBottom(0, 0, 1), SampleInputOffCheckRightBottom(0, 1, 1), SampleInputOffCheckTop(0, 0, -2), 
		SampleInputOffCheckLeft(0, -2, 0), SampleInputOffCheckRight(0, 2, 0), SampleInputOffCheckBottom(0, 0, 2), SampleInputOffCheckBottom(0, 0, 3), SampleInputOffCheckRightBottom(0, 1, 2),
		SampleInputOffCheckLeftBottom(0, -1, 2), SampleInputOffCheckRight(0, 3, 0), SampleInputOffCheckRightBottom(0, 2, 1), SampleInputOffCheckRightTop(0, 2,-1), SampleInputOffCheckLeft(0, -3, 0), 
		SampleInputOffCheckLeftBottom(0, -2, 1), SampleInputOffCheckLeftTop(0, -2, -1), SampleInputOffCheckTop(0, 0, -3), SampleInputOffCheckRightTop(0, 1, -2), SampleInputOffCheckLeftTop(0, -1, -2) 
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

	return float4(D2DSampleInput(0, coord.xy).rgb, Compress(edge * c_comp));
}
