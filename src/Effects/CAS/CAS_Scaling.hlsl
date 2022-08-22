// 移植自 https://github.com/GPUOpen-Effects/FidelityFX-CAS/blob/master/ffx-cas/ffx_cas.h

//!MAGPIE EFFECT
//!VERSION 2

//!PARAMETER
//!DEFAULT 0.4
//!MIN 0
//!MAX 1
float sharpness;

//!TEXTURE
Texture2D INPUT;


//!PASS 1
//!IN INPUT
//!BLOCK_SIZE 16
//!NUM_THREADS 64

// 取消注释此行将降低速度并提高输出质量
// #define CAS_BETTER_DIAGONALS

#define min3(x, y, z) (min(x, min(y, z)))
#define max3(x, y, z) (max(x, max(y, z)))


float3 CasFilter(uint2 ip, float4 const0, float peak) {
	// Scaling algorithm adaptively interpolates between nearest 4 results of the non-scaling algorithm.
 //  a b c d
 //  e f g h
 //  i j k l
 //  m n o p
 // Working these 4 results.
 //  +-----+-----+
 //  |     |     |
 //  |  f..|..g  |
 //  |  .  |  .  |
 //  +-----+-----+
 //  |  .  |  .  |
 //  |  j..|..k  |
 //  |     |     |
 //  +-----+-----+
	float2 pp = float2(ip) * const0.xy + const0.zw;
	float2 fp = floor(pp);
	pp -= fp;
	int2 sp = int2(fp);
	float3 a = INPUT.Load(int3(sp + int2(-1, -1), 0)).rgb;
	float3 b = INPUT.Load(int3(sp + int2(0, -1), 0)).rgb;
	float3 e = INPUT.Load(int3(sp + int2(-1, 0), 0)).rgb;
	float3 f = INPUT.Load(int3(sp, 0)).rgb;
	float3 c = INPUT.Load(int3(sp + int2(1, -1), 0)).rgb;
	float3 d = INPUT.Load(int3(sp + int2(2, -1), 0)).rgb;
	float3 g = INPUT.Load(int3(sp + int2(1, 0), 0)).rgb;
	float3 h = INPUT.Load(int3(sp + int2(2, 0), 0)).rgb;
	float3 i = INPUT.Load(int3(sp + int2(-1, 1), 0)).rgb;
	float3 j = INPUT.Load(int3(sp + int2(0, 1), 0)).rgb;
	float3 m = INPUT.Load(int3(sp + int2(-1, 2), 0)).rgb;
	float3 n = INPUT.Load(int3(sp + int2(0, 2), 0)).rgb;
	float3 k = INPUT.Load(int3(sp + int2(1, 1), 0)).rgb;
	float3 l = INPUT.Load(int3(sp + int2(2, 1), 0)).rgb;
	float3 o = INPUT.Load(int3(sp + int2(1, 2), 0)).rgb;
	float3 p = INPUT.Load(int3(sp + int2(2, 2), 0)).rgb;

	// Soft min and max.
	// These are 2.0x bigger (factored out the extra multiply).
	//  a b c             b
	//  e f g * 0.5  +  e f g * 0.5  [F]
	//  i j k             j
	float mnfR = min3(min3(b.r, e.r, f.r), g.r, j.r);
	float mnfG = min3(min3(b.g, e.g, f.g), g.g, j.g);
	float mnfB = min3(min3(b.b, e.b, f.b), g.b, j.b);
#ifdef CAS_BETTER_DIAGONALS
	float mnfR2 = min3(min3(mnfR, a.r, c.r), i.r, k.r);
	float mnfG2 = min3(min3(mnfG, a.g, c.g), i.g, k.g);
	float mnfB2 = min3(min3(mnfB, a.b, c.b), i.b, k.b);
	mnfR = mnfR + mnfR2;
	mnfG = mnfG + mnfG2;
	mnfB = mnfB + mnfB2;
#endif
	float mxfR = max3(max3(b.r, e.r, f.r), g.r, j.r);
	float mxfG = max3(max3(b.g, e.g, f.g), g.g, j.g);
	float mxfB = max3(max3(b.b, e.b, f.b), g.b, j.b);
#ifdef CAS_BETTER_DIAGONALS
	float mxfR2 = max3(max3(mxfR, a.r, c.r), i.r, k.r);
	float mxfG2 = max3(max3(mxfG, a.g, c.g), i.g, k.g);
	float mxfB2 = max3(max3(mxfB, a.b, c.b), i.b, k.b);
	mxfR = mxfR + mxfR2;
	mxfG = mxfG + mxfG2;
	mxfB = mxfB + mxfB2;
#endif
	//  b c d             c
	//  f g h * 0.5  +  f g h * 0.5  [G]
	//  j k l             k
	float mngR = min3(min3(c.r, f.r, g.r), h.r, k.r);
	float mngG = min3(min3(c.g, f.g, g.g), h.g, k.g);
	float mngB = min3(min3(c.b, f.b, g.b), h.b, k.b);
#ifdef CAS_BETTER_DIAGONALS
	float mngR2 = min3(min3(mngR, b.r, d.r), j.r, l.r);
	float mngG2 = min3(min3(mngG, b.g, d.g), j.g, l.g);
	float mngB2 = min3(min3(mngB, b.b, d.b), j.b, l.b);
	mngR = mngR + mngR2;
	mngG = mngG + mngG2;
	mngB = mngB + mngB2;
#endif
	float mxgR = max3(max3(c.r, f.r, g.r), h.r, k.r);
	float mxgG = max3(max3(c.g, f.g, g.g), h.g, k.g);
	float mxgB = max3(max3(c.b, f.b, g.b), h.b, k.b);
#ifdef CAS_BETTER_DIAGONALS
	float mxgR2 = max3(max3(mxgR, b.r, d.r), j.r, l.r);
	float mxgG2 = max3(max3(mxgG, b.g, d.g), j.g, l.g);
	float mxgB2 = max3(max3(mxgB, b.b, d.b), j.b, l.b);
	mxgR = mxgR + mxgR2;
	mxgG = mxgG + mxgG2;
	mxgB = mxgB + mxgB2;
#endif
	//  e f g             f
	//  i j k * 0.5  +  i j k * 0.5  [J]
	//  m n o             n
	float mnjR = min3(min3(f.r, i.r, j.r), k.r, n.r);
	float mnjG = min3(min3(f.g, i.g, j.g), k.g, n.g);
	float mnjB = min3(min3(f.b, i.b, j.b), k.b, n.b);
#ifdef CAS_BETTER_DIAGONALS
	float mnjR2 = min3(min3(mnjR, e.r, g.r), m.r, o.r);
	float mnjG2 = min3(min3(mnjG, e.g, g.g), m.g, o.g);
	float mnjB2 = min3(min3(mnjB, e.b, g.b), m.b, o.b);
	mnjR = mnjR + mnjR2;
	mnjG = mnjG + mnjG2;
	mnjB = mnjB + mnjB2;
#endif
	float mxjR = max3(max3(f.r, i.r, j.r), k.r, n.r);
	float mxjG = max3(max3(f.g, i.g, j.g), k.g, n.g);
	float mxjB = max3(max3(f.b, i.b, j.b), k.b, n.b);
#ifdef CAS_BETTER_DIAGONALS
	float mxjR2 = max3(max3(mxjR, e.r, g.r), m.r, o.r);
	float mxjG2 = max3(max3(mxjG, e.g, g.g), m.g, o.g);
	float mxjB2 = max3(max3(mxjB, e.b, g.b), m.b, o.b);
	mxjR = mxjR + mxjR2;
	mxjG = mxjG + mxjG2;
	mxjB = mxjB + mxjB2;
#endif
	//  f g h             g
	//  j k l * 0.5  +  j k l * 0.5  [K]
	//  n o p             o
	float mnkR = min3(min3(g.r, j.r, k.r), l.r, o.r);
	float mnkG = min3(min3(g.g, j.g, k.g), l.g, o.g);
	float mnkB = min3(min3(g.b, j.b, k.b), l.b, o.b);
#ifdef CAS_BETTER_DIAGONALS
	float mnkR2 = min3(min3(mnkR, f.r, h.r), n.r, p.r);
	float mnkG2 = min3(min3(mnkG, f.g, h.g), n.g, p.g);
	float mnkB2 = min3(min3(mnkB, f.b, h.b), n.b, p.b);
	mnkR = mnkR + mnkR2;
	mnkG = mnkG + mnkG2;
	mnkB = mnkB + mnkB2;
#endif
	float mxkR = max3(max3(g.r, j.r, k.r), l.r, o.r);
	float mxkG = max3(max3(g.g, j.g, k.g), l.g, o.g);
	float mxkB = max3(max3(g.b, j.b, k.b), l.b, o.b);
#ifdef CAS_BETTER_DIAGONALS
	float mxkR2 = max3(max3(mxkR, f.r, h.r), n.r, p.r);
	float mxkG2 = max3(max3(mxkG, f.g, h.g), n.g, p.g);
	float mxkB2 = max3(max3(mxkB, f.b, h.b), n.b, p.b);
	mxkR = mxkR + mxkR2;
	mxkG = mxkG + mxkG2;
	mxkB = mxkB + mxkB2;
#endif
	// Smooth minimum distance to signal limit divided by smooth max.
	float rcpMfR = rcp(mxfR);
	float rcpMfG = rcp(mxfG);
	float rcpMfB = rcp(mxfB);
	float rcpMgR = rcp(mxgR);
	float rcpMgG = rcp(mxgG);
	float rcpMgB = rcp(mxgB);
	float rcpMjR = rcp(mxjR);
	float rcpMjG = rcp(mxjG);
	float rcpMjB = rcp(mxjB);
	float rcpMkR = rcp(mxkR);
	float rcpMkG = rcp(mxkG);
	float rcpMkB = rcp(mxkB);

#ifdef CAS_BETTER_DIAGONALS
	float ampfR = saturate(min(mnfR, 2.0 - mxfR) * rcpMfR);
	float ampfG = saturate(min(mnfG, 2.0 - mxfG) * rcpMfG);
	float ampfB = saturate(min(mnfB, 2.0 - mxfB) * rcpMfB);
	float ampgR = saturate(min(mngR, 2.0 - mxgR) * rcpMgR);
	float ampgG = saturate(min(mngG, 2.0 - mxgG) * rcpMgG);
	float ampgB = saturate(min(mngB, 2.0 - mxgB) * rcpMgB);
	float ampjR = saturate(min(mnjR, 2.0 - mxjR) * rcpMjR);
	float ampjG = saturate(min(mnjG, 2.0 - mxjG) * rcpMjG);
	float ampjB = saturate(min(mnjB, 2.0 - mxjB) * rcpMjB);
	float ampkR = saturate(min(mnkR, 2.0 - mxkR) * rcpMkR);
	float ampkG = saturate(min(mnkG, 2.0 - mxkG) * rcpMkG);
	float ampkB = saturate(min(mnkB, 2.0 - mxkB) * rcpMkB);
#else
	float ampfR = saturate(min(mnfR, 1.0 - mxfR) * rcpMfR);
	float ampfG = saturate(min(mnfG, 1.0 - mxfG) * rcpMfG);
	float ampfB = saturate(min(mnfB, 1.0 - mxfB) * rcpMfB);
	float ampgR = saturate(min(mngR, 1.0 - mxgR) * rcpMgR);
	float ampgG = saturate(min(mngG, 1.0 - mxgG) * rcpMgG);
	float ampgB = saturate(min(mngB, 1.0 - mxgB) * rcpMgB);
	float ampjR = saturate(min(mnjR, 1.0 - mxjR) * rcpMjR);
	float ampjG = saturate(min(mnjG, 1.0 - mxjG) * rcpMjG);
	float ampjB = saturate(min(mnjB, 1.0 - mxjB) * rcpMjB);
	float ampkR = saturate(min(mnkR, 1.0 - mxkR) * rcpMkR);
	float ampkG = saturate(min(mnkG, 1.0 - mxkG) * rcpMkG);
	float ampkB = saturate(min(mnkB, 1.0 - mxkB) * rcpMkB);
#endif
	// Shaping amount of sharpening.
	ampfR = sqrt(ampfR);
	ampfG = sqrt(ampfG);
	ampfB = sqrt(ampfB);
	ampgR = sqrt(ampgR);
	ampgG = sqrt(ampgG);
	ampgB = sqrt(ampgB);
	ampjR = sqrt(ampjR);
	ampjG = sqrt(ampjG);
	ampjB = sqrt(ampjB);
	ampkR = sqrt(ampkR);
	ampkG = sqrt(ampkG);
	ampkB = sqrt(ampkB);

	// Filter shape.
	//  0 w 0
	//  w 1 w
	//  0 w 0
	float wfR = ampfR * peak;
	float wfG = ampfG * peak;
	float wfB = ampfB * peak;
	float wgR = ampgR * peak;
	float wgG = ampgG * peak;
	float wgB = ampgB * peak;
	float wjR = ampjR * peak;
	float wjG = ampjG * peak;
	float wjB = ampjB * peak;
	float wkR = ampkR * peak;
	float wkG = ampkG * peak;
	float wkB = ampkB * peak;
	// Blend between 4 results.
	//  s t
	//  u v
	float s = (1.0 - pp.x) * (1.0 - pp.y);
	float t = pp.x * (1.0 - pp.y);
	float u = (1.0 - pp.x) * pp.y;
	float v = pp.x * pp.y;
	// Thin edges to hide bilinear interpolation (helps diagonals).
	float thinB = 1.0 / 32.0;

	s *= rcp(thinB + (mxfG - mnfG));
	t *= rcp(thinB + (mxgG - mngG));
	u *= rcp(thinB + (mxjG - mnjG));
	v *= rcp(thinB + (mxkG - mnkG));

	// Final weighting.
	//    b c
	//  e f g h
	//  i j k l
	//    n o
	//  _____  _____  _____  _____
	//         fs        gt 
	//
	//  _____  _____  _____  _____
	//  fs      s gt  fs  t     gt
	//         ju        kv
	//  _____  _____  _____  _____
	//         fs        gt
	//  ju      u kv  ju  v     kv
	//  _____  _____  _____  _____
	//
	//         ju        kv
	float qbeR = wfR * s;
	float qbeG = wfG * s;
	float qbeB = wfB * s;
	float qchR = wgR * t;
	float qchG = wgG * t;
	float qchB = wgB * t;
	float qfR = wgR * t + wjR * u + s;
	float qfG = wgG * t + wjG * u + s;
	float qfB = wgB * t + wjB * u + s;
	float qgR = wfR * s + wkR * v + t;
	float qgG = wfG * s + wkG * v + t;
	float qgB = wfB * s + wkB * v + t;
	float qjR = wfR * s + wkR * v + u;
	float qjG = wfG * s + wkG * v + u;
	float qjB = wfB * s + wkB * v + u;
	float qkR = wgR * t + wjR * u + v;
	float qkG = wgG * t + wjG * u + v;
	float qkB = wgB * t + wjB * u + v;
	float qinR = wjR * u;
	float qinG = wjG * u;
	float qinB = wjB * u;
	float qloR = wkR * v;
	float qloG = wkG * v;
	float qloB = wkB * v;
	// Filter.
	// Using green coef only, depending on dead code removal to strip out the extra overhead.
	float rcpWG = rcp(2.0 * qbeG + 2.0 * qchG + 2.0 * qinG + 2.0 * qloG + qfG + qgG + qjG + qkG);

	return float3(
		saturate((b.r * qbeG + e.r * qbeG + c.r * qchG + h.r * qchG + i.r * qinG + n.r * qinG + l.r * qloG + o.r * qloG + f.r * qfG + g.r * qgG + j.r * qjG + k.r * qkG) * rcpWG),
		saturate((b.g * qbeG + e.g * qbeG + c.g * qchG + h.g * qchG + i.g * qinG + n.g * qinG + l.g * qloG + o.g * qloG + f.g * qfG + g.g * qgG + j.g * qjG + k.g * qkG) * rcpWG),
		saturate((b.b * qbeG + e.b * qbeG + c.b * qchG + h.b * qchG + i.b * qinG + n.b * qinG + l.b * qloG + o.b * qloG + f.b * qfG + g.b * qgG + j.b * qjG + k.b * qkG) * rcpWG)
	);
}


void Pass1(uint2 blockStart, uint3 threadId) {
	uint2 gxy = blockStart + Rmp8x8(threadId.x);
	if (!CheckViewport(gxy)) {
		return;
	}

	float4 const0;
	const0.xy = GetInputSize() * GetOutputPt();
	const0.zw = 0.5 * const0.xy - 0.5;

	const float peak = -rcp(lerp(8.0, 5.0, sharpness));

	WriteToOutput(gxy, CasFilter(gxy, const0, peak));

	gxy.x += 8u;
	if (CheckViewport(gxy)) {
		WriteToOutput(gxy, CasFilter(gxy, const0, peak));
	}

	gxy.y += 8u;
	if (CheckViewport(gxy)) {
		WriteToOutput(gxy, CasFilter(gxy, const0, peak));
	}

	gxy.x -= 8u;
	if (CheckViewport(gxy)) {
		WriteToOutput(gxy, CasFilter(gxy, const0, peak));
	}
}
