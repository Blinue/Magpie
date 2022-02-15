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


float3 FsrRcasF(uint2 pos) {
	// Algorithm uses minimal 3x3 pixel neighborhood.
	//    b 
	//  d e f
	//    h
	float3 b = INPUT.Load(int3(pos.x, pos.y - 1, 0)).rgb;
	float3 d = INPUT.Load(int3(pos.x - 1, pos.y, 0)).rgb;
	float3 e = INPUT.Load(int3(pos, 0)).rgb;
	float3 f = INPUT.Load(int3(pos.x + 1, pos.y, 0)).rgb;
	float3 h = INPUT.Load(int3(pos.x, pos.y + 1, 0)).rgb;
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


uint ABfe(uint src, uint off, uint bits) {
	uint mask = (1u << bits) - 1;
	return (src >> off) & mask;
}
uint ABfiM(uint src, uint ins, uint bits) {
	uint mask = (1u << bits) - 1;
	return (ins & mask) | (src & (~mask));
}
uint2 ARmp8x8(uint a) {
	return uint2(ABfe(a, 1u, 3u), ABfiM(ABfe(a, 3u, 3u), a, 1u));
}

[numthreads(64, 1, 1)]
void main(uint3 LocalThreadId : SV_GroupThreadID, uint3 WorkGroupId : SV_GroupID, uint3 Dtid : SV_DispatchThreadID) {
	uint2 gxy = ARmp8x8(LocalThreadId.x) + uint2(WorkGroupId.x << 4u, WorkGroupId.y << 4u);
	uint2 gxy_viewport = gxy + viewport.xy;

	OUTPUT[gxy_viewport] = float4(FsrRcasF(gxy), 1);
	gxy.x += 8u;
	gxy_viewport.x += 8u;

	if (gxy_viewport.x < viewport.z) {
		OUTPUT[gxy_viewport] = float4(FsrRcasF(gxy), 1);
	}
	
	gxy.y += 8u;
	gxy_viewport.y += 8u;

	if (gxy_viewport.x < viewport.z && gxy_viewport.y < viewport.w) {
		OUTPUT[gxy_viewport] = float4(FsrRcasF(gxy), 1);
	}

	gxy.x -= 8u;
	gxy_viewport.x -= 8u;

	if (gxy_viewport.y < viewport.w) {
		OUTPUT[gxy_viewport] = float4(FsrRcasF(gxy), 1);
	}
}
