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
//!DEFAULT 0.5
//!MIN 0
//!MAX 1
float sharpness;

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
        
	// Edge checker
        float3 sum = (abs(e - b) - 0.5f) * 1.2 + 0.5f;
	sum += (abs(e - f) - 0.5f) * 1.2 + 0.5f;
	sum += (abs(e - h) - 0.5f) * 1.2 + 0.5f;
	sum += (abs(e - d) - 0.5f) * 1.2 + 0.5f;
	
	// Soft min and max.
	//    b
	//  d e f
	//    h

	float3 mnRGB = min(min(min(d, e), min(f, b)), h);

	float3 mxRGB = max(max(max(d, e), max(f, b)), h);

	// Shaping amount of sharpening.
	float3 wRGB = sqrt(min(mnRGB, 1.0 - mxRGB) / mxRGB) * lerp(- 0.125, - 0.175, sharpness);

	// Filter shape.
	//    w  
	//  w 1 w
	//    w   
	// If is not edge
	if(dot(0.0721f ,sum.g) <= 0.1)
		return float4(((((b + d) + (f + h)) * wRGB + e) / (1.0 + 4.0 * wRGB)).rgb, 1);
	else
		return float4(((((b + d) + (f + h)) * wRGB + e * (1 + sharpness * 2)) / (1.0 + 4.0 * wRGB)).rgb, 1);
	// If is edge
}
