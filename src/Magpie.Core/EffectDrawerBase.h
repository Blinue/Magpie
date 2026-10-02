#pragma once

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

	virtual HRESULT Update(EffectDrawerState& state, std::string& message) noexcept = 0;

	virtual HRESULT Draw(
		ComputeContext& computeContext,
		uint32_t inputSrvOffset,
		uint32_t outputUavOffset
	) noexcept = 0;
};

}
