#include "common.hlsli"

// 纹理会自动将值切割到 0~1，为了使值在着色器之间传递需要进行压缩
// 因为精度损失，最终会产生噪声
#define Compress(value) (((value) + 3) / 6)
#define Uncompress(value) ((value) * 6 - 3)

// 有时值在 -1~1 之间，使用下面的压缩算法减少精度损失
#define Compress1(value) (((value) + 1) / 2)
#define Uncompress1(value) ((value) * 2 - 1)

// 无范围限制的压缩，但速度较慢
#define Compress2(value) (atan(value) / PI + 0.5)
#define Uncompress2(value) tan(((value) - 0.5) * PI)

#define Compress3(value) (((value) + 10) / 20)
#define Uncompress3(value) ((value) * 20 - 10)

// Anime4K 中使用的是亮度分量
#define GetLuma(rgb) (0.299 * (rgb).r + 0.587 * (rgb).g + 0.114 * (rgb).b)
