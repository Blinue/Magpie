#include "Common.hlsli"

Texture2D<float4> inputTex : register(t0);
RWTexture2D<float4> outputTex : register(u0);

// 维基百科和 ITU-R BT.2020-2 建议书中 BT.2020 的光电转换函数更加复杂，但伽马 2.4 已经
// 成为事实上的标准，比如 Chrome、FirFox、CSS 标准等全部使用伽马 2.4。经过测试，在浏览器
// 和图片查看器中伽马 2.4 的画面和 sRGB 相同，而标准的光电转换函数会使画面变暗。
// 见 https://github.com/w3c/csswg-drafts/issues/12574
float3 BT2020FromLinear(float3 color) {
	return pow(color, 1.0f / 2.4f);
}

// 来自 https://github.com/GPUOpen-LibrariesAndSDKs/FidelityFX-SDK/blob/8f7498a4b42d06d1daf64b3961ddcaedc07ccaf0/Kits/FidelityFX/api/internal/gpu/ffx_core_gpu_common.h#L2536-L2548
float3 PQFromLinear(float3 value) {
    float3 p = pow(value, 0.159302f);
	return pow((0.835938f + 18.8516f * p) / (1.0f + 18.6875f * p), 78.8438f);
}

float4 ProcessTexel(uint2 gxy) {
	float3 color = inputTex[gxy].rgb;
	
#if defined(MP_WCG) || defined(MP_HDR)
	// 来自 https://www.itu.int/rec/R-REC-BT.2087-0-201510-I
	static const float3x3 mat709to2020 = float3x3(
        0.6274f, 0.3293f, 0.0433f,
        0.0691f, 0.9195f, 0.0114f,
        0.0164f, 0.0880f, 0.8956f
    );
	color = mul(mat709to2020, color);
#ifdef MP_WCG
	color = BT2020FromLinear(saturate(color));
#else
	// scRGB 的 1.0 表示 80 尼特
	color *= 80.0f / 10000.0f;
	// 防止将负值传入 pow
	color = PQFromLinear(max(color, 0.0f));
#endif
#else
	color = EncodeSrgb(saturate(color));
#endif
	
	return float4(color, 1.0f);
}

[numthreads(8, 8, 1)]
void main(uint3 tid : SV_GroupThreadID, uint3 gid : SV_GroupID) {
	uint2 gxy = (gid.xy << 4) + tid.xy;
	outputTex[gxy] = ProcessTexel(gxy);
	
	gxy.x += 8u;
	outputTex[gxy] = ProcessTexel(gxy);
	
	gxy.y += 8u;
	outputTex[gxy] = ProcessTexel(gxy);
	
	gxy.x -= 8u;
	outputTex[gxy] = ProcessTexel(gxy);
}
