// 移植自 https://raw.githubusercontent.com/bjin/mpv-prescalers/master/compute/ravu-zoom-r3.hook

//!MAGPIE EFFECT
//!VERSION 2


//!TEXTURE
Texture2D INPUT;

//!TEXTURE
//!SOURCE RAVU_Zoom_R3_Weights.dds
//!FORMAT R16G16B16A16_FLOAT
Texture2D ravu_zoom_lut3;


//!SAMPLER
//!FILTER POINT
SamplerState sam;

//!SAMPLER
//!FILTER LINEAR
SamplerState sam1;


//!PASS 1
//!IN INPUT, ravu_zoom_lut3
//!BLOCK_SIZE 16, 16
//!NUM_THREADS 16, 16

#define NUM_PIXELS_X (MP_BLOCK_WIDTH + 5)
#define NUM_PIXELS_Y (MP_BLOCK_HEIGHT + 5)

groupshared float samples[NUM_PIXELS_X * NUM_PIXELS_Y];

float GetLuma(float3 color) {
	return dot(float3(0.299f, 0.587f, 0.114f), color);
}

#define PI 3.1415926535897932384626433832795

// https://github.com/mpv-player/mpv/issues/9390#issuecomment-961082863
#define LUT_POS(x, lut_size) lerp(0.5 / (lut_size), 1.0 - 0.5 / (lut_size), (x))

const static float3x3 yuv2rgb = {
	1, -0.00093, 1.401687,
	1, -0.3437, -0.71417,
	1, 1.77216, 0.00099
};

const static float2x3 rgb2uv = {
	-0.169, -0.331, 0.5,
	0.5, -0.419, -0.081
};

float mod(float x, float y) {
	return x - y * floor(x / y);
}


void Pass1(uint2 blockStart, uint3 threadId) {
	const float2 inputPt = GetInputPt();
	const uint2 inputSize = GetInputSize();
	const float2 rcpScale = rcp(GetScale());

	const int2 rectl = floor(blockStart * rcpScale - 0.5f) - 2;
	const int2 rectr = floor((blockStart + uint2(MP_BLOCK_WIDTH, MP_BLOCK_HEIGHT) - 1) * rcpScale - 0.5f) + 3;
	const uint2 rect = uint2(rectr - rectl + 1);

	const uint maxId = rect.x * rect.y;

	for (uint id = threadId.y * MP_NUM_THREADS_X + threadId.x; id < maxId; id += MP_NUM_THREADS_X * MP_NUM_THREADS_Y) {
		uint y = id / rect.x, x = id % rect.x;
		samples[x + y * NUM_PIXELS_X] = GetLuma(INPUT.SampleLevel(sam, inputPt * (rectl + uint2(x, y) + 0.5f), 0).rgb);
	}

	GroupMemoryBarrierWithGroupSync();

	uint2 destPos = blockStart + threadId.xy;
	if (!CheckViewport(destPos)) {
		return;
	}

	float2 pos = (destPos + 0.5f) * rcpScale;
	float2 subpix = frac(pos - 0.5f);
	pos -= subpix;
	subpix = LUT_POS(subpix, 9);
	float2 subpix_inv = 1.0 - subpix;
	subpix /= float2(5.0, 288.0);
	subpix_inv /= float2(5.0, 288.0);
	uint2 ipos = uint2(floor(pos) - rectl);
	uint lpos = ipos.x + ipos.y * NUM_PIXELS_X;
	float sample0 = samples[lpos - 2 * NUM_PIXELS_X - 2];
	float sample1 = samples[lpos - NUM_PIXELS_X - 2];
	float sample2 = samples[lpos - 2];
	float sample3 = samples[lpos + NUM_PIXELS_X - 2];
	float sample4 = samples[lpos + 2 * NUM_PIXELS_X - 2];
	float sample5 = samples[lpos + 3 * NUM_PIXELS_X - 2];
	float sample6 = samples[lpos - 2 * NUM_PIXELS_X - 1];
	float sample7 = samples[lpos - NUM_PIXELS_X - 1];
	float sample8 = samples[lpos - 1];
	float sample9 = samples[lpos + NUM_PIXELS_X - 1];
	float sample10 = samples[lpos + 2 * NUM_PIXELS_X - 1];
	float sample11 = samples[lpos + 3 * NUM_PIXELS_X - 1];
	float sample12 = samples[lpos - 2 * NUM_PIXELS_X];
	float sample13 = samples[lpos - NUM_PIXELS_X];
	float sample14 = samples[lpos];
	float sample15 = samples[lpos + NUM_PIXELS_X];
	float sample16 = samples[lpos + 2 * NUM_PIXELS_X];
	float sample17 = samples[lpos + 3 * NUM_PIXELS_X];
	float sample18 = samples[lpos - 2 * NUM_PIXELS_X + 1];
	float sample19 = samples[lpos - NUM_PIXELS_X + 1];
	float sample20 = samples[lpos + 1];
	float sample21 = samples[lpos + NUM_PIXELS_X + 1];
	float sample22 = samples[lpos + 2 * NUM_PIXELS_X + 1];
	float sample23 = samples[lpos + 3 * NUM_PIXELS_X + 1];
	float sample24 = samples[lpos - 2 * NUM_PIXELS_X + 2];
	float sample25 = samples[lpos - NUM_PIXELS_X + 2];
	float sample26 = samples[lpos + 2];
	float sample27 = samples[lpos + NUM_PIXELS_X + 2];
	float sample28 = samples[lpos + 2 * NUM_PIXELS_X + 2];
	float sample29 = samples[lpos + 3 * NUM_PIXELS_X + 2];
	float sample30 = samples[lpos - 2 * NUM_PIXELS_X + 3];
	float sample31 = samples[lpos - NUM_PIXELS_X + 3];
	float sample32 = samples[lpos + 3];
	float sample33 = samples[lpos + NUM_PIXELS_X + 3];
	float sample34 = samples[lpos + 2 * NUM_PIXELS_X + 3];
	float sample35 = samples[lpos + 3 * NUM_PIXELS_X + 3];
	float3 abd = 0;
	float gx, gy;
	gx = (sample13 - sample1) / 2.0;
	gy = (sample8 - sample6) / 2.0;
	abd += float3(gx * gx, gx * gy, gy * gy) * 0.04792235409415088;
	gx = (sample14 - sample2) / 2.0;
	gy = (-sample10 + 8.0 * sample9 - 8.0 * sample7 + sample6) / 12.0;
	abd += float3(gx * gx, gx * gy, gy * gy) * 0.06153352068439959;
	gx = (sample15 - sample3) / 2.0;
	gy = (-sample11 + 8.0 * sample10 - 8.0 * sample8 + sample7) / 12.0;
	abd += float3(gx * gx, gx * gy, gy * gy) * 0.06153352068439959;
	gx = (sample16 - sample4) / 2.0;
	gy = (sample11 - sample9) / 2.0;
	abd += float3(gx * gx, gx * gy, gy * gy) * 0.04792235409415088;
	gx = (-sample25 + 8.0 * sample19 - 8.0 * sample7 + sample1) / 12.0;
	gy = (sample14 - sample12) / 2.0;
	abd += float3(gx * gx, gx * gy, gy * gy) * 0.06153352068439959;
	gx = (-sample26 + 8.0 * sample20 - 8.0 * sample8 + sample2) / 12.0;
	gy = (-sample16 + 8.0 * sample15 - 8.0 * sample13 + sample12) / 12.0;
	abd += float3(gx * gx, gx * gy, gy * gy) * 0.07901060453704994;
	gx = (-sample27 + 8.0 * sample21 - 8.0 * sample9 + sample3) / 12.0;
	gy = (-sample17 + 8.0 * sample16 - 8.0 * sample14 + sample13) / 12.0;
	abd += float3(gx * gx, gx * gy, gy * gy) * 0.07901060453704994;
	gx = (-sample28 + 8.0 * sample22 - 8.0 * sample10 + sample4) / 12.0;
	gy = (sample17 - sample15) / 2.0;
	abd += float3(gx * gx, gx * gy, gy * gy) * 0.06153352068439959;
	gx = (-sample31 + 8.0 * sample25 - 8.0 * sample13 + sample7) / 12.0;
	gy = (sample20 - sample18) / 2.0;
	abd += float3(gx * gx, gx * gy, gy * gy) * 0.06153352068439959;
	gx = (-sample32 + 8.0 * sample26 - 8.0 * sample14 + sample8) / 12.0;
	gy = (-sample22 + 8.0 * sample21 - 8.0 * sample19 + sample18) / 12.0;
	abd += float3(gx * gx, gx * gy, gy * gy) * 0.07901060453704994;
	gx = (-sample33 + 8.0 * sample27 - 8.0 * sample15 + sample9) / 12.0;
	gy = (-sample23 + 8.0 * sample22 - 8.0 * sample20 + sample19) / 12.0;
	abd += float3(gx * gx, gx * gy, gy * gy) * 0.07901060453704994;
	gx = (-sample34 + 8.0 * sample28 - 8.0 * sample16 + sample10) / 12.0;
	gy = (sample23 - sample21) / 2.0;
	abd += float3(gx * gx, gx * gy, gy * gy) * 0.06153352068439959;
	gx = (sample31 - sample19) / 2.0;
	gy = (sample26 - sample24) / 2.0;
	abd += float3(gx * gx, gx * gy, gy * gy) * 0.04792235409415088;
	gx = (sample32 - sample20) / 2.0;
	gy = (-sample28 + 8.0 * sample27 - 8.0 * sample25 + sample24) / 12.0;
	abd += float3(gx * gx, gx * gy, gy * gy) * 0.06153352068439959;
	gx = (sample33 - sample21) / 2.0;
	gy = (-sample29 + 8.0 * sample28 - 8.0 * sample26 + sample25) / 12.0;
	abd += float3(gx * gx, gx * gy, gy * gy) * 0.06153352068439959;
	gx = (sample34 - sample22) / 2.0;
	gy = (sample29 - sample27) / 2.0;
	abd += float3(gx * gx, gx * gy, gy * gy) * 0.04792235409415088;
	float a = abd.x, b = abd.y, d = abd.z;
	float T = a + d, D = a * d - b * b;
	float delta = sqrt(max(T * T / 4.0 - D, 0.0));
	float L1 = T / 2.0 + delta, L2 = T / 2.0 - delta;
	float sqrtL1 = sqrt(L1), sqrtL2 = sqrt(L2);
	float theta = lerp(mod(atan2(L1 - a, b) + 3.141592653589793, 3.141592653589793), 0.0, abs(b) < 1.192092896e-7);
	float lambda = sqrtL1;
	float mu = lerp((sqrtL1 - sqrtL2) / (sqrtL1 + sqrtL2), 0.0, sqrtL1 + sqrtL2 < 1.192092896e-7);
	float angle = floor(theta * 24.0 / 3.141592653589793);
	float strength = lerp(lerp(0.0, 1.0, lambda >= 0.004), lerp(2.0, 3.0, lambda >= 0.05), lambda >= 0.016);
	float coherence = lerp(lerp(0.0, 1.0, mu >= 0.25), 2.0, mu >= 0.5);
	float coord_y = ((angle * 4.0 + strength) * 3.0 + coherence) / 288.0;
	float res = 0.0;
	float4 w;
	w = ravu_zoom_lut3.SampleLevel(sam1, float2(0.0, coord_y) + subpix, 0);
	res += sample0 * w[0];
	res += sample1 * w[1];
	res += sample2 * w[2];
	res += sample3 * w[3];
	w = ravu_zoom_lut3.SampleLevel(sam1, float2(0.2, coord_y) + subpix, 0);
	res += sample4 * w[0];
	res += sample5 * w[1];
	res += sample6 * w[2];
	res += sample7 * w[3];
	w = ravu_zoom_lut3.SampleLevel(sam1, float2(0.4, coord_y) + subpix, 0);
	res += sample8 * w[0];
	res += sample9 * w[1];
	res += sample10 * w[2];
	res += sample11 * w[3];
	w = ravu_zoom_lut3.SampleLevel(sam1, float2(0.6, coord_y) + subpix, 0);
	res += sample12 * w[0];
	res += sample13 * w[1];
	res += sample14 * w[2];
	res += sample15 * w[3];
	w = ravu_zoom_lut3.SampleLevel(sam1, float2(0.8, coord_y) + subpix, 0);
	res += sample16 * w[0];
	res += sample17 * w[1];
	w = ravu_zoom_lut3.SampleLevel(sam1, float2(0.0, coord_y) + subpix_inv, 0);
	res += sample35 * w[0];
	res += sample34 * w[1];
	res += sample33 * w[2];
	res += sample32 * w[3];
	w = ravu_zoom_lut3.SampleLevel(sam1, float2(0.2, coord_y) + subpix_inv, 0);
	res += sample31 * w[0];
	res += sample30 * w[1];
	res += sample29 * w[2];
	res += sample28 * w[3];
	w = ravu_zoom_lut3.SampleLevel(sam1, float2(0.4, coord_y) + subpix_inv, 0);
	res += sample27 * w[0];
	res += sample26 * w[1];
	res += sample25 * w[2];
	res += sample24 * w[3];
	w = ravu_zoom_lut3.SampleLevel(sam1, float2(0.6, coord_y) + subpix_inv, 0);
	res += sample23 * w[0];
	res += sample22 * w[1];
	res += sample21 * w[2];
	res += sample20 * w[3];
	w = ravu_zoom_lut3.SampleLevel(sam1, float2(0.8, coord_y) + subpix_inv, 0);
	res += sample19 * w[0];
	res += sample18 * w[1];
	res = saturate(res);

	float2 originUV = mul(rgb2uv, INPUT.SampleLevel(sam1, (destPos + 0.5f) * GetOutputPt(), 0).rgb);
	WriteToOutput(destPos, mul(yuv2rgb, float3(res, originUV)));
}
