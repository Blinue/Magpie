#include "common.hlsli"

#define Compress(value) (value / 32)//(atan(value) / PI + 0.5)

#define Uncompress(value) (value * 32)//(tan((value - 0.5) * PI))

