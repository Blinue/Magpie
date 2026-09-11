cbuffer RootConstants : register(b0) {
	uint2 inputSize;
	float2 inputPt;
};

Texture2D inputTex : register(t0);
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

float4 main(noperspective float2 uv : TEXCOORD) : SV_Target {
	float2 pos = uv * inputSize;
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

	int2 coord_top_left = int2(max(uv0 * inputSize, 0.5f));
	int2 coord_bottom_right = int2(min(uv3 * inputSize, inputSize - 0.5f));

	float4 top = inputTex.Load(int3(coord_top_left, 0)) * rowtaps.x;
	top += inputTex.SampleLevel(linearSampler, float2(u_middle, uv0.y), 0) * u_weight_sum;
	top += inputTex.Load(int3(coord_bottom_right.x, coord_top_left.y, 0)) * rowtaps.w;
	float4 total = top * coltaps.x;

	float4 middle = inputTex.SampleLevel(linearSampler, float2(uv0.x, v_middle), 0) * rowtaps.x;
	middle += inputTex.SampleLevel(linearSampler, float2(u_middle, v_middle), 0) * u_weight_sum;
	middle += inputTex.SampleLevel(linearSampler, float2(uv3.x, v_middle), 0) * rowtaps.w;
	total += middle * v_weight_sum;

	float4 bottom = inputTex.Load(int3(coord_top_left.x, coord_bottom_right.y, 0)) * rowtaps.x;
	bottom += inputTex.SampleLevel(linearSampler, float2(u_middle, uv3.y), 0) * u_weight_sum;
	bottom += inputTex.Load(int3(coord_bottom_right, 0)) * rowtaps.w;
	total += bottom * coltaps.w;
	
	return total;
}
