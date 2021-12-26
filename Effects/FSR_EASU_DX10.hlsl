// 适用于 DirectX 功能级别 10 的 FSR_EASU
// 比原始版本稍慢

//!MAGPIE EFFECT
//!VERSION 1


//!CONSTANT
//!VALUE INPUT_PT_X
float inputPtX;

//!CONSTANT
//!VALUE INPUT_PT_Y
float inputPtY;

//!CONSTANT
//!VALUE 1 / SCALE_X
float rcpScaleX;

//!CONSTANT
//!VALUE 1 / SCALE_Y
float rcpScaleY;

//!CONSTANT
//!VALUE OUTPUT_WIDTH
float outputWidth;

//!CONSTANT
//!VALUE OUTPUT_HEIGHT
float outputHeight;

//!TEXTURE
Texture2D INPUT;

//!SAMPLER
//!FILTER POINT
SamplerState sam;


//!PASS 1
//!BIND INPUT

#define min3(a, b, c) min(a, min(b, c))
#define max3(a, b, c) max(a, max(b, c))

// Filtering for a given tap for the scalar.
void FsrEasuTap(
	inout float3 aC, // Accumulated color, with negative lobe.
	inout float aW, // Accumulated weight.
	float2 off, // Pixel offset from resolve position to tap.
	float2 dir, // Gradient direction.
	float2 len, // Length.
	float lob, // Negative lobe strength.
	float clp, // Clipping point.
	float3 c  // Tap color.
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
	aC += c * w; aW += w;
}

// Accumulate direction and length.
void FsrEasuSet(
	inout float2 dir,
	inout float len,
	float2 pp,
	bool biS, bool biT, bool biU, bool biV,
	float lA, float lB, float lC, float lD, float lE) {
	// Compute bilinear weight, branches factor out as predicates are compiler time immediates.
	//  s t
	//  u v
	float w = 0;
	if (biS)w = (1 - pp.x) * (1 - pp.y);
	if (biT)w = pp.x * (1 - pp.y);
	if (biU)w = (1.0 - pp.x) * pp.y;
	if (biV)w = pp.x * pp.y;
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

#define Src(x, y) INPUT.Sample(sam, pos + float2(x * inputPtX, y * inputPtY)).rgb;

float4 Pass1(float2 pos) {

	//------------------------------------------------------------------------------------------------------------------------------
	  // Get position of 'f'.
	float2 pp = (floor(pos * float2(outputWidth, outputHeight)) + 0.5f) * float2(rcpScaleX, rcpScaleY) - 0.5f;
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
	pos = (fp + 0.5f) * float2(inputPtX, inputPtY);

	float3 bc = Src(0, -1);
	float3 cc = Src(1, -1);
	float3 ec = Src(-1, 0);
	float3 fc = Src(0, 0);
	float3 gc = Src(1, 0);
	float3 hc = Src(2, 0);
	float3 ic = Src(-1, 1);
	float3 jc = Src(0, 1);
	float3 kc = Src(1, 1);
	float3 lc = Src(2, 1);
	float3 nc = Src(0, 2);
	float3 oc = Src(1, 2);

	//------------------------------------------------------------------------------------------------------------------------------

	// Rename.
	float bL = bc.b * 0.5 + (bc.r * 0.5 + bc.g);
	float cL = cc.b * 0.5 + (cc.r * 0.5 + cc.g);
	float iL = ic.b * 0.5 + (ic.r * 0.5 + ic.g);
	float jL = jc.b * 0.5 + (jc.r * 0.5 + jc.g);
	float fL = fc.b * 0.5 + (fc.r * 0.5 + fc.g);
	float eL = ec.b * 0.5 + (ec.r * 0.5 + ec.g);
	float kL = kc.b * 0.5 + (kc.r * 0.5 + kc.g);
	float lL = lc.b * 0.5 + (lc.r * 0.5 + lc.g);
	float hL = hc.b * 0.5 + (hc.r * 0.5 + hc.g);
	float gL = gc.b * 0.5 + (gc.r * 0.5 + gc.g);
	float oL = oc.b * 0.5 + (oc.r * 0.5 + oc.g);
	float nL = nc.b * 0.5 + (nc.r * 0.5 + nc.g);
	// Accumulate for bilinear interpolation.
	float2 dir = 0;
	float len = 0;
	FsrEasuSet(dir, len, pp, true, false, false, false, bL, eL, fL, gL, jL);
	FsrEasuSet(dir, len, pp, false, true, false, false, cL, fL, gL, hL, kL);
	FsrEasuSet(dir, len, pp, false, false, true, false, fL, iL, jL, kL, nL);
	FsrEasuSet(dir, len, pp, false, false, false, true, gL, jL, kL, lL, oL);
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
	float3 min4 = min(min3(fc, gc, jc), kc);
	float3 max4 = max(max3(fc, gc, jc), kc);
	// Accumulation.
	float3 aC = 0;
	float aW = 0;
	FsrEasuTap(aC, aW, float2(0.0, -1.0) - pp, dir, len2, lob, clp, bc); // b
	FsrEasuTap(aC, aW, float2(1.0, -1.0) - pp, dir, len2, lob, clp, cc); // c
	FsrEasuTap(aC, aW, float2(-1.0, 1.0) - pp, dir, len2, lob, clp, ic); // i
	FsrEasuTap(aC, aW, float2(0.0, 1.0) - pp, dir, len2, lob, clp, jc); // j
	FsrEasuTap(aC, aW, float2(0.0, 0.0) - pp, dir, len2, lob, clp, fc); // f
	FsrEasuTap(aC, aW, float2(-1.0, 0.0) - pp, dir, len2, lob, clp, ec); // e
	FsrEasuTap(aC, aW, float2(1.0, 1.0) - pp, dir, len2, lob, clp, kc); // k
	FsrEasuTap(aC, aW, float2(2.0, 1.0) - pp, dir, len2, lob, clp, lc); // l
	FsrEasuTap(aC, aW, float2(2.0, 0.0) - pp, dir, len2, lob, clp, hc); // h
	FsrEasuTap(aC, aW, float2(1.0, 0.0) - pp, dir, len2, lob, clp, gc); // g
	FsrEasuTap(aC, aW, float2(1.0, 2.0) - pp, dir, len2, lob, clp, oc); // o
	FsrEasuTap(aC, aW, float2(0.0, 2.0) - pp, dir, len2, lob, clp, nc); // n
  //------------------------------------------------------------------------------------------------------------------------------
	// Normalize and dering.
	float3 c = min(max4, max(min4, aC * rcp(aW)));

	return float4(c, 1.0f);
}
