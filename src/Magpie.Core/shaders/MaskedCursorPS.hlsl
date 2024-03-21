Texture2D originTex : register(t0);
Texture2D cursorTex : register(t1);

// 双线性插值会转换为彩色光标，这里只需考虑最近邻插值
SamplerState pointSampler : register(s0);

float4 main(float2 coord : TEXCOORD) : SV_TARGET {
	float4 mask = cursorTex.Sample(pointSampler, coord);
	
	if (mask.a < 0.5f) {
		return float4(mask.rgb, 1);
	} else {
		float3 origin = originTex.Sample(pointSampler, coord).rgb;
		// 255.001953 的由来见 https://stackoverflow.com/questions/52103720/why-does-d3dcolortoubyte4-multiplies-components-by-255-001953f
		return float4((uint3(origin * 255.001953f) ^ uint3(mask.rgb * 255.001953f)) / 255.0f, 1);
	}
}
