// 绘制单色光标

cbuffer constants : register(b0) {
	int4 cursorRect : packoffset(c0);
};


#define MAGPIE_INPUT_COUNT 2
#define MAGPIE_NO_CHECK
#include "common.hlsli"



D2D_PS_ENTRY(main) {
	InitMagpieSampleInput();
	
	int2 pos = floor(Coord(0).xy / Coord(0).zw);
	if (pos.x < cursorRect.x || pos.x >= cursorRect.z || pos.y < cursorRect.y || pos.y >= cursorRect.w) {
		return SampleInputCur(0);
	} else {
		float2 pos1 = (pos - cursorRect.xy + 0.5) * Coord(1).zw;
		float andMask = SampleInput(1, pos1).x;
		float xorMask = SampleInput(1, float2(pos1.x, pos1.y + (cursorRect.w - cursorRect.y) * Coord(1).w)).x;

		if (andMask > 0.5) {
			if (xorMask > 0.5) {
				return float4(1 - SampleInputCur(0).xyz, 1);
			} else {
				return SampleInputCur(0);
			}
		} else {
			if (xorMask > 0.5) {
				return float4(1, 1, 1, 1);
			} else {
				return float4(0, 0, 0, 1);
			}
		}
	}
}
