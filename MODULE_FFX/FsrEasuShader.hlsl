cbuffer constants : register(b0) {
    int2 srcSize : packoffset(c0.x);
    int2 destSize : packoffset(c0.z);
};

#define MAGPIE_INPUT_COUNT 1
#include "common.hlsli"


#define A_GPU 1
#define A_HLSL 1
#include "ffx_a.hlsli"
#define FSR_EASU_F 1

AF4 FsrEasuRF(AF2 p) {
    return GatherInputRed(0, p * srcSize * Coord(0).zw);
}
AF4 FsrEasuGF(AF2 p) {
    return GatherInputGreen(0, p * srcSize * Coord(0).zw);
}
AF4 FsrEasuBF(AF2 p) {
    return GatherInputBlue(0, p * srcSize * Coord(0).zw);
}


#include "ffx_fsr1.hlsli"


MAGPIE_ENTRY(main) {
    float2 rcpSrc = rcp(srcSize);
    float2 rcpDest = rcp(destSize);
    float2 scale = srcSize * rcpDest;

    float3 c;
    FsrEasuF(
        c,
        Coord(0).xy / Coord(0).zw,
        asuint(float4(scale, 0.5 * scale - 0.5)),
        asuint(float4(rcpSrc.xy, rcpSrc.x, -rcpSrc.y)),
        asuint(float4(-rcpSrc.x, 2 * rcpSrc.y, rcpSrc.x, 2 * rcpSrc.y)),
        asuint(float4(0, 4 * rcpSrc.y, 0, 0))
    );

    return float4(c, 1.0f);
}