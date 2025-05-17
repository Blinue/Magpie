// 移植自 https://github.com/GPUOpen-Effects/FidelityFX-CAS/blob/9fabcc9a2c45f958aff55ddfda337e74ef894b7f/ffx-cas/ffx_cas.h

//!MAGPIE EFFECT
//!VERSION 4
// FP16 会使性能下降

#include "../StubDefs.hlsli"

//!PARAMETER
//!LABEL Sharpness
//!DEFAULT 0.4
//!MIN 0
//!MAX 1
//!STEP 0.01
float sharpness;

//!TEXTURE
Texture2D INPUT;

//!TEXTURE
Texture2D OUTPUT;


//!PASS 1
//!IN INPUT
//!OUT OUTPUT
//!BLOCK_SIZE 16
//!NUM_THREADS 64

// 取消注释此行将降低速度并提高输出质量
// #define CAS_BETTER_DIAGONALS

#define min3(x, y, z) (min(x, min(y, z)))
#define max3(x, y, z) (max(x, max(y, z)))

#ifdef MP_FP16

MF2 MF2_x(MF1 a) { return MF2(a, a); }
#define MF2_(a) MF2_x(MF1(a))

void CasFilterH(
	// Output values are for 2 8x8 tiles in a 16x8 region.
	//  pix<R,G,B>.x = right 8x8 tile
	//  pix<R,G,B>.y =  left 8x8 tile
	// This enables later processing to easily be packed as well.
	out MF2 pixR,
	out MF2 pixG,
	out MF2 pixB,
	uint2 ip,
	float4 const0,
	float offset,
	MF peak
) {
	// Scaling algorithm adaptively interpolates between nearest 4 results of the non-scaling algorithm.
	float2 pp = float2(ip) * const0.xy + const0.zw;
	// Tile 0.
	// Fractional position is needed in high precision here.
	float2 fp0 = floor(pp);
	MF2 ppX;
	ppX.x = MF1(pp.x - fp0.x);
	MF1 ppY = MF1(pp.y - fp0.y);
	int2 sp0 = int2(fp0);
	MF3 a0 = INPUT.Load(int3(sp0 + int2(-1, -1), 0)).rgb;
	MF3 b0 = INPUT.Load(int3(sp0 + int2(0, -1), 0)).rgb;
	MF3 e0 = INPUT.Load(int3(sp0 + int2(-1, 0), 0)).rgb;
	MF3 f0 = INPUT.Load(int3(sp0, 0)).rgb;
	MF3 c0 = INPUT.Load(int3(sp0 + int2(1, -1), 0)).rgb;
	MF3 d0 = INPUT.Load(int3(sp0 + int2(2, -1), 0)).rgb;
	MF3 g0 = INPUT.Load(int3(sp0 + int2(1, 0), 0)).rgb;
	MF3 h0 = INPUT.Load(int3(sp0 + int2(2, 0), 0)).rgb;
	MF3 i0 = INPUT.Load(int3(sp0 + int2(-1, 1), 0)).rgb;
	MF3 j0 = INPUT.Load(int3(sp0 + int2(0, 1), 0)).rgb;
	MF3 m0 = INPUT.Load(int3(sp0 + int2(-1, 2), 0)).rgb;
	MF3 n0 = INPUT.Load(int3(sp0 + int2(0, 2), 0)).rgb;
	MF3 k0 = INPUT.Load(int3(sp0 + int2(1, 1), 0)).rgb;
	MF3 l0 = INPUT.Load(int3(sp0 + int2(2, 1), 0)).rgb;
	MF3 o0 = INPUT.Load(int3(sp0 + int2(1, 2), 0)).rgb;
	MF3 p0 = INPUT.Load(int3(sp0 + int2(2, 2), 0)).rgb;
	// Tile 1 (offset only in x).
	float1 pp1 = pp.x + offset;
	float1 fp1 = floor(pp1);
	ppX.y = MF1(pp1 - fp1);
	int2 sp1 = int2(fp1, sp0.y);
	MF3 a1 = INPUT.Load(int3(sp1 + int2(-1, -1), 0)).rgb;
	MF3 b1 = INPUT.Load(int3(sp1 + int2(0, -1), 0)).rgb;
	MF3 e1 = INPUT.Load(int3(sp1 + int2(-1, 0), 0)).rgb;
	MF3 f1 = INPUT.Load(int3(sp1, 0)).rgb;
	MF3 c1 = INPUT.Load(int3(sp1 + int2(1, -1), 0)).rgb;
	MF3 d1 = INPUT.Load(int3(sp1 + int2(2, -1), 0)).rgb;
	MF3 g1 = INPUT.Load(int3(sp1 + int2(1, 0), 0)).rgb;
	MF3 h1 = INPUT.Load(int3(sp1 + int2(2, 0), 0)).rgb;
	MF3 i1 = INPUT.Load(int3(sp1 + int2(-1, 1), 0)).rgb;
	MF3 j1 = INPUT.Load(int3(sp1 + int2(0, 1), 0)).rgb;
	MF3 m1 = INPUT.Load(int3(sp1 + int2(-1, 2), 0)).rgb;
	MF3 n1 = INPUT.Load(int3(sp1 + int2(0, 2), 0)).rgb;
	MF3 k1 = INPUT.Load(int3(sp1 + int2(1, 1), 0)).rgb;
	MF3 l1 = INPUT.Load(int3(sp1 + int2(2, 1), 0)).rgb;
	MF3 o1 = INPUT.Load(int3(sp1 + int2(1, 2), 0)).rgb;
	MF3 p1 = INPUT.Load(int3(sp1 + int2(2, 2), 0)).rgb;
	// AOS to SOA conversion.
	MF2 aR = MF2(a0.r, a1.r);
	MF2 aG = MF2(a0.g, a1.g);
	MF2 aB = MF2(a0.b, a1.b);
	MF2 bR = MF2(b0.r, b1.r);
	MF2 bG = MF2(b0.g, b1.g);
	MF2 bB = MF2(b0.b, b1.b);
	MF2 cR = MF2(c0.r, c1.r);
	MF2 cG = MF2(c0.g, c1.g);
	MF2 cB = MF2(c0.b, c1.b);
	MF2 dR = MF2(d0.r, d1.r);
	MF2 dG = MF2(d0.g, d1.g);
	MF2 dB = MF2(d0.b, d1.b);
	MF2 eR = MF2(e0.r, e1.r);
	MF2 eG = MF2(e0.g, e1.g);
	MF2 eB = MF2(e0.b, e1.b);
	MF2 fR = MF2(f0.r, f1.r);
	MF2 fG = MF2(f0.g, f1.g);
	MF2 fB = MF2(f0.b, f1.b);
	MF2 gR = MF2(g0.r, g1.r);
	MF2 gG = MF2(g0.g, g1.g);
	MF2 gB = MF2(g0.b, g1.b);
	MF2 hR = MF2(h0.r, h1.r);
	MF2 hG = MF2(h0.g, h1.g);
	MF2 hB = MF2(h0.b, h1.b);
	MF2 iR = MF2(i0.r, i1.r);
	MF2 iG = MF2(i0.g, i1.g);
	MF2 iB = MF2(i0.b, i1.b);
	MF2 jR = MF2(j0.r, j1.r);
	MF2 jG = MF2(j0.g, j1.g);
	MF2 jB = MF2(j0.b, j1.b);
	MF2 kR = MF2(k0.r, k1.r);
	MF2 kG = MF2(k0.g, k1.g);
	MF2 kB = MF2(k0.b, k1.b);
	MF2 lR = MF2(l0.r, l1.r);
	MF2 lG = MF2(l0.g, l1.g);
	MF2 lB = MF2(l0.b, l1.b);
	MF2 mR = MF2(m0.r, m1.r);
	MF2 mG = MF2(m0.g, m1.g);
	MF2 mB = MF2(m0.b, m1.b);
	MF2 nR = MF2(n0.r, n1.r);
	MF2 nG = MF2(n0.g, n1.g);
	MF2 nB = MF2(n0.b, n1.b);
	MF2 oR = MF2(o0.r, o1.r);
	MF2 oG = MF2(o0.g, o1.g);
	MF2 oB = MF2(o0.b, o1.b);
	MF2 pR = MF2(p0.r, p1.r);
	MF2 pG = MF2(p0.g, p1.g);
	MF2 pB = MF2(p0.b, p1.b);
	// Soft min and max.
	// These are 2.0x bigger (factored out the extra multiply).
	//  a b c             b
	//  e f g * 0.5  +  e f g * 0.5  [F]
	//  i j k             j
	MF2 mnfR = min3(min3(bR, eR, fR), gR, jR);
	MF2 mnfG = min3(min3(bG, eG, fG), gG, jG);
	MF2 mnfB = min3(min3(bB, eB, fB), gB, jB);
#ifdef CAS_BETTER_DIAGONALS
	MF2 mnfR2 = min3(min3(mnfR, aR, cR), iR, kR);
	MF2 mnfG2 = min3(min3(mnfG, aG, cG), iG, kG);
	MF2 mnfB2 = min3(min3(mnfB, aB, cB), iB, kB);
	mnfR = mnfR + mnfR2;
	mnfG = mnfG + mnfG2;
	mnfB = mnfB + mnfB2;
#endif
	MF2 mxfR = max3(max3(bR, eR, fR), gR, jR);
	MF2 mxfG = max3(max3(bG, eG, fG), gG, jG);
	MF2 mxfB = max3(max3(bB, eB, fB), gB, jB);
#ifdef CAS_BETTER_DIAGONALS
	MF2 mxfR2 = max3(max3(mxfR, aR, cR), iR, kR);
	MF2 mxfG2 = max3(max3(mxfG, aG, cG), iG, kG);
	MF2 mxfB2 = max3(max3(mxfB, aB, cB), iB, kB);
	mxfR = mxfR + mxfR2;
	mxfG = mxfG + mxfG2;
	mxfB = mxfB + mxfB2;
#endif
	//  b c d             c
	//  f g h * 0.5  +  f g h * 0.5  [G]
	//  j k l             k
	MF2 mngR = min3(min3(cR, fR, gR), hR, kR);
	MF2 mngG = min3(min3(cG, fG, gG), hG, kG);
	MF2 mngB = min3(min3(cB, fB, gB), hB, kB);
#ifdef CAS_BETTER_DIAGONALS
	MF2 mngR2 = min3(min3(mngR, bR, dR), jR, lR);
	MF2 mngG2 = min3(min3(mngG, bG, dG), jG, lG);
	MF2 mngB2 = min3(min3(mngB, bB, dB), jB, lB);
	mngR = mngR + mngR2;
	mngG = mngG + mngG2;
	mngB = mngB + mngB2;
#endif
	MF2 mxgR = max3(max3(cR, fR, gR), hR, kR);
	MF2 mxgG = max3(max3(cG, fG, gG), hG, kG);
	MF2 mxgB = max3(max3(cB, fB, gB), hB, kB);
#ifdef CAS_BETTER_DIAGONALS
	MF2 mxgR2 = max3(max3(mxgR, bR, dR), jR, lR);
	MF2 mxgG2 = max3(max3(mxgG, bG, dG), jG, lG);
	MF2 mxgB2 = max3(max3(mxgB, bB, dB), jB, lB);
	mxgR = mxgR + mxgR2;
	mxgG = mxgG + mxgG2;
	mxgB = mxgB + mxgB2;
#endif
	//  e f g             f
	//  i j k * 0.5  +  i j k * 0.5  [J]
	//  m n o             n
	MF2 mnjR = min3(min3(fR, iR, jR), kR, nR);
	MF2 mnjG = min3(min3(fG, iG, jG), kG, nG);
	MF2 mnjB = min3(min3(fB, iB, jB), kB, nB);
#ifdef CAS_BETTER_DIAGONALS
	MF2 mnjR2 = min3(min3(mnjR, eR, gR), mR, oR);
	MF2 mnjG2 = min3(min3(mnjG, eG, gG), mG, oG);
	MF2 mnjB2 = min3(min3(mnjB, eB, gB), mB, oB);
	mnjR = mnjR + mnjR2;
	mnjG = mnjG + mnjG2;
	mnjB = mnjB + mnjB2;
#endif
	MF2 mxjR = max3(max3(fR, iR, jR), kR, nR);
	MF2 mxjG = max3(max3(fG, iG, jG), kG, nG);
	MF2 mxjB = max3(max3(fB, iB, jB), kB, nB);
#ifdef CAS_BETTER_DIAGONALS
	MF2 mxjR2 = max3(max3(mxjR, eR, gR), mR, oR);
	MF2 mxjG2 = max3(max3(mxjG, eG, gG), mG, oG);
	MF2 mxjB2 = max3(max3(mxjB, eB, gB), mB, oB);
	mxjR = mxjR + mxjR2;
	mxjG = mxjG + mxjG2;
	mxjB = mxjB + mxjB2;
#endif
	//  f g h             g
	//  j k l * 0.5  +  j k l * 0.5  [K]
	//  n o p             o
	MF2 mnkR = min3(min3(gR, jR, kR), lR, oR);
	MF2 mnkG = min3(min3(gG, jG, kG), lG, oG);
	MF2 mnkB = min3(min3(gB, jB, kB), lB, oB);
#ifdef CAS_BETTER_DIAGONALS
	MF2 mnkR2 = min3(min3(mnkR, fR, hR), nR, pR);
	MF2 mnkG2 = min3(min3(mnkG, fG, hG), nG, pG);
	MF2 mnkB2 = min3(min3(mnkB, fB, hB), nB, pB);
	mnkR = mnkR + mnkR2;
	mnkG = mnkG + mnkG2;
	mnkB = mnkB + mnkB2;
#endif
	MF2 mxkR = max3(max3(gR, jR, kR), lR, oR);
	MF2 mxkG = max3(max3(gG, jG, kG), lG, oG);
	MF2 mxkB = max3(max3(gB, jB, kB), lB, oB);
#ifdef CAS_BETTER_DIAGONALS
	MF2 mxkR2 = max3(max3(mxkR, fR, hR), nR, pR);
	MF2 mxkG2 = max3(max3(mxkG, fG, hG), nG, pG);
	MF2 mxkB2 = max3(max3(mxkB, fB, hB), nB, pB);
	mxkR = mxkR + mxkR2;
	mxkG = mxkG + mxkG2;
	mxkB = mxkB + mxkB2;
#endif
	// Smooth minimum distance to signal limit divided by smooth max.
	MF2 rcpMfR = rcp(mxfR);
	MF2 rcpMfG = rcp(mxfG);
	MF2 rcpMfB = rcp(mxfB);
	MF2 rcpMgR = rcp(mxgR);
	MF2 rcpMgG = rcp(mxgG);
	MF2 rcpMgB = rcp(mxgB);
	MF2 rcpMjR = rcp(mxjR);
	MF2 rcpMjG = rcp(mxjG);
	MF2 rcpMjB = rcp(mxjB);
	MF2 rcpMkR = rcp(mxkR);
	MF2 rcpMkG = rcp(mxkG);
	MF2 rcpMkB = rcp(mxkB);
#ifdef CAS_BETTER_DIAGONALS
	MF2 ampfR = saturate(min(mnfR, MF2_(2.0) - mxfR) * rcpMfR);
	MF2 ampfG = saturate(min(mnfG, MF2_(2.0) - mxfG) * rcpMfG);
	MF2 ampfB = saturate(min(mnfB, MF2_(2.0) - mxfB) * rcpMfB);
	MF2 ampgR = saturate(min(mngR, MF2_(2.0) - mxgR) * rcpMgR);
	MF2 ampgG = saturate(min(mngG, MF2_(2.0) - mxgG) * rcpMgG);
	MF2 ampgB = saturate(min(mngB, MF2_(2.0) - mxgB) * rcpMgB);
	MF2 ampjR = saturate(min(mnjR, MF2_(2.0) - mxjR) * rcpMjR);
	MF2 ampjG = saturate(min(mnjG, MF2_(2.0) - mxjG) * rcpMjG);
	MF2 ampjB = saturate(min(mnjB, MF2_(2.0) - mxjB) * rcpMjB);
	MF2 ampkR = saturate(min(mnkR, MF2_(2.0) - mxkR) * rcpMkR);
	MF2 ampkG = saturate(min(mnkG, MF2_(2.0) - mxkG) * rcpMkG);
	MF2 ampkB = saturate(min(mnkB, MF2_(2.0) - mxkB) * rcpMkB);
#else
	MF2 ampfR = saturate(min(mnfR, MF2_(1.0) - mxfR) * rcpMfR);
	MF2 ampfG = saturate(min(mnfG, MF2_(1.0) - mxfG) * rcpMfG);
	MF2 ampfB = saturate(min(mnfB, MF2_(1.0) - mxfB) * rcpMfB);
	MF2 ampgR = saturate(min(mngR, MF2_(1.0) - mxgR) * rcpMgR);
	MF2 ampgG = saturate(min(mngG, MF2_(1.0) - mxgG) * rcpMgG);
	MF2 ampgB = saturate(min(mngB, MF2_(1.0) - mxgB) * rcpMgB);
	MF2 ampjR = saturate(min(mnjR, MF2_(1.0) - mxjR) * rcpMjR);
	MF2 ampjG = saturate(min(mnjG, MF2_(1.0) - mxjG) * rcpMjG);
	MF2 ampjB = saturate(min(mnjB, MF2_(1.0) - mxjB) * rcpMjB);
	MF2 ampkR = saturate(min(mnkR, MF2_(1.0) - mxkR) * rcpMkR);
	MF2 ampkG = saturate(min(mnkG, MF2_(1.0) - mxkG) * rcpMkG);
	MF2 ampkB = saturate(min(mnkB, MF2_(1.0) - mxkB) * rcpMkB);
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
	MF2 wfR = ampfR * peak;
	MF2 wfG = ampfG * peak;
	MF2 wfB = ampfB * peak;
	MF2 wgR = ampgR * peak;
	MF2 wgG = ampgG * peak;
	MF2 wgB = ampgB * peak;
	MF2 wjR = ampjR * peak;
	MF2 wjG = ampjG * peak;
	MF2 wjB = ampjB * peak;
	MF2 wkR = ampkR * peak;
	MF2 wkG = ampkG * peak;
	MF2 wkB = ampkB * peak;
	// Blend between 4 results.
	MF2 s = (MF2_(1.0) - ppX) * (MF2_(1.0) - MF2_(ppY));
	MF2 t = ppX * (MF2_(1.0) - MF2_(ppY));
	MF2 u = (MF2_(1.0) - ppX) * MF2_(ppY);
	MF2 v = ppX * MF2_(ppY);
	// Thin edges to hide bilinear interpolation (helps diagonals).
	MF2 thinB = MF2_(1.0 / 32.0);
	s *= rcp(thinB + (mxfG - mnfG));
	t *= rcp(thinB + (mxgG - mngG));
	u *= rcp(thinB + (mxjG - mnjG));
	v *= rcp(thinB + (mxkG - mnkG));
	// Final weighting.
	MF2 qbeR = wfR * s;
	MF2 qbeG = wfG * s;
	MF2 qbeB = wfB * s;
	MF2 qchR = wgR * t;
	MF2 qchG = wgG * t;
	MF2 qchB = wgB * t;
	MF2 qfR = wgR * t + wjR * u + s;
	MF2 qfG = wgG * t + wjG * u + s;
	MF2 qfB = wgB * t + wjB * u + s;
	MF2 qgR = wfR * s + wkR * v + t;
	MF2 qgG = wfG * s + wkG * v + t;
	MF2 qgB = wfB * s + wkB * v + t;
	MF2 qjR = wfR * s + wkR * v + u;
	MF2 qjG = wfG * s + wkG * v + u;
	MF2 qjB = wfB * s + wkB * v + u;
	MF2 qkR = wgR * t + wjR * u + v;
	MF2 qkG = wgG * t + wjG * u + v;
	MF2 qkB = wgB * t + wjB * u + v;
	MF2 qinR = wjR * u;
	MF2 qinG = wjG * u;
	MF2 qinB = wjB * u;
	MF2 qloR = wkR * v;
	MF2 qloG = wkG * v;
	MF2 qloB = wkB * v;
	// Filter.
	MF2 rcpWG = rcp(MF2_(2.0) * qbeG + MF2_(2.0) * qchG + MF2_(2.0) * qinG + MF2_(2.0) * qloG + qfG + qgG + qjG + qkG);
	pixR = saturate((bR * qbeG + eR * qbeG + cR * qchG + hR * qchG + iR * qinG + nR * qinG + lR * qloG + oR * qloG + fR * qfG + gR * qgG + jR * qjG + kR * qkG) * rcpWG);
	pixG = saturate((bG * qbeG + eG * qbeG + cG * qchG + hG * qchG + iG * qinG + nG * qinG + lG * qloG + oG * qloG + fG * qfG + gG * qgG + jG * qjG + kG * qkG) * rcpWG);
	pixB = saturate((bB * qbeG + eB * qbeG + cB * qchG + hB * qchG + iB * qinG + nB * qinG + lB * qloG + oB * qloG + fB * qfG + gB * qgG + jB * qjG + kB * qkG) * rcpWG);
}

#else

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

#endif

void Pass1(uint2 blockStart, uint3 threadId) {
	uint2 gxy = blockStart + Rmp8x8(threadId.x);
	
	const uint2 outputSize = GetOutputSize();
	if (gxy.x >= outputSize.x || gxy.y >= outputSize.y) {
		return;
	}

	float4 const0;
	const0.xy = GetInputSize() * GetOutputPt();
	const0.zw = 0.5 * const0.xy - 0.5;

	const MF peak = MF(-rcp(lerp(8.0, 5.0, sharpness)));

#ifdef MP_FP16
	const float offset = 8 / GetScale().x;

	MF2 pixR, pixG, pixB;
	CasFilterH(pixR, pixG, pixB, gxy, const0, offset, peak);

	OUTPUT[gxy] = MF4(pixR.x, pixG.x, pixB.x, 1);

	gxy.x += 8u;
	OUTPUT[gxy] = MF4(pixR.y, pixG.y, pixB.y, 1);

	gxy += int2(-8, 8);
	CasFilterH(pixR, pixG, pixB, gxy, const0, offset, peak);

	OUTPUT[gxy] = MF4(pixR.x, pixG.x, pixB.x, 1);

	gxy.x += 8u;
	OUTPUT[gxy] = MF4(pixR.y, pixG.y, pixB.y, 1);
#else
	OUTPUT[gxy] = float4(CasFilter(gxy, const0, peak), 1);

	gxy.x += 8u;
	OUTPUT[gxy] = float4(CasFilter(gxy, const0, peak), 1);

	gxy.y += 8u;
	OUTPUT[gxy] = float4(CasFilter(gxy, const0, peak), 1);

	gxy.x -= 8u;
	OUTPUT[gxy] = float4(CasFilter(gxy, const0, peak), 1);
#endif
}
