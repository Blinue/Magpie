// This is a combination of linear interpolation and light version of cas

//!MAGPIE EFFECT
//!VERSION 2

//!PARAMETER
//!DEFAULT 0.4
//!MIN 0
//!MAX 1
float sharpness;

//!PARAMETER
//!DEFAULT 0.2
//!MIN 0
//!MAX 1
float threshold;

//!TEXTURE
Texture2D INPUT;

//!SAMPLER
//!FILTER LINEAR
SamplerState sam;

//!PASS 1
//!IN INPUT
//!BLOCK_SIZE 16
//!NUM_THREADS 64

void Pass1(uint2 blockStart, uint3 threadId) {
	uint2 gxy = (Rmp8x8(threadId.x) << 1) + blockStart;

	if (!CheckViewport(gxy)) {
		return;
	}

        float2 inputPt = GetInputPt();
	float2 OutputPt = GetOutputPt();
	uint i, j;

	[unroll]
	for (i = 0; i <= 1; ++i) {
		[unroll]
		for (j = 0; j <= 1; ++j) {
			uint2 destPos = gxy + uint2(i, j);

			if (i != 0 && j != 0) {
				if (!CheckViewport(gxy)) {
					continue;
				}
			}

			float2 pos = destPos * OutputPt;

			// fetch a 4 neighborhood pixels around the pixel 'e',
			//	  b 
			//	d(e)f
			//	  h 
			float3 b = INPUT.SampleLevel(sam, pos + float2(0, -inputPt.y), 0).rgb;
			float3 d = INPUT.SampleLevel(sam, pos + float2(-inputPt.x, 0), 0).rgb;
			float3 e = INPUT.SampleLevel(sam, pos, 0).rgb;
			float3 f = INPUT.SampleLevel(sam, pos + float2(inputPt.x, 0), 0).rgb;
			float3 h = INPUT.SampleLevel(sam, pos + float2(0, inputPt.y), 0).rgb;

			// Edge checker.
			float edge = length(abs(d - f) + abs(b - h));
			
			e = (e * 8 + b + d + f + h) / 12;

			// Soft min and max.
			//    b
			//  d e f
			//    h
			float3 mnRGB = min(min(min(d, e), min(f, b)), h);

			float3 mxRGB = max(max(max(d, e), max(f, b)), h);

			// Shaping amount of sharpening.
			float3 wRGB = sqrt(min(mnRGB, 1.0 - mxRGB) / mxRGB) * lerp(-0.125, -0.2, sharpness);

			// Sharpen edge and mask.
			e = lerp((e * 3 - (b + d + f + h + e * 2) / 3), (e * 3 - (b + d + f + h) * 0.5), (edge >= threshold));

			// Filter shape.
			//    w  
			//  w 1 w
			//    w   
			float3 c = ((b + d + f + h) * wRGB + e) / (1.0 + 4.0 * wRGB);
			WriteToOutput(destPos, c);
		}
	}
}
