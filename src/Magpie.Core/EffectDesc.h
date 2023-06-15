#pragma once
#include <variant>
#include "SmallVector.h"

struct ID3D10Blob;
typedef ID3D10Blob ID3DBlob;

namespace Magpie::Core {

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

struct EffectIntermediateTextureDesc {
	std::pair<std::string, std::string> sizeExpr;
	EffectIntermediateTextureFormat format = EffectIntermediateTextureFormat::UNKNOWN;
	std::string name;
	std::string source;
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

template <typename T>
struct EffectConstant {
	T defaultValue;
	T minValue;
	T maxValue;
	T step;
};

struct EffectParameterDesc {
	std::string name;
	std::string label;
	std::variant<EffectConstant<float>, EffectConstant<int>> constant;
};

struct EffectPassDesc {
	winrt::com_ptr<ID3DBlob> cso;
	SmallVector<uint32_t> inputs;
	SmallVector<uint32_t> outputs;
	std::array<uint32_t, 3> numThreads{};
	std::pair<uint32_t, uint32_t> blockSize{};
	std::string desc;
	bool isPSStyle = false;
};

struct EffectFlags {
	// 输入
	static constexpr const uint32_t InlineParams = 1;
	static constexpr const uint32_t FP16 = 1 << 1;
	// 输出
	// 此效果需要帧数和鼠标位置
	static constexpr const uint32_t UseDynamic = 1 << 4;
};

struct EffectDesc {
	std::string name;
	std::string sortName;	// 仅供 UI 使用

	const std::pair<std::string, std::string>& GetOutputSizeExpr() const noexcept {
		return textures[1].sizeExpr;
	}

	std::vector<EffectParameterDesc> params;
	// 0: INPUT
	// 1: OUTPUT
	// > 1: 中间纹理
	std::vector<EffectIntermediateTextureDesc> textures;
	std::vector<EffectSamplerDesc> samplers;
	std::vector<EffectPassDesc> passes;

	uint32_t flags = 0;	// EffectFlags
};

}
