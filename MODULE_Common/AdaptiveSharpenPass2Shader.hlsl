// 自适应锐化算法 Pass2
// 移植自 https://github.com/libretro/common-shaders/blob/master/sharpen/shaders/adaptive-sharpen-pass2.cg
//
// Adaptive sharpen
// Tuned for use post resize, EXPECTS FULL RANGE GAMMA LIGHT

cbuffer constants : register(b0) {
	int2 srcSize : packoffset(c0.x);
	float curveHeight : packoffset(c0.z);	// 锐化强度，必须为正值。一般在 0.3~2.0 之间
};


#define MAGPIE_INPUT_COUNT 1
#include "AdaptiveSharpen.hlsli"


//-------------------------------------------------------------------------------------------------
// Defined values under this row are "optimal" DO NOT CHANGE IF YOU DO NOT KNOW WHAT YOU ARE DOING!

#define curveslope      (curveHeight*0.7)   // Sharpening curve slope, high edge values

#define D_overshoot     0.009                // Max dark overshoot before max compression, >0!
#define D_compr_low     0.250                // Max compression ratio, dark overshoot (1/0.250=4x)
#define D_compr_high    0.500                // -||- pixel surrounded by edges (1/0.500=2x)

#define L_overshoot     0.003                // Max light overshoot before max compression, >0!
#define L_compr_low     0.167                // Max compression ratio, light overshoot (1/0.167=6x)
#define L_compr_high    0.334                // -||- pixel surrounded by edges (1/0.334=3x)

#define max_scale_lim   0.1                  // Abs change before max compression (0.1 = +-10%)


// Saturation loss reduction
#define minim_satloss  ((c[0].rgb*(CtL(c[0].rgb+sharpdiff)/c0_Y)+(c[0].rgb+sharpdiff))/2)

// Soft if, fast
#define soft_if(a,b,c) (saturate((a+b+c)/(saturate(maxedge)+0.0067)-0.85))

// Soft limit
#define soft_lim(v,s)  (((exp(2*min(abs(v),s*16)/s)-1)/(exp(2*min(abs(v),s*16)/s)+1))*s)

// Colour to luma, fast approx gamma
#define CtL(RGB)       (sqrt(dot(float3(0.256,0.651,0.093),saturate((RGB).rgb*abs(RGB).rgb))))

// Center pixel diff
#define mdiff(a,b,c,d,e,f,g) (abs(luma[g]-luma[a])+abs(luma[g]-luma[b])+abs(luma[g]-luma[c])+abs(luma[g]-luma[d])+0.5*(abs(luma[g]-luma[e])+abs(luma[g]-luma[f])))



float4 get(float2 pos) {
	float4 s = SampleInput(0, pos);
	return float4(s.xyz, Uncompress(s.w));
}


D2D_PS_ENTRY(main) {
	InitMagpieSampleInput();

	float4 orig0 = get(Coord(0).xy);
	float c_edge = orig0.w;

	// Displays the origin image if the edge data is not inside a valid range in the .w channel
	if (c_edge > 32 || c_edge < -0.5) {
		return float4(orig0.xyz, 1);
	} else {
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

		// Get points, saturate colour data in c[0]
		// [                c22               ]
		// [           c24, c9,  c23          ]
		// [      c21, c1,  c2,  c3, c18      ]
		// [ c19, c10, c4,  c0,  c5, c11, c16 ]
		// [      c20, c6,  c7,  c8, c17      ]
		// [           c15, c12, c14          ]
		// [                c13               ]
		float4 c[25] = {
			orig0,					// c0
			get(float2(left1X, top1Y)),		// c1
			get(float2(Coord(0).x, top1Y)),	// c2
			get(float2(right1X, top1Y)),	// c3
			get(float2(left1X, Coord(0).y)),	// c4
			get(float2(right1X, Coord(0).y)),	// c5
			get(float2(left1X, bottom1Y)),	// c6
			get(float2(Coord(0).x, bottom1Y)),	// c7
			get(float2(right1X, bottom1Y)),	// c8
			get(float2(Coord(0).x, top2Y)),	// c9
			get(float2(left2X, Coord(0).y)),	// c10
			get(float2(right2X, Coord(0).y)),	// c11
			get(float2(Coord(0).x, bottom2Y)),	// c12
			get(float2(Coord(0).x, bottom3Y)),	// c13
			get(float2(right1X, bottom2Y)),	// c14
			get(float2(left1X, bottom2Y)),	// c15
			get(float2(right3X, Coord(0).y)),	// c16
			get(float2(right2X, bottom1Y)),	// c17
			get(float2(right2X, top1Y)),	// c18
			get(float2(left3X, Coord(0).y)),	// c19
			get(float2(left2X, bottom1Y)),	// c20
			get(float2(left2X, top1Y)),		// c21
			get(float2(Coord(0).x, top3Y)),	// c22
			get(float2(right1X, top2Y)),	// c23
			get(float2(left1X, top2Y))		// c24
		};

		// Allow for higher overshoot if the current edge pixel is surrounded by similar edge pixels
		float maxedge = (max4(max4(c[1].w, c[2].w, c[3].w, c[4].w), max4(c[5].w, c[6].w, c[7].w, c[8].w),
			max4(c[9].w, c[10].w, c[11].w, c[12].w), c[0].w));

		// [          x          ]
		// [       z, x, w       ]
		// [    z, z, x, w, w    ]
		// [ y, y, y, 0, y, y, y ]
		// [    w, w, x, z, z    ]
		// [       w, x, z       ]
		// [          x          ]
		float var = soft_if(c[2].w, c[9].w, c[22].w) * soft_if(c[7].w, c[12].w, c[13].w)  // x dir
			+ soft_if(c[4].w, c[10].w, c[19].w) * soft_if(c[5].w, c[11].w, c[16].w)  // y dir
			+ soft_if(c[1].w, c[24].w, c[21].w) * soft_if(c[8].w, c[14].w, c[17].w)  // z dir
			+ soft_if(c[3].w, c[23].w, c[18].w) * soft_if(c[6].w, c[20].w, c[15].w); // w dir

		float s[2] = { lerp(L_compr_low, L_compr_high, smoothstep(2, 3.1, var)),
					   lerp(D_compr_low, D_compr_high, smoothstep(2, 3.1, var)) };

		// RGB to luma
		float c0_Y = CtL(c[0]);

		float luma[25] = { c0_Y, CtL(c[1]), CtL(c[2]), CtL(c[3]), CtL(c[4]), CtL(c[5]), CtL(c[6]),
						   CtL(c[7]),  CtL(c[8]),  CtL(c[9]),  CtL(c[10]), CtL(c[11]), CtL(c[12]),
						   CtL(c[13]), CtL(c[14]), CtL(c[15]), CtL(c[16]), CtL(c[17]), CtL(c[18]),
						   CtL(c[19]), CtL(c[20]), CtL(c[21]), CtL(c[22]), CtL(c[23]), CtL(c[24]) };

		// Precalculated default squared kernel weights
		float3 w1 = float3(0.5, 1.0, 1.41421356237); // 0.25, 1.0, 2.0
		float3 w2 = float3(0.86602540378, 1.0, 0.5477225575);  // 0.75, 1.0, 0.3

		// Transition to a concave kernel if the center edge val is above thr
		float3 dW = pow(lerp(w1, w2, smoothstep(0.3, 0.6, c_edge)), 2);

		float mdiff_c0 = 0.02 + 3 * (abs(luma[0] - luma[2]) + abs(luma[0] - luma[4])
			+ abs(luma[0] - luma[5]) + abs(luma[0] - luma[7])
			+ 0.25 * (abs(luma[0] - luma[1]) + abs(luma[0] - luma[3])
				+ abs(luma[0] - luma[6]) + abs(luma[0] - luma[8])));

		// Use lower weights for pixels in a more active area relative to center pixel area.
		float weights[12] = { (dW.x), (dW.x), (dW.x), (dW.x), // c2, // c4, // c5, // c7
							   (min(mdiff_c0 / mdiff(24, 21, 2,  4,  9,  10, 1),  dW.y)),   // c1
							   (min(mdiff_c0 / mdiff(23, 18, 5,  2,  9,  11, 3),  dW.y)),   // c3
							   (min(mdiff_c0 / mdiff(4,  20, 15, 7,  10, 12, 6),  dW.y)),   // c6
							   (min(mdiff_c0 / mdiff(5,  7,  17, 14, 12, 11, 8),  dW.y)),   // c8
							   (min(mdiff_c0 / mdiff(2,  24, 23, 22, 1,  3,  9),  dW.z)),   // c9
							   (min(mdiff_c0 / mdiff(20, 19, 21, 4,  1,  6,  10), dW.z)),   // c10
							   (min(mdiff_c0 / mdiff(17, 5,  18, 16, 3,  8,  11), dW.z)),   // c11
							   (min(mdiff_c0 / mdiff(13, 15, 7,  14, 6,  8,  12), dW.z)) }; // c12

		weights[4] = (max(max((weights[8] + weights[9]) / 4, weights[4]), 0.25) + weights[4]) / 2;
		weights[5] = (max(max((weights[8] + weights[10]) / 4, weights[5]), 0.25) + weights[5]) / 2;
		weights[6] = (max(max((weights[9] + weights[11]) / 4, weights[6]), 0.25) + weights[6]) / 2;
		weights[7] = (max(max((weights[10] + weights[11]) / 4, weights[7]), 0.25) + weights[7]) / 2;

		// Calculate the negative part of the laplace kernel and the low threshold weight
		float lowthsum = 0;
		float weightsum = 0;
		float neg_laplace = 0;

		int order[12] = { 2, 4, 5, 7, 1, 3, 6, 8, 9, 10, 11, 12 };

		for (int pix = 0; pix < 12; ++pix) {
			float x = saturate((c[order[pix]].w - 0.01) / 0.11);
			float lowth = x * x * (2.99 - 2 * x) + 0.01;

			neg_laplace += pow(abs(luma[order[pix]] + 0.064), 2.4) * (weights[pix] * lowth);
			weightsum += weights[pix] * lowth;
			lowthsum += lowth;
		}

		neg_laplace = pow((neg_laplace / weightsum), (1.0 / 2.4)) - 0.064;

		// Compute sharpening magnitude function
		float sharpen_val = (lowthsum / 12) * (curveHeight / (curveslope * pow(abs(c_edge), 3.5) + 0.5));

		// Calculate sharpening diff and scale
		float sharpdiff = (c0_Y - neg_laplace) * (sharpen_val * 0.8 + 0.01);

		// Calculate local near min & max, partial cocktail sort (No branching!)
		for (int i = 0; i < 3; ++i) {
			for (int i1 = 1 + i; i1 < 25 - i; ++i1) {
				float temp = luma[i1 - 1];
				luma[i1 - 1] = min(luma[i1 - 1], luma[i1]);
				luma[i1] = max(temp, luma[i1]);
			}

			for (int i2 = 23 - i; i2 > i; --i2) {
				float temp = luma[i2 - 1];
				luma[i2 - 1] = min(luma[i2 - 1], luma[i2]);
				luma[i2] = max(temp, luma[i2]);
			}
		}

		float nmax = max(((luma[22] + luma[23] * 2 + luma[24]) / 4), c0_Y);
		float nmin = min(((luma[0] + luma[1] * 2 + luma[2]) / 4), c0_Y);

		// Calculate tanh scale factor, pos/neg
		float nmax_scale = min(((nmax - c0_Y) + L_overshoot), max_scale_lim);
		float nmin_scale = min(((c0_Y - nmin) + D_overshoot), max_scale_lim);

		// Soft limit sharpening with tanh, lerp to control maximum compression
		sharpdiff = lerp((soft_lim(max(sharpdiff, 0), nmax_scale)), max(sharpdiff, 0), s[0])
			+ lerp(-(soft_lim(min(sharpdiff, 0), nmin_scale)), min(sharpdiff, 0), s[1]);

		// Normal path
		if (sharpdiff > 0) {
			return float4(minim_satloss, 1);
		} else {
			return float4((orig0.rgb + sharpdiff), 1);
		}
	}
}
