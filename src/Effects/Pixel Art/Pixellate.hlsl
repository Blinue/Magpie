// 移植自 https://github.com/libretro/common-shaders/blob/master/interpolation/shaders/pixellate.cg

//!MAGPIE EFFECT
//!VERSION 2


//!TEXTURE
Texture2D INPUT;

//!SAMPLER
//!FILTER POINT
SamplerState sam;


//!PASS 1
//!STYLE PS
//!IN INPUT

float4 Pass1(float2 pos) {
	float2 texelSize = GetInputPt();

	float2 range = GetOutputPt() / 2.0f * 0.999f;

	float left = pos.x - range.x;
	float top = pos.y + range.y;
	float right = pos.x + range.x;
	float bottom = pos.y - range.y;

	float3 topLeftColor = INPUT.SampleLevel(sam, (floor(float2(left, top) / texelSize) + 0.5) * texelSize, 0).rgb;
	float3 bottomRightColor = INPUT.SampleLevel(sam, (floor(float2(right, bottom) / texelSize) + 0.5) * texelSize, 0).rgb;
	float3 bottomLeftColor = INPUT.SampleLevel(sam, (floor(float2(left, bottom) / texelSize) + 0.5) * texelSize, 0).rgb;
	float3 topRightColor = INPUT.SampleLevel(sam, (floor(float2(right, top) / texelSize) + 0.5) * texelSize, 0).rgb;

	float2 border = clamp(round(pos / texelSize) * texelSize, float2(left, bottom), float2(right, top));

	float totalArea = 4.0 * range.x * range.y;

	float3 averageColor;
	averageColor = ((border.x - left) * (top - border.y) / totalArea) * topLeftColor;
	averageColor += ((right - border.x) * (border.y - bottom) / totalArea) * bottomRightColor;
	averageColor += ((border.x - left) * (border.y - bottom) / totalArea) * bottomLeftColor;
	averageColor += ((right - border.x) * (top - border.y) / totalArea) * topRightColor;

	return float4(averageColor, 1.0);
}
