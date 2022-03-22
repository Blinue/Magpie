// This is a combination of linear interpolation and light version of cas

//!MAGPIE EFFECT
//!VERSION 2

//!PARAMETER
//!DEFAULT 0.4
//!MIN 0
//!MAX 1
float sharpness;

//!TEXTURE
Texture2D INPUT;

//!SAMPLER
//!FILTER LINEAR
SamplerState sam;

//!PASS 1
//!IN INPUT
//!BLOCK_SIZE 8
//!NUM_THREADS 64

void Pass1(uint2 blockStart, uint3 threadId) {
	uint2 gxy = Rmp8x8(threadId.x) + blockStart;

	if (!CheckViewport(gxy)) {
		return;
	}
			
      float2 pos = (gxy + 0.5f) * GetOutputPt();
      float2 inputPt = GetInputPt();

	// fetch a 3x3 neighborhood around the pixel 'e',
	//	a b c
	//	d(e)f
	//	g h i
	float3 a = INPUT.SampleLevel(sam, pos + float2(-inputPt.x, -inputPt.y), 0).rgb;
	float3 b = INPUT.SampleLevel(sam, pos + float2(0, -inputPt.y), 0).rgb;
	float3 c = INPUT.SampleLevel(sam, pos + float2(inputPt.x, -inputPt.y), 0).rgb;
	float3 d = INPUT.SampleLevel(sam, pos + float2(-inputPt.x, 0), 0).rgb;
	float3 f = INPUT.SampleLevel(sam, pos + float2(inputPt.x, 0), 0).rgb;
	float3 g = INPUT.SampleLevel(sam, pos + float2(-inputPt.x, inputPt.y), 0).rgb;
	float3 h = INPUT.SampleLevel(sam, pos + float2(0, inputPt.y), 0).rgb;
	float3 i = INPUT.SampleLevel(sam, pos + float2(inputPt.x, inputPt.y), 0).rgb;
	
	float3 e = INPUT.SampleLevel(sam, pos + float2(inputPt.x, inputPt.y) * 0.36, 0).rgb;
	e += INPUT.SampleLevel(sam, pos + float2(-inputPt.x, -inputPt.y) * 0.36, 0).rgb;
	e += INPUT.SampleLevel(sam, pos + float2(inputPt.x, -inputPt.y) * 0.36, 0).rgb;
	e += INPUT.SampleLevel(sam, pos + float2(-inputPt.x, inputPt.y) * 0.36, 0).rgb;
	e /= 4;

	// Soft min and max.
	//  a b c
	//  d e f
	//  g h i
	float3 mnRGB = min(min(min(min(d, e), min(f, b)), h), min(min(a, i), min(c, g)));

	float3 mxRGB = max(max(max(max(d, e), max(f, b)), h), max(max(a, i), max(c, g)));

	// Shaping amount of sharpening.
	float3 wRGB = sqrt(min(mnRGB, 1.0 - mxRGB) / mxRGB) * lerp(-0.0625, -0.1, sharpness);

	// Filter shape.
	//  w w w 
	//  w 1 w
	//  w w w 
	float3 color = ((a + b + c + d + f + g + h + i) * wRGB + (e * 3 - (a + c + g + i + (b + d + f + h) * 2 + e * 4) / 8)) / (1.0 + 8.0 * wRGB);
	WriteToOutput(gxy, (color + clamp(color, mnRGB, mxRGB) * 3) / 4);
}
