uint __Bfe(uint src, uint off, uint bits) {
	uint mask = (1u << bits) - 1;
	return (src >> off) & mask;
}

uint __BfiM(uint src, uint ins, uint bits) {
	uint mask = (1u << bits) - 1;
	return (ins & mask) | (src & (~mask));
}

// 来自 https://github.com/GPUOpen-LibrariesAndSDKs/FidelityFX-SDK/blob/8f7498a4b42d06d1daf64b3961ddcaedc07ccaf0/Kits/FidelityFX/api/internal/gpu/ffx_core_gpu_common.h#L2694-L2712
uint2 Rmp8x8(uint a) {
	return uint2(__Bfe(a, 1u, 3u), __BfiM(__Bfe(a, 3u, 3u), a, 1u));
}

// 来自 https://github.com/GPUOpen-LibrariesAndSDKs/FidelityFX-SDK/blob/8f7498a4b42d06d1daf64b3961ddcaedc07ccaf0/Kits/FidelityFX/api/internal/gpu/ffx_core_gpu_common.h#L2598-L2611
float3 EncodeSrgb(float3 c) {
	float3 j = { 0.0031308 * 12.92, 12.92, 1.0 / 2.4 };
	float2 k = { 1.055, -0.055 };
	return clamp(j.x, c * j.y, pow(c, j.z) * k.x + k.y);
}

float3 DecodeSrgb(float3 c) {
    float3 j = { 0.04045, 1.0 / 12.92, 2.4 };
    float2 k = { 1.0 / 1.055, 0.055 / 1.055 };
    return lerp(c * j.y, pow(c * k.x + k.y, j.z), step(j.x, c));
}
