
cbuffer constants : register(b0) {
	int2 srcSize : packoffset(c0);
};


#define MAGPIE_INPUT_COUNT 1
#define MAGPIE_USE_YUV
#include "Anime4K.hlsli"


D2D_PS_ENTRY(main) {
	return float4(RGB2YUV(SampleInputCur(0).rgb), 1);
}
