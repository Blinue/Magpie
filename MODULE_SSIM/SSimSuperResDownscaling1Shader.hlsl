// SSSR Downscaling I

cbuffer constants : register(b0) {
    int2 srcSize : packoffset(c0.x);
};


#define MAGPIE_INPUT_COUNT 1    // POSTKERNEL
#define MAGPIE_NO_CHECK
#include "common.hlsli"


#define axis 1

#define MN(B,C,x)   (x < 1.0 ? ((2.-1.5*B-(C))*x + (-3.+2.*B+C))*x*x + (1.-(B)/3.) : (((-(B)/6.-(C))*x + (B+5.*C))*x + (-2.*B-8.*C))*x+((4./3.)*B+4.*C))
#define Kernel(x)   MN(0.334, 0.333, abs(x))
#define taps        2.0f

#define Luma(rgb)   ( dot(rgb*rgb, float3(0.2126, 0.7152, 0.0722)) )


D2D_PS_ENTRY(main) {
    float2 cur = Coord(0).xy / Coord(0).zw;

    // Calculate bounds
    float low = ceil(cur.y - taps * 1.5 - 0.5f);
    float high = floor(cur.y + taps * 1.5 - 0.5f);

    float W = 0.0;
    float4 avg = 0;
    float2 pos = cur;
    float4 tex;
    
    for (float k = low; k <= high; k++) {
        pos.y = k + 0.5;
        float w = Kernel((pos.y - cur.y) / 1.5);

        tex.rgb = SampleInputLod(0, pos * Coord(0).zw).rgb;
        tex.a = Luma(tex.rgb);
        avg += w * tex;
        W += w;
    }
    avg /= W;

    return float4(avg.rgb, abs(avg.a - Luma(avg.rgb)) * 20);
}
