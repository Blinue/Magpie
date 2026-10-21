#include "pch.h"
#include "GpuResourceManager.h"
#include "Logger.h"

namespace Magpie {

bool GpuResourceManager::Initialize(
	ID3D12Device5* device,
	IDXGIAdapter4* dxgiAdapter
) noexcept {
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
	D3D12MA::ALLOCATION_DESC allocDesc = { .HeapType = heapType };

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
	D3D12MA::ALLOCATION_DESC allocDesc = { .HeapType = heapType };

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
	uint64_t fenceValue,
	bool isConsumer
) noexcept {
	SmallVectorImpl<_RetiredResource>& retiredResources =
		isConsumer ? _consumerRetiredResources : _producerRetiredResources;

	// fenceValue 不能减小
	assert(retiredResources.empty() || fenceValue >= retiredResources.back().fenceValue);
	retiredResources.emplace_back(std::move(allocation), fenceValue);
}

void GpuResourceManager::OnFrameCompleted(uint64_t completedFenceValue, bool isConsumer) noexcept {
	SmallVectorImpl<_RetiredResource>& retiredResources =
		isConsumer ? _consumerRetiredResources : _producerRetiredResources;

	// retiredResources 中 fenceValue 按升序排列
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
}

}
