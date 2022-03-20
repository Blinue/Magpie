// 移植自 https://github.com/NVIDIAGameWorks/NVIDIAImageScaling/blob/main/NIS/NIS_Scaler.h

//!MAGPIE EFFECT
//!VERSION 2


//!PARAMETER
//!DEFAULT 0.5
//!MIN 0
//!MAX 1
float sharpness;

//!TEXTURE
Texture2D INPUT;

//!TEXTURE
//!SOURCE NIS_Coef_Scale.dds
//!FORMAT R16G16B16A16_FLOAT
Texture2D coef_scaler;

//!TEXTURE
//!SOURCE NIS_Coef_USM.dds
//!FORMAT R16G16B16A16_FLOAT
Texture2D coef_usm;

//!SAMPLER
//!FILTER LINEAR
SamplerState samplerLinearClamp;


//!PASS 1
//!IN INPUT, coef_scaler, coef_usm
//!BLOCK_SIZE 32,32
//!NUM_THREADS 256


#pragma warning(disable: 4714)	// X4714: sum of temp registers and indexable temp registers times 256 threads exceeds the recommended total 16384.  Performance may be reduced

#define kDetectRatio (2.0f * 1127.f / 1024.f)
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

#define NIS_SCALE_FLOAT 1.0f
#define NIS_SCALE_INT 1

#define NIS_BLOCK_WIDTH MP_BLOCK_WIDTH
#define NIS_BLOCK_HEIGHT MP_BLOCK_HEIGHT
#define NIS_THREAD_GROUP_SIZE MP_NUM_THREADS_X
#define kPhaseCount  64
#define kFilterSize  6
#define kSupportSize 6
#define kPadSize     kSupportSize
// 'Tile' is the region of source luminance values that we load into shPixelsY.
// It is the area of source pixels covered by the destination 'Block' plus a
// 3 pixel border of support pixels.
#define kTilePitch              (NIS_BLOCK_WIDTH + kPadSize)
#define kTileSize               (kTilePitch * (NIS_BLOCK_HEIGHT + kPadSize))
// 'EdgeMap' is the region of source pixels for which edge map vectors are derived.
// It is the area of source pixels covered by the destination 'Block' plus a
// 1 pixel border.
#define kEdgeMapPitch           (NIS_BLOCK_WIDTH + 2)
#define kEdgeMapSize            (kEdgeMapPitch * (NIS_BLOCK_HEIGHT + 2))

groupshared float shPixelsY[kTileSize];
groupshared float shCoefScaler[kPhaseCount][kFilterSize];
groupshared float shCoefUSM[kPhaseCount][kFilterSize];
groupshared float4 shEdgeMap[kEdgeMapSize];

float getY(float3 rgba) {
	return 0.2126f * rgba.x + 0.7152f * rgba.y + 0.0722f * rgba.z;
}

float4 GetEdgeMap(float p[4][4], int i, int j) {
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

void LoadFilterBanksSh(int i0, int di) {
	// Load up filter banks to shared memory
	// The work is spread over (kPhaseCount * 2) threads
	for (int i = i0; i < kPhaseCount * 2; i += di) {
		int phase = i >> 1;
		int vIdx = i & 1;

		float4 v = float4(coef_scaler[int2(vIdx, phase)]);
		int filterOffset = vIdx * 4;
		shCoefScaler[phase][filterOffset + 0] = v.x;
		shCoefScaler[phase][filterOffset + 1] = v.y;
		if (vIdx == 0) {
			shCoefScaler[phase][2] = v.z;
			shCoefScaler[phase][3] = v.w;
		}

		v = float4(coef_usm[int2(vIdx, phase)]);
		shCoefUSM[phase][filterOffset + 0] = v.x;
		shCoefUSM[phase][filterOffset + 1] = v.y;
		if (vIdx == 0) {
			shCoefUSM[phase][2] = v.z;
			shCoefUSM[phase][3] = v.w;
		}
	}
}


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
			y += shCoefScaler[phase_int][i] * pxl[i];
		}
	}
	float y_usm = 0.f;
	{
		[unroll]
		for (int i = 0; i < 6; ++i) {
			y_usm += shCoefUSM[phase_int][i] * pxl[i];
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

float FilterNormal(const float p[6][6], int phase_x_frac_int, int phase_y_frac_int) {
	float h_acc = 0.0f;
	[unroll]
	for (int j = 0; j < 6; ++j) {
		float v_acc = 0.0f;
		[unroll]
		for (int i = 0; i < 6; ++i) {
			v_acc += p[i][j] * shCoefScaler[phase_y_frac_int][i];
		}
		h_acc += v_acc * shCoefScaler[phase_x_frac_int][j];
	}

	// let's return the sum unpacked -> we can accumulate it later
	return h_acc;
}

float AddDirFilters(float p[6][6], float phase_x_frac, float phase_y_frac, int phase_x_frac_int, int phase_y_frac_int, float4 w) {
	float f = 0;
	if (w.x > 0.0f) {
		// 0 deg filter
		float interp0Deg[6];
		{
			[unroll]
			for (int i = 0; i < 6; ++i) {
				interp0Deg[i] = lerp(p[i][2], p[i][3], phase_x_frac);
			}
		}
		f += EvalPoly6(interp0Deg, phase_y_frac_int) * w.x;
	}
	if (w.y > 0.0f) {
		// 90 deg filter
		float interp90Deg[6];
		{
			[unroll]
			for (int i = 0; i < 6; ++i) {
				interp90Deg[i] = lerp(p[2][i], p[3][i], phase_y_frac);
			}
		}

		f += EvalPoly6(interp90Deg, phase_x_frac_int) * w.y;
	}
	if (w.z > 0.0f) {
		//45 deg filter
		float pphase_b45 = 0.5f + 0.5f * (phase_x_frac - phase_y_frac);

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

		f += EvalPoly6(interp45Deg, int(pphase_p45 * 64)) * w.z;
	}
	if (w.w > 0.0f) {
		//135 deg filter
		float pphase_b135 = 0.5f * (phase_x_frac + phase_y_frac);

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

		f += EvalPoly6(interp135Deg, int(pphase_p135 * 64)) * w.w;
	}
	return f;
}

void Pass1(uint2 blockStart, uint3 threadId) {
	float2 scale = GetScale();
	float2 inputPt = GetInputPt();

	float kScaleX = 1.0f / scale.x;
	float kScaleY = 1.0f / scale.y;
	float threadIdx = threadId.x;
	float kSrcNormX = inputPt.x;
	float kSrcNormY = inputPt.y;

	// Figure out the range of pixels from input image that would be needed to be loaded for this thread-block
	int dstBlockX = blockStart.x;
	int dstBlockY = blockStart.y;

	const int srcBlockStartX = int(floor((dstBlockX + 0.5f) * kScaleX - 0.5f));
	const int srcBlockStartY = int(floor((dstBlockY + 0.5f) * kScaleY - 0.5f));
	const int srcBlockEndX = int(ceil((dstBlockX + NIS_BLOCK_WIDTH + 0.5f) * kScaleX - 0.5f));
	const int srcBlockEndY = int(ceil((dstBlockY + NIS_BLOCK_HEIGHT + 0.5f) * kScaleY - 0.5f));

	int numTilePixelsX = srcBlockEndX - srcBlockStartX + kSupportSize - 1;
	int numTilePixelsY = srcBlockEndY - srcBlockStartY + kSupportSize - 1;

	// round-up load region to even size since we're loading in 2x2 batches
	numTilePixelsX += numTilePixelsX & 0x1;
	numTilePixelsY += numTilePixelsY & 0x1;
	const int numTilePixels = numTilePixelsX * numTilePixelsY;

	// calculate the equivalent values for the edge map
	const int numEdgeMapPixelsX = numTilePixelsX - kSupportSize + 2;
	const int numEdgeMapPixelsY = numTilePixelsY - kSupportSize + 2;
	const int numEdgeMapPixels = numEdgeMapPixelsX * numEdgeMapPixelsY;

	// fill in input luma tile (shPixelsY) in batches of 2x2 pixels
	// we use texture gather to get extra support necessary
	// to compute 2x2 edge map outputs too
	{
		for (uint i = threadIdx * 2; i < uint(numTilePixels) >> 1; i += NIS_THREAD_GROUP_SIZE * 2) {
			uint py = (i / numTilePixelsX) * 2;
			uint px = i % numTilePixelsX;

			// 0.5 to be in the center of texel
			// - (kSupportSize - 1) / 2 to shift by the kernel support size
			float kShift = 0.5f - (kSupportSize - 1) / 2;

			const float tx = (srcBlockStartX + px + kShift) * kSrcNormX;
			const float ty = (srcBlockStartY + py + kShift) * kSrcNormY;

			float p[2][2];
			{
				const float4 sr = INPUT.GatherRed(samplerLinearClamp, float2(tx, ty));
				const float4 sg = INPUT.GatherGreen(samplerLinearClamp, float2(tx, ty));
				const float4 sb = INPUT.GatherBlue(samplerLinearClamp, float2(tx, ty));

				p[0][0] = getY(float3(sr.w, sg.w, sb.w));
				p[0][1] = getY(float3(sr.z, sg.z, sb.z));
				p[1][0] = getY(float3(sr.x, sg.x, sb.x));
				p[1][1] = getY(float3(sr.y, sg.y, sb.y));
			}

			const uint idx = py * kTilePitch + px;
			shPixelsY[idx] = float(p[0][0]);
			shPixelsY[idx + 1] = float(p[0][1]);
			shPixelsY[idx + kTilePitch] = float(p[1][0]);
			shPixelsY[idx + kTilePitch + 1] = float(p[1][1]);
		}
	}
	GroupMemoryBarrierWithGroupSync();
	{
		// fill in the edge map of 2x2 pixels
		for (uint i = threadIdx * 2; i < uint(numEdgeMapPixels) >> 1; i += NIS_THREAD_GROUP_SIZE * 2) {
			uint py = (i / numEdgeMapPixelsX) * 2;
			uint px = i % numEdgeMapPixelsX;

			const uint edgeMapIdx = py * kEdgeMapPitch + px;

			uint tileCornerIdx = (py + 1) * kTilePitch + px + 1;
			float p[4][4];
			[unroll]
			for (int j = 0; j < 4; j++) {
				[unroll]
				for (int k = 0; k < 4; k++) {
					p[j][k] = shPixelsY[tileCornerIdx + j * kTilePitch + k];
				}
			}

			shEdgeMap[edgeMapIdx] = float4(GetEdgeMap(p, 0, 0));
			shEdgeMap[edgeMapIdx + 1] = float4(GetEdgeMap(p, 0, 1));
			shEdgeMap[edgeMapIdx + kEdgeMapPitch] = float4(GetEdgeMap(p, 1, 0));
			shEdgeMap[edgeMapIdx + kEdgeMapPitch + 1] = float4(GetEdgeMap(p, 1, 1));
		}
	}
	LoadFilterBanksSh(int(threadIdx), NIS_THREAD_GROUP_SIZE);
	GroupMemoryBarrierWithGroupSync();

	// output coord within a tile
	const int2 pos = int2(uint(threadIdx) % uint(NIS_BLOCK_WIDTH), uint(threadIdx) / uint(NIS_BLOCK_WIDTH));
	// x coord inside the output image
	const int dstX = dstBlockX + pos.x;
	// x coord inside the input image
	const float srcX = (0.5f + dstX) * kScaleX - 0.5f;
	// nearest integer part
	const int px = int(floor(srcX) - srcBlockStartX);
	// fractional part
	const float fx = srcX - floor(srcX);
	// discretized phase
	const int fx_int = int(fx * kPhaseCount);

	for (int k = 0; k < NIS_BLOCK_WIDTH * NIS_BLOCK_HEIGHT / NIS_THREAD_GROUP_SIZE; ++k) {
		// y coord inside the output image
		const int dstY = dstBlockY + pos.y + k * (NIS_THREAD_GROUP_SIZE / NIS_BLOCK_WIDTH);
		if (!CheckViewport(int2(dstX, dstY))) {
			return;
		}
		// y coord inside the input image
		const float srcY = (0.5f + dstY) * kScaleY - 0.5f;

		// nearest integer part
		const int py = int(floor(srcY) - srcBlockStartY);
		// fractional part
		const float fy = srcY - floor(srcY);
		// discretized phase
		const int fy_int = int(fy * kPhaseCount);

		// generate weights for directional filters
		const int startEdgeMapIdx = py * kEdgeMapPitch + px;
		float4 edge[2][2];
		[unroll]
		for (int i = 0; i < 2; i++) {
			[unroll]
			for (int j = 0; j < 2; j++) {
				// need to shift edge map sampling since it's a 2x2 centered inside 6x6 grid
				edge[i][j] = shEdgeMap[startEdgeMapIdx + (i * kEdgeMapPitch) + j];
			}
		}
		const float4 w = GetInterpEdgeMap(edge, fx, fy) * NIS_SCALE_INT;

		// load 6x6 support to regs
		const int startTileIdx = py * kTilePitch + px;
		float p[6][6];
		{
			[unroll]
			for (int i = 0; i < 6; ++i) {
				[unroll]
				for (int j = 0; j < 6; ++j) {
					p[i][j] = shPixelsY[startTileIdx + i * kTilePitch + j];
				}
			}
		}

		// weigth for luma
		const float baseWeight = NIS_SCALE_FLOAT - w.x - w.y - w.z - w.w;

		// final luma is a weighted product of directional & normal filters
		float opY = 0;

		// get traditional scaler filter output
		opY += FilterNormal(p, fx_int, fy_int) * baseWeight;

		// get directional filter bank output
		opY += AddDirFilters(p, fx, fy, fx_int, fy_int, w);

		// do bilinear tap for chroma upscaling

		float3 op = INPUT.SampleLevel(samplerLinearClamp, float2((srcX + 0.5f) * kSrcNormX, (srcY + 0.5f) * kSrcNormY), 0).rgb;

		const float corr = opY * (1.0f / NIS_SCALE_FLOAT) - getY(float3(op.x, op.y, op.z));
		op.x += corr;
		op.y += corr;
		op.z += corr;

		WriteToOutput(uint2(dstX, dstY), op);
	}
}
