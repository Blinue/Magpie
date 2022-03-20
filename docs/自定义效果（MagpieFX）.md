MagpieFX 语法

``` hlsl
//!MAGPIE EFFECT
//!VERSION 2
//!OUTPUT_WIDTH INPUT_WIDTH * 2
//!OUTPUT_HEIGHT INPUT_HEIGHT * 2

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
// INPUT 可以作为 Pass 的输出
// 定义 INPUT 是可选的，但为了保持语义的完整性，建议显式定义
// 支持的纹理格式：
// R8_UNORM
// R16_UNORM
// R16_FLOAT
// R8G8_UNORM
// B5G6R5_UNORM
// R16G16_UNORM
// R16G16_FLOAT
// R8G8B8A8_UNORM
// B8G8R8A8_UNORM
// R10G10B10A2_UNORM
// R32_FLOAT
// R11G11B10_FLOAT
// R32G32_FLOAT
// R16G16B16A16_UNORM
// R16G16B16A16_FLOAT
// R32G32B32A32_FLOAT

//!TEXTURE
// 无需定义 FORMAT，INPUT 的格式始终是 B8G8R8A8_UNORM
Texture2D INPUT;

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
//!STYLE CS
//!IN INPUT, tex1
//!BLOCK_SIZE 16, 16
//!NUM_THREADS 64, 1, 1

void Main(uint2 blockStart, uint3 threadId) {
    // 向 OUPUT 写入的同时处理视口和光标渲染
    // 只在最后一个通道中可用，且必须使用此函数写入到输出纹理
    WriteToOutput(blockStart, float3(1,1,1));
}
```

### 内置函数

**void WriteToOutput(uint2 pos, float3 color)**：只在最后一个通道（Pass）中可用，用于将结果写入到输出纹理。

**uint2 GetInputSize()**：获取输入纹理尺寸。

**float2 GetInputPt()**：获取输入纹理每个像素的尺寸。

**uint2 GetOutputSize()**：获取输出纹理尺寸。

**float2 GetOutputPt()**：获取输出纹理每个像素的尺寸。

**float2 GetScale()**：获取输出纹理相对于输入纹理的缩放。

**uint GetFrameCount()**：获取当前总计帧数。

**uint2 GetCursorPos()**：获取当前光标位置。

**uint2 Rmp8x8(uint id)**：将 0~63 的值以 swizzle 顺序映射到 8x8 的正方形内的坐标，用以提高纹理缓存的命中率。

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

支持的格式有 bmp，png，jpg 等常见图像格式以及 DDS 文件。纹理尺寸与源图像尺寸相同。

大多数情况下该纹理可以作为渲染目标（在 SAVE 中使用），除非：源图像为 DDS 格式且它存储的纹理格式无法作为渲染目标（如压缩纹理）。
