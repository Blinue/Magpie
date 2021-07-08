//!DESC SSimDownscaler calc L2 pass 2

cbuffer constants : register(b0) {
    int2 srcSize : packoffset(c0.x);
};


#define MAGPIE_INPUT_COUNT 1    // L1
#include "common.hlsli"



#define MN(B,C,x)   (x < 1.0 ? ((2.-1.5*B-(C))*x + (-3.+2.*B+C))*x*x + (1.-(B)/3.) : (((-(B)/6.-(C))*x + (B+5.*C))*x + (-2.*B-8.*C))*x+((4./3.)*B+4.*C))
#define Kernel(x)   MN(1.0/3.0, 1.0/3.0, abs(x))
#define taps        2.0



D2D_PS_ENTRY(main) {
    InitMagpieSampleInput();

    float2 cur = Coord(0).xy / Coord(0).zw;
    // Calculate bounds
    float low = ceil(cur.x - taps - 0.5);
    float high = floor(cur.x + taps - 0.5);

    float W = 0.0;
    float3 avg = 0;
    float2 pos = cur;

    for (float k = low; k <= high; k++) {
        pos.x = k + 0.5;
        float w = Kernel(pos.x - cur.x);
        avg += w * uncompressLinear(SampleInputLod(0, GetCheckedPos(0, pos * Coord(0).zw)).xyz, -0.5, 1.5);
        W += w;
    }

    avg /= W;
    return float4(compressLinear(avg, -0.5, 1.5), 1);
}
