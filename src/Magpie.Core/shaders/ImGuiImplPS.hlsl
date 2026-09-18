#include "Common.hlsli"

SamplerState sam : register(s0);
Texture2D tex : register(t0);

float4 main(noperspective float2 uv : TEXCOORD, noperspective float4 color : COLOR) : SV_Target {
	// ImGui 输出 sRGB 颜色，需转换到线性空间
	float4 srgb = color * tex.Sample(sam, uv);
	return float4(DecodeSrgb(srgb.rgb), srgb.a);
}
