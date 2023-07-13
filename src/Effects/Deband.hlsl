// Deband
// Port from https://github.com/haasn/gentoo-conf/blob/xor/home/nand/.mpv/shaders/deband.glsl

//!MAGPIE EFFECT
//!VERSION 3
//!OUTPUT_WIDTH INPUT_WIDTH
//!OUTPUT_HEIGHT INPUT_HEIGHT

//!PARAMETER
//!LABEL Threshold
//!DEFAULT 64
//!MIN 0
//!MAX 512
//!STEP 4
// The threshold of difference below which a pixel is considered to be part of
// a gradient. Higher = more debanding, but setting it too high diminishes image
// details.
float threshold;

//!PARAMETER
//!LABEL Range
//!DEFAULT 8
//!MIN 0
//!MAX 16
//!STEP 0.1
// The range (in source pixels) at which to sample for neighbours. Higher values
// will find more gradients, but lower values will deband more aggressively.
float range;

//!PARAMETER
//!LABEL Iterations
//!DEFAULT 4
//!MIN 1
//!MAX 10
//!STEP 1
// The number of debanding iterations to perform. Each iteration samples from
// random positions, so increasing the number of iterations is likely to
// increase the debanding quality. Conversely, it slows the shader down.
// (Each iteration will use a multiple of the configured range, and a
// successively lower THRESHOLD - so setting it much higher has little effect)
int iterations;

//!PARAMETER
//!LABEL Grain
//!DEFAULT 48
//!MIN 0
//!MAX 256
//!STEP 4
// (Optional) Add some extra noise to the image. This significantly helps cover
// up remaining banding and blocking artifacts, at comparatively little visual
// quality. Higher = more grain. Setting it to 0 disables the effect.
float grain;

//!TEXTURE
Texture2D INPUT;

//!SAMPLER
//!FILTER LINEAR
SamplerState sam;

//!SAMPLER
//!FILTER POINT
SamplerState sam1;


//!PASS 1
//!STYLE PS
//!IN INPUT

// Wide usage friendly PRNG, shamelessly stolen from a GLSL tricks forum post
float mod289(float x)  { return x - floor(x / 289.0) * 289.0; }
float permute(float x) { return mod289((34.0*x + 1.0) * x); }
float rand(float x)    { return frac(x / 41.0); }

// Helper: Calculate a stochastic approximation of the avg color around a pixel
float3 average(float2 pos, float r, inout float h)
{
    const float2 pt = GetInputPt();
    // Compute a random rangle and distance
    float dist = rand(h) * r;         h = permute(h);
    float dir  = rand(h) * 6.2831853; h = permute(h);

    float2 o = float2(cos(dir), sin(dir)) * pt * dist;

    // Sample at quarter-turn intervals around the source pixel
    float3 ref[4];
    ref[0] = INPUT.SampleLevel(sam, pos + float2( o.x,  o.y), 0).rgb;
    ref[1] = INPUT.SampleLevel(sam, pos + float2(-o.y,  o.x), 0).rgb;
    ref[2] = INPUT.SampleLevel(sam, pos + float2(-o.x, -o.y), 0).rgb;
    ref[3] = INPUT.SampleLevel(sam, pos + float2( o.y, -o.x), 0).rgb;

    // Return the (normalized) average
    return (ref[0] + ref[1] + ref[2] + ref[3])/4.0;
}

float4 Pass1(float2 pos)
{
    // Initialize the PRNG by hashing the position + a random uniform
    float3 m = float3(pos, 4.6);
    float h = permute(permute(permute(m.x)+m.y)+m.z);

    // Sample the source pixel
    float3 col = INPUT.SampleLevel(sam1, pos, 0).rgb;

    for (int i = 1; i <= iterations; i++) {
        // Use the average instead if the difference is below the threshold
        float3 avg = average(pos, i*range, h);
        float3 diff = abs(col - avg);
        float3 thres = threshold / (i * 16384.0);
        col = lerp(avg, col, step(thres, diff));
    }

    // Add some random noise to the output
    float3 noise;
    noise.x = rand(h); h = permute(h);
    noise.y = rand(h); h = permute(h);
    noise.z = rand(h); h = permute(h);
    col += (grain/8192.0) * (noise - 0.5);

    return float4(col, 1.0);
}
