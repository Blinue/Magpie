// FidelityFX-FSR 中 RCAS 通道
// 移植自 https://github.com/GPUOpen-Effects/FidelityFX-FSR/blob/master/ffx-fsr/ffx_fsr1.h

//!MAGPIE EFFECT
//!VERSION 2
//!OUTPUT_WIDTH INPUT_WIDTH
//!OUTPUT_HEIGHT INPUT_HEIGHT


//!PARAMETER
//!DEFAULT 0.87
//!MIN 1e-5
float sharpness;

//!TEXTURE
Texture2D INPUT;

//!SAMPLER
//!FILTER POINT
SamplerState sam;

//!PASS 1
//!IN INPUT
//!BLOCK_SIZE 16
//!NUM_THREADS 64

#define min3(a, b, c) min(a, min(b, c))
#define max3(a, b, c) max(a, max(b, c))

// This is set at the limit of providing unnatural results for sharpening.
#define FSR_RCAS_LIMIT (0.25-(1.0/16.0))


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

void Pass1(uint2 blockStart, uint3 threadId) {
	uint2 gxy = blockStart + (Rmp8x8(threadId.x) << 1);
	if (!CheckViewport(gxy)) {
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

	WriteToOutput(gxy, FsrRcasF(src[1][0], src[0][1], src[1][1], src[2][1], src[1][2]));

	++gxy.x;
	if (CheckViewport(gxy)) {
		WriteToOutput(gxy, FsrRcasF(src[2][0], src[1][1], src[2][1], src[3][1], src[2][2]));
	}

	++gxy.y;
	if (CheckViewport(gxy)) {
		WriteToOutput(gxy, FsrRcasF(src[2][1], src[1][2], src[2][2], src[3][2], src[2][3]));
	}

	--gxy.x;
	if (CheckViewport(gxy)) {
		WriteToOutput(gxy, FsrRcasF(src[1][1], src[0][2], src[1][2], src[2][2], src[1][3]));
	}
}
