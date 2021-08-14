// SSSR varL

cbuffer constants : register(b0) {
	int2 srcSize : packoffset(c0.x);
	float2 scale : packoffset(c0.z);
};


#define MAGPIE_INPUT_COUNT 2    // PREKERNEL，（第2个输入不使用）
#include "common.hlsli"



#define spread      1.0 / 1000.0

#define sqr(x)      pow(x, 2.0)
#define GetL(x,y)   SampleInputOffChecked(0, float2(x, y)).rgb

#define Luma(rgb)   ( dot(rgb*rgb, float3(0.2126, 0.7152, 0.0722)) )



D2D_PS_ENTRY(main) {
	InitMagpieSampleInputWithScale(scale);

	int X, Y;
	float3 meanL = 0;
	for (X = -1; X <= 1; ++X) {
		for (Y = -1; Y <= 1; ++Y) {
			meanL += GetL(X, Y) * pow(spread, sqr(float(X)) + sqr(float(Y)));
		}
	}
	meanL /= (1.0 + 4.0 * spread + 4.0 * spread * spread);

	float varL = 0.0;
	for (X = -1; X <= 1; X++) {
		for (Y = -1; Y <= 1; Y++) {
			varL += Luma(abs(GetL(X, Y) - meanL)) * pow(spread, sqr(float(X)) + sqr(float(Y)));
		}
	}
	varL /= (spread + 4.0 * spread + 4.0 * spread * spread);
	
	varL *= 8;
	float x = floor(varL * 10) / 10;
	float y = ((varL - x) * 100) / 10;
	return float4(x, y, 0, 1);
}
