// 移植自 https://github.com/NVIDIAGameWorks/NVIDIAImageScaling/blob/main/NIS/NIS_Scaler.h

//!MAGPIE EFFECT
//!VERSION 2
//!OUTPUT_WIDTH INPUT_WIDTH
//!OUTPUT_HEIGHT INPUT_HEIGHT


//!TEXTURE
Texture2D INPUT;

//!SAMPLER
//!FILTER LINEAR
SamplerState samplerLinearClamp;

//!PARAMETER
//!DEFAULT 0.5
//!MIN 0
//!MAX 1
float sharpness;


//!PASS 1
//!IN INPUT
//!BLOCK_SIZE 32, 32
//!NUM_THREADS 256

#pragma warning(disable: 4714)	// X4714: sum of temp registers and indexable temp registers times 256 threads exceeds the recommended total 16384.  Performance may be reduced

#define NIS_BLOCK_WIDTH MP_BLOCK_WIDTH
#define NIS_BLOCK_HEIGHT MP_BLOCK_HEIGHT
#define NIS_THREAD_GROUP_SIZE MP_NUM_THREADS_X

#define kDetectRatio (2 * 1127.f / 1024.f)
#define kDetectThres (64.0f / 1024.0f)
#define kEps (1.0f / 255.0f)
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
#define MaxScale ((sharpen_slider >= 0.0f) ? 1.25f : 1.75f)
#define kSharpStrengthMin (max(0.0f, 0.4f + sharpen_slider * MinScale * 1.2f))
#define kSharpStrengthMax (1.6f + sharpen_slider * MaxScale * 1.8f)
#define LimitScale ((sharpen_slider >= 0.0f) ? 1.25f : 1.0f)
#define kSharpLimitMin (max(0.1f, 0.14f + sharpen_slider * LimitScale * 0.32f))
#define kSharpLimitMax (0.5f + sharpen_slider * LimitScale * 0.6f)
#define kSharpLimitScale (kSharpLimitMax - kSharpLimitMin)
#define LimitScale ((sharpen_slider >= 0.0f) ? 1.25f : 1.0f)
#define kSupportSize 5
#define kNumPixelsX  (NIS_BLOCK_WIDTH + kSupportSize - 1)
#define kNumPixelsY  (NIS_BLOCK_HEIGHT + kSupportSize - 1)

groupshared float shPixelsY[kNumPixelsY][kNumPixelsX];


float getY(float3 rgba) {
	return 0.2126f * rgba.x + 0.7152f * rgba.y + 0.0722f * rgba.z;
}

float4 GetEdgeMap(float p[5][5], int i, int j)
{
	const float g_0 = abs(p[0 + i][0 + j] + p[0 + i][1 + j] + p[0 + i][2 + j] - p[2 + i][0 + j] - p[2 + i][1 + j] - p[2 + i][2 + j]);
	const float g_45 = abs(p[1 + i][0 + j] + p[0 + i][0 + j] + p[0 + i][1 + j] - p[2 + i][1 + j] - p[2 + i][2 + j] - p[1 + i][2 + j]);
	const float g_90 = abs(p[0 + i][0 + j] + p[1 + i][0 + j] + p[2 + i][0 + j] - p[0 + i][2 + j] - p[1 + i][2 + j] - p[2 + i][2 + j]);
	const float g_135 = abs(p[1 + i][0 + j] + p[2 + i][0 + j] + p[2 + i][1 + j] - p[0 + i][1 + j] - p[0 + i][2 + j] - p[1 + i][2 + j]);

	const float g_0_90_max = max(g_0, g_90);
	const float g_0_90_min = min(g_0, g_90);
	const float g_45_135_max = max(g_45, g_135);
	const float g_45_135_min = min(g_45, g_135);

	if (g_0_90_max + g_45_135_max == 0) {
		return float4(0, 0, 0, 0);
	} else {
		float e_0_90 = min(g_0_90_max / (g_0_90_max + g_45_135_max), 1.0f);
		float e_45_135 = 1.0f - e_0_90;

		bool c_0_90 = (g_0_90_max > (g_0_90_min * kDetectRatio)) && (g_0_90_max > kDetectThres) && (g_0_90_max > g_45_135_min);
		bool c_45_135 = (g_45_135_max > (g_45_135_min * kDetectRatio)) && (g_45_135_max > kDetectThres) && (g_45_135_max > g_0_90_min);
		bool c_g_0_90 = g_0_90_max == g_0;
		bool c_g_45_135 = g_45_135_max == g_45;

		float f_e_0_90 = (c_0_90 && c_45_135) ? e_0_90 : 1.0f;
		float f_e_45_135 = (c_0_90 && c_45_135) ? e_45_135 : 1.0f;

		float weight_0 = (c_0_90 && c_g_0_90) ? f_e_0_90 : 0.0f;
		float weight_90 = (c_0_90 && !c_g_0_90) ? f_e_0_90 : 0.0f;
		float weight_45 = (c_45_135 && c_g_45_135) ? f_e_45_135 : 0.0f;
		float weight_135 = (c_45_135 && !c_g_45_135) ? f_e_45_135 : 0.0f;

		return float4(weight_0, weight_90, weight_45, weight_135);
	}
}

float CalcLTIFast(const float y[5]) {
	const float a_min = min(min(y[0], y[1]), y[2]);
	const float a_max = max(max(y[0], y[1]), y[2]);

	const float b_min = min(min(y[2], y[3]), y[4]);
	const float b_max = max(max(y[2], y[3]), y[4]);

	const float a_cont = a_max - a_min;
	const float b_cont = b_max - b_min;

	const float cont_ratio = max(a_cont, b_cont) / (min(a_cont, b_cont) + kEps);
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

void Pass1(uint2 blockStart, uint3 threadId) {
	float2 inputPt = GetInputPt();
	float kSrcNormX = inputPt.x;
	float kSrcNormY = inputPt.y;

	int threadIdx = threadId.x;
	const int dstBlockX = blockStart.x;
	const int dstBlockY = blockStart.y;

	// fill in input luma tile in batches of 2x2 pixels
	// we use texture gather to get extra support necessary
	// to compute 2x2 edge map outputs too
	const float kShift = 0.5f - kSupportSize / 2;

	for (int i = int(threadIdx) * 2; i < kNumPixelsX * kNumPixelsY / 2; i += NIS_THREAD_GROUP_SIZE * 2) {
		uint2 pos = uint2(uint(i) % uint(kNumPixelsX), uint(i) / uint(kNumPixelsX) * 2);
		[unroll]
		for (int dy = 0; dy < 2; dy++) {
			[unroll]
			for (int dx = 0; dx < 2; dx++) {
				const float tx = (dstBlockX + pos.x + dx + kShift) * kSrcNormX;
				const float ty = (dstBlockY + pos.y + dy + kShift) * kSrcNormY;
				const float4 px = INPUT.SampleLevel(samplerLinearClamp, float2(tx, ty), 0);
				shPixelsY[pos.y + dy][pos.x + dx] = getY(px.xyz);
			}
		}
	}

	GroupMemoryBarrierWithGroupSync();

	for (int k = int(threadIdx); k < NIS_BLOCK_WIDTH * NIS_BLOCK_HEIGHT; k += NIS_THREAD_GROUP_SIZE) {
		const int2 pos = int2(uint(k) % uint(NIS_BLOCK_WIDTH), uint(k) / uint(NIS_BLOCK_WIDTH));

		// do bilinear tap and correct rgb texel so it produces new sharpened luma
		const int dstX = dstBlockX + pos.x;
		const int dstY = dstBlockY + pos.y;

		if (!CheckViewport(int2(dstX, dstY))) {
			continue;
		}

		// load 5x5 support to regs
		float p[5][5];
		[unroll]
		for (int i = 0; i < 5; ++i) {
			[unroll]
			for (int j = 0; j < 5; ++j) {
				p[i][j] = shPixelsY[pos.y + i][pos.x + j];
			}
		}

		// get directional filter bank output
		float4 dirUSM = GetDirUSM(p);

		// generate weights for directional filters
		float4 w = GetEdgeMap(p, kSupportSize / 2 - 1, kSupportSize / 2 - 1);

		// final USM is a weighted sum filter outputs
		const float usmY = (dirUSM.x * w.x + dirUSM.y * w.y + dirUSM.z * w.z + dirUSM.w * w.w);

		float3 op = INPUT.SampleLevel(samplerLinearClamp, float2((dstX + 0.5f) * kSrcNormX, (dstY + 0.5f) * kSrcNormY), 0).rgb;
		op += usmY;

		WriteToOutput(uint2(dstX, dstY), op);
	}
}
