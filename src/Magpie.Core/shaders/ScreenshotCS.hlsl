#include "Common.hlsli"

Texture2D<float4> inputTex : register(t0);
RWTexture2D<float4> outputTex : register(u0);

#ifdef MP_WCG
// 来自 https://registry.color.org/rgb-registry/bt2020
float3 BT2020Transfer(float3 color) {
	return pow(color, 1.0f / 2.4f);
}
#endif

#ifdef MP_HDR
// 来自 https://github.com/GPUOpen-LibrariesAndSDKs/FidelityFX-SDK/blob/8f7498a4b42d06d1daf64b3961ddcaedc07ccaf0/Kits/FidelityFX/api/internal/gpu/ffx_core_gpu_common.h#L2536-L2548
float3 PQFromLinear(float3 value) {
	float3 p = pow(value, 0.159302f);
	return pow((0.835938f + 18.8516f * p) / (1.0f + 18.6875f * p), 78.8438f);
}
#endif

float4 ProcessTexel(uint2 gxy) {
	float3 color = inputTex[gxy].rgb;
	
#if defined(MP_WCG) || defined(MP_HDR)
	// 来自 https://www.itu.int/rec/R-REC-BT.2087-0-201510-I
	static const float3x3 mat709to2020 = float3x3(
        0.6274f, 0.3293f, 0.0433f,
        0.0691f, 0.9195f, 0.0114f,
        0.0164f, 0.0880f, 0.8956f
    );
	color = mul(mat709to2020, saturate(color));
#ifdef MP_WCG
	color = BT2020Transfer(color);
#else
	color = PQFromLinear(color);
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
