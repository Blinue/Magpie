#include "Common.hlsli"

cbuffer RootConstants : register(b1) {
	uint2 cursorTexSize;
	uint2 cursorSize;
	uint2 originOffset;
#ifdef MP_SRGB
	uint shouldEncodeSrgb;
#else
	float sdrWhiteLevel;
#endif
};

Texture2D<float4> cursorTex : register(t0);
Texture2D<float4> originTex : register(t1);

float4 main(noperspective float2 uv : TEXCOORD) : SV_TARGET {
	// A 为 0 时用 RGB 通道取代屏幕颜色，为 1 时将 RGB 通道和屏幕颜色进行异或操作
	float4 mask = cursorTex[uint2(uv * cursorTexSize)];
	
	if (mask.a < 0.5f) {
		return float4(mask.rgb, 1);
	}
	
	float3 origin = originTex[uint2(uv * cursorSize) + originOffset].rgb;
	
#ifdef MP_SRGB
	// originTex 有两个来源，可能是完整帧，也可能是复制自渲染目标的临时纹理。如果是
	// 前者，这里需要执行伽马校正。为了和 OS 一致，应在伽马校正后应用掩码。
	[branch]
	if (shouldEncodeSrgb) {
		origin = EncodeSrgb(saturate(origin));
	}
#else
	// WCG/HDR 下将颜色压缩到 0-1，应用掩码然后还原亮度。不追求和 OS 行为一致，截
	// 至 Win11 25H2，HDR 下的单色光标和彩色掩码光标仍没有稳定的表现。
	float white = max(max(origin.r, origin.g), max(origin.b, sdrWhiteLevel));
	origin = saturate(origin / white);
#endif
	
	// 255.001953 来自
	// https://stackoverflow.com/questions/52103720/why-does-d3dcolortoubyte4-multiplies-components-by-255-001953f
	origin = (uint3(origin * 255.001953) ^ uint3(mask.rgb * 255.001953)) / 255.0;
	
#ifndef MP_SRGB
	origin *= white;
#endif
	
	return float4(origin, 1);
}
