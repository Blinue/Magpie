#pragma once
#include "CatmullRomDrawer.h"
#include "EffectTextureFormat.h"
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

	// 从缩放线程调用。
	// effectIdx 的特殊值:
	// -1: 最终输出画面
	// -2: 捕获的画面
	// passIdx 的特殊值:
	// -1: 效果的输出画面
	void TaskScreenshotAsync(int effectIdx, int passIdx = -1) noexcept;

	void OnResized(SizeU rendererSize, SizeU& outputSize) noexcept;

	void OnColorInfoChanged(const ColorInfo& colorInfo) noexcept;

	void OnProducerFrameCompleted(uint64_t fenceValue) noexcept;

private:
	uint32_t _CalcDescriptorCount() const noexcept;

	void _UpdateEffectBindings() noexcept;

	HRESULT _InitializeScreenshotRootSignature() noexcept;

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
		winrt::com_ptr<ID3D12Resource> outputTexture;
	};

	SmallVector<_EffectData> _effectDatas;
	CatmullRomDrawer _catmullRomDrawer;

	// 描述符的布局是 SRV|UAV|SRV|UAV|...
	uint32_t _descriptorBaseOffset = std::numeric_limits<uint32_t>::max();

	winrt::com_ptr<ID3D12QueryHeap> _queryHeap;
	winrt::com_ptr<ID3D12Resource> _queryResultBuffer;
	UINT64 _timestampFrequency = 0;

	// effectIdx 为 -3 表示无截图任务
	std::atomic<std::pair<int, int>> _screenshotTask = std::make_pair(-3, 0);

	struct _ScreenshotData {
		uint64_t fenceValue = 0;
		winrt::com_ptr<ID3D12Resource> buffer;
		void* bufferData = nullptr;
		uint32_t bufferSize = 0;
		SizeU textureSize{};
		uint32_t rowPitch = 0;
		// 由于导出是异步的需要备份 colorInfo
		ColorInfo colorInfo;

		// 用于把要导出的纹理转换为 R16G16B16A16_FLOAT 格式
		winrt::com_ptr<ID3D12Resource> tempTexture;
		uint32_t descriptorBaseOffset = std::numeric_limits<uint32_t>::max();
		
		// 通常只需要一个描述符，但导出最终输出画面时需要额外为环形缓冲创建 SRV
		bool isExportingRingBuffer = false;
		bool isExported = false;
	};
	SmallVector<_ScreenshotData> _screenshotDatas;

	winrt::com_ptr<ID3D12RootSignature> _screenshotRootSignature;
	winrt::com_ptr<ID3D12PipelineState> _screenshotPSO;

	uint32_t _pendingScreenshotCount = 0;
};

}
