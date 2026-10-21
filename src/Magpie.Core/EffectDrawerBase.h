#pragma once
#include "EffectTextureFormat.h"
#include "SmallVector.h"
#include <D3D12MemAlloc.h>

namespace Magpie {

class D3D12Context;
struct EffectOption;
struct EffectInfo;
class ComputeContext;

enum class EffectDrawerState {
	NotReady,
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

	bool IsValid() const noexcept {
		return readbackBufferAllocation &&
			textureSize != SizeU{} &&
			textureFormat > EffectTextureFormat::COLOR_SPACE_ADAPTIVE &&
			textureFormat < EffectTextureFormat::COUNT &&
			textureSize.width * EFFECT_TEXTURE_FORMAT_PROPS[(uint32_t)textureFormat].texelSize <= rowPitch &&
			rowPitch * textureSize.height <= (uint32_t)readbackBufferAllocation->GetResource()->GetDesc().Width;
	}
};

class EffectDrawerBase {
public:
	EffectDrawerBase() = default;
	EffectDrawerBase(const EffectDrawerBase&) = delete;
	EffectDrawerBase(EffectDrawerBase&&) = delete;

	virtual ~EffectDrawerBase() noexcept = default;

	virtual const EffectInfo* Initialize(
		D3D12Context& d3d12Context,
		const EffectOption& effectOption
	) noexcept = 0;

	virtual void Bind(SizeU inputSize, SizeU outputSize, const ColorInfo& colorInfo) noexcept = 0;

	virtual HRESULT Update(EffectDrawerState& state, EffectDebugInfo& debugInfo) noexcept = 0;

	virtual HRESULT Draw(
		ComputeContext& computeContext,
		uint32_t inputSrvOffset,
		uint32_t outputUavOffset,
		EffectDrawerExportData& exportData
	) noexcept = 0;
};

}
