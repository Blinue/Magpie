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

Texture2D<uint> cursorTex : register(t0);
Texture2D<float4> originTex : register(t1);

float4 main(noperspective float2 uv : TEXCOORD) : SV_TARGET {
	// 高四位是 AND 掩码， 低四位是 XOR 掩码
	uint mask = cursorTex[uint2(uv * cursorTexSize)];
	bool andMask = (mask & 0xF0u) != 0u;
	bool xorMask = (mask & 0x0Fu) != 0u;
	
	if (!andMask) {
		// 黑色或白色
#ifdef MP_SRGB
		float c = xorMask ? 1.0f : 0.0f;
#else
		float c = xorMask ? sdrWhiteLevel : 0.0;
#endif
		return float4(c, c, c, 1.0);
	}
	
	float3 origin = originTex[uint2(uv * cursorSize) + originOffset].rgb;
	
#ifdef MP_SRGB
	// originTex 有两个来源，可能是完整帧，也可能是复制自渲染目标的临时纹理。如果是
	// 前者，这里需要执行伽马校正。为了和 OS 一致，应在伽马校正后应用掩码。
	[branch]
	if (shouldEncodeSrgb) {
		origin = EncodeSrgb(saturate(origin));
	}
	
	if (xorMask) {
		// 反色
		origin = 1.0 - origin;
	}
#else
	if (xorMask) {
		// WCG/HDR 下不追求和 OS 行为一致，截至 Win11 25H2，HDR 下的单色光标和彩
		// 色掩码光标仍没有稳定的表现。
		float white = max(max(origin.r, origin.g), max(origin.b, sdrWhiteLevel));
		origin = white - origin;
	}
#endif
	
	return float4(origin, 1.0);
}
