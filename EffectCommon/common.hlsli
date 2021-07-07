#define PI 3.1415926535897932384626433832795
#define HALF_PI  1.5707963267948966192313216916398

#define ZEROS3 (float3(0,0,0))
#define ZEROS4 (float4(0,0,0,0))

#define min4(a, b, c, d) min(min(a, b), min(c, d))
#define max4(a, b, c, d) max(max(a, b), max(c, d))

#define min3(a, b, c) min(a, min(b, c))
#define max3(a, b, c) max(a, max(b, c))

#define max9(a, b, c, d, e, f, g, h, i) max3(max4(a, b, c, d), max4(e, f, g, h), i)
#define min9(a, b, c, d, e, f, g, h, i) min3(min4(a, b, c, d), min4(e, f, g, h), i)

// 纹理会自动将值切割到 0~1，为了使值在着色器之间传递需要进行压缩

#define compressLinear(x, min, max) (((x) - min) / (max - min))
#define uncompressLinear(x, min, max) ((x) * (max - min) + min)

#define compressTan(x) (atan(x) / PI + 0.5)
#define uncompressTan(x) (tan(((x) - 0.5) * PI))


#if MAGPIE_INPUT_COUNT >= 1

/*
* 包含边界检查的 SampleInput
* 欲使用下面的宏需要在包含此文件前定义 MAGPIE_INPUT_COUNT，
* 然后在 main 函数的开头调用 InitMagpieSampleInput 或 InitMagpieSampleInputWithScale
*/

#define D2D_INPUT0_COMPLEX
#ifndef MAGPIE_NO_CHECK
static float2 maxCoord0;
#endif

#if MAGPIE_INPUT_COUNT >= 2
#define D2D_INPUT1_COMPLEX
#ifndef MAGPIE_NO_CHECK
static float2 maxCoord1;
#endif

#if MAGPIE_INPUT_COUNT >= 3
#define D2D_INPUT2_COMPLEX
#ifndef MAGPIE_NO_CHECK
static float2 maxCoord2;
#endif

#if MAGPIE_INPUT_COUNT >= 4
#define D2D_INPUT3_COMPLEX
#ifndef MAGPIE_NO_CHECK
static float2 maxCoord3;
#endif

#if MAGPIE_INPUT_COUNT >= 5
#define D2D_INPUT4_COMPLEX
#ifndef MAGPIE_NO_CHECK
static float2 maxCoord4;
#endif

#if MAGPIE_INPUT_COUNT >= 6
#define D2D_INPUT5_COMPLEX
#ifndef MAGPIE_NO_CHECK
static float2 maxCoord5;
#endif

#if MAGPIE_INPUT_COUNT >= 7
#define D2D_INPUT6_COMPLEX
#ifndef MAGPIE_NO_CHECK
static float2 maxCoord6;
#endif

#if MAGPIE_INPUT_COUNT >= 8
#define D2D_INPUT7_COMPLEX
#ifndef MAGPIE_NO_CHECK
static float2 maxCoord7;
#endif

#if MAGPIE_INPUT_COUNT >= 9
#error Too many inputs.
#endif

#endif
#endif
#endif
#endif
#endif
#endif
#endif

#define D2D_INPUT_COUNT MAGPIE_INPUT_COUNT
#include "d2d1effecthelpers.hlsli"

#define Coord(index) (D2DGetInputCoordinate(index))

#define SampleInput(index, pos) (D2DSampleInput(index, (pos)))
#define SampleInputCur(index) (SampleInput(index, Coord(index).xy))
#define SampleInputOff(index, pos) (SampleInput(index, Coord(index).xy + (pos) * Coord(index).zw))

// 在循环中使用
#define SampleInputLod(index, pos) (InputTexture##index.SampleLevel(InputSampler##index, (pos), 0))

#ifndef MAGPIE_NO_CHECK
#define GetCheckedPos(index, pos) (clamp((pos), 0, maxCoord##index))
#define GetCheckedOffPos(index, pos) (GetCheckedPos(index, Coord(index).xy + (pos) * Coord(index).zw))

#define SampleInputChecked(index, pos) (SampleInput(index, GetCheckedPos(index, (pos))))
#define SampleInputOffChecked(index, pos) (SampleInput(index, GetCheckedOffPos(index, (pos))))
#endif


// 需要 main 函数的开头调用

void InitMagpieSampleInput() {
#ifndef MAGPIE_NO_CHECK
	maxCoord0 = float2((srcSize.x - 1) * Coord(0).z, (srcSize.y - 1) * Coord(0).w);
#if MAGPIE_INPUT_COUNT >= 2
	maxCoord1 = float2((srcSize.x - 1) * Coord(1).z, (srcSize.y - 1) * Coord(1).w);
#if MAGPIE_INPUT_COUNT >= 3
	maxCoord2 = float2((srcSize.x - 1) * Coord(2).z, (srcSize.y - 1) * Coord(2).w);
#if MAGPIE_INPUT_COUNT >= 4
	maxCoord3 = float2((srcSize.x - 1) * Coord(3).z, (srcSize.y - 1) * Coord(3).w);
#if MAGPIE_INPUT_COUNT >= 5
	maxCoord4 = float2((srcSize.x - 1) * Coord(4).z, (srcSize.y - 1) * Coord(4).w);
#if MAGPIE_INPUT_COUNT >= 6
	maxCoord5 = float2((srcSize.x - 1) * Coord(5).z, (srcSize.y - 1) * Coord(5).w);
#if MAGPIE_INPUT_COUNT >= 7
	maxCoord6 = float2((srcSize.x - 1) * Coord(6).z, (srcSize.y - 1) * Coord(6).w);
#if MAGPIE_INPUT_COUNT >= 8
	maxCoord7 = float2((srcSize.x - 1) * Coord(7).z, (srcSize.y - 1) * Coord(7).w);
#endif
#endif
#endif
#endif
#endif
#endif
#endif
#endif
}

void InitMagpieSampleInputWithScale(float2 scale) {
	InitMagpieSampleInput();

	// 将 dest 坐标映射为 src 坐标
	Coord(0).xy /= scale;
}


#endif


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
