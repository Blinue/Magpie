// 移植自 https://github.com/libretro/common-shaders/blob/master/xbrz/shaders/4xbrz.cg

//!MAGPIE EFFECT
//!VERSION 2
//!OUTPUT_WIDTH INPUT_WIDTH * 4
//!OUTPUT_HEIGHT INPUT_HEIGHT * 4


//!TEXTURE
Texture2D INPUT;

//!SAMPLER
//!FILTER POINT
SamplerState sam;


//!PASS 1
//!IN INPUT
//!BLOCK_SIZE 32
//!NUM_THREADS 64

#define BLEND_NONE 0
#define BLEND_NORMAL 1
#define BLEND_DOMINANT 2
#define LUMINANCE_WEIGHT 1.0
#define EQUAL_COLOR_TOLERANCE 30.0/255.0
#define STEEP_DIRECTION_THRESHOLD 2.2
#define DOMINANT_DIRECTION_THRESHOLD 3.6

float reduce(const float3 color) {
	return dot(color, float3(65536.0, 256.0, 1.0));
}

float DistYCbCr(const float3 pixA, const float3 pixB) {
	const float3 w = float3(0.2627, 0.6780, 0.0593);
	const float scaleB = 0.5 / (1.0 - w.b);
	const float scaleR = 0.5 / (1.0 - w.r);
	float3 diff = pixA - pixB;
	float Y = dot(diff, w);
	float Cb = scaleB * (diff.b - Y);
	float Cr = scaleR * (diff.r - Y);

	return sqrt(((LUMINANCE_WEIGHT * Y) * (LUMINANCE_WEIGHT * Y)) + (Cb * Cb) + (Cr * Cr));
}

bool IsPixEqual(const float3 pixA, const float3 pixB) {
	return (DistYCbCr(pixA, pixB) < EQUAL_COLOR_TOLERANCE);
}

bool IsBlendingNeeded(const int4 blend) {
	return any(!(blend == int4(BLEND_NONE, BLEND_NONE, BLEND_NONE, BLEND_NONE)));
}

const static uint destIdx[4][4] = {
	{ 6,  7,  8,  9},
	{ 5,  0,  1, 10},
	{ 4,  3,  2, 11},
	{15, 14, 13, 12}
};

//---------------------------------------
// Input Pixel Mapping:  --|21|22|23|--
//                       19|06|07|08|09
//                       18|05|00|01|10
//                       17|04|03|02|11
//                       --|15|14|13|--
//
// Output Pixel Mapping:  06|07|08|09
//                        05|00|01|10
//                        04|03|02|11
//                        15|14|13|12
void Pass1(uint2 blockStart, uint3 threadId) {
	uint2 gxy = (Rmp8x8(threadId.x) << 2) + blockStart;

	if (!CheckViewport(gxy)) {
		return;
	}

	const float2 inputPt = GetInputPt();
	const float2 pos = ((gxy >> 2) + 0.5f) * inputPt;

	//    A1 B1 C1
	// A0 A  B  C  C4
	// D0 D  E  F  F4
	// G0 G  H  I  I4
	//    G5 H5 I5

	float4 t1 = pos.xxxy + float4(-inputPt.x, 0, inputPt.x, -2.0 * inputPt.y); // A1 B1 C1
	float4 t2 = pos.xxxy + float4(-inputPt.x, 0, inputPt.x, -inputPt.y);		// A  B  C
	float4 t3 = pos.xxxy + float4(-inputPt.x, 0, inputPt.x, 0);				// D  E  F
	float4 t4 = pos.xxxy + float4(-inputPt.x, 0, inputPt.x, inputPt.y);		// G  H  I
	float4 t5 = pos.xxxy + float4(-inputPt.x, 0, inputPt.x, 2.0 * inputPt.y);	// G5 H5 I5
	float4 t6 = pos.xyyy + float4(-2.0 * inputPt.x, -inputPt.y, 0, inputPt.y);	// A0 D0 G0
	float4 t7 = pos.xyyy + float4(2.0 * inputPt.x, -inputPt.y, 0, inputPt.y);	// C4 F4 I4

	//---------------------------------------
	// Input Pixel Mapping:  20|21|22|23|24
	//                       19|06|07|08|09
	//                       18|05|00|01|10
	//                       17|04|03|02|11
	//                       16|15|14|13|12

	float3 src[25];

	src[21] = INPUT.SampleLevel(sam, t1.xw, 0).rgb;
	src[22] = INPUT.SampleLevel(sam, t1.yw, 0).rgb;
	src[23] = INPUT.SampleLevel(sam, t1.zw, 0).rgb;
	src[6] = INPUT.SampleLevel(sam, t2.xw, 0).rgb;
	src[7] = INPUT.SampleLevel(sam, t2.yw, 0).rgb;
	src[8] = INPUT.SampleLevel(sam, t2.zw, 0).rgb;
	src[5] = INPUT.SampleLevel(sam, t3.xw, 0).rgb;
	src[0] = INPUT.SampleLevel(sam, t3.yw, 0).rgb;
	src[1] = INPUT.SampleLevel(sam, t3.zw, 0).rgb;
	src[4] = INPUT.SampleLevel(sam, t4.xw, 0).rgb;
	src[3] = INPUT.SampleLevel(sam, t4.yw, 0).rgb;
	src[2] = INPUT.SampleLevel(sam, t4.zw, 0).rgb;
	src[15] = INPUT.SampleLevel(sam, t5.xw, 0).rgb;
	src[14] = INPUT.SampleLevel(sam, t5.yw, 0).rgb;
	src[13] = INPUT.SampleLevel(sam, t5.zw, 0).rgb;
	src[19] = INPUT.SampleLevel(sam, t6.xy, 0).rgb;
	src[18] = INPUT.SampleLevel(sam, t6.xz, 0).rgb;
	src[17] = INPUT.SampleLevel(sam, t6.xw, 0).rgb;
	src[9] = INPUT.SampleLevel(sam, t7.xy, 0).rgb;
	src[10] = INPUT.SampleLevel(sam, t7.xz, 0).rgb;
	src[11] = INPUT.SampleLevel(sam, t7.xw, 0).rgb;

	//---------------------------------------
	// Input Pixel Mapping:  20|21|22|23|24
	//                       19|06|07|08|09
	//                       18|05|00|01|10
	//                       17|04|03|02|11
	//                       16|15|14|13|12

	float v[9];
	v[0] = reduce(src[0]);
	v[1] = reduce(src[1]);
	v[2] = reduce(src[2]);
	v[3] = reduce(src[3]);
	v[4] = reduce(src[4]);
	v[5] = reduce(src[5]);
	v[6] = reduce(src[6]);
	v[7] = reduce(src[7]);
	v[8] = reduce(src[8]);

	int4 blendResult = int4(BLEND_NONE, BLEND_NONE, BLEND_NONE, BLEND_NONE);

	// Preprocess corners
	// Pixel Tap Mapping: --|--|--|--|--
	//                    --|--|07|08|--
	//                    --|05|00|01|10
	//                    --|04|03|02|11
	//                    --|--|14|13|--

	// Corner (1, 1)
	if (!((v[0] == v[1] && v[3] == v[2]) || (v[0] == v[3] && v[1] == v[2]))) {
		float dist_03_01 = DistYCbCr(src[4], src[0]) + DistYCbCr(src[0], src[8]) + DistYCbCr(src[14], src[2]) + DistYCbCr(src[2], src[10]) + (4.0 * DistYCbCr(src[3], src[1]));
		float dist_00_02 = DistYCbCr(src[5], src[3]) + DistYCbCr(src[3], src[13]) + DistYCbCr(src[7], src[1]) + DistYCbCr(src[1], src[11]) + (4.0 * DistYCbCr(src[0], src[2]));
		bool dominantGradient = (DOMINANT_DIRECTION_THRESHOLD * dist_03_01) < dist_00_02;
		blendResult[2] = ((dist_03_01 < dist_00_02) && (v[0] != v[1]) && (v[0] != v[3])) ? ((dominantGradient) ? BLEND_DOMINANT : BLEND_NORMAL) : BLEND_NONE;
	}


	// Pixel Tap Mapping: --|--|--|--|--
	//                    --|06|07|--|--
	//                    18|05|00|01|--
	//                    17|04|03|02|--
	//                    --|15|14|--|--
	// Corner (0, 1)
	if (!((v[5] == v[0] && v[4] == v[3]) || (v[5] == v[4] && v[0] == v[3]))) {
		float dist_04_00 = DistYCbCr(src[17], src[5]) + DistYCbCr(src[5], src[7]) + DistYCbCr(src[15], src[3]) + DistYCbCr(src[3], src[1]) + (4.0 * DistYCbCr(src[4], src[0]));
		float dist_05_03 = DistYCbCr(src[18], src[4]) + DistYCbCr(src[4], src[14]) + DistYCbCr(src[6], src[0]) + DistYCbCr(src[0], src[2]) + (4.0 * DistYCbCr(src[5], src[3]));
		bool dominantGradient = (DOMINANT_DIRECTION_THRESHOLD * dist_05_03) < dist_04_00;
		blendResult[3] = ((dist_04_00 > dist_05_03) && (v[0] != v[5]) && (v[0] != v[3])) ? ((dominantGradient) ? BLEND_DOMINANT : BLEND_NORMAL) : BLEND_NONE;
	}

	// Pixel Tap Mapping: --|--|22|23|--
	//                    --|06|07|08|09
	//                    --|05|00|01|10
	//                    --|--|03|02|--
	//                    --|--|--|--|--
	// Corner (1, 0)
	if (!((v[7] == v[8] && v[0] == v[1]) || (v[7] == v[0] && v[8] == v[1]))) {
		float dist_00_08 = DistYCbCr(src[5], src[7]) + DistYCbCr(src[7], src[23]) + DistYCbCr(src[3], src[1]) + DistYCbCr(src[1], src[9]) + (4.0 * DistYCbCr(src[0], src[8]));
		float dist_07_01 = DistYCbCr(src[6], src[0]) + DistYCbCr(src[0], src[2]) + DistYCbCr(src[22], src[8]) + DistYCbCr(src[8], src[10]) + (4.0 * DistYCbCr(src[7], src[1]));
		bool dominantGradient = (DOMINANT_DIRECTION_THRESHOLD * dist_07_01) < dist_00_08;
		blendResult[1] = ((dist_00_08 > dist_07_01) && (v[0] != v[7]) && (v[0] != v[1])) ? ((dominantGradient) ? BLEND_DOMINANT : BLEND_NORMAL) : BLEND_NONE;
	}

	// Pixel Tap Mapping: --|21|22|--|--
	//                    19|06|07|08|--
	//                    18|05|00|01|--
	//                    --|04|03|--|--
	//                    --|--|--|--|--
	// Corner (0, 0)
	if (!((v[6] == v[7] && v[5] == v[0]) || (v[6] == v[5] && v[7] == v[0]))) {
		float dist_05_07 = DistYCbCr(src[18], src[6]) + DistYCbCr(src[6], src[22]) + DistYCbCr(src[4], src[0]) + DistYCbCr(src[0], src[8]) + (4.0 * DistYCbCr(src[5], src[7]));
		float dist_06_00 = DistYCbCr(src[19], src[5]) + DistYCbCr(src[5], src[3]) + DistYCbCr(src[21], src[7]) + DistYCbCr(src[7], src[1]) + (4.0 * DistYCbCr(src[6], src[0]));
		bool dominantGradient = (DOMINANT_DIRECTION_THRESHOLD * dist_05_07) < dist_06_00;
		blendResult[0] = ((dist_05_07 < dist_06_00) && (v[0] != v[5]) && (v[0] != v[7])) ? ((dominantGradient) ? BLEND_DOMINANT : BLEND_NORMAL) : BLEND_NONE;
	}

	float3 dst[16];
	dst[0] = src[0];
	dst[1] = src[0];
	dst[2] = src[0];
	dst[3] = src[0];
	dst[4] = src[0];
	dst[5] = src[0];
	dst[6] = src[0];
	dst[7] = src[0];
	dst[8] = src[0];
	dst[9] = src[0];
	dst[10] = src[0];
	dst[11] = src[0];
	dst[12] = src[0];
	dst[13] = src[0];
	dst[14] = src[0];
	dst[15] = src[0];

	// Scale pixel
	if (IsBlendingNeeded(blendResult)) {
		float dist_01_04 = DistYCbCr(src[1], src[4]);
		float dist_03_08 = DistYCbCr(src[3], src[8]);
		bool haveShallowLine = (STEEP_DIRECTION_THRESHOLD * dist_01_04 <= dist_03_08) && (v[0] != v[4]) && (v[5] != v[4]);
		bool haveSteepLine = (STEEP_DIRECTION_THRESHOLD * dist_03_08 <= dist_01_04) && (v[0] != v[8]) && (v[7] != v[8]);
		bool needBlend = (blendResult[2] != BLEND_NONE);
		bool doLineBlend = (blendResult[2] >= BLEND_DOMINANT ||
			!((blendResult[1] != BLEND_NONE && !IsPixEqual(src[0], src[4])) ||
				(blendResult[3] != BLEND_NONE && !IsPixEqual(src[0], src[8])) ||
				(IsPixEqual(src[4], src[3]) && IsPixEqual(src[3], src[2]) && IsPixEqual(src[2], src[1]) && IsPixEqual(src[1], src[8]) && !IsPixEqual(src[0], src[2]))));

		float3 blendPix = (DistYCbCr(src[0], src[1]) <= DistYCbCr(src[0], src[3])) ? src[1] : src[3];
		dst[2] = lerp(dst[2], blendPix, (needBlend && doLineBlend) ? ((haveShallowLine) ? ((haveSteepLine) ? 1.0 / 3.0 : 0.25) : ((haveSteepLine) ? 0.25 : 0.00)) : 0.00);
		dst[9] = lerp(dst[9], blendPix, (needBlend && doLineBlend && haveSteepLine) ? 0.25 : 0.00);
		dst[10] = lerp(dst[10], blendPix, (needBlend && doLineBlend && haveSteepLine) ? 0.75 : 0.00);
		dst[11] = lerp(dst[11], blendPix, (needBlend) ? ((doLineBlend) ? ((haveSteepLine) ? 1.00 : ((haveShallowLine) ? 0.75 : 0.50)) : 0.08677704501) : 0.00);
		dst[12] = lerp(dst[12], blendPix, (needBlend) ? ((doLineBlend) ? 1.00 : 0.6848532563) : 0.00);
		dst[13] = lerp(dst[13], blendPix, (needBlend) ? ((doLineBlend) ? ((haveShallowLine) ? 1.00 : ((haveSteepLine) ? 0.75 : 0.50)) : 0.08677704501) : 0.00);
		dst[14] = lerp(dst[14], blendPix, (needBlend && doLineBlend && haveShallowLine) ? 0.75 : 0.00);
		dst[15] = lerp(dst[15], blendPix, (needBlend && doLineBlend && haveShallowLine) ? 0.25 : 0.00);


		dist_01_04 = DistYCbCr(src[7], src[2]);
		dist_03_08 = DistYCbCr(src[1], src[6]);
		haveShallowLine = (STEEP_DIRECTION_THRESHOLD * dist_01_04 <= dist_03_08) && (v[0] != v[2]) && (v[3] != v[2]);
		haveSteepLine = (STEEP_DIRECTION_THRESHOLD * dist_03_08 <= dist_01_04) && (v[0] != v[6]) && (v[5] != v[6]);
		needBlend = (blendResult[1] != BLEND_NONE);
		doLineBlend = (blendResult[1] >= BLEND_DOMINANT ||
			!((blendResult[0] != BLEND_NONE && !IsPixEqual(src[0], src[2])) ||
				(blendResult[2] != BLEND_NONE && !IsPixEqual(src[0], src[6])) ||
				(IsPixEqual(src[2], src[1]) && IsPixEqual(src[1], src[8]) && IsPixEqual(src[8], src[7]) && IsPixEqual(src[7], src[6]) && !IsPixEqual(src[0], src[8]))));

		blendPix = (DistYCbCr(src[0], src[7]) <= DistYCbCr(src[0], src[1])) ? src[7] : src[1];
		dst[1] = lerp(dst[1], blendPix, (needBlend && doLineBlend) ? ((haveShallowLine) ? ((haveSteepLine) ? 1.0 / 3.0 : 0.25) : ((haveSteepLine) ? 0.25 : 0.00)) : 0.00);
		dst[6] = lerp(dst[6], blendPix, (needBlend && doLineBlend && haveSteepLine) ? 0.25 : 0.00);
		dst[7] = lerp(dst[7], blendPix, (needBlend && doLineBlend && haveSteepLine) ? 0.75 : 0.00);
		dst[8] = lerp(dst[8], blendPix, (needBlend) ? ((doLineBlend) ? ((haveSteepLine) ? 1.00 : ((haveShallowLine) ? 0.75 : 0.50)) : 0.08677704501) : 0.00);
		dst[9] = lerp(dst[9], blendPix, (needBlend) ? ((doLineBlend) ? 1.00 : 0.6848532563) : 0.00);
		dst[10] = lerp(dst[10], blendPix, (needBlend) ? ((doLineBlend) ? ((haveShallowLine) ? 1.00 : ((haveSteepLine) ? 0.75 : 0.50)) : 0.08677704501) : 0.00);
		dst[11] = lerp(dst[11], blendPix, (needBlend && doLineBlend && haveShallowLine) ? 0.75 : 0.00);
		dst[12] = lerp(dst[12], blendPix, (needBlend && doLineBlend && haveShallowLine) ? 0.25 : 0.00);


		dist_01_04 = DistYCbCr(src[5], src[8]);
		dist_03_08 = DistYCbCr(src[7], src[4]);
		haveShallowLine = (STEEP_DIRECTION_THRESHOLD * dist_01_04 <= dist_03_08) && (v[0] != v[8]) && (v[1] != v[8]);
		haveSteepLine = (STEEP_DIRECTION_THRESHOLD * dist_03_08 <= dist_01_04) && (v[0] != v[4]) && (v[3] != v[4]);
		needBlend = (blendResult[0] != BLEND_NONE);
		doLineBlend = (blendResult[0] >= BLEND_DOMINANT ||
			!((blendResult[3] != BLEND_NONE && !IsPixEqual(src[0], src[8])) ||
				(blendResult[1] != BLEND_NONE && !IsPixEqual(src[0], src[4])) ||
				(IsPixEqual(src[8], src[7]) && IsPixEqual(src[7], src[6]) && IsPixEqual(src[6], src[5]) && IsPixEqual(src[5], src[4]) && !IsPixEqual(src[0], src[6]))));

		blendPix = (DistYCbCr(src[0], src[5]) <= DistYCbCr(src[0], src[7])) ? src[5] : src[7];
		dst[0] = lerp(dst[0], blendPix, (needBlend && doLineBlend) ? ((haveShallowLine) ? ((haveSteepLine) ? 1.0 / 3.0 : 0.25) : ((haveSteepLine) ? 0.25 : 0.00)) : 0.00);
		dst[15] = lerp(dst[15], blendPix, (needBlend && doLineBlend && haveSteepLine) ? 0.25 : 0.00);
		dst[4] = lerp(dst[4], blendPix, (needBlend && doLineBlend && haveSteepLine) ? 0.75 : 0.00);
		dst[5] = lerp(dst[5], blendPix, (needBlend) ? ((doLineBlend) ? ((haveSteepLine) ? 1.00 : ((haveShallowLine) ? 0.75 : 0.50)) : 0.08677704501) : 0.00);
		dst[6] = lerp(dst[6], blendPix, (needBlend) ? ((doLineBlend) ? 1.00 : 0.6848532563) : 0.00);
		dst[7] = lerp(dst[7], blendPix, (needBlend) ? ((doLineBlend) ? ((haveShallowLine) ? 1.00 : ((haveSteepLine) ? 0.75 : 0.50)) : 0.08677704501) : 0.00);
		dst[8] = lerp(dst[8], blendPix, (needBlend && doLineBlend && haveShallowLine) ? 0.75 : 0.00);
		dst[9] = lerp(dst[9], blendPix, (needBlend && doLineBlend && haveShallowLine) ? 0.25 : 0.00);


		dist_01_04 = DistYCbCr(src[3], src[6]);
		dist_03_08 = DistYCbCr(src[5], src[2]);
		haveShallowLine = (STEEP_DIRECTION_THRESHOLD * dist_01_04 <= dist_03_08) && (v[0] != v[6]) && (v[7] != v[6]);
		haveSteepLine = (STEEP_DIRECTION_THRESHOLD * dist_03_08 <= dist_01_04) && (v[0] != v[2]) && (v[1] != v[2]);
		needBlend = (blendResult[3] != BLEND_NONE);
		doLineBlend = (blendResult[3] >= BLEND_DOMINANT ||
			!((blendResult[2] != BLEND_NONE && !IsPixEqual(src[0], src[6])) ||
				(blendResult[0] != BLEND_NONE && !IsPixEqual(src[0], src[2])) ||
				(IsPixEqual(src[6], src[5]) && IsPixEqual(src[5], src[4]) && IsPixEqual(src[4], src[3]) && IsPixEqual(src[3], src[2]) && !IsPixEqual(src[0], src[4]))));

		blendPix = (DistYCbCr(src[0], src[3]) <= DistYCbCr(src[0], src[5])) ? src[3] : src[5];
		dst[3] = lerp(dst[3], blendPix, (needBlend && doLineBlend) ? ((haveShallowLine) ? ((haveSteepLine) ? 1.0 / 3.0 : 0.25) : ((haveSteepLine) ? 0.25 : 0.00)) : 0.00);
		dst[12] = lerp(dst[12], blendPix, (needBlend && doLineBlend && haveSteepLine) ? 0.25 : 0.00);
		dst[13] = lerp(dst[13], blendPix, (needBlend && doLineBlend && haveSteepLine) ? 0.75 : 0.00);
		dst[14] = lerp(dst[14], blendPix, (needBlend) ? ((doLineBlend) ? ((haveSteepLine) ? 1.00 : ((haveShallowLine) ? 0.75 : 0.50)) : 0.08677704501) : 0.00);
		dst[15] = lerp(dst[15], blendPix, (needBlend) ? ((doLineBlend) ? 1.00 : 0.6848532563) : 0.00);
		dst[4] = lerp(dst[4], blendPix, (needBlend) ? ((doLineBlend) ? ((haveShallowLine) ? 1.00 : ((haveSteepLine) ? 0.75 : 0.50)) : 0.08677704501) : 0.00);
		dst[5] = lerp(dst[5], blendPix, (needBlend && doLineBlend && haveShallowLine) ? 0.75 : 0.00);
		dst[6] = lerp(dst[6], blendPix, (needBlend && doLineBlend && haveShallowLine) ? 0.25 : 0.00);
	}

	[unroll]
	for (uint i = 0; i < 4; ++i) {
		[unroll]
		for (uint j = 0; j < 4; ++j) {
			const uint2 destPos = gxy + uint2(i, j);

			if (i != 0 || j != 0) {
				if (!CheckViewport(destPos)) {
					continue;
				}
			}

			WriteToOutput(destPos, dst[destIdx[j][i]]);
		}
	}
}
