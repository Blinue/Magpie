#pragma once
#include "EffectTextureFormat.h"
#include "SmallVector.h"
#include <D3D12MemAlloc.h>

namespace Magpie {

class D3D12Context;
struct EffectOption;
class ComputeContext;
class EffectsMemPoolBuilder;

enum class EffectDrawerFlags {
	None,
	// 默认情况下效果的输出纹理确保在效果渲染期间始终有效，因此输出纹理不能和该效果的
	// 中间纹理重叠，这会稍微增加显存占用。指定这个标志表示无需这个保证，由效果自行插
	// 入必要的别名屏障。
	SupportOutputAliasing = 1
};
DEFINE_ENUM_FLAG_OPERATORS(EffectDrawerFlags)

struct EffectDrawerInfo {
	// 0 表示可以自由缩放
	uint32_t scaleFactor = 0;
	// 0 表示不限制
	uint32_t minFrameRate = 0;
	EffectDrawerFlags flags = EffectDrawerFlags::None;
};

enum class EffectDrawerState {
	NotReady,
	WantUpdateResources,
	Ready,
	Error
};

struct EffectPassInfo {
	std::string desc;
	SmallVector<std::string> outputs;
};

struct EffectDebugInfo {
	SmallVector<EffectPassInfo, 0> passInfos;
};

struct EffectDrawerExportData {
	int passIdx = -1;
	uint32_t outputIdx = 0;

	winrt::com_ptr<D3D12MA::Allocation> readbackBufferAllocation;
	SizeU textureSize{};
	EffectTextureFormat textureFormat = EffectTextureFormat::UNKNOWN;
	uint32_t rowPitch = 0;
};

class EffectDrawerBase {
public:
	EffectDrawerBase() = default;
	EffectDrawerBase(const EffectDrawerBase&) = delete;
	EffectDrawerBase(EffectDrawerBase&&) = delete;

	virtual ~EffectDrawerBase() noexcept = default;

	virtual HRESULT Initialize(
		D3D12Context& d3d12Context,
		const EffectOption& effectOption,
		EffectDrawerInfo& effectDrawerInfo
	) noexcept = 0;

	virtual void Bind(SizeU inputSize, SizeU outputSize, const ColorInfo& colorInfo) noexcept = 0;

	virtual HRESULT Update(EffectDrawerState& state, EffectDebugInfo& debugInfo) noexcept = 0;

	virtual void BuildMemPool(EffectsMemPoolBuilder& /*memPoolBuilder*/) noexcept {}

	virtual HRESULT CreateResourcesInMemPool(
		D3D12MA::Allocation* /*textureAllocation*/,
		D3D12MA::Allocation* /*bufferAllocation*/,
		std::span<const uint64_t> /*resourceOffsets*/
	) noexcept {
		return E_NOTIMPL;
	}

	// 如果指定了 SupportOutputAliasing 且 aliasedOutput 不为空，那么写入输出
	// 纹理前必须插入别名屏障。
	virtual HRESULT Draw(
		ComputeContext& computeContext,
		uint32_t inputSrvOffset,
		uint32_t outputUavOffset,
		ID3D12Resource* aliasedOutput,
		EffectDrawerExportData& exportData
	) noexcept = 0;
};

}
