MagpieFX 语法

``` hlsl
//!MAGPIE EFFECT
//!VERSION 1
//!OUTPUT_WIDTH INPUT_WIDTH * 2
//!OUTPUT_HEIGHT INPUT_HEIGHT * 2

// 如果不定义 OUTPUT_WIDTH 和 OUTPUT_HEIGHT 表示此 Effect 可以接受任何大小的输出

// 计算表达式时有一些预定义常量
// INPUT_WIDTH
// INPUT_HEIGHT
// INPUT_PT_X
// INPUT_PT_Y
// OUTPUT_WIDTH
// OUTPUT_HEIGHT
// OUTPUT_PT_X
// OUTPUT_PT_Y
// SCALE_X
// SCALE_Y
// 含有 DYNAMIC 关键字的变量还可以使用下面的常量，不含 DYNAMIC 关键字则它们始终为 0
// FRAME_COUNT：已呈现的总帧数
// CURSOR_X 和 CURSOR_Y：光标位置，左上角为 0，右下角为 1


// 变量定义
// 含有 VALUE 关键字的变量会填充该表达式的值
// 否则为在运行时可以改变的参数



//!CONSTANT
//!VALUE INPUT_WIDTH
int inputWidth;

//!CONSTANT
//!VALUE INPUT_HEIGHT
int inputHeight;

//!CONSTANT
//!DYNAMIC
//!VALUE FRAME_COUNT
int frameCount;

//!CONSTANT
//!DEFAULT 0
//!LABEL 锐度
float sharpness;

// 纹理定义
// INPUT 是特殊关键字
// INPUT 不能作为 Pass 的输出
// 定义 INPUT 是可选的，但为了保持语义的完整性，建议显式定义

//!TEXTURE
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

// 所有 Pass 通用的部分

//!COMMON
#define PI 3.14159265359

// Pass 定义
// 使用纹理时需要绑定

//!PASS 1
//!BIND INPUT
//!SAVE tex1
float func1() {
}

float4 Pass1(float2 pos) {
}

// 没有 SAVE 表示此 Pass 为 Effect 的输出

//!PASS 2
//!BIND tex1
float4 Pass2(float2 pos) {
}
```


**多渲染目标（MRT）**

SAVE 指令可指定多个输出（DirectX 限制最多 8 个）：
``` hlsl
//!SAVE tex1, tex2
```

这时 Pass 函数有不同的签名：
``` hlsl
void Pass[n](float2 pos, out float4 target1, out float4 target2);
```

**从文件加载纹理**

TEXTURE 指令可从文件加载纹理

``` hlsl
//!TEXTURE
//!SOURCE test.bmp
Texture2D testTex;
```

支持的格式有 bmp，png，jpg 等常见图像格式以及 DDS 文件。纹理尺寸与源图像尺寸相同。

大多数情况下该纹理可以作为渲染目标（在 SAVE 中使用），除非：源图像为 DDS 格式且它存储的纹理格式无法作为渲染目标（如压缩纹理）。
