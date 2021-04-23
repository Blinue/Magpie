#include "common.hlsli"

// 纹理会自动将值切割到 0~1，为了使值在着色器之间传递需要进行压缩
// 假设所有值在 -1.5~1.5 之间，深度学习的中间值一般很小，所以几乎没有截断损失
// 但精度损失不可避免，最终会产生噪声
#define Compress(value) ((value + 1.5) / 3)	// (atan(value) / PI + 0.5);

#define Uncompress(value) (value * 3 - 1.5)	// tan((value - 0.5) * PI)

// Anime4K 中使用的是亮度分量
#define GetLuma(rgb) (0.299 * rgb.r + 0.587 * rgb.g + 0.114 * rgb.b)
