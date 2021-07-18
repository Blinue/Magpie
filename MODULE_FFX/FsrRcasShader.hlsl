cbuffer constants : register(b0) {
    uint2 srcSize : packoffset(c0.x);
    float sharpness : packoffset(c0.z);
};

#define MAGPIE_INPUT_COUNT 1
#define MAGPIE_NO_CHECK
#include "common.hlsli"


#define A_GPU
#define A_HLSL
#include "ffx_a.hlsli"
#define FSR_RCAS_F

AF4 FsrRcasLoadF(ASU2 p) { return LoadInput(0, int3(ASU2(p), 0)); }
void FsrRcasInputF(inout AF1 r, inout AF1 g, inout AF1 b) {}


#include "ffx_fsr1.hlsli"


MAGPIE_ENTRY(main) {
    float s = AExp2F1(-sharpness);
    varAF2(hSharp) = initAF2(s, s);

    float3 c;
    FsrRcasF(c.r, c.g, c.b, Coord(0).xy / Coord(0).zw, float4(AU1_AF1(s), AU1_AH2_AF2(hSharp), 0, 0));

    return float4(c, 1.0f);
}