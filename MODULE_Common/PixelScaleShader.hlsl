// Pixel ²åÖµËã·¨


cbuffer constants : register(b0) {
	int2 srcSize : packoffset(c0.x);
	int scale : packoffset(c0.z);
};


#define MAGPIE_INPUT_COUNT 1
#include "common.hlsli"


D2D_PS_ENTRY(main) {
	InitMagpieSampleInputWithScale(float2(scale, scale));

	return SampleInput(0, (floor(Coord(0).xy / Coord(0).zw) + 0.5) * Coord(0).zw);
}
