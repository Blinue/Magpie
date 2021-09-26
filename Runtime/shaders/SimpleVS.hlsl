struct VS_OUTPUT {
	float4 Position : SV_POSITION;
	float2 TexCoord : TEXCOORD0;
};

VS_OUTPUT main(float4 pos : SV_POSITION, float2 texCoord : TEXCOORD) {
	VS_OUTPUT output = { pos, texCoord };
	return output;
}
