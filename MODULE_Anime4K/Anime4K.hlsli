#include "common.hlsli"

 
// 假设值在-3~-3。因为精度损失，最终会产生噪声
#define Compress(value) compressLinear(value, -3, 3)
#define Uncompress(value) uncompressLinear(value, -3, 3)
