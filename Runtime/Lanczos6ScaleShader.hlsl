// Lanczos6 插值算法
// 移植自 https://github.com/libretro/common-shaders/blob/master/windowed/shaders/lanczos6.cg


cbuffer constants : register(b0) {
    int2 srcSize : packoffset(c0.x);
    int2 destSize : packoffset(c0.z);
    float ARStrength : packoffset(c1.x);	// 抗振铃强度。取值范围 0~1，越大抗振铃效果越好，但图像越模糊
};


#define D2D_INPUT_COUNT 1
#define D2D_INPUT0_COMPLEX
#define MAGPIE_USE_SAMPLE_INPUT
#include "common.hlsli"

#define FIX(c) max(abs(c), 1e-5)


float3 weight3(float x) {
    const float radius = 3.0;
    float3 sample = FIX(2.0 * PI * float3(x - 1.5, x - 0.5, x + 0.5));

    // Lanczos3. Note: we normalize outside this function, so no point in multiplying by radius.
    return /*radius **/ sin(sample) * sin(sample / radius) / (sample * sample);
}


float3 line_run(float ypos, float3 xpos1, float3 xpos2, float3 linetaps1, float3 linetaps2) {
    return SampleInputNoCheck(0, float2(xpos1.r, ypos)) * linetaps1.r
        + SampleInputNoCheck(0, float2(xpos1.g, ypos)) * linetaps2.r
        + SampleInputNoCheck(0, float2(xpos1.b, ypos)) * linetaps1.g
        + SampleInputNoCheck(0, float2(xpos2.r, ypos)) * linetaps2.g
        + SampleInputNoCheck(0, float2(xpos2.g, ypos)) * linetaps1.b
        + SampleInputNoCheck(0, float2(xpos2.b, ypos)) * linetaps2.b;
}

D2D_PS_ENTRY(main) {
    InitMagpieSampleInputWithScale(float2(destSize) / srcSize);

    // 用于抗振铃
    float3 neighbors[4] = {
        SampleInputOffCheckLeft(0, -1, 0),
        SampleInputOffCheckRight(0, 1, 0),
        SampleInputOffCheckTop(0, 0, -1),
        SampleInputOffCheckBottom(0, 0, 1)
    };

    float2 f = frac(coord.xy / coord.zw + 0.5);

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
    coord.xy -= (f + 2) * coord.zw;

    float3 xpos1 = float3(coord.x, GetCheckedRight(1), GetCheckedRight(2));
    float3 xpos2 = float3(GetCheckedRight(3), GetCheckedRight(4), GetCheckedRight(5));

    // final sum and weight normalization
    float3 color = line_run(coord.y, xpos1, xpos2, linetaps1, linetaps2) * columntaps1.r
        + line_run(GetCheckedBottom(1), xpos1, xpos2, linetaps1, linetaps2) * columntaps2.r
        + line_run(GetCheckedBottom(2), xpos1, xpos2, linetaps1, linetaps2) * columntaps1.g
        + line_run(GetCheckedBottom(3), xpos1, xpos2, linetaps1, linetaps2) * columntaps2.g
        + line_run(GetCheckedBottom(4), xpos1, xpos2, linetaps1, linetaps2) * columntaps1.b
        + line_run(GetCheckedBottom(5), xpos1, xpos2, linetaps1, linetaps2) * columntaps2.b;

    // 抗振铃
    float3 min_sample = min4(neighbors[0], neighbors[1], neighbors[2], neighbors[3]);
    float3 max_sample = max4(neighbors[0], neighbors[1], neighbors[2], neighbors[3]);
    color = lerp(color, clamp(color, min_sample, max_sample), ARStrength);

    return float4(color, 1);
}
