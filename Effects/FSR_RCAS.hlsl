cbuffer cb : register(b0) {
	float inputWidth;
	float inputHeight;
	float sharpness;

	uint4 viewport;
};

SamplerState sam : register(s0);

Texture2D INPUT : register(t0);
RWTexture2D<float4> OUTPUT : register(u0);


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


#define THREAD_GROUP_SIZE 128
#define BLOCK_WIDTH 32
#define BLOCK_HEIGHT 32
#define SH_PIXELS_WIDTH (BLOCK_WIDTH + 2)
#define SH_PIXELS_HEIGHT (BLOCK_HEIGHT + 2)
groupshared float3 shPixels[SH_PIXELS_HEIGHT][SH_PIXELS_WIDTH];


[numthreads(THREAD_GROUP_SIZE, 1, 1)]
void main(uint3 LocalThreadId : SV_GroupThreadID, uint3 WorkGroupId : SV_GroupID, uint3 Dtid : SV_DispatchThreadID) {
	const uint dstBlockX = BLOCK_WIDTH * WorkGroupId.x;
	const uint dstBlockY = BLOCK_HEIGHT * WorkGroupId.y;

	for (uint i = LocalThreadId.x * 2; i < SH_PIXELS_WIDTH * SH_PIXELS_HEIGHT / 2; i += THREAD_GROUP_SIZE * 2) {
		const uint2 pos = uint2(i % SH_PIXELS_WIDTH, i / SH_PIXELS_WIDTH * 2);

		const float tx = (dstBlockX + pos.x) / inputWidth;
		const float ty = (dstBlockY + pos.y) / inputHeight;

		const float4 sr = INPUT.GatherRed(sam, float2(tx, ty));
		const float4 sg = INPUT.GatherGreen(sam, float2(tx, ty));
		const float4 sb = INPUT.GatherBlue(sam, float2(tx, ty));

		shPixels[pos.y][pos.x] = float3(sr.w, sg.w, sb.w);
		shPixels[pos.y][pos.x + 1] = float3(sr.z, sg.z, sb.z);
		shPixels[pos.y + 1][pos.x] = float3(sr.x, sg.x, sb.x);
		shPixels[pos.y + 1][pos.x + 1] = float3(sr.y, sg.y, sb.y);
	}

	GroupMemoryBarrierWithGroupSync();

	for (i = LocalThreadId.x; i < BLOCK_WIDTH * BLOCK_HEIGHT; i += THREAD_GROUP_SIZE) {
		const uint2 pos = uint2(i % BLOCK_WIDTH, i / BLOCK_WIDTH);

		const uint2 outputPos = uint2(dstBlockX, dstBlockY) + pos + viewport.xy;
		if (outputPos.x >= viewport.z || outputPos.y >= viewport.w) {
			continue;
		}

		float3 b = shPixels[pos.y][pos.x + 1];
		float3 d = shPixels[pos.y + 1][pos.x];
		float3 e = shPixels[pos.y + 1][pos.x + 1];
		float3 f = shPixels[pos.y + 1][pos.x + 2];
		float3 h = shPixels[pos.y + 2][pos.x + 1];
		OUTPUT[outputPos] = float4(FsrRcasF(b, d, e, f, h), 1);
	}
}
