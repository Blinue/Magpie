Texture2D originTex : register(t0);
Texture2D maskTex : register(t1);
SamplerState sam : register(s0);


float4 main(float4 pos : SV_POSITION, float2 coord : TEXCOORD) : SV_Target{
	float2 masks = maskTex.Sample(sam, coord).xy;
	if (masks.x > 0.5) {
		float3 origin = originTex.Sample(sam, coord).rgb;

		if (masks.y > 0.5) {
			return float4(1 - origin, 1);
		} else {
			return float4(origin, 1);
		}
	} else {
		if (masks.y > 0.5) {
			return float4(1, 1, 1, 1);
		} else {
			return float4(0, 0, 0, 1);
		}
	}
}