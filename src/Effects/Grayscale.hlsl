// 灰度
// 使用 BT.709 系数计算亮度

//!MAGPIE EFFECT
//!VERSION 4

//!TEXTURE
Texture2D INPUT;

//!TEXTURE
//!WIDTH INPUT_WIDTH
//!HEIGHT INPUT_HEIGHT
Texture2D OUTPUT;

//!SAMPLER
//!FILTER POINT
SamplerState sam;


//!PASS 1
//!STYLE PS
//!IN INPUT
//!OUT OUTPUT

float4 Pass1(float2 pos) {
	float4 color = INPUT.SampleLevel(sam, pos, 0);
	float luma = dot(color.rgb, float3(0.2126, 0.7152, 0.0722));
	return float4(luma, luma, luma, color.a);
}
