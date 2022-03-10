#pragma once
#include "pch.h"
#include <variant>


enum class EffectIntermediateTextureFormat {
	R32G32B32A32_FLOAT,
	R16G16B16A16_FLOAT,
	R16G16B16A16_UNORM,
	R16G16B16A16_SNORM,
	R32G32_FLOAT,
	R10G10B10A2_UNORM,
	R11G11B10_FLOAT,
	R8G8B8A8_UNORM,
	R8G8B8A8_SNORM,
	R16G16_FLOAT,
	R16G16_UNORM,
	R16G16_SNORM,
	R32_FLOAT,
	R8G8_UNORM,
	R8G8_SNORM,
	R16_FLOAT,
	R16_UNORM,
	R16_SNORM,
	R8_UNORM,
	R8_SNORM,
	UNKNOWN
};

struct EffectIntermediateTextureFormatDesc {
	const char* name;
	DXGI_FORMAT dxgiFormat;
	UINT nChannel;
	const char* srvTexelType;
	const char* uavTexelType;
};

struct EffectIntermediateTextureDesc {
	std::pair<std::string, std::string> sizeExpr;
	EffectIntermediateTextureFormat format = EffectIntermediateTextureFormat::UNKNOWN;
	std::string name;
	std::string source;

	inline static const EffectIntermediateTextureFormatDesc FORMAT_DESCS[] = {
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
};

enum class EffectSamplerFilterType {
	Linear,
	Point
};

enum class EffectSamplerAddressType {
	Clamp,
	Wrap
};

struct EffectSamplerDesc {
	EffectSamplerFilterType filterType = EffectSamplerFilterType::Linear;
	EffectSamplerAddressType addressType = EffectSamplerAddressType::Clamp;
	std::string name;
};

enum class EffectConstantType {
	Float,
	Int
};

struct EffectParameterDesc {
	std::string name;
	std::string label;
	EffectConstantType type = EffectConstantType::Float;
	std::variant<float, int> defaultValue;
	std::variant<std::monostate, float, int> minValue;
	std::variant<std::monostate, float, int> maxValue;
};

struct EffectPassDesc {
	winrt::com_ptr<ID3DBlob> cso;
	std::vector<UINT> inputs;
	std::vector<UINT> outputs;
	std::array<UINT, 3> numThreads{};
	std::pair<UINT, UINT> blockSize{};
	bool isPSStyle = false;
};

enum EffectFlags {
	EFFECT_FLAG_LAST_EFFECT = 0x1,
	EFFECT_FLAG_INLINE_PARAMETERS = 0x2
};

struct EffectDesc {
	std::string name;

	// 用于计算效果的输出，空值表示支持任意大小的输出
	std::pair<std::string, std::string> outSizeExpr;

	std::vector<EffectParameterDesc> params;
	std::vector<EffectIntermediateTextureDesc> textures;
	std::vector<EffectSamplerDesc> samplers;

	std::vector<EffectPassDesc> passes;

	UINT flags = 0;
};

struct EffectParams {
	std::optional<std::pair<float, float>> scale;
	std::map<std::string, std::variant<float, int>> params;
};

union EffectConstant32 {
	FLOAT floatVal;
	UINT uintVal;
	INT intVal;
};

