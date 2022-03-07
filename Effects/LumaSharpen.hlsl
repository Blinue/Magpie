// LumaSharpen
// 移植自 https://github.com/CeeJayDK/SweetFX/blob/master/Shaders/LumaSharpen.fx

/**
   LumaSharpen version 1.5.0
   by Christian Cann Schuldt Jensen ~ CeeJay.dk

   It blurs the original pixel with the surrounding pixels and then subtracts this blur to sharpen the image.
   It does this in luma to avoid color artifacts and allows limiting the maximum sharpning to avoid or lessen halo artifacts.
   This is similar to using Unsharp Mask in Photoshop.
   Version 1.5.1
  - UI improvements for Reshade 3.x
 */

//!MAGPIE EFFECT
//!VERSION 2
//!OUTPUT_WIDTH INPUT_WIDTH
//!OUTPUT_HEIGHT INPUT_HEIGHT


//!PARAMETER
//!DEFAULT 0.65
//!MIN 1e-5

// Shapening strength
float sharpStrength;

//!PARAMETER
//!DEFAULT 0.035
//!MIN 0
//!MAX 1

// Limits maximum amount of sharpening a pixel receives
// This helps avoid "haloing" artifacts which would otherwise occur when you raised the strength too much.
float sharpClamp;

//!PARAMETER
//!DEFAULT 1
//!MIN 0
//!MAX 3

// 0 : Fast
// 1 : Normal
// 2 : Wider
// 3 : Pyramid shaped

int pattern;

//!PARAMETER
//!DEFAULT 1
//!MIN 0

// Offset bias adjusts the radius of the sampling pattern.I designed the pattern for an offset bias of 1.0, but feel free to experiment.
float offsetBias;

//!TEXTURE
Texture2D INPUT;

//!SAMPLER
//!FILTER LINEAR
SamplerState sam;


//!PASS 1
//!STYLE PS
//!IN INPUT

   /*-----------------------------------------------------------.
  /                      Developer settings                     /
  '-----------------------------------------------------------*/
#define CoefLuma float3(0.2126, 0.7152, 0.0722)      // BT.709 & sRBG luma coefficient (Monitors and HD Television)
  //#define CoefLuma float3(0.299, 0.587, 0.114)       // BT.601 luma coefficient (SD Television)
  //#define CoefLuma float3(1.0/3.0, 1.0/3.0, 1.0/3.0) // Equal weight coefficient

float4 Pass1(float2 pos) {
	float2 BUFFER_PIXEL_SIZE = GetInputPt();

	// -- Get the original pixel --
	float3 ori = INPUT.SampleLevel(sam, pos, 0).rgb; // ori = original pixel

	// -- Combining the strength and luma multipliers --
	float3 sharp_strength_luma = (CoefLuma * sharpStrength); //I'll be combining even more multipliers with it later on

	/*-----------------------------------------------------------.
	/                       Sampling patterns                     /
	'-----------------------------------------------------------*/
	float3 blur_ori = { 0,0,0 };

	//   [ NW,   , NE ] Each texture lookup (except ori)
	//   [   ,ori,    ] samples 4 pixels
	//   [ SW,   , SE ]

	// -- Pattern 1 -- A (fast) 7 tap gaussian using only 2+1 texture fetches.
	if (pattern == 0) {
		// -- Gaussian filter --
		//   [ 1/9, 2/9,    ]     [ 1 , 2 ,   ]
		//   [ 2/9, 8/9, 2/9]  =  [ 2 , 8 , 2 ]
		//   [    , 2/9, 1/9]     [   , 2 , 1 ]

		blur_ori = INPUT.SampleLevel(sam, pos + (BUFFER_PIXEL_SIZE / 3.0) * offsetBias, 0).rgb;  // North West
		blur_ori += INPUT.SampleLevel(sam, pos + (-BUFFER_PIXEL_SIZE / 3.0) * offsetBias, 0).rgb; // South East

		//blur_ori += tex2D(ReShade::BackBuffer, tex + (BUFFER_PIXEL_SIZE / 3.0) * offsetBias); // North East
		//blur_ori += tex2D(ReShade::BackBuffer, tex + (-BUFFER_PIXEL_SIZE / 3.0) * offsetBias); // South West

		blur_ori /= 2;  //Divide by the number of texture fetches

		sharp_strength_luma *= 1.5; // Adjust strength to aproximate the strength of pattern 2
	}

	// -- Pattern 2 -- A 9 tap gaussian using 4+1 texture fetches.
	if (pattern == 1) {
		// -- Gaussian filter --
		//   [ .25, .50, .25]     [ 1 , 2 , 1 ]
		//   [ .50,   1, .50]  =  [ 2 , 4 , 2 ]
		//   [ .25, .50, .25]     [ 1 , 2 , 1 ]

		blur_ori = INPUT.SampleLevel(sam, pos + float2(BUFFER_PIXEL_SIZE.x, -BUFFER_PIXEL_SIZE.y) * 0.5 * offsetBias, 0).rgb; // South East
		blur_ori += INPUT.SampleLevel(sam, pos - BUFFER_PIXEL_SIZE * 0.5 * offsetBias, 0).rgb;  // South West
		blur_ori += INPUT.SampleLevel(sam, pos + BUFFER_PIXEL_SIZE * 0.5 * offsetBias, 0).rgb; // North East
		blur_ori += INPUT.SampleLevel(sam, pos - float2(BUFFER_PIXEL_SIZE.x, -BUFFER_PIXEL_SIZE.y) * 0.5 * offsetBias, 0).rgb; // North West

		blur_ori *= 0.25;  // ( /= 4) Divide by the number of texture fetches
	}

	// -- Pattern 3 -- An experimental 17 tap gaussian using 4+1 texture fetches.
	if (pattern == 2) {
		// -- Gaussian filter --
		//   [   , 4 , 6 ,   ,   ]
		//   [   ,16 ,24 ,16 , 4 ]
		//   [ 6 ,24 ,   ,24 , 6 ]
		//   [ 4 ,16 ,24 ,16 ,   ]
		//   [   ,   , 6 , 4 ,   ]

		blur_ori = INPUT.SampleLevel(sam, pos + BUFFER_PIXEL_SIZE * float2(0.4, -1.2) * offsetBias, 0).rgb;  // South South East
		blur_ori += INPUT.SampleLevel(sam, pos - BUFFER_PIXEL_SIZE * float2(1.2, 0.4) * offsetBias, 0).rgb; // West South West
		blur_ori += INPUT.SampleLevel(sam, pos + BUFFER_PIXEL_SIZE * float2(1.2, 0.4) * offsetBias, 0).rgb; // East North East
		blur_ori += INPUT.SampleLevel(sam, pos - BUFFER_PIXEL_SIZE * float2(0.4, -1.2) * offsetBias, 0).rgb; // North North West

		blur_ori *= 0.25;  // ( /= 4) Divide by the number of texture fetches

		sharp_strength_luma *= 0.51;
	}

	// -- Pattern 4 -- A 9 tap high pass (pyramid filter) using 4+1 texture fetches.
	if (pattern == 3) {
		// -- Gaussian filter --
		//   [ .50, .50, .50]     [ 1 , 1 , 1 ]
		//   [ .50,    , .50]  =  [ 1 ,   , 1 ]
		//   [ .50, .50, .50]     [ 1 , 1 , 1 ]

		blur_ori = INPUT.SampleLevel(sam, pos + float2(0.5 * BUFFER_PIXEL_SIZE.x, -BUFFER_PIXEL_SIZE.y * offsetBias), 0).rgb;  // South South East
		blur_ori += INPUT.SampleLevel(sam, pos + float2(offsetBias * -BUFFER_PIXEL_SIZE.x, 0.5 * -BUFFER_PIXEL_SIZE.y), 0).rgb; // West South West
		blur_ori += INPUT.SampleLevel(sam, pos + float2(offsetBias * BUFFER_PIXEL_SIZE.x, 0.5 * BUFFER_PIXEL_SIZE.y), 0).rgb; // East North East
		blur_ori += INPUT.SampleLevel(sam, pos + float2(0.5 * -BUFFER_PIXEL_SIZE.x, BUFFER_PIXEL_SIZE.y * offsetBias), 0).rgb; // North North West

		//blur_ori += (2 * ori); // Probably not needed. Only serves to lessen the effect.

		blur_ori /= 4.0;  //Divide by the number of texture fetches

		sharp_strength_luma *= 0.666; // Adjust strength to aproximate the strength of pattern 2
	}

	/*-----------------------------------------------------------.
   /                            Sharpen                          /
   '-----------------------------------------------------------*/

   // -- Calculate the sharpening --
	float3 sharp = ori - blur_ori;  //Subtracting the blurred image from the original image

#if 0 //older 1.4 code (included here because the new code while faster can be difficult to understand)
	// -- Adjust strength of the sharpening --
	float sharp_luma = dot(sharp, sharp_strength_luma); //Calculate the luma and adjust the strength

	// -- Clamping the maximum amount of sharpening to prevent halo artifacts --
	sharp_luma = clamp(sharp_luma, -sharpClamp, sharpClamp);  //TODO Try a curve function instead of a clamp

#else //new code
	// -- Adjust strength of the sharpening and clamp it--
	float4 sharp_strength_luma_clamp = float4(sharp_strength_luma * (0.5 / sharpClamp), 0.5); //Roll part of the clamp into the dot

	//sharp_luma = saturate((0.5 / sharpClamp) * sharp_luma + 0.5); //scale up and clamp
	float sharp_luma = saturate(dot(float4(sharp, 1.0), sharp_strength_luma_clamp)); //Calculate the luma, adjust the strength, scale up and clamp
	sharp_luma = (sharpClamp * 2.0) * sharp_luma - sharpClamp; //scale down
#endif

	// -- Combining the values to get the final sharpened pixel	--
	float3 outputcolor = ori + sharp_luma;    // Add the sharpening to the the original.

	 /*-----------------------------------------------------------.
	/                     Returning the output                    /
	'-----------------------------------------------------------*/

	//if (show_sharpen) {
	//	//outputcolor = abs(sharp * 4.0);
	//	outputcolor = saturate(0.5 + (sharp_luma * 4.0)).rrr;
	//}

	return float4(saturate(outputcolor), 1);
}
