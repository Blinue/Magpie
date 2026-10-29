Texture2D inputTex : register(t0);
SamplerState sam : register(s0);

float4 main(noperspective float2 coord : TEXCOORD) : SV_Target {
	return inputTex.Sample(sam, coord);
}
