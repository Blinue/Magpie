#include "Common.hlsli"

Texture2D<float4> inputTex : register(t0);
RWTexture2D<float4> outputTex : register(u0);

// BT.2020 的光电转换函数比较混乱。维基百科、ITU-R BT.2020-2 建议书和 PNG 标准中
// BT.2020 的光电转换函数是和 sRGB 类似的分段函数，但 Chrome、CSS 标准和图片查看
// 器等实现全部使用简单的伽马 2.4。目前看来，伽马 2.4 已经成为事实上的标准。
// 见 https://github.com/w3c/csswg-drafts/issues/12574
// 我们在图片中嵌入的 icc (BT2020_G24.icc) 也使用伽马 2.4。
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
