cbuffer constants : register(b0) {
	int2 srcSize : packoffset(c0.x);
	int2 destSize : packoffset(c0.z);
};


#define MAGPIE_INPUT_COUNT 2
#define MAGPIE_USE_YUV
#include "common.hlsli"

#define WEIGHTS_TEXTURE_WIDTH 45
#define WEIGHTS_TEXTURE_HEIGHT 2592


float mod(float x, float y) {
	return x - y * floor(x / y);
}


float4 sampleWeightsTexture(float2 pos) {
	float2 xy = SampleInput(1, pos).xy;
	xy.x = uncompressLinear(xy.x, -0.35019588470458984, 0.35121241211891174);
	xy.y = uncompressLinear(xy.y, -0.36509251594543457, 0.3279324769973755);

	float2 zw = SampleInput(1, float2(pos.x + WEIGHTS_TEXTURE_WIDTH * Coord(1).z, pos.y)).xy;
	zw.x = uncompressLinear(zw.x, -0.309879332780838, 1.4822126626968384);
	zw.y = uncompressLinear(zw.y, -0.30084773898124695, 1.4810420274734497);

	return float4(xy, zw);
}

D2D_PS_ENTRY(main) {
	InitMagpieSampleInput();
	Coord(0).xy /= float2(destSize) / srcSize;

	float2 pos = Coord(0).xy / Coord(0).zw;
	float2 subpix = frac(pos - 0.5);
	pos -= subpix;
	subpix = lerp(float2(0.5, 0.5) / float2(9.0, 9.0), 1.0 - 0.5 / float2(9.0, 9.0), subpix);
	float2 subpix_inv = 1.0 - subpix;
	subpix /= float2(5.0, 288.0);
	subpix *= float2(WEIGHTS_TEXTURE_WIDTH, WEIGHTS_TEXTURE_HEIGHT) * Coord(1).zw;
	subpix_inv /= float2(5.0, 288.0);
	subpix_inv *= float2(WEIGHTS_TEXTURE_WIDTH, WEIGHTS_TEXTURE_HEIGHT) * Coord(1).zw;

	float3 sample0 = SampleInputChecked(0, (pos + float2(-2.0, -2.0)) * Coord(0).zw).xyz;
	float3 sample1 = SampleInputChecked(0, (pos + float2(-2.0, -1.0)) * Coord(0).zw).xyz;
	float3 sample2 = SampleInputChecked(0, (pos + float2(-2.0, 0.0)) * Coord(0).zw).xyz;
	float3 sample3 = SampleInputChecked(0, (pos + float2(-2.0, 1.0)) * Coord(0).zw).xyz;
	float3 sample4 = SampleInputChecked(0, (pos + float2(-2.0, 2.0)) * Coord(0).zw).xyz;
	float3 sample5 = SampleInputChecked(0, (pos + float2(-2.0, 3.0)) * Coord(0).zw).xyz;
	float3 sample6 = SampleInputChecked(0, (pos + float2(-1.0, -2.0)) * Coord(0).zw).xyz;
	float3 sample7 = SampleInputChecked(0, (pos + float2(-1.0, -1.0)) * Coord(0).zw).xyz;
	float3 sample8 = SampleInputChecked(0, (pos + float2(-1.0, 0.0)) * Coord(0).zw).xyz;
	float3 sample9 = SampleInputChecked(0, (pos + float2(-1.0, 1.0)) * Coord(0).zw).xyz;
	float3 sample10 = SampleInputChecked(0, (pos + float2(-1.0, 2.0)) * Coord(0).zw).xyz;
	float3 sample11 = SampleInputChecked(0, (pos + float2(-1.0, 3.0)) * Coord(0).zw).xyz;
	float3 sample12 = SampleInputChecked(0, (pos + float2(0.0, -2.0)) * Coord(0).zw).xyz;
	float3 sample13 = SampleInputChecked(0, (pos + float2(0.0, -1.0)) * Coord(0).zw).xyz;
	float3 sample14 = SampleInputChecked(0, pos * Coord(0).zw).xyz;
	float3 sample15 = SampleInputChecked(0, (pos + float2(0.0, 1.0)) * Coord(0).zw).xyz;
	float3 sample16 = SampleInputChecked(0, (pos + float2(0.0, 2.0)) * Coord(0).zw).xyz;
	float3 sample17 = SampleInputChecked(0, (pos + float2(0.0, 3.0)) * Coord(0).zw).xyz;
	float3 sample18 = SampleInputChecked(0, (pos + float2(1.0, -2.0)) * Coord(0).zw).xyz;
	float3 sample19 = SampleInputChecked(0, (pos + float2(1.0, -1.0)) * Coord(0).zw).xyz;
	float3 sample20 = SampleInputChecked(0, (pos + float2(1.0, 0.0)) * Coord(0).zw).xyz;
	float3 sample21 = SampleInputChecked(0, (pos + float2(1.0, 1.0)) * Coord(0).zw).xyz;
	float3 sample22 = SampleInputChecked(0, (pos + float2(1.0, 2.0)) * Coord(0).zw).xyz;
	float3 sample23 = SampleInputChecked(0, (pos + float2(1.0, 3.0)) * Coord(0).zw).xyz;
	float3 sample24 = SampleInputChecked(0, (pos + float2(2.0, -2.0)) * Coord(0).zw).xyz;
	float3 sample25 = SampleInputChecked(0, (pos + float2(2.0, -1.0)) * Coord(0).zw).xyz;
	float3 sample26 = SampleInputChecked(0, (pos + float2(2.0, 0.0)) * Coord(0).zw).xyz;
	float3 sample27 = SampleInputChecked(0, (pos + float2(2.0, 1.0)) * Coord(0).zw).xyz;
	float3 sample28 = SampleInputChecked(0, (pos + float2(2.0, 2.0)) * Coord(0).zw).xyz;
	float3 sample29 = SampleInputChecked(0, (pos + float2(2.0, 3.0)) * Coord(0).zw).xyz;
	float3 sample30 = SampleInputChecked(0, (pos + float2(3.0, -2.0)) * Coord(0).zw).xyz;
	float3 sample31 = SampleInputChecked(0, (pos + float2(3.0, -1.0)) * Coord(0).zw).xyz;
	float3 sample32 = SampleInputChecked(0, (pos + float2(3.0, 0.0)) * Coord(0).zw).xyz;
	float3 sample33 = SampleInputChecked(0, (pos + float2(3.0, 1.0)) * Coord(0).zw).xyz;
	float3 sample34 = SampleInputChecked(0, (pos + float2(3.0, 2.0)) * Coord(0).zw).xyz;
	float3 sample35 = SampleInputChecked(0, (pos + float2(3.0, 3.0)) * Coord(0).zw).xyz;

	float3 abd = ZEROS3;
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
	float theta = lerp(mod(atan2(L1 - a, b) + PI, PI), 0.0, abs(b) < 1.192092896e-7);
	float lambda = sqrtL1;
	float mu = lerp((sqrtL1 - sqrtL2) / (sqrtL1 + sqrtL2), 0.0, sqrtL1 + sqrtL2 < 1.192092896e-7);
	float angle = floor(theta * 24.0 / PI);
	float strength = lerp(lerp(0.0, 1.0, lambda >= 0.004), lerp(2.0, 3.0, lambda >= 0.05), lambda >= 0.016);
	float coherence = lerp(lerp(0.0, 1.0, mu >= 0.25), 2.0, mu >= 0.5);
	float coord_y = ((angle * 4.0 + strength) * 3.0 + coherence) / 288.0;
	coord_y *= WEIGHTS_TEXTURE_HEIGHT * Coord(1).w;

	float3 res = ZEROS3;
	float4 w;
	w = sampleWeightsTexture(float2(0, coord_y) + subpix);
	res += sample0 * w[0];
	res += sample1 * w[1];
	res += sample2 * w[2];
	res += sample3 * w[3];
	w = sampleWeightsTexture(float2(9 * Coord(1).z, coord_y) + subpix);
	res += sample4 * w[0];
	res += sample5 * w[1];
	res += sample6 * w[2];
	res += sample7 * w[3];
	w = sampleWeightsTexture(float2(18 * Coord(1).z, coord_y) + subpix);
	res += sample8 * w[0];
	res += sample9 * w[1];
	res += sample10 * w[2];
	res += sample11 * w[3];
	w = sampleWeightsTexture(float2(27 * Coord(1).z, coord_y) + subpix);
	res += sample12 * w[0];
	res += sample13 * w[1];
	res += sample14 * w[2];
	res += sample15 * w[3];
	w = sampleWeightsTexture(float2(36 * Coord(1).z, coord_y) + subpix);
	res += sample16 * w[0];
	res += sample17 * w[1];
	w = sampleWeightsTexture(float2(0, coord_y) + subpix_inv);
	res += sample35 * w[0];
	res += sample34 * w[1];
	res += sample33 * w[2];
	res += sample32 * w[3];
	w = sampleWeightsTexture(float2(9 * Coord(1).z, coord_y) + subpix_inv);
	res += sample31 * w[0];
	res += sample30 * w[1];
	res += sample29 * w[2];
	res += sample28 * w[3];
	w = sampleWeightsTexture(float2(18 * Coord(1).z, coord_y) + subpix_inv);
	res += sample27 * w[0];
	res += sample26 * w[1];
	res += sample25 * w[2];
	res += sample24 * w[3];
	w = sampleWeightsTexture(float2(27 * Coord(1).z, coord_y) + subpix_inv);
	res += sample23 * w[0];
	res += sample22 * w[1];
	res += sample21 * w[2];
	res += sample20 * w[3];
	w = sampleWeightsTexture(float2(36 * Coord(1).z, coord_y) + subpix_inv);
	res += sample19 * w[0];
	res += sample18 * w[1];
	res = clamp(res, 0.0, 1.0);

	float3 origin = SampleInputCur(0).xyz;
	if (abs(res.x - origin.x) < 0.03) {
		res = origin;
	}
	return float4(YUV2RGB(res), 1);
}

