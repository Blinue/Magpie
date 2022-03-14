// Bicubic 的特化，等价于 paramB = 0, paramC = 0.5，为速度优化
// 移植自 https://gist.github.com/TheRealMJP/c83b8c0f46b63f3a88a5986f4fa982b1


//!MAGPIE EFFECT
//!VERSION 2


//!TEXTURE
Texture2D INPUT;

//!SAMPLER
//!FILTER LINEAR
SamplerState sam;


//!PASS 1
//!STYLE PS
//!IN INPUT


// Samples a texture with Catmull-Rom filtering, using 9 texture fetches instead of 16.
// See http://vec3.ca/bicubic-filtering-in-fewer-taps/ for more details
float4 Pass1(float2 pos) {
	float2 inputSize = GetInputSize();
	float2 inputPt = GetInputPt();

    // We're going to sample a a 4x4 grid of texels surrounding the target UV coordinate. We'll do this by rounding
    // down the sample location to get the exact center of our "starting" texel. The starting texel will be at
    // location [1, 1] in the grid, where [0, 0] is the top left corner.
    float2 samplePos = pos * inputSize;
    float2 texPos1 = floor(samplePos - 0.5f) + 0.5f;

    // Compute the fractional offset from our starting texel to our original sample location, which we'll
    // feed into the Catmull-Rom spline function to get our filter weights.
    float2 f = samplePos - texPos1;

    // Compute the Catmull-Rom weights using the fractional offset that we calculated earlier.
    // These equations are pre-expanded based on our knowledge of where the texels will be located,
    // which lets us avoid having to evaluate a piece-wise function.
    float2 w0 = f * (-0.5f + f * (1.0f - 0.5f * f));
    float2 w1 = 1.0f + f * f * (-2.5f + 1.5f * f);
    float2 w2 = f * (0.5f + f * (2.0f - 1.5f * f));
    float2 w3 = f * f * (-0.5f + 0.5f * f);

    // Work out weighting factors and sampling offsets that will let us use bilinear filtering to
    // simultaneously evaluate the middle 2 samples from the 4x4 grid.
    float2 w12 = w1 + w2;
    float2 offset12 = w2 / (w1 + w2);

    // Compute the final UV coordinates we'll use for sampling the texture
    float2 texPos0 = texPos1 - 1;
    float2 texPos3 = texPos1 + 2;
    float2 texPos12 = texPos1 + offset12;

    texPos0 *= inputPt;
    texPos3 *= inputPt;
    texPos12 *= inputPt;

    float4 result = 0.0f;
    result += INPUT.SampleLevel(sam, float2(texPos0.x, texPos0.y), 0) * w0.x * w0.y;
    result += INPUT.SampleLevel(sam, float2(texPos12.x, texPos0.y), 0) * w12.x * w0.y;
    result += INPUT.SampleLevel(sam, float2(texPos3.x, texPos0.y), 0) * w3.x * w0.y;

    result += INPUT.SampleLevel(sam, float2(texPos0.x, texPos12.y), 0) * w0.x * w12.y;
    result += INPUT.SampleLevel(sam, float2(texPos12.x, texPos12.y), 0) * w12.x * w12.y;
    result += INPUT.SampleLevel(sam, float2(texPos3.x, texPos12.y), 0) * w3.x * w12.y;

    result += INPUT.SampleLevel(sam, float2(texPos0.x, texPos3.y), 0) * w0.x * w3.y;
    result += INPUT.SampleLevel(sam, float2(texPos12.x, texPos3.y), 0) * w12.x * w3.y;
    result += INPUT.SampleLevel(sam, float2(texPos3.x, texPos3.y), 0) * w3.x * w3.y;

    return result;
}
