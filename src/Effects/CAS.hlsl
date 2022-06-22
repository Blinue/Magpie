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

#ifdef MP_FP16

void CasFilterH(
    MF3 src[4][4],
    uint pos,
    MF peak,
    // Output values are for 2 8x8 tiles in a 16x8 region.
    //  pix<R,G,B>.x = right 8x8 tile
    //  pix<R,G,B>.y =  left 8x8 tile
    // This enables later processing to easily be packed as well.
    out MF2 pixR,
    out MF2 pixG,
    out MF2 pixB
) {
    // AOS to SOA conversion.
    MF2 aR = MF2(src[0][pos + 0].r, src[1][pos + 0].r);
    MF2 aG = MF2(src[0][pos + 0].g, src[1][pos + 0].g);
    MF2 aB = MF2(src[0][pos + 0].b, src[1][pos + 0].b);
    MF2 bR = MF2(src[1][pos + 0].r, src[2][pos + 0].r);
    MF2 bG = MF2(src[1][pos + 0].g, src[2][pos + 0].g);
    MF2 bB = MF2(src[1][pos + 0].b, src[2][pos + 0].b);
    MF2 cR = MF2(src[2][pos + 0].r, src[3][pos + 0].r);
    MF2 cG = MF2(src[2][pos + 0].g, src[3][pos + 0].g);
    MF2 cB = MF2(src[2][pos + 0].b, src[3][pos + 0].b);
    MF2 dR = MF2(src[0][pos + 1].r, src[1][pos + 1].r);
    MF2 dG = MF2(src[0][pos + 1].g, src[1][pos + 1].g);
    MF2 dB = MF2(src[0][pos + 1].b, src[1][pos + 1].b);
    MF2 eR = MF2(src[1][pos + 1].r, src[2][pos + 1].r);
    MF2 eG = MF2(src[1][pos + 1].g, src[2][pos + 1].g);
    MF2 eB = MF2(src[1][pos + 1].b, src[2][pos + 1].b);
    MF2 fR = MF2(src[2][pos + 1].r, src[3][pos + 1].r);
    MF2 fG = MF2(src[2][pos + 1].g, src[3][pos + 1].g);
    MF2 fB = MF2(src[2][pos + 1].b, src[3][pos + 1].b);
    MF2 gR = MF2(src[0][pos + 2].r, src[1][pos + 2].r);
    MF2 gG = MF2(src[0][pos + 2].g, src[1][pos + 2].g);
    MF2 gB = MF2(src[0][pos + 2].b, src[1][pos + 2].b);
    MF2 hR = MF2(src[1][pos + 2].r, src[2][pos + 2].r);
    MF2 hG = MF2(src[1][pos + 2].g, src[2][pos + 2].g);
    MF2 hB = MF2(src[1][pos + 2].b, src[2][pos + 2].b);
    MF2 iR = MF2(src[2][pos + 2].r, src[3][pos + 2].r);
    MF2 iG = MF2(src[2][pos + 2].g, src[3][pos + 2].g);
    MF2 iB = MF2(src[2][pos + 2].b, src[3][pos + 2].b);

    // Soft min and max.
    MF2 mnR = min(min(fR, hR), min(min(bR, dR), eR));
    MF2 mnG = min(min(fG, hG), min(min(bG, dG), eG));
    MF2 mnB = min(min(fB, hB), min(min(bB, dB), eB));
#ifdef CAS_BETTER_DIAGONALS
    MF2 mnR2 = min(min(gR, iR), min(min(aR, cR), mnR));
    MF2 mnG2 = min(min(gG, iG), min(min(aG, cG), mnG));
    MF2 mnB2 = min(min(gB, iB), min(min(aB, cB), mnB));
    mnR = mnR + mnR2;
    mnG = mnG + mnG2;
    mnB = mnB + mnB2;
#endif
    MF2 mxR = max(max(fR, hR), max(max(bR, dR), eR));
    MF2 mxG = max(max(fG, hG), max(max(bG, dG), eG));
    MF2 mxB = max(max(fB, hB), max(max(bB, dB), eB));
#ifdef CAS_BETTER_DIAGONALS
    MF2 mxR2 = max(max(gR, iR), max(max(aR, cR), mxR));
    MF2 mxG2 = max(max(gG, iG), max(max(aG, cG), mxG));
    MF2 mxB2 = max(max(gB, iB), max(max(aB, cB), mxB));
    mxR = mxR + mxR2;
    mxG = mxG + mxG2;
    mxB = mxB + mxB2;
#endif
    // Smooth minimum distance to signal limit divided by smooth max.
    MF2 rcpMR = rcp(mxR);
    MF2 rcpMG = rcp(mxG);
    MF2 rcpMB = rcp(mxB);

#ifdef CAS_BETTER_DIAGONALS
    MF2 ampR = saturate(min(mnR, 2.0 - mxR) * rcpMR);
    MF2 ampG = saturate(min(mnG, 2.0 - mxG) * rcpMG);
    MF2 ampB = saturate(min(mnB, 2.0 - mxB) * rcpMB);
#else
    MF2 ampR = saturate(min(mnR, 1.0 - mxR) * rcpMR);
    MF2 ampG = saturate(min(mnG, 1.0 - mxG) * rcpMG);
    MF2 ampB = saturate(min(mnB, 1.0 - mxB) * rcpMB);
#endif
    // Shaping amount of sharpening.

    ampR = sqrt(ampR);
    ampG = sqrt(ampG);
    ampB = sqrt(ampB);

    // Filter shape.
    MF2 wR = ampR * peak;
    MF2 wG = ampG * peak;
    MF2 wB = ampB * peak;
    // Filter.

    MF2 rcpWeight = rcp(1.0 + 4.0 * wG);

    pixR = saturate((bR * wG + dR * wG + fR * wG + hR * wG + eR) * rcpWeight);
    pixG = saturate((bG * wG + dG * wG + fG * wG + hG * wG + eG) * rcpWeight);
    pixB = saturate((bB * wG + dB * wG + fB * wG + hB * wG + eB) * rcpWeight);
}

#else

MF3 CasFilter(MF3 src[4][4], uint2 pos, MF peak) {
    // a b c 
    // d e f
    // g h i
    MF3 a = src[pos.x - 1][pos.y - 1];
    MF3 b = src[pos.x][pos.y - 1];
    MF3 c = src[pos.x + 1][pos.y - 1];
    MF3 d = src[pos.x - 1][pos.y];
    MF3 e = src[pos.x][pos.y];
    MF3 f = src[pos.x + 1][pos.y];
    MF3 g = src[pos.x - 1][pos.y + 1];
    MF3 h = src[pos.x][pos.y + 1];
    MF3 i = src[pos.x + 1][pos.y + 1];

    // Soft min and max.
    //  a b c             b
    //  d e f * 0.5  +  d e f * 0.5
    //  g h i             h
    // These are 2.0x bigger (factored out the extra multiply).
    MF mnR = min3(min3(d.r, e.r, f.r), b.r, h.r);
    MF mnG = min3(min3(d.g, e.g, f.g), b.g, h.g);
    MF mnB = min3(min3(d.b, e.b, f.b), b.b, h.b);
#ifdef CAS_BETTER_DIAGONALS
    MF mnR2 = min3(min3(mnR, a.r, c.r), g.r, i.r);
    MF mnG2 = min3(min3(mnG, a.g, c.g), g.g, i.g);
    MF mnB2 = min3(min3(mnB, a.b, c.b), g.b, i.b);
    mnR = mnR + mnR2;
    mnG = mnG + mnG2;
    mnB = mnB + mnB2;
#endif
    MF mxR = max3(max3(d.r, e.r, f.r), b.r, h.r);
    MF mxG = max3(max3(d.g, e.g, f.g), b.g, h.g);
    MF mxB = max3(max3(d.b, e.b, f.b), b.b, h.b);
#ifdef CAS_BETTER_DIAGONALS
    MF mxR2 = max3(max3(mxR, a.r, c.r), g.r, i.r);
    MF mxG2 = max3(max3(mxG, a.g, c.g), g.g, i.g);
    MF mxB2 = max3(max3(mxB, a.b, c.b), g.b, i.b);
    mxR = mxR + mxR2;
    mxG = mxG + mxG2;
    mxB = mxB + mxB2;
#endif
    // Smooth minimum distance to signal limit divided by smooth max.

    MF rcpMR = rcp(mxR);
    MF rcpMG = rcp(mxG);
    MF rcpMB = rcp(mxB);

#ifdef CAS_BETTER_DIAGONALS
    MF ampR = saturate(min(mnR, 2.0 - mxR) * rcpMR);
    MF ampG = saturate(min(mnG, 2.0 - mxG) * rcpMG);
    MF ampB = saturate(min(mnB, 2.0 - mxB) * rcpMB);
#else
    MF ampR = saturate(min(mnR, 1.0 - mxR) * rcpMR);
    MF ampG = saturate(min(mnG, 1.0 - mxG) * rcpMG);
    MF ampB = saturate(min(mnB, 1.0 - mxB) * rcpMB);
#endif
    // Shaping amount of sharpening.
    ampR = sqrt(ampR);
    ampG = sqrt(ampG);
    ampB = sqrt(ampB);

    // Filter shape.
    //  0 w 0
    //  w 1 w
    //  0 w 0
    MF wR = ampR * peak;
    MF wG = ampG * peak;
    MF wB = ampB * peak;
    // Filter.
    // Using green coef only, depending on dead code removal to strip out the extra overhead.
    MF rcpWeight = rcp(1.0 + 4.0 * wG);

    return MF3(
        saturate((b.r * wG + d.r * wG + f.r * wG + h.r * wG + e.r) * rcpWeight),
        saturate((b.g * wG + d.g * wG + f.g * wG + h.g * wG + e.g) * rcpWeight),
        saturate((b.b * wG + d.b * wG + f.b * wG + h.b * wG + e.b) * rcpWeight)
        );
}

#endif


void Pass1(uint2 blockStart, uint3 threadId) {
    uint2 gxy = blockStart + (Rmp8x8(threadId.x) << 1);
    if (!CheckViewport(gxy)) {
        return;
    }

    float2 inputPt = GetInputPt();
    uint i, j;

    MF3 src[4][4];
    [unroll]
    for (i = 0; i < 3; i += 2) {
        [unroll]
        for (j = 0; j < 3; j += 2) {
            float2 tpos = (gxy + uint2(i, j)) * inputPt;
            const MF4 sr = (MF4)INPUT.GatherRed(sam, tpos);
            const MF4 sg = (MF4)INPUT.GatherGreen(sam, tpos);
            const MF4 sb = (MF4)INPUT.GatherBlue(sam, tpos);

            // w z
            // x y
            src[i][j] = MF3(sr.w, sg.w, sb.w);
            src[i][j + 1] = MF3(sr.x, sg.x, sb.x);
            src[i + 1][j] = MF3(sr.z, sg.z, sb.z);
            src[i + 1][j + 1] = MF3(sr.y, sg.y, sb.y);
        }
    }

    const MF peak = -rcp(lerp(8.0, 5.0, (MF)sharpness));

#ifdef MP_FP16
    MF2 pixR, pixG, pixB;
    CasFilterH(src, 0, peak, pixR, pixG, pixB);

    WriteToOutput(gxy, float3(pixR.x, pixG.x, pixB.x));

    ++gxy.x;
    if (CheckViewport(gxy)) {
        WriteToOutput(gxy, float3(pixR.y, pixG.y, pixB.y));
    }

    CasFilterH(src, 1, peak, pixR, pixG, pixB);

    ++gxy.y;
    if (CheckViewport(gxy)) {
        WriteToOutput(gxy, float3(pixR.y, pixG.y, pixB.y));
    }

    --gxy.x;
    if (CheckViewport(gxy)) {
        WriteToOutput(gxy, float3(pixR.x, pixG.x, pixB.x));
    }
#else
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
#endif
}
