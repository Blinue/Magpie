#pragma once
#include "pch.h"
#include <variant>


enum class EffectIntermediateTextureFormat {
	R8_UNORM,
	R16_UNORM,
	R16_FLOAT,
	R8G8_UNORM,
	B5G6R5_UNORM,
	R16G16_UNORM,
	R16G16_FLOAT,
	R8G8B8A8_UNORM,
	B8G8R8A8_UNORM,
	R10G10B10A2_UNORM,
	R32_FLOAT,
	R11G11B10_FLOAT,
	R32G32_FLOAT,
	R16G16B16A16_UNORM,
	R16G16B16A16_FLOAT,
	R32G32B32A32_FLOAT
};

struct EffectIntermediateTextureDesc {
	std::pair<std::string, std::string> sizeExpr;
	EffectIntermediateTextureFormat format = EffectIntermediateTextureFormat::R8G8B8A8_UNORM;
	std::string name;
	std::string source;

	inline static const DXGI_FORMAT DXGI_FORMAT_MAP[16]{
		DXGI_FORMAT_R8_UNORM,
		DXGI_FORMAT_R16_UNORM,
		DXGI_FORMAT_R16_FLOAT,
		DXGI_FORMAT_R8G8_UNORM,
		DXGI_FORMAT_B5G6R5_UNORM,
		DXGI_FORMAT_R16G16_UNORM,
		DXGI_FORMAT_R16G16_FLOAT,
		DXGI_FORMAT_R8G8B8A8_UNORM,
		DXGI_FORMAT_B8G8R8A8_UNORM,
		DXGI_FORMAT_R10G10B10A2_UNORM,
		DXGI_FORMAT_R32_FLOAT,
		DXGI_FORMAT_R11G11B10_FLOAT,
		DXGI_FORMAT_R32G32_FLOAT,
		DXGI_FORMAT_R16G16B16A16_UNORM,
		DXGI_FORMAT_R16G16B16A16_FLOAT,
		DXGI_FORMAT_R32G32B32A32_FLOAT
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

