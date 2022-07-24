// 移植自 https://github.com/libretro/slang-shaders/blob/master/misc/image-adjustment.slang

//!MAGPIE EFFECT
//!VERSION 2


//!PARAMETER
//!DEFAULT 2.2
//!MIN 1e-5
float targetGamma;

//!PARAMETER
//!DEFAULT 2.2
//!MIN 1e-5
float monitorGamma;

//!PARAMETER
//!DEFAULT 1
//!MIN 0
float saturation;

//!PARAMETER
//!DEFAULT 1
//!MIN 0
float luminance;

//!PARAMETER
//!DEFAULT 1
//!MIN 0
float contrast;

//!PARAMETER
//!DEFAULT 0
//!MIN -1
//!MAX 1
float brightBoost;

//!TEXTURE
Texture2D INPUT;

//!SAMPLER
//!FILTER POINT
SamplerState sam;


//!PASS 1
//!STYLE PS
//!IN INPUT

float3 RGBtoHSV(float3 c) {
    float4 K = float4(0.0, -1.0 / 3.0, 2.0 / 3.0, -1.0);
    float4 p = c.g < c.b ? float4(c.bg, K.wz) : float4(c.gb, K.xy);
    float4 q = c.r < p.x ? float4(p.xyw, c.r) : float4(c.r, p.yzx);

    float d = q.x - min(q.w, q.y);
    float e = 1.0e-10;
    return float3(abs(q.z + (q.w - q.y) / (6.0 * d + e)), d / (q.x + e), q.x);
}

float3 HSVtoRGB(float3 c) {
    float4 K = float4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    float3 p = abs(frac(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * lerp(K.xxx, saturate(p - K.xxx), c.y);
}

float4 Pass1(float2 pos) {
	float3 color = INPUT.SampleLevel(sam, pos, 0).rgb;

    // saturation and luminance
	color = saturate(HSVtoRGB(RGBtoHSV(color) * float3(1.0, saturation, luminance)));

    // contrast and brightness
    color = saturate((color - 0.5) * contrast + 0.5 + brightBoost);

	// Apply gamma correction
	color = pow(color, targetGamma / monitorGamma);

	return float4(color, 1);
}
