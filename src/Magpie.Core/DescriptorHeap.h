#pragma once
#ifndef _DEBUG
#include <parallel_hashmap/btree.h>
#endif

namespace Magpie {

class DescriptorHeap {
public:
	DescriptorHeap() = default;
	DescriptorHeap(const DescriptorHeap&) = delete;
	DescriptorHeap(DescriptorHeap&&) = delete;

	~DescriptorHeap() noexcept;

	bool Initialize(
		ID3D12Device5* device,
		D3D12_DESCRIPTOR_HEAP_TYPE type,
		uint32_t capacity
	) noexcept;

	HRESULT Alloc(uint32_t count, uint32_t& offset) noexcept;

	void Free(uint32_t offset, uint32_t count) noexcept;

	ID3D12DescriptorHeap* GetHeap() const noexcept {
		return _heap.get();
	}

	uint32_t GetDescriptorSize() const noexcept {
		return _descriptorSize;
	}

	D3D12_CPU_DESCRIPTOR_HANDLE GetCpuHandle(uint32_t offset) const noexcept;

	D3D12_GPU_DESCRIPTOR_HANDLE GetGpuHandle(uint32_t offset) const noexcept;

private:
	winrt::com_ptr<ID3D12DescriptorHeap> _heap;
	D3D12_CPU_DESCRIPTOR_HANDLE _cpuHandle{};
	D3D12_GPU_DESCRIPTOR_HANDLE _gpuHandle{};
	uint32_t _descriptorSize = 0;

	wil::srwlock _freeBlocksLock;

	// end(offset+size) -> size
	// 以 offset+size 作为键可以大大降低删除和插入键的频率
#ifdef _DEBUG
	// phmap::btree_map 没有 natvis，调试不方便
	std::map<uint32_t, uint32_t> _freeBlocks;
	// 用于断言
	uint32_t _capacity = 0;
#else
	phmap::btree_map<uint32_t, uint32_t> _freeBlocks;
#endif
};

}
