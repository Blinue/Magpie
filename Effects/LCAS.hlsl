// This is a combination of linear interpolation and light version of cas

//!MAGPIE EFFECT
//!VERSION 1

//!CONSTANT
//!VALUE INPUT_PT_X
float inputPtX;

//!CONSTANT
//!VALUE INPUT_PT_Y
float inputPtY;

//!TEXTURE
Texture2D INPUT;

//!SAMPLER
//!FILTER LINEAR
SamplerState sam;


//!PASS 1
//!BIND INPUT

float4 Pass1(float2 pos) {
	// fetch a 4 neighborhood pixels around the pixel 'e',
	//	  b 
	//	d(e)f
	//	  h 
	float3 b = INPUT.Sample(sam, pos + float2(0, -inputPtY)).rgb;
	float3 d = INPUT.Sample(sam, pos + float2(-inputPtX, 0)).rgb;
	float3 e = INPUT.Sample(sam, pos).rgb;
	float3 f = INPUT.Sample(sam, pos + float2(inputPtX, 0)).rgb;
	float3 h = INPUT.Sample(sam, pos + float2(0, inputPtY)).rgb;

	// Soft min and max.
	//    b
	//	d e f
	//    h

	float3 mnRGB = min(min(min(d, e), min(f, b)), h);

	float3 mxRGB = max(max(max(d, e), max(f, b)), h);

	// Shaping amount of sharpening.
	float3 wRGB = sqrt(min(mnRGB, 1.0 - mxRGB) / mxRGB) * -0.13665928043115600587312498717265825;

	// Filter shape.
	//    w  
	//  w 1 w
	//    w   
	return float4(((((b + d) + (f + h)) * wRGB + e) / (1.0 + 4.0 * wRGB)).rgb, 1);
}
