// CRT-Hyllian
// 移植自 https://github.com/libretro/common-shaders/blob/master/crt/shaders/crt-hyllian.cg
// 要求整数倍缩放

/*
   Hyllian's CRT Shader

   Copyright (C) 2011-2016 Hyllian - sergiogdb@gmail.com

   Permission is hereby granted, free of charge, to any person obtaining a copy
   of this software and associated documentation files (the "Software"), to deal
   in the Software without restriction, including without limitation the rights
   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
   copies of the Software, and to permit persons to whom the Software is
   furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included in
   all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
   THE SOFTWARE.

*/

//!MAGPIE EFFECT
//!VERSION 2


//!PARAMETER
//!DEFAULT 1
//!MIN 0
//!MAX 1
int phosphor;

//!PARAMETER
//!DEFAULT 0
//!MIN 0
//!MAX 1
int vScanlines;

//!PARAMETER
//!DEFAULT 2.5
//!MIN 0
//!MAX 5
float inputGamma;

//!PARAMETER
//!DEFAULT 2.2
//!MIN 0
//!MAX 5
float outputGamma;

//!PARAMETER
//!DEFAULT 1
//!MIN 1
//!MAX 5
int sharpness;

//!PARAMETER
//!DEFAULT 1.5
//!MIN 1
//!MAX 2
float colorBoost;

//!PARAMETER
//!DEFAULT 1
//!MIN 1
//!MAX 2
float redBoost;

//!PARAMETER
//!DEFAULT 1
//!MIN 1
//!MAX 2
float greenBoost;

//!PARAMETER
//!DEFAULT 1
//!MIN 1
//!MAX 2
float blueBoost;

//!PARAMETER
//!DEFAULT 0.5
//!MIN 0
//!MAX 1
float scanlinesStrength;

//!PARAMETER
//!DEFAULT 0.86
//!MIN 0
//!MAX 1
float beamMinWidth;

//!PARAMETER
//!DEFAULT 1
//!MIN 0
//!MAX 1
float beamMaxWidth;

//!PARAMETER
//!DEFAULT 0.8
//!MIN 0
//!MAX 1
float crtAntiRinging;

//!TEXTURE
Texture2D INPUT;

//!SAMPLER
//!FILTER POINT
SamplerState sam;


//!PASS 1
//!STYLE PS
//!IN INPUT

#pragma warning(disable: 3571) // X3571: pow(f, e) will not work for negative f, use abs(f) or conditionally handle negative values if you expect them

#define GAMMA_IN(color)     pow(color, float3(inputGamma, inputGamma, inputGamma))
#define GAMMA_OUT(color)    pow(color, float3(1.0 / outputGamma, 1.0 / outputGamma, 1.0 / outputGamma))

// Horizontal cubic filter.

// Some known filters use these values:

//    B = 0.0, C = 0.0  =>  Hermite cubic filter.
//    B = 1.0, C = 0.0  =>  Cubic B-Spline filter.
//    B = 0.0, C = 0.5  =>  Catmull-Rom Spline filter. This is the default used in this shader.
//    B = C = 1.0/3.0   =>  Mitchell-Netravali cubic filter.
//    B = 0.3782, C = 0.3109  =>  Robidoux filter.
//    B = 0.2620, C = 0.3690  =>  Robidoux Sharp filter.
//    B = 0.36, C = 0.28  =>  My best config for ringing elimination in pixel art (Hyllian).


// For more info, see: http://www.imagemagick.org/Usage/img_diagrams/cubic_survey.gif

// Change these params to configure the horizontal filter.
const static float  B = 0.0;
const static float  C = 0.5;

const static float4x4 invX = float4x4((-B - 6.0 * C) / 6.0, (3.0 * B + 12.0 * C) / 6.0, (-3.0 * B - 6.0 * C) / 6.0, B / 6.0,
    (12.0 - 9.0 * B - 6.0 * C) / 6.0, (-18.0 + 12.0 * B + 6.0 * C) / 6.0, 0.0, (6.0 - 2.0 * B) / 6.0,
    -(12.0 - 9.0 * B - 6.0 * C) / 6.0, (18.0 - 15.0 * B - 12.0 * C) / 6.0, (3.0 * B + 6.0 * C) / 6.0, B / 6.0,
    (B + 6.0 * C) / 6.0, -C, 0.0, 0.0);

float4 Pass1(float2 pos) {
    uint2 inputSize = GetInputSize();
    uint2 outputSize = GetOutputSize();
    float3 color;

    float2 TextureSize = float2(sharpness * inputSize.x, inputSize.y);

    float2 dx = lerp(float2(1.0 / TextureSize.x, 0.0), float2(0.0, 1.0 / TextureSize.y), vScanlines);
    float2 dy = lerp(float2(0.0, 1.0 / TextureSize.y), float2(1.0 / TextureSize.x, 0.0), vScanlines);

    float2 pix_coord = pos * TextureSize + float2(-0.5, 0.5);

    float2 tc = lerp((floor(pix_coord) + float2(0.5, 0.5)) / TextureSize, (floor(pix_coord) + float2(1.0, -0.5)) / TextureSize, vScanlines);

    float2 fp = lerp(frac(pix_coord), frac(pix_coord.yx), vScanlines);

    float3 c00 = GAMMA_IN(INPUT.SampleLevel(sam, tc - dx - dy, 0).xyz);
    float3 c01 = GAMMA_IN(INPUT.SampleLevel(sam, tc - dy, 0).xyz);
    float3 c02 = GAMMA_IN(INPUT.SampleLevel(sam, tc + dx - dy, 0).xyz);
    float3 c03 = GAMMA_IN(INPUT.SampleLevel(sam, tc + 2.0 * dx - dy, 0).xyz);
    float3 c10 = GAMMA_IN(INPUT.SampleLevel(sam, tc - dx, 0).xyz);
    float3 c11 = GAMMA_IN(INPUT.SampleLevel(sam, tc, 0).xyz);
    float3 c12 = GAMMA_IN(INPUT.SampleLevel(sam, tc + dx, 0).xyz);
    float3 c13 = GAMMA_IN(INPUT.SampleLevel(sam, tc + 2.0 * dx, 0).xyz);

    //  Get min/max samples
    float3 min_sample = min(min(c01, c11), min(c02, c12));
    float3 max_sample = max(max(c01, c11), max(c02, c12));

    float4x3 color_matrix0 = float4x3(c00, c01, c02, c03);
    float4x3 color_matrix1 = float4x3(c10, c11, c12, c13);

    float4 invX_Px = mul(invX, float4(fp.x * fp.x * fp.x, fp.x * fp.x, fp.x, 1.0));
    float3 color0 = mul(invX_Px, color_matrix0);
    float3 color1 = mul(invX_Px, color_matrix1);

    // Anti-ringing
    float3 aux = color0;
    color0 = clamp(color0, min_sample, max_sample);
    color0 = lerp(aux, color0, crtAntiRinging);
    aux = color1;
    color1 = clamp(color1, min_sample, max_sample);
    color1 = lerp(aux, color1, crtAntiRinging);

    float pos0 = fp.y;
    float pos1 = 1 - fp.y;

    float3 lum0 = lerp(beamMinWidth, beamMaxWidth, color0);
    float3 lum1 = lerp(beamMinWidth, beamMaxWidth, color1);

    float3 d0 = clamp(pos0 / (lum0 + 0.0000001), 0.0, 1.0);
    float3 d1 = clamp(pos1 / (lum1 + 0.0000001), 0.0, 1.0);

    d0 = exp(-10.0 * scanlinesStrength * d0 * d0);
    d1 = exp(-10.0 * scanlinesStrength * d1 * d1);

    color = clamp(color0 * d0 + color1 * d1, 0.0, 1.0);

    color *= colorBoost * float3(redBoost, greenBoost, blueBoost);

    float mod_factor = lerp(pos.x * outputSize.x, pos.y * outputSize.y, vScanlines);

    float3 dotMaskWeights = lerp(
        float3(1.0, 0.7, 1.0),
        float3(0.7, 1.0, 0.7),
        floor(fmod(mod_factor, 2.0))
    );

    color.rgb *= lerp(1.0, dotMaskWeights, phosphor);

    color = GAMMA_OUT(color);

    return float4(color, 1.0);
}
