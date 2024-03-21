Texture2D originTex : register(t0);
Texture2D<float2> cursorTex : register(t1);

// 双线性插值会转换为彩色光标，这里只需考虑最近邻插值
SamplerState pointSampler : register(s0);

float4 main(float2 coord : TEXCOORD) : SV_TARGET {
	float2 mask = cursorTex.Sample(pointSampler, coord);
	
	if (mask.x > 0.5f) {
		float3 origin = originTex.Sample(pointSampler, coord).rgb;

		if (mask.y > 0.5f) {
			return float4(1 - origin, 1);
		} else {
			return float4(origin, 1);
		}
	} else {
		if (mask.y > 0.5f) {
			return float4(1, 1, 1, 1);
		} else {
			return float4(0, 0, 0, 1);
		}
	}
}
