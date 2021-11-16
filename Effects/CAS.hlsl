// 对比度自适应锐化

//!MAGPIE EFFECT
//!VERSION 1
//!OUTPUT_WIDTH INPUT_WIDTH
//!OUTPUT_HEIGHT INPUT_HEIGHT

//!CONSTANT
//!VALUE INPUT_PT_X
float inputPtX;

//!CONSTANT
//!VALUE INPUT_PT_Y
float inputPtY;


//!CONSTANT
//!DEFAULT 0.4
//!MIN 0
//!MAX 1
float sharpness;

//!TEXTURE
Texture2D INPUT;

//!SAMPLER
//!FILTER POINT
SamplerState sam;


//!PASS 1
//!BIND INPUT

float4 Pass1(float2 pos) {
	// fetch a 3x3 neighborhood around the pixel 'e',
	//	a b c
	//	d(e)f
	//	g h i
	float3 a = INPUT.Sample(sam, pos + float2(-inputPtX, -inputPtY)).rgb;
	float3 b = INPUT.Sample(sam, pos + float2(0, -inputPtY)).rgb;
	float3 c = INPUT.Sample(sam, pos + float2(inputPtX, -inputPtY)).rgb;
	float3 d = INPUT.Sample(sam, pos + float2(-inputPtX, 0)).rgb;
	float3 e = INPUT.Sample(sam, pos).rgb;
	float3 f = INPUT.Sample(sam, pos + float2(inputPtX, 0)).rgb;
	float3 g = INPUT.Sample(sam, pos + float2(-inputPtX, inputPtY)).rgb;
	float3 h = INPUT.Sample(sam, pos + float2(0, inputPtY)).rgb;
	float3 i = INPUT.Sample(sam, pos + float2(inputPtX, inputPtY)).rgb;

	// Soft min and max.
	//	a b c			  b
	//	d e f * 0.5	 +	d e f * 0.5
	//	g h i			  h
	// These are 2.0x bigger (factored out the extra multiply).
	float3 mnRGB = min(min(min(d, e), min(f, b)), h);
	float3 mnRGB2 = min(mnRGB, min(min(a, c), min(g, i)));
	mnRGB += mnRGB2;

	float3 mxRGB = max(max(max(d, e), max(f, b)), h);
	float3 mxRGB2 = max(mxRGB, max(max(a, c), max(g, i)));
	mxRGB += mxRGB2;

	// Smooth minimum distance to signal limit divided by smooth max.
	float3 ampRGB = saturate(min(mnRGB, 2.0 - mxRGB) / mxRGB);

	float peak = -1.0 / lerp(8.0, 5.0, sharpness);
	// Shaping amount of sharpening.
	float3 wRGB = sqrt(ampRGB) * peak;

	// Filter shape.
	//  0 w 0
	//  w 1 w
	//  0 w 0  
	float3 weightRGB = 1.0 + 4.0 * wRGB;
	float3 window = (b + d) + (f + h);
	return float4(saturate((window * wRGB + e) / weightRGB).rgb, 1);
}
