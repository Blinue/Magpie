#define MAGPIE_INPUT_COUNT 1
#define MAGPIE_NO_CHECK
#include "common.hlsli"

#define WEIGHTS_TEXTURE_WIDTH 45
#define WEIGHTS_TEXTURE_HEIGHT 2592



D2D_PS_ENTRY(main) {
	float2 pos = (floor(Coord(0).xy / Coord(0).zw) + 0.5) * Coord(0).zw;

	float2 x = SampleInput(0, pos).xy;
	float2 y = SampleInput(0, float2(pos.x + WEIGHTS_TEXTURE_WIDTH * Coord(0).z, pos.y)).xy;
	float2 z = SampleInput(0, float2(pos.x + 2 * WEIGHTS_TEXTURE_WIDTH * Coord(0).z, pos.y)).xy;
	float2 w = SampleInput(0, float2(pos.x + 3 * WEIGHTS_TEXTURE_WIDTH * Coord(0).z, pos.y)).xy;

	float4 r = {
		(x.x * 255 * 256 + x.y * 255) / 65535,
		(y.x * 255 * 256 + y.y * 255) / 65535,
		(z.x * 255 * 256 + z.y * 255) / 65535,
		(w.x * 255 * 256 + w.y * 255) / 65535
	};

	return r;
}

