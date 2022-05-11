MagpieFX 语法

``` hlsl
//!MAGPIE EFFECT
//!VERSION 2
//!OUTPUT_WIDTH INPUT_WIDTH * 2
//!OUTPUT_HEIGHT INPUT_HEIGHT * 2
// 若要使用 GetFrameCount 或 GetCursorPos 需指定 USE_DYNAMIC
//!USE_DYNAMIC

// 如果不定义 OUTPUT_WIDTH 和 OUTPUT_HEIGHT 表示此 Effect 可以接受任何大小的输出
// 计算纹理尺寸时可以使用一些预定义常量
// INPUT_WIDTH
// INPUT_HEIGHT
// OUTPUT_WIDTH
// OUTPUT_HEIGHT


// 参数定义
// 含有 VALUE 关键字的变量会填充该表达式的值
// 否则为在运行时可以改变的参数
//!PARAMETER
//!DEFAULT 0
// LABEL 可选，暂时不使用
//!LABEL 锐度
float sharpness;


// 纹理定义
// INPUT 是特殊关键字
// INPUT 不能作为 Pass 的输出
// 定义 INPUT 是可选的，但为了保持语义的完整性，建议显式定义

//!TEXTURE
Texture2D INPUT;

// 支持的纹理格式：
// R32G32B32A32_FLOAT
// R16G16B16A16_FLOAT
// R16G16B16A16_UNORM
// R16G16B16A16_SNORM
// R32G32_FLOAT
// R10G10B10A2_UNORM
// R11G11B10_FLOAT
// R8G8B8A8_UNORM
// R8G8B8A8_SNORM
// R16G16_FLOAT
// R16G16_UNORM
// R16G16_SNORM
// R32_FLOAT
// R8G8_UNORM
// R8G8_SNORM
// R16_FLOAT
// R16_UNORM
// R16_SNORM
// R8_UNORM
// R8_SNORM
// 根据纹理格式的不同，在通道中该纹理的定义也是不同的。如当纹理格式为 R8G8_UNORM，
// 作为通道的输入时定义是 Texture2D<float2>，作为输出时定义是 RWTexture2D<unorm float2>

//!TEXTURE
//!WIDTH INPUT_WIDTH + 100
//!HEIGHT INPUT_HEIGHT + 100
//!FORMAT B8G8R8A8_UNORM
Texture2D tex1;

// 采样器定义
// FILTER 必需，ADDRESS 可选
// ADDRESS 默认值为 CLAMP

//!SAMPLER
//!FILTER LINEAR
//!ADDRESS CLAMP
SamplerState sam1;

//!SAMPLER
//!FILTER POINT
//!ADDRESS WRAP
SamplerState sam2;

// 所有通道通用的部分

//!COMMON
#define PI 3.14159265359

// 通道定义

//!PASS 1
// 描述此通道以便于调试
//!DESC First Pass
// 可选 PS 风格，依然用 CS 实现
// STYLE 默认为 CS
//!STYLE PS
//!IN INPUT
// 支持多渲染目标，最多 8 个
//!OUT tex1

float func1() {
}

// Main 为通道入口点
float4 Main(float2 pos) {
    return float4(1, 1, 1, 1);
}

// 没有 OUT 表示此通道为 Effect 的输出

//!PASS 2
//!IN INPUT, tex1
// BLOCK_SIZE 指定一次 dispatch 处理多大的区域
// 可以只有一维，即同时指定长和高
//!BLOCK_SIZE 16, 16
// NUM_THREADS 指定一次 dispatch 有多少并行线程
// 可以少于三维，缺少的维数默认为 1
//!NUM_THREADS 64, 1, 1

void Pass2(uint2 blockStart, uint3 threadId) {
    // 向 OUPUT 写入的同时处理光标渲染
    // 只在最后一个通道中可用
    WriteToOutput(blockStart, float3(1,1,1));
}
```

### 内置函数

**void WriteToOutput(uint2 pos, float3 color)**：只在最后一个通道（Pass）中可用，用于将结果写入到输出纹理。

**bool CheckViewport(uint2 pos)**：只在最后一个通道中可用，检查输出坐标是否位于视口内。

**uint2 GetInputSize()**：获取输入纹理尺寸。

**float2 GetInputPt()**：获取输入纹理每个像素的尺寸。

**uint2 GetOutputSize()**：获取输出纹理尺寸。

**float2 GetOutputPt()**：获取输出纹理每个像素的尺寸。

**float2 GetScale()**：获取输出纹理相对于输入纹理的缩放。

**uint GetFrameCount()**：获取当前总计帧数。使用此函数时必须指定 USE_DYNAMIC。

**uint2 GetCursorPos()**：获取当前光标位置。使用此函数时必须指定 USE_DYNAMIC。

**uint2 Rmp8x8(uint id)**：将 0~63 的值以 swizzle 顺序映射到 8x8 的正方形内的坐标，用以提高纹理缓存的命中率。


### 内置宏：

**MP_BLOCK_WIDTH、MP_BLOCK_HEIGHT**：当前通道处理的块的大小（由 //!BLOCK_SIZE 指定）

**MP_NUM_THREADS_X、MP_NUM_THREADS_Y**：当前通道每个线程组的线程数（由 //!NUM_THREADS 指定）

**MP_PS_STYLE**：当前通道是否是像素着色器样式（由 //!STYLE 指定）

**MP_INLINE_PARAMS**：当前通道的参数是否为静态常量（由用户通过 inlineParams 参数指定）

**MP_DEBUG**：当前是否为调试模式（调试模式下编译的着色器不进行优化且含有调试信息）

**MP_LAST_PASS**：当前通道是否是当前效果的最后一个通道

**MP_LAST_EFFECT**：当前效果是否是当前缩放模式的最后一个效果（最后一个效果要处理视口和光标渲染）

**MP_FP16**：当前是否使用半精度浮点数（由用户通过 fp16 参数指定）

**MF、MF1、MF2、...、MF4x4**：遵守 fp16 参数的浮点数类型。当未指定 fp16，它们为 float... 的别名，否则为 min16float... 的别名


### 多渲染目标（MRT）

PS 风格下 OUT 指令可指定多个输出（DirectX 限制最多 8 个）：
``` hlsl
//!OUT tex1, tex2
```

这时通道入口有不同的签名：
``` hlsl
void Main(float2 pos, out float4 target1, out float4 target2);
```

### 从文件加载纹理

TEXTURE 指令可从文件加载纹理

``` hlsl
//!TEXTURE
//!SOURCE test.bmp
Texture2D testTex;
```

支持的格式有 bmp，png，jpg 等常见图像格式以及 DDS 文件。纹理尺寸与源图像尺寸相同。可选使用 FORMAT，指定后可以帮助解析器生成正确的定义，不指定始终假设是 float4 类型。

从文件加载的纹理不能作为通道的输出。
