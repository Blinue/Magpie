// Lanczos6 插值算法
// 移植自 https://github.com/libretro/common-shaders/blob/master/windowed/shaders/lanczos6.cg


cbuffer constants : register(b0) {
    int2 srcSize : packoffset(c0.x);
    int2 destSize : packoffset(c0.z);
    float ARStrength : packoffset(c1.x);	// 抗振铃强度。取值范围 0~1，越大抗振铃效果越好，但图像越模糊
};


#define MAGPIE_INPUT_COUNT 1
#include "common.hlsli"

#define FIX(c) max(abs(c), 1e-5)


float3 weight3(float x) {
    const float radius = 3.0;
    float3 sample = FIX(2.0 * PI * float3(x - 1.5, x - 0.5, x + 0.5));

    // Lanczos3. Note: we normalize outside this function, so no point in multiplying by radius.
    return /*radius **/ sin(sample) * sin(sample / radius) / (sample * sample);
}


float3 line_run(float ypos, float3 xpos1, float3 xpos2, float3 linetaps1, float3 linetaps2) {
    return SampleInput(0, float2(xpos1.r, ypos)).rgb * linetaps1.r
        + SampleInput(0, float2(xpos1.g, ypos)).rgb * linetaps2.r
        + SampleInput(0, float2(xpos1.b, ypos)).rgb * linetaps1.g
        + SampleInput(0, float2(xpos2.r, ypos)).rgb * linetaps2.g
        + SampleInput(0, float2(xpos2.g, ypos)).rgb * linetaps1.b
        + SampleInput(0, float2(xpos2.b, ypos)).rgb * linetaps2.b;
}

D2D_PS_ENTRY(main) {
    InitMagpieSampleInputWithScale(float2(destSize) / srcSize);

    // 用于抗振铃
    float3 neighbors[4] = {
        SampleInputOffChecked(0, float2(-1, 0)).rgb,
        SampleInputOffChecked(0, float2(1, 0)).rgb,
        SampleInputOffChecked(0, float2(0, -1)).rgb,
        SampleInputOffChecked(0, float2(0, 1)).rgb
    };

    float2 f = frac(Coord(0).xy / Coord(0).zw + 0.5);

    float3 linetaps1 = weight3(0.5 - f.x * 0.5);
    float3 linetaps2 = weight3(1.0 - f.x * 0.5);
    float3 columntaps1 = weight3(0.5 - f.y * 0.5);
    float3 columntaps2 = weight3(1.0 - f.y * 0.5);

    // make sure all taps added together is exactly 1.0, otherwise some
    // (very small) distortion can occur
    float suml = dot(linetaps1, float3(1, 1, 1)) + dot(linetaps2, float3(1, 1, 1));
    float sumc = dot(columntaps1, float3(1, 1, 1)) + dot(columntaps2, float3(1, 1, 1));
    linetaps1 /= suml;
    linetaps2 /= suml;
    columntaps1 /= sumc;
    columntaps2 /= sumc;

    // !!!改变当前坐标
    Coord(0).xy -= (f + 2) * Coord(0).zw;

    float3 xpos1 = float3(Coord(0).x, min(Coord(0).x + Coord(0).z, maxCoord0.x), min(Coord(0).x + 2 * Coord(0).z, maxCoord0.x));
    float3 xpos2 = float3(min(Coord(0).x + 3 * Coord(0).z, maxCoord0.x), min(Coord(0).x + 4 * Coord(0).z, maxCoord0.x), min(Coord(0).x + 5 * Coord(0).z, maxCoord0.x));

    // final sum and weight normalization
    float3 color = line_run(Coord(0).y, xpos1, xpos2, linetaps1, linetaps2) * columntaps1.r
        + line_run(min(Coord(0).y + Coord(0).w, maxCoord0.y), xpos1, xpos2, linetaps1, linetaps2) * columntaps2.r
        + line_run(min(Coord(0).y + 2 * Coord(0).w, maxCoord0.y), xpos1, xpos2, linetaps1, linetaps2) * columntaps1.g
        + line_run(min(Coord(0).y + 3 * Coord(0).w, maxCoord0.y), xpos1, xpos2, linetaps1, linetaps2) * columntaps2.g
        + line_run(min(Coord(0).y + 4 * Coord(0).w, maxCoord0.y), xpos1, xpos2, linetaps1, linetaps2) * columntaps1.b
        + line_run(min(Coord(0).y + 5 * Coord(0).w, maxCoord0.y), xpos1, xpos2, linetaps1, linetaps2) * columntaps2.b;

    // 抗振铃
    float3 min_sample = min4(neighbors[0], neighbors[1], neighbors[2], neighbors[3]);
    float3 max_sample = max4(neighbors[0], neighbors[1], neighbors[2], neighbors[3]);
    color = lerp(color, clamp(color, min_sample, max_sample), ARStrength);

    return float4(color, 1);
}
