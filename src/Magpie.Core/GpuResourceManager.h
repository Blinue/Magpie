#pragma once
#include "SmallVector.h"
#include <D3D12MemAlloc.h>

namespace Magpie {

class DescriptorHeap;

class GpuResourceManager {
public:
	GpuResourceManager() = default;
	GpuResourceManager(const GpuResourceManager&) = delete;
	GpuResourceManager(GpuResourceManager&&) = delete;

	bool Initialize(
		ID3D12Device5* device,
		IDXGIAdapter4* dxgiAdapter,
		DescriptorHeap& csuDescriptorHeap,
		DescriptorHeap& rtvDescriptorHeap
	) noexcept;

	HRESULT CreateBuffer(
		D3D12_HEAP_TYPE heapType,
		UINT64 bufferSize,
		D3D12_RESOURCE_STATES initialResourceState,
		D3D12MA::Allocation** allocation
	) const noexcept;

	HRESULT CreateTexture(
		D3D12_HEAP_TYPE heapType,
		DXGI_FORMAT format,
		SizeU size,
		D3D12_RESOURCE_FLAGS flags,
		D3D12_RESOURCE_STATES initialResourceState,
		D3D12MA::Allocation** allocation
	) const noexcept;

	void RetireResource(winrt::com_ptr<D3D12MA::Allocation>&& allocation, bool isConsumer) noexcept;

	void RetireDescriptor(
		uint32_t offset,
		uint32_t count,
		bool isRtv,
		bool isConsumer
	) noexcept;

	void OnFrameBegan(uint64_t frameFenceValue, bool isConsumer) noexcept;

	void OnFrameCompleted(uint64_t completedFenceValue, bool isConsumer) noexcept;

private:
	DescriptorHeap* _csuDescriptorHeap = nullptr;
	DescriptorHeap* _rtvDescriptorHeap = nullptr;

	winrt::com_ptr<D3D12MA::Allocator> _allocator;

	uint64_t _curProducerFrameFenceValue = 0;
	uint64_t _curConsumerFrameFenceValue = 0;

	struct _RetiredResource {
		winrt::com_ptr<D3D12MA::Allocation> allocation;
		uint64_t fenceValue = 0;
	};

	struct _RetiredDescriptor {
		uint32_t offset = std::numeric_limits<uint32_t>::max();
		uint32_t count = 0;
		bool isRtv = false;
		uint64_t fenceValue = 0;
	};

	// 生产者和消费者的退休资源分开存储，这可以:
	// 1. 避免加锁
	// 2. 使 fenceValue 按升序排列
	SmallVector<_RetiredResource, 0> _producerRetiredResources;
	SmallVector<_RetiredResource, 0> _consumerRetiredResources;
	SmallVector<_RetiredDescriptor, 0> _producerRetiredDescriptors;
	SmallVector<_RetiredDescriptor, 0> _consumerRetiredDescriptors;
};

}
