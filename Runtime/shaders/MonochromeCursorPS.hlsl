Texture2D originTex : register(t0);
Texture2D maskTex : register(t1);
SamplerState sam : register(s0);

struct VS_OUTPUT {
	float4 Position : SV_POSITION;
	float2 TexCoord : TEXCOORD0;
};

float4 main(VS_OUTPUT input) : SV_Target{
	float2 masks = maskTex.Sample(sam, input.TexCoord).xy;
	if (masks.x > 0.5) {
		float3 origin = originTex.Sample(sam, input.TexCoord).rgb;

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