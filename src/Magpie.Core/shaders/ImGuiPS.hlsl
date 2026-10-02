#include "Common.hlsli"

#ifdef MP_HDR
cbuffer RootConstants : register(b0) {
    float sdrWhiteLevel;
};
#endif

SamplerState sam : register(s0);
Texture2D tex : register(t0);

float4 main(noperspective float2 uv : TEXCOORD, noperspective float4 color : COLOR) : SV_Target {
	// ImGui 输出 sRGB 颜色，需转换到线性空间
    float4 srgb = saturate(color * tex.Sample(sam, uv));
    float3 c = DecodeSrgb(srgb.rgb);
	
#ifdef MP_HDR
	c *= sdrWhiteLevel;
#endif
	
    return float4(c, srgb.a);
}
