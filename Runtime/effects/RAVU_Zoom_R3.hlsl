// 移植自 https://github.com/bjin/mpv-prescalers/blob/master/ravu-zoom-r3.hook

//!MAGPIE EFFECT
//!VERSION 1


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
//!SOURCE RAVU_Zoom_R3_Weights.dds
Texture2D ravu_zoom_lut3;


//!SAMPLER
//!FILTER POINT
SamplerState sam;

//!SAMPLER
//!FILTER LINEAR
SamplerState sam1;


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
//!BIND yuvTex, ravu_zoom_lut3


#define PI 3.1415926535897932384626433832795

// https://github.com/mpv-player/mpv/issues/9390#issuecomment-961082863
#define LUT_POS(x, lut_size) lerp(0.5 / (lut_size), 1.0 - 0.5 / (lut_size), (x))

const static float3x3 _yuv2rgb = {
	1, -0.00093, 1.401687,
	1, -0.3437, -0.71417,
	1, 1.77216, 0.00099
};

float mod(float x, float y) {
	return x - y * floor(x / y);
}


float4 Pass2(float2 pos) {
	float2 inputPt = { inputPtX, inputPtY };
	pos /= inputPt;
	float2 subpix = frac(pos - 0.5);
	pos -= subpix;

	subpix = LUT_POS(subpix, 9);

	float2 subpix_inv = 1.0 - subpix;
	subpix /= float2(5.0, 288.0);
	subpix_inv /= float2(5.0, 288.0);

	float3 sample0 = yuvTex.Sample(sam, (pos + float2(-2.0, -2.0)) * inputPt).xyz;
	float3 sample1 = yuvTex.Sample(sam, (pos + float2(-2.0, -1.0)) * inputPt).xyz;
	float3 sample2 = yuvTex.Sample(sam, (pos + float2(-2.0, 0.0)) * inputPt).xyz;
	float3 sample3 = yuvTex.Sample(sam, (pos + float2(-2.0, 1.0)) * inputPt).xyz;
	float3 sample4 = yuvTex.Sample(sam, (pos + float2(-2.0, 2.0)) * inputPt).xyz;
	float3 sample5 = yuvTex.Sample(sam, (pos + float2(-2.0, 3.0)) * inputPt).xyz;
	float3 sample6 = yuvTex.Sample(sam, (pos + float2(-1.0, -2.0)) * inputPt).xyz;
	float3 sample7 = yuvTex.Sample(sam, (pos + float2(-1.0, -1.0)) * inputPt).xyz;
	float3 sample8 = yuvTex.Sample(sam, (pos + float2(-1.0, 0.0)) * inputPt).xyz;
	float3 sample9 = yuvTex.Sample(sam, (pos + float2(-1.0, 1.0)) * inputPt).xyz;
	float3 sample10 = yuvTex.Sample(sam, (pos + float2(-1.0, 2.0)) * inputPt).xyz;
	float3 sample11 = yuvTex.Sample(sam, (pos + float2(-1.0, 3.0)) * inputPt).xyz;
	float3 sample12 = yuvTex.Sample(sam, (pos + float2(0.0, -2.0)) * inputPt).xyz;
	float3 sample13 = yuvTex.Sample(sam, (pos + float2(0.0, -1.0)) * inputPt).xyz;
	float3 sample14 = yuvTex.Sample(sam, pos * inputPt).xyz;
	float3 sample15 = yuvTex.Sample(sam, (pos + float2(0.0, 1.0)) * inputPt).xyz;
	float3 sample16 = yuvTex.Sample(sam, (pos + float2(0.0, 2.0)) * inputPt).xyz;
	float3 sample17 = yuvTex.Sample(sam, (pos + float2(0.0, 3.0)) * inputPt).xyz;
	float3 sample18 = yuvTex.Sample(sam, (pos + float2(1.0, -2.0)) * inputPt).xyz;
	float3 sample19 = yuvTex.Sample(sam, (pos + float2(1.0, -1.0)) * inputPt).xyz;
	float3 sample20 = yuvTex.Sample(sam, (pos + float2(1.0, 0.0)) * inputPt).xyz;
	float3 sample21 = yuvTex.Sample(sam, (pos + float2(1.0, 1.0)) * inputPt).xyz;
	float3 sample22 = yuvTex.Sample(sam, (pos + float2(1.0, 2.0)) * inputPt).xyz;
	float3 sample23 = yuvTex.Sample(sam, (pos + float2(1.0, 3.0)) * inputPt).xyz;
	float3 sample24 = yuvTex.Sample(sam, (pos + float2(2.0, -2.0)) * inputPt).xyz;
	float3 sample25 = yuvTex.Sample(sam, (pos + float2(2.0, -1.0)) * inputPt).xyz;
	float3 sample26 = yuvTex.Sample(sam, (pos + float2(2.0, 0.0)) * inputPt).xyz;
	float3 sample27 = yuvTex.Sample(sam, (pos + float2(2.0, 1.0)) * inputPt).xyz;
	float3 sample28 = yuvTex.Sample(sam, (pos + float2(2.0, 2.0)) * inputPt).xyz;
	float3 sample29 = yuvTex.Sample(sam, (pos + float2(2.0, 3.0)) * inputPt).xyz;
	float3 sample30 = yuvTex.Sample(sam, (pos + float2(3.0, -2.0)) * inputPt).xyz;
	float3 sample31 = yuvTex.Sample(sam, (pos + float2(3.0, -1.0)) * inputPt).xyz;
	float3 sample32 = yuvTex.Sample(sam, (pos + float2(3.0, 0.0)) * inputPt).xyz;
	float3 sample33 = yuvTex.Sample(sam, (pos + float2(3.0, 1.0)) * inputPt).xyz;
	float3 sample34 = yuvTex.Sample(sam, (pos + float2(3.0, 2.0)) * inputPt).xyz;
	float3 sample35 = yuvTex.Sample(sam, (pos + float2(3.0, 3.0)) * inputPt).xyz;

	float3 abd = 0;
	float gx, gy;
	gx = (sample13.x - sample1.x) / 2.0;
	gy = (sample8.x - sample6.x) / 2.0;
	abd += float3(gx * gx, gx * gy, gy * gy) * 0.04792235409415088;
	gx = (sample14.x - sample2.x) / 2.0;
	gy = (-sample10.x + 8.0 * sample9.x - 8.0 * sample7.x + sample6.x) / 12.0;
	abd += float3(gx * gx, gx * gy, gy * gy) * 0.06153352068439959;
	gx = (sample15.x - sample3.x) / 2.0;
	gy = (-sample11.x + 8.0 * sample10.x - 8.0 * sample8.x + sample7.x) / 12.0;
	abd += float3(gx * gx, gx * gy, gy * gy) * 0.06153352068439959;
	gx = (sample16.x - sample4.x) / 2.0;
	gy = (sample11.x - sample9.x) / 2.0;
	abd += float3(gx * gx, gx * gy, gy * gy) * 0.04792235409415088;
	gx = (-sample25.x + 8.0 * sample19.x - 8.0 * sample7.x + sample1.x) / 12.0;
	gy = (sample14.x - sample12.x) / 2.0;
	abd += float3(gx * gx, gx * gy, gy * gy) * 0.06153352068439959;
	gx = (-sample26.x + 8.0 * sample20.x - 8.0 * sample8.x + sample2.x) / 12.0;
	gy = (-sample16.x + 8.0 * sample15.x - 8.0 * sample13.x + sample12.x) / 12.0;
	abd += float3(gx * gx, gx * gy, gy * gy) * 0.07901060453704994;
	gx = (-sample27.x + 8.0 * sample21.x - 8.0 * sample9.x + sample3.x) / 12.0;
	gy = (-sample17.x + 8.0 * sample16.x - 8.0 * sample14.x + sample13.x) / 12.0;
	abd += float3(gx * gx, gx * gy, gy * gy) * 0.07901060453704994;
	gx = (-sample28.x + 8.0 * sample22.x - 8.0 * sample10.x + sample4.x) / 12.0;
	gy = (sample17.x - sample15.x) / 2.0;
	abd += float3(gx * gx, gx * gy, gy * gy) * 0.06153352068439959;
	gx = (-sample31.x + 8.0 * sample25.x - 8.0 * sample13.x + sample7.x) / 12.0;
	gy = (sample20.x - sample18.x) / 2.0;
	abd += float3(gx * gx, gx * gy, gy * gy) * 0.06153352068439959;
	gx = (-sample32.x + 8.0 * sample26.x - 8.0 * sample14.x + sample8.x) / 12.0;
	gy = (-sample22.x + 8.0 * sample21.x - 8.0 * sample19.x + sample18.x) / 12.0;
	abd += float3(gx * gx, gx * gy, gy * gy) * 0.07901060453704994;
	gx = (-sample33.x + 8.0 * sample27.x - 8.0 * sample15.x + sample9.x) / 12.0;
	gy = (-sample23.x + 8.0 * sample22.x - 8.0 * sample20.x + sample19.x) / 12.0;
	abd += float3(gx * gx, gx * gy, gy * gy) * 0.07901060453704994;
	gx = (-sample34.x + 8.0 * sample28.x - 8.0 * sample16.x + sample10.x) / 12.0;
	gy = (sample23.x - sample21.x) / 2.0;
	abd += float3(gx * gx, gx * gy, gy * gy) * 0.06153352068439959;
	gx = (sample31.x - sample19.x) / 2.0;
	gy = (sample26.x - sample24.x) / 2.0;
	abd += float3(gx * gx, gx * gy, gy * gy) * 0.04792235409415088;
	gx = (sample32.x - sample20.x) / 2.0;
	gy = (-sample28.x + 8.0 * sample27.x - 8.0 * sample25.x + sample24.x) / 12.0;
	abd += float3(gx * gx, gx * gy, gy * gy) * 0.06153352068439959;
	gx = (sample33.x - sample21.x) / 2.0;
	gy = (-sample29.x + 8.0 * sample28.x - 8.0 * sample26.x + sample25.x) / 12.0;
	abd += float3(gx * gx, gx * gy, gy * gy) * 0.06153352068439959;
	gx = (sample34.x - sample22.x) / 2.0;
	gy = (sample29.x - sample27.x) / 2.0;
	abd += float3(gx * gx, gx * gy, gy * gy) * 0.04792235409415088;
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
	float coord_y = ((angle * 4.0 + strength) * 3.0 + coherence) / 288.0;

	float3 res = 0;
	float4 w;
	w = ravu_zoom_lut3.Sample(sam1, float2(0.0, coord_y) + subpix);
	res += sample0 * w[0];
	res += sample1 * w[1];
	res += sample2 * w[2];
	res += sample3 * w[3];
	w = ravu_zoom_lut3.Sample(sam1, float2(0.2, coord_y) + subpix);
	res += sample4 * w[0];
	res += sample5 * w[1];
	res += sample6 * w[2];
	res += sample7 * w[3];
	w = ravu_zoom_lut3.Sample(sam1, float2(0.4, coord_y) + subpix);
	res += sample8 * w[0];
	res += sample9 * w[1];
	res += sample10 * w[2];
	res += sample11 * w[3];
	w = ravu_zoom_lut3.Sample(sam1, float2(0.6, coord_y) + subpix);
	res += sample12 * w[0];
	res += sample13 * w[1];
	res += sample14 * w[2];
	res += sample15 * w[3];
	w = ravu_zoom_lut3.Sample(sam1, float2(0.8, coord_y) + subpix);
	res += sample16 * w[0];
	res += sample17 * w[1];
	w = ravu_zoom_lut3.Sample(sam1, float2(0.0, coord_y) + subpix_inv);
	res += sample35 * w[0];
	res += sample34 * w[1];
	res += sample33 * w[2];
	res += sample32 * w[3];
	w = ravu_zoom_lut3.Sample(sam1, float2(0.2, coord_y) + subpix_inv);
	res += sample31 * w[0];
	res += sample30 * w[1];
	res += sample29 * w[2];
	res += sample28 * w[3];
	w = ravu_zoom_lut3.Sample(sam1, float2(0.4, coord_y) + subpix_inv);
	res += sample27 * w[0];
	res += sample26 * w[1];
	res += sample25 * w[2];
	res += sample24 * w[3];
	w = ravu_zoom_lut3.Sample(sam1, float2(0.6, coord_y) + subpix_inv);
	res += sample23 * w[0];
	res += sample22 * w[1];
	res += sample21 * w[2];
	res += sample20 * w[3];
	w = ravu_zoom_lut3.Sample(sam1, float2(0.8, coord_y) + subpix_inv);
	res += sample19 * w[0];
	res += sample18 * w[1];
	res = saturate(res);
	res = mul(_yuv2rgb, res - float3(0, 0.5, 0.5));

	return float4(res, 1);
}
