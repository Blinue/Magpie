#pragma once
#include "SmallVector.h"

namespace Magpie {

enum class ShaderEffectTextureFormat {
	UNKNOWN,
	// 在 sRGB 和 scRGB 下提供不同的精度
	COLOR_SPACE_ADAPTIVE,
	R8_UNORM,
	R8_SNORM,
	R16_UNORM,
	R16_SNORM,
	R16_FLOAT,
	R8G8_UNORM,
	R8G8_SNORM,
	R32_FLOAT,
	R16G16_UNORM,
	R16G16_SNORM,
	R16G16_FLOAT,
	R11G11B10_FLOAT,
	R8G8B8A8_UNORM,
	R8G8B8A8_SNORM,
	R10G10B10A2_UNORM,
	R32G32_FLOAT,
	R16G16B16A16_UNORM,
	R16G16B16A16_SNORM,
	R16G16B16A16_FLOAT,
	R32G32B32A32_FLOAT,
	// TODO: 添加不支持采样的格式
	COUNT
};

struct ShaderEffectTextureFormatProps {
	const char* name;
	DXGI_FORMAT dxgiFormat;
	uint32_t nChannel;
	const char* texelType;
	const char* uavTexelType;
};

static constexpr ShaderEffectTextureFormatProps SHADER_TEXTURE_FORMAT_PROPS[] = {
	{"UNKNOWN", DXGI_FORMAT_UNKNOWN, 4, nullptr, nullptr},
	{"COLOR_SPACE_ADAPTIVE", DXGI_FORMAT_UNKNOWN, 4, "MF4", "MF4"},
	{"R8_UNORM", DXGI_FORMAT_R8_UNORM, 1, "MF", "unorm float"},
	{"R8_SNORM", DXGI_FORMAT_R8_SNORM, 1, "MF", "snorm float"},
	{"R16_UNORM", DXGI_FORMAT_R16_UNORM, 1, "MF", "unorm float"},
	{"R16_SNORM", DXGI_FORMAT_R16_SNORM, 1, "MF", "snorm float"},
	{"R16_FLOAT", DXGI_FORMAT_R16_FLOAT, 1, "MF", "MF"},
	{"R8G8_UNORM", DXGI_FORMAT_R8G8_UNORM, 2, "MF2", "unorm float2"},
	{"R8G8_SNORM", DXGI_FORMAT_R8G8_SNORM, 2, "MF2", "snorm float2"},
	{"R32_FLOAT" ,DXGI_FORMAT_R32_FLOAT, 1, "float", "float"},
	{"R16G16_UNORM", DXGI_FORMAT_R16G16_UNORM, 2, "MF2", "unorm float2"},
	{"R16G16_SNORM", DXGI_FORMAT_R16G16_SNORM, 2, "MF2", "snorm float2"},
	{"R16G16_FLOAT", DXGI_FORMAT_R16G16_FLOAT, 2, "MF2", "MF2"},
	{"R11G11B10_FLOAT", DXGI_FORMAT_R11G11B10_FLOAT, 3, "MF3", "MF3"},
	{"R8G8B8A8_UNORM", DXGI_FORMAT_R8G8B8A8_UNORM, 4, "MF4", "unorm float4"},
	{"R8G8B8A8_SNORM", DXGI_FORMAT_R8G8B8A8_SNORM, 4, "MF4", "snorm float4"},
	{"R10G10B10A2_UNORM", DXGI_FORMAT_R10G10B10A2_UNORM, 4, "MF4", "unorm float4"},
	{"R32G32_FLOAT", DXGI_FORMAT_R32G32_FLOAT, 2, "float2", "float2"},
	{"R16G16B16A16_UNORM", DXGI_FORMAT_R16G16B16A16_UNORM, 4, "MF4", "unorm float4"},
	{"R16G16B16A16_SNORM", DXGI_FORMAT_R16G16B16A16_SNORM, 4, "MF4", "snorm float4"},
	{"R16G16B16A16_FLOAT", DXGI_FORMAT_R16G16B16A16_FLOAT, 4, "MF4", "MF4"},
	{"R32G32B32A32_FLOAT", DXGI_FORMAT_R32G32B32A32_FLOAT, 4, "float4", "float4"}
};
static_assert((size_t)ShaderEffectTextureFormat::COUNT == std::size(SHADER_TEXTURE_FORMAT_PROPS));

struct ShaderEffectTextureDesc {
	std::string name;
	ShaderEffectTextureFormat format = ShaderEffectTextureFormat::UNKNOWN;
	std::string widthExpr;
	std::string heightExpr;
	std::string source;
};

enum class ShaderEffectSamplerFilterType {
	Point,
	Linear
};

enum class ShaderEffectSamplerAddressType {
	Clamp,
	Wrap
};

struct ShaderEffectSamplerDesc {
	std::string name;
	ShaderEffectSamplerFilterType filterType = ShaderEffectSamplerFilterType::Point;
	ShaderEffectSamplerAddressType addressType = ShaderEffectSamplerAddressType::Clamp;
};

enum class ShaderEffectPassFlags {
	None = 0,
	PSStyle = 1
};
DEFINE_ENUM_FLAG_OPERATORS(ShaderEffectPassFlags)

struct ShaderEffectPassDesc {
	std::string desc;
	winrt::com_ptr<ID3DBlob> byteCode;
	// 0: INPUT
	// 1: OUTPUT
	// 2+: 中间纹理
	SmallVector<uint32_t> inputs;
	SmallVector<uint32_t> outputs;
	std::array<uint32_t, 3> numThreads{};
	SizeU blockSize{};
	ShaderEffectPassFlags flags = ShaderEffectPassFlags::None;
};

struct ShaderEffectDrawInfo {
	// 不包含 INPUT 和 OUTPUT
	SmallVector<ShaderEffectTextureDesc, 0> textures;
	SmallVector<ShaderEffectSamplerDesc> samplers;
	SmallVector<ShaderEffectPassDesc, 0> passes;
};

}
