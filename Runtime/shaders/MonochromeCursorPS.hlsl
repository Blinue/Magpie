Texture2D tex : register(t0);
SamplerState sam : register(s0);

struct VS_OUTPUT {
	float4 Position : SV_POSITION;
	float2 TexCoord : TEXCOORD0;
};

float4 main(VS_OUTPUT input) : SV_Target{
	float2 coord = input.TexCoord * float2(1, 0.5);

	float2 masks = tex.Sample(sam, coord).xy;
	if (masks.x > 0.5) {
		float3 origin = tex.Sample(sam, coord + float2(0, 0.5)).rgb;

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