// SSimDownscaler calc Mean

cbuffer constants : register(b0) {
    int2 srcSize : packoffset(c0.x);
};


#define MAGPIE_INPUT_COUNT 1    // POSTKERNEL
#include "common.hlsli"


#define locality    8.0

#define Kernel(x)   pow(1.0 / locality, abs(x))
#define taps        3.0
#define maxtaps     taps



float3 ScaleH(float2 pos, float curX) {
    // Calculate bounds
    float low = floor(-0.5 * maxtaps);
    float high = floor(0.5 * maxtaps);

    float W = 0;
    float3 avg = 0;

    for (float k = 0.0; k < maxtaps; k++) {
        float rel = k + low + 1.0;
        pos.x = curX + rel;
        float w = Kernel(rel);

        avg += w * SampleInputChecked(0, pos * Coord(0).zw).rgb;
        W += w;
    }
    avg /= W;

    return avg;
}

D2D_PS_ENTRY(main) {
    InitMagpieSampleInput();
    float2 cur = Coord(0).xy / Coord(0).zw;

    // Calculate bounds
    float low = floor(-0.5 * maxtaps);
    float high = floor(0.5 * maxtaps);

    float W = 0;
    float3 avg = 0;
    float2 pos = cur;

    for (float k = 0.0; k < maxtaps; k++) {
        float rel = k + low + 1.0;
        pos.y = cur.y + rel;
        float w = Kernel(rel);

        avg += w * ScaleH(pos, cur.x);
        W += w;
    }
    avg /= W;
    return float4(avg, 1);
}
