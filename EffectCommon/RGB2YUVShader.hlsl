#define MAGPIE_INPUT_COUNT 1
#define MAGPIE_NO_CHECK
#define MAGPIE_USE_YUV
#include "common.hlsli"


D2D_PS_ENTRY(main) {
	return float4(RGB2YUV(SampleInputCur(0).rgb), 1);
}
