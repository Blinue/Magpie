#pragma once
#include <cstdint>
#include <dxgi.h>

namespace Magpie {

struct EffectHelper {
	struct EffectIntermediateTextureFormatDesc {
		const char* name;
		DXGI_FORMAT dxgiFormat;
		uint32_t nChannel;
		const char* srvTexelType;
		const char* uavTexelType;
	};

	static constexpr EffectIntermediateTextureFormatDesc FORMAT_DESCS[] = {
		{"R32G32B32A32_FLOAT", DXGI_FORMAT_R32G32B32A32_FLOAT, 4, "float4", "float4"},
		{"R16G16B16A16_FLOAT", DXGI_FORMAT_R16G16B16A16_FLOAT, 4, "MF4", "MF4"},
		{"R16G16B16A16_UNORM", DXGI_FORMAT_R16G16B16A16_UNORM, 4, "MF4", "unorm MF4"},
		{"R16G16B16A16_SNORM", DXGI_FORMAT_R16G16B16A16_SNORM, 4, "MF4", "snorm MF4"},
		{"R32G32_FLOAT", DXGI_FORMAT_R32G32_FLOAT, 2, "float2", "float2"},
		{"R10G10B10A2_UNORM", DXGI_FORMAT_R10G10B10A2_UNORM, 4, "MF4", "unorm MF4"},
		{"R11G11B10_FLOAT", DXGI_FORMAT_R11G11B10_FLOAT, 3, "MF3", "MF3"},
		{"R8G8B8A8_UNORM", DXGI_FORMAT_R8G8B8A8_UNORM, 4, "MF4", "unorm MF4"},
		{"R8G8B8A8_SNORM", DXGI_FORMAT_R8G8B8A8_SNORM, 4, "MF4", "snorm MF4"},
		{"R16G16_FLOAT", DXGI_FORMAT_R16G16_FLOAT, 2, "MF2", "MF2"},
		{"R16G16_UNORM", DXGI_FORMAT_R16G16_UNORM, 2, "MF2", "unorm MF2"},
		{"R16G16_SNORM", DXGI_FORMAT_R16G16_SNORM, 2, "MF2", "snorm MF2"},
		{"R32_FLOAT" ,DXGI_FORMAT_R32_FLOAT, 1, "float", "float"},
		{"R8G8_UNORM", DXGI_FORMAT_R8G8_UNORM, 2, "MF2", "unorm MF2"},
		{"R8G8_SNORM", DXGI_FORMAT_R8G8_SNORM, 2, "MF2", "snorm MF2"},
		{"R16_FLOAT", DXGI_FORMAT_R16_FLOAT, 1, "MF", "MF"},
		{"R16_UNORM", DXGI_FORMAT_R16_UNORM, 1, "MF", "unorm MF"},
		{"R16_SNORM", DXGI_FORMAT_R16_SNORM,1, "MF", "snorm MF"},
		{"R8_UNORM", DXGI_FORMAT_R8_UNORM, 1, "MF", "unorm MF"},
		{"R8_SNORM", DXGI_FORMAT_R8_SNORM, 1, "MF", "snorm MF"},
		{"UNKNOWN", DXGI_FORMAT_UNKNOWN, 4, "float4", "float4"}
	};

	union Constant32 {
		float floatVal;
		uint32_t uintVal;
		int intVal;
	};
};

}
