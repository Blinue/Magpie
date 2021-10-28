// 移植自 https://github.com/libretro/common-shaders/blob/master/xbrz/shaders/3xbrz.cg

//!MAGPIE EFFECT
//!VERSION 1
//!OUTPUT_WIDTH INPUT_WIDTH * 3
//!OUTPUT_HEIGHT INPUT_HEIGHT * 3


//!CONSTANT
//!VALUE INPUT_PT_X
float inputPtX;

//!CONSTANT
//!VALUE INPUT_PT_Y
float inputPtY;


//!TEXTURE
Texture2D INPUT;

//!SAMPLER
//!FILTER POINT
SamplerState sam;


//!PASS 1
//!BIND INPUT

#define BLEND_NONE 0
#define BLEND_NORMAL 1
#define BLEND_DOMINANT 2
#define LUMINANCE_WEIGHT 1.0
#define EQUAL_COLOR_TOLERANCE 30.0/255.0
#define STEEP_DIRECTION_THRESHOLD 2.2
#define DOMINANT_DIRECTION_THRESHOLD 3.6

static const float one_third = 1.0 / 3.0;
static const float two_third = 2.0 / 3.0;

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

void ScalePixel(const int4 blend, const float3 k[9], inout float3 dst[9]) {
	float v0 = reduce(k[0]);
	float v4 = reduce(k[4]);
	float v5 = reduce(k[5]);
	float v7 = reduce(k[7]);
	float v8 = reduce(k[8]);

	float dist_01_04 = DistYCbCr(k[1], k[4]);
	float dist_03_08 = DistYCbCr(k[3], k[8]);
	bool haveShallowLine = (STEEP_DIRECTION_THRESHOLD * dist_01_04 <= dist_03_08) && (v0 != v4) && (v5 != v4);
	bool haveSteepLine = (STEEP_DIRECTION_THRESHOLD * dist_03_08 <= dist_01_04) && (v0 != v8) && (v7 != v8);
	bool needBlend = (blend[2] != BLEND_NONE);
	bool doLineBlend = (blend[2] >= BLEND_DOMINANT ||
		!((blend[1] != BLEND_NONE && !IsPixEqual(k[0], k[4])) ||
			(blend[3] != BLEND_NONE && !IsPixEqual(k[0], k[8])) ||
			(IsPixEqual(k[4], k[3]) && IsPixEqual(k[3], k[2]) && IsPixEqual(k[2], k[1]) && IsPixEqual(k[1], k[8]) && !IsPixEqual(k[0], k[2]))));

	float3 blendPix = (DistYCbCr(k[0], k[1]) <= DistYCbCr(k[0], k[3])) ? k[1] : k[3];
	dst[1] = lerp(dst[1], blendPix, (needBlend && doLineBlend) ? ((haveSteepLine) ? 0.750 : ((haveShallowLine) ? 0.250 : 0.125)) : 0.000);
	dst[2] = lerp(dst[2], blendPix, (needBlend) ? ((doLineBlend) ? ((!haveShallowLine && !haveSteepLine) ? 0.875 : 1.000) : 0.4545939598) : 0.000);
	dst[3] = lerp(dst[3], blendPix, (needBlend && doLineBlend) ? ((haveShallowLine) ? 0.750 : ((haveSteepLine) ? 0.250 : 0.125)) : 0.000);
	dst[4] = lerp(dst[4], blendPix, (needBlend && doLineBlend && haveShallowLine) ? 0.250 : 0.000);
	dst[8] = lerp(dst[8], blendPix, (needBlend && doLineBlend && haveSteepLine) ? 0.250 : 0.000);
}

//---------------------------------------
// Input Pixel Mapping:  --|21|22|23|--
//                       19|06|07|08|09
//                       18|05|00|01|10
//                       17|04|03|02|11
//                       --|15|14|13|--
//
// Output Pixel Mapping:    06|07|08
//                          05|00|01
//                          04|03|02
float4 Pass1(float2 pos) {
	//    A1 B1 C1
	// A0 A  B  C  C4
	// D0 D  E  F  F4
	// G0 G  H  I  I4
	//    G5 H5 I5

	float4 t1 = pos.xxxy + float4(-inputPtX, 0, inputPtX, -2.0 * inputPtY); // A1 B1 C1
	float4 t2 = pos.xxxy + float4(-inputPtX, 0, inputPtX, -inputPtY);		// A  B  C
	float4 t3 = pos.xxxy + float4(-inputPtX, 0, inputPtX, 0);				// D  E  F
	float4 t4 = pos.xxxy + float4(-inputPtX, 0, inputPtX, inputPtY);		// G  H  I
	float4 t5 = pos.xxxy + float4(-inputPtX, 0, inputPtX, 2.0 * inputPtY);	// G5 H5 I5
	float4 t6 = pos.xyyy + float4(-2.0 * inputPtX, -inputPtY, 0, inputPtY);	// A0 D0 G0
	float4 t7 = pos.xyyy + float4(2.0 * inputPtX, -inputPtY, 0, inputPtY);	// C4 F4 I4

	//---------------------------------------
	// Input Pixel Mapping:  20|21|22|23|24
	//                       19|06|07|08|09
	//                       18|05|00|01|10
	//                       17|04|03|02|11
	//                       16|15|14|13|12

	float3 src[25];

	src[21] = INPUT.Sample(sam, t1.xw).rgb;
	src[22] = INPUT.Sample(sam, t1.yw).rgb;
	src[23] = INPUT.Sample(sam, t1.zw).rgb;
	src[6] = INPUT.Sample(sam, t2.xw).rgb;
	src[7] = INPUT.Sample(sam, t2.yw).rgb;
	src[8] = INPUT.Sample(sam, t2.zw).rgb;
	src[5] = INPUT.Sample(sam, t3.xw).rgb;
	src[0] = INPUT.Sample(sam, t3.yw).rgb;
	src[1] = INPUT.Sample(sam, t3.zw).rgb;
	src[4] = INPUT.Sample(sam, t4.xw).rgb;
	src[3] = INPUT.Sample(sam, t4.yw).rgb;
	src[2] = INPUT.Sample(sam, t4.zw).rgb;
	src[15] = INPUT.Sample(sam, t5.xw).rgb;
	src[14] = INPUT.Sample(sam, t5.yw).rgb;
	src[13] = INPUT.Sample(sam, t5.zw).rgb;
	src[19] = INPUT.Sample(sam, t6.xy).rgb;
	src[18] = INPUT.Sample(sam, t6.xz).rgb;
	src[17] = INPUT.Sample(sam, t6.xw).rgb;
	src[9] = INPUT.Sample(sam, t7.xy).rgb;
	src[10] = INPUT.Sample(sam, t7.xz).rgb;
	src[11] = INPUT.Sample(sam, t7.xw).rgb;

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

	float3 dst[9];
	dst[0] = src[0];
	dst[1] = src[0];
	dst[2] = src[0];
	dst[3] = src[0];
	dst[4] = src[0];
	dst[5] = src[0];
	dst[6] = src[0];
	dst[7] = src[0];
	dst[8] = src[0];

	// Scale pixel
	if (IsBlendingNeeded(blendResult)) {
		float3 k[9];
		float3 tempDst8;
		float3 tempDst7;

		k[8] = src[8];
		k[7] = src[7];
		k[6] = src[6];
		k[5] = src[5];
		k[4] = src[4];
		k[3] = src[3];
		k[2] = src[2];
		k[1] = src[1];
		k[0] = src[0];
		ScalePixel(blendResult.xyzw, k, dst);

		k[8] = src[6];
		k[7] = src[5];
		k[6] = src[4];
		k[5] = src[3];
		k[4] = src[2];
		k[3] = src[1];
		k[2] = src[8];
		k[1] = src[7];
		tempDst8 = dst[8];
		tempDst7 = dst[7];
		dst[8] = dst[6];
		dst[7] = dst[5];
		dst[6] = dst[4];
		dst[5] = dst[3];
		dst[4] = dst[2];
		dst[3] = dst[1];
		dst[2] = tempDst8;
		dst[1] = tempDst7;
		ScalePixel(blendResult.wxyz, k, dst);

		k[8] = src[4];
		k[7] = src[3];
		k[6] = src[2];
		k[5] = src[1];
		k[4] = src[8];
		k[3] = src[7];
		k[2] = src[6];
		k[1] = src[5];
		tempDst8 = dst[8];
		tempDst7 = dst[7];
		dst[8] = dst[6];
		dst[7] = dst[5];
		dst[6] = dst[4];
		dst[5] = dst[3];
		dst[4] = dst[2];
		dst[3] = dst[1];
		dst[2] = tempDst8;
		dst[1] = tempDst7;
		ScalePixel(blendResult.zwxy, k, dst);

		k[8] = src[2];
		k[7] = src[1];
		k[6] = src[8];
		k[5] = src[7];
		k[4] = src[6];
		k[3] = src[5];
		k[2] = src[4];
		k[1] = src[3];
		tempDst8 = dst[8];
		tempDst7 = dst[7];
		dst[8] = dst[6];
		dst[7] = dst[5];
		dst[6] = dst[4];
		dst[5] = dst[3];
		dst[4] = dst[2];
		dst[3] = dst[1];
		dst[2] = tempDst8;
		dst[1] = tempDst7;
		ScalePixel(blendResult.yzwx, k, dst);

		// Rotate the destination pixels back to 0 degrees.
		tempDst8 = dst[8];
		tempDst7 = dst[7];
		dst[8] = dst[6];
		dst[7] = dst[5];
		dst[6] = dst[4];
		dst[5] = dst[3];
		dst[4] = dst[2];
		dst[3] = dst[1];
		dst[2] = tempDst8;
		dst[1] = tempDst7;
	}

	float2 f = frac(pos / float2(inputPtX, inputPtY));
	float3 res = lerp(lerp(dst[6], lerp(dst[7], dst[8], step(two_third, f.x)), step(one_third, f.x)),
		lerp(lerp(dst[5], lerp(dst[0], dst[1], step(two_third, f.x)), step(one_third, f.x)),
			lerp(dst[4], lerp(dst[3], dst[2], step(two_third, f.x)), step(one_third, f.x)), step(two_third, f.y)),
		step(one_third, f.y));
	return float4(res, 1.0);
}
