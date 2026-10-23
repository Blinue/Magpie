#include "pch.h"
#include "GpuResourceManager.h"
#include "DescriptorHeap.h"
#include "Logger.h"

namespace Magpie {

bool GpuResourceManager::Initialize(
	ID3D12Device5* device,
	IDXGIAdapter4* dxgiAdapter,
	DescriptorHeap& csuDescriptorHeap,
	DescriptorHeap& rtvDescriptorHeap
) noexcept {
	_csuDescriptorHeap = &csuDescriptorHeap;
	_rtvDescriptorHeap = &rtvDescriptorHeap;

	D3D12MA::ALLOCATOR_DESC allocatorDesc = {
		.Flags = (D3D12MA::ALLOCATOR_FLAGS)D3D12MA_RECOMMENDED_ALLOCATOR_FLAGS,
		.pDevice = device,
		.pAdapter = dxgiAdapter
	};

	HRESULT hr = D3D12MA::CreateAllocator(&allocatorDesc, _allocator.put());
	if (FAILED(hr)) {
		Logger::Get().ComError("D3D12MA::CreateAllocator 失败", hr);
		return false;
	}

	return true;
}

HRESULT GpuResourceManager::CreateBuffer(
	D3D12_HEAP_TYPE heapType,
	UINT64 bufferSize,
	D3D12_RESOURCE_STATES initialResourceState,
	D3D12MA::Allocation** allocation
) const noexcept {
	D3D12MA::CALLOCATION_DESC allocDesc(heapType);

	CD3DX12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);

	HRESULT hr = _allocator->CreateResource(&allocDesc, &bufferDesc,
		initialResourceState, nullptr, allocation, IID_NULL, nullptr);
	if (FAILED(hr)) {
		Logger::Get().ComError("D3D12MA::Allocator::CreateResource 失败", hr);
		return hr;
	}

	return S_OK;
}

HRESULT GpuResourceManager::CreateTexture(
	D3D12_HEAP_TYPE heapType,
	DXGI_FORMAT format,
	SizeU size,
	D3D12_RESOURCE_FLAGS flags,
	D3D12_RESOURCE_STATES initialResourceState,
	D3D12MA::Allocation** allocation
) const noexcept {
	D3D12MA::CALLOCATION_DESC allocDesc(heapType);

	CD3DX12_RESOURCE_DESC texDesc = CD3DX12_RESOURCE_DESC::Tex2D(
		format, size.width, size.height, 1, 1, 1, 0, flags);

	HRESULT hr = _allocator->CreateResource(
		&allocDesc,
		&texDesc,
		initialResourceState,
		nullptr,
		allocation,
		IID_NULL,
		nullptr
	);
	if (FAILED(hr)) {
		Logger::Get().ComError("D3D12MA::Allocator::CreateResource 失败", hr);
		return hr;
	}

	return S_OK;
}

void GpuResourceManager::RetireResource(
	winrt::com_ptr<D3D12MA::Allocation>&& allocation,
	bool isConsumer
) noexcept {
	if (isConsumer) {
		_consumerRetiredResources.emplace_back(std::move(allocation), _curConsumerFrameFenceValue);
	} else {
		_producerRetiredResources.emplace_back(std::move(allocation), _curProducerFrameFenceValue);
	}
}

void GpuResourceManager::RetireDescriptor(
	uint32_t offset,
	uint32_t count,
	bool isRtv,
	bool isConsumer
) noexcept {
	assert(offset != std::numeric_limits<uint32_t>::max());

	if (isConsumer) {
		_consumerRetiredDescriptors.emplace_back(offset, count, isRtv, _curConsumerFrameFenceValue);
	} else {
		_producerRetiredDescriptors.emplace_back(offset, count, isRtv, _curProducerFrameFenceValue);
	}
}

void GpuResourceManager::OnFrameBegan(uint64_t frameFenceValue, bool isConsumer) noexcept {
	if (isConsumer) {
		assert(frameFenceValue > _curConsumerFrameFenceValue);
		_curConsumerFrameFenceValue = frameFenceValue;
	} else {
		assert(frameFenceValue > _curProducerFrameFenceValue);
		_curProducerFrameFenceValue = frameFenceValue;
	}
}

void GpuResourceManager::OnFrameCompleted(uint64_t completedFenceValue, bool isConsumer) noexcept {
	SmallVectorImpl<_RetiredResource>& retiredResources =
		isConsumer ? _consumerRetiredResources : _producerRetiredResources;
	SmallVectorImpl<_RetiredDescriptor>& retiredDescriptors =
		isConsumer ? _consumerRetiredDescriptors : _producerRetiredDescriptors;

	// retiredResources 和 retiredDescriptors 中 fenceValue 按升序排列
	auto it = std::upper_bound(
		retiredResources.begin(),
		retiredResources.end(),
		completedFenceValue,
		[](uint64_t completedFenceValue, const _RetiredResource& res) {
			return completedFenceValue < res.fenceValue;
		}
	);
	if (it != retiredResources.begin()) {
		retiredResources.erase(retiredResources.begin(), it);
	}

	auto it1 = std::upper_bound(
		retiredDescriptors.begin(),
		retiredDescriptors.end(),
		completedFenceValue,
		[](uint64_t completedFenceValue, const _RetiredDescriptor& res) {
			return completedFenceValue < res.fenceValue;
		}
	);
	if (it1 != retiredDescriptors.begin()) {
		for (auto it2 = retiredDescriptors.begin(); it2 < it1; ++it2) {
			(it2->isRtv ? _rtvDescriptorHeap : _csuDescriptorHeap)->Free(it2->offset, it2->count);
		}
		retiredDescriptors.erase(retiredDescriptors.begin(), it1);
	}
}

}
