// FidelityFX-FSR 中 RCAS 通道
// 移植自 https://github.com/GPUOpen-Effects/FidelityFX-FSR/blob/a21ffb8f6c13233ba336352bdff293894c706575/ffx-fsr/ffx_fsr1.h

//!MAGPIE EFFECT
//!VERSION 4
//!CAPABILITY FP16

#include "../StubDefs.hlsli"

//!PARAMETER
//!LABEL Sharpness
//!DEFAULT 0.87
//!MIN 0
//!MAX 1
//!STEP 0.01
float sharpness;

//!TEXTURE
Texture2D INPUT;

//!TEXTURE
//!WIDTH INPUT_WIDTH
//!HEIGHT INPUT_HEIGHT
Texture2D OUTPUT;

//!SAMPLER
//!FILTER POINT
SamplerState sam;


//!PASS 1
//!IN INPUT
//!OUT OUTPUT
//!BLOCK_SIZE 16
//!NUM_THREADS 64

#define min3(a, b, c) min(a, min(b, c))
#define max3(a, b, c) max(a, max(b, c))

// This is set at the limit of providing unnatural results for sharpening.
#define FSR_RCAS_LIMIT (0.25-(1.0/16.0))

#ifdef MP_FP16

void FsrRcasHx2(
	out MF2 pixR,
	out MF2 pixG,
	out MF2 pixB,
	float3 b0, float3 d0, float3 e0, float3 f0, float3 h0,
	float3 b1, float3 d1, float3 e1, float3 f1, float3 h1,
	MF s
) {
	// Arrays of Structures to Structures of Arrays conversion.
	MF2 bR = MF2(b0.r, b1.r);
	MF2 bG = MF2(b0.g, b1.g);
	MF2 bB = MF2(b0.b, b1.b);
	MF2 dR = MF2(d0.r, d1.r);
	MF2 dG = MF2(d0.g, d1.g);
	MF2 dB = MF2(d0.b, d1.b);
	MF2 eR = MF2(e0.r, e1.r);
	MF2 eG = MF2(e0.g, e1.g);
	MF2 eB = MF2(e0.b, e1.b);
	MF2 fR = MF2(f0.r, f1.r);
	MF2 fG = MF2(f0.g, f1.g);
	MF2 fB = MF2(f0.b, f1.b);
	MF2 hR = MF2(h0.r, h1.r);
	MF2 hG = MF2(h0.g, h1.g);
	MF2 hB = MF2(h0.b, h1.b);
	// Luma times 2.
	MF2 bL = bB * 0.5 + (bR * 0.5 + bG);
	MF2 dL = dB * 0.5 + (dR * 0.5 + dG);
	MF2 eL = eB * 0.5 + (eR * 0.5 + eG);
	MF2 fL = fB * 0.5 + (fR * 0.5 + fG);
	MF2 hL = hB * 0.5 + (hR * 0.5 + hG);
	// Noise detection.
	MF2 nz = 0.25 * bL + 0.25 * dL + 0.25 * fL + 0.25 * hL - eL;
	nz = saturate(abs(nz) * rcp(max3(max3(bL, dL, eL), fL, hL) - min3(min3(bL, dL, eL), fL, hL)));
	nz = -0.5 * nz + 1.0;
	// Min and max of ring.
	MF2 mn4R = min(min3(bR, dR, fR), hR);
	MF2 mn4G = min(min3(bG, dG, fG), hG);
	MF2 mn4B = min(min3(bB, dB, fB), hB);
	MF2 mx4R = max(min3(bR, dR, fR), hR);
	MF2 mx4G = max(min3(bG, dG, fG), hG);
	MF2 mx4B = max(min3(bB, dB, fB), hB);
	// Immediate constants for peak range.
	MF2 peakC = MF2(1.0, -1.0 * 4.0);
	// Limiters, these need to be high precision RCPs.
	MF2 hitMinR = min(mn4R, eR) * rcp(4.0 * mx4R);
	MF2 hitMinG = min(mn4G, eG) * rcp(4.0 * mx4G);
	MF2 hitMinB = min(mn4B, eB) * rcp(4.0 * mx4B);
	MF2 hitMaxR = (peakC.x - max(mx4R, eR)) * rcp(4.0 * mn4R + peakC.y);
	MF2 hitMaxG = (peakC.x - max(mx4G, eG)) * rcp(4.0 * mn4G + peakC.y);
	MF2 hitMaxB = (peakC.x - max(mx4B, eB)) * rcp(4.0 * mn4B + peakC.y);
	MF2 lobeR = max(-hitMinR, hitMaxR);
	MF2 lobeG = max(-hitMinG, hitMaxG);
	MF2 lobeB = max(-hitMinB, hitMaxB);
	MF2 lobe = max(-FSR_RCAS_LIMIT, min(max3(lobeR, lobeG, lobeB), 0.0)) * s;
	// Apply noise removal.
	lobe*=nz;
	// Resolve, which needs the medium precision rcp approximation to avoid visible tonality changes.
	MF2 rcpL = rcp(4.0 * lobe + 1.0);
	pixR = (lobe * bR + lobe * dR + lobe * hR + lobe * fR + eR) * rcpL;
	pixG = (lobe * bG + lobe * dG + lobe * hG + lobe * fG + eG) * rcpL;
	pixB = (lobe * bB + lobe * dB + lobe * hB + lobe * fB + eB) * rcpL;
}

#else

float3 FsrRcasF(float3 b, float3 d, float3 e, float3 f, float3 h) {
	// Algorithm uses minimal 3x3 pixel neighborhood.
	//    b 
	//  d e f
	//    h

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

	float nz;

	// Luma times 2.
	float bL = bB * 0.5 + (bR * 0.5 + bG);
	float dL = dB * 0.5 + (dR * 0.5 + dG);
	float eL = eB * 0.5 + (eR * 0.5 + eG);
	float fL = fB * 0.5 + (fR * 0.5 + fG);
	float hL = hB * 0.5 + (hR * 0.5 + hG);

	// Noise detection.
	nz = 0.25 * bL + 0.25 * dL + 0.25 * fL + 0.25 * hL - eL;
	nz = saturate(abs(nz) * rcp(max3(max3(bL, dL, eL), fL, hL) - min3(min3(bL, dL, eL), fL, hL)));
	nz = -0.5 * nz + 1.0;

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
	float hitMinR = min(mn4R, eR) * rcp(4.0 * mx4R);
	float hitMinG = min(mn4G, eG) * rcp(4.0 * mx4G);
	float hitMinB = min(mn4B, eB) * rcp(4.0 * mx4B);
	float hitMaxR = (peakC.x - max(mx4R, eR)) * rcp(4.0 * mn4R + peakC.y);
	float hitMaxG = (peakC.x - max(mx4G, eG)) * rcp(4.0 * mn4G + peakC.y);
	float hitMaxB = (peakC.x - max(mx4B, eB)) * rcp(4.0 * mn4B + peakC.y);
	float lobeR = max(-hitMinR, hitMaxR);
	float lobeG = max(-hitMinG, hitMaxG);
	float lobeB = max(-hitMinB, hitMaxB);
	float lobe = max(-FSR_RCAS_LIMIT, min(max3(lobeR, lobeG, lobeB), 0)) * sharpness;

	// Apply noise removal.
	lobe *= nz;

	// Resolve, which needs the medium precision rcp approximation to avoid visible tonality changes.
	float rcpL = rcp(4.0 * lobe + 1.0);
	float3 c = {
		(lobe * bR + lobe * dR + lobe * hR + lobe * fR + eR) * rcpL,
		(lobe * bG + lobe * dG + lobe * hG + lobe * fG + eG) * rcpL,
		(lobe * bB + lobe * dB + lobe * hB + lobe * fB + eB) * rcpL
	};

	return c;
}

#endif

void Pass1(uint2 blockStart, uint3 threadId) {
	uint2 gxy = blockStart + (Rmp8x8(threadId.x) << 1);

	const uint2 outputSize = GetOutputSize();
	if (gxy.x >= outputSize.x || gxy.y >= outputSize.y) {
		return;
	}

	float3 src[4][4];
	[unroll]
	for (uint i = 1; i < 3; ++i) {
		[unroll]
		for (uint j = 0; j < 4; ++j) {
			src[i][j] = INPUT.Load(int3(gxy.x + i - 1, gxy.y + j - 1, 0)).rgb;
		}
	}

	src[0][1] = INPUT.Load(int3(gxy.x - 1, gxy.y, 0)).rgb;
	src[0][2] = INPUT.Load(int3(gxy.x - 1, gxy.y + 1, 0)).rgb;
	src[3][1] = INPUT.Load(int3(gxy.x + 2, gxy.y, 0)).rgb;
	src[3][2] = INPUT.Load(int3(gxy.x + 2, gxy.y + 1, 0)).rgb;

#ifdef MP_FP16
	MF2 pixR, pixG, pixB;
	const MF s = (MF)sharpness;
	FsrRcasHx2(pixR, pixG, pixB, src[1][0], src[0][1], src[1][1], src[2][1], src[1][2], src[2][0], src[1][1], src[2][1], src[3][1], src[2][2], s);
	OUTPUT[gxy] = MF4(pixR.x, pixG.x, pixB.x, 1);
	++gxy.x;
	OUTPUT[gxy] = MF4(pixR.y, pixG.y, pixB.y, 1);
	
	FsrRcasHx2(pixR, pixG, pixB, src[2][1], src[1][2], src[2][2], src[3][2], src[2][3], src[1][1], src[0][2], src[1][2], src[2][2], src[1][3], s);
	++gxy.y;
	OUTPUT[gxy] = MF4(pixR.x, pixG.x, pixB.x, 1);
	--gxy.x;
	OUTPUT[gxy] = MF4(pixR.y, pixG.y, pixB.y, 1);
#else
	OUTPUT[gxy] = float4(FsrRcasF(src[1][0], src[0][1], src[1][1], src[2][1], src[1][2]), 1);

	++gxy.x;
	if (gxy.x < outputSize.x && gxy.y < outputSize.y) {
		OUTPUT[gxy] = float4(FsrRcasF(src[2][0], src[1][1], src[2][1], src[3][1], src[2][2]), 1);
	}

	++gxy.y;
	if (gxy.x < outputSize.x && gxy.y < outputSize.y) {
		OUTPUT[gxy] = float4(FsrRcasF(src[2][1], src[1][2], src[2][2], src[3][2], src[2][3]), 1);
	}

	--gxy.x;
	if (gxy.x < outputSize.x && gxy.y < outputSize.y) {
		OUTPUT[gxy] = float4(FsrRcasF(src[1][1], src[0][2], src[1][2], src[2][2], src[1][3]), 1);
	}
#endif
}
