// CRT-Lottes
// 移植自 https://github.com/libretro/common-shaders/blob/master/crt/shaders/crt-lottes.cg


// PUBLIC DOMAIN CRT STYLED SCAN-LINE SHADER
//
//   by Timothy Lottes
//
// This is more along the style of a really good CGA arcade monitor.
// With RGB inputs instead of NTSC.
// The shadow mask example has the mask rotated 90 degrees for less chromatic aberration.
//
// Left it unoptimized to show the theory behind the algorithm.
//
// It is an example what I personally would want as a display option for pixel art games.
// Please take and use, change, or whatever.


//!MAGPIE EFFECT
//!VERSION 2

//!PARAMETER
//!DEFAULT -8
//!MIN -20
//!MAX 0
int hardScan;

//!PARAMETER
//!DEFAULT -3
//!MIN -20
//!MAX 0
int hardPix;

//!PARAMETER
//!DEFAULT 0.031
//!MIN 0
//!MAX 0.125
float warpX;

//!PARAMETER
//!DEFAULT 0.041
//!MIN 0
//!MAX 0.125
float warpY;

//!PARAMETER
//!DEFAULT 0.5
//!MIN 0
//!MAX 2
float maskDark;

//!PARAMETER
//!DEFAULT 1.5
//!MIN 0
//!MAX 2
float maskLight;

//!PARAMETER
//!DEFAULT 3
//!MIN 0
//!MAX 4
int shadowMask;

//!PARAMETER
//!DEFAULT 1
//!MIN 0
//!MAX 2
float brightBoost;

//!PARAMETER
//!DEFAULT -1.5
//!MIN -2
//!MAX -0.5
float hardBloomPix;

//!PARAMETER
//!DEFAULT -2
//!MIN -4
//!MAX -1
float hardBloomScan;

//!PARAMETER
//!DEFAULT 0.15
//!MIN 0
//!MAX 1
float bloomAmount;

//!PARAMETER
//!DEFAULT 2
//!MIN 0
//!MAX 10
float shape;


//!TEXTURE
Texture2D INPUT;

//!SAMPLER
//!FILTER POINT
SamplerState sam;


//!PASS 1
//!IN INPUT
//!BLOCK_SIZE 8
//!NUM_THREADS 64

#pragma warning(disable: 3571) // X3571: pow(f, e) will not work for negative f, use abs(f) or conditionally handle negative values if you expect them

#define DO_BLOOM 1
#define warp float2(warpX, warpY)


// Nearest emulated sample given floating point position and texel offset.
// Also zero's off screen.
float3 Fetch(float2 pos, float2 off, float2 texture_size) {
	pos = (floor(pos * texture_size.xy + off) + float2(0.5, 0.5)) / texture_size.xy;
	return brightBoost * pow(INPUT.SampleLevel(sam, pos, 0).rgb, 2.2f);
}

// Distance in emulated pixels to nearest texel.
float2 Dist(float2 pos, float2 texture_size) { pos = pos * texture_size.xy; return -((pos - floor(pos)) - float2(0.5, 0.5)); }

// 1D Gaussian.
float Gaus(float pos, float scale) { return exp2(scale * pow(abs(pos), shape)); }

// 3-tap Gaussian filter along horz line.
float3 Horz3(float2 pos, float off, float2 texture_size) {
	float3 b = Fetch(pos, float2(-1.0, off), texture_size);
	float3 c = Fetch(pos, float2(0.0, off), texture_size);
	float3 d = Fetch(pos, float2(1.0, off), texture_size);
	float dst = Dist(pos, texture_size).x;
	// Convert distance to weight.
	float scale = hardPix;
	float wb = Gaus(dst - 1.0, scale);
	float wc = Gaus(dst + 0.0, scale);
	float wd = Gaus(dst + 1.0, scale);
	// Return filtered sample.
	return (b * wb + c * wc + d * wd) / (wb + wc + wd);
}

// 5-tap Gaussian filter along horz line.
float3 Horz5(float2 pos, float off, float2 texture_size) {
	float3 a = Fetch(pos, float2(-2.0, off), texture_size);
	float3 b = Fetch(pos, float2(-1.0, off), texture_size);
	float3 c = Fetch(pos, float2(0.0, off), texture_size);
	float3 d = Fetch(pos, float2(1.0, off), texture_size);
	float3 e = Fetch(pos, float2(2.0, off), texture_size);
	float dst = Dist(pos, texture_size).x;
	// Convert distance to weight.
	float scale = hardPix;
	float wa = Gaus(dst - 2.0, scale);
	float wb = Gaus(dst - 1.0, scale);
	float wc = Gaus(dst + 0.0, scale);
	float wd = Gaus(dst + 1.0, scale);
	float we = Gaus(dst + 2.0, scale);
	// Return filtered sample.
	return (a * wa + b * wb + c * wc + d * wd + e * we) / (wa + wb + wc + wd + we);
}

// 7-tap Gaussian filter along horz line.
float3 Horz7(float2 pos, float off, float2 texture_size) {
	float3 a = Fetch(pos, float2(-3.0, off), texture_size);
	float3 b = Fetch(pos, float2(-2.0, off), texture_size);
	float3 c = Fetch(pos, float2(-1.0, off), texture_size);
	float3 d = Fetch(pos, float2(0.0, off), texture_size);
	float3 e = Fetch(pos, float2(1.0, off), texture_size);
	float3 f = Fetch(pos, float2(2.0, off), texture_size);
	float3 g = Fetch(pos, float2(3.0, off), texture_size);
	float dst = Dist(pos, texture_size).x;
	// Convert distance to weight.
	float scale = hardBloomPix;
	float wa = Gaus(dst - 3.0, scale);
	float wb = Gaus(dst - 2.0, scale);
	float wc = Gaus(dst - 1.0, scale);
	float wd = Gaus(dst + 0.0, scale);
	float we = Gaus(dst + 1.0, scale);
	float wf = Gaus(dst + 2.0, scale);
	float wg = Gaus(dst + 3.0, scale);
	// Return filtered sample.
	return (a * wa + b * wb + c * wc + d * wd + e * we + f * wf + g * wg) / (wa + wb + wc + wd + we + wf + wg);
}

// Return scanline weight.
float Scan(float2 pos, float off, float2 texture_size) {
	float dst = Dist(pos, texture_size).y;
	return Gaus(dst + off, hardScan);
}

// Return scanline weight for bloom.
float BloomScan(float2 pos, float off, float2 texture_size) {
	float dst = Dist(pos, texture_size).y;
	return Gaus(dst + off, hardBloomScan);
}

// Allow nearest three lines to effect pixel.
float3 Tri(float2 pos, float2 texture_size) {
	float3 a = Horz3(pos, -1.0, texture_size);
	float3 b = Horz5(pos, 0.0, texture_size);
	float3 c = Horz3(pos, 1.0, texture_size);
	float wa = Scan(pos, -1.0, texture_size);
	float wb = Scan(pos, 0.0, texture_size);
	float wc = Scan(pos, 1.0, texture_size);
	return a * wa + b * wb + c * wc;
}

// Small bloom.
float3 Bloom(float2 pos, float2 texture_size) {
	float3 a = Horz5(pos, -2.0, texture_size);
	float3 b = Horz7(pos, -1.0, texture_size);
	float3 c = Horz7(pos, 0.0, texture_size);
	float3 d = Horz7(pos, 1.0, texture_size);
	float3 e = Horz5(pos, 2.0, texture_size);
	float wa = BloomScan(pos, -2.0, texture_size);
	float wb = BloomScan(pos, -1.0, texture_size);
	float wc = BloomScan(pos, 0.0, texture_size);
	float wd = BloomScan(pos, 1.0, texture_size);
	float we = BloomScan(pos, 2.0, texture_size);
	return a * wa + b * wb + c * wc + d * wd + e * we;
}

// Distortion of scanlines, and end of screen alpha.
float2 Warp(float2 pos) {
	pos = pos * 2.0 - 1.0;
	pos *= float2(1.0 + (pos.y * pos.y) * warp.x, 1.0 + (pos.x * pos.x) * warp.y);
	return pos * 0.5 + 0.5;
}

// Shadow mask 
float3 Mask(float2 pos) {
	float3 mask = float3(maskDark, maskDark, maskDark);

	// Very compressed TV style shadow mask.
	if (shadowMask == 1) {
		float mask_line = maskLight;
		float odd = 0.0;
		if (frac(pos.x / 6.0) < 0.5) odd = 1.0;
		if (frac((pos.y + odd) / 2.0) < 0.5) mask_line = maskDark;
		pos.x = frac(pos.x / 3.0);

		if (pos.x < 0.333)mask.r = maskLight;
		else if (pos.x < 0.666)mask.g = maskLight;
		else mask.b = maskLight;
		mask *= mask_line;
	}

	// Aperture-grille.
	else if (shadowMask == 2) {
		pos.x = frac(pos.x / 3.0);

		if (pos.x < 0.333)mask.r = maskLight;
		else if (pos.x < 0.666)mask.g = maskLight;
		else mask.b = maskLight;
	}

	// Stretched VGA style shadow mask (same as prior shaders).
	else if (shadowMask == 3) {
		pos.x += pos.y * 3.0;
		pos.x = frac(pos.x / 6.0);

		if (pos.x < 0.333)mask.r = maskLight;
		else if (pos.x < 0.666)mask.g = maskLight;
		else mask.b = maskLight;
	}

	// VGA style shadow mask.
	else if (shadowMask == 4) {
		pos.xy = floor(pos.xy * float2(1.0, 0.5));
		pos.x += pos.y * 3.0;
		pos.x = frac(pos.x / 6.0);

		if (pos.x < 0.333)mask.r = maskLight;
		else if (pos.x < 0.666)mask.g = maskLight;
		else mask.b = maskLight;
	}

	return mask;
}

void Pass1(uint2 blockStart, uint3 threadId) {
	uint2 gxy = Rmp8x8(threadId.x) + blockStart;
	if (!CheckViewport(gxy)) {
		return;
	}

	float2 pos = (gxy + 0.5f) * GetOutputPt();

	uint2 inputSize = GetInputSize();
	float2 pos1 = Warp(pos);
	float3 outColor = Tri(pos1, inputSize);

#ifdef DO_BLOOM
	//Add Bloom
	outColor.rgb += Bloom(pos1, inputSize) * bloomAmount;
#endif

	if (shadowMask)
		outColor.rgb *= Mask(gxy + 0.5f);

	WriteToOutput(gxy, pow(outColor.rgb, 1.0f / 2.2f));
}
