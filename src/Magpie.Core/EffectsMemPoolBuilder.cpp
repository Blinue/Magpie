#include "pch.h"
#include "EffectsMemPoolBuilder.h"
#include "DirectXHelper.h"

namespace Magpie {

EffectsMemPoolBuilder::EffectsMemPoolBuilder(
	bool isResourceHeapTier2Supported,
	bool isResourceAliasingDisabled
) noexcept :
	_isResourceHeapTier2Supported(isResourceHeapTier2Supported),
	_isResourceAliasingDisabled(isResourceAliasingDisabled) {}

void EffectsMemPoolBuilder::BeginEffect() noexcept {
	_effectResBaseIdxs.push_back((uint32_t)_resourceBlocks.size());

	_lastOutputResIdx = _curOutputResIdx;
	_curOutputResIdx = UINT_MAX;

	if (!_isResourceAliasingDisabled && _lastOutputResIdx != UINT_MAX) {
		// 输入纹理外的资源全部释放
		_defaultPool.DiscardAllExcept(_resourceBlocks[_lastOutputResIdx].first);
	}
}

uint32_t EffectsMemPoolBuilder::AllocResource(
	const D3D12_RESOURCE_ALLOCATION_INFO& allocInfo,
	bool isBuffer,
	bool isEffectOutput
) noexcept {
	assert(allocInfo.SizeInBytes > 0);
	assert(DirectXHelper::IsPow2(allocInfo.Alignment));

	isBuffer = !_isResourceHeapTier2Supported && isBuffer;
	_MemPool& pool = isBuffer ? _bufferPool : _defaultPool;

	uint64_t originalBlockEnd = 0;
	uint64_t growSize = 0;
	_MemBlock resBlock = pool.AllocResource(allocInfo, originalBlockEnd, growSize);

	if (originalBlockEnd > 0) {
		// 后面的资源后移
		for (auto& pair : _resourceBlocks) {
			if (pair.second == isBuffer && pair.first.offset >= originalBlockEnd) {
				pair.first.offset += growSize;
			}
		}
	}

	uint32_t texResIdx = (uint32_t)_resourceBlocks.size();
	
	if (isEffectOutput) {
		_curOutputResIdx = texResIdx;
	}

	_resourceBlocks.push_back(std::make_pair(resBlock, isBuffer));
	return texResIdx;
}

void EffectsMemPoolBuilder::DiscardResource(uint32_t resId) noexcept {
	if (_isResourceAliasingDisabled) {
		return;
	}

	const auto& [block, isBuffer] = _resourceBlocks[resId];
	_GetMemPool(isBuffer).DiscardResource(block);
}

void EffectsMemPoolBuilder::DiscardInput() noexcept {
	// 如果输入来自捕获则无法释放内存
	if (_isResourceAliasingDisabled || _lastOutputResIdx == UINT_MAX) {
		return;
	}

	const auto& [block, isBuffer] = _resourceBlocks[_lastOutputResIdx];
	_GetMemPool(isBuffer).DiscardResource(block);
}

uint64_t EffectsMemPoolBuilder::GetCapacity(bool isBuffer) const noexcept {
	return _GetMemPool(isBuffer).capacity;
}

uint64_t EffectsMemPoolBuilder::GetAlignment(bool isBuffer) const noexcept {
	return _GetMemPool(isBuffer).alignment;
}

SmallVector<SmallVector<uint64_t>, 0> EffectsMemPoolBuilder::GetResourceOffsets() const noexcept {
	SmallVector<SmallVector<uint64_t>, 0> result(_effectResBaseIdxs.size());

	assert(_effectResBaseIdxs[0] == 0);
	uint64_t startIdx = 0;
	for (uint64_t i = 1; i < _effectResBaseIdxs.size(); ++i) {
		SmallVector<uint64_t>& effectResources = result[i - 1];

		uint64_t count = _effectResBaseIdxs[i] - startIdx;
		effectResources.resize(count);

		for (uint64_t j = 0; j < count; ++j) {
			effectResources[j] = _resourceBlocks[startIdx++].first.offset;
		}
	}

	// 剩余的资源是最后一个效果的
	SmallVector<uint64_t>& effectResources = result.back();
	uint64_t count = _resourceBlocks.size() - startIdx;
	effectResources.resize(count);

	for (uint64_t i = 0; i < count; ++i) {
		effectResources[i] = _resourceBlocks[startIdx++].first.offset;
	}

	return result;
}

const EffectsMemPoolBuilder::_MemPool& EffectsMemPoolBuilder::_GetMemPool(bool isBuffer) const noexcept {
	return ((EffectsMemPoolBuilder*)this)->_GetMemPool(isBuffer);
}

EffectsMemPoolBuilder::_MemPool& EffectsMemPoolBuilder::_GetMemPool(bool isBuffer) noexcept {
	return (_isResourceHeapTier2Supported || !isBuffer) ? _defaultPool : _bufferPool;
}

void EffectsMemPoolBuilder::_MemPool::DiscardAllExcept(const _MemBlock& block) noexcept {
	freeBlocks.clear();

	if (block.offset > 0) {
		_AddFreeBlock(0, block.offset);
	}

	uint64_t inputBlockEnd = block.GetEnd();
	if (inputBlockEnd < capacity) {
		_AddFreeBlock(inputBlockEnd, capacity - inputBlockEnd);
	}
}

// 和通常的内存分配算法不同，这个方法用于计算内存分配的**计划**，因此可以自由改变已经
// 分配的资源的偏移量。分配时每次取出最大的空闲块，如果该空闲块不够大则将其扩容，并将
// 将后面的资源后移，这使得每次分配时内存池尺寸的增长最小。这个算法可能不会得到全局最
// 优的结果，但非常快。
EffectsMemPoolBuilder::_MemBlock EffectsMemPoolBuilder::_MemPool::AllocResource(
	const D3D12_RESOURCE_ALLOCATION_INFO& allocInfo,
	uint64_t& originalBlockEnd,
	uint64_t& growSize
) noexcept {
	alignment = std::max(alignment, allocInfo.Alignment);

	if (freeBlocks.empty()) {
		return _AllocResourceAtBack(allocInfo);
	}

	// 取出最大的空闲块
	std::pop_heap(freeBlocks.begin(), freeBlocks.end());
	const _MemBlock& curFreeBlock = freeBlocks.back();
	uint64_t curFreeBlockEnd = curFreeBlock.GetEnd();

	uint64_t resOffset = DirectXHelper::Align(curFreeBlock.offset, allocInfo.Alignment);
	uint64_t resEnd = resOffset + allocInfo.SizeInBytes;

	if (curFreeBlockEnd <= resOffset) {
		// 当前空闲块是对齐导致的零碎块，不值得使用。放回当前空闲块
		std::push_heap(freeBlocks.begin(), freeBlocks.end());
		return _AllocResourceAtBack(allocInfo);
	}

	freeBlocks.pop_back();

	if (resOffset > curFreeBlock.offset) {
		// 对齐导致的零碎快
		_AddFreeBlock(curFreeBlock.offset, resOffset - curFreeBlock.offset);
	}

	if (curFreeBlockEnd >= resEnd) {
		// 当前空闲块足够大
		if (curFreeBlockEnd > resEnd) {
			_AddFreeBlock(resEnd, curFreeBlockEnd - resEnd);
		}
	} else if (curFreeBlockEnd == capacity) {
		// 当前空闲块虽然不够大但在内存池的末尾，可以直接扩容
		capacity = resEnd;
	} else {
		// 扩容当前空闲块。确保扩容的字节数是 _alignment 的倍数来维护后面资源的对齐
		originalBlockEnd = curFreeBlockEnd;
		growSize = DirectXHelper::Align(resEnd - curFreeBlockEnd, alignment);

		capacity += growSize;

		// 后面的空闲块后移
		for (_MemBlock& block : freeBlocks) {
			if (block.offset >= originalBlockEnd) {
				block.offset += growSize;
			}
		}

		uint64_t newEnd = curFreeBlockEnd + growSize;
		if (newEnd > resEnd) {
			// 对齐导致的零碎快
			_AddFreeBlock(resEnd, newEnd - resEnd);
		}
	}

	return { resOffset, allocInfo.SizeInBytes };
}

// 相邻的空闲块将被合并。由于 freeBlocks 是按大小排序的堆，寻找相邻的空闲块需要两次
// 遍历。这涉及到一个权衡: 分配资源时需要取出最大的空闲块，而释放资源时需要寻找相邻
// 的空闲块。有两个数据结构候选: 按偏移量排序的 B-树和按大小排序的堆。考虑到空闲块一
// 般很少，常数项的影响更大，因此选择堆。
void EffectsMemPoolBuilder::_MemPool::DiscardResource(const _MemBlock& block) noexcept {
	uint64_t resEnd = block.GetEnd();

	// 寻找相邻的空闲块
	uint32_t prevFreeBlockIdx = UINT_MAX;
	uint32_t nextFreeBlockIdx = UINT_MAX;
	uint32_t freeBlockCount = (uint32_t)freeBlocks.size();

	for (uint32_t i = 0; i < freeBlockCount; ++i) {
		if (freeBlocks[i].GetEnd() == block.offset) {
			prevFreeBlockIdx = i;
			break;
		}
	}

	for (uint32_t i = 0; i < freeBlockCount; ++i) {
		if (freeBlocks[i].offset == resEnd) {
			nextFreeBlockIdx = i;
			break;
		}
	}

	bool canMergePrev = prevFreeBlockIdx != UINT_MAX;
	bool canMergeNext = nextFreeBlockIdx != UINT_MAX;

	if (canMergeNext) {
		if (canMergePrev) {
			_MemBlock mergedBlock = {
				freeBlocks[prevFreeBlockIdx].offset,
				freeBlocks[prevFreeBlockIdx].size + block.size + freeBlocks[nextFreeBlockIdx].size
			};

			// 删除索引更大的空闲块，更改索引更小的空闲块
			uint32_t smallIdx = std::min(prevFreeBlockIdx, nextFreeBlockIdx);
			uint32_t largeIdx = std::max(prevFreeBlockIdx, nextFreeBlockIdx);

			freeBlocks.erase(freeBlocks.begin() + largeIdx);
			freeBlocks[smallIdx] = mergedBlock;
		} else {
			freeBlocks[nextFreeBlockIdx] = {
				block.offset,
				block.size + freeBlocks[nextFreeBlockIdx].size
			};
		}

		std::make_heap(freeBlocks.begin(), freeBlocks.end());
	} else {
		if (canMergePrev) {
			freeBlocks[prevFreeBlockIdx].size += block.size;
			std::make_heap(freeBlocks.begin(), freeBlocks.end());
		} else {
			_AddFreeBlock(block.offset, block.size);
		}
	}
}

void EffectsMemPoolBuilder::_MemPool::_AddFreeBlock(uint64_t offset, uint64_t size) noexcept {
	freeBlocks.emplace_back(offset, size);
	std::push_heap(freeBlocks.begin(), freeBlocks.end());
}

EffectsMemPoolBuilder::_MemBlock EffectsMemPoolBuilder::_MemPool::_AllocResourceAtBack(
	const D3D12_RESOURCE_ALLOCATION_INFO& allocInfo
) noexcept {
	uint64_t resOffset = DirectXHelper::Align(capacity, allocInfo.Alignment);

	if (resOffset > capacity) {
		// 对齐导致的零碎快
		_AddFreeBlock(capacity, resOffset - capacity);
	}

	capacity = resOffset + allocInfo.SizeInBytes;
	return { resOffset, allocInfo.SizeInBytes };
}

#ifdef _DEBUG
static Ignore _ = [] {
	EffectsMemPoolBuilder builder(true, false);

	builder.BeginEffect();

	uint32_t id1 = builder.AllocResource({ 1,1 });
	uint32_t id2 = builder.AllocResource({ 3,1 });
	builder.AllocResource({ 2,1 });
	builder.DiscardResource(id1);
	builder.DiscardResource(id2);
	builder.AllocResource({ 5,1 });
	builder.AllocResource({ 1,1 });

	assert(builder.GetCapacity() == 8);
	auto offsets = builder.GetResourceOffsets();
	SmallVector<uint64_t> expectedOffsets{ 0,1,5,0,7 };
	assert(offsets.size() == 1 && offsets[0] == expectedOffsets);

	return Ignore();
}();
#endif

}
