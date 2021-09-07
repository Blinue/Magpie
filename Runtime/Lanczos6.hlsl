Texture2D frameTexture : register(t0);
SamplerState linearSampler : register(s0);


struct VS_OUTPUT {
	float4 Position : SV_POSITION; // vertex position 
	float4 TexCoord : TEXCOORD0;   // vertex texture coords 
};

//--------------------------------------------------------------------------------------
// Vertex Shader
//--------------------------------------------------------------------------------------
VS_OUTPUT VS(float4 pos : POSITION, float4 texCoord : TEXCOORD) {
	VS_OUTPUT output = { pos, texCoord };
	return output;
}


//--------------------------------------------------------------------------------------
// Pixel Shader
//--------------------------------------------------------------------------------------

#define FIX(c) max(abs(c), 1e-5)
#define PI 3.14159265359
#define min4(a, b, c, d) min(min(a, b), min(c, d))
#define max4(a, b, c, d) max(max(a, b), max(c, d))

float3 weight3(float x) {
	const float radius = 3.0;
	float3 s = FIX(2.0 * PI * float3(x - 1.5, x - 0.5, x + 0.5));

	// Lanczos3. Note: we normalize outside this function, so no point in multiplying by radius.
	return /*radius **/ sin(s) * sin(s / radius) / (s * s);
}


float3 line_run(float ypos, float3 xpos1, float3 xpos2, float3 linetaps1, float3 linetaps2) {
	return frameTexture.Sample(linearSampler, float2(xpos1.r, ypos)).rgb * linetaps1.r
		+ frameTexture.Sample(linearSampler, float2(xpos1.g, ypos)).rgb * linetaps2.r
		+ frameTexture.Sample(linearSampler, float2(xpos1.b, ypos)).rgb * linetaps1.g
		+ frameTexture.Sample(linearSampler, float2(xpos2.r, ypos)).rgb * linetaps2.g
		+ frameTexture.Sample(linearSampler, float2(xpos2.g, ypos)).rgb * linetaps1.b
		+ frameTexture.Sample(linearSampler, float2(xpos2.b, ypos)).rgb * linetaps2.b;
}

float4 PS(VS_OUTPUT input) : SV_Target{
	float4 coord = input.TexCoord;

	// 用于抗振铃
	float3 neighbors[4] = {
		frameTexture.Sample(linearSampler, float2(coord.x - coord.z, coord.y)).rgb,
		frameTexture.Sample(linearSampler, float2(coord.x + coord.z, coord.y)).rgb,
		frameTexture.Sample(linearSampler, float2(coord.x, coord.y - coord.w)).rgb,
		frameTexture.Sample(linearSampler, float2(coord.x, coord.y + coord.w)).rgb
	};

	float2 f = frac(coord.xy / coord.zw + 0.5);

	float3 linetaps1 = weight3(0.5 - f.x * 0.5);
	float3 linetaps2 = weight3(1.0 - f.x * 0.5);
	float3 columntaps1 = weight3(0.5 - f.y * 0.5);
	float3 columntaps2 = weight3(1.0 - f.y * 0.5);

	// make sure all taps added together is exactly 1.0, otherwise some
	// (very small) distortion can occur
	float suml = dot(linetaps1, float3(1, 1, 1)) + dot(linetaps2, float3(1, 1, 1));
	float sumc = dot(columntaps1, float3(1, 1, 1)) + dot(columntaps2, float3(1, 1, 1));
	linetaps1 /= suml;
	linetaps2 /= suml;
	columntaps1 /= sumc;
	columntaps2 /= sumc;

	// !!!改变当前坐标
	coord.xy -= (f + 2) * coord.zw;

	float3 xpos1 = float3(coord.x, coord.x + coord.z, coord.x + 2 * coord.z);
	float3 xpos2 = float3(coord.x + 3 * coord.z, coord.x + 4 * coord.z, coord.x + 5 * coord.z);

	// final sum and weight normalization
	float3 color = line_run(coord.y, xpos1, xpos2, linetaps1, linetaps2) * columntaps1.r
		+ line_run(coord.y + coord.w, xpos1, xpos2, linetaps1, linetaps2) * columntaps2.r
		+ line_run(coord.y + 2 * coord.w, xpos1, xpos2, linetaps1, linetaps2) * columntaps1.g
		+ line_run(coord.y + 3 * coord.w, xpos1, xpos2, linetaps1, linetaps2) * columntaps2.g
		+ line_run(coord.y + 4 * coord.w, xpos1, xpos2, linetaps1, linetaps2) * columntaps1.b
		+ line_run(coord.y + 5 * coord.w, xpos1, xpos2, linetaps1, linetaps2) * columntaps2.b;

	// 抗振铃
	float3 min_sample = min4(neighbors[0], neighbors[1], neighbors[2], neighbors[3]);
	float3 max_sample = max4(neighbors[0], neighbors[1], neighbors[2], neighbors[3]);
	color = lerp(color, clamp(color, min_sample, max_sample), 0.5);

	return float4(color, 1);
}
