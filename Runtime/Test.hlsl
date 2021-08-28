
struct VS_OUTPUT {
	float4 Position   : SV_POSITION; // vertex position 
	float4 TexCoord  : TEXCOORD0;   // vertex texture coords 
};
//--------------------------------------------------------------------------------------
// Vertex Shader
//--------------------------------------------------------------------------------------
VS_OUTPUT VS(float4 pos : POSITION, float4 texCoord : TEXCOORD)
{
	VS_OUTPUT output;
	output.Position = pos;
	output.TexCoord = texCoord;
    return output;
}


//--------------------------------------------------------------------------------------
// Pixel Shader
//--------------------------------------------------------------------------------------
float4 PS(VS_OUTPUT input) : SV_Target
{
    return float4(input.TexCoord.xy, 0.0f, 1.0f );    // Yellow, with Alpha = 1
}
