// FidelityFX-FSR 中 EASU 通道
// 移植自 https://github.com/GPUOpen-Effects/FidelityFX-FSR/blob/master/ffx-fsr/ffx_fsr1.h

//!MAGPIE EFFECT
//!VERSION 4
//!USE FP16

#include "..\StubDefs.hlsli"

//!TEXTURE
Texture2D INPUT;

//!TEXTURE
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

#ifdef MP_FP16

MF2 MF2_x(MF1 a) { return MF2(a, a); }
#define MF2_(a) MF2_x(MF1(a))

// This runs 2 taps in parallel.
void FsrEasuTapH(
	inout MF2 aCR,
	inout MF2 aCG,
	inout MF2 aCB,
	inout MF2 aW,
	MF2 offX,
	MF2 offY,
	MF2 dir,
	MF2 len,
	MF1 lob,
	MF1 clp,
	MF2 cR,
	MF2 cG,
	MF2 cB
) {
	MF2 vX, vY;
	vX = offX * dir.xx + offY * dir.yy;
	vY = offX * (-dir.yy) + offY * dir.xx;
	vX *= len.x; vY *= len.y;
	MF2 d2 = vX * vX + vY * vY;
	d2 = min(d2, MF2_(clp));
	MF2 wB = MF2_(2.0 / 5.0) * d2 + MF2_(-1.0);
	MF2 wA = MF2_(lob) * d2 + MF2_(-1.0);
	wB *= wB;
	wA *= wA;
	wB = MF2_(25.0 / 16.0) * wB + MF2_(-(25.0 / 16.0 - 1.0));
	MF2 w = wB * wA;
	aCR += cR * w;
	aCG += cG * w;
	aCB += cB * w;
	aW += w;
}

// This runs 2 taps in parallel.
void FsrEasuSetH(
	inout MF2 dirPX,
	inout MF2 dirPY,
	inout MF2 lenP,
	MF2 pp,
	bool biST,
	bool biUV,
	MF2 lA,
	MF2 lB,
	MF2 lC,
	MF2 lD,
	MF2 lE
) {
	MF2 w = MF2_(0.0);
	if (biST)w = (MF2(1.0, 0.0) + MF2(-pp.x, pp.x)) * MF(MF(1.0) - pp.y);
	if (biUV)w = (MF2(1.0, 0.0) + MF2(-pp.x, pp.x)) * MF(pp.y);
	// ABS is not free in the packed FP16 path.
	MF2 dc = lD - lC;
	MF2 cb = lC - lB;
	MF2 lenX = max(abs(dc), abs(cb));
	lenX = rcp(lenX);
	MF2 dirX = lD - lB;
	dirPX += dirX * w;
	lenX = saturate(abs(dirX) * lenX);
	lenX *= lenX;
	lenP += lenX * w;
	MF2 ec = lE - lC;
	MF2 ca = lC - lA;
	MF2 lenY = max(abs(ec), abs(ca));
	lenY = rcp(lenY);
	MF2 dirY = lE - lA;
	dirPY += dirY * w;
	lenY = saturate(abs(dirY) * lenY);
	lenY *= lenY;
	lenP += lenY * w;
}

MF3 FsrEasu(
	uint2 ip,
	float4 con0,
	float4 con1,
	float4 con2,
	float2 con3
) {
	//------------------------------------------------------------------------------------------------------------------------------
	float2 pp = float2(ip) * con0.xy + con0.zw;
	float2 fp = floor(pp);
	pp -= fp;
	MF2 ppp = MF2(pp);
	//------------------------------------------------------------------------------------------------------------------------------
	float2 p0 = fp * con1.xy + con1.zw;
	float2 p1 = p0 + con2.xy;
	float2 p2 = p0 + con2.zw;
	float2 p3 = p0 + con3.xy;
	MF4 bczzR = INPUT.GatherRed(sam, p0);
	MF4 bczzG = INPUT.GatherGreen(sam, p0);
	MF4 bczzB = INPUT.GatherBlue(sam, p0);
	MF4 ijfeR = INPUT.GatherRed(sam, p1);
	MF4 ijfeG = INPUT.GatherGreen(sam, p1);
	MF4 ijfeB = INPUT.GatherBlue(sam, p1);
	MF4 klhgR = INPUT.GatherRed(sam, p2);
	MF4 klhgG = INPUT.GatherGreen(sam, p2);
	MF4 klhgB = INPUT.GatherBlue(sam, p2);
	MF4 zzonR = INPUT.GatherRed(sam, p3);
	MF4 zzonG = INPUT.GatherGreen(sam, p3);
	MF4 zzonB = INPUT.GatherBlue(sam, p3);
	//------------------------------------------------------------------------------------------------------------------------------
	MF4 bczzL = bczzB * MF(0.5) + (bczzR * MF(0.5) + bczzG);
	MF4 ijfeL = ijfeB * MF(0.5) + (ijfeR * MF(0.5) + ijfeG);
	MF4 klhgL = klhgB * MF(0.5) + (klhgR * MF(0.5) + klhgG);
	MF4 zzonL = zzonB * MF(0.5) + (zzonR * MF(0.5) + zzonG);
	MF1 bL = bczzL.x;
	MF1 cL = bczzL.y;
	MF1 iL = ijfeL.x;
	MF1 jL = ijfeL.y;
	MF1 fL = ijfeL.z;
	MF1 eL = ijfeL.w;
	MF1 kL = klhgL.x;
	MF1 lL = klhgL.y;
	MF1 hL = klhgL.z;
	MF1 gL = klhgL.w;
	MF1 oL = zzonL.z;
	MF1 nL = zzonL.w;
	// This part is different, accumulating 2 taps in parallel.
	MF2 dirPX = MF2_(0.0);
	MF2 dirPY = MF2_(0.0);
	MF2 lenP = MF2_(0.0);
	FsrEasuSetH(dirPX, dirPY, lenP, ppp, true, false, MF2(bL, cL), MF2(eL, fL), MF2(fL, gL), MF2(gL, hL), MF2(jL, kL));
	FsrEasuSetH(dirPX, dirPY, lenP, ppp, false, true, MF2(fL, gL), MF2(iL, jL), MF2(jL, kL), MF2(kL, lL), MF2(nL, oL));
	MF2 dir = MF2(dirPX.r + dirPX.g, dirPY.r + dirPY.g);
	MF1 len = lenP.r + lenP.g;
	//------------------------------------------------------------------------------------------------------------------------------
	MF2 dir2 = dir * dir;
	MF1 dirR = dir2.x + dir2.y;
	bool zro = dirR < MF1(1.0 / 32768.0);
	dirR = rsqrt(dirR);
	dirR = zro ? MF(1.0) : dirR;
	dir.x = zro ? MF(1.0) : dir.x;
	dir *= MF2_(dirR);
	len = len * MF(0.5);
	len *= len;
	MF1 stretch = (dir.x * dir.x + dir.y * dir.y) * rcp(max(abs(dir.x), abs(dir.y)));
	MF2 len2 = MF2(MF(1.0) + (stretch - MF(1.0)) * len, MF(1.0) + MF(-0.5) * len);
	MF1 lob = MF(0.5) + MF((1.0 / 4.0 - 0.04) - 0.5) * len;
	MF1 clp = rcp(lob);
	//------------------------------------------------------------------------------------------------------------------------------
	  // FP16 is different, using packed trick to do min and max in same operation.
	MF2 bothR = max(max(MF2(-ijfeR.z, ijfeR.z), MF2(-klhgR.w, klhgR.w)), max(MF2(-ijfeR.y, ijfeR.y), MF2(-klhgR.x, klhgR.x)));
	MF2 bothG = max(max(MF2(-ijfeG.z, ijfeG.z), MF2(-klhgG.w, klhgG.w)), max(MF2(-ijfeG.y, ijfeG.y), MF2(-klhgG.x, klhgG.x)));
	MF2 bothB = max(max(MF2(-ijfeB.z, ijfeB.z), MF2(-klhgB.w, klhgB.w)), max(MF2(-ijfeB.y, ijfeB.y), MF2(-klhgB.x, klhgB.x)));
	// This part is different for FP16, working pairs of taps at a time.
	MF2 pR = MF2_(0.0);
	MF2 pG = MF2_(0.0);
	MF2 pB = MF2_(0.0);
	MF2 pW = MF2_(0.0);
	FsrEasuTapH(pR, pG, pB, pW, MF2(0.0, 1.0) - ppp.xx, MF2(-1.0, -1.0) - ppp.yy, dir, len2, lob, clp, bczzR.xy, bczzG.xy, bczzB.xy);
	FsrEasuTapH(pR, pG, pB, pW, MF2(-1.0, 0.0) - ppp.xx, MF2(1.0, 1.0) - ppp.yy, dir, len2, lob, clp, ijfeR.xy, ijfeG.xy, ijfeB.xy);
	FsrEasuTapH(pR, pG, pB, pW, MF2(0.0, -1.0) - ppp.xx, MF2(0.0, 0.0) - ppp.yy, dir, len2, lob, clp, ijfeR.zw, ijfeG.zw, ijfeB.zw);
	FsrEasuTapH(pR, pG, pB, pW, MF2(1.0, 2.0) - ppp.xx, MF2(1.0, 1.0) - ppp.yy, dir, len2, lob, clp, klhgR.xy, klhgG.xy, klhgB.xy);
	FsrEasuTapH(pR, pG, pB, pW, MF2(2.0, 1.0) - ppp.xx, MF2(0.0, 0.0) - ppp.yy, dir, len2, lob, clp, klhgR.zw, klhgG.zw, klhgB.zw);
	FsrEasuTapH(pR, pG, pB, pW, MF2(1.0, 0.0) - ppp.xx, MF2(2.0, 2.0) - ppp.yy, dir, len2, lob, clp, zzonR.zw, zzonG.zw, zzonB.zw);
	MF3 aC = MF3(pR.x + pR.y, pG.x + pG.y, pB.x + pB.y);
	MF aW = pW.x + pW.y;
	//------------------------------------------------------------------------------------------------------------------------------
	  // Slightly different for FP16 version due to combined min and max.
	return min(MF3(bothR.y, bothG.y, bothB.y), max(-MF3(bothR.x, bothG.x, bothB.x), aC * rcp(aW)));
}

#else

// Filtering for a given tap for the scalar.
void FsrEasuTapF(
	inout float3 aC, // Accumulated color, with negative lobe.
	inout float aW, // Accumulated weight.
	float2 off, // Pixel offset from resolve position to tap.
	float2 dir, // Gradient direction.
	float2 len, // Length.
	float lob, // Negative lobe strength.
	float clp, // Clipping point.
	float3 c // Tap color.
) {
	// Rotate offset by direction.
	float2 v;
	v.x = (off.x * (dir.x)) + (off.y * dir.y);
	v.y = (off.x * (-dir.y)) + (off.y * dir.x);
	// Anisotropy.
	v *= len;
	// Compute distance^2.
	float d2 = v.x * v.x + v.y * v.y;
	// Limit to the window as at corner, 2 taps can easily be outside.
	d2 = min(d2, clp);
	// Approximation of lanczos2 without sin() or rcp(), or sqrt() to get x.
	//  (25/16 * (2/5 * x^2 - 1)^2 - (25/16 - 1)) * (1/4 * x^2 - 1)^2
	//  |_______________________________________|   |_______________|
	//                   base                             window
	// The general form of the 'base' is,
	//  (a*(b*x^2-1)^2-(a-1))
	// Where 'a=1/(2*b-b^2)' and 'b' moves around the negative lobe.
	float wB = 2.0f / 5.0f * d2 - 1;
	float wA = lob * d2 - 1;
	wB *= wB;
	wA *= wA;
	wB = 25.0f / 16.0f * wB - (25.0f / 16.0f - 1.0f);
	float w = wB * wA;
	// Do weighted average.
	aC += c * w;
	aW += w;
}

// Accumulate direction and length.
void FsrEasuSetF(
	inout float2 dir,
	inout float len,
	float2 pp,
	bool biS,
	bool biT,
	bool biU,
	bool biV,
	float lA,
	float lB,
	float lC,
	float lD,
	float lE
) {
	// Compute bilinear weight, branches factor out as predicates are compiler time immediates.
	//  s t
	//  u v
	float w = 0;
	if (biS)
		w = (1 - pp.x) * (1 - pp.y);
	if (biT)
		w = pp.x * (1 - pp.y);
	if (biU)
		w = (1.0 - pp.x) * pp.y;
	if (biV)
		w = pp.x * pp.y;
	// Direction is the '+' diff.
	//    a
	//  b c d
	//    e
	// Then takes magnitude from abs average of both sides of 'c'.
	// Length converts gradient reversal to 0, smoothly to non-reversal at 1, shaped, then adding horz and vert terms.
	float dc = lD - lC;
	float cb = lC - lB;
	float lenX = max(abs(dc), abs(cb));
	lenX = rcp(lenX);
	float dirX = lD - lB;
	dir.x += dirX * w;
	lenX = saturate(abs(dirX) * lenX);
	lenX *= lenX;
	len += lenX * w;
	// Repeat for the y axis.
	float ec = lE - lC;
	float ca = lC - lA;
	float lenY = max(abs(ec), abs(ca));
	lenY = rcp(lenY);
	float dirY = lE - lA;
	dir.y += dirY * w;
	lenY = saturate(abs(dirY) * lenY);
	lenY *= lenY;
	len += lenY * w;
}

float3 FsrEasu(uint2 ip, float4 con0, float4 con1, float4 con2, float2 con3) {
//------------------------------------------------------------------------------------------------------------------------------
	// Get position of 'f'.
	float2 pp = ip * con0.xy + con0.zw;
	float2 fp = floor(pp);
	pp -= fp;
//------------------------------------------------------------------------------------------------------------------------------
	// 12-tap kernel.
	//    b c
	//  e f g h
	//  i j k l
	//    n o
	// Gather 4 ordering.
	//  a b
	//  r g
	// For packed FP16, need either {rg} or {ab} so using the following setup for gather in all versions,
	//    a b    <- unused (z)
	//    r g
	//  a b a b
	//  r g r g
	//    a b
	//    r g    <- unused (z)
	// Allowing dead-code removal to remove the 'z's.
	float2 p0 = fp * con1.xy + con1.zw;
	// These are from p0 to avoid pulling two constants on pre-Navi hardware.
	float2 p1 = p0 + con2.xy;
	float2 p2 = p0 + con2.zw;
	float2 p3 = p0 + con3;

	float4 bczzR = INPUT.GatherRed(sam, p0);
	float4 bczzG = INPUT.GatherGreen(sam, p0);
	float4 bczzB = INPUT.GatherBlue(sam, p0);
	float4 ijfeR = INPUT.GatherRed(sam, p1);
	float4 ijfeG = INPUT.GatherGreen(sam, p1);
	float4 ijfeB = INPUT.GatherBlue(sam, p1);
	float4 klhgR = INPUT.GatherRed(sam, p2);
	float4 klhgG = INPUT.GatherGreen(sam, p2);
	float4 klhgB = INPUT.GatherBlue(sam, p2);
	float4 zzonR = INPUT.GatherRed(sam, p3);
	float4 zzonG = INPUT.GatherGreen(sam, p3);
	float4 zzonB = INPUT.GatherBlue(sam, p3);
//------------------------------------------------------------------------------------------------------------------------------
	// Simplest multi-channel approximate luma possible (luma times 2, in 2 FMA/MAD).
	float4 bczzL = bczzB * 0.5 + (bczzR * 0.5 + bczzG);
	float4 ijfeL = ijfeB * 0.5 + (ijfeR * 0.5 + ijfeG);
	float4 klhgL = klhgB * 0.5 + (klhgR * 0.5 + klhgG);
	float4 zzonL = zzonB * 0.5 + (zzonR * 0.5 + zzonG);
	// Rename.
	float bL = bczzL.x;
	float cL = bczzL.y;
	float iL = ijfeL.x;
	float jL = ijfeL.y;
	float fL = ijfeL.z;
	float eL = ijfeL.w;
	float kL = klhgL.x;
	float lL = klhgL.y;
	float hL = klhgL.z;
	float gL = klhgL.w;
	float oL = zzonL.z;
	float nL = zzonL.w;
	// Accumulate for bilinear interpolation.
	float2 dir = 0;
	float len = 0;
	FsrEasuSetF(dir, len, pp, true, false, false, false, bL, eL, fL, gL, jL);
	FsrEasuSetF(dir, len, pp, false, true, false, false, cL, fL, gL, hL, kL);
	FsrEasuSetF(dir, len, pp, false, false, true, false, fL, iL, jL, kL, nL);
	FsrEasuSetF(dir, len, pp, false, false, false, true, gL, jL, kL, lL, oL);
//------------------------------------------------------------------------------------------------------------------------------
	// Normalize with approximation, and cleanup close to zero.
	float2 dir2 = dir * dir;
	float dirR = dir2.x + dir2.y;
	bool zro = dirR < 1.0f / 32768.0f;
	dirR = rsqrt(dirR);
	dirR = zro ? 1 : dirR;
	dir.x = zro ? 1 : dir.x;
	dir *= dirR;
	// Transform from {0 to 2} to {0 to 1} range, and shape with square.
	len = len * 0.5;
	len *= len;
	// Stretch kernel {1.0 vert|horz, to sqrt(2.0) on diagonal}.
	float stretch = (dir.x * dir.x + dir.y * dir.y) * rcp(max(abs(dir.x), abs(dir.y)));
	// Anisotropic length after rotation,
	//  x := 1.0 lerp to 'stretch' on edges
	//  y := 1.0 lerp to 2x on edges
	float2 len2 = { 1 + (stretch - 1) * len, 1 - 0.5 * len };
	// Based on the amount of 'edge',
	// the window shifts from +/-{sqrt(2.0) to slightly beyond 2.0}.
	float lob = 0.5 + ((1.0 / 4.0 - 0.04) - 0.5) * len;
	// Set distance^2 clipping point to the end of the adjustable window.
	float clp = rcp(lob);
//------------------------------------------------------------------------------------------------------------------------------
	// Accumulation mixed with min/max of 4 nearest.
	//    b c
	//  e f g h
	//  i j k l
	//    n o
	float3 min4 = min(min3(float3(ijfeR.z, ijfeG.z, ijfeB.z), float3(klhgR.w, klhgG.w, klhgB.w), float3(ijfeR.y, ijfeG.y, ijfeB.y)),
		float3(klhgR.x, klhgG.x, klhgB.x));
	float3 max4 = max(max3(float3(ijfeR.z, ijfeG.z, ijfeB.z), float3(klhgR.w, klhgG.w, klhgB.w), float3(ijfeR.y, ijfeG.y, ijfeB.y)),
		float3(klhgR.x, klhgG.x, klhgB.x));
	// Accumulation.
	float3 aC = 0;
	float aW = 0;
	FsrEasuTapF(aC, aW, float2(0.0, -1.0) - pp, dir, len2, lob, clp, float3(bczzR.x, bczzG.x, bczzB.x)); // b
	FsrEasuTapF(aC, aW, float2(1.0, -1.0) - pp, dir, len2, lob, clp, float3(bczzR.y, bczzG.y, bczzB.y)); // c
	FsrEasuTapF(aC, aW, float2(-1.0, 1.0) - pp, dir, len2, lob, clp, float3(ijfeR.x, ijfeG.x, ijfeB.x)); // i
	FsrEasuTapF(aC, aW, float2(0.0, 1.0) - pp, dir, len2, lob, clp, float3(ijfeR.y, ijfeG.y, ijfeB.y)); // j
	FsrEasuTapF(aC, aW, float2(0.0, 0.0) - pp, dir, len2, lob, clp, float3(ijfeR.z, ijfeG.z, ijfeB.z)); // f
	FsrEasuTapF(aC, aW, float2(-1.0, 0.0) - pp, dir, len2, lob, clp, float3(ijfeR.w, ijfeG.w, ijfeB.w)); // e
	FsrEasuTapF(aC, aW, float2(1.0, 1.0) - pp, dir, len2, lob, clp, float3(klhgR.x, klhgG.x, klhgB.x)); // k
	FsrEasuTapF(aC, aW, float2(2.0, 1.0) - pp, dir, len2, lob, clp, float3(klhgR.y, klhgG.y, klhgB.y)); // l
	FsrEasuTapF(aC, aW, float2(2.0, 0.0) - pp, dir, len2, lob, clp, float3(klhgR.z, klhgG.z, klhgB.z)); // h
	FsrEasuTapF(aC, aW, float2(1.0, 0.0) - pp, dir, len2, lob, clp, float3(klhgR.w, klhgG.w, klhgB.w)); // g
	FsrEasuTapF(aC, aW, float2(1.0, 2.0) - pp, dir, len2, lob, clp, float3(zzonR.z, zzonG.z, zzonB.z)); // o
	FsrEasuTapF(aC, aW, float2(0.0, 2.0) - pp, dir, len2, lob, clp, float3(zzonR.w, zzonG.w, zzonB.w)); // n
//------------------------------------------------------------------------------------------------------------------------------
	// Normalize and dering.
	return min(max4, max(min4, aC * rcp(aW)));
}

#endif

void Pass1(uint2 blockStart, uint3 threadId) {
	uint2 gxy = blockStart + Rmp8x8(threadId.x);

	const uint2 outputSize = GetOutputSize();
	if (gxy.x >= outputSize.x || gxy.y >= outputSize.y) {
		return;
	}

	uint2 inputSize = GetInputSize();
	float2 inputPt = GetInputPt();

	float4 con0, con1, con2;
	float2 con3;
	// Output integer position to a pixel position in viewport.
	con0[0] = (float)inputSize.x / (float)outputSize.x;
	con0[1] = (float)inputSize.y / (float)outputSize.y;
	con0[2] = 0.5f * con0[0] - 0.5f;
	con0[3] = 0.5f * con0[1] - 0.5f;
	// Viewport pixel position to normalized image space.
	// This is used to get upper-left of 'F' tap.
	con1[0] = inputPt.x;
	con1[1] = inputPt.y;
	// Centers of gather4, first offset from upper-left of 'F'.
	//      +---+---+
	//      |   |   |
	//      +--(0)--+
	//      | b | c |
	//  +---F---+---+---+
	//  | e | f | g | h |
	//  +--(1)--+--(2)--+
	//  | i | j | k | l |
	//  +---+---+---+---+
	//      | n | o |
	//      +--(3)--+
	//      |   |   |
	//      +---+---+
	con1[2] = inputPt.x;
	con1[3] = -inputPt.y;
	// These are from (0) instead of 'F'.
	con2[0] = -inputPt.x;
	con2[1] = 2.0f * inputPt.y;
	con2[2] = inputPt.x;
	con2[3] = 2.0f * inputPt.y;
	con3[0] = 0;
	con3[1] = 4.0f * inputPt.y;

	OUTPUT[gxy] = MF4(FsrEasu(gxy, con0, con1, con2, con3), 1);

	gxy.x += 8u;
	if (gxy.x < outputSize.x && gxy.y < outputSize.y) {
		OUTPUT[gxy] = MF4(FsrEasu(gxy, con0, con1, con2, con3), 1);
	}

	gxy.y += 8u;
	if (gxy.x < outputSize.x && gxy.y < outputSize.y) {
		OUTPUT[gxy] = MF4(FsrEasu(gxy, con0, con1, con2, con3), 1);
	}

	gxy.x -= 8u;
	if (gxy.x < outputSize.x && gxy.y < outputSize.y) {
		OUTPUT[gxy] = MF4(FsrEasu(gxy, con0, con1, con2, con3), 1);
	}
}
