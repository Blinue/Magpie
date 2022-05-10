// Jinc2 插值算法
// 移植自 https://github.com/libretro/common-shaders/blob/master/windowed/shaders/jinc2.cg
//
// This is an approximation of Jinc(x)*Jinc(x*r1/r2) for x < 2.5,
// where r1 and r2 are the first two zeros of jinc function.
// For a jinc 2-lobe best approximation, use A=0.5 and B=0.825.

// A=0.5, B=0.825 is the best jinc approximation for x<2.5. if B=1.0, it's a lanczos filter.
// Increase A to get more blur. Decrease it to get a sharper picture. 
// B = 0.825 to get rid of dithering. Increase B to get a fine sharpness, though dithering returns.

//!MAGPIE EFFECT
//!VERSION 2


//!PARAMETER
//!DEFAULT 0.5
//!MIN 1e-5
float windowSinc;

//!PARAMETER
//!DEFAULT 0.825
//!MIN 1e-5
float sinc;

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
//!IN INPUT
//!BLOCK_SIZE 8
//!NUM_THREADS 64

#define PI 3.1415926535897932384626433832795

#define min4(a, b, c, d) min(min(a, b), min(c, d))
#define max4(a, b, c, d) max(max(a, b), max(c, d))


float d(float2 pt1, float2 pt2) {
	float2 v = pt2 - pt1;
	return sqrt(dot(v, v));
}

float4 resampler(float4 x, float wa, float wb) {
	return (x == float4(0.0, 0.0, 0.0, 0.0))
		? float4(wa * wb, wa * wb, wa * wb, wa * wb)
		: sin(x * wa) * sin(x * wb) * rcp(x * x);
}

void Pass1(uint2 blockStart, uint3 threadId) {
	uint2 gxy = Rmp8x8(threadId.x) + blockStart;
	if (!CheckViewport(gxy)) {
		return;
	}

	float2 inputPt = GetInputPt();

	float2 dx = float2(1.0, 0.0);
	float2 dy = float2(0.0, 1.0);

	float2 pc = (gxy + 0.5f) * GetOutputPt() * GetInputSize();
	float2 tc = floor(pc - 0.5f) + 0.5f;

	float wa = windowSinc * PI;
	float wb = sinc * PI;
	float4x4 weights = {
		resampler(float4(d(pc, tc - dx - dy), d(pc, tc - dy), d(pc, tc + dx - dy), d(pc, tc + 2.0 * dx - dy)), wa, wb),
		resampler(float4(d(pc, tc - dx), d(pc, tc), d(pc, tc + dx), d(pc, tc + 2.0 * dx)), wa, wb),
		resampler(float4(d(pc, tc - dx + dy), d(pc, tc + dy), d(pc, tc + dx + dy), d(pc, tc + 2.0 * dx + dy)), wa, wb),
		resampler(float4(d(pc, tc - dx + 2.0 * dy), d(pc, tc + 2.0 * dy), d(pc, tc + dx + 2.0 * dy), d(pc, tc + 2.0 * dx + 2.0 * dy)), wa, wb)
	};

	tc -= 0.5f;

	float3 src[4][4];

	[unroll]
	for (uint i = 0; i <= 2; i += 2) {
		[unroll]
		for (uint j = 0; j <= 2; j += 2) {
			float2 tpos = (tc + uint2(i, j)) * inputPt;
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

	float3 color = mul(weights[0], float4x3(src[0][0], src[1][0], src[2][0], src[3][0]));
	color += mul(weights[1], float4x3(src[0][1], src[1][1], src[2][1], src[3][1]));
	color += mul(weights[2], float4x3(src[0][2], src[1][2], src[2][2], src[3][2]));
	color += mul(weights[3], float4x3(src[0][3], src[2][3], src[2][3], src[3][3]));
	color *= rcp(dot(mul(weights, float4(1, 1, 1, 1)), 1));

	// 抗振铃
	// Get min/max samples
	float3 min_sample = min4(src[1][1], src[2][1], src[1][2], src[2][2]);
	float3 max_sample = max4(src[1][1], src[2][1], src[1][2], src[2][2]);
	color = lerp(color, clamp(color, min_sample, max_sample), ARStrength);

	// final sum and weight normalization
	WriteToOutput(gxy, color);
}
