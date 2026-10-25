#pragma once
#include "CatmullRomDrawer.h"
#include "D3D12Context.h"
#include "EffectDrawerBase.h"
#include "ScreenshotDrawer.h"
#include "SmallVector.h"
#include <D3D12MemAlloc.h>

namespace Magpie {

class ComputeContext;
struct EffectInfo;

class EffectsDrawer {
public:
	EffectsDrawer() noexcept = default;
	EffectsDrawer(const EffectsDrawer&) = delete;
	EffectsDrawer(EffectsDrawer&&) = delete;

	~EffectsDrawer() noexcept;

	bool Initialize(
		D3D12Context& d3d12Context,
		const winrt::DispatcherQueue& dispatcher,
		const ColorInfo& colorInfo,
		SizeU inputSize,
		SizeU rendererSize,
		SizeU& outputSize
	) noexcept;

	// outputResource 处于 COMMON 状态，返回时也处于此状态
	HRESULT Draw(
		ComputeContext& computeContext,
		uint32_t frameIndex,
		ID3D12Resource* inputResource,
		ID3D12Resource* outputResource,
		uint32_t inputSrvOffset,
		uint32_t outputUavOffset,
		uint64_t completedFenceValue,
		uint64_t frameFenceValue
	) noexcept;

	SizeU GetOutputSize() const noexcept {
		return _outputSize;
	}

	// 从缩放线程调用。effectIdx 和 passIdx 有最小值限制，但无最大值限制，超
	// 出范围将取消截图。
	// effectIdx 的特殊值:
	// -1: 最终输出画面
	// -2: 捕获的画面
	// passIdx 的特殊值:
	// -1: 效果的输出画面
	void ExportAsync(int effectIdx, int passIdx = -1, uint32_t outputIdx = 0) noexcept;

	HRESULT OnResized(SizeU rendererSize, SizeU& outputSize) noexcept;

	HRESULT OnColorInfoChanged(const ColorInfo& colorInfo) noexcept;

	void OnProducerFrameCompleted(uint64_t fenceValue) noexcept;

private:
	uint32_t _CalcDescriptorCount() const noexcept;

	void _UpdateEffectBindings() noexcept;

	HRESULT _UpdateEffectOutputTextures() noexcept;

	D3D12Context* _d3d12Context = nullptr;
	winrt::DispatcherQueue _dispatcher{ nullptr };

	SizeU _inputSize{};
	SizeU _outputSize{};
	SizeU _rendererSize{};
	ColorInfo _colorInfo;

	struct _EffectData {
		std::unique_ptr<EffectDrawerBase> drawer;
		const EffectInfo* effectInfo = nullptr;
		SizeU outputSize{};
		winrt::com_ptr<D3D12MA::Allocation> outputTextureAllocation;
		EffectDebugInfo debugInfo;
	};

	SmallVector<_EffectData> _effectDatas;
	CatmullRomDrawer _catmullRomDrawer;
	ScreenshotDrawer _screenshotDrawer;

	// 描述符的布局是 SRV|UAV|SRV|UAV|...
	uint32_t _descriptorBaseOffset = std::numeric_limits<uint32_t>::max();

	winrt::com_ptr<ID3D12QueryHeap> _queryHeap;
	winrt::com_ptr<D3D12MA::Allocation> _queryResultBufferAllocation;
	UINT64 _timestampFrequency = 0;

	struct _ExportTask {
		// -3 表示无截图任务
		int effectIdx = -3;
		int passIdx = -1;
		uint32_t outputIdx = 0;
	};
	// 不知为何如果默认初始化 clang-cl 会编译失败
	std::atomic<_ExportTask> _exportTask{};
};

}
