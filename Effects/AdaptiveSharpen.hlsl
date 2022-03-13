// 自适应锐化算法
// 移植自 https://github.com/libretro/slang-shaders/blob/master/sharpen/shaders/adaptive-sharpen.slang
// 
// Adaptive sharpen - version 2015-05-15 - (requires ps >= 3.0)
// Tuned for use post resize, EXPECTS FULL RANGE GAMMA LIGHT


//!MAGPIE EFFECT
//!VERSION 2
//!OUTPUT_WIDTH INPUT_WIDTH
//!OUTPUT_HEIGHT INPUT_HEIGHT



//!PARAMETER
//!DEFAULT 0.8
//!MIN 1e-5

// Main control of sharpening strength [>0]
// 0.3 <-> 2.0 is a reasonable range of values
float curveHeight;

//!TEXTURE
Texture2D INPUT;

//!SAMPLER
//!FILTER POINT
SamplerState sam;


//!PASS 1
//!IN INPUT
//!BLOCK_SIZE 16
//!NUM_THREADS 64

// Defined values under this row are "optimal" DO NOT CHANGE IF YOU DO NOT KNOW WHAT YOU ARE DOING!

#define curveslope      (curveHeight*1.5f)   // Sharpening curve slope, edge region
#define D_overshoot     0.016f                // Max dark overshoot before max compression
#define D_comp_ratio    0.250f                // Max compression ratio, dark overshoot (1/0.25=4x)
#define L_overshoot     0.004f                // Max light overshoot before max compression
#define L_comp_ratio    0.167f                // Max compression ratio, light overshoot (1/0.167=6x)
#define max_scale_lim   10.0f                 // Abs change before max compression (1/10=±10%)

// Colour to greyscale, fast approx gamma
float CtG(float3 RGB) { return  sqrt((1.0f / 3.0f) * ((RGB * RGB).r + (RGB * RGB).g + (RGB * RGB).b)); }


void Pass1(uint2 blockStart, uint3 threadId) {
	uint2 gxy = (Rmp8x8(threadId.x) << 1) + blockStart;
	if (!CheckViewport(gxy)) {
		return;
	}

	float2 inputPt = GetInputPt();
	int i, j;

	float4 src[8][8];
	[unroll]
	for (i = 0; i <= 6; i += 2) {
		[unroll]
		for (j = 0; j <= 6; j += 2) {
			// 四角共 16 个纹素无需采样
			if ((i == 0 && j == 0) || (i == 6 && j == 0) || (i == 0 && j == 6) || (i == 6 && j == 6)) {
				continue;
			}

			float2 tpos = ((int2)gxy + int2(i, j) - 2) * inputPt;
			const float4 sr = INPUT.GatherRed(sam, tpos);
			const float4 sg = INPUT.GatherGreen(sam, tpos);
			const float4 sb = INPUT.GatherBlue(sam, tpos);

			// w z
			// x y
			src[i][j].rgb = float3(sr.w, sg.w, sb.w);
			src[i][j].w = CtG(src[i][j].rgb);
			src[i][j + 1].rgb = float3(sr.x, sg.x, sb.x);
			src[i][j + 1].w = CtG(src[i][j + 1].rgb);
			src[i + 1][j].rgb = float3(sr.z, sg.z, sb.z);
			src[i + 1][j].w = CtG(src[i + 1][j].rgb);
			src[i + 1][j + 1].rgb = float3(sr.y, sg.y, sb.y);
			src[i + 1][j + 1].w = CtG(src[i + 1][j + 1].rgb);
		}
	}

	[unroll]
	for (i = 0; i <= 1; ++i) {
		[unroll]
		for (j = 0; j <= 1; ++j) {
			const uint2 destPos = gxy + uint2(i, j);

			if (i != 0 && j != 0) {
				if (!CheckViewport(destPos)) {
					continue;
				}
			}

			float2 pos = (destPos + 0.5f) * inputPt;

			// Get points and saturate out of range values (BTB & WTW)
			// [                c22               ]
			// [           c24, c9,  c23          ]
			// [      c21, c1,  c2,  c3, c18      ]
			// [ c19, c10, c4,  c0,  c5, c11, c16 ]
			// [      c20, c6,  c7,  c8, c17      ]
			// [           c15, c12, c14          ]
			// [                c13               ]

			// Blur, gauss 3x3
			float3	blur = (2 * (src[i + 3][j + 2].rgb + src[i + 2][j + 3].rgb + src[i + 4][j + 3].rgb + src[i + 3][j + 4].rgb) + (src[i + 2][j + 2].rgb + src[i + 4][j + 2].rgb + src[i + 2][j + 4].rgb + src[i + 4][j + 4].rgb) + 4 * src[i + 3][j + 3].rgb) / 16;
			float	blur_Y = (blur.r * (1.0 / 3.0) + blur.g * (1.0 / 3.0) + blur.b * (1.0 / 3.0));

			// Edge detection
			// Matrix, relative weights
			// [           1          ]
			// [       4,  4,  4      ]
			// [   1,  4,  4,  4,  1  ]
			// [       4,  4,  4      ]
			// [           1          ]
			float	edge = length(abs(blur - src[i + 3][j + 3].rgb) + abs(blur - src[i + 2][j + 2].rgb) + abs(blur - src[i + 3][j + 2].rgb) + abs(blur - src[i + 4][j + 2].rgb)
				+ abs(blur - src[i + 2][j + 3].rgb) + abs(blur - src[i + 4][j + 3].rgb) + abs(blur - src[i + 2][j + 4].rgb) + abs(blur - src[i + 3][j + 4].rgb) + abs(blur - src[i + 4][j + 4].rgb)
				+ 0.25 * (abs(blur - src[i + 3][j + 1].rgb) + abs(blur - src[i + 1][j + 3].rgb) + abs(blur - src[i + 5][j + 3].rgb) + abs(blur - src[i + 3][j + 5].rgb))) * (1.0 / 3.0);

			// Edge detect contrast compression, center = 0.5
			edge *= min((0.8 + 2.7 * pow(2, (-7.4 * blur_Y))), 3.2);

			// RGB to greyscale
			float	c0_Y = src[i + 3][j + 3].w;

			float	kernel[25] = { c0_Y,  src[i + 2][j + 2].w, src[i + 3][j + 2].w, src[i + 4][j + 2].w,src[i + 2][j + 3].w, src[i + 4][j + 3].w, src[i + 2][j + 4].w, src[i + 3][j + 4].w, src[i + 4][j + 4].w,
									src[i + 3][j + 1].w, src[i + 1][j + 3].w, src[i + 5][j + 3].w, src[i + 3][j + 5].w, src[i + 3][j + 6].w, src[i + 4][j + 5].w, src[i + 2][j + 5].w, src[i + 6][j + 3].w,
									src[i + 5][j + 4].w, src[i + 5][j + 2].w, src[i][j + 3].w, src[i + 1][j + 4].w, src[i + 1][j + 2].w, src[i + 3][j].w, src[i + 4][j + 1].w, src[i + 2][j + 1].w };

			// Partial laplacian outer pixel weighting scheme
			float	mdiff_c0 = 0.03 + 4 * (abs(kernel[0] - kernel[2]) + abs(kernel[0] - kernel[4])
				+ abs(kernel[0] - kernel[5]) + abs(kernel[0] - kernel[7])
				+ 0.25 * (abs(kernel[0] - kernel[1]) + abs(kernel[0] - kernel[3])
					+ abs(kernel[0] - kernel[6]) + abs(kernel[0] - kernel[8])));

			float	mdiff_c9 = (abs(kernel[9] - kernel[2]) + abs(kernel[9] - kernel[24])
				+ abs(kernel[9] - kernel[23]) + abs(kernel[9] - kernel[22])
				+ 0.5 * (abs(kernel[9] - kernel[1]) + abs(kernel[9] - kernel[3])));

			float	mdiff_c10 = (abs(kernel[10] - kernel[20]) + abs(kernel[10] - kernel[19])
				+ abs(kernel[10] - kernel[21]) + abs(kernel[10] - kernel[4])
				+ 0.5 * (abs(kernel[10] - kernel[1]) + abs(kernel[10] - kernel[6])));

			float	mdiff_c11 = (abs(kernel[11] - kernel[17]) + abs(kernel[11] - kernel[5])
				+ abs(kernel[11] - kernel[18]) + abs(kernel[11] - kernel[16])
				+ 0.5 * (abs(kernel[11] - kernel[3]) + abs(kernel[11] - kernel[8])));

			float	mdiff_c12 = (abs(kernel[12] - kernel[13]) + abs(kernel[12] - kernel[15])
				+ abs(kernel[12] - kernel[7]) + abs(kernel[12] - kernel[14])
				+ 0.5 * (abs(kernel[12] - kernel[6]) + abs(kernel[12] - kernel[8])));

			float4	weights = float4((min((mdiff_c0 / mdiff_c9), 2.0)), (min((mdiff_c0 / mdiff_c10), 2.0)),
				(min((mdiff_c0 / mdiff_c11), 2.0)), (min((mdiff_c0 / mdiff_c12), 2.0)));

			// Negative laplace matrix
			 // Matrix, relative weights, *Varying 0<->8
			 // [          8*         ]
			 // [      4,  1,  4      ]
			 // [  8*, 1,      1,  8* ]
			 // [      4,  1,  4      ]
			 // [          8*         ]
			float	neg_laplace = (0.25 * (kernel[2] + kernel[4] + kernel[5] + kernel[7])
				+ (kernel[1] + kernel[3] + kernel[6] + kernel[8])
				+ ((kernel[9] * weights.x) + (kernel[10] * weights.y)
					+ (kernel[11] * weights.z) + (kernel[12] * weights.w)))
				/ (5 + weights.x + weights.y + weights.z + weights.w);

			// Compute sharpening magnitude function, x = edge mag, y = laplace operator mag
			float	sharpen_val = 0.01 + (curveHeight / (curveslope * pow(edge, 3.5) + 0.5))
				- (curveHeight / (8192 * pow((edge * 2.2), 4.5) + 0.5));

			// Calculate sharpening diff and scale
			float	sharpdiff = (c0_Y - neg_laplace) * (sharpen_val * 0.8);

			// Calculate local near min & max, partial cocktail sort (No branching!)
			[unroll]
			for (int k = 0; k < 2; ++k) {
				[unroll]
				for (int i1 = 1 + k; i1 < 25 - k; ++i1) {
					float temp = kernel[i1 - 1];
					kernel[i1 - 1] = min(kernel[i1 - 1], kernel[i1]);
					kernel[i1] = max(temp, kernel[i1]);
				}

				[unroll]
				for (int i2 = 23 - k; i2 > k; --i2) {
					float temp = kernel[i2 - 1];
					kernel[i2 - 1] = min(kernel[i2 - 1], kernel[i2]);
					kernel[i2] = max(temp, kernel[i2]);
				}
			}

			float	nmax = max(((kernel[23] + kernel[24]) / 2), c0_Y);
			float	nmin = min(((kernel[0] + kernel[1]) / 2), c0_Y);

			// Calculate tanh scale factor, pos/neg
			float	nmax_scale = max((1 / ((nmax - c0_Y) + L_overshoot)), max_scale_lim);
			float	nmin_scale = max((1 / ((c0_Y - nmin) + D_overshoot)), max_scale_lim);

			// Soft limit sharpening with tanh, mix to control maximum compression
			sharpdiff = lerp((tanh((max(sharpdiff, 0.0)) * nmax_scale) / nmax_scale), (max(sharpdiff, 0.0)), L_comp_ratio)
				+ lerp((tanh((min(sharpdiff, 0.0)) * nmin_scale) / nmin_scale), (min(sharpdiff, 0.0)), D_comp_ratio);

			WriteToOutput(destPos, src[i + 3][j + 3].rgb + sharpdiff);
		}
	}
}
