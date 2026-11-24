#pragma once
#include "SmallVector.h"

namespace Magpie {

class EffectsMemPoolBuilder {
public:
	EffectsMemPoolBuilder(bool isResourceHeapTier2Supported, bool isResourceAliasingDisabled) noexcept;

	void BeginEffect() noexcept;

	uint32_t AllocResource(
		const D3D12_RESOURCE_ALLOCATION_INFO& allocInfo,
		bool isBuffer = false,
		bool isEffectOutput = false
	) noexcept;

	void DiscardResource(uint32_t resId) noexcept;

	void DiscardInput() noexcept;

	uint64_t GetCapacity(bool isBuffer = false) const noexcept;

	uint64_t GetAlignment(bool isBuffer = false) const noexcept;

	SmallVector<SmallVector<uint64_t>, 0> GetResourceOffsets() const noexcept;

private:
	struct _MemBlock {
		uint64_t offset;
		uint64_t size;

		uint64_t GetEnd() const noexcept {
			return offset + size;
		}

		// 按大小排序
		bool operator<(const _MemBlock& other) noexcept {
			return size < other.size;
		}
	};

	struct _MemPool {
		uint64_t capacity = 0;
		uint64_t alignment = 1;

		// 按大小排序的堆
		SmallVector<_MemBlock> freeBlocks;

		void DiscardAllExcept(const _MemBlock& block) noexcept;

		// 中间的空闲块扩张时会设置 originalBlockEnd 和 growSize
		_MemBlock AllocResource(
			const D3D12_RESOURCE_ALLOCATION_INFO& allocInfo,
			uint64_t& originalBlockEnd,
			uint64_t& growSize
		) noexcept;

		void DiscardResource(const _MemBlock& block) noexcept;

	private:
		void _AddFreeBlock(uint64_t offset, uint64_t size) noexcept;

		_MemBlock _AllocResourceAtBack(const D3D12_RESOURCE_ALLOCATION_INFO& allocInfo) noexcept;
	};

	const _MemPool& _GetMemPool(bool isBuffer) const noexcept;

	_MemPool& _GetMemPool(bool isBuffer) noexcept;

	_MemPool _defaultPool;
	// 如果显卡支持在一个堆中同时存储纹理和缓冲区则不使用这个成员
	_MemPool _bufferPool;

	// 每个效果的第一个资源的索引
	SmallVector<uint32_t> _effectResBaseIdxs;
	SmallVector<std::pair<_MemBlock, bool>> _resourceBlocks;
	uint32_t _lastOutputResIdx = UINT_MAX;
	uint32_t _curOutputResIdx = UINT_MAX;

	bool _isResourceHeapTier2Supported = false;
	bool _isResourceAliasingDisabled = false;
};

}
