// SSimDownscaler calc L2 pass 1

cbuffer constants : register(b0) {
	int2 srcSize : packoffset(c0.x);
    int2 destSize : packoffset(c0.z);
};


#define MAGPIE_INPUT_COUNT 1    // PREKERNEL
#include "common.hlsli"


#define MN(B,C,x)   (x < 1.0 ? ((2.-1.5*B-(C))*x + (-3.+2.*B+C))*x*x + (1.-(B)/3.) : (((-(B)/6.-(C))*x + (B+5.*C))*x + (-2.*B-8.*C))*x+((4./3.)*B+4.*C))
#define Kernel(x)   MN(1.0/3.0, 1.0/3.0, abs(x))
#define taps        2.0


D2D_PS_ENTRY(main) {
    float2 scale = float2(destSize) / srcSize;
    InitMagpieSampleInputWithScale(scale);

    float2 cur = Coord(0).xy / Coord(0).zw;

    // Calculate bounds
    float low = ceil(cur.y - taps - 0.5);
    float high = floor(cur.y + taps - 0.5);

    float W = 0;
    float3 avg = 0;
    float2 pos = cur;

    for (float k = low; k <= high; k++) {
        pos.y =  k + 0.5;
        float w = Kernel((pos.y - cur.y) * scale.y);

        float3 t = SampleInputLod(0, GetCheckedPos(0, pos * Coord(0).zw)).xyz;
        avg += w * t * t;
        W += w;
    }
    avg /= W;

    return float4(compressLinear(avg, -0.5, 1.5), 1);
}
