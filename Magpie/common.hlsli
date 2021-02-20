#define PI 3.1415926535897932384626433832795
#define HALF_PI  1.5707963267948966192313216916398

#define ZEROS4 float4(0,0,0,0)

#define min4(a, b, c, d) min(min(a, b), min(c, d))
#define max4(a, b, c, d) max(max(a, b), max(c, d))

#define min3(a, b, c) min(a, min(b, c))
#define max3(a, b, c) max(a, max(b, c))


float3 RGB2YUV(float3 rgb) {
	float3x3 rgb2yuv = {
		0.299, 0.587, 0.114,
		-0.169, -0.331, 0.5,
		0.5, -0.419, -0.081
	};

	return mul(rgb2yuv, rgb) + float3(0, 0.5, 0.5);
}

float3 YUV2RGB(float3 yuv) {
	float3x3 yuv2rgb = {
		1, -0.00093, 1.401687,
		1, -0.3437, -0.71417,
		1, 1.77216, 0.00099
	};

	return mul(yuv2rgb, float3(yuv.x, yuv.y - 0.5, yuv.z - 0.5));
}

