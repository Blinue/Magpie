// SSSR varH

cbuffer constants : register(b0) {
    int2 srcSize : packoffset(c0.x);
    int2 destSize : packoffset(c0.z);
};

#define MAGPIE_INPUT_COUNT 5    // PREKERNEL, POSTKERNEL, LOWRES, varL, varH
#define MAGPIE_NO_CHECK
#include "common.hlsli"


// -- Window Size --
#define taps        3.0
#define even        (taps - 2.0 * floor(taps / 2.0) == 0.0)
#define minX        int(1.0-ceil(taps/2.0))
#define maxX        int(floor(taps/2.0))


#define Kernel(x)   (cos(acos(-1.0)*(x)/taps)) // Hann kernel

#define sqr(x)      dot(x,x)

// -- Input processing --

#define Gamma(x)    ( pow(clamp(x, 0.0, 1.0), float3(1.0/2.0)) )
#define GammaInv(x) ( pow(clamp(x, 0.0, 1.0), float3(2.0)) )
#define Luma(rgb)   ( dot(rgb*rgb, float3(0.2126, 0.7152, 0.0722)) )

float L(float x, float y) {
    float2 c = SampleInputOff(3, float2(x, y) + 0.5).xy;
    return (c.x + c.y / 10) / 8;
}

float H(float x, float y) {
    float2 c = SampleInputOff(4, float2(x, y) + 0.5).xy;
    return (c.x + c.y / 10) / 8;
}

float4 Lowres(float x, float y) {
    float4 c = SampleInputOff(2, float2(x, y) + 0.5);
    c.a = c.a /20;
    return c;
}

D2D_PS_ENTRY(main) {
    InitMagpieSampleInput();

    float2 scale = float2(destSize) / srcSize;
    Coord(0).xy /= scale;

    int X, Y;

    float4 c0 = SampleInputCur(1);

    // Calculate position
    float2 pos = Coord(1).xy / Coord(1).zw - 0.5;
    float2 offset = pos - (even ? floor(pos) : round(pos));
    pos -= offset;

    float2 mVar = 0;
    for (X = -1; X <= 1; X++)
        for (Y = -1; Y <= 1; Y++) {
            float2 w = clamp(1.5 - abs(float2(X, Y) - offset), 0.0, 1.0);
            mVar += w.r * w.g * float2(Lowres(X, Y).a, 1.0);
        }
    mVar.r /= mVar.g;

    // Calculate faithfulness force
    float weightSum = 0;
    float3 diff = 0;

    for (X = minX; X <= maxX; X++)
        for (Y = minX; Y <= maxX; Y++) {
            float varL = L(X, Y);
            float varH = H(X, Y);
            float R = -sqrt((varL + sqr(0.5 / 255.0)) / (varH + mVar.r + sqr(0.5 / 255.0)));

            float2 krnl = Kernel(float2(X, Y) - offset);
            float weight = krnl.r * krnl.g / (Luma(abs(c0.rgb - Lowres(X, Y).rgb)) + Lowres(X, Y).a + sqr(0.5 / 255.0));

            diff += weight * (SampleInputOff(0, float2(X, Y) / scale).rgb + Lowres(X, Y).rgb * R + (-1.0 - R) * (c0.rgb));
            weightSum += weight;
        }
    diff /= weightSum;

    c0.rgb = ((c0.rgb) + diff);

    return c0;
}