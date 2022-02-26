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

//!CONSTANT
//!DEFAULT 0.2
//!MIN 0
//!MAX 1
float sharpness;

//!CONSTANT
//!DEFAULT 0.2
//!MIN 0
//!MAX 1
float threshold;

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
        
	e = (b + d + f + h) * 0.05 + e * 0.8;
	
	// Edge checker
        float edge = length(abs(d - f) + abs(b - h));
	
	// Soft min and max.
	//    b
	//  d e f
	//    h

	float3 mnRGB = min(min(min(d, e), min(f, b)), h);

	float3 mxRGB = max(max(max(d, e), max(f, b)), h);

	// Shaping amount of sharpening.
	float3 wRGB = sqrt(min(mnRGB, 1.0 - mxRGB) / mxRGB) * lerp(- 0.125, - 0.2, sharpness);

	// Filter shape.
	//    w  
	//  w 1 w
	//    w   
	// If is edge
	if(edge >= threshold)
		return float4((((b + d + f + h) * wRGB + (e * 2 - (b + d + f + h) * 0.25)) / (1.0 + 4.0 * wRGB)).rgb, 1);
	else
		return float4((((b + d + f + h) * wRGB + (e * 3 - (b + d + f + h + e * 2) / 3)) / (1.0 + 4.0 * wRGB)).rgb, 1);
	// If is not edge
}
