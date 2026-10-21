#include "pch.h"
#include "FrameRingBuffer.h"
#include "D3D12Context.h"
#include "GpuResourceManager.h"
#include "Logger.h"
#include "ScalingWindow.h"
#include "DebugInfo.h"

namespace Magpie {

bool FrameRingBuffer::Initialize(
	D3D12Context& d3d12Context,
	SizeU size,
	const ColorInfo& colorInfo
) noexcept {
	_d3d12Context = &d3d12Context;
	_size = size;
	_isScRGB = colorInfo.kind != winrt::AdvancedColorKind::StandardDynamicRange;

	const uint32_t slotCount = ScalingWindow::Get().Options().maxProducerInFlightFrames + 1;
	_slots.resize(slotCount);

	// 消费者应落后于生产者
	_curConsumerIdx = slotCount - 1;

	ID3D12Device5* device = d3d12Context.GetDevice();

	HRESULT hr = device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&_consumerFence));
	if (FAILED(hr)) {
		Logger::Get().ComError("CreateFence 失败", hr);
		return false;
	}

	hr = device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&_producerFence));
	if (FAILED(hr)) {
		Logger::Get().ComError("CreateFence 失败", hr);
		return false;
	}

	hr = _CreateBuffers();
	if (FAILED(hr)) {
		Logger::Get().ComError("_CreateBuffers 失败", hr);
		return false;
	}
	
	return true;
}

ID3D12Resource* FrameRingBuffer::GetBuffer(uint32_t index) noexcept {
	auto lk = _lock.lock_shared();
	return _slots[index].textureAllocation->GetResource();
}

HRESULT FrameRingBuffer::ProducerBeginFrame(
	ID3D12CommandQueue* commandQueue,
	uint32_t& bufferIdx,
	uint64_t& completedFenceValue,
	uint64_t& frameFenceValue
) noexcept {
	auto lk = _lock.lock_exclusive();

	// 等待消费者命令队列不再使用 _curProducerSlot
	_FrameResourceSlot& curSlot = _slots[_curProducerIdx];
	if (_consumerFence->GetCompletedValue() < curSlot.consumerFenceValue) {
		HRESULT hr = commandQueue->Wait(_consumerFence.get(), curSlot.consumerFenceValue);
		if (FAILED(hr)) {
			Logger::Get().ComError("ID3D12CommandQueue::Wait 失败", hr);
			return hr;
		}
	}

	bufferIdx = _curProducerIdx;
	completedFenceValue = _producerFence->GetCompletedValue();
	frameFenceValue = _slots[_curProducerIdx].producerFenceValue;
	return S_OK;
}

HRESULT FrameRingBuffer::ProducerEndFrame(ID3D12CommandQueue* commandQueue) noexcept {
	auto lk = _lock.lock_exclusive();

	HRESULT hr = commandQueue->Signal(_producerFence.get(), _slots[_curProducerIdx].producerFenceValue);
	if (FAILED(hr)) {
		Logger::Get().ComError("ID3D12CommandQueue::Signal 失败", hr);
		return hr;
	}

	const uint32_t slotCount = (uint32_t)_slots.size();
	const uint32_t nextProducerSlot = (_curProducerIdx + 1) % slotCount;
	if (nextProducerSlot == _curConsumerIdx) {
		uint32_t nextConsumerSlot = (_curConsumerIdx + 1) % slotCount;

		uint64_t fenceValueToWait = _slots[nextConsumerSlot].producerFenceValue;
		if (_producerFence->GetCompletedValue() < fenceValueToWait) {
			lk.reset();
			// 等待新缓冲区可用
			hr = _producerFence->SetEventOnCompletion(fenceValueToWait, nullptr);
			if (FAILED(hr)) {
				Logger::Get().ComError("ID3D12Fence::SetEventOnCompletion 失败", hr);
				return hr;
			}
			lk = _lock.lock_exclusive();

			if (_curConsumerIdx == nextProducerSlot) {
				_curConsumerIdx = nextConsumerSlot;
			}
		} else {
			_curConsumerIdx = nextConsumerSlot;
		}
	}

	uint64_t nextFenceValue = _slots[_curProducerIdx].producerFenceValue + 1;
	_curProducerIdx = nextProducerSlot;
	_slots[nextProducerSlot].producerFenceValue = nextFenceValue;

#ifdef MP_DEBUG_INFO
	{
		auto debugLock = DEBUG_INFO.lock.lock_exclusive();
		// 在这里计算的 consumerLatency 不准确
		DEBUG_INFO.producerFrameNumber = (uint32_t)nextFenceValue;
	}
#endif

	return S_OK;
}

bool FrameRingBuffer::ConsumerBeginFrame(
	uint32_t& bufferIdx,
	ID3D12Resource*& frame,
	uint64_t& completedFenceValue,
	uint64_t& fenceValueToSignal
) noexcept {
	auto lk = _lock.lock_exclusive();

	if (_curConsumerIdx != _curProducerIdx) {
		const uint64_t producerCompletedFenceValue = _producerFence->GetCompletedValue();
		if (producerCompletedFenceValue == 0) {
			// 第一帧尚未完成
			return false;
		}

		const uint32_t slotCount = (uint32_t)_slots.size();
		uint32_t nextConsumerSlot = (_curConsumerIdx + 1) % slotCount;
		if (producerCompletedFenceValue >= _slots[nextConsumerSlot].producerFenceValue) {
			_curConsumerIdx = nextConsumerSlot;

			// 寻找最新帧
			while (true) {
				nextConsumerSlot = (_curConsumerIdx + 1) % slotCount;
				if (producerCompletedFenceValue < _slots[nextConsumerSlot].producerFenceValue) {
					break;
				}

				_curConsumerIdx = nextConsumerSlot;

				// 窗口很小，但有发生的可能
				if (_curConsumerIdx == _curProducerIdx) {
					break;
				}
			}
		}
	}

	_FrameResourceSlot& curSlot = _slots[_curConsumerIdx];
	fenceValueToSignal = ++_curConsumerFenceValue;
	curSlot.consumerFenceValue = fenceValueToSignal;

	bufferIdx = _curConsumerIdx;
	frame = curSlot.textureAllocation->GetResource();
	completedFenceValue = _consumerFence->GetCompletedValue();

	return true;
}

HRESULT FrameRingBuffer::ConsumerEndFrame(
	ID3D12CommandQueue* commandQueue,
	uint64_t fenceValueToSignal
) const noexcept {
	HRESULT hr = commandQueue->Signal(_consumerFence.get(), fenceValueToSignal);
	if (FAILED(hr)) {
		Logger::Get().ComError("ID3D12CommandQueue::Signal 失败", hr);
		return hr;
	}

#ifdef MP_DEBUG_INFO
	{
		auto debugLock = DEBUG_INFO.lock.lock_exclusive();
		DEBUG_INFO.consumerFrameNumber = (uint32_t)_slots[_curConsumerIdx].producerFenceValue;
		DEBUG_INFO.consumerLatency = DEBUG_INFO.producerFrameNumber - DEBUG_INFO.consumerFrameNumber;
	}
#endif

	return S_OK;
}

HRESULT FrameRingBuffer::SetEventOnNewFrame(uint64_t& fenceValue, HANDLE hEvent) const noexcept {
	assert(fenceValue != 0);

	HRESULT hr = _producerFence->SetEventOnCompletion(fenceValue, hEvent);
	if (FAILED(hr)) {
		Logger::Get().ComError("ID3D12Fence::SetEventOnCompletion 失败", hr);
		return hr;
	}

	// 下一个要等待的值
	fenceValue = std::max(GetProduerCompletedFenceValue(), fenceValue) + 1;
	return S_OK;
}

uint64_t FrameRingBuffer::GetProduerCompletedFenceValue() const noexcept {
	return _producerFence->GetCompletedValue();
}

HRESULT FrameRingBuffer::OnResized(SizeU size) noexcept {
	_size = size;

	HRESULT hr = _CreateBuffers();
	if (FAILED(hr)) {
		Logger::Get().ComError("_CreateBuffers 失败", hr);
		return hr;
	}

	return S_OK;
}

HRESULT FrameRingBuffer::OnColorInfoChanged(const ColorInfo& colorInfo) noexcept {
	const bool wasScRGB = _isScRGB;
	_isScRGB = colorInfo.kind != winrt::AdvancedColorKind::StandardDynamicRange;

	if (_isScRGB == wasScRGB) {
		return S_OK;
	}

	HRESULT hr = _CreateBuffers();
	if (FAILED(hr)) {
		Logger::Get().ComError("_CreateBuffers 失败", hr);
		return hr;
	}

	return S_OK;
}

HRESULT FrameRingBuffer::_CreateBuffers() noexcept {
	auto& gpuResourceManager = _d3d12Context->GetGpuResourceManager();

	DXGI_FORMAT textureFormat =
		_isScRGB ? DXGI_FORMAT_R16G16B16A16_FLOAT : DXGI_FORMAT_R8G8B8A8_UNORM;

	for (_FrameResourceSlot& slot : _slots) {
		HRESULT hr = gpuResourceManager.CreateTexture(
			D3D12_HEAP_TYPE_DEFAULT,
			textureFormat,
			_size,
			D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
			D3D12_RESOURCE_STATE_COMMON,
			slot.textureAllocation.put()
		);
		if (FAILED(hr)) {
			Logger::Get().ComError("GpuResourceManager::CreateTexture 失败", hr);
			return hr;
		}
	}

	return S_OK;
}

}
