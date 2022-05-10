// Lanczos6 插值算法
// 移植自 https://github.com/libretro/common-shaders/blob/master/windowed/shaders/lanczos6.cg

//!MAGPIE EFFECT
//!VERSION 2


//!PARAMETER
//!DEFAULT 0.5
//!MIN 0
//!MAX 1
float ARStrength;

//!TEXTURE
Texture2D INPUT;

//!SAMPLER
//!FILTER POINT
SamplerState sam;


//!PASS 1
//!STYLE PS
//!IN INPUT

#define FIX(c) max(abs(c), 1e-5)
#define PI 3.14159265359
#define min4(a, b, c, d) min(min(a, b), min(c, d))
#define max4(a, b, c, d) max(max(a, b), max(c, d))

float3 weight3(float x) {
	const float rcpRadius = 1.0f / 3.0f;
	float3 s = FIX(2.0 * PI * float3(x - 1.5, x - 0.5, x + 0.5));
	// Lanczos3. Note: we normalize outside this function, so no point in multiplying by radius.
	return /*radius **/ sin(s) * sin(s * rcpRadius) * rcp(s * s);
}

float4 Pass1(float2 pos) {
	pos *= GetInputSize();
	float2 inputPt = GetInputPt();

	uint i, j;

	float2 f = frac(pos.xy + 0.5f);
	float3 linetaps1 = weight3(0.5f - f.x * 0.5f);
	float3 linetaps2 = weight3(1.0f - f.x * 0.5f);
	float3 columntaps1 = weight3(0.5f - f.y * 0.5f);
	float3 columntaps2 = weight3(1.0f - f.y * 0.5f);

	// make sure all taps added together is exactly 1.0, otherwise some
	// (very small) distortion can occur
	float suml = dot(linetaps1, float3(1, 1, 1)) + dot(linetaps2, float3(1, 1, 1));
	float sumc = dot(columntaps1, float3(1, 1, 1)) + dot(columntaps2, float3(1, 1, 1));
	linetaps1 /= suml;
	linetaps2 /= suml;
	columntaps1 /= sumc;
	columntaps2 /= sumc;

	pos -= f + 1.5f;

	float3 src[6][6];

	[unroll]
	for (i = 0; i <= 4; i += 2) {
		[unroll]
		for (j = 0; j <= 4; j += 2) {
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

	// final sum and weight normalization

	float3 color = float3(0, 0, 0);
	[unroll]
	for (i = 0; i <= 4; i += 2) {
		color += (mul(linetaps1, float3x3(src[0][i], src[2][i], src[4][i])) + mul(linetaps2, float3x3(src[1][i], src[3][i], src[5][i]))) * columntaps1[i / 2] + (mul(linetaps1, float3x3(src[0][i + 1], src[2][i + 1], src[4][i + 1])) + mul(linetaps2, float3x3(src[1][i + 1], src[3][i + 1], src[5][i + 1]))) * columntaps2[i / 2];
	}

	// 抗振铃
	float3 min_sample = min4(src[2][2], src[3][2], src[2][3], src[3][3]);
	float3 max_sample = max4(src[2][2], src[3][2], src[2][3], src[3][3]);
	color = lerp(color, clamp(color, min_sample, max_sample), ARStrength);

	return float4(color, 1);
}
