// 移植自 https://github.com/libretro/common-shaders/tree/master/xbrz/shaders/xbrz-freescale-multipass

//!MAGPIE EFFECT
//!VERSION 1


//!CONSTANT
//!VALUE INPUT_PT_X
float inputPtX;

//!CONSTANT
//!VALUE INPUT_PT_Y
float inputPtY;

//!CONSTANT
//!VALUE SCALE_X
float scaleX;

//!CONSTANT
//!VALUE SCALE_Y
float scaleY;

//!TEXTURE
Texture2D INPUT;

//!TEXTURE
//!WIDTH INPUT_WIDTH
//!HEIGHT INPUT_HEIGHT
//!FORMAT B8G8R8A8_UNORM
Texture2D tex1;

//!SAMPLER
//!FILTER POINT
SamplerState sam;


//!COMMON

#define BLEND_NONE 0
#define BLEND_NORMAL 1
#define BLEND_DOMINANT 2
#define LUMINANCE_WEIGHT 1.0
#define EQUAL_COLOR_TOLERANCE 30.0/255.0
#define STEEP_DIRECTION_THRESHOLD 2.2
#define DOMINANT_DIRECTION_THRESHOLD 3.6

float DistYCbCr(float3 pixA, float3 pixB) {
	const float3 w = float3(0.2627, 0.6780, 0.0593);
	const float scaleB = 0.5 / (1.0 - w.b);
	const float scaleR = 0.5 / (1.0 - w.r);
	float3 diff = pixA - pixB;
	float Y = dot(diff.rgb, w);
	float Cb = scaleB * (diff.b - Y);
	float Cr = scaleR * (diff.r - Y);

	return sqrt(((LUMINANCE_WEIGHT * Y) * (LUMINANCE_WEIGHT * Y)) + (Cb * Cb) + (Cr * Cr));
}

bool IsPixEqual(const float3 pixA, const float3 pixB) {
	return (DistYCbCr(pixA, pixB) < EQUAL_COLOR_TOLERANCE);
}

float get_left_ratio(float2 center, float2 origin, float2 direction, float2 scale) {
	float2 P0 = center - origin;
	float2 proj = direction * (dot(P0, direction) / dot(direction, direction));
	float2 distv = P0 - proj;
	float2 orth = float2(-direction.y, direction.x);
	float side = sign(dot(P0, orth));
	float v = side * length(distv * scale);

	//  return step(0, v);
	return smoothstep(-sqrt(2.0) / 2.0, sqrt(2.0) / 2.0, v);
}

//#define eq(a,b)  (a == b)
bool eq(float3 a, float3 b) {
	return ((a.x == b.x) && (a.y == b.y) && (a.z == b.z));
}
//#define neq(a,b) (a != b)
bool neq(float3 a, float3 b) {
	return !((a.x == b.x) && (a.y == b.y) && (a.z == b.z));
}


//!PASS 1
//!BIND INPUT
//!SAVE tex1

#define P(x,y) INPUT.Sample(sam, pos + float2(inputPtX, inputPtY) * float2(x, y)).rgb

float4 Pass1(float2 pos) {
	//---------------------------------------
	// Input Pixel Mapping:  -|x|x|x|-
	//                       x|A|B|C|x
	//                       x|D|E|F|x
	//                       x|G|H|I|x
	//                       -|x|x|x|-

	float3 A = P(-1., -1.);
	float3 B = P(0., -1.);
	float3 C = P(1., -1.);
	float3 D = P(-1., 0.);
	float3 E = P(0., 0.);
	float3 F = P(1., 0.);
	float3 G = P(-1., 1.);
	float3 H = P(0., 1.);
	float3 I = P(1., 1.);

	// blendResult Mapping: x|y|
	//                      w|z|
	int4 blendResult = int4(BLEND_NONE, BLEND_NONE, BLEND_NONE, BLEND_NONE);

	// Preprocess corners
	// Pixel Tap Mapping: -|-|-|-|-
	//                    -|-|B|C|-
	//                    -|D|E|F|x
	//                    -|G|H|I|x
	//                    -|-|x|x|-
	if (!((eq(E, F) && eq(H, I)) || (eq(E, H) && eq(F, I)))) {
		float dist_H_F = DistYCbCr(G, E) + DistYCbCr(E, C) + DistYCbCr(P(0., 2.), I) + DistYCbCr(I, P(2., 0.)) + (4.0 * DistYCbCr(H, F));
		float dist_E_I = DistYCbCr(D, H) + DistYCbCr(H, P(1., 2.)) + DistYCbCr(B, F) + DistYCbCr(F, P(2., 1.)) + (4.0 * DistYCbCr(E, I));
		bool dominantGradient = (DOMINANT_DIRECTION_THRESHOLD * dist_H_F) < dist_E_I;
		blendResult.z = ((dist_H_F < dist_E_I) && neq(E, F) && neq(E, H)) ? ((dominantGradient) ? BLEND_DOMINANT : BLEND_NORMAL) : BLEND_NONE;
	}


	// Pixel Tap Mapping: -|-|-|-|-
	//                    -|A|B|-|-
	//                    x|D|E|F|-
	//                    x|G|H|I|-
	//                    -|x|x|-|-
	if (!((eq(D, E) && eq(G, H)) || (eq(D, G) && eq(E, H)))) {
		float dist_G_E = DistYCbCr(P(-2., 1.), D) + DistYCbCr(D, B) + DistYCbCr(P(-1., 2.), H) + DistYCbCr(H, F) + (4.0 * DistYCbCr(G, E));
		float dist_D_H = DistYCbCr(P(-2., 0.), G) + DistYCbCr(G, P(0., 2.)) + DistYCbCr(A, E) + DistYCbCr(E, I) + (4.0 * DistYCbCr(D, H));
		bool dominantGradient = (DOMINANT_DIRECTION_THRESHOLD * dist_D_H) < dist_G_E;
		blendResult.w = ((dist_G_E > dist_D_H) && neq(E, D) && neq(E, H)) ? ((dominantGradient) ? BLEND_DOMINANT : BLEND_NORMAL) : BLEND_NONE;
	}

	// Pixel Tap Mapping: -|-|x|x|-
	//                    -|A|B|C|x
	//                    -|D|E|F|x
	//                    -|-|H|I|-
	//                    -|-|-|-|-
	if (!((eq(B, C) && eq(E, F)) || (eq(B, E) && eq(C, F)))) {
		float dist_E_C = DistYCbCr(D, B) + DistYCbCr(B, P(1, -2)) + DistYCbCr(H, F) + DistYCbCr(F, P(2., -1.)) + (4.0 * DistYCbCr(E, C));
		float dist_B_F = DistYCbCr(A, E) + DistYCbCr(E, I) + DistYCbCr(P(0., -2.), C) + DistYCbCr(C, P(2., 0.)) + (4.0 * DistYCbCr(B, F));
		bool dominantGradient = (DOMINANT_DIRECTION_THRESHOLD * dist_B_F) < dist_E_C;
		blendResult.y = ((dist_E_C > dist_B_F) && neq(E, B) && neq(E, F)) ? ((dominantGradient) ? BLEND_DOMINANT : BLEND_NORMAL) : BLEND_NONE;
	}

	// Pixel Tap Mapping: -|x|x|-|-
	//                    x|A|B|C|-
	//                    x|D|E|F|-
	//                    -|G|H|-|-
	//                    -|-|-|-|-
	if (!((eq(A, B) && eq(D, E)) || (eq(A, D) && eq(B, E)))) {
		float dist_D_B = DistYCbCr(P(-2., 0.), A) + DistYCbCr(A, P(0., -2.)) + DistYCbCr(G, E) + DistYCbCr(E, C) + (4.0 * DistYCbCr(D, B));
		float dist_A_E = DistYCbCr(P(-2., -1.), D) + DistYCbCr(D, H) + DistYCbCr(P(-1., -2.), B) + DistYCbCr(B, F) + (4.0 * DistYCbCr(A, E));
		bool dominantGradient = (DOMINANT_DIRECTION_THRESHOLD * dist_D_B) < dist_A_E;
		blendResult.x = ((dist_D_B < dist_A_E) && neq(E, D) && neq(E, B)) ? ((dominantGradient) ? BLEND_DOMINANT : BLEND_NORMAL) : BLEND_NONE;
	}

	float4 FragColor = float4(blendResult);

	// Pixel Tap Mapping: -|-|-|-|-
	//                    -|-|B|C|-
	//                    -|D|E|F|x
	//                    -|G|H|I|x
	//                    -|-|x|x|-
	if (blendResult.z == BLEND_DOMINANT || (blendResult.z == BLEND_NORMAL &&
		!((blendResult.y != BLEND_NONE && !IsPixEqual(E, G)) || (blendResult.w != BLEND_NONE && !IsPixEqual(E, C)) ||
			(IsPixEqual(G, H) && IsPixEqual(H, I) && IsPixEqual(I, F) && IsPixEqual(F, C) && !IsPixEqual(E, I))))) {
		FragColor.z += 4.0;

		float dist_F_G = DistYCbCr(F, G);
		float dist_H_C = DistYCbCr(H, C);

		if ((STEEP_DIRECTION_THRESHOLD * dist_F_G <= dist_H_C) && neq(E, G) && neq(D, G))
			FragColor.z += 16.0;

		if ((STEEP_DIRECTION_THRESHOLD * dist_H_C <= dist_F_G) && neq(E, C) && neq(B, C))
			FragColor.z += 64.0;
	}

	// Pixel Tap Mapping: -|-|-|-|-
	//                    -|A|B|-|-
	//                    x|D|E|F|-
	//                    x|G|H|I|-
	//                    -|x|x|-|-
	if (blendResult.w == BLEND_DOMINANT || (blendResult.w == BLEND_NORMAL &&
		!((blendResult.z != BLEND_NONE && !IsPixEqual(E, A)) || (blendResult.x != BLEND_NONE && !IsPixEqual(E, I)) ||
			(IsPixEqual(A, D) && IsPixEqual(D, G) && IsPixEqual(G, H) && IsPixEqual(H, I) && !IsPixEqual(E, G))))) {
		FragColor.w += 4.0;

		float dist_H_A = DistYCbCr(H, A);
		float dist_D_I = DistYCbCr(D, I);

		if ((STEEP_DIRECTION_THRESHOLD * dist_H_A <= dist_D_I) && neq(E, A) && neq(B, A))
			FragColor.w += 16.0;

		if ((STEEP_DIRECTION_THRESHOLD * dist_D_I <= dist_H_A) && neq(E, I) && neq(F, I))
			FragColor.w += 64.0;
	}

	// Pixel Tap Mapping: -|-|x|x|-
	//                    -|A|B|C|x
	//                    -|D|E|F|x
	//                    -|-|H|I|-
	//                    -|-|-|-|-
	if (blendResult.y == BLEND_DOMINANT || (blendResult.y == BLEND_NORMAL &&
		!((blendResult.x != BLEND_NONE && !IsPixEqual(E, I)) || (blendResult.z != BLEND_NONE && !IsPixEqual(E, A)) ||
			(IsPixEqual(I, F) && IsPixEqual(F, C) && IsPixEqual(C, B) && IsPixEqual(B, A) && !IsPixEqual(E, C))))) {
		FragColor.y += 4.0;

		float dist_B_I = DistYCbCr(B, I);
		float dist_F_A = DistYCbCr(F, A);

		if ((STEEP_DIRECTION_THRESHOLD * dist_B_I <= dist_F_A) && neq(E, I) && neq(H, I))
			FragColor.y += 16.0;

		if ((STEEP_DIRECTION_THRESHOLD * dist_F_A <= dist_B_I) && neq(E, A) && neq(D, A))
			FragColor.y += 64.0;
	}

	// Pixel Tap Mapping: -|x|x|-|-
	//                    x|A|B|C|-
	//                    x|D|E|F|-
	//                    -|G|H|-|-
	//                    -|-|-|-|-
	if (blendResult.x == BLEND_DOMINANT || (blendResult.x == BLEND_NORMAL &&
		!((blendResult.w != BLEND_NONE && !IsPixEqual(E, C)) || (blendResult.y != BLEND_NONE && !IsPixEqual(E, G)) ||
			(IsPixEqual(C, B) && IsPixEqual(B, A) && IsPixEqual(A, D) && IsPixEqual(D, G) && !IsPixEqual(E, A))))) {
		FragColor.x += 4.0;

		float dist_D_C = DistYCbCr(D, C);
		float dist_B_G = DistYCbCr(B, G);

		if ((STEEP_DIRECTION_THRESHOLD * dist_D_C <= dist_B_G) && neq(E, C) && neq(F, C))
			FragColor.x += 16.0;

		if ((STEEP_DIRECTION_THRESHOLD * dist_B_G <= dist_D_C) && neq(E, G) && neq(H, G))
			FragColor.x += 64.0;
	}
	
	return FragColor / 255;
}


//!PASS 2
//!BIND INPUT, tex1

#define P(x,y) INPUT.Sample(sam, pos + float2(inputPtX, inputPtY) * float2(x, y)).rgb

float4 Pass2(float2 pos) {
	
	//---------------------------------------
	// Input Pixel Mapping: -|B|-
	//                      D|E|F
	//                      -|H|-

	float2 scale = float2(scaleX, scaleY);
	float2 f = frac(pos / float2(inputPtX, inputPtY)) - float2(0.5, 0.5);

	float3 B = P(0., -1.);
	float3 D = P(-1., 0.);
	float3 E = P(0., 0.);
	float3 F = P(1., 0.);
	float3 H = P(0., 1.);

	float4 info = floor(tex1.Sample(sam, pos) * 255 + 0.5);

	// info Mapping: x|y|
	//               w|z|

	float4 blendResult = floor(fmod(info, 4.0));
	float4 doLineBlend = floor(fmod(info / 4.0, 4.0));
	float4 haveShallowLine = floor(fmod(info / 16.0, 4.0));
	float4 haveSteepLine = floor(fmod(info / 64.0, 4.0));

	float3 res = E;

	// Pixel Tap Mapping: -|-|-
	//                    -|E|F
	//                    -|H|-

	if (blendResult.z > BLEND_NONE) {
		float2 origin = float2(0.0, 1.0 / sqrt(2.0));
		float2 direction = float2(1.0, -1.0);
		if (doLineBlend.z > 0.0) {
			origin = haveShallowLine.z > 0.0 ? float2(0.0, 0.25) : float2(0.0, 0.5);
			direction.x += haveShallowLine.z;
			direction.y -= haveSteepLine.z;
		}

		float3 blendPix = lerp(H, F, step(DistYCbCr(E, F), DistYCbCr(E, H)));
		res = lerp(res, blendPix, get_left_ratio(f, origin, direction, scale));
	}

	// Pixel Tap Mapping: -|-|-
	//                    D|E|-
	//                    -|H|-
	if (blendResult.w > BLEND_NONE) {
		float2 origin = float2(-1.0 / sqrt(2.0), 0.0);
		float2 direction = float2(1.0, 1.0);
		if (doLineBlend.w > 0.0) {
			origin = haveShallowLine.w > 0.0 ? float2(-0.25, 0.0) : float2(-0.5, 0.0);
			direction.y += haveShallowLine.w;
			direction.x += haveSteepLine.w;
		}

		float3 blendPix = lerp(H, D, step(DistYCbCr(E, D), DistYCbCr(E, H)));
		res = lerp(res, blendPix, get_left_ratio(f, origin, direction, scale));
	}

	// Pixel Tap Mapping: -|B|-
	//                    -|E|F
	//                    -|-|-
	if (blendResult.y > BLEND_NONE) {
		float2 origin = float2(1.0 / sqrt(2.0), 0.0);
		float2 direction = float2(-1.0, -1.0);

		if (doLineBlend.y > 0.0) {
			origin = haveShallowLine.y > 0.0 ? float2(0.25, 0.0) : float2(0.5, 0.0);
			direction.y -= haveShallowLine.y;
			direction.x -= haveSteepLine.y;
		}

		float3 blendPix = lerp(F, B, step(DistYCbCr(E, B), DistYCbCr(E, F)));
		res = lerp(res, blendPix, get_left_ratio(f, origin, direction, scale));
	}

	// Pixel Tap Mapping: -|B|-
	//                    D|E|-
	//                    -|-|-
	if (blendResult.x > BLEND_NONE) {
		float2 origin = float2(0.0, -1.0 / sqrt(2.0));
		float2 direction = float2(-1.0, 1.0);
		if (doLineBlend.x > 0.0) {
			origin = haveShallowLine.x > 0.0 ? float2(0.0, -0.25) : float2(0.0, -0.5);
			direction.x -= haveShallowLine.x;
			direction.y += haveSteepLine.x;
		}

		float3 blendPix = lerp(D, B, step(DistYCbCr(E, B), DistYCbCr(E, D)));
		res = lerp(res, blendPix, get_left_ratio(f, origin, direction, scale));
	}

	return float4(res, 1.0);
}
