// 移植自 https://github.com/libretro/slang-shaders/blob/3f67e1870dbd5be74ae2f09eaed0eeadce6abd15/misc/image-adjustment.slang

//!MAGPIE EFFECT
//!VERSION 3
//!OUTPUT_WIDTH INPUT_WIDTH
//!OUTPUT_HEIGHT INPUT_HEIGHT


//!PARAMETER
//!LABEL Target Gamma
//!DEFAULT 2.2
//!MIN 0.01
//!MAX 5
//!STEP 0.01
float targetGamma;

//!PARAMETER
//!LABEL Monitor Gamma
//!DEFAULT 2.2
//!MIN 0.01
//!MAX 5
//!STEP 0.01
float monitorGamma;

//!PARAMETER
//!LABEL Saturation
//!DEFAULT 1
//!MIN 0
//!MAX 5
//!STEP 0.01
float saturation;

//!PARAMETER
//!LABEL Luminance
//!DEFAULT 1
//!MIN 0
//!MAX 2
//!STEP 0.01
float luminance;

//!PARAMETER
//!LABEL Contrast
//!DEFAULT 1
//!MIN 0
//!MAX 10
//!STEP 0.1
float contrast;

//!PARAMETER
//!LABEL Brightness Boost
//!DEFAULT 0
//!MIN -1
//!MAX 1
//!STEP 0.01
float brightBoost;

//!PARAMETER
//!LABEL Black Level
//!DEFAULT 0
//!MIN -1
//!MAX 1
//!STEP 0.01
float blackLevel;

//!PARAMETER
//!LABEL Red Channel
//!DEFAULT 1
//!MIN 0
//!MAX 2
//!STEP 0.01
float r;

//!PARAMETER
//!LABEL Green Channel
//!DEFAULT 1
//!MIN 0
//!MAX 2
//!STEP 0.01
float g;

//!PARAMETER
//!LABEL Blue Channel
//!DEFAULT 1
//!MIN 0
//!MAX 2
//!STEP 0.01
float b;

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
    float4 p = c.y < c.z ? float4(c.bg, K.wz) : float4(c.gb, K.xy);
    float4 q = c.x < p.x ? float4(p.xyw, c.x) : float4(c.x, p.yzx);

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

    // black level
    color -= blackLevel;
    color = saturate(color / (1 - blackLevel));

	// gamma correction
	color = pow(color, targetGamma / monitorGamma);

    color *= float3(r, g, b);

	return float4(color, 1);
}
