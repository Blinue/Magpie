#pragma once
#include "SmallVector.h"

namespace Magpie {

class D3D12Context;

class FrameRingBuffer {
public:
	FrameRingBuffer() = default;
	FrameRingBuffer(const FrameRingBuffer&) = delete;
	FrameRingBuffer(FrameRingBuffer&&) = delete;

	bool Initialize(
		D3D12Context& d3d12Context,
		SizeU size,
		const ColorInfo& colorInfo
	) noexcept;

	ID3D12Resource* GetBuffer(uint32_t index) noexcept;

	HRESULT ProducerBeginFrame(
		ID3D12CommandQueue* commandQueue,
		uint32_t& bufferIdx
	) noexcept;

	HRESULT ProducerEndFrame(ID3D12CommandQueue* commandQueue) noexcept;

	bool ConsumerBeginFrame(
		uint32_t& bufferIdx,
		ID3D12Resource*& frame,
		uint64_t& completedFenceValue,
		uint64_t& fenceValueToSignal
	) noexcept;

	HRESULT ConsumerEndFrame(
		ID3D12CommandQueue* commandQueue,
		uint64_t fenceValueToSignal
	) const noexcept;

	HRESULT SetEventOnNewFrame(uint64_t& frameNumber, HANDLE hEvent) const noexcept;

	uint64_t GetLatestFrameNumber() const noexcept;

	HRESULT OnResized(SizeU size) noexcept;

	HRESULT OnColorInfoChanged(const ColorInfo& colorInfo) noexcept;

private:
	HRESULT _CreateBuffers() noexcept;

	// 只在生产者线程访问
	D3D12Context* _d3d12Context = nullptr;

	wil::srwlock _lock;

	struct _FrameResourceSlot {
		// 生产者和消费者应确保使用结束后此资源处于 COPY_SOURCE 状态。有两个方面的考虑：
		// 1. 文档说：当一个资源在某个队列上转换到了可写状态时，该资源被视为由该队列独占拥有。在该
		// 资源被另一个队列访问之前，它必须先转换到只读或 COMMON 状态。
		// 2. 消费者使用共享纹理的频率比生产者更高，因此选择对消费者更友好的 COPY_SOURCE 状态。
		winrt::com_ptr<ID3D12Resource> resource;
		uint64_t consumerFenceValue = 0;
		uint64_t producerFenceValue = 1;
	};

	std::vector<_FrameResourceSlot> _slots;
	uint32_t _curConsumerIdx = 0;
	uint32_t _curProducerIdx = 0;

	winrt::com_ptr<ID3D12Fence1> _consumerFence;
	uint64_t _curConsumerFenceValue = 0;
	winrt::com_ptr<ID3D12Fence1> _producerFence;

	SizeU _size{};
	bool _isScRGB = false;
};

}
