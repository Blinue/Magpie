/*cbuffer cb : register(b0) {
	uint4 __off;
	uint __vx;
	uint __vy;

	float sharpness;
};

cbuffer cb1 : register(b1) {
	int4 __cursorRect;
	float2 __cursorPos;
	uint __cursorType;	// 0: Color, 1: Masked Color, 2: Monochrome
	uint __frameCount;
}

Texture2D INPUT : register(t0);
Texture2D CURSOR : register(t1);
RWTexture2D<float4> OUTPUT : register(u0);

SamplerState sam : register(s0);*/

//!MAGPIE EFFECT
//!VERSION 2
//!OUTPUT_WIDTH INPUT_WIDTH * 2
//!OUTPUT_HEIGHT INPUT_HEIGHT * 2


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
//!BLOCK_SIZE 16, 16
//!NUM_THREADS 64, 1, 1

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

/*
void __WriteToOutput(uint2 pos, float3 color) {
	pos += __off.zw;

	if ((int)pos.x >= __cr.x && (int)pos.y >= __cr.y && (int)pos.x < __cr.z && (int)pos.y < __cr.w) {
		// »æÖÆ¹â±ê
		float4 mask = CURSOR.SampleLevel(sam, (pos - __cr.xy + 0.5f) * __cp, 0);

		if (__ct == 0) {
			color = color * mask.a + mask.rgb;
		} else if (__ct == 1) {
			if (mask.a < 0.5f) {
				color = mask.rgb;
			} else {
				color = (uint3(round(color * 255)) ^ uint3(mask.rgb * 255)) / 255.0f;
			}
		} else {
			if (mask.x > 0.5f) {
				if (mask.y > 0.5f) {
					color = 1 - color;
				}
			} else {
				if (mask.y > 0.5f) {
					color = float3(1, 1, 1);
				} else {
					color = float3(0, 0, 0);
				}
			}
		}
	}

	OUTPUT[pos] = float4(color, 1);
}

#define WriteToOutput(pos, color) if(pos.x < __vx && pos.y < __vy)__WriteToOutput(pos,color)

*/

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


void Pass1(uint2 blockStart, uint3 threadId) {
	uint2 gxy = blockStart + ARmp8x8(threadId.x);

	WriteToOutput(gxy, FsrRcasF(gxy));

	gxy.x += 8u;
	WriteToOutput(gxy, FsrRcasF(gxy));

	gxy.y += 8u;
	WriteToOutput(gxy, FsrRcasF(gxy));

	gxy.x -= 8u;
	WriteToOutput(gxy, FsrRcasF(gxy));
}
/*
[numthreads(64, 1, 1)]
void main(uint3 LocalThreadId : SV_GroupThreadID, uint3 WorkGroupId : SV_GroupID, uint3 Dtid : SV_DispatchThreadID) {
	Pass2(uint2(WorkGroupId.x << 4u, WorkGroupId.y << 4u) + __off.xy, LocalThreadId);
}*/
