// 移植自 https://github.com/GPUOpen-Effects/FidelityFX-CAS/blob/master/ffx-cas/ffx_cas.h

//!MAGPIE EFFECT
//!VERSION 2
//!OUTPUT_WIDTH INPUT_WIDTH
//!OUTPUT_HEIGHT INPUT_HEIGHT

//!PARAMETER
//!DEFAULT 0.4
//!MIN 0
//!MAX 1
float sharpness;

//!TEXTURE
Texture2D INPUT;

//!SAMPLER
//!FILTER POINT
SamplerState sam;


//!PASS 1
//!IN INPUT
//!BLOCK_SIZE 16
//!NUM_THREADS 64

// 取消注释此行将降低速度并提高输出质量
// #define CAS_BETTER_DIAGONALS

#define min3(x, y, z) (min(x, min(y, z)))
#define max3(x, y, z) (max(x, max(y, z)))

float3 CasFilter(float3 src[4][4], uint2 pos, float peak) {
    // a b c 
    // d e f
    // g h i
    float3 a = src[pos.x - 1][pos.y - 1];
    float3 b = src[pos.x][pos.y - 1];
    float3 c = src[pos.x + 1][pos.y - 1];
    float3 d = src[pos.x - 1][pos.y];
    float3 e = src[pos.x][pos.y];
    float3 f = src[pos.x + 1][pos.y];
    float3 g = src[pos.x - 1][pos.y + 1];
    float3 h = src[pos.x][pos.y + 1];
    float3 i = src[pos.x + 1][pos.y + 1];

    // Soft min and max.
    //  a b c             b
    //  d e f * 0.5  +  d e f * 0.5
    //  g h i             h
    // These are 2.0x bigger (factored out the extra multiply).
    float mnR = min3(min3(d.r, e.r, f.r), b.r, h.r);
    float mnG = min3(min3(d.g, e.g, f.g), b.g, h.g);
    float mnB = min3(min3(d.b, e.b, f.b), b.b, h.b);
#ifdef CAS_BETTER_DIAGONALS
    float mnR2 = min3(min3(mnR, a.r, c.r), g.r, i.r);
    float mnG2 = min3(min3(mnG, a.g, c.g), g.g, i.g);
    float mnB2 = min3(min3(mnB, a.b, c.b), g.b, i.b);
    mnR = mnR + mnR2;
    mnG = mnG + mnG2;
    mnB = mnB + mnB2;
#endif
    float mxR = max3(max3(d.r, e.r, f.r), b.r, h.r);
    float mxG = max3(max3(d.g, e.g, f.g), b.g, h.g);
    float mxB = max3(max3(d.b, e.b, f.b), b.b, h.b);
#ifdef CAS_BETTER_DIAGONALS
    float mxR2 = max3(max3(mxR, a.r, c.r), g.r, i.r);
    float mxG2 = max3(max3(mxG, a.g, c.g), g.g, i.g);
    float mxB2 = max3(max3(mxB, a.b, c.b), g.b, i.b);
    mxR = mxR + mxR2;
    mxG = mxG + mxG2;
    mxB = mxB + mxB2;
#endif
    // Smooth minimum distance to signal limit divided by smooth max.

    float rcpMR = rcp(mxR);
    float rcpMG = rcp(mxG);
    float rcpMB = rcp(mxB);

#ifdef CAS_BETTER_DIAGONALS
    float ampR = saturate(min(mnR, 2.0 - mxR) * rcpMR);
    float ampG = saturate(min(mnG, 2.0 - mxG) * rcpMG);
    float ampB = saturate(min(mnB, 2.0 - mxB) * rcpMB);
#else
    float ampR = saturate(min(mnR, 1.0 - mxR) * rcpMR);
    float ampG = saturate(min(mnG, 1.0 - mxG) * rcpMG);
    float ampB = saturate(min(mnB, 1.0 - mxB) * rcpMB);
#endif
    // Shaping amount of sharpening.
    ampR = sqrt(ampR);
    ampG = sqrt(ampG);
    ampB = sqrt(ampB);

    // Filter shape.
    //  0 w 0
    //  w 1 w
    //  0 w 0
    float wR = ampR * peak;
    float wG = ampG * peak;
    float wB = ampB * peak;
    // Filter.
    // Using green coef only, depending on dead code removal to strip out the extra overhead.
    float rcpWeight = rcp(1.0 + 4.0 * wG);

    return float3(
        saturate((b.r * wG + d.r * wG + f.r * wG + h.r * wG + e.r) * rcpWeight),
        saturate((b.g * wG + d.g * wG + f.g * wG + h.g * wG + e.g) * rcpWeight),
        saturate((b.b * wG + d.b * wG + f.b * wG + h.b * wG + e.b) * rcpWeight)
    );
}

void Pass1(uint2 blockStart, uint3 threadId) {
    uint2 gxy = blockStart + (Rmp8x8(threadId.x) << 1);
    if (!CheckViewport(gxy)) {
        return;
    }

    float2 inputPt = GetInputPt();
    uint i, j;

    float3 src[4][4];
    [unroll]
    for (i = 0; i < 3; i += 2) {
        [unroll]
        for (j = 0; j < 3; j += 2) {
            float2 tpos = (gxy + uint2(i, j)) * inputPt;
            const float4 sr = INPUT.GatherRed(sam, tpos);
            const float4 sg = INPUT.GatherGreen(sam, tpos);
            const float4 sb = INPUT.GatherBlue(sam, tpos);

            // w z
            // x y
            src[i][j] = float3(sr.w, sg.w, sb.w);
            src[i][j + 1] = float3(sr.x, sg.x, sb.x);
            src[i + 1][j] = float3(sr.z, sg.z, sb.z);
            src[i + 1][j + 1] = float3(sr.y, sg.y, sb.y);
        }
    }

    const float peak = -rcp(lerp(8.0, 5.0, sharpness));

    WriteToOutput(gxy, CasFilter(src, uint2(1, 1), peak));

    ++gxy.x;
    if (CheckViewport(gxy)) {
        WriteToOutput(gxy, CasFilter(src, uint2(2, 1), peak));
    }

    ++gxy.y;
    if (CheckViewport(gxy)) {
        WriteToOutput(gxy, CasFilter(src, uint2(2, 2), peak));
    }

    --gxy.x;
    if (CheckViewport(gxy)) {
        WriteToOutput(gxy, CasFilter(src, uint2(1, 2), peak));
    }
}
