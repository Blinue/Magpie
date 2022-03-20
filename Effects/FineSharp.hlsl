// FineSharp
// 移植自 https://forum.doom9.org/showthread.php?t=171346

// This is the FineSharp avisynth script by Didée converted to MPC - HC shaders.It was written for madVR(need the 16 - bit accuracy in the shader chain), but also works in MPDN.
//
// The sharpener makes no attempt to filter noise or source artefacts and will sharpen those too.So denoise / clean your source first if necessary.Probably won't work very well on a really old GPU, the weakest I have tried is a GTX 560 at 1080p 60fps with no problems.

//!MAGPIE EFFECT
//!VERSION 2
//!OUTPUT_WIDTH INPUT_WIDTH
//!OUTPUT_HEIGHT INPUT_HEIGHT


//!PARAMETER
//!DEFAULT 2.0
//!MIN 0

// Strength of sharpening, 0.0 up to 8.0 or more. If you change this, then alter cstr below
float sstr;

//!PARAMETER
//!DEFAULT 0.9
//!MIN 0

// Strength of equalisation, 0.0 to 2.0 or more. Suggested settings for cstr based on sstr value: 
// sstr=0->cstr=0, sstr=0.5->cstr=0.1, 1.0->0.6, 2.0->0.9, 2.5->1.00, 3.0->1.09, 3.5->1.15, 4.0->1.19, 8.0->1.249, 255.0->1.5
float cstr;

//!PARAMETER
//!DEFAULT 0.19
//!MIN 0
//!MAX 1

// Strength of XSharpen-style final sharpening, 0.0 to 1.0 (but, better don't go beyond 0.249 ...)
float xstr;

//!PARAMETER
//!DEFAULT 0.25
//!MIN 0

// Repair artefacts from final sharpening, 0.0 to 1.0 or more (-Vit- addition to original script)
float xrep;

//!TEXTURE
Texture2D INPUT;

//!TEXTURE
//!WIDTH OUTPUT_WIDTH
//!HEIGHT OUTPUT_HEIGHT
//!FORMAT R16G16B16A16_FLOAT
Texture2D tex1;

//!TEXTURE
//!WIDTH OUTPUT_WIDTH
//!HEIGHT OUTPUT_HEIGHT
//!FORMAT R16G16B16A16_FLOAT
Texture2D tex2;

//!SAMPLER
//!FILTER POINT
SamplerState sam;


//!PASS 1
//!IN INPUT
//!OUT tex1
//!BLOCK_SIZE 16
//!NUM_THREADS 64


#define RGBtoYUV(Kb,Kr) float3x3(float3(Kr, 1 - Kr - Kb, Kb), float3(-Kr, Kr + Kb - 1, 1 - Kb) / (2*(1 - Kb)), float3(1 - Kr, Kr + Kb - 1, -Kb) / (2*(1 - Kr)))
static const float3x3 RGBtoYUV = GetInputSize().y <= 576 ? RGBtoYUV(0.114, 0.299) : RGBtoYUV(0.0722, 0.2126);

void Pass1(uint2 blockStart, uint3 threadId) {
	uint2 gxy = (Rmp8x8(threadId.x) << 1) + blockStart;
	uint2 inputSize = GetInputSize();
	if (gxy.x >= inputSize.x || gxy.y >= inputSize.y) {
		return;
	}

	float2 inputPt = GetInputPt();
	uint i, j;

	float3 src[4][4];
	[unroll]
	for (i = 0; i < 3; i += 2) {
		[unroll]
		for (j = 0; j < 3; j += 2) {
			float2 tpos = (gxy + uint2(i, j)) * inputPt;
			const float4 sr = INPUT.GatherRed(sam, tpos);
			const float4 sg = INPUT.GatherGreen(sam, tpos);
			const float4 sb = INPUT.GatherBlue(sam, tpos);

			// w z
			// x y
			src[i][j] = mul(RGBtoYUV, float3(sr.w, sg.w, sb.w)) + float3(0, 0.5, 0.5);
			src[i][j + 1] = mul(RGBtoYUV, float3(sr.x, sg.x, sb.x)) + float3(0, 0.5, 0.5);
			src[i + 1][j] = mul(RGBtoYUV, float3(sr.z, sg.z, sb.z)) + float3(0, 0.5, 0.5);
			src[i + 1][j + 1] = mul(RGBtoYUV, float3(sr.y, sg.y, sb.y)) + float3(0, 0.5, 0.5);
		}
	}

	[unroll]
	for (i = 1; i <= 2; ++i) {
		[unroll]
		for (j = 1; j <= 2; ++j) {
			float4 o = src[i][j].rgbr;

			o.x += o.x;
			o.x += src[i][j - 1].x + src[i - 1][j].x + src[i + 1][j].x + src[i][j + 1].x;
			o.x += o.x;
			o.x += src[i - 1][j - 1].x + src[i + 1][j - 1].x + src[i - 1][j + 1].x + src[i + 1][j + 1].x;
			o.x *= 0.0625f;

			tex1[gxy + uint2(i - 1, j - 1)] = o;
		}
	}
}


//!PASS 2
//!IN tex1
//!OUT tex2
//!BLOCK_SIZE 16
//!NUM_THREADS 64

// The variables passed to these median macros will be swapped around as part of the process. A temporary variable t of the same type is also required.
#define sort(a1,a2)                         (t=min(a1,a2),a2=max(a1,a2),a1=t)
#define median3(a1,a2,a3)                   (sort(a2,a3),sort(a1,a2),min(a2,a3))
#define median5(a1,a2,a3,a4,a5)             (sort(a1,a2),sort(a3,a4),sort(a1,a3),sort(a2,a4),median3(a2,a3,a5))
#define median9(a1,a2,a3,a4,a5,a6,a7,a8,a9) (sort(a1,a2),sort(a3,a4),sort(a5,a6),sort(a7,a8),\
											 sort(a1,a3),sort(a5,a7),sort(a1,a5),sort(a3,a5),sort(a3,a7),\
											 sort(a2,a4),sort(a6,a8),sort(a4,a8),sort(a4,a6),sort(a2,a6),median5(a2,a4,a5,a7,a9))

void Pass2(uint2 blockStart, uint3 threadId) {
	uint2 gxy = (Rmp8x8(threadId.x) << 1) + blockStart;
	uint2 inputSize = GetInputSize();
	if (gxy.x >= inputSize.x || gxy.y >= inputSize.y) {
		return;
	}

	float2 inputPt = GetInputPt();
	uint i, j;

	float4 src[4][4];
	[unroll]
	for (i = 0; i < 3; i += 2) {
		[unroll]
		for (j = 0; j < 3; j += 2) {
			float2 tpos = (gxy + uint2(i, j)) * inputPt;
			const float4 sr = tex1.GatherRed(sam, tpos);

			// w z
			// x y
			src[i][j].r = sr.w;
			src[i][j + 1].r = sr.x;
			src[i + 1][j].r = sr.z;
			src[i + 1][j + 1].r = sr.y;
		}
	}

	float2 tpos = (gxy + 1) * inputPt;
	const float4 sg = tex1.GatherGreen(sam, tpos);
	const float4 sb = tex1.GatherBlue(sam, tpos);
	const float4 sa = tex1.GatherAlpha(sam, tpos);
	src[1][1].gba = float3(sg.w, sb.w, sa.w);
	src[1][2].gba = float3(sg.x, sb.x, sa.x);
	src[2][1].gba = float3(sg.z, sb.z, sa.z);
	src[2][2].gba = float3(sg.y, sb.y, sa.y);

	[unroll]
	for (i = 1; i <= 2; ++i) {
		[unroll]
		for (j = 1; j <= 2; ++j) {
			float4 o = src[i][j];

			o.x += o.x;
			o.x += src[i][j - 1].x + src[i - 1][j].x + src[i + 1][j].x + src[i][j + 1].x;
			o.x += o.x;
			o.x += src[i - 1][j - 1].x + src[i + 1][j - 1].x + src[i - 1][j + 1].x + src[i + 1][j + 1].x;
			o.x *= 0.0625f;

			float t;
			float t1 = src[i - 1][j - 1].x;
			float t2 = src[i][j - 1].x;
			float t3 = src[i + 1][j - 1].x;
			float t4 = src[i - 1][j].x;
			float t5 = o.x;
			float t6 = src[i + 1][j].x;
			float t7 = src[i - 1][j + 1].x;
			float t8 = src[i][j + 1].x;
			float t9 = src[i + 1][j + 1].x;
			o.x = median9(t1, t2, t3, t4, t5, t6, t7, t8, t9);

			tex2[gxy + uint2(i - 1, j - 1)] = o;
		}
	}
}

//!PASS 3
//!IN tex2
//!OUT tex1
//!BLOCK_SIZE 16
//!NUM_THREADS 64

#define lstr 1.49  // Modifier for non-linear sharpening
#define pstr 1.272 // Exponent for non-linear sharpening
#define ldmp (sstr+0.1f) // "Low damp", to not over-enhance very small differences (noise coming out of flat areas)

// To use the "mode" setting in original you must change shaders earlier in chain: mode=1->RG11 RG4, mode=2->RG4 RG11, mode=3->RG4 RG11 RG4
// Negative modes are not supported
// XSharpen settings are in Part C

float SharpDiff(float4 c) {
	float t = c.a - c.x;
	return sign(t) * (sstr / 255.0f) * pow(abs(t) / (lstr / 255.0f), 1.0f / pstr) * ((t * t) / (t * t + ldmp / (255.0f * 255.0f)));
}

void Pass3(uint2 blockStart, uint3 threadId) {
	uint2 gxy = (Rmp8x8(threadId.x) << 1) + blockStart;
	uint2 inputSize = GetInputSize();
	if (gxy.x >= inputSize.x || gxy.y >= inputSize.y) {
		return;
	}

	float2 inputPt = GetInputPt();
	uint i, j;

	float4 src[4][4];
	float sharpDiffs[4][4];

	[unroll]
	for (i = 0; i < 3; i += 2) {
		[unroll]
		for (j = 0; j < 3; j += 2) {
			float2 tpos = (gxy + uint2(i, j)) * inputPt;
			const float4 sr = tex2.GatherRed(sam, tpos);
			const float4 sa = tex2.GatherAlpha(sam, tpos);

			// w z
			// x y
			src[i][j].ra = float2(sr.w, sa.w);
			src[i][j + 1].ra = float2(sr.x, sa.x);
			src[i + 1][j].ra = float2(sr.z, sa.z);
			src[i + 1][j + 1].ra = float2(sr.y, sa.y);

			sharpDiffs[i][j] = SharpDiff(src[i][j]);
			sharpDiffs[i][j + 1] = SharpDiff(src[i][j + 1]);
			sharpDiffs[i + 1][j] = SharpDiff(src[i + 1][j]);
			sharpDiffs[i + 1][j + 1] = SharpDiff(src[i + 1][j + 1]);
		}
	}

	float2 tpos = (gxy + 1) * inputPt;
	const float4 sg = tex2.GatherGreen(sam, tpos);
	const float4 sb = tex2.GatherBlue(sam, tpos);
	src[1][1].gb = float2(sg.w, sb.w);
	src[1][2].gb = float2(sg.x, sb.x);
	src[2][1].gb = float2(sg.z, sb.z);
	src[2][2].gb = float2(sg.y, sb.y);

	[unroll]
	for (i = 1; i <= 2; ++i) {
		[unroll]
		for (j = 1; j <= 2; ++j) {
			float4 o = src[i][j];

			float sd = sharpDiffs[i][j];
			o.x = o.a + sd;
			sd += sd;
			sd += sharpDiffs[i][j - 1] + sharpDiffs[i - 1][j] + sharpDiffs[i + 1][j] + sharpDiffs[i][j + 1];
			sd += sd;
			sd += sharpDiffs[i - 1][j - 1] + sharpDiffs[i + 1][j - 1] + sharpDiffs[i - 1][j + 1] + sharpDiffs[i + 1][j + 1];
			sd *= 0.0625f;
			o.x -= cstr * sd;
			o.a = o.x;

			tex1[gxy + uint2(i - 1, j - 1)] = o;
		}
	}
}


//!PASS 4
//!IN tex1
//!OUT tex2
//!BLOCK_SIZE 16
//!NUM_THREADS 64

// The variables passed to these sorting macros will be swapped around as part of the process. A temporary variable t of the same type is also required.
#define sort(a1,a2)                               (t=min(a1,a2),a2=max(a1,a2),a1=t)
#define sort_min_max3(a1,a2,a3)                   (sort(a1,a2),sort(a1,a3),sort(a2,a3))
#define sort_min_max5(a1,a2,a3,a4,a5)             (sort(a1,a2),sort(a3,a4),sort(a1,a3),sort(a2,a4),sort(a1,a5),sort(a4,a5))
#define sort_min_max7(a1,a2,a3,a4,a5,a6,a7)       (sort(a1,a2),sort(a3,a4),sort(a5,a6),sort(a1,a3),sort(a1,a5),sort(a2,a6),sort(a4,a5),sort(a1,a7),sort(a6,a7))
#define sort_min_max9(a1,a2,a3,a4,a5,a6,a7,a8,a9) (sort(a1,a2),sort(a3,a4),sort(a5,a6),sort(a7,a8),sort(a1,a3),sort(a5,a7),sort(a1,a5),sort(a2,a4),sort(a6,a7),sort(a4,a8),sort(a1,a9),sort(a8,a9))
// sort9_partial1 only sorts the min and max into place (at the ends), sort9_partial2 sorts the top two max and min values, etc. Used for avisynth "Repair" script equivalent 
#define sort9_partial1(a1,a2,a3,a4,a5,a6,a7,a8,a9) (sort_min_max9(a1,a2,a3,a4,a5,a6,a7,a8,a9))
#define sort9_partial2(a1,a2,a3,a4,a5,a6,a7,a8,a9) (sort_min_max9(a1,a2,a3,a4,a5,a6,a7,a8,a9),sort_min_max7(a2,a3,a4,a5,a6,a7,a8))
#define sort9_partial3(a1,a2,a3,a4,a5,a6,a7,a8,a9) (sort_min_max9(a1,a2,a3,a4,a5,a6,a7,a8,a9),sort_min_max7(a2,a3,a4,a5,a6,a7,a8),sort_min_max5(a3,a4,a5,a6,a7))
#define sort9(a1,a2,a3,a4,a5,a6,a7,a8,a9)          (sort_min_max9(a1,a2,a3,a4,a5,a6,a7,a8,a9),sort_min_max7(a2,a3,a4,a5,a6,a7,a8),sort_min_max5(a3,a4,a5,a6,a7),sort_min_max3(a4,a5,a6))


void Pass4(uint2 blockStart, uint3 threadId) {
	uint2 gxy = (Rmp8x8(threadId.x) << 1) + blockStart;
	uint2 inputSize = GetInputSize();
	if (gxy.x >= inputSize.x || gxy.y >= inputSize.y) {
		return;
	}

	float2 inputPt = GetInputPt();
	uint i, j;

	float4 src[4][4];
	[unroll]
	for (i = 0; i < 3; i += 2) {
		[unroll]
		for (j = 0; j < 3; j += 2) {
			float2 tpos = (gxy + uint2(i, j)) * inputPt;
			const float4 sa = tex1.GatherAlpha(sam, tpos);

			// w z
			// x y
			src[i][j].a = sa.w;
			src[i][j + 1].a = sa.x;
			src[i + 1][j].a = sa.z;
			src[i + 1][j + 1].a = sa.y;
		}
	}

	float2 tpos = (gxy + 1) * inputPt;
	const float4 sr = tex1.GatherRed(sam, tpos);
	const float4 sg = tex1.GatherGreen(sam, tpos);
	const float4 sb = tex1.GatherBlue(sam, tpos);
	src[1][1].rgb = float3(sr.w, sg.w, sb.w);
	src[1][2].rgb = float3(sr.x, sg.x, sb.x);
	src[2][1].rgb = float3(sr.z, sg.z, sb.z);
	src[2][2].rgb = float3(sr.y, sg.y, sb.y);

	[unroll]
	for (i = 1; i <= 2; ++i) {
		[unroll]
		for (j = 1; j <= 2; ++j) {
			float4 o = src[i][j];

			float t;
			float t1 = src[i - 1][j - 1].a;
			float t2 = src[i][j - 1].a;
			float t3 = src[i + 1][j - 1].a;
			float t4 = src[i - 1][j].a;
			float t5 = o.a;
			float t6 = src[i + 1][j].a;
			float t7 = src[i - 1][j + 1].a;
			float t8 = src[i][j + 1].a;
			float t9 = src[i + 1][j + 1].a;

			o.x += t1 + t2 + t3 + t4 + t6 + t7 + t8 + t9;
			o.x /= 9.0f;
			o.x = o.a + 9.9f * (o.a - o.x);

			sort9_partial2(t1, t2, t3, t4, t5, t6, t7, t8, t9);
			o.x = max(o.x, min(t2, o.a));
			o.x = min(o.x, max(t8, o.a));

			tex2[gxy + uint2(i - 1, j - 1)] = o;
		}
	}
}


//!PASS 5
//!IN tex2
//!BLOCK_SIZE 16
//!NUM_THREADS 64


#define YUVtoRGB(Kb,Kr) float3x3(float3(1, 0, 2*(1 - Kr)), float3(Kb + Kr - 1, 2*(1 - Kb)*Kb, 2*Kr*(1 - Kr)) / (Kb + Kr - 1), float3(1, 2*(1 - Kb),0))
static const float3x3 YUVtoRGB = GetInputSize().y <= 576 ? YUVtoRGB(0.114, 0.299) : YUVtoRGB(0.0722, 0.2126);


void Pass5(uint2 blockStart, uint3 threadId) {
	uint2 gxy = (Rmp8x8(threadId.x) << 1) + blockStart;
	if (!CheckViewport(gxy)) {
		return;
	}

	float2 inputPt = GetInputPt();
	uint i, j;

	float4 src[4][4];
	[unroll]
	for (i = 0; i < 3; i += 2) {
		[unroll]
		for (j = 0; j < 3; j += 2) {
			float2 tpos = (gxy + uint2(i, j)) * inputPt;
			const float4 sr = tex2.GatherRed(sam, tpos);

			// w z
			// x y
			src[i][j].r = sr.w;
			src[i][j + 1].r = sr.x;
			src[i + 1][j].r = sr.z;
			src[i + 1][j + 1].r = sr.y;
		}
	}

	float2 tpos = (gxy + 1) * inputPt;
	const float4 sg = tex2.GatherGreen(sam, tpos);
	const float4 sb = tex2.GatherBlue(sam, tpos);
	const float4 sa = tex2.GatherAlpha(sam, tpos);
	src[1][1].gba = float3(sg.w, sb.w, sa.w);
	src[1][2].gba = float3(sg.x, sb.x, sa.x);
	src[2][1].gba = float3(sg.z, sb.z, sa.z);
	src[2][2].gba = float3(sg.y, sb.y, sa.y);

	[unroll]
	for (i = 1; i <= 2; ++i) {
		[unroll]
		for (j = 1; j <= 2; ++j) {
			uint2 destPos = gxy + uint2(i - 1, j - 1);

			if (i != 1 && j != 1) {
				if (!CheckViewport(destPos)) {
					continue;
				}
			}
			
			float4 o = src[i][j];

			float edge = abs(src[i][j - 1].x + src[i - 1][j].x + src[i + 1][j].x + src[i][j + 1].x - 4 * o.x);
			o.x = lerp(o.a, o.x, xstr * (1 - saturate(edge * xrep)));

			WriteToOutput(destPos, mul(YUVtoRGB, o.xyz - float3(0.0, 0.5, 0.5)));
		}
	}
}
