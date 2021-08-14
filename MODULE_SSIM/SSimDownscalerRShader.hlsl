// SSimDownscaler calc R

cbuffer constants : register(b0) {
	int2 srcSize : packoffset(c0.x);   // srcSize
};


#define MAGPIE_INPUT_COUNT 3    // POSTKERNEL, L2_2, Mean
#include "common.hlsli"


#define locality    8.0


#define Kernel(x)   pow(1.0 / locality, abs(x))
#define taps        3.0
#define maxtaps     taps



float2x3 ScaleH(float2 pos, float curX) {
	// Calculate bounds
	float low = floor(-0.5 * maxtaps);
	float high = floor(0.5 * maxtaps);

	float W = 0.0;
	float2x3 avg = 0;

	for (float k = 0.0; k < maxtaps; k++) {
		float rel = k + low + 1.0;
		pos[0] = curX + rel;
		
		float w = Kernel(rel);

		float3 s1 = SampleInputChecked(0, pos * Coord(0).zw).rgb;
		s1 *= s1;
		avg += w * float2x3(s1, uncompressLinear(SampleInputChecked(1, pos * Coord(1).zw).rgb, -0.5, 1.5));
		W += w;
	}
	avg /= W;

	return avg;
}

D2D_PS_ENTRY(main) {
	InitMagpieSampleInput();
	float2 cur = Coord(0).xy / Coord(0).zw;

	// Calculate bounds
	float low = floor(-0.5 * maxtaps);
	float high = floor(0.5 * maxtaps);

	float W = 0.0;
	float2x3 avg = 0;
	float2 pos = cur;

	for (float k = 0.0; k < maxtaps; k++) {
		float rel = k + low + 1.0;
		pos.y = cur.y + rel;
		
		float w = Kernel(rel);

		avg += w * ScaleH(pos, cur.x);
		W += w;
	}
	avg /= W;
	
	float3 cur2 = SampleInputCur(2).rgb;
	cur2 *= cur2;
	float3 Sl = abs(avg[0] - cur2);
	float3 Sh = abs(avg[1] - cur2);
	
	// 受限于精度，这里增大噪声阈值
	float3 r = lerp(0.5f, 1.0 / (1.0 + sqrt(Sh / Sl)), 1 - step(Sl, 8e-4));
	return float4(r, 1);
}
