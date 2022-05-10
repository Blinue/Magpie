// ravu-lite-r3
// 移植自 https://github.com/bjin/mpv-prescalers/blob/master/ravu-lite-r3.hook

//!MAGPIE EFFECT
//!VERSION 2
//!OUTPUT_WIDTH INPUT_WIDTH * 2
//!OUTPUT_HEIGHT INPUT_HEIGHT * 2


//!TEXTURE
Texture2D INPUT;

//!TEXTURE
//!SOURCE RAVU_Lite_R3_Weights.dds
//!FORMAT R16G16B16A16_FLOAT
Texture2D ravu_lite_lut3;

//!SAMPLER
//!FILTER POINT
SamplerState sam;


//!PASS 1
//!IN INPUT, ravu_lite_lut3
//!BLOCK_SIZE 32, 16
//!NUM_THREADS 16, 8

#pragma warning(disable: 3557) // X3557: loop only executes for 1 iteration(s), forcing loop to unroll

#define NUM_PIXELS_X (MP_BLOCK_WIDTH + 4)
#define NUM_PIXELS_Y (MP_BLOCK_HEIGHT + 4)

groupshared float inp[NUM_PIXELS_Y][NUM_PIXELS_X];

#define PI 3.1415926535897932384626433832795

float GetLuma(float3 color) {
	return dot(float3(0.299f, 0.587f, 0.114f), color);
}

const static float2x3 rgb2uv = {
	-0.169, -0.331, 0.5,
	0.5, -0.419, -0.081
};

const static float3x3 yuv2rgb = {
	1, -0.00093, 1.401687,
	1, -0.3437, -0.71417,
	1, 1.77216, 0.00099
};

float mod(float x, float y) {
	return x - y * floor(x / y);
}

void Pass1(uint2 blockStart, uint3 threadId) {
	const float2 inputPt = GetInputPt();

	for (uint id = threadId.y * MP_NUM_THREADS_X + threadId.x; id < NUM_PIXELS_X * NUM_PIXELS_Y; id += MP_NUM_THREADS_X * MP_NUM_THREADS_Y) {
		uint2 pos = { id % NUM_PIXELS_X, id / NUM_PIXELS_X };
		inp[pos.y][pos.x] = GetLuma(INPUT.SampleLevel(sam, inputPt * ((blockStart / 2) + pos - 1.5f), 0).rgb);
	}

	GroupMemoryBarrierWithGroupSync();

	uint2 destPos = blockStart + threadId.xy * 2;
	if (!CheckViewport(destPos)) {
		return;
	}

	float src[5][5];
	for (uint i = 0; i < 5; ++i) {
		for (uint j = 0; j < 5; ++j) {
			src[j][i] = inp[threadId.y + i][threadId.x + j];
		}
	}

	float3 abd = 0;
	float gx, gy;
	gx = (src[2][1] - src[0][1]) / 2.0;
	gy = (src[1][2] - src[1][0]) / 2.0;
	abd += float3(gx * gx, gx * gy, gy * gy) * 0.1018680644198163;
	gx = (src[2][2] - src[0][2]) / 2.0;
	gy = (src[1][3] - src[1][1]) / 2.0;
	abd += float3(gx * gx, gx * gy, gy * gy) * 0.11543163961422666;
	gx = (src[2][3] - src[0][3]) / 2.0;
	gy = (src[1][4] - src[1][2]) / 2.0;
	abd += float3(gx * gx, gx * gy, gy * gy) * 0.1018680644198163;
	gx = (src[3][1] - src[1][1]) / 2.0;
	gy = (src[2][2] - src[2][0]) / 2.0;
	abd += float3(gx * gx, gx * gy, gy * gy) * 0.11543163961422666;
	gx = (src[3][2] - src[1][2]) / 2.0;
	gy = (src[2][3] - src[2][1]) / 2.0;
	abd += float3(gx * gx, gx * gy, gy * gy) * 0.13080118386382833;
	gx = (src[3][3] - src[1][3]) / 2.0;
	gy = (src[2][4] - src[2][2]) / 2.0;
	abd += float3(gx * gx, gx * gy, gy * gy) * 0.11543163961422666;
	gx = (src[4][1] - src[2][1]) / 2.0;
	gy = (src[3][2] - src[3][0]) / 2.0;
	abd += float3(gx * gx, gx * gy, gy * gy) * 0.1018680644198163;
	gx = (src[4][2] - src[2][2]) / 2.0;
	gy = (src[3][3] - src[3][1]) / 2.0;
	abd += float3(gx * gx, gx * gy, gy * gy) * 0.11543163961422666;
	gx = (src[4][3] - src[2][3]) / 2.0;
	gy = (src[3][4] - src[3][2]) / 2.0;
	abd += float3(gx * gx, gx * gy, gy * gy) * 0.1018680644198163;
	float a = abd.x, b = abd.y, d = abd.z;
	float T = a + d, D = a * d - b * b;
	float delta = sqrt(max(T * T / 4.0 - D, 0.0));
	float L1 = T / 2.0 + delta, L2 = T / 2.0 - delta;
	float sqrtL1 = sqrt(L1), sqrtL2 = sqrt(L2);
	float theta = lerp(mod(atan2(b, L1 - a) + PI, PI), 0.0, abs(b) < 1.192092896e-7);
	float lambda = sqrtL1;
	float mu = lerp((sqrtL1 - sqrtL2) / (sqrtL1 + sqrtL2), 0.0, sqrtL1 + sqrtL2 < 1.192092896e-7);
	float angle = floor(theta * 24.0 / PI);
	float strength = lerp(lerp(0.0, 1.0, lambda >= 0.004), lerp(2.0, 3.0, lambda >= 0.05), lambda >= 0.016);
	float coherence = lerp(lerp(0.0, 1.0, mu >= 0.25), 2.0, mu >= 0.5);
	float coord_y = ((angle * 4.0f + strength) * 3.0f + coherence + 0.5f) / 288.0f;
	float4 res = 0, w;
	w = ravu_lite_lut3.SampleLevel(sam, float2(0.038461538461538464, coord_y), 0);
	res += src[0][0] * w + src[4][4] * w.wzyx;
	w = ravu_lite_lut3.SampleLevel(sam, float2(0.11538461538461539, coord_y), 0);
	res += src[0][1] * w + src[4][3] * w.wzyx;
	w = ravu_lite_lut3.SampleLevel(sam, float2(0.19230769230769232, coord_y), 0);
	res += src[0][2] * w + src[4][2] * w.wzyx;
	w = ravu_lite_lut3.SampleLevel(sam, float2(0.2692307692307692, coord_y), 0);
	res += src[0][3] * w + src[4][1] * w.wzyx;
	w = ravu_lite_lut3.SampleLevel(sam, float2(0.34615384615384615, coord_y), 0);
	res += src[0][4] * w + src[4][0] * w.wzyx;
	w = ravu_lite_lut3.SampleLevel(sam, float2(0.4230769230769231, coord_y), 0);
	res += src[1][0] * w + src[3][4] * w.wzyx;
	w = ravu_lite_lut3.SampleLevel(sam, float2(0.5, coord_y), 0);
	res += src[1][1] * w + src[3][3] * w.wzyx;
	w = ravu_lite_lut3.SampleLevel(sam, float2(0.5769230769230769, coord_y), 0);
	res += src[1][2] * w + src[3][2] * w.wzyx;
	w = ravu_lite_lut3.SampleLevel(sam, float2(0.6538461538461539, coord_y), 0);
	res += src[1][3] * w + src[3][1] * w.wzyx;
	w = ravu_lite_lut3.SampleLevel(sam, float2(0.7307692307692307, coord_y), 0);
	res += src[1][4] * w + src[3][0] * w.wzyx;
	w = ravu_lite_lut3.SampleLevel(sam, float2(0.8076923076923077, coord_y), 0);
	res += src[2][0] * w + src[2][4] * w.wzyx;
	w = ravu_lite_lut3.SampleLevel(sam, float2(0.8846153846153846, coord_y), 0);
	res += src[2][1] * w + src[2][3] * w.wzyx;
	w = ravu_lite_lut3.SampleLevel(sam, float2(0.9615384615384616, coord_y), 0);
	res += src[2][2] * w;
	res = saturate(res);

	float2 originUV = mul(rgb2uv, INPUT.SampleLevel(sam, inputPt * (destPos / 2 + 0.5f), 0).rgb);
	WriteToOutput(destPos, mul(yuv2rgb, float3(res.x, originUV)));

	++destPos.y;
	if (CheckViewport(destPos)) {
		WriteToOutput(destPos, mul(yuv2rgb, float3(res.y, originUV)));
	}

	++destPos.x;
	if (CheckViewport(destPos)) {
		WriteToOutput(destPos, mul(yuv2rgb, float3(res.w, originUV)));
	}

	--destPos.y;
	if (CheckViewport(destPos)) {
		WriteToOutput(destPos, mul(yuv2rgb, float3(res.z, originUV)));
	}
}
