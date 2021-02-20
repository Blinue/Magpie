#define D2D_INPUT_COUNT 1
#define D2D_INPUT0_COMPLEX
#include "d2d1effecthelpers.hlsli"
#include "common.hlsli"

#define FIX(c) max(abs(c), 1e-5)


cbuffer constants : register(b0) {
    int2 srcSize : packoffset(c0.x);
    int2 destSize : packoffset(c0.z);
};



float3 weight3(float x) {
    const float radius = 3.0;
    float3 sample = FIX(2.0 * PI * float3(x - 1.5, x - 0.5, x + 0.5));

    // Lanczos3. Note: we normalize outside this function, so no point in multiplying by radius.
    return /*radius **/ sin(sample) * sin(sample / radius) / (sample * sample);
}

float3 pixel(float xpos, float ypos) {
    float4 coord = D2DGetInputCoordinate(0);
    return D2DSampleInput(0, float2(xpos * srcSize.x * coord.z, ypos * srcSize.y * coord.w)).rgb;
}

float3 line_run(float ypos, float3 xpos1, float3 xpos2, float3 linetaps1, float3 linetaps2) {
    return
        pixel(xpos1.r, ypos) * linetaps1.r +
        pixel(xpos1.g, ypos) * linetaps2.r +
        pixel(xpos1.b, ypos) * linetaps1.g +
        pixel(xpos2.r, ypos) * linetaps2.g +
        pixel(xpos2.g, ypos) * linetaps1.b +
        pixel(xpos2.b, ypos) * linetaps2.b;
}

D2D_PS_ENTRY(main) {
    float4 coord = D2DGetInputCoordinate(0);

    float2 texCoord = coord.xy / coord.zw / destSize;

    float2 stepxy = 1.0 / destSize;
    float2 pos = texCoord + stepxy * 0.5;
    float2 f = frac(pos / stepxy);

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

    float2 xystart = (-2.5 - f) * stepxy + pos;
    float3 xpos1 = float3(xystart.x, xystart.x + stepxy.x, xystart.x + stepxy.x * 2.0);
    float3 xpos2 = float3(xystart.x + stepxy.x * 3.0, xystart.x + stepxy.x * 4.0, xystart.x + stepxy.x * 5.0);

    // final sum and weight normalization
    return float4(
        line_run(xystart.y, xpos1, xpos2, linetaps1, linetaps2) * columntaps1.r +
        line_run(xystart.y + stepxy.y, xpos1, xpos2, linetaps1, linetaps2) * columntaps2.r +
        line_run(xystart.y + stepxy.y * 2.0, xpos1, xpos2, linetaps1, linetaps2) * columntaps1.g +
        line_run(xystart.y + stepxy.y * 3.0, xpos1, xpos2, linetaps1, linetaps2) * columntaps2.g +
        line_run(xystart.y + stepxy.y * 4.0, xpos1, xpos2, linetaps1, linetaps2) * columntaps1.b +
        line_run(xystart.y + stepxy.y * 5.0, xpos1, xpos2, linetaps1, linetaps2) * columntaps2.b, 1.0);
}
