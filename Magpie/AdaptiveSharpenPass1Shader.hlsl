#define D2D_INPUT_COUNT 1
#define D2D_INPUT0_COMPLEX
#include "d2d1effecthelpers.hlsli"
#include "AdaptiveSharpen.hlsli"

// Adaptive sharpen
// Tuned for use post resize, EXPECTS FULL RANGE GAMMA LIGHT


// Get destination pixel values
#define get(x,y)  (saturate(sample0(coord.xy + float2(x,y) * coord.zw)))

// Compute diff
#define b_diff(z) (abs(blur-c[z]))

cbuffer constants : register(b0) {
	int2 srcSize : packoffset(c0.x);
};


float3 sample0(float2 pos) {
	float4 coord = D2DGetInputCoordinate(0);

	pos.x = max(0, pos.x);
	pos.x = min((srcSize.x - 1) * coord.z, pos.x);
	pos.y = max(0, pos.y);
	pos.y = min((srcSize.y - 1) * coord.w, pos.y);

	return D2DSampleInput(0, pos).rgb;
}


D2D_PS_ENTRY(main) {
	float4 coord = D2DGetInputCoordinate(0);


	// Get points and saturate out of range values (BTB & WTW)
	// [                c22               ]
	// [           c24, c9,  c23          ]
	// [      c21, c1,  c2,  c3, c18      ]
	// [ c19, c10, c4,  c0,  c5, c11, c16 ]
	// [      c20, c6,  c7,  c8, c17      ]
	// [           c15, c12, c14          ]
	// [                c13               ]
	float3 c[25] = { get(0, 0), get(-1,-1), get(0,-1), get(1,-1), get(-1, 0),
					 get(1, 0), get(-1, 1), get(0, 1), get(1, 1), get(0,-2),
					 get(-2, 0), get(2, 0), get(0, 2), get(0, 3), get(1, 2),
					 get(-1, 2), get(3, 0), get(2, 1), get(2,-1), get(-3, 0),
					 get(-2, 1), get(-2,-1), get(0,-3), get(1,-2), get(-1,-2) };
	
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
