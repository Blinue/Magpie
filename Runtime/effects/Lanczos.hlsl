// Lanczos6 插值算法
// 移植自 https://github.com/libretro/common-shaders/blob/master/windowed/shaders/lanczos6.cg

//!MAGPIE EFFECT
//!VERSION 1


//!CONSTANT
//!VALUE INPUT_PT_X
float inputPtX;

//!CONSTANT
//!VALUE INPUT_PT_Y
float inputPtY;

//!CONSTANT
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
//!BIND INPUT

#define FIX(c) max(abs(c), 1e-5)
#define PI 3.14159265359
#define min4(a, b, c, d) min(min(a, b), min(c, d))
#define max4(a, b, c, d) max(max(a, b), max(c, d))

float3 weight3(float x) {
	const float radius = 3.0;
	float3 s = FIX(2.0 * PI * float3(x - 1.5, x - 0.5, x + 0.5));
	// Lanczos3. Note: we normalize outside this function, so no point in multiplying by radius.
	return /*radius **/ sin(s) * sin(s / radius) / (s * s);
}

float3 line_run(float ypos, float3 xpos1, float3 xpos2, float3 linetaps1, float3 linetaps2) {
	return INPUT.Sample(sam, float2(xpos1.r, ypos)).rgb * linetaps1.r
		+ INPUT.Sample(sam, float2(xpos1.g, ypos)).rgb * linetaps2.r
		+ INPUT.Sample(sam, float2(xpos1.b, ypos)).rgb * linetaps1.g
		+ INPUT.Sample(sam, float2(xpos2.r, ypos)).rgb * linetaps2.g
		+ INPUT.Sample(sam, float2(xpos2.g, ypos)).rgb * linetaps1.b
		+ INPUT.Sample(sam, float2(xpos2.b, ypos)).rgb * linetaps2.b;
}

float4 Pass1(float2 pos) {
	// 用于抗振铃
	float3 neighbors[4] = {
		INPUT.Sample(sam, float2(pos.x - inputPtX, pos.y)).rgb,
		INPUT.Sample(sam, float2(pos.x + inputPtX, pos.y)).rgb,
		INPUT.Sample(sam, float2(pos.x, pos.y - inputPtY)).rgb,
		INPUT.Sample(sam, float2(pos.x, pos.y + inputPtY)).rgb
	};

	float2 f = frac(pos.xy / float2(inputPtX, inputPtY) + 0.5);
	float3 linetaps1 = weight3(0.5 - f.x * 0.5);
	float3 linetaps2 = weight3(1.0 - f.x * 0.5);
	float3 columntaps1 = weight3(0.5 - f.y * 0.5);
	float3 columntaps2 = weight3(1.0 - f.y * 0.5);

	// make sure all taps added together is exactly 1.0, otherwise some
	// (very small) distortion can occur
	float suml = dot(linetaps1, float3(1, 1, 1)) + dot(linetaps2, float3(1, 1, 1));
	float sumc = dot(columntaps1, float3(1, 1, 1)) + dot(columntaps2, float3(1, 1, 1));
	linetaps1 /= suml;
	linetaps2 /= suml;
	columntaps1 /= sumc;
	columntaps2 /= sumc;

	// !!!改变当前坐标
	pos -= (f + 2) * float2(inputPtX, inputPtY);
	float3 xpos1 = float3(pos.x, pos.x + inputPtX, pos.x + 2 * inputPtX);
	float3 xpos2 = float3(pos.x + 3 * inputPtX, pos.x + 4 * inputPtX, pos.x + 5 * inputPtX);

	// final sum and weight normalization
	float3 color = line_run(pos.y, xpos1, xpos2, linetaps1, linetaps2) * columntaps1.r
		+ line_run(pos.y + inputPtY, xpos1, xpos2, linetaps1, linetaps2) * columntaps2.r
		+ line_run(pos.y + 2 * inputPtY, xpos1, xpos2, linetaps1, linetaps2) * columntaps1.g
		+ line_run(pos.y + 3 * inputPtY, xpos1, xpos2, linetaps1, linetaps2) * columntaps2.g
		+ line_run(pos.y + 4 * inputPtY, xpos1, xpos2, linetaps1, linetaps2) * columntaps1.b
		+ line_run(pos.y + 5 * inputPtY, xpos1, xpos2, linetaps1, linetaps2) * columntaps2.b;

	// 抗振铃
	float3 min_sample = min4(neighbors[0], neighbors[1], neighbors[2], neighbors[3]);
	float3 max_sample = max4(neighbors[0], neighbors[1], neighbors[2], neighbors[3]);
	color = lerp(color, clamp(color, min_sample, max_sample), ARStrength);

	return float4(color, 1);
}
