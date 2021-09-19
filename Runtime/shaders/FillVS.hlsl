struct VS_OUTPUT {
	float4 Position : SV_POSITION;
	float4 TexCoord : TEXCOORD0;
};

VS_OUTPUT main(uint id : SV_VERTEXID) {
	VS_OUTPUT output;

	float2 texCoord = float2(id & 1, id >> 1) * 2.0;
	output.TexCoord = float4(texCoord, 0, 0);
	output.Position = float4(texCoord * float2(2.0, -2.0) + float2(-1.0, 1.0), 0.0, 1.0);

	return output;
}
