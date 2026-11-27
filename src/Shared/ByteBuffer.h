#pragma once
#ifdef _DEBUG
#include "SmallVector.h"
#endif

namespace Magpie {

// 申请的内存未初始化
class ByteBuffer {
public:
	ByteBuffer() = default;
	ByteBuffer(const ByteBuffer&) = delete;
	ByteBuffer(ByteBuffer&&) = default;

	ByteBuffer& operator=(const ByteBuffer&) = delete;
	ByteBuffer& operator=(ByteBuffer&&) = default;

#ifdef _DEBUG
	// 第二个参数用于提醒调用者申请的内存没有置零
	ByteBuffer(uint64_t size, for_overwrite_t) : _data(size, for_overwrite) {
		assert(size > 0);
	}

	uint8_t& operator[](uint64_t index) noexcept {
		assert(index < _data.size());
		return _data[index];
	}

	const uint8_t& operator[](uint64_t index) const noexcept {
		assert(index < _data.size());
		return _data[index];
	}

	operator bool() const noexcept {
		return !_data.empty();
	}

	void Reset(uint64_t size, for_overwrite_t) noexcept {
		assert(size > 0);
		_data.resize_for_overwrite(size);
	}

	void Clear() noexcept {
		_data.clear();
	}

	uint8_t* Data() noexcept {
		return _data.data();
	}

	const uint8_t* Data() const noexcept {
		return _data.data();
	}
private:
	SmallVector<uint8_t, 0> _data;
#else
	// 第二个参数用于提醒调用者申请的内存没有置零
	ByteBuffer(uint64_t size, for_overwrite_t) : _data(std::make_unique_for_overwrite<uint8_t[]>(size)) {
		assert(size > 0);
	}

	uint8_t& operator[](uint64_t index) noexcept {
		return _data[index];
	}

	const uint8_t& operator[](uint64_t index) const noexcept {
		return _data[index];
	}

	operator bool() const noexcept {
		return (bool)_data;
	}

	void Reset(uint64_t size, for_overwrite_t) noexcept {
		assert(size > 0);
		_data = std::make_unique_for_overwrite<uint8_t[]>(size);
	}

	void Clear() noexcept {
		_data.reset();
	}

	uint8_t* Data() noexcept {
		return _data.get();
	}

	const uint8_t* Data() const noexcept {
		return _data.get();
	}
private:
	std::unique_ptr<uint8_t[]> _data;
#endif
};

}
