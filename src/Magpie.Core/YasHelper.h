#pragma once
#include "SmallVector.h"
// YAS 暂不支持 ARM64
// https://github.com/niXman/yas/pull/121
#ifdef _M_ARM64
#define _LITTLE_ENDIAN
#endif
#pragma warning(push)
// C4458: “size”的声明隐藏了类成员
// C4127: 条件表达式是常量
#pragma warning(disable: 4458 4127)
#include <yas/mem_streams.hpp>
#include <yas/binary_oarchive.hpp>
#include <yas/binary_iarchive.hpp>
#include <yas/types/std/pair.hpp>
#include <yas/types/std/string.hpp>
#include <yas/types/std/string_view.hpp>
#include <yas/types/std/vector.hpp>
#include <yas/types/std/variant.hpp>
#pragma warning(pop)

namespace yas::detail {

// 可平凡复制类型，注意不检查指针成员
template <size_t F, typename T>
struct serializer<
	type_prop::not_a_fundamental,
	ser_case::use_internal_serializer,
	F,
	T
> {
	template <typename Archive, typename = std::enable_if_t<std::is_trivially_copyable_v<T> && !std::is_pointer_v<T>, T>>
	static Archive& save(Archive& ar, const T& o) noexcept {
		ar.write(&o, sizeof(T));
		return ar;
	}

	template <typename Archive, typename = std::enable_if_t<std::is_trivially_copyable_v<T> && !std::is_pointer_v<T>, T>>
	static Archive& load(Archive& ar, T& o) noexcept {
		ar.read(&o, sizeof(T));
		return ar;
	}
};

// SmallVector
template <size_t F, typename T, unsigned N>
struct serializer<
	type_prop::not_a_fundamental,
	ser_case::use_internal_serializer,
	F,
	Magpie::SmallVector<T, N>
> {
	template <typename Archive>
	static Archive& save(Archive& ar, const Magpie::SmallVectorImpl<T>& vector) noexcept {
		return concepts::array::save<F>(ar, vector);
	}

	template <typename Archive>
	static Archive& load(Archive& ar, Magpie::SmallVectorImpl<T>& vector) noexcept {
		return concepts::array::load<F>(ar, vector);
	}
};

}

namespace Magpie {

struct SmallVectorOstream {
	SmallVectorOstream(SmallVectorImpl<uint8_t>& buffer) : _buffer(buffer) {}
	SmallVectorOstream(const SmallVectorOstream&) = delete;
	SmallVectorOstream(SmallVectorOstream&&) = default;

	template <typename T>
	std::size_t write(const T* ptr, std::size_t size) noexcept {
		const uint8_t* cptr = (const uint8_t*)ptr;
		_buffer.insert(_buffer.end(), cptr, cptr + size);
		return size;
	}

private:
	Magpie::SmallVectorImpl<uint8_t>& _buffer;
};

}

#ifdef _M_ARM64
#undef _LITTLE_ENDIAN
#endif
