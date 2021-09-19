Texture2D inputTex : register(t0);
Texture2D cursorTex : register(t1);

SamplerState linearSam : register(s0);
SamplerState pointSam : register(s1);

cbuffer constants : register(b0) {
	float4 cursorRect;
};

struct VS_OUTPUT {
	float4 Position : SV_POSITION;
	float4 TexCoord : TEXCOORD0;
};

float4 main(VS_OUTPUT input) : SV_TARGET{
	float2 coord = input.TexCoord.xy;

	float3 cur = inputTex.Sample(linearSam, coord).rgb;
	if (coord.x < cursorRect.x || coord.x > cursorRect.z || coord.y < cursorRect.y || coord.y > cursorRect.w) {
		return float4(cur, 1);
	} else {
		float2 cursorCoord = (coord - cursorRect.xy) / (cursorRect.zw - cursorRect.xy);
		cursorCoord.y /= 2;

		uint3 andMask = cursorTex.Sample(pointSam, cursorCoord).rgb * 255;
		float4 xorMask = cursorTex.Sample(pointSam, float2(cursorCoord.x, cursorCoord.y + 0.5f));

		if (andMask.x && xorMask.a == 0) {
			return float4(cur, 1);
		} else {
			return float4(lerp(xorMask.xyz, cur, 1 - xorMask.a) ,1);
		}
	}
}
