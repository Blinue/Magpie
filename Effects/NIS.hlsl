// 移植自 https://github.com/NVIDIAGameWorks/NVIDIAImageScaling/blob/main/NIS/NIS_Scaler.h

//!MAGPIE EFFECT
//!VERSION 1


//!CONSTANT
//!VALUE INPUT_PT_X
float inputPtX;

//!CONSTANT
//!VALUE INPUT_PT_Y
float inputPtY;

//!CONSTANT
//!VALUE SCALE_X
float scaleX;

//!CONSTANT
//!VALUE SCALE_Y
float scaleY;

//!TEXTURE
Texture2D INPUT;

//!TEXTURE
//!WIDTH INPUT_WIDTH
//!HEIGHT INPUT_HEIGHT
//!FORMAT R16G16B16A16_FLOAT
Texture2D shEdgeMap;

//!TEXTURE
//!SOURCE NIS_Coef_Scale.dds
Texture2D coefScale;

//!TEXTURE
//!SOURCE NIS_Coef_USM.dds
Texture2D coefUSM;

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


//!COMMON

#define kDetectRatio (1127.f / 1024.f)
#define kDetectThres (64.0f / 1024.0f)
#define kPhaseCount 64
#define kFilterSize 8
#define kEps 1.0f
#define kMinContrastRatio 2.0f
#define kMaxContrastRatio 10.0f
#define kRatioNorm (1.0f / (kMaxContrastRatio - kMinContrastRatio))
#define kContrastBoost 1.0f
#define kSharpStartY 0.45f
#define kSharpEndY 0.9f
#define kSharpScaleY (1.0f / (kSharpEndY - kSharpStartY))
#define kSharpStrengthScale (kSharpStrengthMax - kSharpStrengthMin)
#define sharpen_slider (sharpness - 0.5f)
#define MinScale ((sharpen_slider >= 0.0f) ? 1.25f : 1.0f)
#define kSharpStrengthMin (max(0.0f, 0.4f + sharpen_slider * MinScale * 1.2f))
#define kSharpStrengthMax (1.6f + sharpen_slider * 1.8f)
#define LimitScale ((sharpen_slider >= 0.0f) ? 1.25f : 1.0f)
#define kSharpLimitMin (max(0.1f, 0.14f + sharpen_slider * LimitScale * 0.32f))
#define kSharpLimitMax (0.5f + sharpen_slider * LimitScale * 0.6f)
#define kSharpLimitScale (kSharpLimitMax - kSharpLimitMin)

#define NIS_SCALE_FLOAT 1.0f
#define NIS_SCALE_INT 1


float getY(float3 rgba) {
	return 0.2126f * rgba.x + 0.7152f * rgba.y + 0.0722f * rgba.z;
}


//!PASS 1
//!BIND INPUT
//!SAVE shEdgeMap

float4 GetEdgeMap(float p[3][3]) {
	const float g_0 = abs(p[0][0] + p[0][1] + p[0][2] - p[2][0] - p[2][1] - p[2][2]);
	const float g_45 = abs(p[1][0] + p[0][0] + p[0][1] - p[2][1] - p[2][2] - p[1][2]);
	const float g_90 = abs(p[0][0] + p[1][0] + p[2][0] - p[0][2] - p[1][2] - p[2][2]);
	const float g_135 = abs(p[1][0] + p[2][0] + p[2][1] - p[0][1] - p[0][2] - p[1][2]);

	const float g_0_90_max = max(g_0, g_90);
	const float g_0_90_min = min(g_0, g_90);
	const float g_45_135_max = max(g_45, g_135);
	const float g_45_135_min = min(g_45, g_135);

	float e_0_90 = 0;
	float e_45_135 = 0;

	if ((g_0_90_max + g_45_135_max) != 0) {
		e_0_90 = g_0_90_max / (g_0_90_max + g_45_135_max);
		e_0_90 = min(e_0_90, 1.0f);
		e_45_135 = 1.0f - e_0_90;
	}

	float e = ((g_0_90_max > (g_0_90_min * kDetectRatio)) && (g_0_90_max > kDetectThres) && (g_0_90_max > g_45_135_min)) ? 1.f : 0.f;
	float edge_0 = (g_0_90_max == g_0) ? e : 0.f;
	float edge_90 = (g_0_90_max == g_0) ? 0.f : e;

	e = ((g_45_135_max > (g_45_135_min * kDetectRatio)) && (g_45_135_max > kDetectThres) && (g_45_135_max > g_0_90_min)) ? 1.f : 0.f;
	float edge_45 = (g_45_135_max == g_45) ? e : 0.f;
	float edge_135 = (g_45_135_max == g_45) ? 0.f : e;

	float weight_0 = 0.f;
	float weight_90 = 0.f;
	float weight_45 = 0.f;
	float weight_135 = 0.f;
	if ((edge_0 + edge_90 + edge_45 + edge_135) >= 2.0f) {
		weight_0 = (edge_0 == 1.0f) ? e_0_90 : 0.f;
		weight_90 = (edge_0 == 1.0f) ? 0.f : e_0_90;

		weight_45 = (edge_45 == 1.0f) ? e_45_135 : 0.f;
		weight_135 = (edge_45 == 1.0f) ? 0.f : e_45_135;
	} else if ((edge_0 + edge_90 + edge_45 + edge_135) >= 1.0f) {
		weight_0 = edge_0;
		weight_90 = edge_90;
		weight_45 = edge_45;
		weight_135 = edge_135;
	}

	return float4(weight_0, weight_90, weight_45, weight_135);
}

float4 Pass1(float2 pos) {
	float p[3][3];

	[unroll]
	for (int j = 0; j < 3; j++) {
		[unroll]
		for (int k = 0; k < 3; k++) {
			const float3 px = INPUT.Sample(samPoint, pos + float2(k - 1, j - 1) * float2(inputPtX, inputPtY)).xyz;
			p[j][k] = getY(px);
		}
	}

	return GetEdgeMap(p);
}


//!PASS 2
//!BIND INPUT, shEdgeMap, coefScale, coefUSM

float CalcLTI(float p0, float p1, float p2, float p3, float p4, float p5, int phase_index) {
	const bool selector = (phase_index <= kPhaseCount / 2);
	float sel = selector ? p0 : p3;
	const float a_min = min(min(p1, p2), sel);
	const float a_max = max(max(p1, p2), sel);
	sel = selector ? p2 : p5;
	const float b_min = min(min(p3, p4), sel);
	const float b_max = max(max(p3, p4), sel);

	const float a_cont = a_max - a_min;
	const float b_cont = b_max - b_min;

	const float cont_ratio = max(a_cont, b_cont) / (min(a_cont, b_cont) + kEps);
	return (1.0f - saturate((cont_ratio - kMinContrastRatio) * kRatioNorm)) * kContrastBoost;
}


float4 GetInterpEdgeMap(const float4 edge[2][2], float phase_frac_x, float phase_frac_y) {
	float4 h0 = lerp(edge[0][0], edge[0][1], phase_frac_x);
	float4 h1 = lerp(edge[1][0], edge[1][1], phase_frac_x);
	return lerp(h0, h1, phase_frac_y);
}

float EvalPoly6(const float pxl[6], int phase_int) {
	float y = 0.f;
	{
		[unroll]
		for (int i = 0; i < 6; ++i) {
			y += coefScale.Sample(samPoint, float2((phase_int + 0.5f) / kPhaseCount, (i + 0.5f) / 6.0f)).x * pxl[i];
		}
	}
	float y_usm = 0.f;
	{
		[unroll]
		for (int i = 0; i < 6; ++i) {
			y_usm += coefUSM.Sample(samPoint, float2((phase_int + 0.5f) / kPhaseCount, (i + 0.5f) / 6.0f)).x * pxl[i];
		}
	}

	// let's compute a piece-wise ramp based on luma
	const float y_scale = 1.0f - saturate((y * (1.0f / NIS_SCALE_FLOAT) - kSharpStartY) * kSharpScaleY);

	// scale the ramp to sharpen as a function of luma
	const float y_sharpness = y_scale * kSharpStrengthScale + kSharpStrengthMin;

	y_usm *= y_sharpness;

	// scale the ramp to limit USM as a function of luma
	const float y_sharpness_limit = (y_scale * kSharpLimitScale + kSharpLimitMin) * y;

	y_usm = min(y_sharpness_limit, max(-y_sharpness_limit, y_usm));
	// reduce ringing
	y_usm *= CalcLTI(pxl[0], pxl[1], pxl[2], pxl[3], pxl[4], pxl[5], phase_int);

	return y + y_usm;
}


float4 GetDirFilters(float p[6][6], float phase_x_frac, float phase_y_frac, int phase_x_frac_int, int phase_y_frac_int) {
	float4 f;
	// 0 deg filter
	float interp0Deg[6];
	{
		[unroll]
		for (int i = 0; i < 6; ++i) {
			interp0Deg[i] = lerp(p[i][2], p[i][3], phase_x_frac);
		}
	}

	f.x = EvalPoly6(interp0Deg, phase_y_frac_int);

	// 90 deg filter
	float interp90Deg[6];
	{
		[unroll]
		for (int i = 0; i < 6; ++i) {
			interp90Deg[i] = lerp(p[2][i], p[3][i], phase_y_frac);
		}
	}

	f.y = EvalPoly6(interp90Deg, phase_x_frac_int);

	//45 deg filter
	float pphase_b45;
	pphase_b45 = 0.5f + 0.5f * (phase_x_frac - phase_y_frac);

	float temp_interp45Deg[7];
	temp_interp45Deg[1] = lerp(p[2][1], p[1][2], pphase_b45);
	temp_interp45Deg[3] = lerp(p[3][2], p[2][3], pphase_b45);
	temp_interp45Deg[5] = lerp(p[4][3], p[3][4], pphase_b45);
	{
		pphase_b45 = pphase_b45 - 0.5f;
		float a = (pphase_b45 >= 0.f) ? p[0][2] : p[2][0];
		float b = (pphase_b45 >= 0.f) ? p[1][3] : p[3][1];
		float c = (pphase_b45 >= 0.f) ? p[2][4] : p[4][2];
		float d = (pphase_b45 >= 0.f) ? p[3][5] : p[5][3];
		temp_interp45Deg[0] = lerp(p[1][1], a, abs(pphase_b45));
		temp_interp45Deg[2] = lerp(p[2][2], b, abs(pphase_b45));
		temp_interp45Deg[4] = lerp(p[3][3], c, abs(pphase_b45));
		temp_interp45Deg[6] = lerp(p[4][4], d, abs(pphase_b45));
	}

	float interp45Deg[6];
	float pphase_p45 = phase_x_frac + phase_y_frac;
	if (pphase_p45 >= 1) {
		[unroll]
		for (int i = 0; i < 6; i++) {
			interp45Deg[i] = temp_interp45Deg[i + 1];
		}
		pphase_p45 = pphase_p45 - 1;
	} else {
		[unroll]
		for (int i = 0; i < 6; i++) {
			interp45Deg[i] = temp_interp45Deg[i];
		}
	}

	f.z = EvalPoly6(interp45Deg, (int)(pphase_p45 * 64));

	//135 deg filter
	float pphase_b135;
	pphase_b135 = 0.5f * (phase_x_frac + phase_y_frac);

	float temp_interp135Deg[7];

	temp_interp135Deg[1] = lerp(p[3][1], p[4][2], pphase_b135);
	temp_interp135Deg[3] = lerp(p[2][2], p[3][3], pphase_b135);
	temp_interp135Deg[5] = lerp(p[1][3], p[2][4], pphase_b135);

	{
		pphase_b135 = pphase_b135 - 0.5f;
		float a = (pphase_b135 >= 0.f) ? p[5][2] : p[3][0];
		float b = (pphase_b135 >= 0.f) ? p[4][3] : p[2][1];
		float c = (pphase_b135 >= 0.f) ? p[3][4] : p[1][2];
		float d = (pphase_b135 >= 0.f) ? p[2][5] : p[0][3];
		temp_interp135Deg[0] = lerp(p[4][1], a, abs(pphase_b135));
		temp_interp135Deg[2] = lerp(p[3][2], b, abs(pphase_b135));
		temp_interp135Deg[4] = lerp(p[2][3], c, abs(pphase_b135));
		temp_interp135Deg[6] = lerp(p[1][4], d, abs(pphase_b135));
	}

	float interp135Deg[6];
	float pphase_p135 = 1 + (phase_x_frac - phase_y_frac);
	if (pphase_p135 >= 1) {
		[unroll]
		for (int i = 0; i < 6; ++i) {
			interp135Deg[i] = temp_interp135Deg[i + 1];
		}
		pphase_p135 = pphase_p135 - 1;
	} else {
		[unroll]
		for (int i = 0; i < 6; ++i) {
			interp135Deg[i] = temp_interp135Deg[i];
		}
	}

	f.w = EvalPoly6(interp135Deg, (int)(pphase_p135 * 64));
	return f;
}

float FilterNormal(const float p[6][6], int phase_x_frac_int, int phase_y_frac_int) {
	float h_acc = 0.0f;
	[unroll]
	for (int j = 0; j < 6; ++j) {
		float v_acc = 0.0f;
		[unroll]
		for (int i = 0; i < 6; ++i) {
			v_acc += p[i][j] * coefScale.Sample(samPoint, float2((phase_y_frac_int + 0.5f) / kPhaseCount, (i + 0.5f) / 6.0f)).x;
		}
		h_acc += v_acc * coefScale.Sample(samPoint, float2((phase_x_frac_int + 0.5f) / kPhaseCount, (j + 0.5f) / 6.0f)).x;
	}

	// let's return the sum unpacked -> we can accumulate it later
	return h_acc;
}

float4 Pass2(float2 pos) {
	float2 srcPos = pos / float2(inputPtX, inputPtY) + 0.5f;
	float2 srcPosB = floor(srcPos);

	// load 6x6 support to regs
	float p[6][6];
	{
		[unroll]
		for (int i = 0; i < 6; ++i) {
			[unroll]
			for (int j = 0; j < 6; ++j) {
				p[i][j] = getY(INPUT.Sample(samPoint, (srcPosB + float2(j - 2, i - 2) - 0.5f) * float2(inputPtX, inputPtY)).rgb);
			}
		}
	}

	// compute discretized filter phase
	const float2 f = srcPos - srcPosB;
	const int fx_int = (int)(f.x * kPhaseCount);
	const int fy_int = (int)(f.y * kPhaseCount);
	
	// get traditional scaler filter output
	const float pixel_n = FilterNormal(p, fx_int, fy_int);
	
	// get directional filter bank output
	float4 opDirYU = GetDirFilters(p, f.x, f.y, fx_int, fy_int);

	// final luma is a weighted product of directional & normal filters

	// generate weights for directional filters
	float4 edge[2][2];
	edge[0][0] = shEdgeMap.Sample(samPoint, (srcPosB + float2(0, 0)) * float2(inputPtX, inputPtY));
	edge[0][1] = shEdgeMap.Sample(samPoint, (srcPosB + float2(1, 0)) * float2(inputPtX, inputPtY));
	edge[1][0] = shEdgeMap.Sample(samPoint, (srcPosB + float2(0, 1)) * float2(inputPtX, inputPtY));
	edge[1][1] = shEdgeMap.Sample(samPoint, (srcPosB + float2(1, 1)) * float2(inputPtX, inputPtY));

	const float4 w = GetInterpEdgeMap(edge, f.x, f.y) * NIS_SCALE_INT;
	
	// final pixel is a weighted sum filter outputs
	const float opY = (opDirYU.x * w.x + opDirYU.y * w.y + opDirYU.z * w.z + opDirYU.w * w.w +
		pixel_n * (NIS_SCALE_FLOAT - w.x - w.y - w.z - w.w)) * (1.0f / NIS_SCALE_FLOAT);
	// do bilinear tap for chroma upscaling

	float4 op = INPUT.Sample(samLinear, pos);

	const float corr = opY * (1.0f / NIS_SCALE_FLOAT) - getY(float3(op.x, op.y, op.z));
	op.x += corr;
	op.y += corr;
	op.z += corr;

	return op;
}
