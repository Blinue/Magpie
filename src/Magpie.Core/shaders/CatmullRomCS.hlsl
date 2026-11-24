// 基于 https://github.com/obsproject/obs-studio/blob/master/libobs/data/bicubic_scale.effect

#include "Common.hlsli"

cbuffer RootConstants : register(b0) {
	uint2 inputSize;
	float2 inputPt;
	float2 outputPt;
};

Texture2D<float4> inputTex : register(t0);
RWTexture2D<float4> outputTex : register(u0);

SamplerState linearSampler : register(s0);

float4 weight4(float x) {
	// Sharper version.  May look better in some cases. B=0, C=0.75
	return float4(
		((-0.75 * x + 1.5) * x - 0.75) * x,
		(1.25 * x - 2.25) * x * x + 1.0,
		((-1.25 * x + 1.5) * x + 0.75) * x,
		(0.75 * x - 0.75) * x * x
	);
}

float4 CatmullRom(float2 pos) {
	pos *= inputSize;
	float2 pos1 = floor(pos - 0.5) + 0.5;
	float2 f = pos - pos1;

	float4 rowtaps = weight4(f.x);
	float4 coltaps = weight4(f.y);
	
	float2 uv1 = pos1 * inputPt;
	float2 uv0 = uv1 - inputPt;
	float2 uv2 = uv1 + inputPt;
	float2 uv3 = uv2 + inputPt;

	float u_weight_sum = rowtaps.y + rowtaps.z;
	float u_middle_offset = rowtaps.z * inputPt.x / u_weight_sum;
	float u_middle = uv1.x + u_middle_offset;

	float v_weight_sum = coltaps.y + coltaps.z;
	float v_middle_offset = coltaps.z * inputPt.y / v_weight_sum;
	float v_middle = uv1.y + v_middle_offset;

	int2 coord_top_left = int2(max(uv0 * inputSize, 0.5));
	int2 coord_bottom_right = int2(min(uv3 * inputSize, inputSize - 0.5));

	float3 top = inputTex.Load(int3(coord_top_left, 0)).rgb * rowtaps.x;
	top += inputTex.SampleLevel(linearSampler, float2(u_middle, uv0.y), 0).rgb * u_weight_sum;
	top += inputTex.Load(int3(coord_bottom_right.x, coord_top_left.y, 0)).rgb * rowtaps.w;
	float3 total = top * coltaps.x;

	float3 middle = inputTex.SampleLevel(linearSampler, float2(uv0.x, v_middle), 0).rgb * rowtaps.x;
	middle += inputTex.SampleLevel(linearSampler, float2(u_middle, v_middle), 0).rgb * u_weight_sum;
	middle += inputTex.SampleLevel(linearSampler, float2(uv3.x, v_middle), 0).rgb * rowtaps.w;
	total += middle * v_weight_sum;

	float3 bottom = inputTex.Load(int3(coord_top_left.x, coord_bottom_right.y, 0)).rgb * rowtaps.x;
	bottom += inputTex.SampleLevel(linearSampler, float2(u_middle, uv3.y), 0).rgb * u_weight_sum;
	bottom += inputTex.Load(int3(coord_bottom_right, 0)).rgb * rowtaps.w;
	total += bottom * coltaps.w;

#ifdef MP_SRGB
	total = EncodeSrgb(saturate(total));
#endif
	return float4(total, 1);
}

[numthreads(64, 1, 1)]
void main(uint3 tid : SV_GroupThreadID, uint3 gid : SV_GroupID) {
	uint2 gxy = (gid.xy << 4) + Rmp8x8(tid.x);
	float2 pos = (gxy + 0.5) * outputPt;
	const float2 step = 8 * outputPt;

	outputTex[gxy] = CatmullRom(pos);

	gxy.x += 8u;
	pos.x += step.x;
	outputTex[gxy] = CatmullRom(pos);
	
	gxy.y += 8u;
	pos.y += step.y;
	outputTex[gxy] = CatmullRom(pos);
	
	gxy.x -= 8u;
	pos.x -= step.x;
	outputTex[gxy] = CatmullRom(pos);
}
