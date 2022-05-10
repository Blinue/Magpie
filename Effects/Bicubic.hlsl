// Bicubic 插值算法
// 移植自 https://github.com/libretro/common-shaders/blob/master/bicubic/shaders/bicubic-normal.cg

//!MAGPIE EFFECT
//!VERSION 2


//!PARAMETER
//!DEFAULT 0.333333
//!MIN 0
//!MAX 1

float paramB;

//!PARAMETER
//!DEFAULT 0.333333
//!MIN 0
//!MAX 1

float paramC;

//!TEXTURE
Texture2D INPUT;

//!SAMPLER
//!FILTER POINT
SamplerState sam;


//!PASS 1
//!STYLE PS
//!IN INPUT


float weight(float x) {
	float ax = abs(x);

	if (ax < 1.0) {
		return (x * x * ((12.0 - 9.0 * paramB - 6.0 * paramC) * ax + (-18.0 + 12.0 * paramB + 6.0 * paramC)) + (6.0 - 2.0 * paramB)) / 6.0;
	} else if (ax >= 1.0 && ax < 2.0) {
		return (x * x * ((-paramB - 6.0 * paramC) * ax + (6.0 * paramB + 30.0 * paramC)) + (-12.0 * paramB - 48.0 * paramC) * ax + (8.0 * paramB + 24.0 * paramC)) / 6.0;
	} else {
		return 0.0;
	}
}

float4 weight4(float x) {
	return float4(
		weight(x - 2.0),
		weight(x - 1.0),
		weight(x),
		weight(x + 1.0)
	);
}


float4 Pass1(float2 pos) {
	float2 inputPt = GetInputPt();
	uint i, j;

	pos *= GetInputSize();
	float2 f = frac(pos + 0.5f);

	float4 linetaps = weight4(1.0 - f.x);
	float4 columntaps = weight4(1.0 - f.y);

	// make sure all taps added together is exactly 1.0, otherwise some (very small) distortion can occur
	linetaps /= linetaps.r + linetaps.g + linetaps.b + linetaps.a;
	columntaps /= columntaps.r + columntaps.g + columntaps.b + columntaps.a;

	pos -= f + 0.5f;

	float3 src[4][4];

	[unroll]
	for (i = 0; i <= 2; i += 2) {
		[unroll]
		for (j = 0; j <= 2; j += 2) {
			float2 tpos = (pos + uint2(i, j)) * inputPt;
			const float4 sr = INPUT.GatherRed(sam, tpos);
			const float4 sg = INPUT.GatherGreen(sam, tpos);
			const float4 sb = INPUT.GatherBlue(sam, tpos);

			// w z
			// x y
			src[i][j] = float3(sr.w, sg.w, sb.w);
			src[i][j + 1] = float3(sr.x, sg.x, sb.x);
			src[i + 1][j] = float3(sr.z, sg.z, sb.z);
			src[i + 1][j + 1] = float3(sr.y, sg.y, sb.y);
		}
	}

	float3 result = float3(0, 0, 0);
	[unroll]
	for (i = 0; i < 4; ++i) {
		result += mul(linetaps, float4x3(src[0][i], src[1][i], src[2][i], src[3][i])) * columntaps[i];
	}

	return float4(result, 1);
}
