#pragma once
#include "EffectTextureFormat.h"
#include "SmallVector.h"
#include <D3D12MemAlloc.h>

namespace Magpie {

class ComputeContext;
class D3D12Context;
struct EffectDrawerExportData;

class ScreenshotDrawer {
public:
	ScreenshotDrawer() = default;
	ScreenshotDrawer(const ScreenshotDrawer&) = delete;
	ScreenshotDrawer(ScreenshotDrawer&&) = delete;

	~ScreenshotDrawer();

	void Initialize(
		D3D12Context& d3d12Context,
		const winrt::DispatcherQueue& dispatcher,
		const ColorInfo& colorInfo
	) noexcept;

	HRESULT Draw(
		ComputeContext& computeContext,
		uint32_t textureSrvOffset,
		SizeU textureSize,
		uint64_t frameFenceValue
	) noexcept;

	// texture 应处于 NON_PIXEL_SHADER_RESOURCE 状态
	HRESULT Draw(
		ComputeContext& computeContext,
		ID3D12Resource* texture,
		EffectTextureFormat textureFormat,
		SizeU textureSize,
		uint64_t frameFenceValue
	) noexcept;

	void ExportInternalTexture(
		EffectDrawerExportData&& exportData,
		uint64_t frameFenceValue,
		uint32_t effectIdx
	) noexcept;

	void OnColorInfoChanged(const ColorInfo& colorInfo) noexcept;

	void OnProducerFrameCompleted(uint64_t fenceValue) noexcept;

private:
	HRESULT _InitializeScreenshotRootSignature() noexcept;

	HRESULT _DrawImpl(
		ComputeContext& computeContext,
		uint32_t textureSrvOffset,
		ID3D12Resource* texture,
		EffectTextureFormat textureFormat,
		SizeU textureSize,
		uint64_t frameFenceValue
	) noexcept;

	D3D12Context* _d3d12Context = nullptr;
	winrt::DispatcherQueue _dispatcher{ nullptr };
	ColorInfo _colorInfo;

	struct _ScreenshotData {
		uint64_t fenceValue = 0;
		winrt::com_ptr<D3D12MA::Allocation> readbackBufferAllocation;
		void* readbackBufferData = nullptr;
		SizeU textureSize{};
		EffectTextureFormat textureFormat = EffectTextureFormat::R16G16B16A16_UNORM;
		uint32_t rowPitch = 0;
		uint64_t bufferSize = 0;
		// 由于导出是异步的需要备份 colorInfo
		ColorInfo colorInfo;

		// 用于把要导出的纹理转换为 R16G16B16A16_FLOAT 格式
		winrt::com_ptr<D3D12MA::Allocation> tempTextureAllocation;
		uint32_t descriptorBaseOffset = std::numeric_limits<uint32_t>::max();

		// 通常只需要 tempTexture 的 UAV，但导出最终输出画面时需要额外为环形缓冲创建 SRV
		bool hasSRV = false;
	};
	SmallVector<_ScreenshotData> _screenshotDatas;

	winrt::com_ptr<ID3D12RootSignature> _screenshotRootSignature;
	winrt::com_ptr<ID3D12PipelineState> _screenshotPSO;

	uint32_t _pendingScreenshotCount = 0;
};

}
