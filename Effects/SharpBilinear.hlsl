// 移植自 https://github.com/libretro/common-shaders/blob/master/interpolation/shaders/sharp-bilinear.cg

//!MAGPIE EFFECT
//!VERSION 1


//!CONSTANT
//!VALUE INPUT_WIDTH
float inputWidth;

//!CONSTANT
//!VALUE INPUT_HEIGHT
float inputHeight;

//!CONSTANT
//!VALUE SCALE_X
float scaleX;

//!CONSTANT
//!VALUE SCALE_Y
float scaleY;

//!TEXTURE
Texture2D INPUT;

//!SAMPLER
//!FILTER LINEAR

// sharp-bilinear 使用双线性插值
SamplerState sam;


//!PASS 1
//!BIND INPUT

float4 Pass1(float2 pos) {
	float2 texel = pos * float2(inputWidth, inputHeight);
//   float2 texel_floored = floor(texel);
//   float scale = (AUTO_PRESCALE > 0.5) ? floor(output_size.y / video_size.y) : SHARP_BILINEAR_PRE_SCALE;
	float2 scale = { scaleX, scaleY };
	float2 texel_floored = floor(texel);
	float2 s = frac(texel);
	float2 region_range = 0.5 - 0.5 / scale;

	// Figure out where in the texel to sample to get correct pre-scaled bilinear.
	// Uses the hardware bilinear interpolator to avoid having to sample 4 times manually.

	float2 center_dist = s - 0.5;
	float2 f = (center_dist - clamp(center_dist, -region_range, region_range)) * scale + 0.5;

	float2 mod_texel = texel_floored + f;
	return INPUT.Sample(sam, mod_texel / float2(inputWidth, inputHeight));
}
