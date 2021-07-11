// SSimDownscaler final pass

cbuffer constants : register(b0) {
    int2 srcSize : packoffset(c0.x);
};


#define MAGPIE_INPUT_COUNT 3    // POSTKERNEL, Mean, R
#include "common.hlsli"


#define locality    8.0

#define Kernel(x)   pow(1.0 / locality, abs(x))
#define taps        3.0
#define maxtaps     taps

#define Gamma(x)    (pow(x, 0.5f))
#define GammaInv(x) (pow(x, 2))

#define GetLuma(x) (dot(x, float3(0.299, 0.587, 0.114)))


float3x3 ScaleH(float2 pos, float curX) {
    // Calculate bounds
    float low = floor(-0.5 * maxtaps);
    float high = floor(0.5 * maxtaps);

    float W = 0.0;
    float3x3 avg = 0;

    for (float k = 0.0; k < maxtaps; k++) {
        float rel = k + low + 1.0;
        pos.x = curX + rel;
        float w = Kernel(rel);

        float3 M = Gamma(SampleInputChecked(1, pos * Coord(1).zw).rgb);
        float3 R = SampleInputChecked(2, pos * Coord(2).zw).rgb;
        R = 1.0 / R - 1.0;
        avg += w * float3x3(R * M, M, R);
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

    float W = 0.0;
    float3x3 avg = 0;
    float2 pos = cur;

    for (float k = 0.0; k < maxtaps; k++) {
        float rel = k + low + 1.0;
        pos.y = cur.y + rel;
        float w = Kernel(rel);

        avg += w * ScaleH(pos, cur.x);
        W += w;
    }
    avg /= W;

    
    float3 origin = SampleInputCur(0).rgb;
    float3 r = GammaInv(avg[1] + avg[2] * Gamma(origin) - avg[0]);

    return float4(r, 1);
}
