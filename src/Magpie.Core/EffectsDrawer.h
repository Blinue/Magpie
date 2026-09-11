#pragma once
#include "CatmullRomDrawer.h"
#include "SmallVector.h"

namespace Magpie {

class ComputeContext;
class EffectDrawerBase;
struct EffectInfo;

class EffectsDrawer {
public:
	EffectsDrawer() noexcept = default;
	EffectsDrawer(const EffectsDrawer&) = delete;
	EffectsDrawer(EffectsDrawer&&) = delete;

	~EffectsDrawer() noexcept;

	bool Initialize(
		D3D12Context& d3d12Context,
		const ColorInfo& colorInfo,
		SizeU inputSize,
		SizeU rendererSize,
		SizeU& outputSize
	) noexcept;

	HRESULT Draw(
		ComputeContext& computeContext,
		uint32_t frameIndex,
		ID3D12Resource* inputResource,
		ID3D12Resource* outputResource,
		uint32_t inputSrvOffset,
		uint32_t outputUavOffset
	) noexcept;

	SizeU GetOutputSize() const noexcept {
		return _outputSize;
	}

	void OnResized(SizeU rendererSize, SizeU& outputSize) noexcept;

	void OnColorInfoChanged(const ColorInfo& colorInfo) noexcept;

private:
	uint32_t _CalcDescriptorCount() const noexcept;

	void _UpdateEffectBindings() noexcept;

	D3D12Context* _d3d12Context = nullptr;

	SizeU _inputSize{};
	SizeU _outputSize{};
	SizeU _rendererSize{};
	ColorInfo _colorInfo;

	struct _EffectData {
		std::unique_ptr<EffectDrawerBase> drawer;
		const EffectInfo* effectInfo = nullptr;
		SizeU outputSize{};
		winrt::com_ptr<ID3D12Resource> outputTexture;
	};

	SmallVector<_EffectData> _effectDatas;
	CatmullRomDrawer _catmullRomDrawer;

	// 描述符的布局是 SRV|UAV|SRV|UAV|...
	uint32_t _descriptorBaseOffset = std::numeric_limits<uint32_t>::max();

	winrt::com_ptr<ID3D12QueryHeap> _queryHeap;
	winrt::com_ptr<ID3D12Resource> _queryResultBuffer;
	UINT64 _timestampFrequency = 0;
};

}
