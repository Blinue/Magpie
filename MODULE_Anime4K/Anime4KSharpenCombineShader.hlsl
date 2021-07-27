// 对Anime4K计算出的差值进行锐化以及输出最终结果
// 使用了单步的自适应锐化，参考自 https://github.com/libretro/common-shaders/blob/master/sharpen/shaders/adaptive-sharpen.cg


cbuffer constants : register(b0) {
	int2 srcSize : packoffset(c0.x);
	float curveHeight : packoffset(c0.z);	// 锐化强度，零表示不锐化，否则必须为正值，一般在 0.3~2.0 之间
};


#define MAGPIE_INPUT_COUNT 2
#define MAGPIE_USE_YUV
#include "common.hlsli"


//-------------------------------------------------------------------------------------------------
// Defined values under this row are "optimal" DO NOT CHANGE IF YOU DO NOT KNOW WHAT YOU ARE DOING!

#define curveslope      (curveHeight*1.5)   // Sharpening curve slope, edge region
#define D_overshoot     0.016                // Max dark overshoot before max compression
#define D_comp_ratio    0.250                // Max compression ratio, dark overshoot (1/0.25=4x)
#define L_overshoot     0.004                // Max light overshoot before max compression
#define L_comp_ratio    0.167                // Max compression ratio, light overshoot (1/0.167=6x)
#define max_scale_lim   10.0                 // Abs change before max compression (1/10=±10%)

#define noise_threshold 0.005


float get1(float2 pos) {
	float2 f = frac(pos / Coord(1).zw);
	
	int2 i = int2(f * 2);
	float l = uncompressLinear(SampleInput(1, pos + (float2(0.5, 0.5) - f) * Coord(1).zw)[i.y * 2 + i.x], -1, 1);

	if (abs(l) < noise_threshold) {
		return 0;
	} else {
		return l;
	}
}

float getDiff() {
	if (curveHeight < 0.01) {
		return get1(Coord(1).xy);
	} else {
		float left1X = max(0, Coord(1).x - Coord(1).z);
		float left2X = max(0, left1X - Coord(1).z);
		float left3X = max(0, left2X - Coord(1).z);
		float right1X = min(maxCoord1.x, Coord(1).x + Coord(1).z);
		float right2X = min(maxCoord1.x, right1X + Coord(1).z);
		float right3X = min(maxCoord1.x, right2X + Coord(1).z);
		float top1Y = max(0, Coord(1).y - Coord(1).w);
		float top2Y = max(0, top1Y - Coord(1).w);
		float top3Y = max(0, top2Y - Coord(1).w);
		float bottom1Y = min(maxCoord1.y, Coord(1).y + Coord(1).w);
		float bottom2Y = min(maxCoord1.y, bottom1Y + Coord(1).w);
		float bottom3Y = min(maxCoord1.y, bottom2Y + Coord(1).w);


		// Get points and saturate out of range values (BTB & WTW)
		// [                c22               ]
		// [           c24, c9,  c23          ]
		// [      c21, c1,  c2,  c3, c18      ]
		// [ c19, c10, c4,  c0,  c5, c11, c16 ]
		// [      c20, c6,  c7,  c8, c17      ]
		// [           c15, c12, c14          ]
		// [                c13               ]
		float c[25] = {
			get1(Coord(1).xy),					// c0
			get1(float2(left1X, top1Y)),		// c1
			get1(float2(Coord(1).x, top1Y)),	// c2
			get1(float2(right1X, top1Y)),	// c3
			get1(float2(left1X, Coord(1).y)),	// c4
			get1(float2(right1X, Coord(1).y)),	// c5
			get1(float2(left1X, bottom1Y)),	// c6
			get1(float2(Coord(1).x, bottom1Y)),	// c7
			get1(float2(right1X, bottom1Y)),	// c8
			get1(float2(Coord(1).x, top2Y)),	// c9
			get1(float2(left2X, Coord(1).y)),	// c10
			get1(float2(right2X, Coord(1).y)),	// c11
			get1(float2(Coord(1).x, bottom2Y)),	// c12
			get1(float2(Coord(1).x, bottom3Y)),	// c13
			get1(float2(right1X, bottom2Y)),	// c14
			get1(float2(left1X, bottom2Y)),	// c15
			get1(float2(right3X, Coord(1).y)),	// c16
			get1(float2(right2X, bottom1Y)),	// c17
			get1(float2(right2X, top1Y)),	// c18
			get1(float2(left3X, Coord(1).y)),	// c19
			get1(float2(left2X, bottom1Y)),	// c20
			get1(float2(left2X, top1Y)),		// c21
			get1(float2(Coord(1).x, top3Y)),	// c22
			get1(float2(right1X, top2Y)),	// c23
			get1(float2(left1X, top2Y))		// c24
		};

		// Blur, gauss 3x3
		float blur = (2 * (c[2] + c[4] + c[5] + c[7]) + (c[1] + c[3] + c[6] + c[8]) + 4 * c[0]) / 16;

		// Edge detection
		// Matrix, relative weights
		// [           1          ]
		// [       4,  4,  4      ]
		// [   1,  4,  4,  4,  1  ]
		// [       4,  4,  4      ]
		// [           1          ]
		float edge = length(abs(blur - c[0]) + abs(blur - c[1]) + abs(blur - c[2]) + abs(blur - c[3])
			+ abs(blur - c[4]) + abs(blur - c[5]) + abs(blur - c[6]) + abs(blur - c[7]) + abs(blur - c[8])
			+ 0.25 * (abs(blur - c[9]) + abs(blur - c[10]) + abs(blur - c[11]) + abs(blur - c[12]))) * (1.0 / 3.0);

		// Edge detect contrast compression, center = 0.5
		edge *= min((0.8 + 2.7 * pow(2, (-7.4 * blur))), 3.2);

		float kernel[25] = c;

		// Partial laplacian outer pixel weighting scheme
		float mdiff_c0 = 0.03 + 4 * (abs(kernel[0] - kernel[2]) + abs(kernel[0] - kernel[4])
			+ abs(kernel[0] - kernel[5]) + abs(kernel[0] - kernel[7])
			+ 0.25 * (abs(kernel[0] - kernel[1]) + abs(kernel[0] - kernel[3])
				+ abs(kernel[0] - kernel[6]) + abs(kernel[0] - kernel[8])));

		float mdiff_c9 = (abs(kernel[9] - kernel[2]) + abs(kernel[9] - kernel[24])
			+ abs(kernel[9] - kernel[23]) + abs(kernel[9] - kernel[22])
			+ 0.5 * (abs(kernel[9] - kernel[1]) + abs(kernel[9] - kernel[3])));

		float mdiff_c10 = (abs(kernel[10] - kernel[20]) + abs(kernel[10] - kernel[19])
			+ abs(kernel[10] - kernel[21]) + abs(kernel[10] - kernel[4])
			+ 0.5 * (abs(kernel[10] - kernel[1]) + abs(kernel[10] - kernel[6])));

		float mdiff_c11 = (abs(kernel[11] - kernel[17]) + abs(kernel[11] - kernel[5])
			+ abs(kernel[11] - kernel[18]) + abs(kernel[11] - kernel[16])
			+ 0.5 * (abs(kernel[11] - kernel[3]) + abs(kernel[11] - kernel[8])));

		float mdiff_c12 = (abs(kernel[12] - kernel[13]) + abs(kernel[12] - kernel[15])
			+ abs(kernel[12] - kernel[7]) + abs(kernel[12] - kernel[14])
			+ 0.5 * (abs(kernel[12] - kernel[6]) + abs(kernel[12] - kernel[8])));

		float4 weights = float4((min((mdiff_c0 / mdiff_c9), 2)), (min((mdiff_c0 / mdiff_c10), 2)),
			(min((mdiff_c0 / mdiff_c11), 2)), (min((mdiff_c0 / mdiff_c12), 2)));

		// Negative laplace matrix
		// Matrix, relative weights, *Varying 0<->8
		// [          8*         ]
		// [      4,  1,  4      ]
		// [  8*, 1,      1,  8* ]
		// [      4,  1,  4      ]
		// [          8*         ]
		float neg_laplace = (0.25 * (kernel[2] + kernel[4] + kernel[5] + kernel[7])
			+ (kernel[1] + kernel[3] + kernel[6] + kernel[8])
			+ ((kernel[9] * weights.x) + (kernel[10] * weights.y)
				+ (kernel[11] * weights.z) + (kernel[12] * weights.w)))
			/ (5 + weights.x + weights.y + weights.z + weights.w);

		// Compute sharpening magnitude function, x = edge mag, y = laplace operator mag
		float sharpen_val = 0.01 + (curveHeight / (curveslope * pow(edge, 3.5) + 0.5))
			- (curveHeight / (8192 * pow((edge * 2.2), 4.5) + 0.5));

		// Calculate sharpening diff and scale
		float sharpdiff = (c[0] - neg_laplace) * (sharpen_val * 0.8);

		// 不知道为什么，下面的代码会严重拖累运行速度，因此重写了算法
		// Calculate local near min & max, partial cocktail sort (No branching!)
		/*for (int i = 0; i < 2; ++i) {
			for (int i1 = 1 + i; i1 < 25 - i; ++i1)     {
				float temp = kernel[i1 - 1];
				kernel[i1 - 1] = min(kernel[i1 - 1], kernel[i1]);
				kernel[i1] = max(temp, kernel[i1]);
			}

			for (int i2 = 23 - i; i2 > i; --i2)     {
				float temp = kernel[i2 - 1];
				kernel[i2 - 1] = min(kernel[i2 - 1], kernel[i2]);
				kernel[i2] = max(temp, kernel[i2]);
			}
		}*/
		float min1 = kernel[0];
		float min2 = min1;
		float max1 = min1;
		float max2 = min1;
		for (int i = 0; i < 25; ++i) {
			float v = kernel[i];
			if (v < min1) {
				min2 = min1;
				min1 = v;
			} else if (v < min2) {
				min2 = v;
			} else if (v > max1) {
				max2 = max1;
				max1 = v;
			} else if (v > max2) {
				max2 = v;
			}
		}

		float nmax = max(((max1 + max2) / 2), c[0]);
		float nmin = min(((min1 + min2) / 2), c[0]);

		// Calculate tanh scale factor, pos/neg
		float nmax_scale = max((1 / ((nmax - c[0]) + L_overshoot)), max_scale_lim);
		float nmin_scale = max((1 / ((c[0] - nmin) + D_overshoot)), max_scale_lim);

		// Soft limit sharpening with tanh, lerp to control maximum compression
		sharpdiff = lerp((tanh((max(sharpdiff, 0)) * nmax_scale) / nmax_scale), (max(sharpdiff, 0)), L_comp_ratio)
			+ lerp((tanh((min(sharpdiff, 0)) * nmin_scale) / nmin_scale), (min(sharpdiff, 0)), D_comp_ratio);


		return c[0] + sharpdiff;
	}
}


D2D_PS_ENTRY(main) {
	InitMagpieSampleInput();
	Coord(0).xy /= 2;
	Coord(1).xy /= 2;
	
	float l = getDiff();

	float3 yuv = SampleInputCur(0).xyz;
	return float4(YUV2RGB(float3(yuv.x + l, yuv.yz)), 1);
}
