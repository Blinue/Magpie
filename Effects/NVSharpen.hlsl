//!MAGPIE EFFECT
//!VERSION 1


//!CONSTANT
//!VALUE INPUT_PT_X
float inputPtX;

//!CONSTANT
//!VALUE INPUT_PT_Y
float inputPtY;

//!TEXTURE
Texture2D INPUT;

//!SAMPLER
//!FILTER LINEAR
SamplerState samLinear;

//!SAMPLER
//!FILTER POINT
SamplerState samPoint;

//!CONSTANT
//!DEFAULT 0.5
//!MIN 0
//!MAX 1
float sharpness;


//!PASS 1
//!BIND INPUT

#define kDetectRatio (1127.f / 1024.f)
#define kDetectThres (64.0f / 1024.0f)
#define kEps 1.0f
#define NIS_SCALE_FLOAT 1.0f
#define kMinContrastRatio 2.0f
#define kMaxContrastRatio 5.0f
#define kRatioNorm (1.0f / (kMaxContrastRatio - kMinContrastRatio))
#define kContrastBoost 1.0f
#define kSharpStartY 0.45f
#define kSharpEndY 0.9f
#define kSharpScaleY (1.0f / (kSharpEndY - kSharpStartY))
#define sharpen_slider (sharpness - 0.5f)
#define MinScale ((sharpen_slider >= 0.0f) ? 1.25f : 1.0f)
#define MaxScale ((sharpen_slider >= 0.0f) ? 1.25f : 1.75f)
#define kSharpStrengthMin max(0.0f, 0.4f + sharpen_slider * MinScale * 1.1f)
#define kSharpStrengthMax (2.2f + sharpen_slider * MaxScale * 1.8f)
#define kSharpStrengthScale (kSharpStrengthMax - kSharpStrengthMin)
#define kSharpLimitMin max(0.06f, 0.10f + sharpen_slider * LimitScale * 0.28f)
#define kSharpLimitMax (0.6f + sharpen_slider * LimitScale * 0.6f)
#define kSharpLimitScale (kSharpLimitMax - kSharpLimitMin)
#define LimitScale ((sharpen_slider >= 0.0f) ? 1.25f : 1.0f)
#define kSupportSize 6


float getY(float3 rgba) {
	return 0.2126f * rgba.x + 0.7152f * rgba.y + 0.0722f * rgba.z;
}

float4 GetEdgeMap(float p[5][5], int i, int j) {
	const float g_0 = abs(p[0 + i][0 + j] + p[0 + i][1 + j] + p[0 + i][2 + j] - p[2 + i][0 + j] - p[2 + i][1 + j] - p[2 + i][2 + j]);
	const float g_45 = abs(p[1 + i][0 + j] + p[0 + i][0 + j] + p[0 + i][1 + j] - p[2 + i][1 + j] - p[2 + i][2 + j] - p[1 + i][2 + j]);
	const float g_90 = abs(p[0 + i][0 + j] + p[1 + i][0 + j] + p[2 + i][0 + j] - p[0 + i][2 + j] - p[1 + i][2 + j] - p[2 + i][2 + j]);
	const float g_135 = abs(p[1 + i][0 + j] + p[2 + i][0 + j] + p[2 + i][1 + j] - p[0 + i][1 + j] - p[0 + i][2 + j] - p[1 + i][2 + j]);

	const float g_0_90_max = max(g_0, g_90);
	const float g_0_90_min = min(g_0, g_90);
	const float g_45_135_max = max(g_45, g_135);
	const float g_45_135_min = min(g_45, g_135);

	float e_0_90 = 0;
	float e_45_135 = 0;

	float edge_0 = 0;
	float edge_45 = 0;
	float edge_90 = 0;
	float edge_135 = 0;

	if ((g_0_90_max + g_45_135_max) == 0) {
		e_0_90 = 0;
		e_45_135 = 0;
	} else {
		e_0_90 = g_0_90_max / (g_0_90_max + g_45_135_max);
		e_0_90 = min(e_0_90, 1.0f);
		e_45_135 = 1.0f - e_0_90;
	}

	if ((g_0_90_max > (g_0_90_min * kDetectRatio)) && (g_0_90_max > kDetectThres) && (g_0_90_max > g_45_135_min)) {
		if (g_0_90_max == g_0) {
			edge_0 = 1.0f;
			edge_90 = 0;
		} else {
			edge_0 = 0;
			edge_90 = 1.0f;
		}
	} else {
		edge_0 = 0;
		edge_90 = 0;
	}

	if ((g_45_135_max > (g_45_135_min * kDetectRatio)) && (g_45_135_max > kDetectThres) &&
		(g_45_135_max > g_0_90_min)) {

		if (g_45_135_max == g_45) {
			edge_45 = 1.0f;
			edge_135 = 0;
		} else {
			edge_45 = 0;
			edge_135 = 1.0f;
		}
	} else {
		edge_45 = 0;
		edge_135 = 0;
	}

	float weight_0, weight_90, weight_45, weight_135;
	if ((edge_0 + edge_90 + edge_45 + edge_135) >= 2.0f) {
		if (edge_0 == 1.0f) {
			weight_0 = e_0_90;
			weight_90 = 0;
		} else {
			weight_0 = 0;
			weight_90 = e_0_90;
		}

		if (edge_45 == 1.0f) {
			weight_45 = e_45_135;
			weight_135 = 0;
		} else {
			weight_45 = 0;
			weight_135 = e_45_135;
		}
	} else if ((edge_0 + edge_90 + edge_45 + edge_135) >= 1.0f) {
		weight_0 = edge_0;
		weight_90 = edge_90;
		weight_45 = edge_45;
		weight_135 = edge_135;
	} else {
		weight_0 = 0;
		weight_90 = 0;
		weight_45 = 0;
		weight_135 = 0;
	}

	return float4(weight_0, weight_90, weight_45, weight_135);
}

float CalcLTIFast(const float y[5]) {
	const float a_min = min(min(y[0], y[1]), y[2]);
	const float a_max = max(max(y[0], y[1]), y[2]);

	const float b_min = min(min(y[2], y[3]), y[4]);
	const float b_max = max(max(y[2], y[3]), y[4]);

	const float a_cont = a_max - a_min;
	const float b_cont = b_max - b_min;

	const float cont_ratio = max(a_cont, b_cont) / (min(a_cont, b_cont) + kEps * (1.0f / NIS_SCALE_FLOAT));
	return (1.0f - saturate((cont_ratio - kMinContrastRatio) * kRatioNorm)) * kContrastBoost;
}

float EvalUSM(const float pxl[5], const float sharpnessStrength, const float sharpnessLimit) {
	// USM profile
	float y_usm = -0.6001f * pxl[1] + 1.2002f * pxl[2] - 0.6001f * pxl[3];
	// boost USM profile
	y_usm *= sharpnessStrength;
	// clamp to the limit
	y_usm = min(sharpnessLimit, max(-sharpnessLimit, y_usm));
	// reduce ringing
	y_usm *= CalcLTIFast(pxl);

	return y_usm;
}

float4 GetDirUSM(const float p[5][5]) {
	// sharpness boost & limit are the same for all directions
	const float scaleY = 1.0f - saturate((p[2][2] - kSharpStartY) * kSharpScaleY);
	// scale the ramp to sharpen as a function of luma
	const float sharpnessStrength = scaleY * kSharpStrengthScale + kSharpStrengthMin;
	// scale the ramp to limit USM as a function of luma
	const float sharpnessLimit = (scaleY * kSharpLimitScale + kSharpLimitMin) * p[2][2];

	float4 rval;
	// 0 deg filter
	float interp0Deg[5];
	{
		for (int i = 0; i < 5; ++i) {
			interp0Deg[i] = p[i][2];
		}
	}

	rval.x = EvalUSM(interp0Deg, sharpnessStrength, sharpnessLimit);

	// 90 deg filter
	float interp90Deg[5];
	{
		for (int i = 0; i < 5; ++i) {
			interp90Deg[i] = p[2][i];
		}
	}

	rval.y = EvalUSM(interp90Deg, sharpnessStrength, sharpnessLimit);

	//45 deg filter
	float interp45Deg[5];
	interp45Deg[0] = p[1][1];
	interp45Deg[1] = lerp(p[2][1], p[1][2], 0.5f);
	interp45Deg[2] = p[2][2];
	interp45Deg[3] = lerp(p[3][2], p[2][3], 0.5f);
	interp45Deg[4] = p[3][3];

	rval.z = EvalUSM(interp45Deg, sharpnessStrength, sharpnessLimit);

	//135 deg filter
	float interp135Deg[5];
	interp135Deg[0] = p[3][1];
	interp135Deg[1] = lerp(p[3][2], p[2][1], 0.5f);
	interp135Deg[2] = p[2][2];
	interp135Deg[3] = lerp(p[2][3], p[1][2], 0.5f);
	interp135Deg[4] = p[1][3];

	rval.w = EvalUSM(interp135Deg, sharpnessStrength, sharpnessLimit);
	return rval;
}

float4 Pass1(float2 pos) {
	// load 5x5 support to regs
	float p[5][5];
	[unroll]
	for (int i = 0; i < 5; ++i) {
		[unroll]
		for (int j = 0; j < 5; ++j) {
			p[i][j] = getY(INPUT.Sample(samPoint, pos + float2(j - 2, i - 2) * float2(inputPtX, inputPtY)).rgb);
		}
	}

	// get directional filter bank output
	const float4 dirUSM = GetDirUSM(p);

	// generate weights for directional filters
	float4 w = GetEdgeMap(p, kSupportSize / 2 - 1, kSupportSize / 2 - 1);

	// final USM is a weighted sum filter outputs
	const float usmY = (dirUSM.x * w.x + dirUSM.y * w.y + dirUSM.z * w.z + dirUSM.w * w.w);

	float4 op = INPUT.Sample(samLinear, pos);

	op.x += usmY;
	op.y += usmY;
	op.z += usmY;

	return op;
}