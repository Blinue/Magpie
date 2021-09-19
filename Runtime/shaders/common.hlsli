#define PI 3.1415926535897932384626433832795
#define HALF_PI  1.5707963267948966192313216916398

#define ZEROS3 (float3(0,0,0))
#define ZEROS4 (float4(0,0,0,0))

#define min3(a, b, c) min(a, min(b, c))
#define max3(a, b, c) max(a, max(b, c))

#define min4(a, b, c, d) min(min(a, b), min(c, d))
#define max4(a, b, c, d) max(max(a, b), max(c, d))

#define max9(a, b, c, d, e, f, g, h, i) max3(max4(a, b, c, d), max4(e, f, g, h), i)
#define min9(a, b, c, d, e, f, g, h, i) min3(min4(a, b, c, d), min4(e, f, g, h), i)

// 纹理会自动将值切割到 0~1，为了使值在着色器之间传递需要进行压缩

#define compressLinear(x, min, max) (((x) - min) / (max - min))
#define uncompressLinear(x, min, max) ((x) * (max - min) + min)

#define compressTan(x) (atan(x) / PI + 0.5)
#define uncompressTan(x) (tan(((x) - 0.5) * PI))


#ifdef MAGPIE_USE_YUV

/*
* RGB 和 YUV 互转
* 需在包含此文件前定义 MAGPIE_USE_YUV
*/

const static float3x3 _rgb2yuv = {
		0.299, 0.587, 0.114,
		-0.169, -0.331, 0.5,
		0.5, -0.419, -0.081
};
const static float3x3 _yuv2rgb = {
		1, -0.00093, 1.401687,
		1, -0.3437, -0.71417,
		1, 1.77216, 0.00099
};

#define RGB2YUV(rgb) (mul(_rgb2yuv, (rgb)) + float3(0, 0.5, 0.5))
#define YUV2RGB(yuv) (mul(_yuv2rgb, (yuv) - float3(0, 0.5, 0.5)))

#endif
