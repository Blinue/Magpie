// CRT-Geom
// 移植自 https://github.com/libretro/common-shaders/blob/master/crt/shaders/crt-geom.cg

/*
	CRT-interlaced

	Copyright (C) 2010-2012 cgwg, Themaister and DOLLS

	This program is free software; you can redistribute it and/or modify it
	under the terms of the GNU General Public License as published by the Free
	Software Foundation; either version 2 of the License, or (at your option)
	any later version.

	(cgwg gave their consent to have the original version of this shader
	distributed under the GPL in this message:

		http://board.byuu.org/viewtopic.php?p=26075#p26075

		"Feel free to distribute my shaders under the GPL. After all, the
		barrel distortion code was taken from the Curvature shader, which is
		under the GPL."
	)
	This shader variant is pre-configured with screen curvature
*/

//!MAGPIE EFFECT
//!VERSION 2


//!PARAMETER
//!DEFAULT 2.4
//!MIN 0.1
//!MAX 5
float CRTGamma;

//!PARAMETER
//!DEFAULT 2.2
//!MIN 0.1
//!MAX 5
float monitorGamma;

//!PARAMETER
//!DEFAULT 1.5
//!MIN 0.1
//!MAX 3.0
float distance;

//!PARAMETER
//!DEFAULT 1
//!MIN 0
//!MAX 1
int curvature;

//!PARAMETER
//!DEFAULT 2
//!MIN 0.1
//!MAX 10
float radius;

//!PARAMETER
//!DEFAULT 0.03
//!MIN 0.001
//!MAX 1.0
float cornerSize;

//!PARAMETER
//!DEFAULT 1000
//!MIN 80
//!MAX 2000
int cornerSmooth;

//!PARAMETER
//!DEFAULT 0
//!MIN -0.5
//!MAX 0.5
float xTilt;

//!PARAMETER
//!DEFAULT 0
//!MIN -0.5
//!MAX 0.5
float yTilt;

//!PARAMETER
//!DEFAULT 100
//!MIN -125
//!MAX 125
int overScanX;

//!PARAMETER
//!DEFAULT 100
//!MIN -125
//!MAX 125
int overScanY;

//!PARAMETER
//!DEFAULT 0.3
//!MIN 0
//!MAX 0.3
float dotMask;

//!PARAMETER
//!DEFAULT 1
//!MIN 1
//!MAX 3
int sharper;

//!PARAMETER
//!DEFAULT 0.3
//!MIN 0.1
//!MAX 0.5
float scanlineWeight;

//!PARAMETER
//!DEFAULT 0
//!MIN 0
//!MAX 1
float lum;

//!PARAMETER
//!DEFAULT 1
//!MIN 0
//!MAX 1
int interlace;


//!TEXTURE
Texture2D INPUT;

//!SAMPLER
//!FILTER POINT
SamplerState sam;


//!PASS 1
//!STYLE PS
//!IN INPUT

#pragma warning(disable: 3571) // X3571: pow(f, e) will not work for negative f, use abs(f) or conditionally handle negative values if you expect them

// Use the older, purely gaussian beam profile; uncomment for speed
// #define USEGAUSSIAN

// Macros.
#define FIX(c) max(abs(c), 1e-5)
#define PI 3.141592653589

#define TEX2D(c) pow(INPUT.SampleLevel(sam, (c), 0), CRTGamma)


// aspect ratio
#define aspect float2(1.0, 0.75)


float intersect(float2 xy, float4 sin_cos_angle) {
	float A = dot(xy, xy) + distance * distance;
	float B = 2.0 * (radius * (dot(xy, sin_cos_angle.xy) - distance * sin_cos_angle.zw.x * sin_cos_angle.zw.y) - distance * distance);
	float C = distance * distance + 2.0 * radius * distance * sin_cos_angle.zw.x * sin_cos_angle.zw.y;
	return (-B - sqrt(B * B - 4.0 * A * C)) / (2.0 * A);
}

float2 bkwtrans(float2 xy, float4 sin_cos_angle) {
	float c = intersect(xy, sin_cos_angle);
	float2 point_ = c * xy;
	point_ += radius * sin_cos_angle.xy;
	point_ /= radius;
	float2 tang = sin_cos_angle.xy / sin_cos_angle.zw;
	float2 poc = point_ / sin_cos_angle.zw;
	float A = dot(tang, tang) + 1.0;
	float B = -2.0 * dot(poc, tang);
	float C = dot(poc, poc) - 1.0;
	float a = (-B + sqrt(B * B - 4.0 * A * C)) / (2.0 * A);
	float2 uv = (point_ - a * sin_cos_angle.xy) / sin_cos_angle.zw;
	float r = FIX(radius * acos(a));
	return uv * r / sin(r / radius);
}

float2 fwtrans(float2 uv, float4 sin_cos_angle) {
	float r = FIX(sqrt(dot(uv, uv)));
	uv *= sin(r / radius) / r;
	float x = 1.0 - cos(r / radius);
	float D = distance / radius + x * sin_cos_angle.z * sin_cos_angle.w + dot(uv, sin_cos_angle.xy);
	return distance * (uv * sin_cos_angle.zw - x * sin_cos_angle.xy) / D;
}

float3 maxscale(float4 sin_cos_angle) {
	float2 c = bkwtrans(-radius * sin_cos_angle.xy / (1.0 + radius / distance * sin_cos_angle.z * sin_cos_angle.w), sin_cos_angle);
	float2 a = 0.5 * aspect;
	float2 lo = float2(fwtrans(float2(-a.x, c.y), sin_cos_angle).x,
		fwtrans(float2(c.x, -a.y), sin_cos_angle).y) / aspect;
	float2 hi = float2(fwtrans(float2(+a.x, c.y), sin_cos_angle).x,
		fwtrans(float2(c.x, +a.y), sin_cos_angle).y) / aspect;
	return float3((hi + lo) * aspect * 0.5, max(hi.x - lo.x, hi.y - lo.y));
}

// Calculate the influence of a scanline on the current pixel.
//
// 'distance' is the distance in texture coordinates from the current
// pixel to the scanline in question.
// 'color' is the colour of the scanline at the horizontal location of
// the current pixel.
float4 scanlineWeights(float distance1, float4 color) {
	// "wid" controls the width of the scanline beam, for each RGB
	// channel The "weights" lines basically specify the formula
	// that gives you the profile of the beam, i.e. the intensity as
	// a function of distance from the vertical center of the
	// scanline. In this case, it is gaussian if width=2, and
	// becomes nongaussian for larger widths. Ideally this should
	// be normalized so that the integral across the beam is
	// independent of its width. That is, for a narrower beam
	// "weights" should have a higher peak at the center of the
	// scanline than for a wider beam.
#ifdef USEGAUSSIAN
	float4 wid = 0.3 + 0.1 * pow(color, 3.0);
	float v = distance1 / (wid * scanline_weight / 0.3);
	float4 weights = { v, v, v, v };
	return (lum + 0.4) * exp(-weights * weights) / wid;
#else
	float4 wid = 2.0 + 2.0 * pow(color, 4.0);
	float v = distance1 / scanlineWeight;
	float4 weights = float4(v, v, v, v);
	return (lum + 1.4) * exp(-pow(weights * rsqrt(0.5 * wid), wid)) / (0.6 + 0.2 * wid);
#endif
}

float4 Pass1(float2 pos) {
	const uint2 outputSize = GetOutputSize();
	const uint2 inputSize = GetInputSize();

	float4 sin_cos_angle = { sin(float2(xTilt, yTilt)), cos(float2(xTilt, yTilt)) };
	float3 stretch = maxscale(sin_cos_angle);
	float2 TextureSize = float2(sharper * inputSize.x, inputSize.y);
	// Resulting X pixel-coordinate of the pixel we're drawing.
	float mod_factor = pos.x * outputSize.x;
	float2 ilfac = { 1.0, clamp(floor(inputSize.y / (200.0 * (-4 * interlace + 5))), 1.0, 2.0)};
	float2 one = ilfac / TextureSize;

	// Here's a helpful diagram to keep in mind while trying to
	// understand the code:
	//
	//  |      |      |      |      |
	// -------------------------------
	//  |      |      |      |      |
	//  |  01  |  11  |  21  |  31  | <-- current scanline
	//  |      | @    |      |      |
	// -------------------------------
	//  |      |      |      |      |
	//  |  02  |  12  |  22  |  32  | <-- next scanline
	//  |      |      |      |      |
	// -------------------------------
	//  |      |      |      |      |
	//
	// Each character-cell represents a pixel on the output
	// surface, "@" represents the current pixel (always somewhere
	// in the bottom half of the current scan-line, or the top-half
	// of the next scanline). The grid of lines represents the
	// edges of the texels of the underlying texture.

	// Texture coordinates of the texel containing the active pixel.
	float2 xy = 0.0;
	if (curvature > 0) {
		float2 cd = pos;
		cd = (cd - 0.5) * aspect * stretch.z + stretch.xy;
		xy = bkwtrans(cd, sin_cos_angle) / float2(overScanX / 100.0, overScanY / 100.0) / aspect + float2(0.5, 0.5);
	} else {
		xy = pos;
	}

	float2 cd2 = xy;
	cd2 = (cd2 - 0.5) * float2(overScanX, overScanY) / 100.0 + 0.5;
	cd2 = min(cd2, 1.0 - cd2) * aspect;
	float2 cdist = float2(cornerSize, cornerSize);
	cd2 = (cdist - min(cd2, cdist));
	float dist = sqrt(dot(cd2, cd2));
	float cval = clamp((cdist.x - dist) * cornerSmooth, 0.0, 1.0);

	// Of all the pixels that are mapped onto the texel we are
	// currently rendering, which pixel are we currently rendering?
	float2 ilfloat = float2(0.0, ilfac.y > 1.5 ? fmod(GetFrameCount(), 2.0) : 0.0);

	float2 ratio_scale = (xy * TextureSize - 0.5 + ilfloat) / ilfac;

	float filter = rcp(GetScale().y);
	float2 uv_ratio = frac(ratio_scale);

	// Snap to the center of the underlying texel.

	xy = (floor(ratio_scale) * ilfac + 0.5 - ilfloat) / TextureSize;

	// Calculate Lanczos scaling coefficients describing the effect
	// of various neighbour texels in a scanline on the current
	// pixel.
	float4 coeffs = PI * float4(1.0 + uv_ratio.x, uv_ratio.x, 1.0 - uv_ratio.x, 2.0 - uv_ratio.x);

	// Prevent division by zero.
	coeffs = FIX(coeffs);

	// Lanczos2 kernel.
	coeffs = 2.0 * sin(coeffs) * sin(coeffs / 2.0) / (coeffs * coeffs);

	// Normalize.
	coeffs /= dot(coeffs, float4(1.0, 1.0, 1.0, 1.0));

	// Calculate the effective colour of the current and next
	// scanlines at the horizontal location of the current pixel,
	// using the Lanczos coefficients above.
	float4 col = clamp(mul(coeffs, float4x4(
		TEX2D(xy + float2(-one.x, 0.0)),
		TEX2D(xy),
		TEX2D(xy + float2(one.x, 0.0)),
		TEX2D(xy + float2(2.0 * one.x, 0.0)))),
		0.0, 1.0);
	float4 col2 = clamp(mul(coeffs, float4x4(
		TEX2D(xy + float2(-one.x, one.y)),
		TEX2D(xy + float2(0.0, one.y)),
		TEX2D(xy + one),
		TEX2D(xy + float2(2.0 * one.x, one.y)))),
		0.0, 1.0);

	col = pow(col, CRTGamma);
	col2 = pow(col2, CRTGamma);

	// Calculate the influence of the current and next scanlines on
	// the current pixel.
	float4 weights = scanlineWeights(uv_ratio.y, col);
	float4 weights2 = scanlineWeights(1.0 - uv_ratio.y, col2);

	uv_ratio.y = uv_ratio.y + 1.0 / 3.0 * filter;
	weights = (weights + scanlineWeights(uv_ratio.y, col)) / 3.0;
	weights2 = (weights2 + scanlineWeights(abs(1.0 - uv_ratio.y), col2)) / 3.0;
	uv_ratio.y = uv_ratio.y - 2.0 / 3.0 * filter;
	weights = weights + scanlineWeights(abs(uv_ratio.y), col) / 3.0;
	weights2 = weights2 + scanlineWeights(abs(1.0 - uv_ratio.y), col2) / 3.0;

	float3 mul_res = (col * weights + col2 * weights2).rgb;
	mul_res *= float3(cval, cval, cval);

	// dot-mask emulation:
	// Output pixels are alternately tinted green and magenta.
	float3 dotMaskWeights = lerp(
		float3(1.0, 1.0 - dotMask, 1.0),
		float3(1.0 - dotMask, 1.0, 1.0 - dotMask),
		floor(fmod(mod_factor, 2.0))
	);
	mul_res *= dotMaskWeights;

	// Convert the image gamma for display on our output device.
	mul_res = pow(mul_res, 1.0 / monitorGamma);

	// Color the texel.
	return float4(mul_res, 1.0);
}
