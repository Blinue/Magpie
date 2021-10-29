// 移植自 https://github.com/libretro/common-shaders/blob/master/interpolation/shaders/pixellate.cg

//!MAGPIE EFFECT
//!VERSION 1


//!CONSTANT
//!VALUE INPUT_PT_X
float inputPtX;

//!CONSTANT
//!VALUE INPUT_PT_Y
float inputPtY;

//!CONSTANT
//!VALUE OUTPUT_PT_X
float outputPtX;

//!CONSTANT
//!VALUE OUTPUT_PT_Y
float outputPtY;

//!TEXTURE
Texture2D INPUT;

//!SAMPLER
//!FILTER POINT
SamplerState sam;


//!PASS 1
//!BIND INPUT

float4 Pass1(float2 pos) {
	float2 texelSize = { inputPtX, inputPtY };

	float2 range = float2(outputPtX, outputPtY) / 2.0 * 0.999;

	float left = pos.x - range.x;
	float top = pos.y + range.y;
	float right = pos.x + range.x;
	float bottom = pos.y - range.y;

	float3 topLeftColor = INPUT.Sample(sam, (floor(float2(left, top) / texelSize) + 0.5) * texelSize).rgb;
	float3 bottomRightColor = INPUT.Sample(sam, (floor(float2(right, bottom) / texelSize) + 0.5) * texelSize).rgb;
	float3 bottomLeftColor = INPUT.Sample(sam, (floor(float2(left, bottom) / texelSize) + 0.5) * texelSize).rgb;
	float3 topRightColor = INPUT.Sample(sam, (floor(float2(right, top) / texelSize) + 0.5) * texelSize).rgb;

	float2 border = clamp(round(pos / texelSize) * texelSize, float2(left, bottom), float2(right, top));

	float totalArea = 4.0 * range.x * range.y;

	float3 averageColor;
	averageColor = ((border.x - left) * (top - border.y) / totalArea) * topLeftColor;
	averageColor += ((right - border.x) * (border.y - bottom) / totalArea) * bottomRightColor;
	averageColor += ((border.x - left) * (border.y - bottom) / totalArea) * bottomLeftColor;
	averageColor += ((right - border.x) * (top - border.y) / totalArea) * topRightColor;

	return float4(averageColor, 1.0);
}
