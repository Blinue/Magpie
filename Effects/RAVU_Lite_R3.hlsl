// ravu-lite-r3
// 移植自 https://github.com/bjin/mpv-prescalers/blob/master/ravu-lite-r3.hook

//!MAGPIE EFFECT
//!VERSION 1
//!OUTPUT_WIDTH INPUT_WIDTH * 2
//!OUTPUT_HEIGHT INPUT_HEIGHT * 2


//!CONSTANT
//!VALUE INPUT_PT_X
float inputPtX;

//!CONSTANT
//!VALUE INPUT_PT_Y
float inputPtY;


//!TEXTURE
Texture2D INPUT;

//!TEXTURE
//!WIDTH INPUT_WIDTH
//!HEIGHT INPUT_HEIGHT
//!FORMAT B8G8R8A8_UNORM
Texture2D yuvTex;

//!TEXTURE
//!SOURCE RAVU_Lite_R3_Weights.dds
Texture2D ravu_lite_lut3;

//!TEXTURE
//!WIDTH INPUT_WIDTH
//!HEIGHT INPUT_HEIGHT
//!FORMAT B8G8R8A8_UNORM
Texture2D tex1;

//!SAMPLER
//!FILTER POINT
SamplerState sam;


//!PASS 1
//!BIND INPUT
//!SAVE yuvTex

const static float3x3 _rgb2yuv = {
	0.299, 0.587, 0.114,
	-0.169, -0.331, 0.5,
	0.5, -0.419, -0.081
};

float4 Pass1(float2 pos) {
	float3 color = INPUT.Sample(sam, pos).rgb;
	return float4(mul(_rgb2yuv, color) + float3(0, 0.5, 0.5), 1);
}


//!PASS 2
//!BIND yuvTex, ravu_lite_lut3
//!SAVE tex1

#define PI 3.1415926535897932384626433832795


float mod(float x, float y) {
	return x - y * floor(x / y);
}

float4 Pass2(float2 pos) {
	// [0, 5, 10, 15, 20]
	// [1, 6, 11, 16, 21]
	// [2, 7, 12, 17, 22]
	// [3, 8, 13, 18, 23]
	// [4, 9, 14, 19, 24]
	float luma0 = yuvTex.Sample(sam, pos + float2(-2 * inputPtX, -2 * inputPtY)).x;
	float luma1 = yuvTex.Sample(sam, pos + float2(-2 * inputPtX, -inputPtY)).x;
	float luma2 = yuvTex.Sample(sam, pos + float2(-2 * inputPtX, 0)).x;
	float luma3 = yuvTex.Sample(sam, pos + float2(-2 * inputPtX, inputPtY)).x;
	float luma4 = yuvTex.Sample(sam, pos + float2(-2 * inputPtX, 2 * inputPtY)).x;
	float luma5 = yuvTex.Sample(sam, pos + float2(-inputPtX, -2 * inputPtY)).x;
	float luma6 = yuvTex.Sample(sam, pos + float2(-inputPtX, -inputPtY)).x;
	float luma7 = yuvTex.Sample(sam, pos + float2(-inputPtX, 0)).x;
	float luma8 = yuvTex.Sample(sam, pos + float2(-inputPtX, inputPtY)).x;
	float luma9 = yuvTex.Sample(sam, pos + float2(-inputPtX, 2 * inputPtY)).x;
	float luma10 = yuvTex.Sample(sam, pos + float2(0, -2 * inputPtY)).x;
	float luma11 = yuvTex.Sample(sam, pos + float2(0, -inputPtY)).x;
	float luma12 = yuvTex.Sample(sam, pos).x;
	float luma13 = yuvTex.Sample(sam, pos + float2(0, inputPtY)).x;
	float luma14 = yuvTex.Sample(sam, pos + float2(0, 2 * inputPtY)).x;
	float luma15 = yuvTex.Sample(sam, pos + float2(inputPtX, -2 * inputPtY)).x;
	float luma16 = yuvTex.Sample(sam, pos + float2(inputPtX, -inputPtY)).x;
	float luma17 = yuvTex.Sample(sam, pos + float2(inputPtX, 0)).x;
	float luma18 = yuvTex.Sample(sam, pos + float2(inputPtX, inputPtY)).x;
	float luma19 = yuvTex.Sample(sam, pos + float2(inputPtX, 2 * inputPtY)).x;
	float luma20 = yuvTex.Sample(sam, pos + float2(2 * inputPtX, -2 * inputPtY)).x;
	float luma21 = yuvTex.Sample(sam, pos + float2(2 * inputPtX, -inputPtY)).x;
	float luma22 = yuvTex.Sample(sam, pos + float2(2 * inputPtX, 0)).x;
	float luma23 = yuvTex.Sample(sam, pos + float2(2 * inputPtX, inputPtY)).x;
	float luma24 = yuvTex.Sample(sam, pos + float2(2 * inputPtX, 2 * inputPtY)).x;

	float3 abd = 0;
	float gx, gy;
	gx = (luma11 - luma1) / 2.0;
	gy = (luma7 - luma5) / 2.0;
	abd += float3(gx * gx, gx * gy, gy * gy) * 0.1018680644198163;
	gx = (luma12 - luma2) / 2.0;
	gy = (luma8 - luma6) / 2.0;
	abd += float3(gx * gx, gx * gy, gy * gy) * 0.11543163961422666;
	gx = (luma13 - luma3) / 2.0;
	gy = (luma9 - luma7) / 2.0;
	abd += float3(gx * gx, gx * gy, gy * gy) * 0.1018680644198163;
	gx = (luma16 - luma6) / 2.0;
	gy = (luma12 - luma10) / 2.0;
	abd += float3(gx * gx, gx * gy, gy * gy) * 0.11543163961422666;
	gx = (luma17 - luma7) / 2.0;
	gy = (luma13 - luma11) / 2.0;
	abd += float3(gx * gx, gx * gy, gy * gy) * 0.13080118386382833;
	gx = (luma18 - luma8) / 2.0;
	gy = (luma14 - luma12) / 2.0;
	abd += float3(gx * gx, gx * gy, gy * gy) * 0.11543163961422666;
	gx = (luma21 - luma11) / 2.0;
	gy = (luma17 - luma15) / 2.0;
	abd += float3(gx * gx, gx * gy, gy * gy) * 0.1018680644198163;
	gx = (luma22 - luma12) / 2.0;
	gy = (luma18 - luma16) / 2.0;
	abd += float3(gx * gx, gx * gy, gy * gy) * 0.11543163961422666;
	gx = (luma23 - luma13) / 2.0;
	gy = (luma19 - luma17) / 2.0;
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
	float coord_y = ((angle * 4.0 + strength) * 3.0 + coherence + 0.5) / 288.0;
	float4 res = 0, w;
	w = ravu_lite_lut3.Sample(sam, float2(0.038461538461538464, coord_y));
	res += luma0 * w + luma24 * w.wzyx;
	w = ravu_lite_lut3.Sample(sam, float2(0.11538461538461539, coord_y));
	res += luma1 * w + luma23 * w.wzyx;
	w = ravu_lite_lut3.Sample(sam, float2(0.19230769230769232, coord_y));
	res += luma2 * w + luma22 * w.wzyx;
	w = ravu_lite_lut3.Sample(sam, float2(0.2692307692307692, coord_y));
	res += luma3 * w + luma21 * w.wzyx;
	w = ravu_lite_lut3.Sample(sam, float2(0.34615384615384615, coord_y));
	res += luma4 * w + luma20 * w.wzyx;
	w = ravu_lite_lut3.Sample(sam, float2(0.4230769230769231, coord_y));
	res += luma5 * w + luma19 * w.wzyx;
	w = ravu_lite_lut3.Sample(sam, float2(0.5, coord_y));
	res += luma6 * w + luma18 * w.wzyx;
	w = ravu_lite_lut3.Sample(sam, float2(0.5769230769230769, coord_y));
	res += luma7 * w + luma17 * w.wzyx;
	w = ravu_lite_lut3.Sample(sam, float2(0.6538461538461539, coord_y));
	res += luma8 * w + luma16 * w.wzyx;
	w = ravu_lite_lut3.Sample(sam, float2(0.7307692307692307, coord_y));
	res += luma9 * w + luma15 * w.wzyx;
	w = ravu_lite_lut3.Sample(sam, float2(0.8076923076923077, coord_y));
	res += luma10 * w + luma14 * w.wzyx;
	w = ravu_lite_lut3.Sample(sam, float2(0.8846153846153846, coord_y));
	res += luma11 * w + luma13 * w.wzyx;
	w = ravu_lite_lut3.Sample(sam, float2(0.9615384615384616, coord_y));
	res += luma12 * w;
	return saturate(res);

}

//!PASS 3
//!BIND tex1, yuvTex

const static float3x3 _yuv2rgb = {
	1, -0.00093, 1.401687,
	1, -0.3437, -0.71417,
	1, 1.77216, 0.00099
};

float4 Pass3(float2 pos) {
	float2 dir = frac(pos / float2(inputPtX, inputPtY)) - 0.5;
	int idx = int(dir.x > 0.0) * 2 + int(dir.y > 0.0);

	float luma = tex1.Sample(sam, pos - dir * float2(inputPtX, inputPtX))[idx];
	float3 yuv = yuvTex.Sample(sam, pos).xyz;

	return float4(mul(_yuv2rgb, float3(luma, yuv.yz) - float3(0, 0.5, 0.5)), 1);
}
