#include "common.hlsli"

// 因为 d2d 会将着色器间传递的值限制到 [0, 1]，pass1 传递到 pass2 的值需要进行压缩
// 假设所有值在 0~32 之间
#define Compress(value) (value / 32)	// (atan(value) / PI + 0.5)
#define Uncompress(value) (value * 32)	// (tan((value - 0.5) * PI))
