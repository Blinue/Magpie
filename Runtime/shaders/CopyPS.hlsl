Texture2D tex : register(t0);
SamplerState sam : register(s0);

struct VS_OUTPUT {
	float4 Position : SV_POSITION;
	float4 TexCoord : TEXCOORD0;
};

float4 main(VS_OUTPUT input) : SV_Target{
	return tex.Sample(sam, input.TexCoord.xy);
}
