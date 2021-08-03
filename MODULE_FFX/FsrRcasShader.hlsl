cbuffer constants : register(b0) {
    uint2 srcSize : packoffset(c0.x);
    float sharpness : packoffset(c0.z);
};

#define MAGPIE_INPUT_COUNT 1
#include "common.hlsli"


// This is set at the limit of providing unnatural results for sharpening.
#define FSR_RCAS_LIMIT (0.25-(1.0/16.0))


MAGPIE_ENTRY(main) {
    InitMagpieSampleInput();

    int2 sp = floor(Coord(0).xy / Coord(0).zw);
    int2 leftTop= max(0, sp + int2(-1, -1));
    int2 rightBottom = min(srcSize - 1, sp + int2(1, 1));

    // Algorithm uses minimal 3x3 pixel neighborhood.
    //    b 
    //  d e f
    //    h
    float3 b = LoadInput(0, int2(sp.x, leftTop.y)).rgb;
    float3 d = LoadInput(0, int2(leftTop.x, sp.y)).rgb;
    float3 e = LoadInput(0, sp).rgb;
    float3 f = LoadInput(0, int2(rightBottom.x, sp.y)).rgb;
    float3 h = LoadInput(0, int2(sp.x, rightBottom.y)).rgb;
    // Rename (32-bit) or regroup (16-bit).
    float bR = b.r;
    float bG = b.g;
    float bB = b.b;
    float dR = d.r;
    float dG = d.g;
    float dB = d.b;
    float eR = e.r;
    float eG = e.g;
    float eB = e.b;
    float fR = f.r;
    float fG = f.g;
    float fB = f.b;
    float hR = h.r;
    float hG = h.g;
    float hB = h.b;
	
// #ifdef FSR_RCAS_DENOISE
    // Luma times 2.
    float bL = bB * 0.5 + (bR * 0.5 + bG);
    float dL = dB * 0.5 + (dR * 0.5 + dG);
    float eL = eB * 0.5 + (eR * 0.5 + eG);
    float fL = fB * 0.5 + (fR * 0.5 + fG);
    float hL = hB * 0.5 + (hR * 0.5 + hG);
    
    // Noise detection.
    float nz = 0.25 * bL + 0.25 * dL + 0.25 * fL + 0.25 * hL - eL;
    nz = saturate(abs(nz) * rcp(max3(max3(bL, dL, eL), fL, hL) - min3(min3(bL, dL, eL), fL, hL)));
    nz = -0.5 * nz + 1.0;
// #endif
	
    // Min and max of ring.
    float mn4R = min(min3(bR, dR, fR), hR);
    float mn4G = min(min3(bG, dG, fG), hG);
    float mn4B = min(min3(bB, dB, fB), hB);
    float mx4R = max(max3(bR, dR, fR), hR);
    float mx4G = max(max3(bG, dG, fG), hG);
    float mx4B = max(max3(bB, dB, fB), hB);
    // Immediate constants for peak range.
    float2 peakC = { 1.0, -1.0 * 4.0 };
    // Limiters, these need to be high precision RCPs.
    float hitMinR = mn4R * rcp(4.0 * mx4R);
    float hitMinG = mn4G * rcp(4.0 * mx4G);
    float hitMinB = mn4B * rcp(4.0 * mx4B);
    float hitMaxR = (peakC.x - mx4R) * rcp(4.0 * mn4R + peakC.y);
    float hitMaxG = (peakC.x - mx4G) * rcp(4.0 * mn4G + peakC.y);
    float hitMaxB = (peakC.x - mx4B) * rcp(4.0 * mn4B + peakC.y);
    float lobeR = max(-hitMinR, hitMaxR);
    float lobeG = max(-hitMinG, hitMaxG);
    float lobeB = max(-hitMinB, hitMaxB);
    float lobe = max(-FSR_RCAS_LIMIT, min(max3(lobeR, lobeG, lobeB), 0)) * exp2(-sharpness);
	
    // Apply noise removal.
// #ifdef FSR_RCAS_DENOISE
    lobe *= nz;
// #endif
	
    // Resolve, which needs the medium precision rcp approximation to avoid visible tonality changes.
    float rcpL = rcp(4.0 * lobe + 1.0);
    float3 c = {
        (lobe * bR + lobe * dR + lobe * hR + lobe * fR + eR)* rcpL,
        (lobe * bG + lobe * dG + lobe * hG + lobe * fG + eG)* rcpL,
        (lobe * bB + lobe * dB + lobe * hB + lobe * fB + eB)* rcpL
    };

    return float4(c, 1.0f);
}