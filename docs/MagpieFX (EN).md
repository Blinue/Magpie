MagpieFX is based on DirectX 11 compute shader

``` hlsl
//!MAGPIE EFFECT
//!VERSION 4
// Specify "USE_DYNAMIC" to use GetFrameCount.
//!USE_DYNAMIC
// Use "SORT_NAME" to specify the name used for sorting, otherwise the files will be sorted by their file names.
//!SORT_NAME test1


// Definition of parameters
//!PARAMETER
// "LABEL" refers to the name of the parameter that is displayed on the user interface.
//!LABEL Sharpness
// "DEFAULT", "MIN", "MAX", and "STEP" are all required.
//!DEFAULT 0.1
//!MIN 0.01
//!MAX 5
//!STEP 0.01
float sharpness;


// Definition of textures
// "INPUT" and "OUTPUT" are special keywords.
// "INPUT" cannot be used as the output of a pass; "OUTPUT" cannot be used as the input of a pass.
// Defining INPUT/OUTPUT is optional, but it is recommended to define them explicitly for the sake of semantic completeness.
// The size of the OUTPUT represents the output size of this effect. Not specifying it indicates support for output of any size.

//!TEXTURE
Texture2D INPUT;

//!TEXTURE
//!WIDTH INPUT_WIDTH * 2
//!HEIGHT INPUT_HEIGHT * 2
Texture2D OUTPUT;

// You can use some pre-defined constants to calculate texture size.
// INPUT_WIDTH
// INPUT_HEIGHT
// OUTPUT_WIDTH
// OUTPUT_HEIGHT

// Supported texture formats:
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
// The definition of a texture in a pass varies depending on its format. For example,
// when the texture format is R8G8_UNORM, its definition as an input is Texture2D<float2>,
// and as an output it is RWTexture2D<unorm float2>.

//!TEXTURE
//!WIDTH INPUT_WIDTH + 100
//!HEIGHT INPUT_HEIGHT + 100
//!FORMAT R8G8B8A8_UNORM
Texture2D tex1;

// Definition of samplers
// "FILTER" is required，"ADDRESS" is optional
// The default value of "ADDRESS" is "CLAMP"

//!SAMPLER
//!FILTER LINEAR
//!ADDRESS CLAMP
SamplerState sam1;

//!SAMPLER
//!FILTER POINT
//!ADDRESS WRAP
SamplerState sam2;

// Code block shared by all passes

//!COMMON
#define PI 3.14159265359

// Definition of passes

//!PASS 1
// This description will appear in the in-game overlay.
//!DESC First Pass
// You can use a PS-like style, but it will still be implemented with CS.
// The default value of "STYLE" is "CS".
//!STYLE PS
//!IN INPUT
// Supports multiple render targets, up to 8.
//!OUT tex1

float func1() {
}

// "Pass[n]" is the entry point of the pass.
float4 Pass1(float2 pos) {
    return float4(1, 1, 1, 1);
}

//!PASS 2
//!IN INPUT, tex1
// The output of the last pass must be "OUTPUT".
//!OUT OUTPUT
// "BLOACK_SIZE" specifies how large an area is processed in one dispatch.
// "BLOACK_SIZE" can have only one dimension, meaning that length and height are specified at the same time.
//!BLOCK_SIZE 16, 16
// "NUM_THREADS" specifies how many parallel threads are involved in a single dispatch.
// "NUM_THREADS" can be less than three dimensions, and the missing dimensions are assumed to be 1 by default.
//!NUM_THREADS 64, 1, 1

void Pass2(uint2 blockStart, uint3 threadId) {
    // Write to OUPUT
    OUTPUT[blockStart] = float4(1,1,1,1);
}
```

### Predefined functions

**uint2 GetInputSize()**: Retrieves the size of the input texture.

**float2 GetInputPt()**: Retrieves the size of pixel in the input texture.

**uint2 GetOutputSize()**: Retrieves the size of the output texture.

**float2 GetOutputPt()**: Retrieves the size of pixel in the output texture.

**float2 GetScale()**: Retrieves the scaling factor between the output texture and the input texture.

**uint GetFrameCount()**: Retrieves the total number of frames rendered so far. When using this function, you must specify USE_DYNAMIC.

**uint2 Rmp8x8(uint id)**: Maps the values of 0 to 63 to coordinates in an 8x8 square in swizzle order, which can improve texture cache hit rate.


### Predefined macros

**MP_BLOCK_WIDTH、MP_BLOCK_HEIGHT**: The size of the block being processed in the current pass (specified by "BLOCK_SIZE").

**MP_NUM_THREADS_X、MP_NUM_THREADS_Y**: The number of threads per thread group in the current pass (specified by "NUM_THREADS").

**MP_PS_STYLE**: Whether the current pass is a pixel shader style pass (specified by "STYLE").

**MP_INLINE_PARAMS**: Whether the parameters for the current pass are static constants (specifed by user).

**MP_DEBUG**: Whether the shader is being compiled in debug mode (when compiling shaders in debug mode, they are not optimized and contain debug information).

**MP_FP16**: Whether to use half-precision floating-point numbers (specifed by user).

**MF、MF1、MF2、...、MF4x4**: Floating-point data types that conform to MP_FP16. When half-precision is not specified, they are aliases for float..., otherwise they are aliases for min16float...


### Multiple Render Targets (MRT)

In PS-style, the OUT instruction allows specifying multiple outputs (up to 8 due to DirectX limitations).
``` hlsl
//!OUT tex1, tex2
```

Now the entry point has a different signature:
``` hlsl
void Pass1(float2 pos, out float4 target1, out float4 target2);
```

### Load texture from file

``` hlsl
//!TEXTURE
//!SOURCE test.bmp
Texture2D testTex;
```

The TEXTURE instruction supports loading textures from files in common image formats such as BMP, PNG, JPG, and DDS. The texture size is the same as the source image size. FORMAT can be optionally specified to help the parser generate the correct definition. If FORMAT is not specified, it is always assumed to be of type float4.

Textures loaded from files cannot be used as the output of passes.
