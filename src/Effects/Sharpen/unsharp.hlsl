// unsharp
// 移植自 https://github.com/mpv-player/mpv/blob/233e89698e69242209645f61472445bffb36fa42/video/out/gpu/video_shaders.c#L1019

//!MAGPIE EFFECT
//!VERSION 4

//!PARAMETER
//!LABEL Sharpness
//!DEFAULT 0.0
//!MIN -4.0
//!MAX 4.0
//!STEP 0.1
float SHARP;

//!TEXTURE
Texture2D INPUT;

//!TEXTURE
//!WIDTH INPUT_WIDTH
//!HEIGHT INPUT_HEIGHT
Texture2D OUTPUT;

//!SAMPLER
//!FILTER LINEAR
SamplerState sam;

//!PASS 1
//!STYLE PS
//!IN INPUT
//!OUT OUTPUT

MF4 Pass1(float2 pos) {
    const float st1 = 1.2;
    const float st2 = 1.5;

    float2 pt = GetInputPt();
    MF4 p = INPUT.SampleLevel(sam, pos, 0);

    MF4 sum1 = INPUT.SampleLevel(sam, pos + st1 * float2(+1, +1) * pt, 0)
             + INPUT.SampleLevel(sam, pos + st1 * float2(+1, -1) * pt, 0)
             + INPUT.SampleLevel(sam, pos + st1 * float2(-1, +1) * pt, 0)
             + INPUT.SampleLevel(sam, pos + st1 * float2(-1, -1) * pt, 0);

    MF4 sum2 = INPUT.SampleLevel(sam, pos + st2 * float2(+1,  0) * pt, 0)
             + INPUT.SampleLevel(sam, pos + st2 * float2( 0, +1) * pt, 0)
             + INPUT.SampleLevel(sam, pos + st2 * float2(-1,  0) * pt, 0)
             + INPUT.SampleLevel(sam, pos + st2 * float2( 0, -1) * pt, 0);

    MF4 t = p * 0.859375 + sum2 * -0.1171875 + sum1 * -0.09765625;

    return p + t * SHARP;
}

