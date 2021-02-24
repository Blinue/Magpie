#include "common.hlsli"

// 纹理会自动将值切割到 0~1，为了使值在着色器之间传递需要进行压缩
// 假设所有值在 -2~2 之间，深度学习的中间值一般很小，所以几乎没有损失
// 使用 tan 函数压缩会因为精度损失产生噪声
#define Compress(value) ((value + 2) / 4)	// (atan(value) / PI + 0.5);

#define Uncompress(value) (value * 4 - 2)	// tan((value - 0.5) * PI)

// Anime4K 中使用的是亮度分量
#define GetLuma(rgb) (0.299 * rgb.r + 0.587 * rgb.g + 0.114 * rgb.b)
