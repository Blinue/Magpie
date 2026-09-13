#pragma once

namespace Magpie {

// 申请的内存都已置零
class ByteBuffer {
public:
	ByteBuffer() = default;
	ByteBuffer(const ByteBuffer&) = delete;
	ByteBuffer(ByteBuffer&&) = default;

	ByteBuffer& operator=(const ByteBuffer&) = delete;
	ByteBuffer& operator=(ByteBuffer&&) = default;

#ifdef _DEBUG
	explicit ByteBuffer(uint32_t size) : _data(size) {
		assert(size > 0);
	}

	uint8_t& operator[](uint32_t index) noexcept {
		assert(index < _data.size());
		return _data[index];
	}

	const uint8_t& operator[](uint32_t index) const noexcept {
		assert(index < _data.size());
		return _data[index];
	}

	operator bool() const noexcept {
		return !_data.empty();
	}

	void Resize(uint32_t size) noexcept {
		assert(size > 0);
		_data.resize(size);
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
	std::vector<uint8_t> _data;
#else
	explicit ByteBuffer(uint32_t size) : _data(std::make_unique<uint8_t[]>(size)) {
		assert(size > 0);
	}

	uint8_t& operator[](uint32_t index) noexcept {
		return _data.get()[index];
	}

	const uint8_t& operator[](uint32_t index) const noexcept {
		return _data.get()[index];
	}

	operator bool() const noexcept {
		return (bool)_data;
	}

	void Resize(uint32_t size) noexcept {
		assert(size > 0);
		_data = std::make_unique<uint8_t[]>(size);
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
