MagpieFX Syntax

``` hlsl
//!MAGPIE EFFECT
//!VERSION 1
//!OUTPUT_WIDTH INPUT_WIDTH * 2
//!OUTPUT_HEIGHT INPUT_HEIGHT * 2

// Not defining OUTPUT_WIDTH and OUTPUT_HEIGHT means that this Effect takes any size of output

// Some predefined constants
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
// Variables with the DYNAMIC keyword may use the constants below. They will be kept 0 when not having the DYNAMIC keyword.
// FRAME_COUNT: The total number of frames presented
// CURSOR_X and CURSOR_Y: Cursor position. Top and left are 0. Bottom and right are 1.


// Define variables
// The variables with the VALUE keyword will fit the values in the corresponding expressions.
// They are mutable parameters at runtime otherwise.



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
//!LABEL Sharpness
float sharpness;

// Pattern definitions
// INPUT is a special keyword
// INPUT can not serve as the output for PASS
// Defining the INPUT is optional. However, to keep the completeness oif the syntax, we recommend you to explicitly define it.

//!TEXTURE
Texture2D INPUT;

//!TEXTURE
//!WIDTH INPUT_WIDTH + 100
//!HEIGHT INPUT_HEIGHT + 100
//!FORMAT B8G8R8A8_UNORM
Texture2D tex1;

// Sampler definitions
// FILTER (required), ADDRESS (optional)
// The default value for ADDRESS is CLAMP

//!SAMPLER
//!FILTER LINEAR
//!ADDRESS CLAMP
SamplerState sam1;

//!SAMPLER
//!FILTER POINT
//!ADDRESS WRAP
SamplerState sam2;

// Common part for all Pass-es

//!COMMON
#define PI 3.14159265359

// Pass definitions
// Needs to bind before using textures.

//!PASS 1
//!BIND INPUT
//!SAVE tex1
float func1() {
}

float4 Pass1(float2 pos) {
}

// Not defining SAVE means that this Pass is the output of the Effect

//!PASS 2
//!BIND tex1
float4 Pass2(float2 pos) {
}
```


**Multi Rendering Target (MRT)**

The SAVE command may designate multiple outputs (The limit of DirectX is 8):
``` hlsl
//!SAVE tex1, tex2
```

Here the Pass function has different signatures:
``` hlsl
void Pass[n](float2 pos, out float4 target1, out float4 target2);
```

**Load textures from files**

The TEXTURE command loads textures from files.

``` hlsl
//!TEXTURE
//!SOURCE test.bmp
Texture2D testTex;
```

Supported formats are common image formats like bmp, png, and jpg. DDS files are also supported. The texture size is exactly the same as the source image size.

In most situations the textures can serve as rendering targets (use in SAVE), unless the source file is in DDS format and the texture format cannot be used as a rendering target (e.g. compressing texture).
