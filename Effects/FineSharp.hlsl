// FineSharp
// 移植自 https://forum.doom9.org/showthread.php?t=171346

// This is the FineSharp avisynth script by Didée converted to MPC - HC shaders.It was written for madVR(need the 16 - bit accuracy in the shader chain), but also works in MPDN.
//
// The sharpener makes no attempt to filter noise or source artefacts and will sharpen those too.So denoise / clean your source first if necessary.Probably won't work very well on a really old GPU, the weakest I have tried is a GTX 560 at 1080p 60fps with no problems.

//!MAGPIE EFFECT
//!VERSION 1
//!OUTPUT_WIDTH INPUT_WIDTH
//!OUTPUT_HEIGHT INPUT_HEIGHT


//!CONSTANT
//!VALUE INPUT_PT_X
float inputPtX;

//!CONSTANT
//!VALUE INPUT_PT_Y
float inputPtY;

//!CONSTANT
//!VALUE INPUT_HEIGHT
float inputHeight;

//!CONSTANT
//!DEFAULT 2.0
//!MIN 0

// Strength of sharpening, 0.0 up to 8.0 or more. If you change this, then alter cstr below
float sstr;

//!CONSTANT
//!DEFAULT 0.9
//!MIN 0

// Strength of equalisation, 0.0 to 2.0 or more. Suggested settings for cstr based on sstr value: 
// sstr=0->cstr=0, sstr=0.5->cstr=0.1, 1.0->0.6, 2.0->0.9, 2.5->1.00, 3.0->1.09, 3.5->1.15, 4.0->1.19, 8.0->1.249, 255.0->1.5
float cstr;

//!CONSTANT
//!DEFAULT 0.19
//!MIN 0
//!MAX 1

// Strength of XSharpen-style final sharpening, 0.0 to 1.0 (but, better don't go beyond 0.249 ...)
float xstr;

//!CONSTANT
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

//!COMMON

#define Src(tex, a, b) tex.Sample(sam, pos + float2(a * inputPtX, b * inputPtY))


//!PASS 1
//!BIND INPUT
//!SAVE tex1

#define RGBtoYUV(Kb,Kr) float3x3(float3(Kr, 1 - Kr - Kb, Kb), float3(-Kr, Kr + Kb - 1, 1 - Kb) / (2*(1 - Kb)), float3(1 - Kr, Kr + Kb - 1, -Kb) / (2*(1 - Kr)))

static const float3x3 RGBtoYUV = inputHeight <= 576 ? RGBtoYUV(0.114, 0.299) : RGBtoYUV(0.0722, 0.2126);

float4 Pass1(float2 pos) {
	float3 yuv = mul(RGBtoYUV, INPUT.Sample(sam, pos).rgb) + float3(0.0, 0.5, 0.5);
	return float4(yuv, yuv.x);
}

//!PASS 2
//!BIND tex1
//!SAVE tex2

float4 Pass2(float2 pos) {
	float4 o = Src(tex1, 0, 0);

	o.x += o.x;
	o.x += Src(tex1, 0, -1).x + Src(tex1, -1, 0).x + Src(tex1, 1, 0).x + Src(tex1, 0, 1).x;
	o.x += o.x;
	o.x += Src(tex1, -1, -1).x + Src(tex1, 1, -1).x + Src(tex1, -1, 1).x + Src(tex1, 1, 1).x;
	o.x *= 0.0625f;

	return o;
}

//!PASS 3
//!BIND tex2
//!SAVE tex1

// The variables passed to these median macros will be swapped around as part of the process. A temporary variable t of the same type is also required.
#define sort(a1,a2)                         (t=min(a1,a2),a2=max(a1,a2),a1=t)
#define median3(a1,a2,a3)                   (sort(a2,a3),sort(a1,a2),min(a2,a3))
#define median5(a1,a2,a3,a4,a5)             (sort(a1,a2),sort(a3,a4),sort(a1,a3),sort(a2,a4),median3(a2,a3,a5))
#define median9(a1,a2,a3,a4,a5,a6,a7,a8,a9) (sort(a1,a2),sort(a3,a4),sort(a5,a6),sort(a7,a8),\
											 sort(a1,a3),sort(a5,a7),sort(a1,a5),sort(a3,a5),sort(a3,a7),\
											 sort(a2,a4),sort(a6,a8),sort(a4,a8),sort(a4,a6),sort(a2,a6),median5(a2,a4,a5,a7,a9))

float4 Pass3(float2 pos) {
	float4 o = Src(tex2, 0, 0);

	float t;
	float t1 = Src(tex2, -1, -1).x;
	float t2 = Src(tex2, 0, -1).x;
	float t3 = Src(tex2, 1, -1).x;
	float t4 = Src(tex2, -1, 0).x;
	float t5 = o.x;
	float t6 = Src(tex2, 1, 0).x;
	float t7 = Src(tex2, -1, 1).x;
	float t8 = Src(tex2, 0, 1).x;
	float t9 = Src(tex2, 1, 1).x;
	o.x = median9(t1, t2, t3, t4, t5, t6, t7, t8, t9);

	return o;
}

//!PASS 4
//!BIND tex1
//!SAVE tex2


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

float4 Pass4(float2 pos) {
	float4 o = Src(tex1, 0, 0);

	float sd = SharpDiff(o);
	o.x = o.a + sd;
	sd += sd;
	sd += SharpDiff(Src(tex1, 0, -1)) + SharpDiff(Src(tex1, -1, 0)) + SharpDiff(Src(tex1, 1, 0)) + SharpDiff(Src(tex1, 0, 1));
	sd += sd;
	sd += SharpDiff(Src(tex1, -1, -1)) + SharpDiff(Src(tex1, 1, -1)) + SharpDiff(Src(tex1, -1, 1)) + SharpDiff(Src(tex1, 1, 1));
	sd *= 0.0625f;
	o.x -= cstr * sd;
	o.a = o.x;

	return o;
}

//!PASS 5
//!BIND tex2
//!SAVE tex1

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


float4 Pass5(float2 pos) {
	float4 o = Src(tex2, 0, 0);

	float t;
	float t1 = Src(tex2, -1, -1).a;
	float t2 = Src(tex2, 0, -1).a;
	float t3 = Src(tex2, 1, -1).a;
	float t4 = Src(tex2, -1, 0).a;
	float t5 = o.a;
	float t6 = Src(tex2, 1, 0).a;
	float t7 = Src(tex2, -1, 1).a;
	float t8 = Src(tex2, 0, 1).a;
	float t9 = Src(tex2, 1, 1).a;

	o.x += t1 + t2 + t3 + t4 + t6 + t7 + t8 + t9;
	o.x /= 9.0f;
	o.x = o.a + 9.9f * (o.a - o.x);

	sort9_partial2(t1, t2, t3, t4, t5, t6, t7, t8, t9);
	o.x = max(o.x, min(t2, o.a));
	o.x = min(o.x, max(t8, o.a));

	return o;
}

//!PASS 6
//!BIND tex1

#define YUVtoRGB(Kb,Kr) float3x3(float3(1, 0, 2*(1 - Kr)), float3(Kb + Kr - 1, 2*(1 - Kb)*Kb, 2*Kr*(1 - Kr)) / (Kb + Kr - 1), float3(1, 2*(1 - Kb),0))

static const float3x3 YUVtoRGB = inputHeight <= 576 ? YUVtoRGB(0.114, 0.299) : YUVtoRGB(0.0722, 0.2126);


float4 Pass6(float2 pos) {
	float4 o = Src(tex1, 0, 0);

	float edge = abs(Src(tex1, 0, -1).x + Src(tex1, -1, 0).x + Src(tex1, 1, 0).x + Src(tex1, 0, 1).x - 4 * o.x);
	o.x = lerp(o.a, o.x, xstr * (1 - saturate(edge * xrep)));

	o.rgb = mul(YUVtoRGB, o.xyz - float3(0.0, 0.5, 0.5));

	return o;
}
