#pragma once
#include <dxgi.h>
#include <cstdint>

namespace Magpie::Core {

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
		{"R16G16B16A16_FLOAT", DXGI_FORMAT_R16G16B16A16_FLOAT, 4, "float4", "float4"},
		{"R16G16B16A16_UNORM", DXGI_FORMAT_R16G16B16A16_UNORM, 4, "float4", "unorm float4"},
		{"R16G16B16A16_SNORM", DXGI_FORMAT_R16G16B16A16_SNORM, 4, "float4", "snorm float4"},
		{"R32G32_FLOAT", DXGI_FORMAT_R32G32_FLOAT, 2, "float2", "float2"},
		{"R10G10B10A2_UNORM", DXGI_FORMAT_R10G10B10A2_UNORM, 4, "float4", "unorm float4"},
		{"R11G11B10_FLOAT", DXGI_FORMAT_R11G11B10_FLOAT, 3, "float3", "float3"},
		{"R8G8B8A8_UNORM", DXGI_FORMAT_R8G8B8A8_UNORM, 4, "float4", "unorm float4"},
		{"R8G8B8A8_SNORM", DXGI_FORMAT_R8G8B8A8_SNORM, 4, "float4", "snorm float4"},
		{"R16G16_FLOAT", DXGI_FORMAT_R16G16_FLOAT, 2, "float2", "float2"},
		{"R16G16_UNORM", DXGI_FORMAT_R16G16_UNORM, 2, "float2", "unorm float2"},
		{"R16G16_SNORM", DXGI_FORMAT_R16G16_SNORM, 2, "float2", "snorm float2"},
		{"R32_FLOAT" ,DXGI_FORMAT_R32_FLOAT, 1, "float", "float"},
		{"R8G8_UNORM", DXGI_FORMAT_R8G8_UNORM, 2, "float2", "unorm float2"},
		{"R8G8_SNORM", DXGI_FORMAT_R8G8_SNORM, 2, "float2", "snorm float2"},
		{"R16_FLOAT", DXGI_FORMAT_R16_FLOAT, 1, "float", "float"},
		{"R16_UNORM", DXGI_FORMAT_R16_UNORM, 1, "float", "unorm float"},
		{"R16_SNORM", DXGI_FORMAT_R16_SNORM,1, "float", "snorm float"},
		{"R8_UNORM", DXGI_FORMAT_R8_UNORM, 1, "float", "unorm float"},
		{"R8_SNORM", DXGI_FORMAT_R8_SNORM, 1, "float", "snorm float"},
		{"UNKNOWN", DXGI_FORMAT_UNKNOWN, 4, "float4", "float4"}
	};

	union Constant32 {
		float floatVal;
		uint32_t uintVal;
		int intVal;
	};
};

}
