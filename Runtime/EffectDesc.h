#pragma once
#include "pch.h"
#include <variant>


enum class EffectIntermediateTextureFormat {
	B8G8R8A8_UNORM,
	R16G16B16A16_FLOAT
};

struct EffectIntermediateTextureDesc {
	std::pair<std::string, std::string> sizeExpr;
	EffectIntermediateTextureFormat format = EffectIntermediateTextureFormat::B8G8R8A8_UNORM;
	std::string name;
};

enum class EffectSamplerFilterType {
	Linear,
	Point
};

struct EffectSamplerDesc {
	EffectSamplerFilterType filterType = EffectSamplerFilterType::Linear;
	std::string name;
};

enum class EffectConstantType {
	Float,
	Int
};

struct EffectValueConstantDesc {
	std::string name;
	EffectConstantType type = EffectConstantType::Float;
	std::string valueExpr;
};

struct EffectConstantDesc {
	std::string name;
	std::string label;
	EffectConstantType type = EffectConstantType::Float;
	std::variant<float, int> defaultValue;
	std::variant<std::monostate, float, int> minValue;
	std::variant<std::monostate, float, int> maxValue;
};

struct EffectPassDesc {
	std::vector<UINT> inputs;
	std::vector<UINT> outputs;
	ComPtr<ID3DBlob> cso;
};

struct EffectDesc {
	UINT version;
	
	// 用于计算效果的输出，空值表示支持任意大小的输出
	std::pair<std::string, std::string> outSizeExpr;

	std::vector<EffectConstantDesc> constants;
	std::vector<EffectValueConstantDesc> valueConstants;

	std::vector<EffectIntermediateTextureDesc> textures;
	std::vector<EffectSamplerDesc> samplers;

	std::vector<EffectPassDesc> passes;
};
