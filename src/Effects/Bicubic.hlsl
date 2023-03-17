// Bicubic 插值算法
// 移植自 https://github.com/ActualMandM/cemu_graphic_packs/blob/468d165cf27dae13a06e8bdc3d588d0af775ad91/Filters/Bicubic/output.glsl

//!MAGPIE EFFECT
//!VERSION 3
//!GENERIC_DOWNSCALER


//!PARAMETER
//!LABEL B
//!DEFAULT 0.33
//!MIN 0
//!MAX 1
//!STEP 0.01

float paramB;

//!PARAMETER
//!LABEL C
//!DEFAULT 0.33
//!MIN 0
//!MAX 1
//!STEP 0.01

float paramC;

//!TEXTURE
Texture2D INPUT;

//!SAMPLER
//!FILTER LINEAR
SamplerState sam;


//!PASS 1
//!STYLE PS
//!IN INPUT


float weight(float x) {
	const float B = paramB;
	const float C = paramC;

	float ax = abs(x);

	if (ax < 1.0) {
		return (x * x * ((12.0 - 9.0 * B - 6.0 * C) * ax + (-18.0 + 12.0 * B + 6.0 * C)) + (6.0 - 2.0 * B)) / 6.0;
	} else if (ax >= 1.0 && ax < 2.0) {
		return (x * x * ((-B - 6.0 * C) * ax + (6.0 * B + 30.0 * C)) + (-12.0 * B - 48.0 * C) * ax + (8.0 * B + 24.0 * C)) / 6.0;
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
	const float2 inputPt = GetInputPt();
	const float2 inputSize = GetInputSize();

	pos *= inputSize;
	float2 pos1 = floor(pos - 0.5) + 0.5;
	float2 f = pos - pos1;

	float4 rowtaps = weight4(1 - f.x);
	float4 coltaps = weight4(1 - f.y);

	// make sure all taps added together is exactly 1.0, otherwise some (very small) distortion can occur
	rowtaps /= rowtaps.r + rowtaps.g + rowtaps.b + rowtaps.a;
	coltaps /= coltaps.r + coltaps.g + coltaps.b + coltaps.a;

	float2 uv1 = pos1 * inputPt;
	float2 uv0 = uv1 - inputPt;
	float2 uv2 = uv1 + inputPt;
	float2 uv3 = uv2 + inputPt;

	float u_weight_sum = rowtaps.y + rowtaps.z;
	float u_middle_offset = rowtaps.z * inputPt.x / u_weight_sum;
	float u_middle = uv1.x + u_middle_offset;

	float v_weight_sum = coltaps.y + coltaps.z;
	float v_middle_offset = coltaps.z * inputPt.y / v_weight_sum;
	float v_middle = uv1.y + v_middle_offset;

	int2 coord_top_left = int2(max(uv0 * inputSize, 0.5));
	int2 coord_bottom_right = int2(min(uv3 * inputSize, inputSize - 0.5));

	float4 top = INPUT.Load(int3(coord_top_left, 0)) * rowtaps.x;
	top += INPUT.SampleLevel(sam, float2(u_middle, uv0.y), 0) * u_weight_sum;
	top += INPUT.Load(int3(coord_bottom_right.x, coord_top_left.y, 0)) * rowtaps.w;
	float4 total = top * coltaps.x;

	float4 middle = INPUT.SampleLevel(sam, float2(uv0.x, v_middle), 0) * rowtaps.x;
	middle += INPUT.SampleLevel(sam, float2(u_middle, v_middle), 0) * u_weight_sum;
	middle += INPUT.SampleLevel(sam, float2(uv3.x, v_middle), 0) * rowtaps.w;
	total += middle * v_weight_sum;

	float4 bottom = INPUT.Load(int3(coord_top_left.x, coord_bottom_right.y, 0)) * rowtaps.x;
	bottom += INPUT.SampleLevel(sam, float2(u_middle, uv3.y), 0) * u_weight_sum;
	bottom += INPUT.Load(int3(coord_bottom_right, 0)) * rowtaps.w;
	total += bottom * coltaps.w;

	return total;
}
