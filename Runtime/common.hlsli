#include "d2d1effecthelpers.hlsli"

#define PI 3.1415926535897932384626433832795
#define HALF_PI  1.5707963267948966192313216916398

#define ZEROS4 (float4(0,0,0,0))

#define min4(a, b, c, d) min(min(a, b), min(c, d))
#define max4(a, b, c, d) max(max(a, b), max(c, d))

#define min3(a, b, c) min(a, min(b, c))
#define max3(a, b, c) max(a, max(b, c))


#ifdef MAGPIE_USE_SAMPLE_INPUT

/*
* 包含边界检查的 SampleInput
* 欲使用下面的宏需要在包含此文件前定义 MAGPIE_USE_SAMPLE_INPUT，
* 然后在 main 函数的开头调用 InitMagpieSampleInput 或 InitMagpieSampleInputWithScale
*/

static float4 coord = 0;

#define SampleInputNoCheck(index, pos) (D2DSampleInput(index, pos).rgb)
#define SampleInputRGBANoCheck(index, pos) (D2DSampleInput(index, pos))
#define SampleInputOffNoCheck(index, pos) (SampleInputNoCheck(index, coord.xy + pos * coord.zw))
#define SampleInputRGBAOffNoCheck(index, pos) (SampleInputRGBANoCheck(index, coord.xy + pos * coord.zw))
#define SampleInputCur(index) (D2DSampleInput(index, coord.xy).rgb)
#define SampleInputRGBACur(index) (D2DSampleInput(index, coord.xy))

static float2 _maxCoord = 0;

// 使用 rg 而不是 xy
#define _checkLeft(x) (max(0, x))
#define _checkRight(x) (min(_maxCoord.r, x))
#define _checkTop(y) (max(0, y))
#define _checkBottom(y) (min(_maxCoord.g, y))

// 检查边界的 D2DSampleInput
#define SampleInputCheckLeft(index, x, y) (SampleInputNoCheck(index, float2(_checkLeft(x), y)))
#define SampleInputCheckRight(index, x, y) (SampleInputNoCheck(index, float2(_checkRight(x), y)))
#define SampleInputCheckTop(index, x, y) (SampleInputNoCheck(index, float2(x, _checkTop(y))))
#define SampleInputCheckBottom(index, x, y) (SampleInputNoCheck(index, float2(x, _checkBottom(y))))
#define SampleInputCheckLeftTop(index, x, y) (SampleInputNoCheck(index, float2(_checkLeft(x), _checkTop(y))))
#define SampleInputCheckLeftBottom(index, x, y) (SampleInputNoCheck(index, float2(_checkLeft(x), _checkBottom(y))))
#define SampleInputCheckRightTop(index, x, y) (SampleInputNoCheck(index, float2(_checkRight(x), _checkTop(y))))
#define SampleInputCheckRightBottom(index, x, y) (SampleInputNoCheck(index, float2(_checkRight(x), _checkBottom(y))))

// 检查边界的 D2DSampleInputAtOffset
// 使用 rg 而不是 xy
#define SampleInputOffCheckLeft(index, x, y) SampleInputCheckLeft(index, x * coord.z + coord.r, y * coord.w + coord.g)
#define SampleInputOffCheckRight(index, x, y) SampleInputCheckRight(index, x * coord.z + coord.r, y * coord.w + coord.g)
#define SampleInputOffCheckTop(index, x, y) SampleInputCheckTop(index, x * coord.z + coord.r, y * coord.w + coord.g)
#define SampleInputOffCheckBottom(index, x, y) SampleInputCheckBottom(index, x * coord.z + coord.r, y * coord.w + coord.g)
#define SampleInputOffCheckLeftTop(index, x, y) SampleInputCheckLeftTop(index, x * coord.z + coord.r, y * coord.w + coord.g)
#define SampleInputOffCheckLeftBottom(index, x, y) SampleInputCheckLeftBottom(index, x * coord.z + coord.r, y * coord.w + coord.g)
#define SampleInputOffCheckRightTop(index, x, y) SampleInputCheckRightTop(index, x * coord.z + coord.r, y * coord.w + coord.g)
#define SampleInputOffCheckRightBottom(index, x, y) SampleInputCheckRightBottom(index, x * coord.z + coord.r, y * coord.w + coord.g)

// 限制坐标在边界内
// n 为 offset
#define GetCheckedLeft(n) _checkLeft(coord.x - n * coord.z)
#define GetCheckedRight(n) _checkRight(coord.x + n * coord.z)
#define GetCheckedTop(n) _checkTop(coord.y - n * coord.w)
#define GetCheckedBottom(n) _checkBottom(coord.y + n * coord.w)


// 需要 main 函数的开头调用

void InitMagpieSampleInput() {
	coord = D2DGetInputCoordinate(0);
	_maxCoord = float2((srcSize.x - 1) * coord.z, (srcSize.y - 1) * coord.w);
}

void InitMagpieSampleInputWithScale(float2 scale) {
	coord = D2DGetInputCoordinate(0);
	coord.xy /= scale;	// 将 dest 坐标映射为 src 坐标
	_maxCoord = float2((srcSize.x - 1) * coord.z, (srcSize.y - 1) * coord.w);
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

#define RGB2YUV(rgb) (mul(_rgb2yuv, rgb) + float3(0, 0.5, 0.5))
#define YUV2RGB(y, u, v) (mul(_yuv2rgb, float3(y, u - 0.5, v - 0.5)))

#endif
