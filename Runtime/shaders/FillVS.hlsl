struct VS_OUTPUT {
	float4 Position : SV_POSITION;
	float2 TexCoord : TEXCOORD0;
};

VS_OUTPUT main(uint id : SV_VERTEXID) {
	VS_OUTPUT output;

	output.TexCoord = float2(id & 1, id >> 1) * 2.0;
	output.Position = float4(output.TexCoord * float2(2.0, -2.0) + float2(-1.0, 1.0), 0.0, 1.0);

	return output;
}
