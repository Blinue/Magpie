// Anime4K Deblur 版本中去模糊所用的核
// 移植自 https://github.com/bloc97/Anime4K/blob/master/glsl/Upscale%2BDeblur/Anime4K_Upscale_CNN_M_x2_Deblur.glsl
// 考虑到 transform 间传递纹理造成的性能损失，将 Kernel(X) 和 Kernel(Y) 合并
// 
// Anime4K-v3.1-Upscale(x2)+Deblur-CNN(M)-Kernel(X/Y)


cbuffer constants : register(b0) {
	int2 srcSize : packoffset(c0);
};

#define D2D_INPUT_COUNT 1
#define D2D_INPUT0_COMPLEX
#define MAGPIE_USE_SAMPLE_INPUT
#include "Anime4K.hlsli"


#define max9(a, b, c, d, e, f, g, h, i) max3(max4(a, b, c, d), max4(e, f, g, h), i)
#define min9(a, b, c, d, e, f, g, h, i) min3(min4(a, b, c, d), min4(e, f, g, h), i)


D2D_PS_ENTRY(main) {
	InitMagpieSampleInput();

	float left1X = GetCheckedLeft(1);
	float right1X = GetCheckedRight(1);
	float top1Y = GetCheckedTop(1);
	float bottom1Y = GetCheckedBottom(1);

	// [ a, d, g ]
	// [ b, e, h ]
	// [ c, f, i ]
	float a = GetLuma(SampleInputNoCheck(0, float2(left1X, top1Y)));
	float b = GetLuma(SampleInputNoCheck(0, float2(left1X, coord.y)));
	float c = GetLuma(SampleInputNoCheck(0, float2(left1X, bottom1Y)));
	float d = GetLuma(SampleInputNoCheck(0, float2(coord.x, top1Y)));
	float e = GetLuma(SampleInputCur(0));
	float f = GetLuma(SampleInputNoCheck(0, float2(coord.x, bottom1Y)));
	float g = GetLuma(SampleInputNoCheck(0, float2(right1X, top1Y)));
	float h = GetLuma(SampleInputNoCheck(0, float2(right1X, coord.y)));
	float i = GetLuma(SampleInputNoCheck(0, float2(right1X, bottom1Y)));

	return float4(min9(a, b, c, d, e, f, g, h, i), max9(a, b, c, d, e, f, g, h, i), 0, 0);
}
