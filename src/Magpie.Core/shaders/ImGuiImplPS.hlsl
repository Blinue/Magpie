sampler sampler0;
Texture2D texture0;

float4 main(float2 coord : TEXCOORD, float4 color : COLOR) : SV_Target {
	return color * float4(1, 1, 1, texture0.Sample(sampler0, coord).r);
}
