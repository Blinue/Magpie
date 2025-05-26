// 移植自 https://github.com/NVIDIAGameWorks/NVIDIAImageScaling/blob/main/NIS/NIS_Scaler.h

//!MAGPIE EFFECT
//!VERSION 4
//!CAPABILITY FP16

#include "../StubDefs.hlsli"

//!PARAMETER
//!LABEL Sharpness
//!DEFAULT 0.5
//!MIN 0
//!MAX 1
//!STEP 0.01
float sharpness;

//!TEXTURE
Texture2D INPUT;

//!TEXTURE
//!WIDTH INPUT_WIDTH
//!HEIGHT INPUT_HEIGHT
Texture2D OUTPUT;

//!SAMPLER
//!FILTER LINEAR
SamplerState samplerLinearClamp;


//!PASS 1
//!IN INPUT
//!OUT OUTPUT
//!BLOCK_SIZE 32, 32
//!NUM_THREADS 256

#ifdef MP_DEBUG
#pragma warning(disable: 4000)	// X4000: use of potentially uninitialized variable (GetEdgeMap)
#pragma warning(disable: 4714)	// X4714: sum of temp registers and indexable temp registers times 256 threads exceeds the recommended total 16384.  Performance may be reduced
#endif

#define NIS_SCALER 0
#define NIS_HLSL 1

#ifdef MP_FP16
#define NIS_USE_HALF_PRECISION 1
#else
#define NIS_USE_HALF_PRECISION 0
#endif

static const float kDetectRatio = 2.0f * 1127.f / 1024.f;
static const float kDetectThres = 64.0f / 1024.0f;
static const float kEps = 1.0f / 255.0f;
static const float kMinContrastRatio = 2.0f;
static const float kMaxContrastRatio = 10.0f;
static const float kRatioNorm = 1.0f / (kMaxContrastRatio - kMinContrastRatio);
static const float kContrastBoost = 1.0f;
static const float kSharpStartY = 0.45f;
static const float kSharpEndY = 0.9f;
static const float kSharpScaleY = 1.0f / (kSharpEndY - kSharpStartY);
static const float sharpen_slider = sharpness - 0.5f;
static const float MinScale = (sharpen_slider >= 0.0f) ? 1.25f : 1.0f;
static const float MaxScale = (sharpen_slider >= 0.0f) ? 1.25f : 1.75f;
static const float kSharpStrengthMin = max(0.0f, 0.4f + sharpen_slider * MinScale * 1.2f);
static const float kSharpStrengthMax = 1.6f + sharpen_slider * MaxScale * 1.8f;
static const float kSharpStrengthScale = kSharpStrengthMax - kSharpStrengthMin;
static const float LimitScale = (sharpen_slider >= 0.0f) ? 1.25f : 1.0f;
static const float kSharpLimitMin = max(0.1f, 0.14f + sharpen_slider * LimitScale * 0.32f);
static const float kSharpLimitMax = 0.5f + sharpen_slider * LimitScale * 0.6f;
static const float kSharpLimitScale = kSharpLimitMax - kSharpLimitMin;

#define NIS_BLOCK_WIDTH MP_BLOCK_WIDTH
#define NIS_BLOCK_HEIGHT MP_BLOCK_HEIGHT
#define NIS_THREAD_GROUP_SIZE MP_NUM_THREADS_X

#define in_texture INPUT
#define out_texture OUTPUT

static const float kSrcNormX = GetInputPt().x;
static const float kSrcNormY = GetInputPt().y;

#include "NIS_Scaler.hlsli"

void Pass1(uint2 blockStart, uint3 threadId) {
	NVSharpen(blockStart / uint2(MP_BLOCK_WIDTH, MP_BLOCK_HEIGHT), threadId.x);
}
